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
        inputX = input.drive[0]
        inputY = input.drive[1]
        inputZ = input.turning

        inputGyro = 0 #constant for testing

        #assign inputs to vectors
        leftStick = V2(inputX, inputY)
        FLTurningVector = V2(math.cos(45) * inputZ, math.cos(45) * inputZ)
        FRTurningVector = V2(math.cos(45) * inputZ, -math.cos(45) * inputZ)
        BLTurningVector = V2(-math.cos(45) * inputZ, -math.cos(45) * inputZ)
        BRTurningVector = V2(-math.cos(45) * inputZ, math.cos(45) * inputZ)

        #field oriented offset (change later from gyro to encoder readings)
        leftStick = leftStick.rotateDegrees(-inputGyro)

        FLVector = leftStick.avg(FLTurningVector)
        FRVector = leftStick.avg(FRTurningVector)
        BLVector = leftStick.avg(BLTurningVector)
        BRVector = leftStick.avg(BRTurningVector)

        """-----------------------------------------"""

        #Encoder positions conversion to angle and first wrap (0 to 359)
        FLPos = abs(data[tags.FLSteering + tags.ENCODER_READING] * 360)
        if FLPos >= 360:
            FLPos = 360 - FLPos

        FRPos = abs(data[tags.FRSteering + tags.ENCODER_READING] * 360)
        if FRPos >= 360:
            FRPos = 360 - FRPos

        BLPos = abs(data[tags.BLSteering + tags.ENCODER_READING] * 360)
        if BLPos >= 360:
            BLPos = 360 - BLPos

        BRPos = abs(data[tags.BRSteering + tags.ENCODER_READING] * 360)
        if BRPos >= 360:
            BRPos = 360 - BRPos

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

        """------------------------------------"""

        #find quickest path and change values to it
        if (FLTarget - FLPos) > 180:
            FLTarget = FLTarget + 180
            FLPower = FLPower * -1

        if (FRTarget - FRPos) > 180:
            FRTarget = FRTarget + 180
            FRPower = FRPower * -1

        if (BLTarget - BLPos) > 180:
            BLTarget = BLTarget + 180
            BLPower = BLPower * -1

        if (BRTarget - BRPos) > 180:
            BRTarget = BRTarget + 180
            BRPower = BRPower * -1

        """-------------------------------"""

        #angle wrap for targets (0 to 359)
        #FL
        if FLTarget >= 360:
            FLTarget = FLTarget - 360
        
        #FR
        if FRTarget >= 360:
            FRTarget = FRTarget - 360
        
        #BL
        if BLTarget >= 360:
            BLTarget = BLTarget - 360
        
        #BR
        if BRTarget >= 360:
            BRTarget = BRTarget - 360

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
            scalar = 1 / maxPowerInput
        else:
            scalar = 1

        """---------------------------------------------"""

        #drive values (power)
        data[tags.FLDrive + tags.MOTOR_SPEED_CONTROL] = FLPower * scalar
        data[tags.FRDrive + tags.MOTOR_SPEED_CONTROL] = FRPower * scalar
        data[tags.BLDrive + tags.MOTOR_SPEED_CONTROL] = BLPower * scalar
        data[tags.BRDrive + tags.MOTOR_SPEED_CONTROL] = BRPower * scalar

        #target steering values (in rotaions)
        data[tags.FLSteering + tags.MOTOR_SPEED_CONTROL] = inputZ #FLTarget / 360 # "/ 360" converts back to rotations
        data[tags.FRSteering + tags.MOTOR_SPEED_CONTROL] = inputZ #FRTarget / 360
        data[tags.BLSteering + tags.MOTOR_SPEED_CONTROL] = inputZ #BLTarget / 360
        data[tags.BRSteering + tags.MOTOR_SPEED_CONTROL] = inputZ #BRTarget / 360
        

