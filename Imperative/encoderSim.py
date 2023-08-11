import rev
import wpimath.system.plant as plant
import wpimath.system as sys
import random
import math

# TODO: get real measurements
ENCODER_STD_DEV = 0.001

class EncoderSim:

    def __init__(self, motorSpec: plant.DCMotor, inertia: float, gearing: float) -> None:

        self.motorSpec = motorSpec
        self.inertia = inertia
        self.linearSys = plant.LinearSystemId.DCMotorSystem(self.motorSpec, self.inertia, gearing)
        # NOTE: state[0] is position (in radians), state[1] is velocity (in rads/sec)
        self.state: list[float] = [ 0, 0 ]

    def update(self, dt: float, motor: rev.CANSparkMax, encoder: rev.RelativeEncoder):
        # TODO: check if this is correct
        inputVec = [motor.get() * self.motorSpec.nominalVoltage]
        self.state = self.linearSys.calculateX(self.state, inputVec, dt)
        encoder.setPosition(random.normalvariate(self.state[0] / (2 * math.pi), ENCODER_STD_DEV))
