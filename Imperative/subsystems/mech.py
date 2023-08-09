
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

    if max > 1: return [s / max for s in speeds]
    else: return speeds
