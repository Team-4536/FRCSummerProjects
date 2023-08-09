from real import normalizeWheelSpeeds

def mechController(x: float, y: float, t: float) -> list[float]:
    speeds = [
        (y + x + t), # FL
        (y - x - t), # FR
        (y - x + t), # Bl
        (y + x - t), # BR
    ]

    return normalizeWheelSpeeds(speeds)
