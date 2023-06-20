from node import *
import utils.tags as tags
from hardware.Input import DemoInputProfile

class TankProf(Node):

    def __init__(self) -> None:
        self.name = "tankProf"
        self.priority = NODE_PROF

    def tick(self, data: dict[str, Any]) -> None:
        input = getOrAssert(tags.INPUT, DemoInputProfile, data)

        left = input.driveLeft
        right = input.driveRight

        speeds = [
            left, #FL
            right, #FR
            left, #BL
            right #BR
        ]

        prefs = [ tags.FLDrive, tags.FRDrive, tags.BLDrive, tags.BRDrive ]
        for i in range(0, 4):
            data[prefs[i] + tags.MOTOR_SPEED_CONTROL]  = speeds[i] * 0.1