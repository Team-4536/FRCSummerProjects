import wpilib
import rev
import ntcore
import math
import navx
from inputs import FlymerInputs

import sim
from real import V2f
import timing
from swerveController import SwerveController
from telemetryHelp import publishExpression
from virtualGyro import VirtualGyro
from swerveEstimation import SwerveEstimator
import socketing
from swervePathFollower import SwervePathFollower


WHEEL_DIA = 0.1016 # 4 in. in meters
WHEEL_RADIUS = WHEEL_DIA / 2
WHEEL_CIRC = WHEEL_DIA * math.pi


class SwerveBot(wpilib.TimedRobot):

    # TODO: assert saftey

    def robotInit(self) -> None:

        self.server = socketing.Server()
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


    def robotPeriodic(self) -> None:
        self.time = timing.TimeData(self.time)

        prefs = ["FL", "FR", "BL", "BR"]
        for i in range(0, 4):
            self.server.putUpdate(prefs[i] + "DriveSpeed", self.driveMotors[i].get())
            self.server.putUpdate(prefs[i] + "DrivePos", self.driveEncoders[i].getPosition())
            self.server.putUpdate(prefs[i] + "SteerSpeed", self.steerMotors[i].get())
            self.server.putUpdate(prefs[i] + "SteerPos", self.steerEncoders[i].getPosition())

        self.server.putUpdate("posX", self.sim.position.x)
        self.server.putUpdate("posY", self.sim.position.y)
        self.server.putUpdate("yaw", self.gyro.getYaw())

        self.server.putUpdate("test", int(420))

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
        self.pathFollower = SwervePathFollower([
            (V2f(0, 0), 0),
            (V2f(4, 0), 90),
            (V2f(4, 4), 180),
            (V2f(-4, -4), 0),
            (V2f(0, 0), 0)
        ])

    def autonomousPeriodic(self) -> None:
        pose = self.estimator.estimatedPose
        d, t = self.pathFollower.tick(pose, self.gyro.getYaw(), self.time.dt)
        self.swerveController.tick(self.gyro.getYaw(), d.x, d.y, t, self.time.dt, False)



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
