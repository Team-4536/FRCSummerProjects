
import ntcore
from os.path import basename
import dearpygui.dearpygui as dpg
import widgets.widget


inst = ntcore.NetworkTableInstance.getDefault()
inst.startClient4(basename(__file__))
# inst.setServer("10.45.36.2")
inst.setServer("localhost")
telemTable = inst.getTable("telemetry")




class clientWidget(widgets.widget.widget):

    def __init__(self) -> None:

        self.prevHeart: float = 0.0




        with dpg.window(label="Client window", tag="Client window"):

            with dpg.group(horizontal=True):

                dpg.add_text("comms: ")

                with dpg.drawlist(width=30, height=13):
                    dpg.draw_rectangle([-100, -100], [100, 100], fill=[255, 0, 0])

                    with dpg.draw_node() as node:
                        self.heartbeatNode: int|str = node # type: ignore
                        dpg.draw_rectangle([-100, -100], [100, 100], fill=[0, 255, 0])





    def tick(self) -> None:

        heart = telemTable.getValue("heartbeat", None)
        x = 0

        if type(heart) is float: # timeout detection
            if heart - self.prevHeart < 1.1: x = 1
            self.prevHeart = heart

        # scale green box by x
        # 0 if no conneciton, 1 if yes conneciton
        dpg.apply_transform(self.heartbeatNode, dpg.create_scale_matrix([x, x]))
