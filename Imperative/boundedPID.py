import PIDController
from wpilib import DigitalInput
class oneLimitSwitchMoterController:
    def __init__(self, knownEncoderPoz, encoderDistance, kp: float = 0, ki: float = 0, kd:float = 0) -> None:
        self.encoderBase = knownEncoderPoz
        self.encoderDistance = encoderDistance
        self.PID = PIDController.PIDController(kp, ki, kd)

    def tick(self, target: float, position: float, dt: float) -> float:
        out = None
        if target > self.encoderBase + self.encoderDistance:
            out = self.PID.tick(self.encoderBase + self.encoderDistance, position, dt)
        elif target < self.encoderBase:
            out = self.PID.tick(self.encoderBase, position, dt)
        else:
            out = self.PID.tick(target, position, dt)
        return out
    
class dualLimitSwitchMoterController:
    def __init__(self, upperEncoder, lowerEncoder, kp: float = 0, ki: float = 0, kd:float = 0) -> None:
        self.upperEncoder = upperEncoder
        self.lowerEncoder = lowerEncoder
        self.PID = PIDController.PIDController(kp, ki, kd)

    def tick(self, target: float, position: float, dt: float) -> float:
        out = None
        if target > self.upperEncoder:
            out = self.PID.tick(self.upperEncoder, position, dt)
        elif target < self.lowerEncoder:
            out = self.PID.tick(self.lowerEncoder, position, dt)
        else:
            out = self.PID.tick(target, position, dt)
        return out