import wpilib
import rev

class TestInputs():
    def __init__(self, mainCtrlr: wpilib.XboxController) -> None:
        self.motorSwitch = mainCtrlr.getYButtonPressed()
        self.leftMainX = mainCtrlr.getLeftX()
        self.leftMainY = mainCtrlr.getLeftY()
        self.rightMainX = mainCtrlr.getRightX()
        self.rightMainY = mainCtrlr.getRightY()
        self.xButtonPressed = mainCtrlr.getXButtonPressed()
        self.aButtonPressed = mainCtrlr.getAButtonPressed()
        self.bButtonPressed = mainCtrlr.getBButtonPressed()
        pass


class TestBox(wpilib.TimedRobot):
    def robotInit(self) -> None:
        #motors======================================================
        BRUSHLESS = rev.CANSparkMax.MotorType.kBrushless
        BRUSHED = rev.CANSparkMax.MotorType.kBrushed
        self.shooterLeft = rev.CANSparkMax(1,BRUSHED)
        self.shooterRight = rev.CANSparkMax(2,BRUSHED)
        self.motor1 = rev.CANSparkMax(3,BRUSHED)
        self.motor2 = rev.CANSparkMax(4,BRUSHED)
        self.motor3 = rev.CANSparkMax(5,BRUSHED)
        self.motor4 = rev.CANSparkMax(6,BRUSHED)
        self.motor5 = rev.CANSparkMax(7,BRUSHED)
        self.motor6 = rev.CANSparkMax(8,BRUSHED)
        #controllers=================================================
        self.mainCtrlr = wpilib.XboxController(0)

        #variables===================================================
        self.motorAccessed = 1







    def robotPeriodic(self) -> None:
        self.input = TestInputs(self.mainCtrlr)
        
        return super().robotPeriodic()
    
    
    def teleopPeriodic(self) -> None:
        #switching which motor is accessed
        if(self.input.motorSwitch == True):
            if(self.motorAccessed<7)
                self.motorAccessed = self.motorAccessed + 1
            else:self.motorAccessed = 1
        #controlling the motor
        if(self.motorAccessed == 1):
            self.shooterLeft.set(self.input.leftMainY * 0.01)
            self.shooterRight.set(self.input.leftMainY * -0.01)
        elif(self.motorAccessed == 2):
            self.motor1.set(self.input.)
        elif(self.motorAccessed == 3):
            self.motor2.set(self.input.)
        elif(self.motorAccessed == 4):
            self.motor3.set(self.input.)
        elif(self.motorAccessed == 5):
            self.motor4.set(self.input.)
        elif(self.motorAccessed == 6):
            self.motor5.set(self.input.)
        elif(self.motorAccessed == 7):
            self.motor6.set(self.input.)

        return super().teleopPeriodic()
    
    
    def disabledPeriodic(self) -> None:
        return super().disabledPeriodic()
