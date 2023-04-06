
from typing import Any
from node import *
import utils.tables
from utils.tables import cmdTable
import time
import hardware.DCMotors



class TelemNode(Node):

    def __init__(self) -> None:
        self.priority = NODE_LAST
        self.name = "telem"
        self.published: list[str] = []



    def tick(self, data: dict[str, Any]) -> None:

        """
        for x in self.__hardware:
            # utils.tables.telemTable.putString(x, str(self.__hardware[x].__dict__))

            motor = self.__hardware[x]
            if type(motor) == hardware.DCMotors.DCMotor:
                utils.tables.telemTable.putValue(x+"Speed", motor.getRaw())
        """
        for x in self.published:
            d = data[x]

            if type(d) == float:
                utils.tables.telemTable.putNumber(x, data[x])
            else:
                utils.tables.telemTable.putNumber(x, d)




    def command(self, args: list[str], data: dict[str, Any]) -> str:

        if len(args) != 2:
            return "missing args: [publish|remove], dataTag"

        if not (args[1] in data):
            return f"value \"{args[1]}\" does not exist"


        if args[0] == "publish":
            self.published.append(args[1])
        elif args[0] == "remove":
            self.published.remove(args[1])
        else:
            return "incorrect args. actual: [publish|remove], dataTag"


        return f"value \"{args[1]}\" published"


