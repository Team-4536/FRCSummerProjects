import ntcore
import wpilib
import wpimath.system.plant as plant
import rev
import navx

import drive
import inputs
from telemetryHelp import publishExpression
import sim



class TimeData:
    def __init__(self, prev) -> None:
        time = wpilib.getTime()

        if prev is None:
            self.initTime = time
            self.prevTime = time
            self.dt = 0
            self.timeSinceInit = 0
        else:
            self.dt = time - prev.prevTime
            self.timeSinceInit = time - prev.initTime
            self.prevTime = time
            self.initTime = prev.initTime






class DemoBot(wpilib.TimedRobot):


    def robotInit(self) -> None:

        self.telemTable = ntcore.NetworkTableInstance.getDefault().getTable("telemetry")

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
        self.time = TimeData(None)



    def _simulationInit(self) -> None:
        self.FLEncoder = rev.RelativeEncoder()
        self.FREncoder = rev.RelativeEncoder()
        self.BLEncoder = rev.RelativeEncoder()
        self.BREncoder = rev.RelativeEncoder()

        self.FLEncSim = sim.EncoderSim(plant.DCMotor.NEO(1), 1)
        self.FREncSim = sim.EncoderSim(plant.DCMotor.NEO(1), 1)
        self.BLEncSim = sim.EncoderSim(plant.DCMotor.NEO(1), 1)
        self.BREncSim = sim.EncoderSim(plant.DCMotor.NEO(1), 1)

    def _simulationPeriodic(self) -> None:
        self.FLEncSim.update(self.FLDrive, self.FLEncoder, self.time.dt)
        self.FREncSim.update(self.FLDrive, self.FLEncoder, self.time.dt)
        self.BLEncSim.update(self.FLDrive, self.FLEncoder, self.time.dt)
        self.BREncSim.update(self.FLDrive, self.FLEncoder, self.time.dt)





    def robotPeriodic(self) -> None:

        self.time = TimeData(self.time)

        publishExpression("__class__.__name__", self, self.telemTable)

        self.telemTable.putNumber("FLSpeed", self.FLDrive.get())
        self.telemTable.putNumber("FRSpeed", self.FRDrive.get())
        self.telemTable.putNumber("BLSpeed", self.BLDrive.get())
        self.telemTable.putNumber("BRSpeed", self.BRDrive.get())
        self.telemTable.putNumber("ShooterSpeed", self.shooter.get())


        self.shooterEnabled: bool = self.telemTable.getBoolean("shooter enabled", True) # type: ignore
        self.shooterSpeed: float = self.telemTable.getNumber("shooter speed", 0.6) # type: ignore


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

    """
    r = Robot()
    r.robotInit()
    r._simulationInit()
    r.teleopInit()

    while True:
        r.robotPeriodic()
        r._simulationPeriodic()
        r.teleopPeriodic()
    # """


