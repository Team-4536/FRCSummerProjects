from typing import Callable, Any
import wpilib
import rev
from node import *
import utils.tags as tags





# power set vals are assumed to be clamped
class VirtualController():
    def __init__(self) -> None:
        self.speed: float = 0.0

    def get(self) -> float: return self.speed
    def set(self, pwr: float) -> None: self.speed = pwr



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

    def __init__(self, pref: str, spec: type[DCMotorSpec], ctrlr: VirtualController|rev.CANSparkMax) -> None:
        self.spec: type[DCMotorSpec] = spec
        self.controller = ctrlr

        self.pref = pref
        self.name = self.pref + tags.MOTOR_NAME
        self.priority = NODE_HARDWARE


    def tick(self, data: dict[str, Any]):

        val = data.get(self.pref + tags.MOTOR_SPEED_CONTROL)

        if type(val) == float or type(val) == int:
            val = min(1, max(-1, val)) # type: ignore
            self.controller.set(val)
        else:
            data.update({ self.pref + tags.MOTOR_SPEED_CONTROL : self.controller.get() })

