from node import *
from typing import Any
from hardware.DCMotors import DCMotorNode
import utils.tables
import utils.tags as tags


class MotorTestNode(Node):

    def __init__(self, srcTag: str, motorPrefix: str) -> None:
        self.priority = NODE_HARDWARE
        self.name = "motorTestNode"
        self.srcTag = srcTag
        self.motorPrefix = motorPrefix


    def tick(self, data: dict[str, Any]) -> None:

        self.name = self.motorPrefix + tags.TESTER_NAME

        val = utils.tables.sundialTable.getValue(self.srcTag, None)
        if type(val) is not float: return

        data.update({ self.motorPrefix + tags.MOTOR_SPEED_CONTROL : val })

    def command(self, args: list[str], data: dict[str, Any]) -> str:

        if len(args) <= 0:
            return "missing args [info]"

        if args[0] == "info":
            return f"Tag being set: {self.motorPrefix + tags.MOTOR_SPEED_CONTROL}, Source tag: {self.srcTag}"

        return "missing args [info]"









