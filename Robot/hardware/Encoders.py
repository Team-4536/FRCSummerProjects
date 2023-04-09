import rev
from node import *
import random






class Encoder(Node):

    # it's like this because I'm lazy. :).
    def __init__(self, name: str, encObj: rev.RelativeEncoder, inverted: bool) -> None:
        self.name = name
        self.priority = NODE_HARDWARE
        raise NotImplementedError()

    def getRotations(self) -> float:
        raise NotImplementedError()

    def tick(self, data: dict[str, Any]) -> None:
        data.update({self.name + "Pos" : self.getRotations()})



class RelativeEncoder(Encoder):

    def __init__(self, name: str, encObj: rev.RelativeEncoder) -> None:
        self.__encoder: rev.RelativeEncoder = encObj
        self.name = name
        self.priority = NODE_HARDWARE

    def getRotations(self) -> float:
        return self.__encoder.getPosition()


# virtual encoder members should only be written to by sim nodes
# output will move with motor output, so reversal doesnt need to happen if the motors are all reversed correctly
class VirtualEncoder(Encoder):

    NOISE = 0.005

    def __init__(self, name: str) -> None:
        self.realPosition: float = 0.0

        self.priority = NODE_HARDWARE
        self.name = name

    def getRotations(self) -> float:
        return self.realPosition + random.normalvariate(0, VirtualEncoder.NOISE)


