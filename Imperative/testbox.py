import wpilib
import rev

class TestInputs():
    def __init__(self, mainCtrlr: wpilib.XboxController) -> None:
        self.motorSwitchDown = mainCtrlr.getLeftBumperPressed()
        self.motorSwitchUp = mainCtrlr.getRightBumperPressed()
        self.leftMainX = mainCtrlr.getLeftX()
        self.leftMainY = mainCtrlr.getLeftY()


class TestBox(wpilib.TimedRobot):
    def robotInit(self) -> None:
        #motors======================================================
        #BRUSHLESS = rev.CANSparkMax.MotorType.kBrushless
        BRUSHED = rev.CANSparkMax.MotorType.kBrushed
        
        #demobot shooter code (untested)=============================
        #self.shooterLeft = rev.CANSparkMax(1,BRUSHED)
        #self.shooterRight = rev.CANSparkMax(2,BRUSHED)
    
        
        self.motors = []
        for i in range(8):
            self.motors.append(rev.CANSparkMax(i, BRUSHED))
        
        
        #controllers=================================================
        self.mainCtrlr = wpilib.XboxController(0)

        #variables===================================================
        self.motorAccessed = 0







    def robotPeriodic(self) -> None:
        self.input = TestInputs(self.mainCtrlr)
        
        return super().robotPeriodic()
    
    
    def teleopPeriodic(self) -> None:
        
        if(self.input.motorSwitchUp):
            self.motorAccessed += 1
        elif(self.input.motorSwitchDown):
            self.motorAccessed -= 1
        self.motorAccessed = self.motorAccessed % 8

        self.motors[self.motorAccessed].set(self.input.leftMainX * 0.15)

        #demobot code====================================
        # if(self.motorAccessed == 1):
        #     self.shooterLeft.set(self.input.leftMainY * 0.01)
        #     self.shooterRight.set(self.input.leftMainY * -0.01)
       

        return super().teleopPeriodic()
    
    
    def disabledPeriodic(self) -> None:
        return super().disabledPeriodic()
