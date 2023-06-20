
from typing import Callable, Any
import wpilib
from node import *
import ntcore
import utils.tags as tags
import math


def deadZone(input: float) -> float:
        if(abs(input) < 0.1):
            return 0.0
        else:
            return float(input)

class FlymerInputProfile:

    def __init__(self) -> None:
        self.drive: tuple[float, float] = (0.0, 0.0)
        self.turning: float = 0.0

        self.lift: float = 0.0
        self.turret: float = 0.0
        self.retract: float = 0.0

        self.brakeToggle: bool = False
        self.grabToggle: bool = False

    def publish(self, name: str, table: ntcore.NetworkTable) -> None:

        # CLEANUP: these should probs be in the tags file
        table.putNumber(name + "/driveX", self.drive[0])
        table.putNumber(name + "/driveY", self.drive[1])
        table.putNumber(name + "/turning", self.turning)
        table.putNumber(name + "/lift", self.lift)
        table.putNumber(name + "/turret", self.turret)
        table.putNumber(name + "/retract", self.retract)
        table.putNumber(name + "/brakeToggle", int(self.brakeToggle))
        table.putNumber(name + "/grabToggle", int(self.grabToggle))





class FlymerInputNode(Node):

    def __init__(self) -> None:
        self.driveController = wpilib.XboxController(0)
        self.armController = wpilib.XboxController(1)
        self.buttonPanel = wpilib.Joystick(2)

        self.priority = NODE_FIRST
        self.name = tags.INPUT


    def tick(self, data: dict[str, Any]) -> None:

        input = FlymerInputProfile()

        input.drive = (deadZone(self.driveController.getLeftX()), deadZone((-self.driveController.getLeftY())))
        input.turning = deadZone(self.driveController.getRightX())
        input.brakeToggle = self.driveController.getAButtonPressed()

        input.lift = deadZone(self.armController.getLeftY())
        input.turret = deadZone(self.armController.getLeftX())
        input.retract = deadZone(self.armController.getRightY())
        input.grabToggle = self.armController.getAButtonPressed()


        data.update({ tags.INPUT : input })

class DemoInputProfile:

    def __init__(self) -> None:
        self.driveLeft: float = 0.0
        self.driveRight: float = 0.0


    def publish(self, name: str, table: ntcore.NetworkTable) -> None:

        # CLEANUP: these should probs be in the tags file
        table.putNumber(name + "/driveL", self.driveLeft)
        table.putNumber(name + "/driveR", self.driveRight)
        
class DemoInputNode(Node):

    def __init__(self) -> None:
        self.driveController = wpilib.XboxController(0)

        self.priority = NODE_FIRST
        self.name = tags.INPUT


    def tick(self, data: dict[str, Any]) -> None:

        input = DemoInputProfile()

        input.driveLeft = deadZone((-self.driveController.getLeftY()))
        input.driveRight = deadZone((-self.driveController.getRightY()))



        data.update({ tags.INPUT : input })