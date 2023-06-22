
import rev
import wpimath.system.plant as plant
import wpimath.system as sys




class EncoderSim:

    # NOTE: state[0] is position, state[1] is velocity
    def __init__(self, motorSpec: plant.DCMotor, inertia: float) -> None:
        self.motorSpec = motorSpec
        self.inertia = inertia
        self.linearSys = plant.LinearSystemId.DCMotorSystem(self.motorSpec, self.inertia, 1)
        self.state = [ 0, 0 ]

    def update(self, motor: rev.CANSparkMax, encoder: rev.RelativeEncoder, dt: float):

        inputVec = [motor.get() * self.motorSpec.nominalVoltage] # TODO: check if this is correct
        self.state = self.linearSys.calculateX(self.state, inputVec, dt)
        encoder.setPosition(self.state[0])

# TODO: figure a way to make this more functional ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^


