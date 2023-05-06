from widgets.widget import widget
from typing import Any
import ntcore
import dearpygui.dearpygui as dpg
import time
import utils.tags as tags

from utils.tables import telemTable

class cmdWidget(widget):

    def __init__(self) -> None:

        self.expectedStamp:str = ""


        with dpg.value_registry():
            self.msgTag = dpg.add_string_value(default_value="")
            self.resTag = dpg.add_string_value(default_value="")



        with dpg.window(label="Cmd"):

            def buttonCallback(s, d):
                msg = dpg.get_value(self.msgTag)
                self.expectedStamp = str(time.time())
                telemTable.putString(tags.MSG, f"{self.expectedStamp}:{msg}")

                dpg.set_value(self.msgTag, "")
            buttonTag = dpg.add_button(label="Send!", callback=buttonCallback)



            with dpg.group(horizontal=True, before=buttonTag):
                dpg.add_text("Message: ")
                dpg.add_input_text(source=self.msgTag)

            with dpg.group(horizontal=True, before=buttonTag):
                dpg.add_text("Response: ")
                dpg.add_text(source=self.resTag, wrap=400)


    def tick(self) -> None:

        ntVal = telemTable.getValue(tags.RES, "")
        assert(type(ntVal) is str)

        split = ntVal.split(':', 1)
        if len(split) != 2: return

        if split[0] == self.expectedStamp:
            dpg.set_value(self.resTag, split[1])
            self.expectedStamp = split[0]

        print(self.expectedStamp, end="\n")
        print(split[0], "\n\n")






