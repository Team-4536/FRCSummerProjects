




def mechController(x: float, y: float, t: float) -> list[float]:

    speeds = [
        (y + x + t), # FL
        (y - x - t), # FR
        (y - x + t), # Bl
        (y + x - t), # BR
    ]

    max = abs(speeds[0])
    for i in range(0, 4):
        if (max < abs(speeds[i])):
            max = abs(speeds[i])

    if max > 1: return scaleSpeeds(speeds, 1/max)
    else: return speeds



def arcadeController(l: float, r: float) -> list[float]:
    return [ l, r, l, r ]

# NOTE: turning is CW+
def tankController(drive: float, turning: float) -> list[float]:
    speeds = [
        drive + turning,
        drive - turning,
        drive + turning,
        drive - turning
    ]

    max = abs(speeds[0])
    for i in range(0, 4):
        if (max < abs(speeds[i])):
            max = abs(speeds[i])

    if max > 1: return scaleSpeeds(speeds, 1/max)
    else: return speeds





def scaleSpeeds(speeds: list[float], scale) -> list[float]:
    l = []
    for x in speeds:
        l.append(x * scale)
    return l

def setMotors(speeds: list[float], fl, fr, bl, br) -> None:
    assert(len(speeds) == 4)
    fl.set(speeds[0])
    fr.set(speeds[1])
    bl.set(speeds[2])
    br.set(speeds[3])