import DCMotors
import inspect
import rev





class bcolors:
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'


def testMotors():

    motorControllers = []
    specs = []

    for x in DCMotors.__dict__:
        val = DCMotors.__dict__[x]
        if isinstance(val, type):
            if issubclass(val, DCMotors.DCMotorController) and val is not DCMotors.DCMotorController:
                motorControllers.append(val)
            elif issubclass(val, DCMotors.DCMotorSpec) and val is not DCMotors.DCMotorSpec:
                specs.append(val)


    i = 0
    for c in motorControllers:
        for s in specs:
            controller = None

            if c is DCMotors.SparkMaxController:
                controller = c(rev.CANSparkMax(i, rev.CANSparkMax.MotorType.kBrushless))
            else:
                controller = c()

            motor = DCMotors.DCMotor("motor", s, controller)
            testMotorConfig(motor)
            i+=1



def testMotorConfig(x: DCMotors.DCMotor):

    try:
        assert(x is not None)

        x.setRaw(1)
        assert(1 == x.getRaw())

        x.setRaw(4)
        assert(x.getRaw() == 1)

        x.setRaw(-4)
        assert(x.getRaw() == -1)

        x.setRPS(x.spec.maxRPS / 2)
        assert(x.getRPS() == x.spec.maxRPS / 2)

    #"""
    except:
        print(f"{bcolors.FAIL}Config {x.controller.__class__.__name__}, {x.spec.__name__} failed{bcolors.ENDC}")
    else:
        print(f"{bcolors.OKGREEN}Config {x.controller.__class__.__name__}, {x.spec.__name__} passed{bcolors.ENDC}")
    #"""

if __name__ == "__main__":
    testMotors()