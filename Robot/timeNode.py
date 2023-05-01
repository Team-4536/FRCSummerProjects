
from utils import profiling
import time
from typing import Any
from node import *
import utils.tables
import utils.tags as tags
import wpilib
import robot



class TimeNode(Node):

    def __init__(self) -> None:
        self.priority = NODE_FIRST
        self.name = "timing"

        self.prevTime: float = 0.0
        self.initTime: float = wpilib.getTime()



    def tick(self, data: dict[str, Any]) -> None:

        data.update({ tags.DT : wpilib.getTime() - self.prevTime }) # ???: Is using one DT sample per frame accurate enough? or should each node sample?
        data.update({ tags.TIME_SINCE_INIT : wpilib.getTime() - self.initTime })
        self.prevTime = wpilib.getTime()

        # CLEANUP: pretty sure this is redundant
        frameTime = profiling.popProf()
        profiling.pushProf()
        data.update({ tags.FRAME_TIME : frameTime * 1000 })

