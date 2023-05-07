
from node import Node

import navx
import telemetryNode
import timeNode
import motorTestNode
import simulationNodes.encoderSimNode as encoderSimNode
from hardware.DCMotors import *
from hardware.Encoders import *
import hardware.Input
import swerveController
import hardware.pneumatics as pneumatics
import hardware.gyros as gyros

def makeSwerveDrive(nodes: list[Node]):

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

        tags.FLSteering + tags.ENCODER_READING,
        tags.FRSteering + tags.ENCODER_READING,
        tags.BLSteering + tags.ENCODER_READING,
        tags.BRSteering + tags.ENCODER_READING,

        tags.FLSteering + tags.MOTOR_SPEED_CONTROL,
        tags.FRSteering + tags.MOTOR_SPEED_CONTROL,
        tags.BLSteering + tags.MOTOR_SPEED_CONTROL,
        tags.BRSteering + tags.MOTOR_SPEED_CONTROL,

        tags.GYRO_PITCH,
        tags.GYRO_YAW,
        tags.GYRO_ROLL,

        tags.TIME_SINCE_INIT,
        tags.FRAME_TIME
    ]))


    nodes.append(gyros.GyroNode(gyros.VirtualGyro()))
    nodes.append(hardware.Input.FlymerInputNode())
    # SWERVE CTRLR
    nodes.append(swerveController.SwerveProf())



    drivePrefs = [ tags.FLDrive, tags.FRDrive, tags.BLDrive, tags.BRDrive ]
    driveMotors = []
    driveEncoders = []

    steerPrefs = [ tags.FLSteering, tags.FRSteering, tags.BLSteering, tags.BRSteering ]
    steerMotors = []
    steerEncoders = []

    for i in range(0, 4):

        driveMotors.append(DCMotorNode(drivePrefs[i], NEOSpec, VirtualController()))
        nodes.append(driveMotors[-1])

        driveEncoders.append(VirtualEncoderNode(drivePrefs[i]))
        nodes.append(driveEncoders[-1])

        nodes.append(encoderSimNode.EncoderSimNode(drivePrefs[i], driveMotors[i], driveEncoders[i]))



        steerMotors.append(DCMotorNode(steerPrefs[i], NEOSpec, VirtualController()))
        nodes.append(steerMotors[-1])

        steerEncoders.append(VirtualEncoderNode(steerPrefs[i]))
        nodes.append(steerEncoders[-1])

        nodes.append(encoderSimNode.EncoderSimNode(steerPrefs[i], steerMotors[i], steerEncoders[i]))




