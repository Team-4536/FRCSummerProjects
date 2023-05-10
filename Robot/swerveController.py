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

        #raw encoder positions
        FLPos = data[tags.FLSteering + tags.ENCODER_READING]
        FRPos = data[tags.FRSteering + tags.ENCODER_READING]
        BLPos = data[tags.BLSteering + tags.ENCODER_READING]
        BRPos = data[tags.BRSteering + tags.ENCODER_READING]


        #Encoder positions as angles and first wrap (0 to 359)
        FLPosAngle = (data[tags.FLSteering + tags.ENCODER_READING] % 1) * 360
        FRPosAngle = (data[tags.FRSteering + tags.ENCODER_READING] % 1) * 360
        BLPosAngle = (data[tags.BLSteering + tags.ENCODER_READING] % 1) * 360
        BRPosAngle = (data[tags.BRSteering + tags.ENCODER_READING] % 1) * 360

        if FLPosAngle < 0:
            FLPosAngle = FLPosAngle + 360

        """
        if FLPosAngle > 180:
            FLPosAngle = FLPosAngle - 360
        if FRPosAngle > 180:
            FRPosAngle = FRPosAngle - 360
        if BLPosAngle > 180:
            BLPosAngle = BLPosAngle - 360
        if BRPosAngle > 180:
            BRPosAngle = BRPosAngle - 360
        """
        """----------------------------------------------------"""

        #set values                  
        FLTarget = FLVector.getAngle()
        FRTarget = FRVector.getAngle()
        BLTarget = BLVector.getAngle()
        BRTarget = BRVector.getAngle()

        if FLTarget >= 360:
            FLTarget = FLTarget - 360
        if FRTarget > 180:
            FRTarget = FRTarget - 360
        if BLTarget > 180:
            BLTarget = BLTarget - 360
        if BRTarget > 180:
            BRTarget = BRTarget - 360

        FLPower = FLVector.getLength()
        FRPower = FRVector.getLength()
        BLPower = BLVector.getLength()
        BRPower = BRVector.getLength()

        """------------------------------------"""
        """
        #find quickest path and change values to it
        if (FLTarget - FLPosAngle) >= 180:
            FLTarget = FLTarget + 180
            FLPower = -FLPower

        if (FRTarget - FRPosAngle) >= 180:
            FRTarget = FRTarget + 180
            FRPower = -FRPower

        if (BLTarget - BLPosAngle) >= 180:
            BLTarget = BLTarget + 180
            BLPower = -BLPower

        if (BRTarget - BRPosAngle) >= 180:
            BRTarget = BRTarget + 180
            BRPower = -BRPower
        """
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

        #i spent an hour trying to figure out why the error is never set to a negative number EVER
        #its like 11:30pm now and i still don't know whats wrong
        #normally i would ask rob but he went to bed after doing school work so im on my own
        #everything was almost working, and now nothing works - i love it when that happens
        #i have been working on this since 6pm and it still doesnt work
        #i hope rob finishes his work soon so that i can get this working
        #i am making a diary here to distract my mind from the unsolvale problem
        #it took me like 7 tries to figure out how to spell problem - i should probably go to bed
        #i just want our stupid serve to work but ive ran through the sim in my head and did all the math this file does
        #and it got a different result
        #wtf
        #goodnight
        FLSteeringError = (360 - FLPosAngle) - FLTarget
        FRSteeringError = FRTarget - FRPosAngle
        BLSteeringError = BLTarget - BLPosAngle
        BRSteeringError = BRTarget - BRPosAngle
        
        if FLSteeringError > 180:
            FLSteeringError = 360 - FLSteeringError

        if FRSteeringError > 180:
            FRSteeringError = FRSteeringError - 360

        if BLSteeringError > 180:
            BLSteeringError = BLSteeringError - 360

        if BRSteeringError > 180:
            BRSteeringError = BRSteeringError - 360
        """
        #ignore wrap
        if (FLSteeringError) >= 180:
            FLTarget = FLTarget + 180
            FLPower = -FLPower

        if (FRSteeringError) >= 180:
            FRTarget = FRTarget + 180
            FRPower = -FRPower

        if (BLSteeringError) >= 180:
            BLTarget = BLTarget + 180
            BLPower = -BLPower

        if (BRSteeringError) >= 180:
            BRTarget = BRTarget + 180
            BRPower = -BRPower
        """
        """============================================="""

        #drive values (power)
        data[tags.FLDrive + tags.MOTOR_SPEED_CONTROL] = FLPower * scalarPower
        data[tags.FRDrive + tags.MOTOR_SPEED_CONTROL] = FRPower * scalarPower
        data[tags.BLDrive + tags.MOTOR_SPEED_CONTROL] = BLPower * scalarPower
        data[tags.BRDrive + tags.MOTOR_SPEED_CONTROL] = BRPower * scalarPower

        #target steering values (in rotaions)
        data[tags.FLSteering + tags.MOTOR_SPEED_CONTROL] = FLPID.tickErr(FLSteeringError / 360, data[tags.DT])

        data[tags.FRSteering + tags.MOTOR_SPEED_CONTROL] = FRPID.tick(FRSteeringError / 360, FRPosAngle / 360, data[tags.DT])
        data[tags.BLSteering + tags.MOTOR_SPEED_CONTROL] = BLPID.tick(BLSteeringError / 360, BLPosAngle / 360, data[tags.DT])
        data[tags.BRSteering + tags.MOTOR_SPEED_CONTROL] = BRPID.tick(BRSteeringError / 360, BRPosAngle / 360, data[tags.DT])

        data[tags.BLSteering + tags.MOTOR_SPEED_CONTROL] = FLTarget #test values
        data[tags.BRSteering + tags.MOTOR_SPEED_CONTROL] = FLPosAngle
        data[tags.FRSteering + tags.MOTOR_SPEED_CONTROL] = FLSteeringError
