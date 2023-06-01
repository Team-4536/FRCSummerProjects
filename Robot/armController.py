import math
from typing import Any
from node import * # [* means all]
from hardware.Input import FlymerInputProfile
from utils import tags

#this will break and die so don't actually run it

class ArmProf(Node):

    def __init__(self) -> None:
        self.name = "ArmProfile"
        self.priority = NODE_PROF

    def tick(self, data: dict[str, Any]) -> None:

        input = data.get(tags.INPUT)
        if type(input) is not FlymerInputProfile: return

        liftPower = input.lift

        data[tags.LIFT_MOTOR] = liftPower