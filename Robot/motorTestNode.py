from node import *
from typing import Any
from hardware.DCMotors import DCMotor
import utils.tables


class motorTestNode(Node):

    def __init__(self, powerTag: str, motor: DCMotor) -> None:
        self.priority = NODE_HARDWARE
        self.name = "motorTestNode"
        self.pwrTag = powerTag
        self.motor = motor


    def tick(self, data: dict[str, Any]) -> None:

        val = utils.tables.telemTable.getValue(self.pwrTag, None)
        if type(val) is not float: return

        self.motor.setRaw(val)







