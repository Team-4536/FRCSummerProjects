from node import *
import wpilib
import math
import utils.tags as tags
from hardware.Input import FlymerInputProfile
from utils.V2 import V2
from typing import Any
from utils.PIDController import PIDController

class SwerveProf(Node):

    def __init__(self) -> None:
        self.name = "SwerveProfile"
        self.priority = NODE_PROF
        
        self.brakes = 1

    def tick(self, data: dict[str, Any]) -> None:

        input = data.get(tags.INPUT)
        if type(input) is not FlymerInputProfile: return

        #grab controller inputs
        input = data[tags.INPUT]
        assert type(input) == FlymerInputProfile

        #raw inputs

        # Y = Up/Down
        # X = Left/Right
        # Z = Turning
        # Brakes are nothing right now

        inputY = input.drive[0]
        inputX = input.drive[1]
        inputZ = input.turning

        #brake input toggle
        brake = input.brakeToggle
        if brake == True:
            self.brakes = self.brakes * -1

        #gyro input
        inputGyro = 0 #constant for testing

        #assign inputs to vectors
        leftStick = V2(inputX, inputY)
        FLTurningVector = V2(math.cos(45) * inputZ, math.cos(45) * inputZ)
        FRTurningVector = V2(-math.cos(45) * inputZ, math.cos(45) * inputZ)
        BLTurningVector = V2(math.cos(45) * inputZ, -math.cos(45) * inputZ)
        BRTurningVector = V2(-math.cos(45) * inputZ, -math.cos(45) * inputZ)

        #field oriented offset (change later to gyro readings)
        leftStick = leftStick.rotateDegrees(-inputGyro)

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
        if self.brakes == -1:
            FLTarget = 135
            FRTarget = 225
            BLTarget = 45
            BRTarget = 315
            FLPower = 0
            FRPower = 0
            BLPower = 0
            BRPower = 0

        """---------------------------------------"""

        #PID Controller for  steering
        FLPID = PIDController(10.0, 0.0, 0.0)
        FRPID = PIDController(10.0, 0.0, 0.0)
        BLPID = PIDController(10.0, 0.0, 0.0)
        BRPID = PIDController(10.0, 0.0, 0.0)

        """---------------------------------------"""

        #calculate error

        #FL
        FLFakeError = FLTarget - FLPosAngle
        FLTarget = 0
        FLPos = -(FLFakeError)
        while FLPos > 90:
            FLPos = FLPos - 180
            FLPower = -(FLPower)
        FLSteeringError = FLTarget - FLPos
        while FLSteeringError > 90:
            FLSteeringError = FLSteeringError - 180
            FLPower = -(FLPower)
        if FLSteeringError > 180:
            FLSteeringError = FLSteeringError - 360

        #FR
        FRFakeError = FRTarget - FRPosAngle
        FRTarget = 0
        FRPos = -(FRFakeError)
        while FRPos > 90:
            FRPos = FRPos - 180
            FRPower = -(FRPower)
        FRSteeringError = FRTarget - FRPos
        while FRSteeringError > 90:
            FRSteeringError = FRSteeringError - 180
            FRPower = -(FRPower)
        if FRSteeringError > 180:
            FRSteeringError = FRSteeringError - 360

        #BL
        BLFakeError = BLTarget -BLPosAngle
        BLTarget = 0
        BLPos = -(BLFakeError)
        while BLPos > 90:
            BLPos = BLPos - 180
            BLPower = -(BLPower)
        BLSteeringError = BLTarget - BLPos
        while BLSteeringError > 90:
            BLSteeringError = BLSteeringError - 180
            BLPower = -(BLPower)
        if BLSteeringError > 180:
            BLSteeringError = BLSteeringError - 360

        #BR
        BRFakeError = BRTarget - BRPosAngle
        BRTarget = 0
        BRPos = -(BRFakeError)
        while BRPos > 90:
            BRPos = BRPos - 180
            BRPower = -(BRPower)
        BRSteeringError = BRTarget - BRPos
        while BRSteeringError > 90:
            BRSteeringError = BRSteeringError - 180
            BRPower = -(BRPower)
        if BRSteeringError > 180:
            BRSteeringError = BRSteeringError - 360

        #assign motor powers
        FLSteeringPower = FLPID.tickErr(FLSteeringError / 360, data[tags.DT])
        FRSteeringPower = FRPID.tickErr(FRSteeringError / 360, data[tags.DT])
        BLSteeringPower = BLPID.tickErr(BLSteeringError / 360, data[tags.DT])
        BRSteeringPower = BRPID.tickErr(BRSteeringError / 360, data[tags.DT])

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