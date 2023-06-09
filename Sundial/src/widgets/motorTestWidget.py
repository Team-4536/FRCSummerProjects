from math import cos
import dearpygui.dearpygui as dpg
import widgets.widget
from utils.tables import telemTable, sundialTable



# NOTE: This whole ass class is very outdated and dumb. Please do not use until someone fixes it to be better
# - 5/6/23
class MotorTest(widgets.widget.widget):


    __count = 0
    parent = 0

    def __init__(self):



        self.__datax = []
        self.__datay = []
        for i in range(0, 1000):
            self.__datax.append(i / 1000)
            self.__datay.append(0.5)




        if self.__class__.__count == 0:
            with dpg.window(label="Motor Testers") as window:
                self.__class__.parent = window
                def x():
                    m = MotorTest()

                dpg.add_button(label="Create new", callback=x)




        with dpg.value_registry():
            self.valTag = dpg.add_double_value(default_value=0)
            self.NTRecTag = dpg.add_string_value(default_value="")
            self.NTSendTag = dpg.add_string_value(default_value="motor" + str(self.__class__.__count))




        with dpg.group(horizontal=True, parent=self.parent) as group:
            self.groupTag: int|str = group # type: ignore

            with dpg.plot(no_title=True, no_mouse_pos=True, height=100) as plot:

                with dpg.theme() as item_theme:
                    with dpg.theme_component(dpg.mvAll):
                        dpg.add_theme_style(dpg.mvPlotStyleVar_PlotPadding, 0, 0, category=dpg.mvThemeCat_Plots)
                        dpg.add_theme_style(dpg.mvPlotStyleVar_PlotBorderSize, 1, 1, category=dpg.mvThemeCat_Plots)

                dpg.bind_item_theme(plot, item_theme) # type: ignore
                # REQUIRED: create x and y axes
                dpg.add_plot_axis(dpg.mvXAxis, lock_min=True, lock_max=True, no_tick_labels=True)
                self.yAxisTag = dpg.add_plot_axis(dpg.mvYAxis, no_gridlines=True, no_tick_labels=True, lock_max=True, lock_min=True)

                # series belong to a y axis
                self.seriesTag = dpg.add_line_series(self.__datax, self.__datay, parent=self.yAxisTag)




            def setVal(sender, data):
                dpg.set_value(self.valTag, data)
            dpg.add_slider_double(min_value=-1, max_value=1, callback=setVal, source=self.valTag, vertical=True, width=40)




            def stopMtr(sender, data):
                dpg.set_value(self.valTag, 0.0)
            dpg.add_button(label="Stop motor", callback=stopMtr, height=100, width=100)


            # """
            with dpg.group():

                with dpg.group(horizontal=True):
                    dpg.add_text("Send tag")
                    dpg.add_input_text(source=self.NTSendTag, width=100)

                with dpg.group(horizontal=True):
                    dpg.add_text("Recieve tag")
                    dpg.add_input_text(source=self.NTRecTag, width=100)


                """
                motors = [
                    "Spark",
                    "SparkMax",
                    "TalonSRX"
                ]

                with dpg.group(horizontal=True):
                    dpg.add_text("Type: ")
                    dpg.add_combo(motors, width=100)


                def deleteSelf(s, d) -> None:
                    from ..main import widgets
                    widgets.remove(self)
                    dpg.delete_item(self.groupTag)
                dpg.add_button(label="End proc", callback=deleteSelf)
                """
            # """




        self.__class__.__count += 1







    def tick(self):

        recieved = telemTable.getValue(dpg.get_value(self.NTRecTag), None)
        val: float = 0
        if type(recieved) is float: val = recieved


        sundialTable.putNumber(dpg.get_value(self.NTSendTag), dpg.get_value(self.valTag))

        x = self.__datay[1:len(self.__datay)]
        x.append(val*0.9/2+0.5)
        self.__datay = x


        dpg.set_value(self.seriesTag, [self.__datax, self.__datay])

