import DCMotors
import inspect




class bcolors:
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'



def testMotors():

    for x in DCMotors.__dict__:
        val = DCMotors.__dict__[x]
        if isinstance(val, type):
            if issubclass(val, DCMotors.DCMotor) and val is not DCMotors.DCMotor:
                if val.__name__.find("Templ") != len(val.__name__) - 5:
                    testMotorClass(val)




def testMotorClass(t: type[DCMotors.DCMotor]):


    try:
        x = t(0)
        assert(x is not None)

        x.setRaw(1)
        assert(1 == x.getRaw())

        x.setRaw(4)
        assert(x.getRaw() == 1)

        x.setRaw(-4)
        assert(x.getRaw() == -1)

        x.setRPM(x.__class__.maxRPM / 2)
        assert(x.getRPM() == x.__class__.maxRPM / 2)

    except:
        print(f"{bcolors.FAIL}Motor {t.__name__} failed{bcolors.ENDC}")
    else:
        print(f"{bcolors.OKGREEN}Motor {t.__name__} passed{bcolors.ENDC}")
    # """

if __name__ == "__main__":
    testMotors()