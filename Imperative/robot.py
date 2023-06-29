

import wpilib
import rev
import navx


class PogoBot(wpilib.TimedRobot):


    def robotInit(self) -> None:

        self.controller = wpilib.XboxController(0)

        self.FLPiston = wpilib.DoubleSolenoid(0, wpilib.PneumaticsModuleType.CTREPCM, 0, 1)
        self.FRPiston = wpilib.DoubleSolenoid(0, wpilib.PneumaticsModuleType.CTREPCM, 2, 3)
        self.BLPiston = wpilib.DoubleSolenoid(0, wpilib.PneumaticsModuleType.CTREPCM, 4, 5)
        self.BRPiston = wpilib.DoubleSolenoid(0, wpilib.PneumaticsModuleType.CTREPCM, 6, 7)

        self.extended = False


    def robotPeriodic(self) -> None:
        return super().robotPeriodic()

    def teleopInit(self) -> None:
        return super().teleopInit()

    def teleopPeriodic(self) -> None:

        if(self.controller.getAButtonPressed()):
            self.extended = not self.extended
            toggle = wpilib.DoubleSolenoid.Value.kForward if self.extended else wpilib.DoubleSolenoid.Value.kReverse

            self.FLPiston.set(toggle)
            self.FRPiston.set(toggle)
            self.BLPiston.set(toggle)
            self.BRPiston.set(toggle)


if __name__ == "__main__":
    wpilib.run(PogoBot)