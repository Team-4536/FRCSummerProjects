import wpilib



profileStack: list[float] = [ ]

def pushProf(accTime:int = 0): # adds new prof, with a duration >= acctime, accTime is in secs
    profileStack.append(wpilib.getTime()-accTime)

def popProf() -> float: # returns duration of last prof on stack
    if len(profileStack) == 0:
        return 0

    return wpilib.getTime() - profileStack.pop()

