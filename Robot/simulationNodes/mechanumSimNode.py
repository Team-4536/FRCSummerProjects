# NOTE: depends on proper motor simulation being done
"""

from typing import Any
from node import *
import wpimath.kinematics
import wpimath.geometry

from hardware import DCMotors, Encoders, gyros



class mechSimNode(Node):

    # NOTE: wheel positions is relative to robot center, in meters
    # NOTE: start pos is in meters
    # NOTE: wheel circ is in meters
    def __init__(self, startPos: tuple[float, float], gyro: gyros.GyroNode, motors: list[DCMotors.DCMotorNode], encoders: list[Encoders.EncoderNode], wheelPositions: list[tuple[float, float]], wheelCirc: float) -> None:

        for i in range(0, 4):
            assert(type(motors[i].controller) is DCMotors.VirtualController)
            assert(type(encoders[i]) is Encoders.VirtualEncoderNode)
        assert(type(gyro.gyro) is gyros.VirtualGyro)

        self.name = "mechSim"
        self.priority = NODE_SIM

        self.motors = motors
        self.gyro = gyro
        self.encoders = encoders
        self.wheelCirc = wheelCirc





    def tick(self, data: dict[str, Any]) -> None:

        dists: list[float] = []
        for i in range(0, 4):
            angTravel = self.encoders[i].getRotations()
            dists[i] = 






"""