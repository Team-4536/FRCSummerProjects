
from node import Node

import navx
import telemetryNode
import timeNode
import motorTestNode
import simulationNodes.encoderSimNode as encoderSimNode
from hardware.DCMotors import *
from hardware.Encoders import *
import hardware.Input
import mechController
import hardware.pneumatics as pneumatics
import hardware.gyros as gyros

def makeMechDrive(nodes: list[Node]):

    nodes.append(timeNode.TimeNode())

    nodes.append(telemetryNode.TelemNode([
        tags.FLDrive + tags.ENCODER_READING,
        tags.FRDrive + tags.ENCODER_READING,
        tags.BLDrive + tags.ENCODER_READING,
        tags.BRDrive + tags.ENCODER_READING,

        tags.FLDrive + tags.MOTOR_SPEED_CONTROL,
        tags.FRDrive + tags.MOTOR_SPEED_CONTROL,
        tags.BLDrive + tags.MOTOR_SPEED_CONTROL,
        tags.BRDrive + tags.MOTOR_SPEED_CONTROL,

        tags.GYRO_PITCH,
        tags.GYRO_YAW,
        tags.GYRO_ROLL,

        tags.TIME_SINCE_INIT,
        tags.FRAME_TIME
    ]))


    nodes.append(gyros.GyroNode(gyros.VirtualGyro()))
    nodes.append(hardware.Input.FlymerInputNode())
    # SWERVE CTRLR
    nodes.append(mechController.MechProf())



    drivePrefs = [ tags.FLDrive, tags.FRDrive, tags.BLDrive, tags.BRDrive ]
    driveMotors = []
    driveEncoders = []

  
    for i in range(0, 4):

        driveMotors.append(DCMotorNode(drivePrefs[i], NEOSpec, VirtualController()))
        nodes.append(driveMotors[-1])

        driveEncoders.append(VirtualEncoderNode(drivePrefs[i]))
        nodes.append(driveEncoders[-1])

        nodes.append(encoderSimNode.EncoderSimNode(drivePrefs[i], driveMotors[i], driveEncoders[i]))



    

