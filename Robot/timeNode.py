
from utils import profiling
import time
from typing import Any
from node import *
import utils.tables
import utils.tags as tags



class TimeNode(Node):

    def __init__(self) -> None:
        self.priority = NODE_FIRST
        self.name = "timing"

        self.prevTime: float = 0.0



    def tick(self, data: dict[str, Any]) -> None:

        data.update({ tags.DT : time.time() - self.prevTime })
        self.prevTime = time.time()

        frameTime = profiling.popProf()
        profiling.pushProf()
        utils.tables.telemTable.putNumber("Frame time ms", frameTime / 1000000)


