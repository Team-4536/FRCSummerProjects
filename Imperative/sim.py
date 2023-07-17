import rev
import wpimath.system.plant as plant
import wpimath.system as sys
import wpilib
import navx
import random
from math import degrees, radians
import math

from virtualGyro import VirtualGyro
from real import V2f, angleWrap


# TODO: get real measurements
ENCODER_STD_DEV = 0.001

class EncoderSim:

    def __init__(self, motor: rev.CANSparkMax, motorSpec: plant.DCMotor, encoder: rev.RelativeEncoder, inertia: float, gearing: float) -> None:

        self.motor = motor
        self.encoder = encoder

        self.motorSpec = motorSpec
        self.inertia = inertia
        self.linearSys = plant.LinearSystemId.DCMotorSystem(self.motorSpec, self.inertia, gearing)
        # NOTE: state[0] is position (in radians), state[1] is velocity (in rads/sec)
        self.state: list[float] = [ 0, 0 ]

    def update(self, dt: float):

        inputVec = [self.motor.get() * self.motorSpec.nominalVoltage] # TODO: check if this is correct
        self.state = self.linearSys.calculateX(self.state, inputVec, dt)

        self.encoder.setPosition(random.normalvariate(self.state[0] / (2 * math.pi), ENCODER_STD_DEV))




# NOTE: directly contains 8 encoder sims to manage hardware
class SwerveSim:

    def __init__(self,
                 driveMotors: list[rev.CANSparkMax],
                 driveEncoders: list[rev.RelativeEncoder],
                 steeringMotors: list[rev.CANSparkMax],
                 steeringEncoders: list[rev.RelativeEncoder],
                 gyro: VirtualGyro,
                 podPositions: list[V2f], # in meters
                 wheelCirc: float) -> None: # also in meters


        self.driveSims: list[EncoderSim] = [ ]
        for i in range(4):
           sim = EncoderSim(driveMotors[i], plant.DCMotor.NEO(1), driveEncoders[i], 0.005, 6.12)
           self.driveSims.append(sim)

        self.steerSims: list[EncoderSim] = [ ]
        for i in range(4):
           sim = EncoderSim(steeringMotors[i], plant.DCMotor.NEO(1), steeringEncoders[i], 0.001, 1)
           # TODO: get real inertia vals
           # TODO: is this the corect gearing?
           self.steerSims.append(sim)

        self.gyro = gyro

        self.position = V2f(0, 0)
        self.rotation: float = 0.0
        self.wheelCirc = wheelCirc
        self.wheelPositions = podPositions

        self.posDeltas: list[V2f] = [] # DEBUG

    # TODO: add real friction to wheel sims

    def update(self, dt: float):

        self.posDeltas: list[V2f] = []

        for i in range(0, 4):
            sim = self.driveSims[i]
            sim.update(dt)
            driveDelta = (sim.state[1] / (2 * math.pi)) * dt * self.wheelCirc # delta in meters

            sim.state[1] *= 0.9 # CLEANUP: friction hack

            sim = self.steerSims[i]
            sim.update(dt)
            steerPos = (sim.state[0] / (2 * math.pi)) * 360 # position in degrees

            sim.state[1] *= 0.5

            self.posDeltas.append((V2f(0, 1) * driveDelta).rotateDegrees(steerPos))

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

        self.gyro.setYaw(self.rotation)


