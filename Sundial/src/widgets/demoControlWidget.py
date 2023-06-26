import dearpygui.dearpygui as dpg
import widgets.widget
import ntcore
from widgets.widget import widget
from utils.tables import telemTable

class DemoControlWidget(widget):

    def __init__(self) -> None:


        with dpg.window(label="Demo controls"):

            def checkCallback(sender, data):
                telemTable.putBoolean("shooter enabled", data)
            checkCallback(None, True)
            dpg.add_checkbox(label="Shooter enabled", default_value=True, callback=checkCallback)

            def slideCall(sender, data):
                telemTable.putNumber("shooter speed", data)
            checkCallback(None, 0.6)
            dpg.add_slider_float(label="Shooter speed", min_value=-0.7, max_value=0.7, default_value=0.6, callback=slideCall)



    def tick(self) -> None:
        pass

