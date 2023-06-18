from typing import Callable, Any

"""
NOTE: this file is like the only one that you are good to import * on.
global constants have been prefixed.

Please do not import * on anything else.
"""



# Note: beacause the node execution is just awful, keep these numbers adjacent (no jumps from like 4 to 10)
NODE_FIRST: int =       0 # robot system shit, commands, telemetry, etc.
NODE_SIM: int =         1 # simulation processses
NODE_BACKGROUND: int =  2 # procs that use sensor data to come up w stuff
NODE_PROF: int =        3 # profile nodes, what should be doing high-ish level control
NODE_CTRL: int =        4 # hardware controllers, use output from prof.
NODE_HARDWARE: int =    5 # hardware obj tick
NODE_LAST: int =        6 # anything that needs to happen after, telemetry, file logging, etc.

NodeFunction = Callable[[dict[str, Any]], None]





class Node:

    def addToo(self, set):
        set.append(self)
        return self

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



# grabs thing at [key] from [data] and asserts type
# asserts if key does not exist
def getOrAssert(key: str, expectedType: type, data: dict[str, Any]):
    val = data.get(key)
    assert(val is not None)
    assert(type(val) is expectedType)
    return val