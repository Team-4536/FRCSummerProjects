

import wpilib
import rev
from socketing import Server
import inputs
import timing
import ctre.sensors
from PIDController import PIDController
from real import angleWrap
from swerveBot import SwerveState
from real import V2f
from subsystems.swerve import SwerveController
import navx
from ctre.led import CANdle
import ctre.led


robotInst = None

class swerveTest(wpilib.TimedRobot):

    def robotInit(self) -> None:
        robotInst = self

        self.driveList = [rev.CANSparkMax(1, rev.CANSparkMax.MotorType.kBrushless), 
                          rev.CANSparkMax(3, rev.CANSparkMax.MotorType.kBrushless),
                          rev.CANSparkMax(5, rev.CANSparkMax.MotorType.kBrushless),
                          rev.CANSparkMax(7, rev.CANSparkMax.MotorType.kBrushless)
                          ]
        
        self.deList = [self.driveList[0].getEncoder(),
                       self.driveList[1].getEncoder(),
                       self.driveList[2].getEncoder(),
                       self.driveList[3].getEncoder()
                       ]

        self.steerList = [rev.CANSparkMax(2, rev.CANSparkMax.MotorType.kBrushless),
                          rev.CANSparkMax(4, rev.CANSparkMax.MotorType.kBrushless),
                          rev.CANSparkMax(6, rev.CANSparkMax.MotorType.kBrushless),
                          rev.CANSparkMax(8, rev.CANSparkMax.MotorType.kBrushless)
                          ]
        
        self.seList = [ctre.sensors.CANCoder(21),
                       ctre.sensors.CANCoder(22),
                       ctre.sensors.CANCoder(23),
                       ctre.sensors.CANCoder(24)
                       ]
        
        self.gyro = navx.AHRS(wpilib.SPI.Port.kMXP)
        
        tenInches = 0.254
        self.wheelOffSets = [
            V2f(tenInches, tenInches),
            V2f(tenInches, -tenInches),
            V2f(-tenInches, tenInches),
            V2f(-tenInches, -tenInches)
        ]
        
        self.controller = wpilib.XboxController(0)
        self.server = Server(self.isReal())
        self.time = timing.TimeData(None)

        # self.target = self.se.getPosition() / 360
        # expects ticks with rotations
        # self.pid = PIDController.PIDController(.9, 0, 0)
        
        self.swerve = SwerveState(14.2, self.wheelOffSets, 0.0508, self.driveList, self.steerList, self.deList, self.seList)
        self.swerveController = SwerveController()
        self.brakeDefault = False

        ledStart = 13
        ledLength = 76
        self.ledController = CANdle(20)

        self.rainbowLED = ctre.led.RainbowAnimation(255, .5, ledLength, False, ledStart)





    def robotPeriodic(self) -> None:
        self.time = timing.TimeData(self.time)

        # self.server.putUpdate("drive", self.drive.get() / 6.12)
        # self.server.putUpdate("steer", self.steer.get() / (150/7))
        """
        self.server.putUpdate("drive", self.drive.get())
        self.server.putUpdate("steer", self.steer.get())
        self.server.putUpdate("drivePos", self.de.getPosition() / 6.12)

        self.server.putUpdate("steerPos", self.se.getPosition() / 360)
        self.server.putUpdate("steerTarget", self.target)
        
        """

        self.server.putUpdate("FLPos", self.seList[0].getAbsolutePosition())
        self.server.putUpdate("FRPos", self.seList[1].getAbsolutePosition())
        self.server.putUpdate("BLPos", self.seList[2].getAbsolutePosition())
        self.server.putUpdate("BRPos", self.seList[3].getAbsolutePosition())

        self.server.update(self.time.timeSinceInit)

        """---leds---"""

        frontStartIdx, frontLength, frontLength2, backStartIdx, backLength, backLength2 = self.makeLEDsFollowGyro(76, 13, 87, 4, 4, self.gyro.getYaw(), 90)

        self.ledController.setLEDs(0, 0, 0, 0, 0, 88)

        self.ledController.setLEDs(0, 255, 0, 0, frontStartIdx, frontLength)
        self.ledController.setLEDs(0, 255, 0, 0, 13, frontLength2)

        self.ledController.setLEDs(255, 0, 0, 0, backStartIdx, backLength)
        self.ledController.setLEDs(255, 0, 0, 0, 13, backLength2)


    """----------------------------------------------------------"""



    def teleopInit(self) -> None:
        
        kp = 0.01
        ki = 0
        kd = 0

        self.pid = PIDController(kp, ki, kd)
        self.target = angleWrap(self.gyro.getYaw())



    def teleopPeriodic(self) -> None:

        # 0.3 = good for testing || 0.7 = for inexperienced drivers/indoor practice || 1.0 = competition and full field practice
        # speedScalar = 0.3 #speed control set as constant
        speedScalar = .1 + self.controller.getRightTriggerAxis() #speed control on trigger
        if speedScalar > 1: speedScalar = 1

        # 0.2 = good for testing || 0.3 = for inexperienced drivers/indoor practice || 0.4 = competition and fill field practice
        turnScalar = 0.2


        #inputs (change if drivers want them different)
        driveX = inputs.deadZone(self.controller.getLeftX())
        driveY = -inputs.deadZone(self.controller.getLeftY())
        turnSpeed = inputs.deadZone(self.controller.getRightX()) * turnScalar
        self.target += (inputs.deadZone(self.controller.getRightX()) * 2)
        driveStick = V2f(driveY, driveX) * speedScalar

        brakes = self.controller.getBButtonPressed()
        startButton = self.controller.getStartButtonPressed()
        gyroReset = self.controller.getYButtonPressed()

        if startButton:
            self.brakeDefault = not self.brakeDefault
            brakes = True #make sure this doesn't break anything

        #angle pid
        angle = angleWrap(self.gyro.getYaw())
        error = angleWrap(self.target-angle)
        # turnSpeed = self.pid.tickErr(error, self.time.dt) * turnScalar

        #make swerve move
        self.swerveController.tick(driveStick.x, driveStick.y, turnSpeed, self.time.dt, brakes, self.brakeDefault, gyroReset, self.swerve, self.gyro, self.ledController, self.server)


        """------LEDs------"""
        
        #ledID = 12 + number
        ledLength = 76
        ledStartIdx = 13
        ledEndIdx = 88
        totalLEDs = 88

        frontLeft = 22 #FL
        frontRight = 41 #FR
        backRight = 59 #BR
        backLeft = 78 #BL

        frontSideLength = 19 #FL
        rightSideLength = 19 #both corners
        backSideLength = 18 #None
        leftSideLength = 19 #BL

        avgLength = 18.75

        frontCenter = 31, 32
        degreesPerLed = 4.8
        lineThickness = 4
        gyroOffset = 90

        """
        frontStartIdx, frontLength, frontLength2, backStartIdx, backLength, backLength2 = self.makeLEDsFollowGyro(ledLength, ledStartIdx, ledEndIdx - 1, 4, 4, self.gyro.getYaw(), gyroOffset)

        self.ledController.setLEDs(0, 0, 0, 0, 0, 88)

        self.ledController.setLEDs(0, 255, 0, 0, frontStartIdx, frontLength)
        self.ledController.setLEDs(0, 255, 0, 0, ledStartIdx, frontLength2)

        self.ledController.setLEDs(255, 0, 0, 0, backStartIdx, backLength)
        self.ledController.setLEDs(255, 0, 0, 0, ledStartIdx, backLength2)

        
        ledGyro = round((((self.gyro.getYaw() + gyroOffset) %360) / 4.8))
        ledGyroInverted = round((((self.gyro.getYaw() + gyroOffset + 180) %360) / 4.8))

        outLength = 4
        outLength2 = 0
        outStartIdx = ledGyro - 1 + 12

        outLengthBack = 4
        outLength2Back = 0
        outStartIdxBack = ledGyroInverted - 1 + 12

        for i in range(3):
            if outStartIdx + i >= 88:
                outLength = i
                outLength2 = 4-i

        for i in range(3):
            if outStartIdxBack + i >= 88:
                outLengthBack = i
                outLength2Back = 4-i
        

        self.ledController.setLEDs(0, 0, 0, 0, 0, 88)

        self.ledController.setLEDs(0, 255, 0, 0, outStartIdx, outLength)
        self.ledController.setLEDs(0, 255, 0, 0, 13, outLength2)

        self.ledController.setLEDs(255, 0, 0, 0, outStartIdxBack, outLengthBack)
        self.ledController.setLEDs(255, 0, 0, 0, 13, outLength2Back)"""


    def makeLEDsFollowGyro(self, length: int, firstLed: int, lastLed: int, frontWidth: int, rearWidth: int, gyroAngle: float, gyroOffset: int):
        degreesPerLight = 360 - length

        ledGyro = round((((gyroAngle + gyroOffset) %360) / degreesPerLight))
        ledGyroInverted = round((((gyroAngle + gyroOffset + 180) %360) / degreesPerLight))

        outLength = frontWidth
        outLength2 = 0
        outStartIdx = ledGyro - frontWidth/2 + firstLed + 1

        outLengthBack = rearWidth
        outLength2Back = 0
        outStartIdxBack = ledGyroInverted - rearWidth/2 + firstLed + 1

        for i in range(frontWidth - 1):
            if outStartIdx + i > lastLed:
                outLength = i
                outLength2 = frontWidth-i

        for i in range(rearWidth - 1):
            if outStartIdxBack + i > lastLed:
                outLengthBack = i
                outLength2Back = rearWidth-i

        frontStartIdx = int(outStartIdx)
        frontLength = int(outLength)
        frontLength2 = int(outLength2)
        backStartIdx = int(outStartIdxBack)
        backLength = int(outLengthBack)
        backLength2 = int(outLength2Back)

        return frontStartIdx, frontLength, frontLength2, backStartIdx, backLength, backLength2
    
    
    """--------------------------------------------------------------------------------"""




    def disabledInit(self) -> None:
        self.disabledPeriodic()

    def disabledPeriodic(self) -> None:
        self.driveList[0].stopMotor()
        self.driveList[1].stopMotor()
        self.driveList[2].stopMotor()
        self.driveList[3].stopMotor()

        self.steerList[0].stopMotor()
        self.steerList[1].stopMotor()
        self.steerList[2].stopMotor()
        self.steerList[3].stopMotor()

if __name__ == "__main__":
    wpilib.run(swerveTest)