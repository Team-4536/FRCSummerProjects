import math


class V2:

    def __init__(self, x: float = 0, y: float = 0) -> None:
        self.x = x
        self.y = y

    #rotate degrees
    def rotateDegrees(self, d):

        x = self.x
        y = self.y

        cos = math.cos(d * (math.pi / 180))
        sin = math.sin(d * (math.pi / 180))

        return V2(x * cos - y * sin, y * cos + x * sin)

    #length
    def getLength(self):

        x = self.x
        y = self.y

        length = math.sqrt((x**2 + y**2))

        return float(length)

    #angle
    def getAngle(self):

        angle = (math.atan2(self.y, self.x)) * 57.2958 #radians to angle conversion factor

        return float(angle)
    
    #add
    def add(self, newV2):

        return V2(self.x + newV2.x, self.y + newV2.y)
    
    #subtract
    def subtract(self, newV2):

        return V2(self.x - newV2.x, self.y - newV2.y)
    
    #multiply
    def multiplyBy(self, factor):

        return V2(self.x * factor, self.y * factor)
    
    #average
    def avg(self, newV2):

        averageX = (self.x + newV2.x) / 2
        averageY  = (self.y + newV2.y) / 2

        return V2(averageX, averageY)
    
    #there's probably an easier way to do this
    def noValue(self):
        
        if self.x == 0 and self.y == 0:
            noValue = True
        else:
            noValue = False

        return noValue
  
#Flymer = V2(1.0, 0.6)
#Flymer = Flymer.rotateDegrees(90)

#print(Flymer)
