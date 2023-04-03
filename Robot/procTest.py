import wpilib
from process import *





def makeLogProc(msg: str, pri: int) -> Process:
    return (pri, lambda: print(msg))

