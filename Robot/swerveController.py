from node import *
import utils.tags as tags
from hardware.Input import FlymerInputProfile


class SwerveProf(Node):

    def __init__(self) -> None:
        self.name = "SwerveProfile"
        self.priority = NODE_PROF

    def tick(self, data: dict[str, Any]) -> None:

        input = data.get(tags.INPUT)
        if type(input) is not FlymerInputProfile: return


        x = input.drive[0]
        y = input.drive[1]
        t = input.turning

        drivePrefs = [ tags.FLDrive, tags.FRDrive, tags.BLDrive, tags.BRDrive ]
        steerPrefs = [ tags.FLSteering, tags.FRSteering, tags.BLSteering, tags.BRSteering ]
        for i in range(0, 4):
            data[drivePrefs[i] + tags.MOTOR_SPEED_CONTROL] = y
            data[steerPrefs[i] + tags.MOTOR_SPEED_CONTROL] = x

        data[tags.FLSteering + tags.MOTOR_SPEED_CONTROL] = t













