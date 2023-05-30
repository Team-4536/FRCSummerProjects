
import utils.tags as tags
import traceback
from typing import Any
from utils.tables import telemTable
import dearpygui.dearpygui as dpg
from widgets.visuals import swerveWidget
from widgets.visuals import mechWidget
from widgets import ntPlot
from widgets import client
from widgets import cmdWidget
from widgets import botLog
import fileLogging


import os
from widgets.widget import widget


class sundial:
    def __init__(self) -> None:
        # CLEANUP: is this the best way to do it?
        self.resDirectory = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', "res"))
        self.resDirectory += "/"
        dpg.create_context()

        # Load a font that looks ok
        with dpg.font_registry():
            default_font = dpg.add_font(self.resDirectory + "consolab.ttf", 13)
        dpg.bind_font(default_font)

        # Enable docking
        dpg.create_viewport(title='Sundial', width=600, height=600)
        dpg.configure_app(docking=True, docking_space=True)

        # Load the docking layout
        dpg.configure_app(init_file=self.resDirectory+"dpg.ini")

        dpg.setup_dearpygui()


        # menu bar initalization
        def save_init():
            dpg.save_init_file(self.resDirectory+"dpg.ini")

        with dpg.viewport_menu_bar():
            with dpg.menu(label="Add widgets"):
                dpg.add_menu_item(label="NT Plot", callback=lambda s,d: self.widgets.append(ntPlot.ntPlot()))
            dpg.add_button(label="save layout", callback=save_init)




        self.widgets: list[widget] = [

            mechWidget.mechWidget(),
            client.clientWidget(),
            ntPlot.ntPlot(),
            cmdWidget.cmdWidget(),
            botLog.botLogWidget()
        ]

        dpg.show_viewport()



    def run(self):

        logFile = open(self.resDirectory + "log.txt", "w")



        while dpg.is_dearpygui_running():

            for x in self.widgets:
                try:
                    x.tick()
                except Exception as e:
                    print(traceback.format_exc(chain=False)) # NOTE: Add this to the logger sometime lol


            try: # CLEANUP: get rid of this tr~ycatch
                # fileLogging.writeFrame(logFile)
                pass
            except Exception as e:
                print(traceback.format_exc(chain=False))

            dpg.render_dearpygui_frame()

        logFile.close()
        dpg.destroy_context()




if __name__ == "__main__":


    s = sundial()
    s.run()

    """ #NOTE: I tried to get this to work but docking kinda really broke it
    import dearpygui_ext.themes as themes
    LIGHT_IMGUI_THEME = themes.create_theme_imgui_light()
    dpg.bind_theme(LIGHT_IMGUI_THEME)
    """



