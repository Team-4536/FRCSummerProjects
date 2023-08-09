import wpilib
import rev
import math

from PIDController import PIDController
from real import V2f, angleWrap



# this file has functions for generating paths




def lerp(a, b, t):
    return a+((b-a)*t)

def invLerp(a, b, pt):
    return (pt-a)/(b-a)


# samples taken with uniform t values, meaning that points may not be eveny distributed
def getSpline2dPoints(points: list[tuple[V2f, V2f, V2f, V2f]], count: int) -> list[V2f]:
    out: list[V2f] = []

    for i in range(0, count):
        t = i/count
        idx: int = math.floor(t * len(points))
        pct = (t * len(points)) % 1

        pts = points[idx]
        a = lerp(pts[0], pts[1], pct)
        b = lerp(pts[2], pts[3], pct)
        out.append(lerp(a, b, pct))

    return out


# domain of pts expected to lie within 0-1 range
# X vals are expected to increase from pt 0
def getLinear2dPoints(points: list[V2f], count: int) -> list[float]:
    out: list[float] = []
    idx = 1
    for i in range(0, count):
        t = i / count

        while True:
            if(idx > len(points)):
                s = points[-1].y
                break
            else:
                if(points[idx].x < t):
                    idx += 1
                else:
                    pct = invLerp(points[idx-1].x, points[idx].x, t)
                    s = lerp(points[idx-1].y, points[idx].y, pct)
                    break

        out.append(s)
    return out