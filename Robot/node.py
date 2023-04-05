from typing import Callable, Any




# Note: beacause the node execution is just awful, keep these numbers adjacent (no jumps from like 4 to 10)
NODE_FIRST: int =       0
NODE_SIM: int =         1
NODE_BACKGROUND: int =  2
NODE_PROF: int =        3
NODE_CTRL: int =        4
NODE_HARDWARE: int =    5
NODE_LAST: int =        6

NodeFunction = Callable[[dict[str, Any]], None]

class Node:

    # please leave spaces out of names in nodes and hardware and data
    # so that they can be adressed by commands
    def __init__(self) -> None:
        self.priority = NODE_FIRST
        self.name = "untitled"
        raise NotImplementedError()

    def tick(self, data: dict[str, Any]) -> None:
        raise NotImplementedError()


    # args passed in will exclude the name of the proc, I.E.
    # command sent is: telem publish speed1
    # args would be:   publish speed1
    # and the node named "telem" would execute
    def command(self, args: list[str], data: dict[str, Any]) -> str:
        raise NotImplementedError()


