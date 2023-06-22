import PIDController

class dualLimitSwitchMoterController:
    def __init__(self, limitSwitch1, limitSwitch2, kp: float = 0, ki: float = 0, kd:float = 0) -> None:
        if limitSwitch1 > limitSwitch2:
            self.upperSwitch = limitSwitch1
            self.lowerSwitch = limitSwitch2
        else:
            self.upperSwitch = limitSwitch2
            self.lowerSwitch = limitSwitch1
        self.PID = PIDController.PIDController(kp, ki, kd)
    
    def resetSwitch(self, position) -> None:
    

    def tick(self, target: float, position: float, dt: float, isTouchingSwitch) -> float:
        if isTouchingSwitch:
            self.resetSwitch(position)
        out = None
        if position > self.upperSwitch:
            out = self.PID.tick(target, self.upperSwitch, dt)
        elif position < self.lowerSwitch:
            out = self.PID.tick(target, self.lowerSwitch, dt)
        else:
            out = self.PID.tick(target, position, dt)
        return out
