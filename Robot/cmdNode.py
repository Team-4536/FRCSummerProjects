from node import *
from utils.tables import cmdTable
import time




HEARTBEAT: float = 1.0
MSG_CUTOFF: float = 2.0
RES = 'res'
UNKNOWN_RES = 'unknown cmd'


"""
Message format:

<stamp>:<cmd> <space separated args>

messages stamp is determined by the str before the first colon, and args are space seperated

badly formatted messages are ignored and not responded to
"""




class cmdNode(Node):


    def __init__(self) -> None:
        self.priority = NODE_FIRST
        self.name = "cmdNode"

        cmdTable.putNumber("heartbeat", 0.0)
        self.prevTime: float = 0.0

        self.msgTopic = cmdTable.getStringTopic("msg").subscribe("default")



    def tick(self, data: dict[str, Any]) -> None:

        t = time.time()
        if (t - self.prevTime) > HEARTBEAT:
            cmdTable.putNumber("heartbeat", t)
            self.prevTime = t



        for x in self.msgTopic.readQueue():
            res = self.parseAndRespondMsg(x.value)

            if res is not None:
                cmdTable.putString(RES, res)

    # none return type indicates bad message format (no parsable stamp)
    def parseAndRespondMsg(self, msg: str) -> str | None:

        split = msg.split(':', 1)
        if len(split) <= 1: return None

        msgStamp = split[0]
        args = split[1].split(' ')


        response: str = UNKNOWN_RES

        # handle message
        if args[0] == "ping":
            response = "ping response!"


        return msgStamp + ":" + response







def __parseTest() -> None:
    n: cmdNode = cmdNode()

    msg: str = "100.0:ping"
    print(n.parseAndRespondMsg(msg))
    msg = "200.0:log x"
    print(n.parseAndRespondMsg(msg))
    msg = "30hi eilrutghelriuhl"
    print(n.parseAndRespondMsg(msg))

