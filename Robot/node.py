from typing import Callable, Any




# Note: beacause the node execution is just awful, keep these numbers adjacent (no jumps from like 4 to 10)
NODE_FIRST: int =       0
NODE_SIM: int =         1
NODE_PROF: int =        2
NODE_CTRL: int =        3
NODE_HARDWARE: int =    4
NODE_LAST: int =        5

NodeFunction = Callable[[dict[str, Any]], None]

class Node:

    def __init__(self) -> None:
        self.priority = NODE_FIRST
        self.name = "untitled"
        raise NotImplementedError()

    def tick(self, data: dict[str, Any]) -> None:
        raise NotImplementedError()


