
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



def addNode(nodes: list[Node], node):
    nodes.append(node)
    return node




def makeBasics(nodes: list[Node]):

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







def makeVirtualFlymer(nodes: list[Node]):
    makeBasics(nodes)

    nodes.append(hardware.Input.FlymerInputNode())
    nodes.append(mechController.MechProf())


    nodes.append(gyros.GyroNode(gyros.VirtualGyro()))

    liftMotor = addNode(nodes, DCMotorNode(tags.LIFT_MOTOR, NEOSpec, VirtualController()))
    liftEncoder = addNode(nodes, EncoderNode(tags.LIFT_MOTOR, VirtualEncoder()))


    drivePrefs = [ tags.FLDrive, tags.FRDrive, tags.BLDrive, tags.BRDrive ]
    for i in range(0, 4):

        motor = addNode(nodes, DCMotorNode(drivePrefs[i], NEOSpec, VirtualController()))
        encoder = addNode(nodes, EncoderNode(drivePrefs[i], VirtualEncoder()))
        nodes.append(encoderSimNode.EncoderSimNode(drivePrefs[i], motor, encoder))








def makeRealFlymer(nodes: list[Node]):
    makeBasics(nodes)

    nodes.append(hardware.Input.FlymerInputNode())
    nodes.append(mechController.MechProf())


    nodes.append(gyros.GyroNode(navx.AHRS(wpilib.SPI.Port.kMXP)))


    drivePrefs = [ tags.FLDrive, tags.FRDrive, tags.BLDrive, tags.BRDrive ]
    drivePorts = [ 4, 1, 3, 2 ]
    driveFlips = [ False, True, False, True]
    for i in range(0, 4):

        spark = rev.CANSparkMax(drivePorts[i], rev.CANSparkMax.MotorType.kBrushless)
        spark.setInverted(driveFlips[i])

        motor = addNode(nodes, DCMotorNode(drivePrefs[i], NEOSpec, spark))
        encoder = addNode(nodes, EncoderNode(drivePrefs[i], motor.controller.getEncoder()))

