from node import *
import wpilib
import math
import utils.tags as tags
from hardware.Input import FlymerInputProfile
from utils.V2 import V2
from typing import Any


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
        #leftStick = leftStick.rotateDegrees(-inputGyro)

        FLVector = leftStick.avg(FLTurningVector)
        FRVector = leftStick.avg(FRTurningVector)
        BLVector = leftStick.avg(BLTurningVector)
        BRVector = leftStick.avg(BRTurningVector)

        #scalar
        #find highest value
        if FLVector.getLength() > FRVector.getLength():
            maxPowerInput = FLVector.getLength()
        elif FRVector.getLength() > BLVector.getLength():
            maxPowerInput = FRVector.getLength()
        elif BLVector.getLength() > BRVector.getLength():
            maxPowerInput = BRVector.getLength()
        else:
            maxPowerInput = BRVector.getLength()
        
        #set scalar value
        if maxPowerInput > 1:
            scalar = 1 / maxPowerInput
        else:
            scalar = 1
        

        #completed wheel value assignement
        FLScaled = FLVector.multiplyBy(scalar)
        FRScaled = FRVector.multiplyBy(scalar)
        BLScaled = BLVector.multiplyBy(scalar)
        BRScaled = BRVector.multiplyBy(scalar)

        
        data[tags.FLDrive + tags.MOTOR_SPEED_CONTROL] = inputY #wheel output is still always positive for some reason
        data[tags.FRDrive + tags.MOTOR_SPEED_CONTROL] = inputY
        data[tags.BLDrive + tags.MOTOR_SPEED_CONTROL] = inputY
        data[tags.BRDrive + tags.MOTOR_SPEED_CONTROL] = inputY


        data[tags.FLSteering] = 0 #will be an angle once pid is created, for now just nothing
        data[tags.FRSteering] = 0
        data[tags.BLSteering] = 0
        data[tags.BRSteering] = 0













