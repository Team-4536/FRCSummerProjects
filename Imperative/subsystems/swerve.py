import rev
import wpimath.system.plant as plant
import math

from virtualGyro import VirtualGyro
from real import V2f, angleWrap, normalizeWheelSpeeds
from encoderSim import EncoderSim
from PIDController import PIDController

from socketing import Server

import wpimath
import wpimath.estimator
import wpimath.kinematics
from wpimath.kinematics import SwerveModulePosition, SwerveDrive4Odometry;
from wpimath.geometry import Translation2d, Pose2d, Rotation2d
import ctre.sensors
import wpilib
import navx

#TODO: make module class, fix imports

class goofyAhh(wpilib.timedRobot):

    def robotInit(self) -> None:
        
        self.FL = Translation2d(.1,.1)
        self.FR = Translation2d(.1,-.1)
        self.BL = Translation2d(-.1,.1)
        self.BR = Translation2d(-.1,-.1)
        self.kinematics = wpimath.kinematics(self.FL,  self.FR,  self.BL,  self.BR)
        self.gyroAngle = navx.AHRS(wpilib.SPI.Port.kMXP).getYaw()
        self.initPos = Pose2d()

        self.odom = SwerveDrive4Odometry(self.kinematics, self.gyroAngle, (self.FL,  self.FR,  self.BL,  self.BR), self.initPos)

    def robotPeriodic(self) -> None:
        self.odom.update(self.gyroAngle)