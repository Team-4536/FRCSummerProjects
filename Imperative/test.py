

import wpilib
import rev
from socketing import Server
import inputs
import timing
import ctre.sensors
import PIDController
from real import angleWrap
from swerveBot import SwerveState
from real import V2f
from subsystems.swerve import SwerveController
import navx


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
        
    def teleopPeriodic(self) -> None:

        driveStick = V2f(-self.controller.getLeftY(), self.controller.getLeftX()) * .3
        #driveStick = driveStick.rotateDegrees(-self.gyro.getYaw())
        
        self.swerveController.tick(driveStick.x, driveStick.y, self.controller.getRightX() * 0.2, self.time.dt, self.controller.getBButtonPressed(), self.controller.getYButtonPressed, self.swerve, self.gyro)

        """
        forward = -inputs.deadZone(self.controller.getLeftY())
        turn = inputs.deadZone(self.controller.getRightX())
        # self.target += turn * 0.5 * self.time.dt
        if(self.controller.getPOV() != -1):
            self.target = self.controller.getPOV() / 360

        self.drive.set(forward * 0.1)
        error = angleWrap(self.target*360 - self.se.getPosition()) / 360
        self.server.putUpdate("error", error)
        self.steer.set(self.pid.tickErr(-error, self.time.dt))
        """
    

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