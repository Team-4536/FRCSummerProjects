import rev



# Any other encoder defs. that follow this spec are useable elsewhere

class Encoder:

    # it's like this because I'm lazy. :).
    def __init__(self, encObj: rev.RelativeEncoder, inverted: bool) -> None:
        self.__encoder: rev.RelativeEncoder = encObj

    def getRotations(self) -> float:
        return self.__encoder.getPosition()


# virtual encoder members should only be written to by sim nodes
# output will move with motor output, so reversal doesnt need to happen if the motors are all reversed correctly
class VirtualEncoder:

    def __init__(self) -> None:
        self.position = 0

    def getRotations(self) -> float:
        return self.position


