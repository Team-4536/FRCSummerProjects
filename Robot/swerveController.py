from node import *
import wpilib
import math
import utils.tags as tags
from hardware.Input import FlymerInputProfile
from utils.V2 import V2
from typing import Any
from utils.PIDController import PIDController

#next thing to do: change brake mode from toggle to happen until input received

class SwerveProf:

    def __init__(self, turnList, driveList, turnEncoders, driveEncoders) -> None:

        self.brakes = 1

        #choose between brake or hold position when no input is given (if false brake will be a toggle on button "A")
        self.brakeDefault = False

        self.FLPID = PIDController(10.0, 0.0, 0.0)
        self.FRPID = PIDController(10.0, 0.0, 0.0)
        self.BLPID = PIDController(10.0, 0.0, 0.0)
        self.BRPID = PIDController(10.0, 0.0, 0.0)

        assert len(turnList) == 4
        assert len(driveList) == 4
        assert len(turnEncoders) == 4
        assert len(driveEncoders) == 4
        
        self.turnList = turnList
        self.driveList = driveList
        self.turnEncoders = turnEncoders
        self.driveEncoders = driveEncoders

    def tick(self, data: dict[str, Any]) -> None:


        #grab controller inputs
        input = getOrAssert(tags.INPUT, FlymerInputProfile, data)

        #raw inputs

        # Y = Up/Down
        # X = Left/Right
        # Z = Turning
        # Brakes are nothing right now

        inputY = input.driveY
        inputX = input.driveX
        inputZ = input.turning

        #brake input toggle
        brake = input.brakeToggle
        if brake == True:
            self.brakes = self.brakes * -1

        #gyro input
        inputGyro = 0 #constant for testing, can be changed to simulate gyro input

        #assign inputs to vectors
        leftStick = V2(inputX, inputY)

        FLTurningVector = V2(math.cos(45) * inputZ, math.cos(45) * inputZ)
        FRTurningVector = V2(-math.cos(45) * inputZ, math.cos(45) * inputZ)
        BLTurningVector = V2(math.cos(45) * inputZ, -math.cos(45) * inputZ)
        BRTurningVector = V2(-math.cos(45) * inputZ, -math.cos(45) * inputZ)

        #field oriented offset (change later to gyro readings)
        leftStick = leftStick.rotateDegrees(-data[tags.GYRO_YAW])

        #joystick scalar (adjusts speed depending on how far you move the joysticks)
        inputScalar = leftStick.getLength() + abs(inputZ)
        if inputScalar > 1:
            inputScalar = 1

        FLVector = leftStick.avg(FLTurningVector)
        FRVector = leftStick.avg(FRTurningVector)
        BLVector = leftStick.avg(BLTurningVector)
        BRVector = leftStick.avg(BRTurningVector)

        """-----------------------------------------"""

        #read encoder positions and set them angles
        FLPosAngle = (data[tags.FLSteering + tags.ENCODER_READING] % 1) * 360
        FRPosAngle = (data[tags.FRSteering + tags.ENCODER_READING] % 1) * 360
        BLPosAngle = (data[tags.BLSteering + tags.ENCODER_READING] % 1) * 360
        BRPosAngle = (data[tags.BRSteering + tags.ENCODER_READING] % 1) * 360

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

        #PID Controller for steering
        

        """---------------------------------------"""

        #calculate error

        #Function test
        def getSteeringError(Target, PosAngle, Power):
            FakeError = Target - PosAngle
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

            return SteeringError
        
        FLSteeringError = getSteeringError(FLTarget, FLPosAngle, FLPower)
        FRSteeringError = getSteeringError(FRTarget, FRPosAngle, FRPower)
        BLSteeringError = getSteeringError(BLTarget, BLPosAngle, BLPower)
        BRSteeringError = getSteeringError(BRTarget, BRPosAngle, BRPower)

        #hold position if no input is given
        if self.brakeDefault == False:
            if FLVector.noValue() == True and self.brakes != -1:
                FLSteeringError = 0
            if FRVector.noValue() == True and self.brakes != -1:
                FRSteeringError = 0
            if BLVector.noValue() == True and self.brakes != -1:
                BLSteeringError  = 0
            if BRVector.noValue() == True and self.brakes != -1:
                BRSteeringError = 0

        #assign motor powers
        FLSteeringPower = self.FLPID.tickErr(FLSteeringError / 360, data[tags.DT])
        FRSteeringPower = self.FRPID.tickErr(FRSteeringError / 360, data[tags.DT])
        BLSteeringPower = self.BLPID.tickErr(BLSteeringError / 360, data[tags.DT])
        BRSteeringPower = self.BRPID.tickErr(BRSteeringError / 360, data[tags.DT])

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
        data[tags.FLDrive + tags.MOTOR_SPEED_CONTROL] = FLPower
        data[tags.FRDrive + tags.MOTOR_SPEED_CONTROL] = FRPower
        data[tags.BLDrive + tags.MOTOR_SPEED_CONTROL] = BLPower
        data[tags.BRDrive + tags.MOTOR_SPEED_CONTROL] = BRPower

        #target steering values (in rotaions)
        data[tags.FLSteering + tags.MOTOR_SPEED_CONTROL] = FLSteeringPower
        data[tags.FRSteering + tags.MOTOR_SPEED_CONTROL] = FRSteeringPower
        data[tags.BLSteering + tags.MOTOR_SPEED_CONTROL] = BLSteeringPower
        data[tags.BRSteering + tags.MOTOR_SPEED_CONTROL] = BRSteeringPower