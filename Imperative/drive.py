
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

    if max > 1: return [s / max for s in speeds]
    else: return speeds
