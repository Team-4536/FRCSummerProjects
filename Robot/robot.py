import wpilib
import ntcore
import robotpy




class Robot(wpilib.TimedRobot):

    def robotInit(self) -> None:

        self.joystick = wpilib.Joystick(0)
        self.t: ntcore.NetworkTable = ntcore.NetworkTableInstance.getDefault().getTable("RobotInfo")



    def robotPeriodic(self) -> None:

        self.t.putNumber("FL Pwr", -self.joystick.getY())







    def teleopInit(self) -> None:
        pass

    def teleopPeriodic(self) -> None:
        pass



    def autonomousInit(self) -> None:
        pass

    def autonomousPeriodic(self) -> None:
        pass




    def disabledInit(self) -> None:
        pass

    def disabledPeriodic(self) -> None:
        pass








if __name__ == "__main__":
    wpilib.run(Robot)