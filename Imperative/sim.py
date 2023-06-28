from real import V2f, angleWrap
import rev
import wpimath.system.plant as plant
import wpimath.system as sys
import wpilib




class EncoderSim:

    def __init__(self, motor: rev.CANSparkMax, motorSpec: plant.DCMotor, encoder: rev.RelativeEncoder, inertia: float) -> None:

        self.motor = motor
        self.encoder = encoder

        self.motorSpec = motorSpec
        self.inertia = inertia
        self.linearSys = plant.LinearSystemId.DCMotorSystem(self.motorSpec, self.inertia, 1)
        # NOTE: state[0] is position, state[1] is velocity
        self.state: list[float] = [ 0, 0 ]

    def update(self, dt: float):

        inputVec = [self.motor.get() * self.motorSpec.nominalVoltage] # TODO: check if this is correct
        self.state = self.linearSys.calculateX(self.state, inputVec, dt)
        self.encoder.setPosition(self.state[0])

# TODO: figure a way to make this more functional ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^




class SwerveSim:

    def __init__(self,
                 driveMotors: list[rev.CANSparkMax],
                 driveEncoders: list[rev.RelativeEncoder],
                 steeringMotors: list[rev.CANSparkMax],
                 steeringEncoders: list[rev.RelativeEncoder],
                 podPositions: list[V2f], # in meters
                 wheelCirc: float) -> None: # also in meters


        self.driveSims: list[EncoderSim] = [ ]
        for i in range(4):
           sim = EncoderSim(driveMotors[i], plant.DCMotor.NEO(1), driveEncoders[i], 1)
           self.driveSims.append(sim)

        self.steerSims: list[EncoderSim] = [ ]
        for i in range(4):
           sim = EncoderSim(steeringMotors[i], plant.DCMotor.NEO(1), steeringEncoders[i], 0.4) # TODO: get real friction vals
           self.steerSims.append(sim)

        self.position = V2f(0, 0)
        self.rotation: float = 0.0
        self.wheelCirc = wheelCirc
        self.wheelPositions = podPositions

        self.posDeltas: list[V2f] = [] # DEBUG

    # TODO: get real swerve gearings
    # TODO: add real friction to wheel sims
    # TODO: get real measurements for wheel inertia/friction
    # TODO: vary the "inertia" of each mechanism with speed/rotation speed of drive // add extra resistance when braking and going against current speed somehow

    def update(self, dt: float):

        self.posDeltas: list[V2f] = []

        for i in range(0, 4):
            sim = self.driveSims[i]
            sim.update(dt)
            driveDelta = sim.state[1] * dt

            sim.state[1] *= 0.9 # CLEANUP: friction hack

            sim = self.steerSims[i]
            sim.update(dt)
            steerPos = sim.state[0]

            sim.state[1] *= 0.9

            self.posDeltas.append((V2f(0, 1) * driveDelta * self.wheelCirc).rotateDegrees(steerPos * 360))

        posChange = (self.posDeltas[0] + self.posDeltas[1] + self.posDeltas[2] + self.posDeltas[3]) / 4
        self.position += posChange.rotateDegrees(self.rotation)

        # TODO: right now position and angle are beign updated via averaging wheel deltas. Find out if this is correct

        angleDeltas: list[float] = [ ]
        for i in range(0, 4):
            inital = self.wheelPositions[i]
            new = inital + self.posDeltas[i]
            angle = angleWrap(inital.getAngle() - new.getAngle())
            angleDeltas.append(angle)

        self.rotation += (angleDeltas[0] + angleDeltas[1] + angleDeltas[2] + angleDeltas[3]) / 4


