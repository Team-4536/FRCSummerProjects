from typing import Any
from node import *
import ntcore
from utils import profiling


telemTable = ntcore.NetworkTableInstance.getDefault().getTable("telemetry")


class TelemNode(Node):

    def __init__(self, hardware: dict[str, Any]) -> None:
        self.priority = NODE_LAST
        self.name = "telemetrySender"
        self.__hardware = hardware


    def execute(self, data: dict[str, Any]) -> None:

        for x in self.__hardware:
            telemTable.putString(x, str(self.__hardware[x].__dict__))




class TimeNode(Node):

    def __init__(self) -> None:
        self.priority = NODE_FIRST
        self.name = "timing"


    def execute(self, data: dict[str, Any]) -> None:

        frameTime = profiling.popProf()
        profiling.pushProf()
        telemTable.putNumber("Frame time ms", frameTime / 1000000)





