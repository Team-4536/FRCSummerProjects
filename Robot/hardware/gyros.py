
from typing import Any
import wpilib
from node import *
import utils.tags as tags
import navx
import random





class VirtualGyro():

    NOISE = 0.05

    def __init__(self) -> None:
        self.pitch = 0
        self.yaw = 0
        self.roll = 0

    def getPitch(self):
        return self.pitch + random.normalvariate(0, VirtualGyro.NOISE)
    def getYaw(self):
        return self.yaw + random.normalvariate(0, VirtualGyro.NOISE)
    def getRoll(self):
        return self.roll + random.normalvariate(0, VirtualGyro.NOISE)

class GyroNode(Node):

    def __init__(self, gyro: navx.AHRS | VirtualGyro) -> None:
        self.gyro = gyro
        self.name = "gyro"
        self.priority = NODE_HARDWARE


    def tick(self, data: dict[str, Any]) -> None:
        data.update({tags.GYRO_PITCH : self.gyro.getPitch()})
        data.update({tags.GYRO_YAW : self.gyro.getYaw()})
        data.update({tags.GYRO_ROLL : self.gyro.getRoll()})











