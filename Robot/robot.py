import wpilib
import ntcore
import robotpy
from node import *
from node import Node

import hardware.DCMotors
from typing import Any

import telemetryNode
import timeNode
import utils.profiling





class Robot(wpilib.TimedRobot):




    def robotInit(self) -> None:

        self.ctrls: dict[str, Callable[[str], None]] = { }

        self.hardware: dict[str, Any] = {  } # hardware objs
        self.data: dict[str, Any] = { } # continuous data
        self.procs: list[Node] = [ ] # processes



        self.hardware.update({"motor 1" : hardware.DCMotors.SparkImpl(0)})
        self.hardware.update({"motor 2" : hardware.DCMotors.SparkImpl(1)})

        self.procs.append(telemetryNode.TelemNode(self.hardware))

        self.procs.append(timeNode.TimeNode(self.data))









    # Execute all running proc nodes
    def robotPeriodic(self) -> None:


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





    def runProcessNodeSafe(self, x: Node):
        try:
            x.execute(self.data)
        except Exception as exception:
            print(f"Exception in node \"{x.name}\": {repr(exception)}")









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