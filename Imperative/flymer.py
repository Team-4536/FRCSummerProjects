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
from real import V2f




class Flymer(wpilib.TimedRobot):


    def robotInit(self) -> None:

        self.telemTable = ntcore.NetworkTableInstance.getDefault().getTable("telemetry")

        # DRIVE MOTORS ==================================================

        brushlessMotor = rev.CANSparkMax.MotorType.kBrushless
        brushedMotor = rev.CANSparkMax.MotorType.kBrushed

        self.FLDrive = rev.CANSparkMax(4, brushlessMotor)
        self.FRDrive = rev.CANSparkMax(1, brushlessMotor)
        self.BLDrive = rev.CANSparkMax(3, brushlessMotor)
        self.BRDrive = rev.CANSparkMax(2, brushlessMotor)

        self.liftMotor = rev.CANSparkMax(7, brushedMotor)
        self.retractMotor = rev.CANSparkMax(6, brushedMotor)
        self.turretMotor = rev.CANSparkMax(5, brushlessMotor)

        self.pcm = wpilib.PneumaticsControlModule()

        self.grabber = self.pcm.makeDoubleSolenoid(7, 5)
        self.brakes = self.pcm.makeDoubleSolenoid(4, 6)

        self.grabber.set(wpilib.DoubleSolenoid.Value.kReverse)
        self.brakes.set(wpilib.DoubleSolenoid.Value.kReverse)

        self.FRDrive.setInverted(True)
        self.BRDrive.setInverted(True)

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

        self.telemTable.putNumber("FLSpeed", self.FLDrive.get())
        self.telemTable.putNumber("FRSpeed", self.FRDrive.get())
        self.telemTable.putNumber("BLSpeed", self.BLDrive.get())
        self.telemTable.putNumber("BRSpeed", self.BRDrive.get())

        self.telemTable.putNumber("LiftSpeed", self.liftMotor.get())
        self.telemTable.putNumber("RetractSpeed", self.retractMotor.get())
        self.telemTable.putNumber("TurretSpeed", self.turretMotor.get())

        self.telemTable.putBoolean("grabber", True if self.grabber.get() == wpilib.DoubleSolenoid.Value.kForward else False)
        self.telemTable.putBoolean("brakes", True if self.brakes.get() == wpilib.DoubleSolenoid.Value.kForward else False)

        self.telemTable.putNumber("Gyro", self.gyro.getYaw())




    def teleopInit(self) -> None:

        self.gyro.reset()





    def teleopPeriodic(self) -> None:

        self.input = inputs.FlymerInputs(self.driveCtrlr, self.armCtrlr)
        self.leftStickVector = V2f(self.input.driveX, self.input.driveY).rotateDegrees(-self.gyro.getYaw())

        self.driveSpeeds = drive.mechController(self.leftStickVector.x, self.leftStickVector.y, self.input.turning)
        self.driveSpeeds = drive.scaleSpeeds(self.driveSpeeds, 0.3)
        drive.setMotors(self.driveSpeeds, self.FLDrive, self.FRDrive, self.BLDrive, self.BRDrive)

        if self.input.gyroReset:
            self.gyro.reset()

        self.liftMotor.set(self.input.lift * 0.4)
        self.retractMotor.set(self.input.retract * 0.5)
        self.turretMotor.set(self.input.turret * 0.08)

        if self.input.grabToggle:
            self.grabber.toggle()

        if self.input.brakeToggle:
            self.brakes.toggle()


    def disabledPeriodic(self) -> None:
        drive.setMotors([0, 0, 0, 0], self.FLDrive, self.FRDrive, self.BLDrive, self.BRDrive)

        self.liftMotor.set(0)
        self.retractMotor.set(0)
        self.turretMotor.set(0)


if __name__ == "__main__":
    wpilib.run(Flymer)



