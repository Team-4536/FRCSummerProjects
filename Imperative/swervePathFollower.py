import wpilib
import rev
import math

from PIDController import PIDController
from real import V2f, angleWrap


class TrajectoryPt2d:
    def __init__(self, pt: V2f, speed: float) -> None:
        self.pt = pt
        self.speed = speed


def lerp(a, b, t):
    return a+((b-a)*t)

class Spline2d():
    def __init__(self, points: list[tuple[V2f, V2f, V2f, V2f]]) -> None:
        self.points = points

    def sample(self, t: float):
        idx: int = math.floor(t * len(self.points))
        pct = (t * len(self.points)) % 1

        pts = self.points[idx]
        a = lerp(pts[0], pts[1], pct)
        b = lerp(pts[2], pts[3], pct)
        return lerp(a, b, pct)

    # Speed == 1 is the segment with the greatest distance
    # others are proportional
    def getTrajectory(self, count: int) -> list[TrajectoryPt2d]:
        out: list[TrajectoryPt2d] = []
        prevpt = self.sample(0)
        out.append(TrajectoryPt2d(prevpt, 0))
        maxLen = 0.0
        for i in range(1, count):
            pt = self.sample(i / count)
            l = (pt - prevpt).getLength()
            maxLen = max(l, maxLen)
            out[-1].speed = l
            out.append(TrajectoryPt2d(pt, 0))
            prevpt = pt

        for t in out:
            t.speed /= maxLen

        return out


