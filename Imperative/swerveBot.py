import wpilib
import rev
import ntcore
import math
import navx

import wpimath.system.plant as plant
from wpimath.geometry import Pose2d, Rotation2d
from wpimath.kinematics import ChassisSpeeds
from wpimath.controller import RamseteController

from real import V2f, angleWrap
import socketing
import timing
from inputs import deadZone
from virtualGyro import VirtualGyro
from paths import getSpline2dSample, getLinear2dSample

from subsystems.swerve import SwerveState, SwerveController, SwerveSim


WHEEL_DIA = 0.1016 # 4 in. in meters
WHEEL_RADIUS = WHEEL_DIA / 2
WHEEL_CIRC = WHEEL_DIA * math.pi

class SwerveBotInputs():
    def __init__(self, driveCtrlr: wpilib.XboxController, armCtrlr: wpilib.XboxController) -> None:
        self.driveX = deadZone(driveCtrlr.getLeftX())
        self.driveY = deadZone((-driveCtrlr.getLeftY()))
        self.turning = deadZone(armCtrlr.getLeftX())


class SwerveBot(wpilib.TimedRobot):

    # TODO: assert saftey

    def robotInit(self) -> None:

        self.server = socketing.Server(self.isReal())
        self.driveCtrlr = wpilib.XboxController(0)
        self.armCtrlr = wpilib.XboxController(1)
        self.time = timing.TimeData(None)
        self.gyro = VirtualGyro()
        # self.gyro = navx.AHRS(wpilib.SPI.Port.kMXP)

        driveType = rev.CANSparkMax.MotorType.kBrushless
        driveMotors = [
            rev.CANSparkMax(0, driveType),
            rev.CANSparkMax(1, driveType),
            rev.CANSparkMax(2, driveType),
            rev.CANSparkMax(3, driveType)
        ]
        driveEncoders: list[rev.RelativeEncoder] = [x.getEncoder() for x in driveMotors]
        steerMotors = [
            rev.CANSparkMax(4, driveType),
            rev.CANSparkMax(5, driveType),
            rev.CANSparkMax(6, driveType),
            rev.CANSparkMax(7, driveType)
        ]
        steerEncoders: list[rev.RelativeEncoder] = [x.getEncoder() for x in steerMotors]

        # NOTE: X+ is forward, so FL should be up and right in world coords
        wheelPositions = [ V2f(1, 1), V2f(1, -1), V2f(-1, 1), V2f(-1, -1) ]
        wheelPositions = [p.getNormalized() * 0.5 for p in wheelPositions]
        self.swerve = SwerveState(4.2, wheelPositions, WHEEL_RADIUS, driveMotors, steerMotors, driveEncoders, steerEncoders)
        self.swerveController = SwerveController()

        self.prevPos = self.swerve.estimatedPosition
        self.prevAngle = self.gyro.getYaw()

    def robotPeriodic(self) -> None:

        self.time = timing.TimeData(self.time)


        self.server.putUpdate("time", self.time.timeSinceInit)

        prefs = ["FL", "FR", "BL", "BR"]
        for i in range(0, 4):
            self.server.putUpdate(prefs[i] + "DriveSpeed", self.swerve.driveMotors[i].get())
            self.server.putUpdate(prefs[i] + "DrivePos", self.swerve.driveEncoders[i].getPosition())
            self.server.putUpdate(prefs[i] + "SteerSpeed", self.swerve.steerMotors[i].get())
            self.server.putUpdate(prefs[i] + "SteerPos", self.swerve.steerEncoders[i].getPosition())

        self.server.putUpdate("posX", float(self.sim.position.x))
        self.server.putUpdate("posY", float(self.sim.position.y))
        self.server.putUpdate("yaw", self.gyro.getYaw())

        self.swerve.updateEstimation(self.time.timeSinceInit, self.gyro.getYaw())
        self.server.putUpdate("estX", self.swerve.estimatedPosition.x)
        self.server.putUpdate("estY", self.swerve.estimatedPosition.y)

        vel = (self.swerve.estimatedPosition - self.prevPos) / self.time.dt
        self.server.putUpdate("velX", vel.x)
        self.server.putUpdate("velY", vel.y)
        self.prevPos = self.swerve.estimatedPosition

        vel = (self.gyro.getYaw() - self.prevAngle) / self.time.dt
        self.server.putUpdate("angVel", vel)
        self.prevAngle = self.gyro.getYaw()

        #TODO: debug expression in cpp sundial

        if self.isDisabled(): opmode = "disabled"
        elif self.isAutonomousEnabled(): opmode = "auto"
        elif self.isTeleopEnabled(): opmode = "teleop"
        else: opmode = "????????"
        self.server.putUpdate("opmode", opmode)

        self.server.putUpdate("enabled", self.isEnabled())


        self.server.update(self.time.timeSinceInit)





    def _simulationInit(self) -> None:
        self.sim = SwerveSim()

    def _simulationPeriodic(self) -> None:
        self.sim.update(self.time.dt, self.gyro, self.swerve)




    def autonomousInit(self) -> None:
        self.path = [
            (V2f(0, 0), V2f(1, 2), V2f(3, 2), V2f(4, 0)),
            (V2f(4, 0), V2f(3, -2), V2f(1, -2), V2f(0, 0))
        ]
        self.speedPath = [ V2f(0, 4.2), V2f(0.9, 4.2), V2f(1, 1) ]

        self.anglePath = [ V2f(0, 0), V2f(0.5, 90), V2f(1, -90) ]
        self.pathStart = self.time.timeSinceInit
        self.pathLength = 2.8 # in seconds


    def autonomousPeriodic(self) -> None:

        t = (self.time.timeSinceInit - self.pathStart) / self.pathLength
        t = min(1, t)
        self.server.putUpdate("t", t)

        position = self.swerve.estimatedPosition
        nextPt = getSpline2dSample(self.path, t)
        speed = (getSpline2dSample(self.path, min(1, t+0.001)) - nextPt).getNormalized() * getLinear2dSample(self.speedPath, t)
        nextAngle = getLinear2dSample(self.anglePath, t)
        self.server.putUpdate("targetX", nextPt.x)
        self.server.putUpdate("targetY", nextPt.y)
        self.server.putUpdate("targetSpeedX", speed.x)
        self.server.putUpdate("targetSpeedY", speed.y)
        self.server.putUpdate("targetAngle", nextAngle)

        recoveryDist = 4
        recoveryTime = 0.2
        l = (nextPt - position).getLength()
        l = max(0, min(l, recoveryDist))
        e = (1-(l/recoveryDist))**(2.4)
        e = 1-e
        correction = (nextPt - position) / recoveryTime
        trajectory = speed
        out = V2f.lerp(trajectory, correction, e)
        self.server.putUpdate("e", e)

        outA = angleWrap(nextAngle - self.gyro.getYaw()) * 40
        self.swerveController.tickReal(V2f(out.x, out.y).rotateDegrees(-self.gyro.getYaw()), outA, self.time.dt, self.swerve, self.server)
        self.server.putUpdate("outX", out.x)
        self.server.putUpdate("outY", out.y)
        self.server.putUpdate("outA", outA)




    def teleopPeriodic(self) -> None:

        self.input = SwerveBotInputs(self.driveCtrlr, self.armCtrlr)
        self.server.putUpdate("inputX", self.input.driveX)
        self.server.putUpdate("inputY", self.input.driveY)
        self.server.putUpdate("inputT", self.input.turning)

        drive = V2f(self.input.driveX, self.input.driveY).getNormalized()
        drive *= 4.2
        drive = drive.rotateDegrees(-self.gyro.getYaw())
        self.server.putUpdate("driveX", drive.x)
        self.server.putUpdate("driveY", drive.y)

        turning = self.input.turning * 180
        self.server.putUpdate("turning", turning)

        self.swerveController.tickReal(drive, turning, self.time.dt, self.swerve, self.server)
        # drive = drive.rotateDegrees(-self.gyro.getYaw() - 90)
        # self.swerveController.tick(drive.x, drive.y, self.input.turning, self.time.dt, self.input.brakeToggle, self.swerve)

    def disabledInit(self) -> None:
        self.disabledPeriodic()

    def disabledPeriodic(self) -> None:
        for i in range(0, 4):
            self.swerve.driveMotors[i].stopMotor()
            self.swerve.steerMotors[i].stopMotor()


if __name__ == "__main__":
    wpilib.run(SwerveBot)

    """
    r = SwerveBot()
    r.robotInit()
    r._simulationInit()
    r.teleopInit()

    while(True):
        r.robotPeriodic()
        r._simulationPeriodic()
        r.teleopPeriodic()
    # """
