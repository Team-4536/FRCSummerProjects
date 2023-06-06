import math
from typing import Any
from node import * # [* means all]
from hardware.Input import FlymerInputProfile
from utils import tags
import utils.PIDController


"""---------------Lift Motor Control--------------------------"""

class LiftProf(Node):

    def __init__(self) -> None:
        self.name = "LiftProfile"
        self.priority = NODE_PROF

    def tick(self, data: dict[str, Any]) -> None:

        input = data.get(tags.INPUT)
        if type(input) is not FlymerInputProfile: return

        liftPower = input.lift
        liftPos = data[tags.LIFT_MOTOR + tags.ENCODER_READING]

        data[tags.LIFT_MOTOR + tags.MOTOR_SPEED_CONTROL] = liftPower


"""----------------Turret Motor Control---------------------------"""

class TurretProf(Node):

    def __init__(self) -> None:
        self.name = "TurretProfile"
        self.priority = NODE_PROF

    def tick(self, data: dict[str, Any]) -> None:

        input = data.get(tags.INPUT)
        if type(input) is not FlymerInputProfile: return

        turretPower = input.turret
        turretPos = data[tags.TURRET_MOTOR + tags.ENCODER_READING]

        data[tags.TURRET_MOTOR + tags.MOTOR_SPEED_CONTROL] = turretPower


    """---------------Retract Motor Control---------------------------"""

    class RetractProf(Node):

        def __init__(self) -> None:
            self.name = "RetractProfile"
            self.priority = NODE_PROF

        def tick(self, data: dict[str, Any]) -> None:

            input = data.get(tags.INPUT)
            if type(input) is not FlymerInputProfile: return

            retractPower = input.retract
            retractPos = data[tags.RETRACT_MOTOR + tags.ENCODER_READING]

            data[tags.RETRACT_MOTOR + tags.MOTOR_SPEED_CONTROL] = retractPower

    """-----------------------------------------------------------------------"""