import ntcore
import wpilib
import rev
import navx

import mechController
import inputs





class Robot(wpilib.TimedRobot):


    def robotInit(self) -> None:

        self.telemTable = ntcore.NetworkTableInstance.getDefault().getTable("telemetry")

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

        # CONTROLLERS ====================================================
        self.driveCtrlr = wpilib.XboxController(0)
        self.armCtrlr = wpilib.XboxController(1)




    def robotPeriodic(self) -> None:
        self.input = inputs.FlymerInputs(self.driveCtrlr, self.armCtrlr)

        self.driveSpeeds = mechController.mechController(
            self.input.driveX,
            self.input.driveY,
            self.input.turning)

        self.FLDrive.set(self.driveSpeeds[0])
        self.FRDrive.set(self.driveSpeeds[1])
        self.BLDrive.set(self.driveSpeeds[2])
        self.BRDrive.set(self.driveSpeeds[3])


    def teleopInit(self) -> None:
        return super().teleopInit()

    def teleopPeriodic(self) -> None:
        return super().teleopPeriodic()





if __name__ == "__main__":
    wpilib.run(Robot)


