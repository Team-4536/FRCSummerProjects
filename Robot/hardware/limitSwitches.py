from typing import Callable, Any
from node import *
import utils.tags as tags

class limitSwitchNode (Node):
    def __init__(self, hardware) -> None:
        self.hardware = hardware

    def tick(self, data: dict[str, Any]) -> None:
        data[tags.LIMIT_SWITCH] = self.hardware.get()

class virtualLimitSwitch:
    def __init__(self) -> None:
        self.pressed = False

    def get(self) -> object:
        return self.pressed

