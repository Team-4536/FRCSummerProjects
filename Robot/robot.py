import wpilib
from typing import Any
import time

from hardware.DCMotors import *
from hardware.Encoders import *
from utils.tables import *
from node import *
import telemetryNode
import timeNode
import motorTestNode
import motorSimNode





HEARTBEAT: float = 1.0
MSG_CUTOFF: float = 2.0
RES = 'res'
UNKNOWN_RES = 'unknown cmd'


class Robot(wpilib.TimedRobot):



    def returnPing(self, args: list[str]) -> str:
        return "ping response!"

    def lis(self, args: list[str]) -> str:

        if len(args) == 0:
            return "args missing: [nodes|data]"

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


        return "args missing: [nodes|data|hardware]"


    def kill(self, args: list[str]) -> str:
        raise SystemExit()




    def robotInit(self) -> None:

        self.ctrls: dict[str, Callable[[list[str]], str]] = {
            "ping" : self.returnPing,
            "list" : self.lis,
            "kill" : self.kill
            }

        self.prevTime: float = 0.0
        self.msgTopic = cmdTable.getStringTopic("msg").subscribe("default")





        self.data: dict[str, Any] = { } # continuous data
        self.procs: list[Node] = [ ] # processes / including hardware



        motors = [
            DCMotor("FLDrive", VirtualSpec, VirtualController()),
            DCMotor("FRDrive", VirtualSpec, VirtualController()),
            DCMotor("BLDrive", VirtualSpec, VirtualController()),
            DCMotor("BRDrive", VirtualSpec, VirtualController())
        ]

        for m in motors:
            self.procs.append(m)

        encoders = [
            VirtualEncoder("FLEncoder"),
            VirtualEncoder("FREncoder"),
            VirtualEncoder("BLEncoder"),
            VirtualEncoder("BREncoder")
        ]

        for e in encoders:
            self.procs.append(e)

        self.procs.append(motorSimNode.motorSimNode(motors[0], encoders[0]))



        self.procs.append(telemetryNode.TelemNode())
        self.procs.append(timeNode.TimeNode())

        self.procs.append(motorTestNode.motorTestNode("motor0", "FLDrive", self.procs))
        self.procs.append(motorTestNode.motorTestNode("motor1", "FRDrive", self.procs))









    # Execute all running proc nodes
    def robotPeriodic(self) -> None:


        # respond to commands +==========================================================

        t = time.time()
        if (t - self.prevTime) > HEARTBEAT:
            telemTable.putNumber("heartbeat", t)
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




    """
    Message format:

    <stamp>:<cmd> <space separated args>

    messages stamp is determined by the str before the first colon, and args are space seperated

    badly formatted messages are ignored and not responded to
    """

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