from typing import Callable, Any
from node import *
import utils.tags as tags

class limitSwitchNode (Node):
    def __init__(self, hardware, tag) -> None:
        self.hardware = hardware
        self.tag = tag

    def tick(self, data: dict[str, Any]) -> None:
        data[self.tag] = self.hardware.get()

class virtualLimitSwitch:
    def __init__(self) -> None:
        self.pressed = False

    def get(self) -> object:
        return self.pressed

