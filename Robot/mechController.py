from node import *
import utils.tags as tags
from hardware.Input import FlymerInputProfile









class MechProf(Node):

    def __init__(self) -> None:
        self.name = "MechProfile"
        self.priority = NODE_PROF


    def tick(self, data: dict[str, Any]) -> None:


        input = data.get(tags.INPUT)
        if type(input) is not FlymerInputProfile: return


        if input.grabToggle:
            data.update({tags.GRABBER + tags.DBLSOLENOID_STATE : not data[tags.GRABBER + tags.DBLSOLENOID_STATE] })

        x = input.drive[0]
        y = input.drive[1]
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

        prefs = [ tags.FL, tags.FR, tags.BL, tags.BR ]
        for i in range(0, 4):
            data[prefs[i] + tags.MOTOR_SPEED_CONTROL]  = speeds[i]













