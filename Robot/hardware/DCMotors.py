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
    maxRPM: float = 0.0

# override members
class VirtualSpec(DCMotorSpec):
    maxRPM = 30.0





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
    def setRPM(self, v: float) -> None:
        v = min(self.spec.maxRPM, max(-self.spec.maxRPM, v))
        self.controller.set(v / self.spec.maxRPM)

    # NTS: CHECK LINEARITY
    def getRPM(self) -> float:
        return self.controller.get() * self.spec.maxRPM



    def tick(self, data: dict[str, Any]):
        data.update({ self.name + "RPM" : self.getRPM() })
