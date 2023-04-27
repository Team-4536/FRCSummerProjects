
from node import Node

import telemetryNode
import timeNode
import motorTestNode
import encoderSimNode
from hardware.DCMotors import *
from hardware.Encoders import *
import hardware.Input
import mechController
import hardware.pneumatics as pneumatics

def makeSimMechDrive(nodes: list[Node]):

    nodes.append(timeNode.TimeNode())

    nodes.append(telemetryNode.TelemNode([
        tags.FL + tags.ENCODER_READING,
        tags.FR + tags.ENCODER_READING,
        tags.BL + tags.ENCODER_READING,
        tags.BR + tags.ENCODER_READING,

        tags.FL + tags.MOTOR_SPEED_CONTROL,
        tags.FR + tags.MOTOR_SPEED_CONTROL,
        tags.BL + tags.MOTOR_SPEED_CONTROL,
        tags.BR + tags.MOTOR_SPEED_CONTROL,

        tags.GRABBER + tags.DBLSOLENOID_STATE
    ]))




    prefixes = [ tags.FL, tags.FR, tags.BL, tags.BR ]
    motors = []
    encoders = []

    nodes.append(mechController.MechProf())
    nodes.append(pneumatics.PneumaticsNode(tags.GRABBER, wpilib.PneumaticsControlModule(0).makeDoubleSolenoid(0, 1)))

    i = 0
    for pref in prefixes:

        motors.append(DCMotorNode(pref, NEOSpec, VirtualController()))
        nodes.append(motors[-1])

        encoders.append(VirtualEncoderNode(pref))
        nodes.append(encoders[-1])

        # nodes.append(motorTestNode.MotorTestNode("motor"+str(i), pref))
        nodes.append(encoderSimNode.EncoderSimNode(pref, motors[i], encoders[i]))
        i += 1

    nodes.append(hardware.Input.FlymerInputNode())


