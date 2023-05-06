from typing import Any, TextIO
import utils.tags as tags
from utils.tables import telemTable

loggedTags = [
    tags.FL + tags.MOTOR_SPEED_CONTROL,
    tags.FR + tags.MOTOR_SPEED_CONTROL,
    tags.BL + tags.MOTOR_SPEED_CONTROL,
    tags.BR + tags.MOTOR_SPEED_CONTROL,
    tags.FL + tags.ENCODER_READING,
    tags.FR + tags.ENCODER_READING,
    tags.BL + tags.ENCODER_READING,
    tags.BR + tags.ENCODER_READING,
    tags.MSG,
    tags.LOG_TAG,
    tags.RES,
    tags.TIME_SINCE_INIT,
    tags.GYRO_PITCH,
    tags.GYRO_YAW,
    tags.GYRO_ROLL
]



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







