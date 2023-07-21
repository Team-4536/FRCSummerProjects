import wpilib


def deadZone(input: float) -> float:
        if(abs(input) < 0.1):
            return 0.0
        else:
            return float(input)



class FlymerInputs():

    # TODO: switch keyboard/controller modes from sundial

    def __init__(self, driveCtrlr: wpilib.XboxController, armCtrlr: wpilib.XboxController) -> None:
        self.driveX = deadZone(driveCtrlr.getLeftX())
        self.driveY = deadZone((-driveCtrlr.getLeftY()))
        self.turning = deadZone(driveCtrlr.getRightX())
        # self.turning = deadZone(armCtrlr.getLeftX())

        self.gyroReset = driveCtrlr.getYButtonPressed()

        self.brakeToggle = driveCtrlr.getBButtonPressed()

        self.lift = deadZone(armCtrlr.getLeftY())
        self.turret = deadZone(armCtrlr.getLeftX())
        self.retract = deadZone(armCtrlr.getRightY())
        self.grabToggle = armCtrlr.getAButtonPressed()



class DemoInputs():
    def __init__(self, driveCtrlr: wpilib.XboxController) -> None:
        self.drive = -deadZone(driveCtrlr.getLeftY())
        self.turning = deadZone(driveCtrlr.getRightX())
        self.turret = driveCtrlr.getRightTriggerAxis() - driveCtrlr.getLeftTriggerAxis()