
import traceback
import dearpygui.dearpygui as dpg
from utils import tables
from typing import Any
from widgets import driveWidget
from widgets import motorTestWidget
from widgets import ntPlot
from widgets import client
from widgets import cmdWidget
from widgets import botLog
import fileLog


import os
from widgets.widget import widget



if __name__ == "__main__":

    # CLEANUP: is this the best way to do it?
    resDirectory = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', "res"))
    resDirectory += "/"
    dpg.create_context()



    with dpg.font_registry():
        default_font = dpg.add_font(resDirectory + "consolab.ttf", 13)
    dpg.bind_font(default_font)







    dpg.create_viewport(title='Sundial', width=600, height=600)
    dpg.configure_app(docking=True, docking_space=True)
    dpg.configure_app(init_file=resDirectory+"dpg.ini")
    dpg.setup_dearpygui()



    widgets: list[widget] = [ ]

    widgets.append(driveWidget.driveWidget())
    widgets.append(client.clientWidget())
    widgets.append(ntPlot.ntPlot())
    widgets.append(motorTestWidget.MotorTest())
    widgets.append(motorTestWidget.MotorTest())
    widgets.append(cmdWidget.cmdWidget())
    widgets.append(botLog.botLogWidget())




    def save_init():
        dpg.save_init_file(resDirectory+"dpg.ini")

    with dpg.viewport_menu_bar():
        with dpg.menu(label="Add widgets"):
            dpg.add_menu_item(label="NT Plot", callback=lambda s,d: widgets.append(ntPlot.ntPlot()))
        dpg.add_button(label="save layout", callback=save_init)



    """

    import dearpygui_ext.themes as themes
    LIGHT_IMGUI_THEME = themes.create_theme_imgui_light()
    dpg.bind_theme(LIGHT_IMGUI_THEME)
    """

    l = fileLog.logger(resDirectory)

    dpg.show_viewport()
    while dpg.is_dearpygui_running():

        for x in widgets:
            x.tick()

        items:dict[str, Any] = { }

        for k in tables.telemTable.getKeys():
            items.update({ k : tables.telemTable.getValue(k, None) })

        try: # CLEANUP: get rid of this trycatch
            l.writeFrame(items)
        except Exception as e:
            print(traceback.format_exc(chain=False))
        dpg.render_dearpygui_frame()

    l.close()

    dpg.destroy_context()

