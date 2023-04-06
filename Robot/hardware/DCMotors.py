from typing import Callable, Any
import wpilib
import rev
from node import *




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
    maxRPS: float = 0.0

# override members
class VirtualSpec(DCMotorSpec):
    maxRPS = 7.0





# composes spec and controller, provides nicer interfacing
class DCMotor(Node):
    def __init__(self, name: str, spec: type[DCMotorSpec], ctrlr: DCMotorController) -> None:
        self.spec: type[DCMotorSpec] = spec
        self.controller: DCMotorController = ctrlr

        self.name = name
        self.priority = NODE_HARDWARE

    def setRaw(self, v: float) -> None:
        v = min(1, max(-1, v))
        self.controller.set(v)

    def getRaw(self) -> float:
        return self.controller.get()


    # NTS: CHECK LINEARITY
    def setRPS(self, v: float) -> None:
        v = min(self.spec.maxRPS, max(-self.spec.maxRPS, v))
        self.controller.set(v / self.spec.maxRPS)

    # NTS: CHECK LINEARITY
    def getRPS(self) -> float:
        return self.controller.get() * self.spec.maxRPS



    def tick(self, data: dict[str, Any]):
        data.update({ self.name + "RPS" : self.getRPS() })
