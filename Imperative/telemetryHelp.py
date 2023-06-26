import ntcore
import wpilib



def publishExpression(exp: str, root: object, table: ntcore.NetworkTable) -> None:

    isFunc = exp.endswith("()")
    exp = exp.removesuffix("()")
    # TODO: array indexing

    try:
        val = getExp(root, exp.split('.'))
    except Exception as e:
        print(f"exception while publishing exp \'{exp}\': {repr(e)}")
        return None


    try:
        if isFunc: table.putValue("expr", val())
        else: table.putValue("expr", val)
    except Exception as e:
        print(f"{type(val)}")
        print(f"Exception while publishing value of expression \'{exp}\': {repr(e)}")


def getExp(root, propList):
    if len(propList) == 1:
        return getattr(root, propList[0])
    else:
        return getExp(getattr(root, propList[0]), propList[1:])


