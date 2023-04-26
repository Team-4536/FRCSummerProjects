from node import *
from hardware.DCMotors import DCMotorNode
from hardware.encoders import *
import random
import utils.tags as tags


class EncoderSimNode(Node):


    def __init__(self, pref: tags.Tag, motor: DCMotorNode, encoder: VirtualEncoderNode) -> None:

        self.pref = pref

        self.name = pref + tags.ENCODER_SIM_NAME
        self.priority = NODE_SIM

        self.motor = motor
        self.encoder = encoder

        self.position: float = 0.0 # in rotations
        self.velocity: float = 0.0 # in RPS




    def tick(self, data: dict[str, Any]) -> None:

        dt = data["dt"]
        assert(type(dt) == float)

        speedDelta = (self.motor.controller.get() - self.velocity) * 0.8 # targeted speed relative to last

        self.velocity += speedDelta
        self.position += self.velocity * dt
        self.encoder.realPosition = self.position

        data.update({ self.pref + tags.ENCODER_SIM_VELOCITY : self.velocity })



    def command(self, args: list[str], data: dict[str, Any]) -> str:

        if len(args) < 1:
            return "missing args: [info]"

        if args[0] == "info":
            return f"Motor: {self.motor.name}, Encoder: {self.encoder.name}"

        return "invalid args. actual: [info]"







