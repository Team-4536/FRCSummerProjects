
import rev
import wpimath.system.plant as plant
import math

from virtualGyro import VirtualGyro
from real import V2f, angleWrap
from encoderSim import EncoderSim
from PIDController import PIDController

import wpimath.estimator
import wpimath.kinematics
from wpimath.kinematics import SwerveModulePosition;
from wpimath.geometry import Translation2d, Pose2d, Rotation2d

class SwerveState:
    def __init__(self, maxSpeed: float, wheelOffsets: list[V2f], wheelRadius: float, driveMotors, steerMotors, driveEncoders, steerEncoders) -> None:

        self.driveMotors: list[rev.CANSparkMax] = driveMotors
        self.steerMotors: list[rev.CANSparkMax] = steerMotors
        self.driveEncoders: list[rev.RelativeEncoder] = driveEncoders
        self.steerEncoders: list[rev.RelativeEncoder] = steerEncoders

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
        self.brakes = 1

        #choose between brake or hold position when no input is given (if false brake will be a toggle on button "A")
        self.brakeDefault = False

        kp = 2
        ki = 0
        kd = 12
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

        for i in range(4):
            tv = turningVectors[i].getNormalized() * (turningInRads * swerve.wheelOffsets[i].getLength())
            vec = speed + tv
            wheelSpeed = vec.getLength()

            if(wheelSpeed == 0):
                swerve.steerMotors[i].set(0)
                swerve.driveMotors[i].set(0)
                continue

            error = angleWrap(vec.getAngle() - swerve.steerEncoders[i].getPosition()*360)
            if(abs(error) > 90):
                error = error - 180
                wheelSpeed *= -1

            swerve.steerMotors[i].set(self.pids[i].tickErr(angleWrap(error)/360, dt))
            swerve.driveMotors[i].set(wheelSpeed / swerve.maxSpeed)






    # forward = forward/back
    # right = Left/Right
    # turning is CW+
    def tick(self, forward: float, right: float, turn: float, dt: float, brakeButtonPressed, swerve: SwerveState) -> None:
        #brake input toggle
        if brakeButtonPressed == True:
            self.brakes = self.brakes * -1

        #assign inputs to vectors
        leftStick = V2f(forward, right)

        FLTurningVector = V2f(math.cos(45) * turn, math.cos(45) * turn)
        FRTurningVector = V2f(math.cos(45) * turn, -math.cos(45) * turn)
        BLTurningVector = V2f(-math.cos(45) * turn, math.cos(45) * turn)
        BRTurningVector = V2f(-math.cos(45) * turn, -math.cos(45) * turn)

        #joystick scalar (adjusts speed depending on how far you move the joysticks)
        inputScalar = leftStick.getLength() + abs(turn)
        if inputScalar > 1:
            inputScalar = 1

        FLVector = (leftStick + FLTurningVector) / 2
        FRVector = (leftStick + FRTurningVector) / 2
        BLVector = (leftStick + BLTurningVector) / 2
        BRVector = (leftStick + BRTurningVector) / 2

        """-----------------------------------------"""

        #read encoder positions and set them angles
        FLPosAngle = (swerve.steerEncoders[0].getPosition() % 1) * 360
        FRPosAngle = (swerve.steerEncoders[1].getPosition() % 1) * 360
        BLPosAngle = (swerve.steerEncoders[2].getPosition() % 1) * 360
        BRPosAngle = (swerve.steerEncoders[3].getPosition() % 1) * 360

        #wrap angles (0, 90, 180, -90)
        if FLPosAngle > 180:
            FLPosAngle = FLPosAngle - 360
        if FRPosAngle > 180:
            FRPosAngle = FRPosAngle - 360
        if BLPosAngle > 180:
            BLPosAngle = BLPosAngle - 360
        if BRPosAngle > 180:
            BRPosAngle = BRPosAngle - 360

        """----------------------------------------------------"""

        #separate vector values
        FLTarget = FLVector.getAngle()
        FRTarget = FRVector.getAngle()
        BLTarget = BLVector.getAngle()
        BRTarget = BRVector.getAngle()

        FLPower = FLVector.getLength()
        FRPower = FRVector.getLength()
        BLPower = BLVector.getLength()
        BRPower = BRVector.getLength()

        #set brakes
        if self.brakeDefault == False:
            if self.brakes == -1:
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

        def getSteeringError(Target, PosAngle, Power):
            # ???????????????????????????????????
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

        FLSteeringError, FLPower = getSteeringError(FLTarget, FLPosAngle, FLPower)
        FRSteeringError, FRPower = getSteeringError(FRTarget, FRPosAngle, FRPower)
        BLSteeringError, BLPower = getSteeringError(BLTarget, BLPosAngle, BLPower)
        BRSteeringError, BRPower = getSteeringError(BRTarget, BRPosAngle, BRPower)

        #hold position if no input is given
        if self.brakeDefault == False:
            if FLVector == V2f() and self.brakes != -1:
                FLSteeringError = 0
            if FRVector == V2f() and self.brakes != -1:
                FRSteeringError = 0
            if BLVector == V2f() and self.brakes != -1:
                BLSteeringError  = 0
            if BRVector == V2f() and self.brakes != -1:
                BRSteeringError = 0

        #assign motor powers
        FLSteeringPower = self.FLPID.tickErr(angleWrap(FLSteeringError / 360), dt)
        FRSteeringPower = self.FRPID.tickErr(angleWrap(FRSteeringError / 360), dt)
        BLSteeringPower = self.BLPID.tickErr(angleWrap(BLSteeringError / 360), dt)
        BRSteeringPower = self.BRPID.tickErr(angleWrap(BRSteeringError / 360), dt)

        """--------------------------------------------------"""

        #scalars
        maxPowerInput = 0
        maxSteeringInput = 0

        #power scalar
        #FL vs FR
        if abs(FLPower) > abs(FRPower):
            maxPowerInput = abs(FLPower)
        else:
            maxPowerInput = abs(FLPower)
        #BL
        if abs(BLPower) > maxPowerInput:
            maxPowerInput = abs(BLPower)
        #BR
        if abs(BRPower) > maxPowerInput:
            maxPowerInput = abs(BRPower)

        #set scalar value
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

        #set scalar value
        if maxSteeringInput == 0:
            steeringScalar = 1
        elif maxSteeringInput > 1:
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
        swerve.driveMotors[1].set(FRPower)
        swerve.driveMotors[2].set(BLPower)
        swerve.driveMotors[3].set(BRPower)

        #target steering values (in rotaions)
        swerve.steerMotors[0].set(FLSteeringPower)
        swerve.steerMotors[1].set(FRSteeringPower)
        swerve.steerMotors[2].set(BLSteeringPower)
        swerve.steerMotors[3].set(BRSteeringPower)