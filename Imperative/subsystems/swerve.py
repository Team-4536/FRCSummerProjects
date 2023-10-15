
import rev
import wpimath.system.plant as plant
import math

from virtualGyro import VirtualGyro
from real import V2f, angleWrap, normalizeWheelSpeeds
from encoderSim import EncoderSim
from PIDController import PIDController

from socketing import Server

import wpimath.estimator
import wpimath.kinematics
from wpimath.kinematics import SwerveModulePosition;
from wpimath.geometry import Translation2d, Pose2d, Rotation2d
import ctre.sensors

class SwerveState:
    def __init__(self, maxSpeed: float, wheelOffsets: list[V2f], wheelRadius: float, driveMotors, steerMotors, driveEncoders, steerEncoders) -> None:

        self.driveMotors: list[rev.CANSparkMax] = driveMotors
        self.steerMotors: list[rev.CANSparkMax] = steerMotors
        self.driveEncoders: list[rev.RelativeEncoder] = driveEncoders
        self.steerEncoders: list[ctre.sensors.CANCoder] = steerEncoders

        self.maxSpeed = maxSpeed
        self.wheelRadius = wheelRadius
        self.wheelCirc = wheelRadius * 2 * math.pi
        self.wheelOffsets: list[V2f] = wheelOffsets
        wheelTranslations = [Translation2d(w.x, w.y) for w in wheelOffsets]

        self.wheelStates = (
            SwerveModulePosition(0, Rotation2d(0)),
            SwerveModulePosition(0, Rotation2d(0)),
            SwerveModulePosition(0, Rotation2d(0)),
            SwerveModulePosition(0, Rotation2d(0))
        )
        kine = wpimath.kinematics.SwerveDrive4Kinematics(*wheelTranslations)
        self.estimatedPosition = V2f(0, 0)
        self.est = wpimath.estimator.SwerveDrive4PoseEstimator(
            kine,
            Rotation2d(0),
            self.wheelStates,
            Pose2d(Translation2d(self.estimatedPosition.x, self.estimatedPosition.y), Rotation2d())
            )

    # TODO: reset states button to set conditions inside of WPI items reset PIDs
    # TODO: reset button from sundial

    # angle is in degs CW+
    def updateEstimation(self, currentTime: float, currentAngle) -> None:
        r = Rotation2d(math.radians(-currentAngle)) # wpi uses CCW angles

        for i in range(0, 4):
            self.wheelStates[i].angle = Rotation2d(-(self.steerEncoders[i].getPosition() * 2 * math.pi))
            self.wheelStates[i].distance = self.driveEncoders[i].getPosition() * self.wheelCirc

        nPose = self.est.updateWithTime(currentTime, r, self.wheelStates)
        self.estimatedPosition = V2f(nPose.X(), nPose.Y())








class SwerveSim:
    def __init__(self) -> None:
        self.driveSims: list[EncoderSim] = [
            EncoderSim(plant.DCMotor.NEO(1), 0.005, 6.12) for i in range(4)
            ]

        # TODO: get real inertia vals
        # TODO: is this the corect gearing?
        self.steerSims: list[EncoderSim] = [
            EncoderSim(plant.DCMotor.NEO(1), 0.001, 1) for i in range(4)
            ]

        self.position = V2f(0, 0)
        self.rotation: float = 0.0

    # TODO: add real friction to wheel sims
    # NOTE: updates hardware given inside of swerveState
    def update(self, dt: float, gyro: VirtualGyro, swerve: SwerveState):
        posDeltas: list[V2f] = []

        for i in range(0, 4):
            sim = self.driveSims[i]
            sim.update(dt, swerve.driveMotors[i], swerve.driveEncoders[i])
            driveDelta = (sim.state[1] / (2 * math.pi)) * dt * swerve.wheelCirc # delta in meters
            sim.state[1] *= 0.9 # CLEANUP: friction hack

            sim = self.steerSims[i]
            sim.update(dt, swerve.steerMotors[i], swerve.steerEncoders[i])
            steerPos = (sim.state[0] / (2 * math.pi)) * 360 # position in degrees
            sim.state[1] *= 0.5

            posDeltas.append((V2f(1, 0) * driveDelta).rotateDegrees(steerPos))

        posChange = (posDeltas[0] + posDeltas[1] + posDeltas[2] + posDeltas[3]) / 4
        self.position += posChange.rotateDegrees(self.rotation)

        # TODO: right now position and angle are beign updated via averaging wheel deltas. Find out if this is correct

        angleDeltas: list[float] = [ ]
        for i in range(0, 4):
            inital = swerve.wheelOffsets[i]
            new = inital + posDeltas[i]
            angleDeltas.append(angleWrap(new.getAngle() - inital.getAngle()))

        self.rotation += (angleDeltas[0] + angleDeltas[1] + angleDeltas[2] + angleDeltas[3]) / 4
        gyro.setYaw(self.rotation)












# TODO: change brake mode from toggle to happen until input received
class SwerveController:
    def __init__(self) -> None:
        self.brakes = False

        #choose between brake or hold position when no input is given (if false brake will be a toggle on button "A")
        self.brakeDefault = False

        kp = 2
        ki = 0
        kd = 0
        self.FLPID = PIDController(kp, ki, kd)
        self.FRPID = PIDController(kp, ki, kd)
        self.BLPID = PIDController(kp, ki, kd)
        self.BRPID = PIDController(kp, ki, kd)

        self.pids = [PIDController(kp, ki, kd) for i in range(4)]


    # X component is forward+ in M/s, y is left+, turning is CW+ in degs/sec
    def tickReal(self, speed: V2f, turning: float, dt: float, swerve: SwerveState, server) -> None:

        turningInRads = math.radians(turning)
        turningVectors = [
            V2f(1, -1),
            V2f(-1, -1),
            V2f(1, 1),
            V2f(-1, 1)
        ]

        driveSpeeds = [ ]
        for i in range(4):
            tv = turningVectors[i].getNormalized() * (turningInRads * swerve.wheelOffsets[i].getLength())
            vec = speed + tv
            wheelSpeed = vec.getLength()

            if(wheelSpeed == 0):
                swerve.steerMotors[i].set(0)
                swerve.driveMotors[i].set(0)
                continue

            error = angleWrap(vec.getAngle() - swerve.steerEncoders[i].getPosition())
            if(abs(error) > 90):
                error = error - 180
                wheelSpeed *= -1

            if(i == 0):
                server.putUpdate("target", float(vec.getAngle()))
            swerve.steerMotors[i].set(self.pids[i].tickErr(angleWrap(error)/360, dt))
            driveSpeeds.append(wheelSpeed / swerve.maxSpeed)

        driveSpeeds = normalizeWheelSpeeds(driveSpeeds)
        for x in zip(driveSpeeds, swerve.driveMotors):
            x[1].set(x[0])





    # forward = forward/back
    # right = Left/Right
    # turning is CW+
    def tick(self, forward: float, right: float, turn: float, dt: float, brakeButtonPressed, gyroReset, swerve: SwerveState, gyro) -> None:
        #brake input toggle
        if brakeButtonPressed == True:
            self.brakes = not self.brakes

        if gyroReset == True:
            gyroReset()

        inputGyro = gyro.getYaw()

        #assign inputs to vectors
        leftStick = V2f(forward, right)
        leftStick = leftStick.rotateDegrees(inputGyro)

        FLTurningVector = V2f(math.cos(45) * turn, math.cos(45) * turn)
        FRTurningVector = V2f(math.cos(45) * turn, -math.cos(45) * turn)
        BLTurningVector = V2f(-math.cos(45) * turn, math.cos(45) * turn)
        BRTurningVector = V2f(-math.cos(45) * turn, -math.cos(45) * turn)

        #joystick scalar (adjusts speed depending on how far you move the joysticks)
        inputScalar = leftStick.getLength() + abs(turn)
        if inputScalar > 1:
            inputScalar = 1

        FLVector = (leftStick + FLTurningVector) / 2
        FRVector = (leftStick + -FRTurningVector) / 2
        BLVector = (leftStick + -BLTurningVector) / 2
        BRVector = (leftStick + BRTurningVector) / 2

        """-----------------------------------------"""

        #Assign encoders and make sure they're measuring in degrees

        #if steer encoders return rotations
        """
        FLPosAngle = (swerve.steerEncoders[0].getPosition() % 1) * 360
        FRPosAngle = (swerve.steerEncoders[1].getPosition() % 1) * 360
        BLPosAngle = (swerve.steerEncoders[2].getPosition() % 1) * 360
        BRPosAngle = (swerve.steerEncoders[3].getPosition() % 1) * 360
        """

        #if encoders return angle (0-360 degrees)
        FLPosAngle = (swerve.steerEncoders[0].getAbsolutePosition())
        FRPosAngle = (swerve.steerEncoders[1].getAbsolutePosition())
        BLPosAngle = (swerve.steerEncoders[2].getAbsolutePosition())
        BRPosAngle = (swerve.steerEncoders[3].getAbsolutePosition())

        #wrap angles to (0, 90, 180, -90) instead of (0, 90, 180, 270)
        while FLPosAngle > 180:
            FLPosAngle = FLPosAngle - 360
        while FRPosAngle > 180:
            FRPosAngle = FRPosAngle - 360
        while BLPosAngle > 180:
            BLPosAngle = BLPosAngle - 360
        while BRPosAngle > 180:
            BRPosAngle = BRPosAngle - 360

        """----------------------------------------------------"""

        #separate vector values
        #getAngle returns 0 to 360 degrees
        FLTarget = FLVector.getAngle()
        FRTarget = FRVector.getAngle()
        BLTarget = BLVector.getAngle()
        BRTarget = BRVector.getAngle()

        Server.inst.putUpdate("FLTarget", FLTarget)

        Server.inst.putUpdate("GyroYaw", inputGyro)

        #getLength returns power value from 0 to 1
        FLPower = FLVector.getLength()
        FRPower = FRVector.getLength()
        BLPower = BLVector.getLength()
        BRPower = BRVector.getLength()

        Server.inst.putUpdate("Brakes", self.brakes)

        #set brakes
        if self.brakeDefault == False:
            if self.brakes == True:
                FLTarget = 135
                FRTarget = 225
                BLTarget = 45
                BRTarget = 315
                FLPower = 0
                FRPower = 0
                BLPower = 0
                BRPower = 0
        else:
            if forward == 0 and right == 0 and turn == 0:
                FLTarget = 135
                FRTarget = 225
                BLTarget = 45
                BRTarget = 315

        """---------------------------------------"""

        #calculate error

        """
        def getSteeringError(Target, PosAngle, Power):
            # ???????????????????????????????????
            # just trust the process rob
            FakeError = -Target - PosAngle
            Pos = -(FakeError)
            while Pos > 90:
                Pos = Pos - 180
                Power = -(Power)
            SteeringError = -Pos
            while SteeringError > 90:
                SteeringError = SteeringError - 180
                Power = -(Power)
            if SteeringError > 180:
                SteeringError = SteeringError - 360

            return SteeringError, Power
        """

        #attept at making that less confusing (probably works)
        def getBetterSteeringError(target, pos, power):
            error = target - pos

            while error < -180:
                error = error + 360
            while error > 180:
                error = error - 360

            #calculate shortest path to target (module should never move more than 90 degrees at once)
            
            if error > 90:
                error = error - 180
                power = -power
            if error < -90:
                error = error + 180
                power = -power
            
            
            return error, power

        #actually call the function to find error for each module
        FLSteeringError, FLPower = getBetterSteeringError(FLTarget, FLPosAngle, FLPower)
        FRSteeringError, FRPower = getBetterSteeringError(FRTarget, FRPosAngle, FRPower)
        BLSteeringError, BLPower = getBetterSteeringError(BLTarget, BLPosAngle, BLPower)
        BRSteeringError, BRPower = getBetterSteeringError(BRTarget, BRPosAngle, BRPower)

        Server.inst.putUpdate("FLSteeringError", FLSteeringError)

        #hold position if no input is given
        if self.brakeDefault == False:
            if FLVector == V2f() and self.brakes == False:
                FLSteeringError = 0
            if FRVector == V2f() and self.brakes == False:
                FRSteeringError = 0
            if BLVector == V2f() and self.brakes == False:
                BLSteeringError  = 0
            if BRVector == V2f() and self.brakes == False:
                BRSteeringError = 0

        #assign motor powers
        FLSteeringPower = self.FLPID.tickErr(angleWrap(FLSteeringError)/360, dt)
        FRSteeringPower = self.FRPID.tickErr(angleWrap(FRSteeringError)/360, dt)
        BLSteeringPower = self.BLPID.tickErr(angleWrap(BLSteeringError)/360, dt)
        BRSteeringPower = self.BRPID.tickErr(angleWrap(BRSteeringError)/360, dt)

        """--------------------------------------------------"""

        #scalars
        maxPowerInput = 0
        maxSteeringInput = 0

        #power scalar
        #FL vs FR
        if abs(FLPower) > abs(FRPower):
            maxPowerInput = abs(FLPower)
        else:
            maxPowerInput = abs(FRPower)
        #BL
        if abs(BLPower) > maxPowerInput:
            maxPowerInput = abs(BLPower)
        #BR
        if abs(BRPower) > maxPowerInput:
            maxPowerInput = abs(BRPower)

        #set power scalar value
        if maxPowerInput == 0:
            powerScalar = 1
        else:
            powerScalar = 1 / maxPowerInput


        #steering scalar
        #FL vs FR
        if abs(FLSteeringPower) > abs(FRSteeringPower):
            maxSteeringInput = abs(FLSteeringPower)
        else:
            maxSteeringInput = abs(FRSteeringPower)
        #BL
        if abs(BLSteeringPower) > maxSteeringInput:
            maxSteeringInput = abs(BLSteeringPower)
        #BR
        if abs(BRSteeringPower) > maxSteeringInput:
            maxSteeringInput = abs(BRSteeringPower)

        #set steering scalar value
        if maxSteeringInput > 1:
            steeringScalar = 1 / maxSteeringInput
        else:
            steeringScalar = 1
        """============================================="""

        #final scaled motor speeds
        FLPower = (FLPower * powerScalar) * inputScalar
        FRPower = (FRPower * powerScalar) * inputScalar
        BLPower = (BLPower * powerScalar) * inputScalar
        BRPower = (BRPower * powerScalar) * inputScalar

        FLSteeringPower = FLSteeringPower * steeringScalar
        FRSteeringPower = FRSteeringPower * steeringScalar
        BLSteeringPower = BLSteeringPower * steeringScalar
        BRSteeringPower = BRSteeringPower * steeringScalar

        """============================================="""

        
        #drive values (power)
        swerve.driveMotors[0].set(FLPower)
        swerve.driveMotors[1].set(-FRPower)
        swerve.driveMotors[2].set(BLPower)
        swerve.driveMotors[3].set(-BRPower)

        #steering power (NOT target position)
        swerve.steerMotors[0].set(-FLSteeringPower)
        swerve.steerMotors[1].set(-FRSteeringPower)
        swerve.steerMotors[2].set(-BLSteeringPower)
        swerve.steerMotors[3].set(-BRSteeringPower)
        