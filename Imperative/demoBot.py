import wpilib
import navx

import real
import inputs
import timing
import socketing


def tankController(drive: float, turning: float) -> list[float]:
    speeds = [
        drive + turning,
        drive - turning,
        drive + turning,
        drive - turning
    ]
    return real.normalizeWheelSpeeds(speeds)

def arcadeController(l: float, r: float) -> list[float]:
    return [ l, r, l, r ]



class DemoBot(wpilib.TimedRobot):

    def robotInit(self) -> None:
        self.server = socketing.Server(self.isReal())

        self.motors = [
            wpilib.Spark(2),
            wpilib.Spark(3),
            wpilib.Spark(1),
            wpilib.Spark(4)
        ]
        self.motors[1].setInverted(True)
        self.motors[3].setInverted(True)
        self.turretMotor = wpilib.Spark(0)
        self.turretEncoder = wpilib.Encoder(0, 1)

        self.gyro = navx.AHRS(wpilib.SPI.Port.kMXP)

        self.driveCtrlr = wpilib.XboxController(0)
        self.armCtrlr = wpilib.XboxController(1)

        self.time = timing.TimeData(None)

    def robotPeriodic(self) -> None:

        self.time = timing.TimeData(self.time)

        self.server.putUpdate("FLSpeed", self.motors[0].get())
        self.server.putUpdate("FRSpeed", self.motors[1].get())
        self.server.putUpdate("BLSpeed", self.motors[2].get())
        self.server.putUpdate("BRSpeed", self.motors[3].get())
        self.server.putUpdate("TurretSpeed", self.turretMotor.get())
        self.server.putUpdate("enabled", self.isEnabled())
        self.server.putUpdate("turretPos", self.turretEncoder.get())

        self.server.update(self.time.timeSinceInit)

    def teleopPeriodic(self) -> None:
        self.input = inputs.DemoInputs(self.driveCtrlr)

        self.driveSpeeds = tankController(self.input.drive * 0.8, self.input.turning)
        self.driveSpeeds = [s * 0.5 for s in self.driveSpeeds]
        for i in range(4): self.motors[i].set(self.driveSpeeds[i])

        self.turretMotor.set(self.input.turret * 0.5)

    def disabledPeriodic(self) -> None:
        for m in self.motors: m.set(0)
        self.turretMotor.set(0)




if __name__ == "__main__":
    wpilib.run(DemoBot)


