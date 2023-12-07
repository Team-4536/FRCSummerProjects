import wpilib
import rev


class TestBox(wpilib.TimedRobot):
    def robotInit(self) -> None:
        #motors======================================================
        BRUSHLESS = rev.CANSparkMax.MotorType.kBrushless
        BRUSHED = rev.CANSparkMax.MotorType.kBrushed
        self.shooterLeft = rev.CANSparkMax(1,BRUSHED)
        self.shooterRight = rev.CANSparkMax(2,BRUSHED)
        #controllers=================================================
    def robotPeriodic(self) -> None:
        return super().robotPeriodic()
    def teleopPeriodic(self) -> None:
        return super().teleopPeriodic()
    def disabledPeriodic(self) -> None:
        return super().disabledPeriodic()
