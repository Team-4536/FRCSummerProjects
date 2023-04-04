
import dearpygui.dearpygui as dpg
import ui
from widgets import driveWidget
from widgets import motorTestWidget
from widgets import ntPlot
from widgets import client


import os
from widgets.widget import widget



if __name__ == "__main__":


    ui.createWindow()

    widgets: list[widget] = [ ]

    widgets.append(driveWidget.driveWidget())
    widgets.append(client.clientWidget())
    widgets.append(ntPlot.ntPlot())
    widgets.append(motorTestWidget.MotorTest())




    def loop():

        for w in widgets:
            w.tick()





    dpg.show_style_editor()
    ui.startWindow(loop)


