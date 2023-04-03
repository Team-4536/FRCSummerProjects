
import robotpy
import ntcore
from os.path import basename
import dearpygui.dearpygui as dpg
from enum import Enum

import Tags


inst = ntcore.NetworkTableInstance.getDefault()
inst.startClient4(basename(__file__))
# inst.setServer("10.45.36.2")
inst.setServer("localhost")
robotInfo = inst.getTable("RobotInfo")




def create():

    with dpg.value_registry():
        for v in Tags.__dict__:
            if v[0] != '_':
                val = Tags.__dict__[v]
                dpg.add_double_value(label=val, tag=val, default_value=0)


    with dpg.window(label="Client window", tag="Client window"):
        for v in Tags.__dict__:
            if v[0] != '_':
                val = Tags.__dict__[v]
                dpg.add_slider_double(source=val, label=f"{val}: ", min_value=-1, max_value=1)




def tick():

    for v in Tags.__dict__:
        if v[0] != '_':
            val = Tags.__dict__[v]
            ret = robotInfo.getValue(val, None)
            if ret is not None:
                dpg.set_value(val, ret)
    pass