import wpilib
import rev
import math

from PIDController import PIDController
from real import V2f, angleWrap, lerp, invLerp



# this file has functions for generating paths

# points may not be eveny distributed
# each tuple of four takes up a uniform amount of t-space
# param T must be within 0 and 1 otherwise wacky stuff will happen
def getSpline2dSample(pts: tuple[V2f, V2f, V2f, V2f], t: float) -> V2f:

    a = V2f.lerp(pts[0], pts[1], t)
    b = V2f.lerp(pts[1], pts[2], t)
    c = V2f.lerp(pts[2], pts[3], t)

    d = V2f.lerp(a, b, t)
    e = V2f.lerp(b, c, t)
    return V2f.lerp(d, e, t)

def getSplineLength(points:tuple[V2f, V2f, V2f, V2f]):
    totald = 0;
    for i in range(10):
        perc4000 = i/10;
        sample = getSpline2dSample(points, perc4000);
        sample2 = getSpline2dSample(points, perc4000+1/10);
        disty = (sample2-sample).getLength();
        totald+=disty;
    return totald;

def getPathLength(points:list[tuple[V2f,V2f,V2f,V2f]]) -> float:
    totaldisty = 0
    for i in points:
        perc5000 = getSplineLength(i);
        totaldisty += perc5000;
    return totaldisty;

def getPathSample(points:list[tuple[V2f,V2f,V2f,V2f]], t) -> V2f:

    length = getPathLength(points)
    distance = length*t;

    for curve in points:
        if distance > getSplineLength(curve):
            distance-=getSplineLength(curve);
            continue
        else:
            perc9000 = distance/getSplineLength(curve);
            return getSpline2dSample(curve, perc9000);
    return points[-1][3]


# domain of pts expected to lie within 0-1 range
# X vals are expected to increase from pt 0
# t must be within 0 and 1
# must be at least one point
def getLinear2dSample(points: list[V2f], t: float) -> float:

    idx = 1
    while True:
        if(idx > len(points)):
            return points[-1].y
        else:
            if(points[idx].x < t):
                idx += 1
            else:
                pct = invLerp(points[idx-1].x, points[idx].x, t)
                s = lerp(points[idx-1].y, points[idx].y, pct)
                return s


# NOTE: extremely unsafe and will throw like 30 errors on a bad file
def loadPath(filePath: str) -> list[V2f]:
    f = open(filePath)
    s = f.read()

    lines = s.split('\n')
    components = [l.split(', ') for l in lines]

    out: list[V2f] = []
    for l in components[0:-1]:
        flts = []
        assert(len(l) == 2)
        for c in l:
            flts.append(float(c))
        out.append(V2f(flts[0], flts[1]))

    return out
