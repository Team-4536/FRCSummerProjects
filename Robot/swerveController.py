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

    def tick(self, data: dict[str, Any]) -> None:

        input = data.get(tags.INPUT)
        if type(input) is not FlymerInputProfile: return

        #grab controller inputs
        input = data[tags.INPUT]
        assert type(input) == FlymerInputProfile

        #raw inputs
        inputY = input.drive[0]
        inputX = input.drive[1]
        inputZ = input.turning

        inputGyro = 0 #constant for testing

        #assign inputs to vectors
        leftStick = V2(inputX, inputY)
        FLTurningVector = V2(math.cos(45) * inputZ, math.cos(45) * inputZ)
        FRTurningVector = V2(-math.cos(45) * inputZ, math.cos(45) * inputZ)
        BLTurningVector = V2(math.cos(45) * inputZ, -math.cos(45) * inputZ)
        BRTurningVector = V2(-math.cos(45) * inputZ, -math.cos(45) * inputZ)

        #field oriented offset (change later from gyro to encoder readings)
        #leftStick = leftStick.rotateDegrees(-inputGyro)

        FLVector = leftStick.avg(FLTurningVector)
        FRVector = leftStick.avg(FRTurningVector)
        BLVector = leftStick.avg(BLTurningVector)
        BRVector = leftStick.avg(BRTurningVector)

        """-----------------------------------------"""

        #Encoder positions as angles and first wrap (0 to 359)
        FLPosAngle = (data[tags.FLSteering + tags.ENCODER_READING] % 1) * 360
        FRPosAngle = (data[tags.FRSteering + tags.ENCODER_READING] % 1) * 360
        BLPosAngle = (data[tags.BLSteering + tags.ENCODER_READING] % 1) * 360
        BRPosAngle = (data[tags.BRSteering + tags.ENCODER_READING] % 1) * 360

        if FLPosAngle > 180:
            FLPosAngle = FLPosAngle - 360
        if FRPosAngle > 180:
            FRPosAngle = FRPosAngle - 360
        if BLPosAngle > 180:
            BLPosAngle = BLPosAngle - 360
        if BRPosAngle > 180:
            BRPosAngle =BRPosAngle - 360

        """----------------------------------------------------"""

        #set values                  
        FLTarget = FLVector.getAngle()
        FRTarget = FRVector.getAngle()
        BLTarget = BLVector.getAngle()
        BRTarget = BRVector.getAngle()

        FLPower = FLVector.getLength()
        FRPower = FRVector.getLength()
        BLPower = BLVector.getLength()
        BRPower = BRVector.getLength()

        """---------------------------------------"""

        #scalar
        #find highest value
        maxPowerInput = 0

        #FL vs FR
        if abs(FLPower) > abs(FRPower):
            maxPowerInput = abs(FLPower)
        else: 
            maxPowerInput = abs(FRPower)
        #BL
        if abs(BLPower) > maxPowerInput:
            maxPowerInput = abs(BLPower)
        #BR
        if abs(BRPower) > maxPowerInput:
            maxPowerInput = abs(BRPower)
        
        #set scalar value, if loop can be removed later for fastest speeds but it broke last time I tried and will probably kill someone if it runs
        if maxPowerInput > 1:
            scalarPower = 1 / maxPowerInput
        else:
            scalarPower = 1

        """---------------------------------------------"""

        #PID Controller for  steering
        FLPID = PIDController(5.0, 0.0, 0.0)
        FRPID = PIDController(5.0, 0.0, 0.0)
        BLPID = PIDController(5.0, 0.0, 0.0)
        BRPID = PIDController(5.0, 0.0, 0.0)

        #calculate error
        FLFakeError = FLTarget - FLPosAngle
        FLTarget = 0
        FLPos = -(FLFakeError)
        FLSteeringError = FLTarget - FLPos
        if FLSteeringError > 180:
            FLSteeringError = FLSteeringError - 360
            
        FRFakeError = FRTarget - FRPosAngle
        FRTarget = 0
        FRPos = -(FRFakeError)
        FRSteeringError = FRTarget - FRPos
        if FRSteeringError > 180:
            FRSteeringError = FRSteeringError - 360

        BLFakeError = BLTarget -BLPosAngle
        BLTarget = 0
        BLPos = -(BLFakeError)
        BLSteeringError = BLTarget - BLPos
        if BLSteeringError > 180:
            BLSteeringError = BLSteeringError - 360

        BRFakeError = BRTarget - BRPosAngle
        BRTarget = 0
        BRPos = -(BRFakeError)
        BRSteeringError = BRTarget - BRPos
        if BRSteeringError > 180:
            BRSteeringError = BRSteeringError - 360
        
        """============================================="""

        #drive values (power)
        data[tags.FLDrive + tags.MOTOR_SPEED_CONTROL] = FLPower * scalarPower
        data[tags.FRDrive + tags.MOTOR_SPEED_CONTROL] = FRPower * scalarPower
        data[tags.BLDrive + tags.MOTOR_SPEED_CONTROL] = BLPower * scalarPower
        data[tags.BRDrive + tags.MOTOR_SPEED_CONTROL] = BRPower * scalarPower

        #target steering values (in rotaions)
        data[tags.FLSteering + tags.MOTOR_SPEED_CONTROL] = FLPID.tickErr(FLSteeringError / 360, data[tags.DT])

        data[tags.FRSteering + tags.MOTOR_SPEED_CONTROL] = FRPID.tickErr(FRSteeringError / 360, data[tags.DT])
        data[tags.BLSteering + tags.MOTOR_SPEED_CONTROL] = BLPID.tickErr(BLSteeringError / 360, data[tags.DT])
        data[tags.BRSteering + tags.MOTOR_SPEED_CONTROL] = BRPID.tickErr(BRSteeringError / 360, data[tags.DT])

        #data[tags.BLSteering + tags.MOTOR_SPEED_CONTROL] = FLTarget #test values
        #data[tags.BRSteering + tags.MOTOR_SPEED_CONTROL] = FLPosAngle
        #data[tags.FRSteering + tags.MOTOR_SPEED_CONTROL] = FLSteeringError
