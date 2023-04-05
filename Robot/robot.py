import wpilib
import ntcore
import robotpy
from node import *
from node import Node

import hardware.DCMotors
from typing import Any
import time

import telemetryNode
import timeNode
import motorTestNode
import utils.profiling
from utils.tables import *
import cmdNode






HEARTBEAT: float = 1.0
MSG_CUTOFF: float = 2.0
RES = 'res'
UNKNOWN_RES = 'unknown cmd'


class Robot(wpilib.TimedRobot):



    def returnPing(self, args: list[str]) -> str:
        return "ping response!"

    def list(self, args: list[str]) -> str:

        if len(args) == 0:
            return "args missing: [nodes|data|hardware]"

        if args[0] == "nodes":

            res: str = self.procs[0].name
            if len(self.procs) == 0: return "[There are no processes]"

            for i in range(1, len(self.procs)):
                res += ",\n" + self.procs[i].name

            return res

        elif args[0] == "data":
            if len(self.data) == 0: return "[There is no data]"

            res = ""
            for k in self.data:
                res += k + ",\n"
            return res

        elif args[0] == "hardware":
            if len(self.hardware) == 0: return "[There is no hardware on bot]"

            res = ""
            for k in self.hardware:
                res += k + ",\n"
            return res

        return "args missing: [nodes|data|hardware]"





    def robotInit(self) -> None:

        self.ctrls: dict[str, Callable[[list[str]], str]] = {
            "ping" : self.returnPing,
            "list" : self.list,
            }

        cmdTable.putNumber("heartbeat", 0.0)
        self.prevTime: float = 0.0
        self.msgTopic = cmdTable.getStringTopic("msg").subscribe("default")





        self.hardware: dict[str, Any] = {  } # hardware objs
        self.data: dict[str, Any] = { } # continuous data
        self.procs: list[Node] = [ ] # processes



        self.hardware.update({"motor1" : hardware.DCMotors.SparkImpl(0)})
        self.hardware.update({"motor2" : hardware.DCMotors.SparkImpl(1)})


        self.procs.append(telemetryNode.TelemNode(self.hardware))
        self.procs.append(timeNode.TimeNode(self.data))
        # self.procs.append(cmdNode.cmdNode())

        self.procs.append(motorTestNode.motorTestNode("speed", self.hardware["motor1"]))









    # Execute all running proc nodes
    def robotPeriodic(self) -> None:


        # respond to commands +==========================================================

        t = time.time()
        if (t - self.prevTime) > HEARTBEAT:
            cmdTable.putNumber("heartbeat", t)
            self.prevTime = t



        for x in self.msgTopic.readQueue():
            res = self.parseAndRespondMsg(x.value)
            if res is not None: cmdTable.putString(RES, res)



        # run procs +=====================================================================
        ordered: dict[int, list[Node]] = {}
        maxPri = 0
        for val in self.procs:

            maxPri = max(val.priority, maxPri)

            # execute all first pri procs
            if val.priority == NODE_FIRST: self.runProcessNodeSafe(val)
            # store the rest, ordered
            else:
                if val.priority in ordered:
                    ordered[val.priority].append(val)
                else:
                    ordered.update({ val.priority : [val] })


        # execute other procs, in order
        for i in range(0, maxPri+1):

            if i in ordered:
                l = ordered[i]
                for x in l:
                    self.runProcessNodeSafe(x)
        # run procs +=====================================================================





    def runProcessNodeSafe(self, x: Node):
        try:
            x.tick(self.data)
        except Exception as exception:
            print(f"Exception in node \"{x.name}\": {repr(exception)}")



    # none return type indicates bad message format (no parsable stamp)
    def parseAndRespondMsg(self, msg: str) -> str | None:

        split = msg.split(':', 1)
        if len(split) <= 1: return None

        msgStamp = split[0]
        args = split[1].split(' ')


        response: str = UNKNOWN_RES

        # handle message
        found: bool = False

        for k, v in self.ctrls.items():
            if args[0] == k:

                try:
                    response = v(args[1:])
                except Exception as e:
                    response = "Exception in command: " + repr(e)

                found = True
                break

        if not found:
            found = False
            for n in self.procs:
                if args[0] == n.name:

                    try:
                        response = n.command(args[1:], self.data)
                    except Exception as e:
                        response = "Exception in command: " + repr(e)

                    found  = True
                    break

        return msgStamp + ":" + response
















    def teleopInit(self) -> None:
        pass

    def teleopPeriodic(self) -> None:
        pass



    def autonomousInit(self) -> None:
        pass

    def autonomousPeriodic(self) -> None:
        pass




    def disabledInit(self) -> None:
        pass

    def disabledPeriodic(self) -> None:
        pass









if __name__ == "__main__":
    wpilib.run(Robot)


"""
if __name__ == "__main__":

    src = ""
    with open(__file__, 'r') as file:
        src = file.read()

    x = compile(src, __file__, mode="exec")
    exec(x, {"Node" : Node, "wpilib" : wpilib}, {})
    print("prg ran!")


if __name__ == "builtins":
    print("running!")
    wpilib.run(Robot)

"""