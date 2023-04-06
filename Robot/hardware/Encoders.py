import rev
from node import *


# Any other encoder defs. that follow this spec are useable elsewhere

class Encoder(Node):

    # it's like this because I'm lazy. :).
    def __init__(self, name: str, encObj: rev.RelativeEncoder, inverted: bool) -> None:
        self.__encoder: rev.RelativeEncoder = encObj
        self.name = name
        self.priority = NODE_HARDWARE

    def getRotations(self) -> float:
        return self.__encoder.getPosition()

    def tick(self, data: dict[str, Any]) -> None:
        data.update({self.name + "Pos" : self.getRotations()})


# virtual encoder members should only be written to by sim nodes
# output will move with motor output, so reversal doesnt need to happen if the motors are all reversed correctly
class VirtualEncoder(Node):

    def __init__(self, name: str) -> None:
        self.position = 0

        self.priority = NODE_HARDWARE
        self.name = name

    def getRotations(self) -> float:
        return self.position

    def tick(self, data: dict[str, Any]) -> None:
        data.update({self.name + "Pos" : self.getRotations()})


