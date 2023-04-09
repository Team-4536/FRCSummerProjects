
from typing import Callable, Any
import wpilib
import rev
from node import *
from enum import Enum
import ntcore




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
        self.name = "input"


    def tick(self, data: dict[str, Any]) -> None:

        input = FlymerInputProfile()

        input.drive = (self.driveController.getLeftX(), self.driveController.getLeftY())
        input.turning = self.driveController.getRightX()
        input.brakeToggle = self.driveController.getAButtonPressed()

        input.lift = self.armController.getLeftY()
        input.turret = self.armController.getLeftX()
        input.retract = self.armController.getRightY()
        input.grabToggle = self.armController.getAButtonPressed()


        data.update({ "input":input })
