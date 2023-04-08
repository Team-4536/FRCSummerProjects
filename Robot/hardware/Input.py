
from typing import Callable, Any
import wpilib
import rev
from node import *
from enum import Enum


class buttons(Enum):
    A = 0
    B = 0
    X = 0
    Y = 0
    LB = 0
    RB = 0
    DPADD = 0
    DPADU = 0
    DPADL = 0
    DPADR = 0


class rawInputProfile:

    def __init__(self) -> None:

        self.leftStick: tuple[float, float] = (0.0, 0.0)
        self.rightStick: tuple[float, float] = (0.0, 0.0)

        self.lt: float = 0.0
        self.rt: float = 0.0

        self.dpadUp: bool = False
        self.dpadDown: bool = False
        self.dpadLeft: bool = False
        self.dpadRight: bool = False




class InputNode(Node):
    def __init__(self) -> None:
        pass










class FlymerInputProfile:
    def __init__(self) -> None:

        self.drive: tuple[float, float] = (0.0, 0.0)
        self.turning: float = 0.0

        self.lift: float = 0.0
        self.turret: float = 0.0
        self.retract: float = 0.0

        self.brakeToggle: bool = False
        self.grabToggle: bool = False





class FlymerTrimNode(Node):
    def __init__(self) -> None:
        self.priority = NODE_BACKGROUND
        self.name = "inputTrimmer"

    def tick(self, data: dict[str, Any]) -> None:

        ctrl = data[RAW_INPUT_TARGET]

        data.update({ TRIMMED_INPUT_TARGET : x })
        pass


