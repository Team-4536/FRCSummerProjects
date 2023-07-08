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

class SwerveBot(wpilib.TimedRobot):


    def robotInit(self) -> None:

        self.server = socketing.Server()

        self.telemTable = ntcore.NetworkTableInstance.getDefault().getTable("telemetry")
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


    def _simulationInit(self) -> None:
        self.sim = sim.SwerveSim(
            self.driveMotors,
            self.driveEncoders,
            self.steerMotors,
            self.steerEncoders,
            self.gyro,
            [ V2f(-1, 1), V2f(1, 1), V2f(-1, -1), V2f(1, -1) ],
            0.1016 * math.pi #this 4in in meters, it's the wheel circ
        )

    def _simulationPeriodic(self) -> None:
        self.sim.update(self.time.dt)



    def robotPeriodic(self) -> None:
        self.time = timing.TimeData(self.time)

        prefs = ["FL", "FR", "BL", "BR"]
        for i in range(0, 4):
            self.telemTable.putNumber(prefs[i] + "DriveSpeed", self.driveMotors[i].get())
            self.telemTable.putNumber(prefs[i] + "DrivePos", self.driveEncoders[i].getPosition())
            self.telemTable.putNumber(prefs[i] + "SteerSpeed", self.steerMotors[i].get())
            self.telemTable.putNumber(prefs[i] + "SteerPos", self.steerEncoders[i].getPosition())

            self.server.putUpdateMessage(prefs[i] + "DriveSpeed", self.driveMotors[i].get())
            self.server.putUpdateMessage(prefs[i] + "DrivePos", self.driveEncoders[i].getPosition())
            self.server.putUpdateMessage(prefs[i] + "SteerSpeed", self.steerMotors[i].get())
            self.server.putUpdateMessage(prefs[i] + "SteerPos", self.steerEncoders[i].getPosition())


        self.telemTable.putNumber("PosX", self.sim.position.x)
        self.telemTable.putNumber("PosY", self.sim.position.y)
        self.telemTable.putNumber("Yaw", self.gyro.getYaw())

        self.server.putUpdateMessage("PosX", self.sim.position.x)
        self.server.putUpdateMessage("PosY", self.sim.position.y)
        self.server.putUpdateMessage("Yaw", self.gyro.getYaw())



        self.server.update(self.time.timeSinceInit)



    def teleopInit(self) -> None:
        self.swerveController = SwerveController(
            self.steerMotors,
            self.driveMotors,
            self.steerEncoders,
            self.driveEncoders)

    def teleopPeriodic(self) -> None:

        self.input = FlymerInputs(self.driveCtrlr, self.armCtrlr)

        self.swerveController.tick(
            -self.gyro.getYaw(),
            self.input.driveX,
            self.input.driveY,
            self.input.turning,
            self.time.dt,
            self.input.brakeToggle)

        # publishExpression("swerveController.leftStick.x", "x", self, self.telemTable)
        # publishExpression("swerveController.leftStick.y", "y", self, self.telemTable)


    def disabledPeriodic(self) -> None:
        return super().disabledPeriodic()


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
