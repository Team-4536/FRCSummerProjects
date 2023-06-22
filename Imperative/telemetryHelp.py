import ntcore
import wpilib



def publishExpression(exp: str, root: str, table: ntcore.NetworkTable) -> None:

    try:
        val = getExp(root, exp.split('.'))
    except Exception as e:
        val = None

    # NOTE: none of this works lol


def getExp(root, propList):
    if len(propList) == 1:
        return getattr(root, propList[0])
    else:
        return getExp(getattr(root, propList[0]), propList[1:])


