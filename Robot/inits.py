
from node import Node

import telemetryNode
import timeNode
import motorTestNode
import motorSimNode
from hardware.DCMotors import *
from hardware.Encoders import *
import hardware.Input

def makeSimMechDrive(nodes: list[Node]):

    nodes.append(timeNode.TimeNode())

    nodes.append(telemetryNode.TelemNode([
        "FLDriveRPS",
        "FRDriveRPS",
        "BLDriveRPS",
        "BRDriveRPS",
        "FLEncoderPos",
        "FREncoderPos",
        "BLEncoderPos",
        "BREncoderPos",
    ]))




    prefixes = [ "FL", "FR", 'BL', "BR" ]
    motors = []
    encoders = []


    i = 0
    for pref in prefixes:

        motors.append(DCMotorNode(pref+"Drive", VirtualSpec, VirtualController()))
        nodes.append(motors[-1])

        encoders.append(VirtualEncoder(pref+"Encoder"))
        nodes.append(encoders[-1])

        nodes.append(motorTestNode.motorTestNode("motor"+str(i), pref+"Drive", nodes))
        i += 1


    for i in range(0, 1):
        nodes.append(motorSimNode.motorSimNode(motors[i], encoders[i]))

    nodes.append(hardware.Input.FlymerInputNode())


