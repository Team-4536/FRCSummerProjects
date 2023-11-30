import ntcore
import wpilib
import wpimath.system.plant as plant
import rev
import navx
import wpimath
from wpimath import kinematics
from wpimath import geometry
import timing
from real import V2f
from subsystems.mech import mechController
import socketing
from inputs import deadZone





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

class Flymer(wpilib.TimedRobot):


    def robotInit(self) -> None:

        # GYRO ==========================================================
        self.gyro = navx.AHRS(wpilib.SPI.Port.kMXP)

        # CONTROLLERS ====================================================

        self.driveCtrlr = wpilib.XboxController(0)
        self.armCtrlr = wpilib.XboxController(1)

        # TIME =========================================================
        self.armMotor = rev.CANSparkMax(0, rev.CANSparkMax.MotorType.kBrushless)
        self.liftMotor = rev.CANSparkMax(0, rev.CANSparkMax.MotorType.kBrushless)
        self.time = timing.TimeData(None)
        self.frontLeftWheel = rev.CANSparkMax(0, rev.CANSparkMax.MotorType.kBrushless)
        self.frontRightWheel = rev.CANSparkMax(0, rev.CANSparkMax.MotorType.kBrushless)
        self.rearLeftWheel = rev.CANSparkMax(0, rev.CANSparkMax.MotorType.kBrushless)
        self.rearRightWheel = rev.CANSparkMax(0, rev.CANSparkMax.MotorType.kBrushless)
        #wipmath.geometry.Tra
        self.initialPose = geometry._geometry.Pose2d(geometry.Translation2d(0, 0), geometry.Rotation2d(0))
        
        self.pose = geometry._geometry.Pose2d

        self.kinematics = kinematics.MecanumDriveKinematics(frontLeftWheel = geometry.Translation2d(-1, 1), frontRightWheel = geometry.Translation2d(1, 1), rearLeftWheel = geometry.Translation2d(-1, -1), rearRightWheel = geometry.Translation2d(1, -1))
        self.gyroAngle = self.gyro.getRotation2d()
        self.wheelPositions = kinematics.MecanumDriveWheelPositions()
        self.wheelPositions.frontLeft = 0
        self.wheelPositions.frontRight = 0
        self.wheelPositions.rearLeft = 0
        self.wheelPositions.rearRight = 0

        self.flymerPos = kinematics.MecanumDriveOdometry(self.kinematics, self.gyroAngle, self.wheelPositions, self.initialPose)

    
    def robotPeriodic(self) -> None:

        self.time = timing.TimeData(self.time)

        self.server = socketing.Server(self.isReal())
        self.server.update(self.time.timeSinceInit)

        self.flymerPos.update(self.gyroAngle, self.wheelPositions)

    def teleopPeriodic(self) -> None:

        self.input = FlymerInputs(self.driveCtrlr, self.armCtrlr)
        self.armMotor.set(self.input.turret * 0.01)
        self.liftMotor.set(self.input.lift*0.05)


    def disabledPeriodic(self) -> None: 
        self.armMotor.set(0)
        self.liftMotor.set(0)



if __name__ == "__main__":
    wpilib.run(Flymer)



