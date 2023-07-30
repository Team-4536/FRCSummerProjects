


import wpimath.estimator
import wpimath.kinematics
from wpimath.kinematics import SwerveModulePosition;
from wpimath.geometry import Translation2d, Pose2d, Rotation2d
import numpy
import math
from real import V2f


class SwerveEstimator:

    def __init__(self) -> None:

        wheelTranslations = (
            Translation2d(-1, 1),
            Translation2d(1, 1),
            Translation2d(-1, -1),
            Translation2d(1, -1)
        )

        # initial wheel states
        self.wheelStates = (
            SwerveModulePosition(0, Rotation2d(0)),
            SwerveModulePosition(0, Rotation2d(0)),
            SwerveModulePosition(0, Rotation2d(0)),
            SwerveModulePosition(0, Rotation2d(0))
        )

        kine = wpimath.kinematics.SwerveDrive4Kinematics(*wheelTranslations)

        self.est = wpimath.estimator.SwerveDrive4PoseEstimator(
            kine,
            Rotation2d(0),
            self.wheelStates,
            Pose2d()
            )

        self.estimatedPose = V2f(0, 0)

    def update(self, curTime: float, gyroAngleCWDeg: float, positionsM: list[float], anglesDEGCW: list[float]):
        r = Rotation2d(math.radians(-gyroAngleCWDeg)) # wpi uses CCW angles

        for i in range(0, 4):
            self.wheelStates[i].angle = Rotation2d(-math.radians(anglesDEGCW[i]))
            self.wheelStates[i].distance = positionsM[i]

        nPose = self.est.updateWithTime(curTime, r, self.wheelStates)
        self.estimatedPose = V2f(nPose.X(), nPose.Y())
        return self.estimatedPose

# TODO: reset button from sundial




