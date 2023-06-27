import wpilib
import rev
import ntcore

import sim
from real import V2f
import timing

class SwerveBot(wpilib.TimedRobot):


    def robotInit(self) -> None:

        self.telemTable = ntcore.NetworkTableInstance.getDefault().getTable("telemetry")
        self.driveCtrlr = wpilib.XboxController(0)
        self.time = timing.TimeData(None)


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
            [ V2f(-1, 1), V2f(1, 1), V2f(-1, -1), V2f(1, -1) ],
            1
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

            self.telemTable.putNumber("PosX", self.sim.position.x)
            self.telemTable.putNumber("PosY", self.sim.position.y)
            self.telemTable.putNumber("Angle", self.sim.rotation)



    def teleopInit(self) -> None:
        return super().teleopInit()

    def teleopPeriodic(self) -> None:

        steer = self.driveCtrlr.getLeftX()
        drive = -self.driveCtrlr.getLeftY()

        for i in range(0, 4):
            self.steerMotors[i].set(steer)
            self.driveMotors[i].set(drive)



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
