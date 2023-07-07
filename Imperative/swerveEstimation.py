


import wpimath.estimator
import wpimath.kinematics
from wpimath.kinematics import SwerveModulePosition;
from wpimath.geometry import Translation2d, Pose2d, Rotation2d

import math


class SwerveEstimator:

    def __init__(self) -> None:
        wheelDist = math.sqrt(1**2 + 1**2)

        wheelTranslations = (
            Translation2d(-1, 1),
            Translation2d(1, 1),
            Translation2d(-1, -1),
            Translation2d(1, -1)
        )

        self.wheelPositions = (
            # initial wheel states
            SwerveModulePosition(0, Rotation2d(0)),
            SwerveModulePosition(0, Rotation2d(0)),
            SwerveModulePosition(0, Rotation2d(0)),
            SwerveModulePosition(0, Rotation2d(0))
        )

        kine = wpimath.kinematics.SwerveDrive4Kinematics(*wheelTranslations)

        self.est = wpimath.estimator.SwerveDrive4PoseEstimator(
            kine,
            Rotation2d(0),
            self.wheelPositions,
            Pose2d()
            )

    def update(self, curTime: float, gyroAngleCWDeg: float, positionsM: list[float], anglesDEGCW: list[float]):
        r = Rotation2d(math.radians(-gyroAngleCWDeg))

        for i in range(0, 4):
            self.wheelPositions[i].angle = Rotation2d(-math.radians(anglesDEGCW[i]))
            self.wheelPositions[i].distance = positionsM[i]

        nPose = self.est.updateWithTime(curTime, r, self.wheelPositions)
        return nPose







