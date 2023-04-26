
from typing import Any
from node import *
import utils.tables
from utils.tables import cmdTable
import time
import hardware.DCMotors


# Note: if you have a class that needs to be sent over telemetry, implement the "publish" method.
# example can be found in Input.py

class TelemNode(Node):

    def __init__(self, defaultPublished: list[str]) -> None:
        self.priority = NODE_LAST
        self.name = "telem"
        self.published: list[str] = defaultPublished



    def tick(self, data: dict[str, Any]) -> None:

        for x in self.published:
            d = data[x]

            if type(d) == float:
                utils.tables.telemTable.putNumber(x, data[x])
            elif type(d) == bool:
                val = data[x]
                utils.tables.telemTable.putNumber(x, 1 if val else 0)

            if callable(getattr(d, "publish", None)):
                d.publish(x, utils.tables.telemTable) # type: ignore
            else:
                utils.tables.telemTable.putNumber(x, d)




    def command(self, args: list[str], data: dict[str, Any]) -> str:

        if len(args) < 1:
            return "missing args: [publish|remove|list], [dataTags]"

        if args[0] == "list":
            if len(self.published) == 0: return "[no published values]"
            s: str = ""
            for x in self.published:
                s += x + "\n"
            return s

        if len(args) < 2:
            return "missing args: [publish|remove|list], dataTags"


        ret: str = ""
        for i in range(1, len(args)):

            if not (args[i] in data):
                ret += f"value \"{args[i]}\" does not exist\n"
                continue


            if args[0] == "publish":
                self.published.append(args[i])
            elif args[0] == "remove":
                self.published.remove(args[i])
            else:
                return "incorrect args. actual: [publish|remove|list], dataTags"


            ret += f"value \"{args[i]}\" published\n"

        return ret


