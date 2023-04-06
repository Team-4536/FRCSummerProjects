from node import *
from typing import Any
from hardware.DCMotors import DCMotor
import utils.tables


class motorTestNode(Node):

    def __init__(self, powerTag: str, motor: str, nodes: list[Node]) -> None:
        self.priority = NODE_HARDWARE
        self.name = "motorTestNode"
        self.pwrTag = powerTag
        self.motor = motor
        self.nodes = nodes


    def tick(self, data: dict[str, Any]) -> None:


        # yuck
        m = None
        for x in self.nodes:
            if x.name == self.motor:
                m = x

        assert(type(m) ==  DCMotor)

        self.name = "MotorTester" + self.motor

        val = utils.tables.sundialTable.getValue(self.pwrTag, None)
        if type(val) is not float: return

        m.setRaw(val)

    def command(self, args: list[str], data: dict[str, Any]) -> str:

        if len(args) <= 0:
            return "missing args [info]"

        if args[0] == "info":
            return f"Motor tag: {self.motor}, Recieving tag: {self.pwrTag}"

        return "missing args [info]"









