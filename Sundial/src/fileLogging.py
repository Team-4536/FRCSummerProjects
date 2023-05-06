from typing import Any, TextIO





# CLEANUP: data is allegedly not in sync if being sent from multiple keys

def writeFrame(file: TextIO, items: dict[str, Any]):

    if len(items) == 0: return


    for k, v in items.items():
        if v is None: continue


        line: str = ""

        # CLEANUP: Right now this only works with floats because that's kind of all the robot code can handle. Worth improving
        # CLEANUP: These could cause big problems at competition
        if type(v) is not float: assert(False)
        if ':' in k: assert(False)

        line += "f:"
        line += f"{k}:"
        line += str(v)

        file.write(line + "\n")
    file.write("\n")






