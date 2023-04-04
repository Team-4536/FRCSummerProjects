from typing import Callable, Any




NODE_FIRST: int =       0
NODE_PROF: int =        1
NODE_CTRL: int =        2
NODE_HARDWARE: int =    3
NODE_LAST: int =        4

NodeFunction = Callable[[dict[str, Any]], None]

class Node:

    def __init__(self) -> None:
        self.priority = NODE_FIRST
        self.name = "untitled"
        raise NotImplementedError()

    def execute(self, data: dict[str, Any]) -> None:
        raise NotImplementedError()


