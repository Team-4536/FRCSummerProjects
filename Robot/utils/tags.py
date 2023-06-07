

from typing import TypeAlias
from enum import Enum

# tag prefixes denote a certain piece of hardware
# to avoid conflicts between nodes being used more than once

FLDrive = "FLDrive"
FRDrive = "FRDrive"
BLDrive = "BLDrive"
BRDrive = "BRDrive"

FLSteering = "FLSteering"
FRSteering = "FRSteering"
BLSteering = "BLSteering"
BRSteering = "BRSteering"

GRABBER = "Grab"

LIFT_MOTOR = "Lift"


# Suffixes are for what each node is doing

MOTOR_SPEED_CONTROL = "Speed"
ENCODER_READING = "Pos"
DBLSOLENOID_STATE = "Extended"

ENCODER_SIM_VELOCITY = "Simvel"

ENCODER_NAME = "Encoder"
MOTOR_NAME = "Motor"
ENCODER_SIM_NAME = "Encodersim"
TESTER_NAME = "Tester"
DBLSOLENOID_NAME = "DBLSolenoid"

UP_LIMIT_SWITCH = "UpSwitch"
DOWN_LIMIT_SWITCH = "DownSwitch"
CLOCK_LIMIT_SWITCH = "ClockSwitch"
COUTNER_LIMIT_SWITCH = "CounterSwitch"

# any straight values that only happen once

DT = "dt"
TIME_SINCE_INIT = "time"
HEARTBEAT = "heartbeat"

GYRO_PITCH = "pitch"
GYRO_YAW = "yaw"
GYRO_ROLL = "roll"

FRAME_TIME = "frameTime"
INPUT = "input"


LOG_TAG = "logs"
MSG = "msg"
RES = 'res'







"""
Use tag prefixes and suffixes to keep your tags organized.

Prefixes are used to differentiate the outputs of multiple nodes doing the same thing.
Like simulations for each wheel, motors, encoders
Ex. Each encoder node would write to (Some prefix) + tags.ENCODER_READING
"""


