import wpilib
import rev
import ntcore
import math
import navx

from inputs import FlymerInputs
from real import V2f, angleWrap
from swerveController import SwerveController
from telemetryHelp import publishExpression
from virtualGyro import VirtualGyro
from swerveEstimation import SwerveEstimator
from wpimath.controller import RamseteController
import socketing
import sim
import timing
from paths import getSpline2dPoints, getLinear2dPoints
import wpimath.system.plant as plant
from wpimath.geometry import Pose2d, Rotation2d


WHEEL_DIA = 0.1016 # 4 in. in meters
WHEEL_RADIUS = WHEEL_DIA / 2
WHEEL_CIRC = WHEEL_DIA * math.pi


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

        self.driveMotors = [
            rev.CANSparkMax(0, driveType),
            rev.CANSparkMax(1, driveType),
            rev.CANSparkMax(2, driveType),
            rev.CANSparkMax(3, driveType)
        ]
        self.driveEncoders: list[rev.RelativeEncoder] = [x.getEncoder() for x in self.driveMotors]

        self.steerMotors = [
            rev.CANSparkMax(4, driveType),
            rev.CANSparkMax(5, driveType),
            rev.CANSparkMax(6, driveType),
            rev.CANSparkMax(7, driveType)
        ]
        self.steerEncoders: list[rev.RelativeEncoder] = [x.getEncoder() for x in self.steerMotors]

        self.estimator = SwerveEstimator()


        self.swerveController = SwerveController(
            self.steerMotors,
            self.driveMotors,
            self.steerEncoders,
            self.driveEncoders)

        m = rev.CANSparkMax(10, driveType)


    def robotPeriodic(self) -> None:

        self.time = timing.TimeData(self.time)


        self.server.putUpdate("time", self.time.timeSinceInit)

        prefs = ["FL", "FR", "BL", "BR"]
        for i in range(0, 4):
            self.server.putUpdate(prefs[i] + "DriveSpeed", self.driveMotors[i].get())
            self.server.putUpdate(prefs[i] + "DrivePos", self.driveEncoders[i].getPosition())
            self.server.putUpdate(prefs[i] + "SteerSpeed", self.steerMotors[i].get())
            self.server.putUpdate(prefs[i] + "SteerPos", self.steerEncoders[i].getPosition())

        self.server.putUpdate("posX", float(self.sim.position.x))
        self.server.putUpdate("posY", float(self.sim.position.y))
        self.server.putUpdate("yaw", self.gyro.getYaw())

        estimatedPose = self.estimator.update(self.time.timeSinceInit, self.gyro.getYaw(),
            [self.driveEncoders[i].getPosition() * WHEEL_CIRC for i in range(0, 4)],
            [self.steerEncoders[i].getPosition() * 360 for i in range(0, 4)]
            )

        self.server.putUpdate("estX", estimatedPose.x)
        self.server.putUpdate("estY", estimatedPose.y)


        #TODO: debug expression in cpp sundial

        if self.isDisabled(): opmode = "disabled"
        elif self.isAutonomousEnabled(): opmode = "auto"
        elif self.isTeleopEnabled(): opmode = "teleop"
        else: opmode = "????????"
        self.server.putUpdate("opmode", opmode)

        self.server.putUpdate("enabled", self.isEnabled())


        self.server.update(self.time.timeSinceInit)





    def _simulationInit(self) -> None:
        self.sim = sim.SwerveSim(
            self.driveMotors,
            self.driveEncoders,
            self.steerMotors,
            self.steerEncoders,
            self.gyro,
            [ V2f(-1, 1), V2f(1, 1), V2f(-1, -1), V2f(1, -1) ],
            WHEEL_CIRC
        )

    def _simulationPeriodic(self) -> None:
        self.sim.update(self.time.dt)





    def autonomousInit(self) -> None:

        pointCount = 100
        self.path = getSpline2dPoints([
            (V2f(0, 0), V2f(1, 2), V2f(3, 2), V2f(4, 0)),
            (V2f(4, 0), V2f(3, -2), V2f(1, -2), V2f(0, 0))
        ], pointCount)

        self.speedPath = getLinear2dPoints([
            V2f(0.8, 1), V2f(1, 0.4)
        ], pointCount)

        self.anglePath = getLinear2dPoints([
            V2f(0, 0), V2f(0.5, 90), V2f(1, -90)
        ], pointCount)

        self.pathIdx = 0

        self.ramsete = RamseteController()

    def autonomousPeriodic(self) -> None:

        self.server.putUpdate("idx", self.pathIdx)
        if(self.pathIdx >= len(self.path)):
            self.pathIdx = 99

        position = self.estimator.estimatedPose

        nextPt = self.path[self.pathIdx]
        self.server.putUpdate("targetX", nextPt.x)
        self.server.putUpdate("targetY", nextPt.y)
        speed = self.speedPath[self.pathIdx]
        self.server.putUpdate("targetSpeed", speed)

        nextAngle = self.anglePath[self.pathIdx]
        self.server.putUpdate("targetAngle", nextAngle)

        currentPose = Pose2d(position.x, position.y, math.radians(-self.gyro.getYaw()))
        targetPose = Pose2d(nextPt.x, nextPt.y, math.radians(-nextAngle))
        linVel = speed # m/s
        angularVel = 1 # rads/s
        out = self.ramsete.calculate(currentPose, targetPose, linVel, angularVel)

        self.swerveController.tick(self.gyro.getYaw(), out.vx, out.vy, -math.degrees(out.omega), self.time.dt, False)
        self.server.putUpdate("outX", out.vx)
        self.server.putUpdate("outY", out.vy)
        self.server.putUpdate("outA", -math.degrees(out.omega))

        if((position - nextPt).getLength() < 0.6): self.pathIdx += 1




    def teleopPeriodic(self) -> None:

        self.input = FlymerInputs(self.driveCtrlr, self.armCtrlr)

        self.swerveController.tick(
            self.gyro.getYaw(),
            self.input.driveX,
            self.input.driveY,
            self.input.turning,
            self.time.dt,
            self.input.brakeToggle)








    def disabledInit(self) -> None:
        self.disabledPeriodic()

    def disabledPeriodic(self) -> None:
        for i in range(0, 4):
            self.driveMotors[i].stopMotor()
            self.steerMotors[i].stopMotor()


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
