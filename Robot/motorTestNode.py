from node import *
from typing import Any
from hardware.DCMotors import DCMotorNode
import utils.tables
import utils.tags as tags
from hardware.Input import FlymerInputProfile
from robot import reportMsg
import rev


class MotorTestNode(Node):

    def __init__(self, motorPrefix: str) -> None:
        self.priority = NODE_PROF
        self.name = "motorTestNode"
        self.motorPrefix = motorPrefix

        #self.motor = rev.CANSparkMax(3, rev.CANSparkMax.MotorType.kBrushless)


    def tick(self, data: dict[str, Any]) -> None:

        input = getOrAssert(tags.INPUT, FlymerInputProfile, data)
        """
        if  getOrAssert(tags.OPMODE, int, data) == tags.OP_TELEOP:
            self.motor.set(.1)
        else:
            self.motor.set(0)
        """
        data[tags.FLDrive + tags.MOTOR_SPEED_CONTROL] = input.drive[1] * 0.2