
import dearpygui.dearpygui as dpg
import ui
from widgets import driveWidget, client
from tests import tests
import os




def runSundial():


    m = tests.MotorTest()

    w = [
        driveWidget.create,
        client.create,
        m.sundialInit
        ]

    ui.createWindow(w)




    def loop():
        driveWidget.tick()
        client.tick()
        m.sundialTick()


    dpg.show_style_editor()
    ui.startWindow(loop)

if __name__ == "__main__":
    runSundial()