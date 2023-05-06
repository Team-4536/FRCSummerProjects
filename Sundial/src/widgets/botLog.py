from widgets.widget import widget
from typing import Any
import dearpygui_ext.logger
import dearpygui.dearpygui as dpg
from utils.tables import telemTable
import utils.tags as tags




# NOTE/CLEANUP: This does not work with rapidly sent messages because networktables is a bitch
class botLogWidget(widget):

    def __init__(self) -> None:
        self.lastTag: str = ""
        self.msgTopic = telemTable.getStringTopic(tags.LOG_TAG).subscribe("default")
        self.logger = dearpygui_ext.logger.mvLogger()

    def tick(self) -> None:

        # CLEANUP: doesn't work with data[] system
        for x in self.msgTopic.readQueue():
            split = x.value.split(":", 2)
            if (len(split) == 3):

                if(split[1] == "ERR"):
                    self.logger.log_error(split[2])
                if(split[1] == "MSG"):
                    self.logger.log_debug(split[2])









