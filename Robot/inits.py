
from node import Node

import navx
import telemetryNode
import simulationNodes.encoderSimNode as encoderSimNode
from hardware.DCMotors import *
from hardware.Encoders import *
import hardware.Input
import mechController
import hardware.pneumatics as pneumatics
import hardware.gyros as gyros
import motorTestNode
import tankController





"""
Creates and adds a motor node and encoder node. Adds virtual versions if isReal is false
"""
def sparkMaxAndEncoderPair(nodes: list[Node], isReal: bool, prefix: str, motorSpec: type[DCMotorSpec], motorBrushed: bool, motorPort: int, motorFlipped: bool):

    if isReal:
        t = rev.CANSparkMax.MotorType.kBrushed if motorBrushed else rev.CANSparkMax.MotorType.kBrushless
        ctrlr = rev.CANSparkMax(motorPort, t)
        ctrlr.setInverted(motorFlipped)
    else:
        ctrlr = VirtualController()

    motor = DCMotorNode(prefix, motorSpec, ctrlr).addToo(nodes)


    encoder = EncoderNode(prefix,
            motor.controller.getEncoder() if isReal else # type: ignore
            VirtualEncoder()
        ).addToo(nodes)

    return motor, encoder










def makeFlymer(nodes: list[Node], isReal: bool):

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
        tags.FRAME_TIME,
        tags.OPMODE
    ]))

    # -------------------------- DEFAULT PROFILE --------------------------------------------------

    hardware.Input.FlymerInputNode().addToo(nodes)
    mechController.MechProf().addToo(nodes)
    # motorTestNode.MotorTestNode(tags.FLDrive).addToo(nodes)


    # --------------------------- HARDWARE ---------------------------------------------------------

    gyros.GyroNode(
            navx.AHRS(wpilib.SPI.Port.kMXP) if isReal else
            gyros.VirtualGyro()
        ).addToo(nodes)


    # liftMotor, liftEncoder = sparkMaxAndEncoderPair(nodes, isReal, tags.LIFT_MOTOR, NEOSpec, False, 0, False)
    # if not isReal: encoderSimNode.EncoderSimNode(tags.LIFT_MOTOR, liftMotor, liftEncoder).addToo(nodes)

    
    drivePrefs = [ tags.FLDrive, tags.FRDrive, tags.BLDrive, tags.BRDrive ]
    driveFlips = [ False, False, False, False]
    drivePorts = [ 2, 18, 1, 0 ]
    for i in range(0, 4):

        motor, encoder = sparkMaxAndEncoderPair(nodes, isReal,
            prefix=drivePrefs[i],
            motorSpec=NEOSpec,
            motorBrushed=False,
            motorPort=drivePorts[i],
            motorFlipped=driveFlips[i])

        if not isReal: encoderSimNode.EncoderSimNode(drivePrefs[i], motor, encoder).addToo(nodes)
        


    #testMotor = DCMotorNode(tags.FLDrive, NEOSpec, rev.CANSparkMax(3, rev.CANSparkMax.MotorType.kBrushless)).addToo(nodes)
    #self.motor = rev.CANSparkMax(3, rev.CANSparkMax.MotorType.kBrushless)

    testMotor, testEncoder = sparkMaxAndEncoderPair(nodes, isReal, tags.FLDrive, NEOSpec, False, 3, False)
    # if not isReal: encoderSimNode.EncoderSimNode(tags.FLDrive, testMotor, testEncoder).addToo(nodes)
    # reportMsg(str(isReal))

def makeDemo(nodes: list[Node], isReal: bool):

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
        tags.FRAME_TIME,
        tags.OPMODE
    ]))

    # -------------------------- DEFAULT PROFILE --------------------------------------------------

    hardware.Input.DemoInputNode().addToo(nodes)
    tankController.TankProf().addToo(nodes)
    # motorTestNode.MotorTestNode(tags.FLDrive).addToo(nodes)


    # --------------------------- HARDWARE ---------------------------------------------------------

    gyros.GyroNode(
            navx.AHRS(wpilib.SPI.Port.kMXP) if isReal else
            gyros.VirtualGyro()
        ).addToo(nodes)


    
    drivePrefs = [ tags.FLDrive, tags.FRDrive, tags.BLDrive, tags.BRDrive ]
    driveFlips = [ False, False, False, False]
    drivePorts = [ 2, 3, 1, 4 ]
    for i in range(0, 4):

        motor = DCMotorNode(drive)
        motor, encoder = sparkMaxAndEncoderPair(nodes, isReal,
            prefix=drivePrefs[i],
            motorSpec=NEOSpec,
            motorBrushed=False,
            motorPort=drivePorts[i],
            motorFlipped=driveFlips[i])

        if not isReal: encoderSimNode.EncoderSimNode(drivePrefs[i], motor, encoder).addToo(nodes)
        