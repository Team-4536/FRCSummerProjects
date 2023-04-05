
from typing import Any
from node import *
import utils.tables
from utils.tables import cmdTable
import time



class TelemNode(Node):

    def __init__(self, hardware: dict[str, Any]) -> None:
        self.priority = NODE_LAST
        self.name = "telem"
        self.__hardware = hardware



    def tick(self, data: dict[str, Any]) -> None:

        for x in self.__hardware:
            utils.tables.telemTable.putNumber(x, self.__hardware[x].getRaw())
            utils.tables.telemTable.putString(x, str(self.__hardware[x].__dict__))

    def command(self, args: list[str], data: dict[str, Any]) -> str:
        return str(args)



