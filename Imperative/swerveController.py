import wpilib
import math
from real import V2f, angleWrap
from typing import Any
from PIDController import PIDController

#next thing to do: change brake mode from toggle to happen until input received

# TODO: One time, for no reason, this thing had a stroke and wouldnt move


class SwerveController:

    def __init__(self, turnList, driveList, turnEncoders, driveEncoders) -> None:

        self.brakes = 1

        #choose between brake or hold position when no input is given (if false brake will be a toggle on button "A")
        self.brakeDefault = False

        kp = 2
        ki = 0
        kd = 12
        self.FLPID = PIDController(kp, ki, kd)
        self.FRPID = PIDController(kp, ki, kd)
        self.BLPID = PIDController(kp, ki, kd)
        self.BRPID = PIDController(kp, ki, kd)

        assert len(turnList) == 4
        assert len(driveList) == 4
        assert len(turnEncoders) == 4
        assert len(driveEncoders) == 4

        self.turnList = turnList
        self.driveList = driveList
        self.turnEncoders = turnEncoders
        self.driveEncoders = driveEncoders

    # Y = forward/back
    # X = Left/Right
    # Z = Turning, CW+
    def tick(self, inputX, inputY, inputZ, dt, brakeButtonPressed) -> None:

        #brake input toggle
        if brakeButtonPressed == True:
            self.brakes = self.brakes * -1

        #assign inputs to vectors
        leftStick = V2f(inputX, inputY)

        FLTurningVector = V2f(math.cos(45) * inputZ, math.cos(45) * inputZ)
        FRTurningVector = V2f(math.cos(45) * inputZ, -math.cos(45) * inputZ)
        BLTurningVector = V2f(-math.cos(45) * inputZ, math.cos(45) * inputZ)
        BRTurningVector = V2f(-math.cos(45) * inputZ, -math.cos(45) * inputZ)

        #joystick scalar (adjusts speed depending on how far you move the joysticks)
        inputScalar = leftStick.getLength() + abs(inputZ)
        if inputScalar > 1:
            inputScalar = 1

        FLVector = (leftStick + FLTurningVector) / 2
        FRVector = (leftStick + FRTurningVector) / 2
        BLVector = (leftStick + BLTurningVector) / 2
        BRVector = (leftStick + BRTurningVector) / 2

        """-----------------------------------------"""

        #read encoder positions and set them angles
        FLPosAngle = (self.turnEncoders[0].getPosition() % 1) * 360
        FRPosAngle = (self.turnEncoders[1].getPosition() % 1) * 360
        BLPosAngle = (self.turnEncoders[2].getPosition() % 1) * 360
        BRPosAngle = (self.turnEncoders[3].getPosition() % 1) * 360

        #wrap angles (0, 90, 180, -90)
        if FLPosAngle > 180:
            FLPosAngle = FLPosAngle - 360
        if FRPosAngle > 180:
            FRPosAngle = FRPosAngle - 360
        if BLPosAngle > 180:
            BLPosAngle = BLPosAngle - 360
        if BRPosAngle > 180:
            BRPosAngle = BRPosAngle - 360

        """----------------------------------------------------"""

        #separate vector values
        FLTarget = FLVector.getAngle()
        FRTarget = FRVector.getAngle()
        BLTarget = BLVector.getAngle()
        BRTarget = BRVector.getAngle()

        FLPower = FLVector.getLength()
        FRPower = FRVector.getLength()
        BLPower = BLVector.getLength()
        BRPower = BRVector.getLength()

        #set brakes
        if self.brakeDefault == False:
            if self.brakes == -1:
                FLTarget = 135
                FRTarget = 225
                BLTarget = 45
                BRTarget = 315
                FLPower = 0
                FRPower = 0
                BLPower = 0
                BRPower = 0
        else:
            if inputX == 0 and inputY == 0 and inputZ == 0:
                FLTarget = 135
                FRTarget = 225
                BLTarget = 45
                BRTarget = 315

        """---------------------------------------"""

        #calculate error

        def getSteeringError(Target, PosAngle, Power):
            # ???????????????????????????????????
            FakeError = -Target - PosAngle
            Pos = -(FakeError)
            while Pos > 90:
                Pos = Pos - 180
                Power = -(Power)
            SteeringError = -Pos
            while SteeringError > 90:
                SteeringError = SteeringError - 180
                Power = -(Power)
            if SteeringError > 180:
                SteeringError = SteeringError - 360

            return SteeringError, Power

        FLSteeringError, FLPower = getSteeringError(FLTarget, FLPosAngle, FLPower)
        FRSteeringError, FRPower = getSteeringError(FRTarget, FRPosAngle, FRPower)
        BLSteeringError, BLPower = getSteeringError(BLTarget, BLPosAngle, BLPower)
        BRSteeringError, BRPower = getSteeringError(BRTarget, BRPosAngle, BRPower)

        #hold position if no input is given
        if self.brakeDefault == False:
            if FLVector == V2f() and self.brakes != -1:
                FLSteeringError = 0
            if FRVector == V2f() and self.brakes != -1:
                FRSteeringError = 0
            if BLVector == V2f() and self.brakes != -1:
                BLSteeringError  = 0
            if BRVector == V2f() and self.brakes != -1:
                BRSteeringError = 0

        #assign motor powers
        FLSteeringPower = self.FLPID.tickErr(angleWrap(FLSteeringError / 360), dt)
        FRSteeringPower = self.FRPID.tickErr(angleWrap(FRSteeringError / 360), dt)
        BLSteeringPower = self.BLPID.tickErr(angleWrap(BLSteeringError / 360), dt)
        BRSteeringPower = self.BRPID.tickErr(angleWrap(BRSteeringError / 360), dt)

        """--------------------------------------------------"""

        #scalars
        maxPowerInput = 0
        maxSteeringInput = 0

        #power scalar
        #FL vs FR
        if abs(FLPower) > abs(FRPower):
            maxPowerInput = abs(FLPower)
        else:
            maxPowerInput = abs(FLPower)
        #BL
        if abs(BLPower) > maxPowerInput:
            maxPowerInput = abs(BLPower)
        #BR
        if abs(BRPower) > maxPowerInput:
            maxPowerInput = abs(BRPower)

        #set scalar value
        if maxPowerInput == 0:
            powerScalar = 1
        else:
            powerScalar = 1 / maxPowerInput


        #steering scalar
        #FL vs FR
        if abs(FLSteeringPower) > abs(FRSteeringPower):
            maxSteeringInput = abs(FLSteeringPower)
        else:
            maxSteeringInput = abs(FRSteeringPower)
        #BL
        if abs(BLSteeringPower) > maxSteeringInput:
            maxSteeringInput = abs(BLSteeringPower)
        #BR
        if abs(BRSteeringPower) > maxSteeringInput:
            maxSteeringInput = abs(BRSteeringPower)

        #set scalar value
        if maxSteeringInput == 0:
            steeringScalar = 1
        elif maxSteeringInput > 1:
            steeringScalar = 1 / maxSteeringInput
        else:
            steeringScalar = 1
        """============================================="""

        #final scaled motor speeds
        FLPower = (FLPower * powerScalar) * inputScalar
        FRPower = (FRPower * powerScalar) * inputScalar
        BLPower = (BLPower * powerScalar) * inputScalar
        BRPower = (BRPower * powerScalar) * inputScalar

        FLSteeringPower = FLSteeringPower * steeringScalar
        FRSteeringPower = FRSteeringPower * steeringScalar
        BLSteeringPower = BLSteeringPower * steeringScalar
        BRSteeringPower = BRSteeringPower * steeringScalar

        """============================================="""
        
        #drive values (power)
        self.driveList[0].set(FLPower)
        self.driveList[1].set(FRPower)
        self.driveList[2].set(BLPower)
        self.driveList[3].set(BRPower)

        #target steering values (in rotaions)
        self.turnList[0].set(FLSteeringPower)
        self.turnList[1].set(FRSteeringPower)
        self.turnList[2].set(BLSteeringPower)
        self.turnList[3].set(BRSteeringPower)
        