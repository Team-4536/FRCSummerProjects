import wpilib
from typing import Any
import time

import inits

from hardware.DCMotors import *
from hardware.Encoders import *
from utils.tables import *
from node import *
import hardware.Input as Input

import telemetryNode
import traceback

HEARTBEAT: float = 1.0
UNKNOWN_RES = 'unknown cmd'



logCount: int = 0

# NOTE: please do not write directly to the log table value, it reqires formatting and bad messages are ignored
def reportErr(message: str) -> None:
    global logCount # CLEANUP: this entire system is a hack, but messages aren't a thing in networktables and making a socket system is very difficult
    telemTable.putString(tags.LOG_TAG, f"{logCount}:ERR:" + str(message))
    logCount+=1
def reportMsg(message: str) -> None:
    global logCount
    telemTable.putString(tags.LOG_TAG, f"{logCount}:MSG:" + str(message))
    logCount+=1


# Ok i know this is kind of awful, but nodes that would set members that didn't exist wouldn't throw and it was really annoying so i made it throw
class ThrowDict(dict):
    def __missing__(self, key):
        raise KeyError

    def __setitem__(self, __key: Any, __value: Any) -> None:
        if not (__key in self): raise KeyError
        return super().__setitem__(__key, __value)




class Robot(wpilib.TimedRobot):

    def robotInit(self) -> None:

        self.ctrls: dict[str, Callable[[list[str]], str]] = {
            "ping" : self.returnPing,
            "list" : self.lis,
            "kill" : self.kill,
            "value" : self.value
            }

        self.initTime = wpilib.getTime()
        self.prevTime: float = 0.0
        self.msgTopic = telemTable.getStringTopic(tags.MSG).subscribe("default")

        self.data: dict[str, Any] = ThrowDict()
        self.procs: list[Node] = [ ] # NODES / including hardware

        self.driveController = wpilib.XboxController(0)
        self.armController = wpilib.XboxController(1)
        self.buttonPanel = wpilib.Joystick(2)


        # TODO: imperative refactor
        inits.makeDemo(self.procs, self.data, not self.isSimulation())
        # inits.makeFlymer(self.procs, self.data, not self.isSimulation())


        # CLEANUP: move telem into the robot class
        found = False
        for x in self.procs:
            found = x.__class__ == telemetryNode.TelemNode
            if found: break
        assert(found)

        # TODO: imperative refactor
        found = False
        for kv in self.data.items():
            if isinstance(kv[1], Input.InputProfile):
                found = True
                kv[1].update(self.driveController, self.armController, self.buttonPanel)
                break
        assert(found)




    # Execute all running proc nodes
    def robotPeriodic(self) -> None:


        self.data.update({ tags.DT : wpilib.getTime() - self.prevTime }) # ???: Is using one DT sample per frame accurate enough? or should each node sample?
        self.data.update({ tags.TIME_SINCE_INIT : wpilib.getTime() - self.initTime })
        self.prevTime = wpilib.getTime()


        if self.isAutonomousEnabled(): state = tags.OP_AUTO
        elif self.isTeleopEnabled():   state = tags.OP_TELEOP
        else:                          state = tags.OP_DISABLED
        self.data.update({ tags.OPMODE : state })


        self.data.update({ tags.ISREAL : not self.isSimulation() })


        # TODO: exception saftey
        # TODO: imperitive refactor
        self.data[tags.INPUT].update(self.driveController, self.armController, self.buttonPanel)



        # respond to commands +==========================================================

        for x in self.msgTopic.readQueue():
            res = self.parseAndRespondMsg(x.value)
            if res is not None: telemTable.putString(tags.RES, res)

        # run procs +=====================================================================

        ordered: dict[int, list[Node]] = {}
        maxPri = 0
        for val in self.procs:

            maxPri = max(val.priority, maxPri)

            # order nodes based on priority
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
            err = f"Exception in node \"{x.name}\": {traceback.format_exc(chain=False)}"
            reportErr(err)

            if not self.isReal: print(err) # NOTE: this is a litte redundant, and probably tanks performance





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
                    reportErr(f"Exception in command \'{k}\': " + repr(e))

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
                        reportErr(f"Exception in command \'{n.name}\': " + repr(e))

                    found  = True
                    break

        return msgStamp + ":" + response













    # COMMANDS ==================================================================================

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

    def value(self, args: list[str]) ->str:
        if len(args) != 1: return f"Invalid arg count {len(args)}."

        if args[0] in self.data:
            return str(self.data[args[0]])
        else: return f"{args[0]} is not in data."


    def kill(self, args: list[str]) -> str:
        raise SystemExit()

# end robot class





if __name__ == "__main__":
    wpilib.run(Robot)

    """
    driveController = wpilib.XboxController(0)
    armController = wpilib.XboxController(1)
    buttonPanel = wpilib.Joystick(2)

    inp = Input.FlymerInputProfile()
    inp.update(driveController, armController, buttonPanel)

    t = telemetryNode.TelemNode([tags.INPUT])
    t.tick({ tags.INPUT : inp })
    """

    # uncomment these for debugging the init func
    # x = Robot()
    # x.robotInit()