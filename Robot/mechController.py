from node import *
import utils.tags as tags
from hardware.Input import FlymerInputProfile



class MechProf(Node):

    def __init__(self) -> None:
        self.name = "MechProfile"
        self.priority = NODE_PROF


    def tick(self, data: dict[str, Any]) -> None:

        input = getOrAssert(tags.INPUT, FlymerInputProfile, data)


        x = input.driveX
        y = input.driveY
        t = input.turning

        speeds = [
            (y + x + t), # FL
            (y - x - t), # FR
            (y - x + t), # Bl
            (y + x - t), # BR
        ]

        max = abs(speeds[0])
        for i in range(0, 4):
            if (max < abs(speeds[i])):
                max = abs(speeds[i])

        if (max > 1):
            for i in range(0, 4):
                speeds[i] /= max

        prefs = [ tags.FLDrive, tags.FRDrive, tags.BLDrive, tags.BRDrive ]
        for i in range(0, 4):
            data[prefs[i] + tags.MOTOR_SPEED_CONTROL]  = speeds[i] * 0.1













