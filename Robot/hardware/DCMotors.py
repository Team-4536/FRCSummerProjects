from typing import Callable, Any
import wpilib
import rev
from node import *
import utils.tags as tags




# stores state needed to operate, along with get and set interface
class DCMotorController:

    def get(self) -> float:
        raise NotImplementedError()

    def set(self, pwr: float) -> None:
        raise NotImplementedError()


# power set vals are assumed to be clamped
class VirtualController(DCMotorController):
    def __init__(self) -> None:
        self.speed: float = 0.0

    def get(self) -> float: return self.speed
    def set(self, pwr: float) -> None: self.speed = pwr


class SparkMaxController(DCMotorController):
    def __init__(self, ctrlr: rev.CANSparkMax) -> None:
        self.controller = ctrlr

    def get(self) -> float: return self.controller.get()
    def set(self, pwr: float) -> None: self.controller.set(pwr)







class DCMotorSpec:
    stallTorque = 0.0
    stallCurrent = 0.0
    freeSpeed = 0.0
    freeCurrent = 0.0


class NEOSpec(DCMotorSpec):
    stallTorque = 3.28
    stallCurrent = 181
    freeSpeed = 5820
    freeCurrent = 1.7

class FalconSpec(DCMotorSpec):
    stallTorque = 4.69
    stallCurrent = 257
    freeSpeed = 6380
    freeCurrent = 1.5




# composes spec and controller, provides nicer interfacing
class DCMotorNode(Node):

    def __init__(self, pref: tags.Tag, spec: type[DCMotorSpec], ctrlr: DCMotorController) -> None:
        self.spec: type[DCMotorSpec] = spec
        self.controller: DCMotorController = ctrlr

        self.pref = pref
        self.name = self.pref + tags.MOTOR_NAME
        self.priority = NODE_HARDWARE


    def tick(self, data: dict[str, Any]):

        val = data.get(self.pref + tags.MOTOR_SPEED_CONTROL)

        if type(val) == float:
            val = min(1, max(-1, val))
            self.controller.set(val)
        else:
            data.update({ self.pref + tags.MOTOR_SPEED_CONTROL : self.controller.get() })
