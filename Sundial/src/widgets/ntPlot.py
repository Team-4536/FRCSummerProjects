import robotpy
import ntcore
import dearpygui.dearpygui as dpg
from widgets.widget import widget






class ntPlot(widget):
    table = ntcore.NetworkTableInstance.getDefault().getTable("telemetry")

    def __init__(self) -> None:

        self.tag: str = ""
        self.i = 0
        self.datax = []
        self.datay = []

        with dpg.window(label="NT Value plot"):

            # NT tag picker
            with dpg.group(horizontal=True):

                dpg.add_text("NT tag: ")

                def textCallback(s, d):
                    self.tag = d
                    self.resetPlotData()
                dpg.add_input_text(callback=textCallback)




            # plot
            with dpg.plot(no_title=True, no_mouse_pos=True, height=200) as plot:

                with dpg.theme() as item_theme:
                    with dpg.theme_component(dpg.mvAll):
                        dpg.add_theme_style(dpg.mvPlotStyleVar_PlotPadding, 0, 0, category=dpg.mvThemeCat_Plots)
                        dpg.add_theme_style(dpg.mvPlotStyleVar_PlotBorderSize, 1, 1, category=dpg.mvThemeCat_Plots)

                dpg.bind_item_theme(plot, item_theme) # type: ignore
                # REQUIRED: create x and y axes
                # dpg.add_plot_axis(dpg.mvXAxis, """, lock_min=True, lock_max=True, no_tick_labels=True""")
                # dpg.add_plot_axis(dpg.mvYAxis, tag="y_axis" """, no_gridlines=True, no_tick_labels=True, lock_max=True, lock_min=True""")
                dpg.add_plot_axis(dpg.mvXAxis)
                dpg.add_plot_axis(dpg.mvYAxis, tag="y_axis")

                # series belong to a y axis
                dpg.add_line_series(self.datax, self.datay, parent="y_axis", tag="series_tag")

        self.resetPlotData()





    def resetPlotData(self) -> None:

        self.datax = []
        self.datay = []

        for i in range(0, 1000):
            self.datax.append(i / 1000)
            self.datay.append(0.5)
            dpg.set_value('series_tag', [self.datax, self.datay])




    def tick(self) -> None:

        val = self.__class__.table.getValue(self.tag, None)
        if type(val) is float:
            x = self.datay[1:len(self.datay)]
            x.append(val)
            self.datay = x

        dpg.set_value('series_tag', [self.datax, self.datay])