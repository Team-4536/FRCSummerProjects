from widgets.widget import widget
import ntcore
import dearpygui.dearpygui as dpg
import time

from utils.tables import cmdTable

class cmdWidget(widget):

    def __init__(self) -> None:




        with dpg.value_registry():
            self.msgTag = dpg.add_string_value(default_value="")
            self.resTag = dpg.add_string_value(default_value="")


        with dpg.window(label="Cmd"):



            def buttonCallback(s, d):
                msg = dpg.get_value(self.msgTag)
                cmdTable.putString("msg", f"{time.time()}:{msg}")

                dpg.set_value(self.msgTag, "")
            buttonTag = dpg.add_button(label="Send!", callback=buttonCallback)





            with dpg.group(horizontal=True, before=buttonTag):
                dpg.add_text("Message: ")

                dpg.add_input_text(source=self.msgTag)


            with dpg.group(horizontal=True, before=buttonTag):
                dpg.add_text("Response: ")
                dpg.add_text(source=self.resTag, wrap=400)


    def tick(self) -> None:

        ntVal = cmdTable.getString("res", None)
        if type(ntVal) is not str: return

        split = ntVal.split(':', 1)
        if len(split) != 2: return

        dpg.set_value(self.resTag, split[1])






