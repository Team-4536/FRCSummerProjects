

from typing import TypeAlias
from enum import Enum


Tag: TypeAlias = str

# tag prefixes denote a certain piece of hardware
# to avoid conflicts between nodes being used more than once

FL = "FL"
FR = "FR"
BL = "BL"
BR = "BR"

GRABBER = "Grab"



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



# any straight values that only happen once

DT = "dt"
INPUT = "input"




"""
Use tag prefixes and suffixes to keep your tags organized.

Prefixes are used to differentiate the outputs of multiple nodes doing the same thing.
Like simulations for each wheel, motors, encoders
Ex. Each encoder node would write to (Some prefix) + tags.ENCODER_READING
"""


