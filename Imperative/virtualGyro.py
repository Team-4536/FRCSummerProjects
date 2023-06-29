




class VirtualGyro:

    def __init__(self) -> None:
        self.pitch: float = 0.0
        self.yaw: float = 0.0
        self.roll: float = 0.0

    def getPitch(self) -> float: return self.pitch
    def getYaw(self) -> float: return self.yaw
    def getRoll(self) -> float: return self.roll

    def setPitch(self, n: float) -> None: self.pitch = n
    def setYaw(self, n: float) -> None: self.yaw = n
    def setRoll(self, n: float) -> None: self.roll = n

