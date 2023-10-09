import ntcore
import wpilib
import wpimath.system.plant as plant
import rev
import navx

import timing
from real import V2f
from subsystems.mech import mechController
import socketing
from inputs import deadZone
from PIDController import PIDController




class FlymerInputs():

    # TODO: switch keyboard/controller modes from sundial

    def __init__(self, driveCtrlr: wpilib.XboxController, armCtrlr: wpilib.XboxController) -> None:
        self.driveX = deadZone(driveCtrlr.getLeftX())
        self.driveY = deadZone((-driveCtrlr.getLeftY()))
        self.turning = deadZone(driveCtrlr.getRightX())
        self.speedControl = driveCtrlr.getRightTriggerAxis() + .2

        self.gyroReset = driveCtrlr.getYButtonPressed()

        self.absoluteDriveToggle = driveCtrlr.getXButtonPressed()

        self.brakeToggle = driveCtrlr.getBButtonPressed()

        self.lift = deadZone(armCtrlr.getRightY())
        self.turret = deadZone(armCtrlr.getRightX())
        self.retract = deadZone(armCtrlr.getLeftY())
        self.grabToggle = armCtrlr.getAButtonPressed()

AUTO_BALANCE = "balance"
AUTO_EXIT = "exit"
class Flymer(wpilib.TimedRobot):


    def robotInit(self) -> None:
        self.chooser = wpilib.SendableChooser()
        self.chooser.setDefaultOption(AUTO_EXIT, AUTO_EXIT)
        self.chooser.addOption(AUTO_BALANCE, AUTO_BALANCE)
        self.server = socketing.Server(self.isReal())

        # DRIVE MOTORS ==================================================

        brushlessMotor = rev.CANSparkMax.MotorType.kBrushless
        brushedMotor = rev.CANSparkMax.MotorType.kBrushed

        self.FLDrive = rev.CANSparkMax(4, brushlessMotor)
        self.FRDrive = rev.CANSparkMax(1, brushlessMotor)
        self.BLDrive = rev.CANSparkMax(3, brushlessMotor)
        self.BRDrive = rev.CANSparkMax(2, brushlessMotor)

        self.absoluteDrive = False #default is off, hit X to toggle

        self.turningScalar = .2 #change for comp to about .3

        self.liftMotor = rev.CANSparkMax(7, brushedMotor)
        self.retractMotor = rev.CANSparkMax(6, brushedMotor)
        self.turretMotor = rev.CANSparkMax(5, brushlessMotor)
        self.liftEncoder = self.liftMotor.getEncoder()
        self.retractEncoder = self.retractMotor.getEncoder()
        self.turretEncoder = self.turretMotor.getEncoder()


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

    def robotPeriodic(self) -> None:

        self.time = timing.TimeData(self.time)

        self.server.putUpdate("FLSpeed", self.FLDrive.get())
        self.server.putUpdate("FRSpeed", self.FRDrive.get())
        self.server.putUpdate("BLSpeed", self.BLDrive.get())
        self.server.putUpdate("BRSpeed", self.BRDrive.get())

        self.server.putUpdate("LiftSpeed", self.liftMotor.get())
        self.server.putUpdate("RetractSpeed", self.retractMotor.get())
        self.server.putUpdate("TurretSpeed", self.turretMotor.get())

        self.server.putUpdate("grabber", True if self.grabber.get() == wpilib.DoubleSolenoid.Value.kForward else False)
        self.server.putUpdate("brakes", True if self.brakes.get() == wpilib.DoubleSolenoid.Value.kForward else False)

        self.server.putUpdate("Gyro", self.gyro.getYaw())
        self.server.putUpdate("AbsoluteDrive", self.absoluteDrive)

        self.server.update(self.time.timeSinceInit)

    def teleopInit(self) -> None:
        self.gyro.reset()

    def teleopPeriodic(self) -> None:

        self.input = FlymerInputs(self.driveCtrlr, self.armCtrlr)
        self.leftStickVector = V2f(self.input.driveX, self.input.driveY).rotateDegrees(-self.gyro.getYaw())

        speedControl = self.input.speedControl

        if self.input.absoluteDriveToggle:
            self.absoluteDrive = not self.absoluteDrive

        if speedControl > .8:
            speedControl = 1

        if self.absoluteDrive:
            self.driveSpeeds = mechController(self.leftStickVector.x * speedControl, self.leftStickVector.y * speedControl, self.input.turning * self.turningScalar)
        else:
            self.driveSpeeds = mechController(self.input.driveX * speedControl, self.input.driveY * speedControl, self.input.turning * self.turningScalar)

        if self.input.speedControl > .7:
            self.input.speedControl = 1

        # self.driveSpeeds = drive.scaleSpeeds(self.driveSpeeds, 0.3)
        self.FLDrive.set(self.driveSpeeds[0])
        self.FRDrive.set(self.driveSpeeds[1])
        self.BLDrive.set(self.driveSpeeds[2])
        self.BRDrive.set(self.driveSpeeds[3])

        if self.input.gyroReset:
            self.gyro.reset()

        self.liftMotor.set(self.input.lift * 0.4)
        self.retractMotor.set(self.input.retract * 0.5)
        self.turretMotor.set(self.input.turret * 0.08)

        if self.input.grabToggle:
            self.grabber.toggle()

        if self.input.brakeToggle:
            self.brakes.toggle()


    def scoreInit(self) -> None:
        self.retractcontroller = PIDController(0.1,0,0)
        self.liftcontroller = PIDController(0.1,0,0)
        self.liftEncoder.setPosition(0)
        self.retractEncoder.setPosition(0)
        self.turretEncoder.setPosition(0)
        
       

    def scorePeriodic(self) -> None:
        retractpos = 1
        liftpos = 1
        turretpos = 1
        retractspeed = self.retractcontroller.tick(retractpos, self.retractEncoder.getPosition(), self.time.dt)
        liftspeed = self.liftcontroller.tick(liftpos, self.liftEncoder.getPosition(), self.time.dt)
     
  
    def autonomousInit(self) -> None:
        self.ontop = False
        self.balance = self.chooser.getSelected()

        self.timerstart = self.time.timeSinceInit


    def autonomousPeriodic(self) -> None:
        autospeed = .1
        balancespeed = .15
    
        
        if self.balance == AUTO_BALANCE: #balanceauto

    
            if abs(self.gyro.getPitch()) > 10:
                self.ontop = True

            if self.ontop == False:
                self.FLDrive.set(autospeed)
                self.FRDrive.set(autospeed)
                self.BLDrive.set(autospeed)
                self.BRDrive.set(autospeed)

            if self.ontop == True and abs(self.gyro.getPitch()) < 10:
                self.brakes.set(wpilib.DoubleSolenoid.Value.kForward)
                self.FLDrive.set(0)
                self.FRDrive.set(0)
                self.BLDrive.set(0)
                self.BRDrive.set(0)

            if self.ontop == True and self.gyro.getPitch() > 10:
                self.FLDrive.set(-balancespeed)
                self.FRDrive.set(-balancespeed)
                self.BLDrive.set(-balancespeed)
                self.BRDrive.set(-balancespeed)


            if self.ontop == True and self.gyro.getPitch() < 10:
                self.FLDrive.set(balancespeed)
                self.FRDrive.set(balancespeed)
                self.BLDrive.set(balancespeed)
                self.BRDrive.set(balancespeed)

        elif self.balance == AUTO_EXIT: #exitauto
             self.FLDrive.set(balancespeed)
             self.FRDrive.set(balancespeed)
             self.BLDrive.set(balancespeed)
             self.BRDrive.set(balancespeed)

             if self.time.timeSinceInit - self.timerstart > 6:
                self.FLDrive.set(0)
                self.FRDrive.set(0)
                self.BLDrive.set(0)
                self.BRDrive.set(0)

        else:
            assert(False)
            
            


    def disabledPeriodic(self) -> None:
        self.FLDrive.set(0)
        self.FRDrive.set(0)
        self.BLDrive.set(0)
        self.BRDrive.set(0)

        self.liftMotor.set(0)
        self.retractMotor.set(0)
        self.turretMotor.set(0)


if __name__ == "__main__":
    wpilib.run(Flymer)



