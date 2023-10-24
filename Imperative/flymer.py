import ntcore
import wpilib
import wpimath.system.plant as plant
import rev
import navx
import math
import autoStaging

import timing
from real import V2f
from subsystems.mech import mechController
import socketing
from inputs import deadZone
from PIDController import PIDController
from real import angleWrap
from encoderSim import EncoderSim


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
AUTO_EXIT_SCORE = "exit+score"
AUTO_EXIT = "exit"
class Flymer(wpilib.TimedRobot):


    def robotInit(self) -> None:
        self.chooser = wpilib.SendableChooser()

        self.chooser.setDefaultOption(AUTO_NONE, AUTO_NONE)
        self.chooser.addOption(AUTO_BALANCE, AUTO_BALANCE)
        self.chooser.addOption(AUTO_EXIT, AUTO_EXIT)
        self.chooser.addOption(AUTO_EXIT_SCORE, AUTO_EXIT_SCORE)                                                                                                                                                            #code code code 
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

        self.server.putUpdate("LiftPos", self.liftEncoder.getPosition())
        self.server.putUpdate("RetractPos", self.retractEncoder.getPosition())
        self.server.putUpdate("TurretPos", self.turretEncoder.getPosition())

        self.server.putUpdate("grabber", True if self.grabber.get() == wpilib.DoubleSolenoid.Value.kForward else False)
        self.server.putUpdate("brakes", True if self.brakes.get() == wpilib.DoubleSolenoid.Value.kForward else False)

        self.server.putUpdate("Gyro", self.gyro.getYaw())
        self.server.putUpdate("AbsoluteDrive", self.absoluteDrive)

        self.server.update(self.time.timeSinceInit)

    def _simulationInit(self) -> None:
        self.liftSim = EncoderSim(plant.DCMotor.NEO(1), 1, 1, False)
        self.retractSim = EncoderSim(plant.DCMotor.NEO(1), 1, 1, False)

    def _simulationPeriodic(self) -> None:
        self.liftSim.update(self.time.dt, self.liftMotor, self.liftEncoder)
        self.retractSim.update(self.time.dt, self.retractMotor, self.retractEncoder)

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

        self.liftMotor.set(self.input.lift * 0.7)
        self.retractMotor.set(self.input.retract * 0.5)
        self.turretMotor.set(self.input.turret * 0.08)

        if self.input.grabToggle:
            self.grabber.toggle()
        if self.input.brakeToggle:
            self.brakes.toggle()

    def driveArmGoal(self, liftgoal:float, retractgoal:float) -> None:
        retractspeed = -self.retractcontroller.tick(retractgoal, self.retractEncoder.getPosition(), self.time.dt)
        liftspeed = self.liftcontroller.tick(liftgoal, self.liftEncoder.getPosition(), self.time.dt)
        self.retractMotor.set(retractspeed)
        self.liftMotor.set(liftspeed)
       
    def driveUnif(self, speed: float) -> None:
        self.FLDrive.set(speed)
        self.FRDrive.set(speed)
        self.BLDrive.set(speed)
        self.BRDrive.set(speed)
        
    def driveTurn(self, speed:float) -> None:
        self.FLDrive.set(speed)
        self.FRDrive.set(-speed)
        self.BLDrive.set(speed)
        self.BRDrive.set(-speed)

    def driveArmSpeed(self, retractspeed: float, liftspeed:float) -> None:
        self.retractMotor.set(retractspeed)
        self.liftMotor.set(liftspeed)

    def autonomousInit(self) -> None:
        self.retractcontroller = PIDController(0.1,0,0)
        self.liftcontroller = PIDController(0.1,0,0)
        self.liftEncoder.setPosition(0)
        self.retractEncoder.setPosition(0)
        self.turretEncoder.setPosition(0)
        self.approachspeed = 0.2
        self.autostage = 0
        self.defaultgoal = V2f()
        self.ontop = False
        self.selectedauto = self.chooser.getSelected()
        self.autospeed = .2
        self.balancespeed = .1
        self.scoregoal = V2f(5,5)
        self.defaultgoal = V2f(0,0)
        stagelist = []
        scorelist = [autoStaging.approach, autoStaging.extend, autoStaging.score, autoStaging.retreat, autoStaging.turn]
       
        if self.selectedauto == AUTO_BALANCE: #balance auto
            stagelist = scorelist + [autoStaging.balance]
        elif self.selectedauto == AUTO_EXIT_SCORE: #exit score auto
            stagelist = scorelist + [autoStaging.exit]
        elif self.selectedauto == AUTO_EXIT: #exit auto
            stagelist = [autoStaging.exit]
        elif self.selectedauto == AUTO_NONE:#no auto :)
            pass
        else:
            pass
        
        self.auto = autoStaging.Auto(stagelist, self.time.timeSinceInit)

    def autonomousPeriodic(self) -> None:
        self.auto.update(self)
    
    def disabledPeriodic(self) -> None:
        self.driveUnif(0)
        self.liftMotor.set(0)
        self.retractMotor.set(0)
        self.turretMotor.set(0)

if __name__ == "__main__":
    wpilib.run(Flymer)



