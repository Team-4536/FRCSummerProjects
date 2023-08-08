
import math


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
        angle = -(math.atan2(self.x, self.y)) * 57.2958 #radians to angle conversion factor
        return float(angle)

    def getNormalized(self):
        l = self.getLength()
        if(l != 0): return self / l
        else: return V2f()



    def __add__(self, b): return V2f(self.x + b.x, self.y + b.y)
    def __sub__(self, b): return V2f(self.x - b.x, self.y - b.y)
    def __mul__(self, s): return V2f(self.x * s, self.y * s)
    def __truediv__(self, s): return V2f(self.x / s, self.y / s)
    def __eq__(self, other): return (self.x == other.x) and (self.y == other.y)
    def __ne__(self, other): return not (self == other)





# CLEANUP: this
def angleWrap(a: float) -> float:
    while a > 180: a -= 360
    while a < -180: a += 360
    return a