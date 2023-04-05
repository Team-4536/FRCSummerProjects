
from utils import profiling
import time
from typing import Any
from node import *
import utils.tables




class TimeNode(Node):

    def __init__(self, data: dict[str, Any]) -> None:
        self.priority = NODE_FIRST
        self.name = "timing"



    def tick(self, data: dict[str, Any]) -> None:

        frameTime = profiling.popProf()
        profiling.pushProf()
        utils.tables.telemTable.putNumber("Frame time ms", frameTime / 1000000)




