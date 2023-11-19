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
from typing_extensions import Self

import copy

class FlymerHalBuffer():
    def __init__(self) -> None:
        self.driveSpeeds: list[float] = [0, 0, 0, 0] # -1 to 1
        self.drivePositions: list[float] = [0, 0, 0, 0] # IO, rotations

        self.liftSpeed: float = 0.0 # -1 to 1
        self.retractSpeed: float = 0.0 # -1 to 1
        self.turretSpeed: float = 0.0 # -1 to 1

        self.liftPos: float = 0 # IO, rotations
        self.retractPos: float = 0.0 # IO, rotations
        self.turretPos: float = 0.0 # IO, rotations

        self.grabberOpen: bool = False
        self.brakesExtended: bool = False

        self.leftLimitPressed: bool = False # I
        self.rightLimitPressed: bool = False # I
        self.liftBottomPressed: bool = False # I
        self.liftTopPressed: bool = False # I
        self.liftTopSidePressed: bool = False # I

        self.gyroPitch: float = 0.0 # I
        self.gyroYaw: float = 0.0 # IO, CW degree
        self.gyroRoll: float = 0.0 # I

        # TODO: what happens when a limit siwtch alias accedentally gets set

    def resetEncoders(self) -> None:
        self.drivePositions = [0, 0, 0, 0]
        self.liftPos = 0
        self.retractPos = 0
        self.turretPos = 0


    def publish(self, server: socketing.Server, prefix: str):
        for k in self.__dict__:
            prop = self.__getattribute__(k)
            if type(prop) == float or type(prop) == int or type(prop) == bool:
                server.putUpdate(prefix + k, prop)

        prefs = [ "FL", "FR", "BL", "BR" ]
        for i in range(0, 4):
            server.putUpdate(prefix + prefs[i] + "Speed", self.driveSpeeds[i])
            server.putUpdate(prefix + prefs[i] + "Pos", self.drivePositions[i])

# TODO: flymer sim hal

class FlymerHal():
    def __init__(self) -> None:
        self.prev: FlymerHalBuffer = FlymerHalBuffer()

        BRUSHLESS = rev.CANSparkMax.MotorType.kBrushless
        BRUSHED = rev.CANSparkMax.MotorType.kBrushed
        self.driveMotors = [
            rev.CANSparkMax(4, BRUSHLESS),
            rev.CANSparkMax(1, BRUSHLESS),
            rev.CANSparkMax(3, BRUSHLESS),
            rev.CANSparkMax(2, BRUSHLESS)
        ]
        self.driveEncoders = [x.getEncoder() for x in self.driveMotors]

        self.liftMotor = rev.CANSparkMax(7, BRUSHED)
        self.retractMotor = rev.CANSparkMax(6, BRUSHED)
        self.turretMotor = rev.CANSparkMax(5, BRUSHLESS)
        self.liftEncoder = self.liftMotor.getEncoder(rev.SparkMaxRelativeEncoder.Type.kQuadrature, 8192)
        self.retractEncoder = self.retractMotor.getEncoder(rev.SparkMaxRelativeEncoder.Type.kQuadrature, 8192)
        self.turretEncoder = self.turretMotor.getEncoder()

        self.leftLimit = wpilib.DigitalInput(6)
        self.rightLimit = wpilib.DigitalInput(5)
        self.liftLowerLimit = wpilib.DigitalInput(4)
        self.liftSideSwitch = wpilib.DigitalInput(3)
        self.liftUpperLimit = wpilib.DigitalInput(2)

        self.pcm = wpilib.PneumaticsControlModule()
        self.grabber = self.pcm.makeDoubleSolenoid(7, 5)
        self.brakes = self.pcm.makeDoubleSolenoid(4, 6)

        self.gyro = navx.AHRS(wpilib.SPI.Port.kMXP)



        self.driveMotors[1].setInverted(True)
        self.driveMotors[3].setInverted(True)



    def update(self, buf: FlymerHalBuffer) -> None:
        prev = self.prev
        self.prev = copy.deepcopy(buf)

        for m, s in zip(self.driveMotors, buf.driveSpeeds):
            m.set(s)

        for i in range(0, 4):
            e = self.driveEncoders[i]
            if(buf.drivePositions[i] != prev.drivePositions[i]):
                e.setPosition(buf.drivePositions[i])
            buf.drivePositions[i] = e.getPosition()

        self.liftMotor.set(buf.liftSpeed)
        self.retractMotor.set(buf.retractSpeed)
        self.turretMotor.set(buf.turretSpeed)

        if buf.liftPos != prev.liftPos: self.liftEncoder.setPosition(buf.liftPos)
        buf.liftPos = self.liftEncoder.getPosition()
        if buf.retractPos != prev.retractPos: self.retractEncoder.setPosition(buf.retractPos)
        buf.retractPos = self.retractEncoder.getPosition()
        if buf.turretPos != prev.turretPos: self.turretEncoder.setPosition(buf.turretPos)
        buf.turretPos = self.turretEncoder.getPosition()

        buf.leftLimitPressed = self.leftLimit.get()
        buf.rightLimitPressed = self.rightLimit.get()
        buf.liftBottomPressed = self.liftLowerLimit.get()
        buf.liftTopPressed = self.liftSideSwitch.get()
        buf.liftTopSidePressed = self.liftUpperLimit.get()

        FORWARD = wpilib.DoubleSolenoid.Value.kForward
        REVERSE = wpilib.DoubleSolenoid.Value.kReverse
        grabberVal = FORWARD if buf.grabberOpen else REVERSE
        self.grabber.set(grabberVal)

        brakeVal = FORWARD if buf.brakesExtended else REVERSE
        self.brakes.set(brakeVal)

        if buf.gyroYaw != prev.gyroYaw: self.gyro.zeroYaw()
        buf.gyroYaw = self.gyro.getYaw()
        buf.gyroPitch = self.gyro.getPitch()
        buf.gyroRoll = self.gyro.getRoll()








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

        self.hal = FlymerHalBuffer()
        self.hardware = FlymerHal()

        self.driveCtrlr = wpilib.XboxController(0)
        self.armCtrlr = wpilib.XboxController(1)

        self.time = timing.TimeData(None)

        self.absoluteDrive = False
        self.turningScalar = .3

    def robotPeriodic(self) -> None:
        self.time = timing.TimeData(self.time)
        self.hal.publish(self.server, "hal/")
        self.server.update(self.time.timeSinceInit)

    def teleopInit(self) -> None:
        self.hal.gyroYaw = 0

    def teleopPeriodic(self) -> None:
        self.input = FlymerInputs(self.driveCtrlr, self.armCtrlr)

        self.leftStickVector = V2f(self.input.driveX, self.input.driveY).rotateDegrees(-self.hal.gyroYaw)
        speedControl = self.input.speedControl

        if self.input.absoluteDriveToggle:
            self.absoluteDrive = not self.absoluteDrive
        if speedControl > .8:
            speedControl = 1

        driveVec = V2f(self.input.driveX, self.input.driveY) * speedControl
        self.hal.driveSpeeds = mechController(driveVec.x, driveVec.y, self.input.turning * self.turningScalar)

        if self.input.gyroReset: self.hal.gyroYaw = 0

        liftScalar = 0.5
        if self.hal.liftTopSidePressed and self.input.lift < 0:
             liftScalar = 0.2
        self.hal.liftSpeed = self.input.lift * liftScalar

        if self.hal.liftTopPressed:
            self.hal.liftPos = 0
            if self.input.lift < 0:
                self.hal.liftSpeed = 0

        if self.hal.liftBottomPressed:
            if self.input.lift > 0:
                self.hal.liftSpeed = 0

        self.hal.retractSpeed = self.input.retract * 0.5

        speed = self.input.turret * 0.08
        if self.hal.rightLimitPressed:
            if speed > 0:
                speed = 0
        if self.hal.leftLimitPressed:
              if speed < 0:
                speed = 0
        self.hal.turretSpeed = speed

        if self.input.grabToggle: self.hal.grabberOpen = not self.hal.grabberOpen
        if self.input.brakeToggle: self.hal.brakesExtended = not self.hal.brakesExtended

    def driveArmGoal(self, liftgoal:float, retractgoal:float) -> None:
        self.hal.retractSpeed = -self.retractcontroller.tick(retractgoal, self.hal.retractPos, self.time.dt)
        self.hal.liftSpeed = self.liftcontroller.tick(liftgoal, self.hal.liftPos, self.time.dt)

    def driveUnif(self, speed: float) -> None:
        for i in range(4): self.hal.driveSpeeds[i] = speed

    def driveTurn(self, speed:float) -> None:
        self.hal.driveSpeeds = [ speed, -speed, speed, -speed ]

    def driveArmSpeed(self, retractspeed: float, liftspeed: float) -> None:
        self.hal.retractSpeed = retractspeed
        self.hal.liftSpeed = liftspeed

    def autonomousInit(self) -> None:
        self.hal.resetEncoders()
        self.hal.gyroYaw = 0

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
        self.hal.liftSpeed = 0
        self.hal.retractSpeed = 0
        self.hal.turretSpeed = 0

if __name__ == "__main__":
    wpilib.run(Flymer)



