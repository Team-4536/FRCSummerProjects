from typing import Any





# CLEANUP: data is allegedly not in sync if being sent from multiple keys
class logger():


    # NOTE: path should include trailing slash
    def __init__(self, folderPath: str) -> None:
        self.file = open(folderPath + "log.txt", "w")


    def writeFrame(self, items: dict[str, Any]):
        for k, v in items.items():
            line: str = ""

            if type(v) is float: line += "f:"
            else: assert(False) # CLEANUP: These could cause big problems at competition

            if ':' in k: assert(False)
            else: line += k

            line += ":" + str(v)

            self.file.write(line + "\n")
        self.file.write("\n")

    def close(self):
        self.file.close()







