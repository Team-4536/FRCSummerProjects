import robotpy
import ntcore
import dearpygui.dearpygui as dpg
from widgets.widget import widget
from typing import Callable




class ntPlot(widget):
    table = ntcore.NetworkTableInstance.getDefault().getTable("telemetry")

    def __init__(self) -> None:

        self.tags: list[str] = []
        self.datax: list[list[float]] = [[]]
        self.datay: list[list[float]] = [[]]
        self.seriesTags: list[str|int] = []

        with dpg.window(label="NT Value plot") as window:
            self.windowTag: int|str = window # type: ignore


            # plot
            with dpg.plot(no_title=True, no_mouse_pos=True, height=200) as plot:

                with dpg.theme() as item_theme:
                    with dpg.theme_component(dpg.mvAll):
                        dpg.add_theme_style(dpg.mvPlotStyleVar_PlotPadding, 0, 0, category=dpg.mvThemeCat_Plots)
                        dpg.add_theme_style(dpg.mvPlotStyleVar_PlotBorderSize, 1, 1, category=dpg.mvThemeCat_Plots)

                dpg.bind_item_theme(plot, item_theme) # type: ignore

                dpg.add_plot_axis(dpg.mvXAxis, lock_min=True, lock_max=True, no_tick_labels=True)
                self.yAxisTag = dpg.add_plot_axis(dpg.mvYAxis)

                # self.seriesTag = dpg.add_line_series(self.datax[0], self.datay[0], parent=self.yAxisTag)



            def addDataPoint():

                idx: int = len(self.tags)
                self.datax.append([])
                self.datay.append([])
                self.tags.append("")

                # NT tag picker
                with dpg.group(horizontal=True, parent=self.windowTag):
                    dpg.add_text("NT tag: ")

                    def makeTextCall(i) -> Callable[[object, object], None]:
                        def textCallback(s, d):
                            self.tags[i] = d
                            dpg.set_item_label(self.seriesTags[i], d)
                            self.resetPlotData(idx)
                        return textCallback
                    dpg.add_input_text(callback=makeTextCall(idx))

                self.seriesTags.append(dpg.add_line_series(self.datax[len(self.datax)-1], self.datay[len(self.datay)-1], parent=self.yAxisTag, label="Hello"))



            dpg.add_button(label="Add data src", callback=addDataPoint)




    def resetPlotData(self, idx: int) -> None:

        self.datax[idx] = []
        self.datay[idx] = []

        for i in range(0, 1000):
            self.datax[idx].append(i / 1000)
            self.datay[idx].append(0.5)
            dpg.set_value(self.seriesTags[idx], [self.datax[idx], self.datay[idx]])




    def tick(self) -> None:

        for i in range(len(self.tags)):

            val = self.__class__.table.getValue(self.tags[i], None)
            if type(val) is float:
                x = self.datay[i][1:len(self.datay[i])]
                x.append(val)
                self.datay[i] = x

            dpg.set_value(self.seriesTags[i], [self.datax[i], self.datay[i]])