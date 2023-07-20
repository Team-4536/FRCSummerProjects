import ntcore
import wpilib
import wpimath.system.plant as plant
import rev
import navx

import drive
import inputs
from telemetryHelp import publishExpression
import sim
import timing
import socketing







class DemoBot(wpilib.TimedRobot):


    def robotInit(self) -> None:

        self.server = socketing.Server()

        # DRIVE MOTORS ==================================================
        self.FLDrive = wpilib.Spark(2)
        self.FRDrive = wpilib.Spark(3)
        self.BLDrive = wpilib.Spark(1)
        self.BRDrive = wpilib.Spark(4)
        self.FRDrive.setInverted(True)
        self.BRDrive.setInverted(True)

        self.shooter = wpilib.Spark(0)
        self.shooter.setInverted(True)

        # GYRO ==========================================================
        self.gyro = navx.AHRS(wpilib.SPI.Port.kMXP)

        # CONTROLLERS ====================================================

        self.driveCtrlr = wpilib.XboxController(0)
        self.armCtrlr = wpilib.XboxController(1)

        # TIME =========================================================
        self.time = timing.TimeData(None)



    def _simulationInit(self) -> None:
        pass

    def _simulationPeriodic(self) -> None:
        pass





    def robotPeriodic(self) -> None:

        self.time = timing.TimeData(self.time)

        self.server.putUpdate("FLSpeed", self.FLDrive.get())
        self.server.putUpdate("FRSpeed", self.FRDrive.get())
        self.server.putUpdate("BLSpeed", self.BLDrive.get())
        self.server.putUpdate("BRSpeed", self.BRDrive.get())
        self.server.putUpdate("ShooterSpeed", self.shooter.get())


        self.shooterEnabled: bool = self.server.tracked.get("shooter enabled", True) # type: ignore
        self.shooterSpeed: float = self.server.tracked.get("shooter speed", 0.6) # type: ignore

        self.server.update(self.time.timeSinceInit)


    def teleopPeriodic(self) -> None:

        self.input = inputs.DemoInputs(self.driveCtrlr)

        self.driveSpeeds = drive.tankController(self.input.drive, self.input.turning)
        self.driveSpeeds = drive.scaleSpeeds(self.driveSpeeds, 0.3)
        drive.setMotors(self.driveSpeeds, self.FLDrive, self.FRDrive, self.BLDrive, self.BRDrive)


        target = 0.0
        if self.shooterEnabled: target = self.shooterSpeed if self.driveCtrlr.getAButton() else 0.0
        self.shooter.set(target)


    def disabledPeriodic(self) -> None:
        drive.setMotors([0, 0, 0, 0], self.FLDrive, self.FRDrive, self.BLDrive, self.BRDrive)
        self.shooter.set(0)




if __name__ == "__main__":
    wpilib.run(DemoBot)


