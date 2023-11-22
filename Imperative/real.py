from __future__ import annotations
import math

def lerp(a: float, b: float, t: float) -> float:
    return a + (b-a)*t

def invLerp(a, b, pt):
    return (pt-a)/(b-a)


class V2f:

    def __init__(self, x: float = 0, y: float = 0) -> None:
        self.x = x
        self.y = y


    def rotateDegrees(self, d):
        d *= -1
        cos = math.cos(d * (math.pi / 180))
        sin = math.sin(d * (math.pi / 180))

        return V2f(
            self.x * cos - self.y * sin,
            self.y * cos + self.x * sin)


    def getLength(self):
        length = math.sqrt((self.x**2 + self.y**2))
        return float(length)


    def getAngle(self):
        angle = -(math.atan2(self.y, self.x)) * 57.2958 #radians to angle conversion factor
        return float(angle)

    def getNormalized(self):
        l = self.getLength()
        if(l != 0): return self / l
        else: return V2f()



    def __add__(self, b): return V2f(self.x + b.x, self.y + b.y)
    def __sub__(self, b): return V2f(self.x - b.x, self.y - b.y)
    def __neg__(self): return V2f(-self.x, -self.y)
    def __mul__(self, s): return V2f(self.x * s, self.y * s)
    def __truediv__(self, s): return V2f(self.x / s, self.y / s)
    def __eq__(self, other): return (self.x == other.x) and (self.y == other.y)
    def __ne__(self, other): return not (self == other)

    @staticmethod
    def lerp(a: V2f, b: V2f, t: float) -> V2f:
        return V2f(lerp(a.x, b.x, t), lerp(a.y, b.y, t))



# CLEANUP: this
def angleWrap(a: float) -> float:
    while a > 180: a -= 360
    while a < -180: a += 360
    return a


def normalizeWheelSpeeds(l: list[float]) -> list[float]:
    m = 0
    for speed in l:
        if abs(speed) > m: m = abs(speed)

    if(m > 1):
        return [s / m for s in l]
    return l