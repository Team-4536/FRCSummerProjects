import ntcore
import wpilib
import wpimath.system.plant as plant
import rev
import navx
import math

import timing
from real import V2f
from subsystems.mech import mechController
import socketing
from inputs import deadZone
from PIDController import PIDController
from real import angleWrap



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

        self.lift = deadZone(armCtrlr.getLeftY())
        self.turret = deadZone(armCtrlr.getLeftX())
        self.retract = deadZone(armCtrlr.getRightY())
        self.grabToggle = armCtrlr.getAButtonPressed()

AUTO_NONE = "none"
AUTO_BALANCE = "balance"
AUTO_EXIT = "exit"
class Flymer(wpilib.TimedRobot):


    def robotInit(self) -> None:
        self.chooser = wpilib.SendableChooser()

        self.chooser.setDefaultOption(AUTO_NONE, AUTO_NONE)
        self.chooser.addOption(AUTO_BALANCE, AUTO_BALANCE)
        self.chooser.addOption(AUTO_EXIT, AUTO_EXIT)
        wpilib.SmartDashboard.putData("autos", self.chooser)
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
        self.liftEncoder = self.liftMotor.getEncoder(rev.SparkMaxRelativeEncoder.Type.kQuadrature)
        self.retractEncoder = self.retractMotor.getEncoder(rev.SparkMaxRelativeEncoder.Type.kQuadrature)
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
        self.approachspeed = 0.2
        self.autostage = -1
        self.defaultgoal = V2f()
        self.scoregoal = V2f(1, 1) ##CHANGE THESE VALUES LATER
        self.stagestart = self.time.timeSinceInit   
    
    def scorePeriodic(self) -> bool:
        retractgoal = 0
        liftgoal = 0
        if self.autostage == -1: ##APPROACHING NODE
            self.FLDrive.set(self.approachspeed)
            self.FRDrive.set(self.approachspeed)
            self.BLDrive.set(self.approachspeed)
            self.BRDrive.set(self.approachspeed)
            if self.time.timeSinceInit - self.stagestart > .5:
                self.autostage+=1
        if self.autostage == 0: ##EXTENDING
            retractgoal = self.scoregoal.x
            liftgoal = self.scoregoal.y
            if self.retractEncoder.getPosition() >= (self.scoregoal.x-.05):
                 self.stagestart = self.time.timeSinceInit
                 self.autostage+=1
        if self.autostage == 1: ##SCORING
            retractgoal = self.scoregoal.x
            liftgoal = self.scoregoal.y
            self.grabber.toggle()
            if self.time.timeSinceInit - self.stagestart > 1:
                self.stagestart = self.time.timeSinceInit
                self.autostage+=.5
        if self.autostage == 1.5: ##MOVING AWAY
            self.FLDrive.set(-self.approachspeed)
            self.FRDrive.set(-self.approachspeed)
            self.BLDrive.set(-self.approachspeed)
            self.BRDrive.set(-self.approachspeed)
            if self.time.timeSinceInit - self.stagestart > 1:  
                self.FLDrive.set(0)
                self.FRDrive.set(0)
                self.BLDrive.set(0)
                self.BRDrive.set(0)
                self.stagestart = self.time.timeSinceInit
                self.autostage+=.5
        if self.autostage == 2: ##RETRACTING
            retractgoal = self.defaultgoal.x
            liftgoal = self.defaultgoal.y
            if self.retractEncoder.getPosition() <= (self.defaultgoal.x+.05):
                self.autostage+=1
        if self.autostage == 3:##TURNING
            retractgoal = self.defaultgoal.x
            liftgoal = self.defaultgoal.y
            angle = angleWrap(180 - self.gyro.getAngle())
            turnspeed = angle*.005
            self.FLDrive.set(turnspeed)
            self.FRDrive.set(turnspeed)
            self.BLDrive.set(-turnspeed)
            self.BRDrive.set(-turnspeed)
            if abs(angle) <= 5:
                self.autostage+=1
        if self.autostage == 4:
            return True
        retractspeed = self.retractcontroller.tick(retractgoal, self.retractEncoder.getPosition(), self.time.dt)
        liftspeed = self.liftcontroller.tick(liftgoal, self.liftEncoder.getPosition(), self.time.dt)
        return False

    def autonomousInit(self) -> None:
        self.ontop = False
        self.balance = self.chooser.getSelected()
        self.timerstart = self.time.timeSinceInit
        if self.balance == AUTO_BALANCE:
            self.scoreInit()
        if self.balance == AUTO_EXIT:
            self.scoreInit()

    def autonomousPeriodic(self) -> None:
        autospeed = .1
        balancespeed = .05
    
        
        if self.balance == AUTO_BALANCE: #balanceauto
            finished = self.scorePeriodic()
            if finished == False:
                return
    
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
             finished = self.scorePeriodic()
             if finished == False:
                return
             self.FLDrive.set(autospeed)
             self.FRDrive.set(autospeed)
             self.BLDrive.set(autospeed)
             self.BRDrive.set(autospeed)

             if self.time.timeSinceInit - self.timerstart > 6:
                self.FLDrive.set(0)
                self.FRDrive.set(0)
                self.BLDrive.set(0)
                self.BRDrive.set(0)

        elif self.balance == AUTO_NONE:
            pass

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



