import dearpygui.dearpygui as dpg
import dearpygui_ext.logger
import os

def createWindow():

    dpg.create_context()



    """
    import dearpygui_ext.themes as themes
    LIGHT_IMGUI_THEME = themes.create_theme_imgui_light()
    dpg.bind_theme(LIGHT_IMGUI_THEME)
    # """

    with dpg.font_registry():
        resDirectory = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', "res/consolab.ttf"))
        default_font = dpg.add_font(resDirectory, 13)
    dpg.bind_font(default_font)


    dpg.create_viewport(title='Sundial', width=600, height=600)

    with dpg.viewport_menu_bar():
        with dpg.menu(label="File"):
            dpg.add_menu_item(label="Save")
            dpg.add_menu_item(label="Save As")

            with dpg.menu(label="Settings"):
                dpg.add_menu_item(label="Setting 1")
                dpg.add_menu_item(label="Setting 2")

        dpg.add_menu_item(label="Help")

        with dpg.menu(label="Widget Items"):
            dpg.add_checkbox(label="Pick Me")
            dpg.add_button(label="Press Me")
            dpg.add_color_picker(label="Color Me")


    dpg.configure_app(docking=True, docking_space=True)

    l = dearpygui_ext.logger.mvLogger()
    for i in range(0, 100):
        l.log_info("Hello world!")

    dpg.setup_dearpygui()





def startWindow(loopFunc):

    dpg.show_viewport()

    while dpg.is_dearpygui_running():

        loopFunc()
        dpg.render_dearpygui_frame()

    dpg.destroy_context()