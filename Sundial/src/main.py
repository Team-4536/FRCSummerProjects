
import dearpygui.dearpygui as dpg
import ui
from widgets import driveWidget
from widgets import motorTestWidget
from widgets import ntPlot
from widgets import client
from widgets import cmdWidget


import os
from widgets.widget import widget



if __name__ == "__main__":


    ui.createWindow()

    widgets: list[widget] = [ ]

    widgets.append(driveWidget.driveWidget())
    widgets.append(client.clientWidget())
    widgets.append(ntPlot.ntPlot())
    widgets.append(motorTestWidget.MotorTest())
    widgets.append(motorTestWidget.MotorTest())
    widgets.append(cmdWidget.cmdWidget())





    # l = dearpygui_ext.logger.mvLogger()
    # for i in range(0, 100):
    #     l.log_info("Hello world!")

    # dpg.show_style_editor()

    def loop():
        for w in widgets: w.tick()
    ui.startWindow(loop)
