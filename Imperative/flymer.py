import ntcore
import wpilib
import wpilib.drive
from wpimath import geometry
import wpimath.system.plant as plant
import rev
import navx
import wpimath.kinematics
import timing
from real import V2f
from subsystems.mech import mechController
import socketing
from inputs import deadZone
from wpimath.geometry import Pose2d
from wpimath.geometry import Translation2d
from wpimath.geometry._geometry import Rotation2d
import wpilib.interfaces

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


    def __init__(self) -> None:

        #server==========================================================
        self.server = socketing.Server(self.isReal())
        # GYRO ==========================================================
        self.gyro = navx.AHRS(wpilib.SPI.Port.kMXP)
        self.gyro.reset()
        
        
        self.kinematics = wpimath.kinematics.MecanumDriveKinematics(geometry.Translation2d(-1,1),geometry.Translation2d(1,1),geometry.Translation2d(-1,-1), geometry.Translation2d(1,-1))
        self.initailpose = geometry._geometry.Pose2d(Translation2d(x=0.000000, y=0.000000), Rotation2d(0.000000))
        
        #DRIVE MOTORS====================================================
        BRUSHLESS = rev.CANSparkMax.MotorType.kBrushless
        self.flDrive = rev.CANSparkMax(4,BRUSHLESS)
        self.frDrive = rev.CANSparkMax(1,BRUSHLESS)
        self.blDrive = rev.CANSparkMax(3,BRUSHLESS)
        self.brDrive = rev.CANSparkMax(2,BRUSHLESS)
        self.MecanumDrive = wpilib.drive.MecanumDrive(self.flDrive,self.frDrive,self.blDrive,self.brDrive)
        # CONTROLLERS ===================================================

        self.driveCtrlr = wpilib.XboxController(0)
        self.armCtrlr = wpilib.XboxController(1)
        self.turretLeft = wpilib.DigitalInput(6)
        self.turretRight = wpilib.DigitalInput(5)
        self.armTop = wpilib.DigitalInput(2)
        

        # TIME =========================================================
        self.armMotor = rev.CANSparkMax(6, rev.CANSparkMax.MotorType.kBrushed)
        self.liftMotor = rev.CANSparkMax(7, rev.CANSparkMax.MotorType.kBrushed)
        self.time = timing.TimeData(None)
        
    def robotPeriodic(self) -> None:
        
        self.gyroAngle = self.gyro.getRotation2d() # known good
        flDriveEncoder = self.flDrive.getEncoder().getPosition() 
        frDriveEncoder = self.frDrive.getEncoder().getPosition()
        blDriveEncoder = self.blDrive.getEncoder().getPosition()
        brDriveEncoder = self.brDrive.getEncoder().getPosition()
        self.wheelsEncoded = [flDriveEncoder,frDriveEncoder, blDriveEncoder, brDriveEncoder]

        
        self.time = timing.TimeData(self.time)
       # self.gyroAngle = geometry._geometry.Rotatio
        wheel_positions = wpimath.kinematics._kinematics.MecanumDriveWheelPositions()
    
        wheel_positions.frontLeft  = flDriveEncoder
        wpimath.kinematics._kinematics.MecanumDriveWheelPositions.frontRight = frDriveEncoder
        wpimath.kinematics._kinematics.MecanumDriveWheelPositions.rearLeft = blDriveEncoder
        wpimath.kinematics._kinematics.MecanumDriveWheelPositions.rearRight = brDriveEncoder
        # wpimath.kinematics._kinematics.MecanumDriveWheelPositions = (wpimath.kinematics._kinematics.MecanumDriveWheelPositions.rearRight, wpimath.kinematics._kinematics.MecanumDriveWheelPositions.rearLeft, wpimath.kinematics._kinematics.MecanumDriveWheelPositions.frontRight, wpimath.kinematics._kinematics.MecanumDriveWheelPositions.frontLeft)

        self.pose = self.kinematics.update(self.gyroAngle, wheel_positions)


        # self.pose = wpimath.kinematics.MecanumDriveOdometry.getPose()
        

        
        self.server.update(self.time.timeSinceInit)

    def teleopPeriodic(self) -> None:

        self.input = FlymerInputs(self.driveCtrlr, self.armCtrlr)
        self.armMotor.set(self.input.turret * 0.01)
        self.liftMotor.set(self.input.lift*0.05)

        
        self.MecanumDrive.driveCartesian(self.input.driveY*0.15,self.input.driveX*0.15,self.input.turning*0.15,wpimath.geometry._geometry.Rotation2d(self.gyro.getYaw()*(2*3.14/360)))
        
    


    def disabledPeriodic(self) -> None: 
        self.armMotor.set(0)
        self.liftMotor.set(0)



if __name__ == "__main__":
    wpilib.run(Flymer)



