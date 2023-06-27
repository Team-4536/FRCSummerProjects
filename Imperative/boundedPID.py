import PIDController
from wpilib import DigitalInput
class dualLimitSwitchMoterController:
    def __init__(self, knownEncoderPoz, encoderDistance, kp: float = 0, ki: float = 0, kd:float = 0) -> None:
        self.encoderBase = knownEncoderPoz
        self.encoderDistance = encoderDistance
        self.PID = PIDController.PIDController(kp, ki, kd)

    def tick(self, target: float, position: float, dt: float, limitSwitch1, limitSwitch2) -> float:
        out = None
        if position > self.encoderBase + self.encoderDistance:
            out = self.PID.tick(target, self.encoderBase + self.encoderDistance, dt)
        elif position < self.encoderBase:
            out = self.PID.tick(target, self.encoderBase, dt)
        else:
            out = self.PID.tick(target, position, dt)
        return out
