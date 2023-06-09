from node import *
import utils.tags as tags


import wpilib
import robot


# Note: true = set solenoid to forward, false = set solenoid reverse
class PneumaticsNode(Node):

    def __init__(self, pref: str, solenoid: wpilib.DoubleSolenoid) -> None:
        self.pref = pref

        self.doubleSolenoid = solenoid

        self.name = pref + tags.DBLSOLENOID_NAME
        self.priority = NODE_HARDWARE


    def tick(self, data: dict[str, Any]):

        val = data.get(self.pref + tags.DBLSOLENOID_STATE)
        if type(val) == bool:
            end = wpilib.DoubleSolenoid.Value.kForward if val else wpilib.DoubleSolenoid.Value.kReverse

            if data[tags.OPMODE] != tags.OP_DISABLED:
                self.doubleSolenoid.set(end)

        rec = True if self.doubleSolenoid.get() else False
        data.update({ self.pref + tags.DBLSOLENOID_STATE : rec })






