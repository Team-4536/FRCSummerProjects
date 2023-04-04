import time




profileStack: list[float] = [ ]

def pushProf(accTime:int = 0): # adds new prof, with a duration >= acctime, accTime is in nanos
    profileStack.append(time.time_ns()-accTime)

def popProf() -> float: # returns duration of last prof on stack
    if len(profileStack) == 0:
        return 0

    return time.time_ns() - profileStack.pop()

