from widgets.widget import widget
import dearpygui_ext.logger
import dearpygui.dearpygui as dpg
import utils.tables as tables




# NOTE/CLEANUP: This does not work with rapidly sent messages because networktables is a bitch
class botLogWidget(widget):

    def __init__(self) -> None:
        self.lastTag: str = ""
        self.msgTopic = tables.cmdTable.getStringTopic("logs").subscribe("default")
        self.logger = dearpygui_ext.logger.mvLogger()

    def tick(self) -> None:

        for x in self.msgTopic.readQueue():
            split = x.value.split(":", 2)
            if (len(split) == 3):

                if(split[1] == "ERR"):
                    self.logger.log_error(split[2])
                if(split[1] == "MSG"):
                    self.logger.log_debug(split[2])









