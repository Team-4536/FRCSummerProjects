from typing import Any, TextIO
import utils.tags as tags
from utils.tables import telemTable

loggedTags = [
    tags.FLDrive + tags.MOTOR_SPEED_CONTROL,
    tags.FRDrive + tags.MOTOR_SPEED_CONTROL,
    tags.BLDrive + tags.MOTOR_SPEED_CONTROL,
    tags.BRDrive + tags.MOTOR_SPEED_CONTROL,
    tags.FLDrive + tags.ENCODER_READING,
    tags.FRDrive + tags.ENCODER_READING,
    tags.BLDrive + tags.ENCODER_READING,
    tags.BRDrive + tags.ENCODER_READING,
    tags.FLSteering + tags.MOTOR_SPEED_CONTROL,
    tags.FRSteering + tags.MOTOR_SPEED_CONTROL,
    tags.BLSteering + tags.MOTOR_SPEED_CONTROL,
    tags.BRSteering + tags.MOTOR_SPEED_CONTROL,
    tags.FLSteering + tags.ENCODER_READING,
    tags.FRSteering + tags.ENCODER_READING,
    tags.BLSteering + tags.ENCODER_READING,
    tags.BRSteering + tags.ENCODER_READING,
    tags.TIME_SINCE_INIT,
    tags.GYRO_PITCH,
    tags.GYRO_YAW,
    tags.GYRO_ROLL
]
# NOTE: not logging msg, err, or response rn bc they are strs and would be bad to log



# CLEANUP: data is allegedly not in sync if being sent from multiple keys

# Collects info from NT and writes to the given file
def writeFrame(file: TextIO):


    items = {}
    for t in loggedTags:
        val = telemTable.getValue(t, None)
        if val is not None:
            items.update({ t : val })

    if len(items) == 0: return


    i = 0
    for k, v in items.items():
        if v is None: continue


        line: str = ""

        # CLEANUP: Right now this only works with floats because that's kind of all the robot code can handle. Worth improving
        # CLEANUP: These could cause big problems at competition
        if type(v) is not float: assert(False)
        if ':' in k: assert(False)

        line += "f:"
        line += f"{k}:"
        line += str(v)

        file.write(line + "\n")
        i += 1

    if i == 0: return
    file.write("\n")







