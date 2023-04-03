from typing import Callable
import wpilib
import rev


def _throwUninplemented(*args) -> None:
    raise NotImplementedError()




class DCMotor:

    # inheriting motors should not override the functions in this class,
    # other than get, set, and ctor
    maxRPM: float = 0

    # get and set are expected to be -1 to 1 range
    setFunction: Callable[[any, float], None] = _throwUninplemented #type: ignore
    getFunction: Callable[[any], float] = _throwUninplemented # type: ignore
    ctor: Callable[[int], any] = _throwUninplemented # type: ignore



    def __init__(self, port: int) -> None:
        self.__motor = self.__class__.ctor(port)


    def getRaw(self) -> float:
        return self.__class__.getFunction(self.__motor)

    def setRaw(self, power: float):
        self.__class__.setFunction(self.__motor, power)



    def setRPM(self, rpm: float) -> None:
        self.__class__.setFunction(self.__motor, rpm / self.maxRPM)

    def getRPM(self) -> float:
        return self.__class__.getFunction(self.__motor) * self.maxRPM





class SparkMaxBrushedTempl(DCMotor):

    ctor = lambda p: rev.CANSparkMax(p, rev.CANSparkMax.MotorType.kBrushed)
    setFunction = lambda m, p: m.set(max(min(p, 1), -1))
    getFunction = lambda m: m.get()

class SparkMaxBrushlessTempl(SparkMaxBrushedTempl):
    ctor = lambda p: rev.CANSparkMax(p, rev.CANSparkMax.MotorType.kBrushless)




class SparkSpec(DCMotor):
    maxRPM = 1

    ctor = lambda p: wpilib.Spark(p)
    setFunction = lambda m, p: m.set(p)
    getFunction = lambda m: m.get()


class DriveSpec(SparkMaxBrushlessTempl):
    maxRPM = 40



