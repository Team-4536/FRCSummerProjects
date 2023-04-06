import dearpygui.dearpygui as dpg
import dearpygui_ext.logger
import os

def createWindow():

    dpg.create_context()




    with dpg.font_registry():
        resDirectory = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', "res/consolab.ttf"))
        default_font = dpg.add_font(resDirectory, 13)
    dpg.bind_font(default_font)

    dpg.create_viewport(title='Sundial', width=600, height=600)
    dpg.configure_app(docking=True, docking_space=True)


    dpg.setup_dearpygui()





def startWindow(loopFunc):

    dpg.show_viewport()

    while dpg.is_dearpygui_running():

        loopFunc()
        dpg.render_dearpygui_frame()

    dpg.destroy_context()