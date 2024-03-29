

class PIDController:

    def __init__(self, kp: float = 0, ki: float = 0, kd:float = 0) -> None:
        self.kp: float = kp
        self.ki: float = ki
        self.kd: float = kd

        self.integral: float = 0
        self.prevErr: float = 0



    # function returns the recommended force towards the target
    def tick(self, target: float, position: float, dt: float) -> float:
        error = target - position

        derivative = (error - self.prevErr) * dt
        self.integral += error * dt

        out = (self.kp * error) + (self.ki * self.integral) + (self.kd * derivative)
        self.prevErr = error

        return out




    # output will be the same sign as the input error
    def tickErr(self, error: float, dt: float) -> float:

        derivative = (error - self.prevErr) * dt
        self.integral += error * dt

        out = (self.kp * error) + (self.ki * self.integral) + (self.kd * derivative)
        self.prevErr = error

        return out


    def reset(self) -> None:
        self.integral = 0
        self.prevErr = 0


