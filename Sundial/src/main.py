
import dearpygui.dearpygui as dpg
import ui
from widgets import driveWidget, client, motorTestWidget
import os




def runSundial():


    m = motorTestWidget.MotorTest()

    w = [
        driveWidget.create,
        client.create,
        m.init
        ]

    ui.createWindow(w)




    def loop():
        driveWidget.tick()
        client.tick()
        m.tick()


    dpg.show_style_editor()
    ui.startWindow(loop)

if __name__ == "__main__":
    runSundial()