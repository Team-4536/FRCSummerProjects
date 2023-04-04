import wpilib
import ntcore
import robotpy
from process import *
import procTest

from typing import Any



class Robot(wpilib.TimedRobot):




    def robotInit(self) -> None:

        self.joystick = wpilib.Joystick(0)
        self.infoTable: ntcore.NetworkTable = ntcore.NetworkTableInstance.getDefault().getTable("RobotInfo")



        self.ctrls: dict[str, Callable[[str], None]] = { }

        self.hardware: dict[str, Any] = {  } # hardware objs
        self.data: dict[str, Any] = { } # continuous data
        self.temp: dict[str, Any] = { } # inter proc comms
        self.procs: dict[str, Process] = { } # processes

        self.procs["test log 0"] = procTest.makeLogProc("Hello p0!", PROC_FIRST)
        self.procs["test log 1"] = procTest.makeLogProc("Hello hardware!", PROC_HARDWARE)
        self.procs["test log last"] = procTest.makeLogProc("Hello last!", PROC_LAST)
        self.procs["test log first"] = procTest.makeLogProc("Hello first!", PROC_FIRST)




    def robotPeriodic(self) -> None:


        ordered: dict[int, list[tuple[str, Callable[[], None]]]] = {}
        maxPri = 0
        for key, val in self.procs.items():

            maxPri = max(val[0], maxPri)

            # execute all first pri procs
            if val[0] == PROC_FIRST:
                try:
                    val[1]()
                except:
                    print(f"Exception in process \"{key}\":")
                    raise
            # store the rest, ordered
            else:
                try:
                    if val[0] in ordered:
                        l = ordered[val[0]]
                        l.append((key, val[1]))
                    else:
                        l = ordered.update({ val[0] : [(key, val[1])] })

                except KeyError:
                    pass

        # execute other procs, in order
        for i in range(0, maxPri+1):

            if i in ordered:
                l = ordered[i]
                for x in l:
                    try:
                        x[1]()
                    except:
                        print(f"Exception in process \"{x[0]}\":")
                        raise

        raise SystemExit()














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

    r = Robot()
    r.robotInit()
    r.robotPeriodic()
    # wpilib.run(Robot)
