


prefStack = [ ]

def pushPref(str):
    prefStack.append(str)

def popPref():
    prefStack.pop()


def pref(str):
    x = str
    for p in prefStack:
        x += "/" + p
    return x