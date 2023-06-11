import utils.tags as tags
import ntcore
from os.path import basename
import dearpygui.dearpygui as dpg
import widgets.widget


inst = ntcore.NetworkTableInstance.getDefault()
inst.startClient4(basename(__file__))

# CLEANUP: figure something out to set this from within sundial, and keep it in a load file
# inst.setServer("10.45.36.2")
inst.setServer("localhost")
from utils.tables import telemTable



class clientWidget(widgets.widget.widget):

    def __init__(self) -> None:

        self.prevHeart: float = 0.0

        with dpg.value_registry():
            self.valueTag = dpg.add_string_value(default_value="")


        with dpg.window(label="Client window"):

            with dpg.group(horizontal=True):

                dpg.add_text("heartbeat: ")

                with dpg.drawlist(width=30, height=13):
                    dpg.draw_rectangle([-100, -100], [100, 100], fill=[255, 0, 0])

                    with dpg.draw_node() as node:
                        self.heartbeatNode: int|str = node # type: ignore
                        dpg.draw_rectangle([-100, -100], [100, 100], fill=[0, 255, 0])

            with dpg.group(horizontal=True):
                dpg.add_text("Opmode: ")
                dpg.add_text(source=self.valueTag)





    def tick(self) -> None:


        # scale green box by x
        # 0 if no conneciton, 1 if yes conneciton
        x = 1 if inst.isConnected() else 0
        dpg.apply_transform(self.heartbeatNode, dpg.create_scale_matrix([x, x]))




        val = telemTable.getValue(tags.OPMODE, None)
        print(type(val))
        if type(val) is not float: val = "Disconnected"
        else:
            dict = { 0.0 : "Disabled" , 1.0 : "Auto", 2.0 : "Teleop" }
            val = dict[val]
        dpg.set_value(self.valueTag, str(val))