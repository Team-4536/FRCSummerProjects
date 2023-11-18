import ntcore
import wpilib
import wpimath.system.plant as plant
import rev
import navx
import math
import autoStaging
from encoderSim import EncoderSim
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

        #drive controller
        self.driveX = deadZone(driveCtrlr.getLeftX())
        self.driveY = deadZone((-driveCtrlr.getLeftY()))
        self.turning = deadZone(driveCtrlr.getRightX())
        self.speedControl = driveCtrlr.getRightTriggerAxis() + .2

        self.dpadAngle = driveCtrlr.getPOV()
        self.brakeToggle = driveCtrlr.getBButtonPressed()
        self.gyroReset = driveCtrlr.getYButtonPressed()
        self.absToggle = driveCtrlr.getXButtonPressed()

        #arm controller
        self.lift = deadZone(armCtrlr.getRightY())
        self.turret = deadZone(armCtrlr.getRightX())
        self.retract = deadZone(armCtrlr.getLeftY())
        self.grabToggle = armCtrlr.getAButtonPressed()
        self.coneHigh = armCtrlr.getYButtonPressed()
        self.coneMid = armCtrlr.getXButtonPressed()
        self.homeArm = armCtrlr.getBButtonPressed()
        self.leftBumper = armCtrlr.getLeftBumper()


AUTO_NONE = "none"
AUTO_BALANCE = "balance"
AUTO_EXIT_SCORE = "exit+score"
AUTO_EXIT = "exit"

class Flymer(wpilib.TimedRobot):

    def robotInit(self) -> None:

        self.chooser = wpilib.SendableChooser()
        self.chooser.setDefaultOption(AUTO_NONE, AUTO_NONE)
        # self.chooser.addOption(AUTO_BALANCE, AUTO_BALANCE)
        self.chooser.addOption(AUTO_EXIT, AUTO_EXIT)
        self.chooser.addOption(AUTO_EXIT_SCORE, AUTO_EXIT_SCORE)  # code code code

        wpilib.SmartDashboard.putData("autos", self.chooser)
        self.server = socketing.Server(self.isReal())

        self.retractcontroller = PIDController(0.1, 0, 0)
        self.liftcontroller = PIDController(0.01, 0, 0)

        # DRIVE MOTORS ==================================================

        brushlessMotor = rev.CANSparkMax.MotorType.kBrushless
        brushedMotor = rev.CANSparkMax.MotorType.kBrushed

        self.FLDrive = rev.CANSparkMax(4, brushlessMotor)
        self.FRDrive = rev.CANSparkMax(1, brushlessMotor)
        self.BLDrive = rev.CANSparkMax(3, brushlessMotor)
        self.BRDrive = rev.CANSparkMax(2, brushlessMotor)

        self.absoluteDrive = True  # default is off, hit X to toggle

        self.turningScalar = .3  # change for comp to about .3

        self.liftMotor = rev.CANSparkMax(7, brushedMotor)
        self.retractMotor = rev.CANSparkMax(6, brushedMotor)
        self.turretMotor = rev.CANSparkMax(5, brushlessMotor)
        self.liftEncoder = self.liftMotor.getEncoder(rev.SparkMaxRelativeEncoder.Type.kQuadrature)
        self.retractEncoder = self.retractMotor.getEncoder(rev.SparkMaxRelativeEncoder.Type.kQuadrature)
        self.retractEncoder.setPosition(0)
        self.turretEncoder = self.turretMotor.getEncoder()

        self.turretCCWLimit = wpilib.DigitalInput(6)
        self.turretCWLimit = wpilib.DigitalInput(5)
        self.liftLowerLimit = wpilib.DigitalInput(4)
        self.liftSideSwitch = wpilib.DigitalInput(3)
        self.liftUpperLimit = wpilib.DigitalInput(2)

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
        self.server.putUpdate("LeftSwitch", self.turretCCWLimit.get())
        self.server.putUpdate("RightSwitch", self.turretCWLimit.get())
        self.server.putUpdate("LiftSideSwitch", self.liftSideSwitch.get())
        self.server.putUpdate("LiftUpperSwitch", self.liftUpperLimit.get())
        self.server.putUpdate("LiftLowerSwitch", self.liftLowerLimit.get())

        self.server.update(self.time.timeSinceInit)

    def _simulationInit(self) -> None:
        self.turretSim = EncoderSim(plant.DCMotor.NEO(1), 1, 1, False)
        self.liftSim = EncoderSim(plant.DCMotor.NEO(1), 1, 1, False)
        self.retractSim = EncoderSim(plant.DCMotor.NEO(1), 1, 1, False)

    def _simulationPeriodic(self) -> None:
        self.liftSim.update(self.time.dt, self.liftMotor, self.liftEncoder)
        self.retractSim.update(self.time.dt, self.retractMotor, self.retractEncoder)
        self.turretSim.update(self.time.dt, self.turretMotor, self.turretEncoder)





    def teleopInit(self) -> None:
        #reset gyro
        self.gyro.reset()
        
        #create variables
        self.armHoming = False
        self.scoringHigh = False
        self.scoringMid = False
        self.retractBounded = True
        self.target = self.gyro.getYaw()



    def teleopPeriodic(self) -> None:
        #get controller inputs
        self.input = FlymerInputs(self.driveCtrlr, self.armCtrlr)
        self.leftStickVector = V2f(self.input.driveX, self.input.driveY).rotateDegrees(-self.gyro.getYaw())

        speedControl = self.input.speedControl

        #absolute drive toggle button
        if self.input.absToggle:
            self.absoluteDrive = not self.absoluteDrive

        #speed controller bounds
        if speedControl > 1:
            speedControl = 1

        #get wheel speeds
        if self.absoluteDrive:
            self.driveSpeeds = mechController(self.leftStickVector.x * speedControl, self.leftStickVector.y * speedControl, self.input.turning * self.turningScalar)
        else:
            self.driveSpeeds = mechController(self.input.driveX * speedControl, self.input.driveY * speedControl, self.input.turning * self.turningScalar)

        #assign wheel speeds
        # self.driveSpeeds = drive.scaleSpeeds(self.driveSpeeds, 0.3)
        self.FLDrive.set(self.driveSpeeds[0])
        self.FRDrive.set(self.driveSpeeds[1])
        self.BLDrive.set(self.driveSpeeds[2])
        self.BRDrive.set(self.driveSpeeds[3])

        #reset buttons
        if self.input.gyroReset:
            self.gyro.reset()

        if self.input.leftBumper:
            self.retractBounded = False
            self.retractEncoder.setPosition(0)
        else:
            self.retractBounded = True


        """--arm scalars--"""
        # (arm down = power+, encoder+)
        liftScalar = 0.5
        liftSpeed = self.input.lift

        # (arm in/retract = power+, encoder-)
        retractScalar = 0.5
        retractSpeed = self.input.retract

        # (CW = power+, encoder+)
        turretScalar = 0.1
        turretSpeed = self.input.turret

        
        """----------arm setpoints----------"""
        if self.input.homeArm: self.armHoming = True 
        elif self.input.coneHigh and self.liftUpperLimit.get(): self.scoringHigh = True
        #elif self.input.coneMid and self.liftUpperLimit.get(): self.scoringMid = True

        if self.input.lift != 0 or self.input.retract != 0 or self.retractBounded == False: 
            self.armHoming = False
            self.scoringHigh = False
            self.scoringMid = False

        #home
        if self.armHoming:   
            liftSpeed = -1
            retractError = -self.retractEncoder.getPosition()

            if  self.retractEncoder.getPosition() < 2500:
                retractSpeed = -self.retractcontroller.tickErr(retractError, self.time.dt)
            else: retractSpeed = 0

            if abs(retractError) < 10 and self.liftUpperLimit.get(): self.armHoming = False
        else: self.armHoming = False
        
        #score cone high
        if self.scoringHigh:
            liftError = 592 - self.liftEncoder.getPosition()
            retractError = 1505 - self.retractEncoder.getPosition()

            liftSpeed = self.liftcontroller.tickErr(liftError, self.time.dt)
            retractSpeed = -self.retractcontroller.tickErr(retractError, self.time.dt)
            if abs(liftError) < 50 and abs(retractError) < 10: self.scoringHigh = False
        else: self.scoringHigh = False

        """
        #score cone middle (DOES NOT WORK - DO NOT USE)
        if self.scoringMid:
            liftError = 0 #change to targets and stuff
            retractError = 0

            liftSpeed = self.liftcontroller.tickErr(liftError, self.time.dt)
            retractSpeed = -self.retractcontroller.tickErr(retractError, self.time.dt)
        else: self.scoringMid = False
        """

        """--------motors and limits--------"""
        #lift motor
        if self.liftSideSwitch.get() and liftSpeed < 0: liftScalar = 0.2

        if self.liftUpperLimit.get():
            self.liftEncoder.setPosition(0)
            if liftSpeed < 0: 
                liftSpeed = 0

        if self.liftLowerLimit.get() and liftSpeed > 0: liftSpeed = 0

        if liftSpeed > 1: liftSpeed = 1
        self.liftMotor.set(liftSpeed * liftScalar)

        #retract motor
        if self.retractEncoder.getPosition() > 2500 and self.retractEncoder.getPosition() < 10000 and self.retractBounded and retractSpeed < 0:
            retractSpeed = 0

        if self.retractEncoder.getPosition() > 10000 and retractSpeed > 0 and self.retractBounded:
            retractSpeed = 0

        if retractSpeed > 1: retractSpeed = 1
        self.retractMotor.set(retractSpeed * retractScalar)

        self.server.putUpdate("bounded", self.retractBounded)

        #turret motor
        if self.turretCWLimit.get() and turretSpeed > 0: turretSpeed = 0
        if self.turretCCWLimit.get() and turretSpeed < 0: turretSpeed = 0

        if turretSpeed > 1: turretSpeed = 1
        self.turretMotor.set(turretSpeed * turretScalar)

        """---------pneumatics---------"""
        #grabber
        if self.input.grabToggle: self.grabber.toggle()

        #brakes
        if self.input.brakeToggle: self.brakes.toggle()




    def getArmPower(self, liftError: float, retractError: float):
        liftSpeed = self.liftcontroller.tickErr(liftError, self.time.dt)
        retractSpeed = -self.retractcontroller.tickErr(retractError, self.time.dt)
        return liftSpeed, retractSpeed

    def driveArmGoal(self, liftgoal: float, retractgoal: float) -> None:
        if self.retractEncoder.getPosition() > 40000:
            retractspeed = -0.5
        else:
            retractspeed = - self.retractcontroller.tick(retractgoal, self.retractEncoder.getPosition(), self.time.dt)
        liftspeed = self.liftcontroller.tick(liftgoal, self.liftEncoder.getPosition(), self.time.dt)
        self.retractMotor.set(retractspeed)
        self.liftMotor.set(liftspeed)

    def driveUnif(self, speed: float) -> None:
        self.FLDrive.set(speed)
        self.FRDrive.set(speed)
        self.BLDrive.set(speed)
        self.BRDrive.set(speed)

    def driveTurn(self, speed: float) -> None:
        self.FLDrive.set(speed)
        self.FRDrive.set(-speed)
        self.BLDrive.set(speed)
        self.BRDrive.set(-speed)

    def driveArmSpeed(self, retractspeed: float, liftspeed: float) -> None:
        self.retractMotor.set(retractspeed)
        self.liftMotor.set(liftspeed)

    def autonomousInit(self) -> None:
        self.retractcontroller = PIDController(0.001, 0, 0)
        self.liftcontroller = PIDController(0.0002, 0, 0)
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
        self.scoregoal = V2f(1550, 600)
        self.defaultgoal = V2f(0, 0)

        stagelist = []
        scorelist = [autoStaging.approach, autoStaging.extend, autoStaging.score, autoStaging.retreat, autoStaging.turn]

        if self.selectedauto == AUTO_BALANCE: #balance auto
            stagelist = scorelist + [autoStaging.balance]
        elif self.selectedauto == AUTO_EXIT_SCORE: #exit score auto
            stagelist = scorelist + [autoStaging.exit]
        elif self.selectedauto == AUTO_EXIT: #exit auto
            stagelist = [autoStaging.exit]
        elif self.selectedauto == AUTO_NONE: #no auto :)
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
