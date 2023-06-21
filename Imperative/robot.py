import wpilib
import rev
import navx

import mechController





class Robot(wpilib.TimedRobot):


    def robotInit(self) -> None:

        # DRIVE MOTORS ==================================================
        driveType = rev.CANSparkMax.MotorType.kBrushless
        self.FLDrive = rev.CANSparkMax(0, driveType)
        self.FRDrive = rev.CANSparkMax(0, driveType)
        self.BLDrive = rev.CANSparkMax(0, driveType)
        self.BRDrive = rev.CANSparkMax(0, driveType)
        self.FLEncoder = self.FLDrive.getEncoder()
        self.FREncoder = self.FRDrive.getEncoder()
        self.BLEncoder = self.BLDrive.getEncoder()
        self.BREncoder = self.BRDrive.getEncoder()
        self.FRDrive.setInverted(True)
        self.BRDrive.setInverted(True)

        # GYRO ==========================================================
        self.gyro = navx.AHRS(wpilib.SPI.Port.kMXP)




    def robotPeriodic(self) -> None:

        driveSpeeds = mechController.mechController()

    def teleopInit(self) -> None:
        return super().teleopInit()

    def teleopPeriodic(self) -> None:
        return super().teleopPeriodic()





if __name__ == "__main__":
    wpilib.run(Robot)


