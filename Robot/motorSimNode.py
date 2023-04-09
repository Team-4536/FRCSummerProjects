from node import *
from hardware.DCMotors import DCMotorNode
from hardware.Encoders import *
import random








class motorSimNode(Node):


    def __init__(self, motor: DCMotorNode, encoder: VirtualEncoder) -> None:

        self.name = "motorSim"
        self.priority = NODE_SIM

        self.motor = motor
        self.encoder = encoder

        self.position: float = 0.0 # in rotations
        self.velocity: float = 0.0 # in RPS




    def tick(self, data: dict[str, Any]) -> None:

        dt = data["dt"]
        assert(type(dt) == float)

        speedDelta = (self.motor.getRPS() - self.velocity) * 0.8 # targeted speed relative to last

        self.velocity += speedDelta
        # self.velocity += (self.motor.getRPM() / 60) * dt
        self.position += self.velocity * dt
        self.encoder.realPosition = self.position

        data.update({ "simVelocity" : self.velocity })
        data.update({ "simPosition" : self.position })
        data.update({ "speedDelta" : speedDelta })




    def command(self, args: list[str], data: dict[str, Any]) -> str:

        if len(args) < 1:
            return "missing args: [info]"

        if args[0] == "info":
            return f"Motor: {self.motor.name}, Encoder: {self.encoder.name}"

        return "invalid args. actual: [info]"







