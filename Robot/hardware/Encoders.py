import rev
from node import *
import random
import utils.tags as tags



class EncoderNode(Node):

    def __init__(self, pref: str) -> None:
        self.pref: str = pref

        self.name: str = pref + tags.ENCODER_NAME
        self.priority = NODE_HARDWARE

        raise NotImplementedError()

    def getRotations(self) -> float:
        raise NotImplementedError()

    def tick(self, data: dict[str, Any]) -> None:
        data.update({self.pref + tags.ENCODER_READING : self.getRotations()})



class RelativeEncoderNode(EncoderNode):

    def __init__(self, pref: str, encObj: rev.RelativeEncoder) -> None:

        self.__encoder: rev.RelativeEncoder = encObj
        self.pref = pref
        self.name = pref + tags.ENCODER_NAME
        self.priority = NODE_HARDWARE

    def getRotations(self) -> float:
        return self.__encoder.getPosition()


# virtual encoder members should only be written to by sim nodes
# output will move with motor output, so reversal doesnt need to happen if the motors are all reversed correctly
class VirtualEncoderNode(EncoderNode):

    NOISE = 0.005

    def __init__(self, pref: str) -> None:

        self.realPosition: float = 0.0
        self.pref = pref

        self.name = pref + tags.ENCODER_NAME
        self.priority = NODE_HARDWARE

    def getRotations(self) -> float:
        return self.realPosition + random.normalvariate(0, VirtualEncoderNode.NOISE)


