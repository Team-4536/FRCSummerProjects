import rev
from node import *
import random
import utils.tags as tags



# virtual encoder members should only be written to by sim nodes
# output will move with motor output, so reversal doesnt need to happen if the motors are all reversed correctly
class VirtualEncoder():

    NOISE = 0.001

    def __init__(self) -> None:
        self.realPosition: float = 0.0

    def getPosition(self) -> float:
        return self.realPosition + random.normalvariate(0, VirtualEncoder.NOISE)






class EncoderNode(Node):

    def __init__(self, pref: str, encoderObj: rev.RelativeEncoder|VirtualEncoder) -> None:
        self.pref: str = pref
        self.name: str = pref + tags.ENCODER_NAME
        self.priority = NODE_HARDWARE
        self.encoderObj = encoderObj

    def tick(self, data: dict[str, Any]) -> None:
        data.update({self.pref + tags.ENCODER_READING : self.encoderObj.getPosition()})


