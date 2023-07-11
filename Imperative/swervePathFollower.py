import wpilib
import rev

from PIDController import PIDController
from real import V2f, angleWrap






class SwervePathFollower:

    DISTANCE_CUTOFF = 0.5
    ANGLE_CUTOFF = 30

    def __init__(self, poses: list[tuple[V2f, float]]) -> None:

        self.poses = poses
        self.index = 0

        kp = 0.15
        ki = 0
        kd = -0.2
        self.xPID = PIDController(kp, ki, kd)
        self.yPID = PIDController(kp, ki, kd)

        self.tPID = PIDController(0.01, 0, 0)


    # returns a tuple with drive speed and turning speed
    def tick(self, position: V2f, angle: float, dt: float) -> tuple[V2f, float]:

        if(self.index >= len(self.poses)):
            return (V2f(0, 0), 0)

        targetPose = self.poses[self.index]

        # TODO: tighter cutoff for final poses?
        if(abs((position - targetPose[0]).getLength()) < SwervePathFollower.DISTANCE_CUTOFF):
            if(abs(angleWrap(targetPose[1] - angle)) < SwervePathFollower.ANGLE_CUTOFF):
                self.index += 1

        if(self.index >= len(self.poses)):
            return (V2f(0, 0), 0)


        x = self.xPID.tick(targetPose[0].x, position.x, dt)
        y = self.xPID.tick(targetPose[0].y, position.y, dt)
        t = self.tPID.tick(targetPose[1], angle, dt)

        return V2f(x, y), t


















