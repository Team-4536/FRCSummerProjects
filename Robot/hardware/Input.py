
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









"""
IMPORTANT NOTE:
because inputs need to be logged, and defining publishing functions for every one is a little much,
type schenanigans are being used.

Every single prop in an input prof are looped over and sent over
Props that arent floats or ints or bools with raise an assertion error
"""
class InputProfile:

    def update(self, driveController: wpilib.XboxController, armController: wpilib.XboxController, buttonPanel: wpilib.Joystick):
        raise NotImplementedError()

    def publish(self, name: str, table: ntcore.NetworkTable) -> None:
        for propKV in self.__dict__.items():

            if type(propKV[1]) == float:
                table.putNumber(name + "/" + propKV[0], propKV[1])
            elif type(propKV[1]) == int:
                table.putNumber(name + "/" + propKV[0], propKV[1])
            elif type(propKV[1]) == bool:
                table.putBoolean(name + "/" + propKV[0], propKV[1])









class FlymerInputProfile(InputProfile):

    def update(self, driveController: wpilib.XboxController, armController: wpilib.XboxController, buttonPanel: wpilib.Joystick):

        self.driveX = deadZone(driveController.getLeftX())
        self.driveY = deadZone((-driveController.getLeftY()))
        self.turning = deadZone(driveController.getRightX())
        self.brakeToggle = driveController.getAButtonPressed()

        self.lift = deadZone(armController.getLeftY())
        self.turret = deadZone(armController.getLeftX())
        self.retract = deadZone(armController.getRightY())
        self.grabToggle = armController.getAButtonPressed()



class DemoInputProfile(InputProfile):

    def update(self,
            driveController: wpilib.XboxController,
            armController: wpilib.XboxController,
            buttonPanel: wpilib.Joystick
            ):

        self.driveLeft = deadZone((-driveController.getLeftY()))
        self.driveRight = deadZone((-driveController.getRightY()))



class TestingInputProfile(InputProfile):

    def update(self,
            driveController: wpilib.XboxController,
            armController: wpilib.XboxController,
            buttonPanel: wpilib.Joystick):

        self.stick = deadZone((-driveController.getRightY()))
        self.buttonToggle = driveController.getAButtonPressed()
