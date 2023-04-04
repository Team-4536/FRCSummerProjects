
from typing import Any
from node import *
import utils.tables





class TelemNode(Node):

    def __init__(self, hardware: dict[str, Any]) -> None:
        self.priority = NODE_LAST
        self.name = "telemetrySender"
        self.__hardware = hardware


    def execute(self, data: dict[str, Any]) -> None:

        for x in self.__hardware:
            utils.tables.telemTable.putString(x, str(self.__hardware[x].__dict__))



