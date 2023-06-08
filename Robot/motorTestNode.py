from node import *
from typing import Any
from hardware.DCMotors import DCMotorNode
import utils.tables
import utils.tags as tags
from hardware.Input import FlymerInputProfile


class MotorTestNode(Node):

    def __init__(self, motorPrefix: str) -> None:
        self.priority = NODE_PROF
        self.name = "motorTestNode"
        self.motorPrefix = motorPrefix


    def tick(self, data: dict[str, Any]) -> None:

        input = data[tags.INPUT]
        assert(type(input) == FlymerInputProfile)

        data[tags.FLDrive + tags.MOTOR_SPEED_CONTROL] = input.drive[1] * 0.2