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

    def stopMotors(self) -> None:
        self.driveSpeeds = [ 0, 0, 0, 0 ]
        self.liftSpeed = 0
        self.retractSpeed = 0
        self.turretSpeed = 0

    def publish(self, server: socketing.Server, prefix: str):
        for k in self.__dict__:
            prop = self.__getattribute__(k)
            if type(prop) == float or type(prop) == int or type(prop) == bool:
                server.putUpdate(prefix + k, prop)

        prefs = [ "FL", "FR", "BL", "BR" ]
        for i in range(0, 4):
            server.putUpdate(prefix + prefs[i] + "Speed", self.driveSpeeds[i])
            server.putUpdate(prefix + prefs[i] + "Pos", self.drivePositions[i])

    def sanitize(self, boundRetract: bool) -> None:
        # (arm down = power+, encoder+)
        if self.liftTopSidePressed and self.liftSpeed < 0: # clamp speed if in upper danger zone
            self.liftSpeed = 0.1
        if self.liftTopPressed: # clamp speed to be positive (down) in on top switch
            self.liftSpeed = max(self.liftSpeed, 0)
        if self.liftBottomPressed: # clamp speed to be negative (up) if hitting bottom
            self.liftSpeed = min(self.liftSpeed, 0)

        # (arm in/retract = power+, encoder-)
        if self.retractPos > 2500 and self.retractPos < 10000 and boundRetract and self.retractSpeed < 0:
            selfretractSpeed = 0
        if self.retractPos > 10000 and self.retractSpeed > 0 and boundRetract:
            selfretractSpeed = 0

        # (CW = power+, encoder+)
        if self.rightLimitPressed and self.turretSpeed > 0:
            self.turretSpeed = 0
        if self.leftLimitPressed and self.turretSpeed < 0:
            self.turretSpeed = 0



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
        self.driveMotors[1].setInverted(True)
        self.driveMotors[3].setInverted(True)

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
        self.chooser.addOption(AUTO_BALANCE, AUTO_BALANCE)
        self.chooser.addOption(AUTO_EXIT, AUTO_EXIT)
        self.chooser.addOption(AUTO_EXIT_SCORE, AUTO_EXIT_SCORE)  # code code code
        wpilib.SmartDashboard.putData("autos", self.chooser)

        self.server = socketing.Server(self.isReal())

        self.retractcontroller = PIDController(0.006, 0, 0)
        self.liftcontroller = PIDController(0.004, 0, 0)

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
        #reset gyro
        self.hal.gyroYaw = 0

        #create variables
        self.armHoming = False
        self.scoringHigh = False
        self.scoringMid = False
        self.retractBounded = True



    def teleopPeriodic(self) -> None:
        self.input = FlymerInputs(self.driveCtrlr, self.armCtrlr)


        """ ===== DRIVE CONTROLS ==== """
        #absolute drive toggle button
        if self.input.absToggle:
            self.absoluteDrive = not self.absoluteDrive

        #speed controller bounds
        speedControl = self.input.speedControl
        if speedControl > 1:
            speedControl = 1
        driveVec = V2f(self.input.driveX, self.input.driveY) * speedControl
        if self.absoluteDrive: driveVec = driveVec.rotateDegrees(self.hal.gyroYaw)
        self.hal.driveSpeeds = mechController(driveVec.x, driveVec.y, self.input.turning * self.turningScalar)



        if self.input.gyroReset: self.hal.gyroYaw = 0

        if self.input.leftBumper:
            self.retractBounded = False
            self.hal.retractPos = 0
        else:
            self.retractBounded = True
        self.server.putUpdate("bounded", self.retractBounded)





        if self.input.homeArm: self.armHoming = True
        elif self.input.coneHigh and self.hal.liftTopPressed: self.scoringHigh = True
        #elif self.input.coneMid and self.liftUpperLimit.get(): self.scoringMid = True

        if self.input.lift != 0 or self.input.retract != 0 or self.retractBounded == False:
            self.armHoming = False
            self.scoringHigh = False
            self.scoringMid = False


        """----------arm setpoints----------"""
        if self.armHoming or self.scoringHigh or self.scoringMid:
            # SETPOINT CONTROLS
            #home
            if self.armHoming:
                retractError = -self.hal.retractPos

                if self.hal.retractPos < 2500:
                    self.hal.retractSpeed = -self.retractcontroller.tickErr(retractError, self.time.dt)
                else: self.hal.retractSpeed = 0
                self.hal.liftSpeed = -1

                if abs(retractError) < 10 and self.hal.liftTopPressed: self.armHoming = False

            #score cone high
            if self.scoringHigh:
                liftError = 3.407 - self.hal.liftPos
                retractError = 2126 - self.hal.retractPos

                self.hal.liftSpeed = self.liftcontroller.tickErr(liftError, self.time.dt)
                self.hal.retractSpeed = -self.retractcontroller.tickErr(retractError, self.time.dt)
                if abs(liftError) < 50 and abs(retractError) < 10: self.scoringHigh = False

            """
            #score cone middle (DOES NOT WORK - DO NOT USE)
            if self.scoringMid:
                liftError = 0 #change to targets and stuff
                retractError = 0

                liftSpeed = self.liftcontroller.tickErr(liftError, self.time.dt)
                retractSpeed = -self.retractcontroller.tickErr(retractError, self.time.dt)
            else: self.scoringMid = False
            """

        ## REGULAR ARM CONTROLS
        else:
            # (arm down = power+, encoder+)
            self.hal.liftSpeed = self.input.lift * 0.5

            if self.hal.liftTopPressed:
                self.hal.liftPos = 0

            # (arm in/retract = power+, encoder-)
            self.hal.retractSpeed = self.input.retract * 0.5

            # (CW = power+, encoder+)
            self.hal.turretSpeed = self.input.turret * 0.1



        """---------pneumatics---------"""
        if self.input.grabToggle: self.hal.grabberOpen = not self.hal.grabberOpen
        if self.input.brakeToggle: self.hal.brakesExtended = not self.hal.brakesExtended

        self.hal.sanitize(self.retractBounded)  # ensure nothing on the robot is going to destoy itself
        self.hardware.update(self.hal) # push speed values out to motors



    def getArmPower(self, liftError: float, retractError: float):
        liftSpeed = self.liftcontroller.tickErr(liftError, self.time.dt)
        retractSpeed = -self.retractcontroller.tickErr(retractError, self.time.dt)
        return liftSpeed, retractSpeed

    def driveArmGoal(self, liftgoal: float, retractgoal: float) -> None:
        retractspeed = -self.retractcontroller.tick(retractgoal, self.hal.retractPos, self.time.dt)
        liftspeed = self.liftcontroller.tick(liftgoal, self.hal.liftPos, self.time.dt)

        self.hal.retractSpeed = retractspeed
        self.hal.liftSpeed = liftspeed

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
        self.scoregoal = V2f(1893, 700)
        self.defaultgoal = V2f(0, 0)

        stagelist = []
        scorelist = [autoStaging.extend, autoStaging.score, autoStaging.retreat, autoStaging.turn]

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
        self.hal.stopMotors()
        self.auto.update(self)
        self.hal.sanitize(True)
        self.hardware.update(self.hal)

    def disabledPeriodic(self) -> None:
        self.hal.stopMotors()
        self.hardware.update(self.hal)


if __name__ == "__main__":
    wpilib.run(Flymer)
