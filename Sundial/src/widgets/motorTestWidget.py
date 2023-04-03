from math import cos
import time
import dearpygui.dearpygui as dpg
import wpilib
import ntcore




testTableName = "Testing/"
table = ntcore.NetworkTableInstance.getDefault().getTable(testTableName)






class MotorTest:


    __count = 0
    __i = 0
    parent = 0

    def init(self):



        self.__datax = []
        self.__datay = []
        for i in range(0, 1000):
            self.__datax.append(i / 1000)
            self.__datay.append(0.5)



        pref = str(self.__class__.__count)
        self.pref = str(self.__class__.__count)

        if self.__class__.__count == 0:
            with dpg.window(label="Motor Testers") as window:
                self.__class__.parent = window
                def x():
                    m = MotorTest()
                    m.init()

                dpg.add_button(label="Create new", callback=x)





        # with dpg.window(label="Spark Tester", min_size=[572, 135], max_size=[572, 135], no_resize=True, no_scrollbar=True, user_data=self) as window:

            # dpg.set_item_callback(window, self.sundialTick)


        with dpg.group(horizontal=True, parent=self.parent, tag=pref+"/group"):

            with dpg.plot(no_title=True, no_mouse_pos=True, height=100) as plot:

                with dpg.theme() as item_theme:
                    with dpg.theme_component(dpg.mvAll):
                        dpg.add_theme_style(dpg.mvPlotStyleVar_PlotPadding, 0, 0, category=dpg.mvThemeCat_Plots)
                        dpg.add_theme_style(dpg.mvPlotStyleVar_PlotBorderSize, 1, 1, category=dpg.mvThemeCat_Plots)

                dpg.bind_item_theme(plot, item_theme) # type: ignore
                # REQUIRED: create x and y axes
                dpg.add_plot_axis(dpg.mvXAxis, lock_min=True, lock_max=True, no_tick_labels=True)
                dpg.add_plot_axis(dpg.mvYAxis, tag=pref+"/y_axis", no_gridlines=True, no_tick_labels=True, lock_max=True, lock_min=True)

                # series belong to a y axis
                dpg.add_line_series(self.__datax, self.__datay, parent=pref+"/y_axis", tag=pref+"/series_tag")




            def setVal(sender, data):
                dpg.set_value(pref+"/speedVal", data)
            dpg.add_slider_double(min_value=-1, max_value=1,  callback=setVal, source=pref+"/speedVal", vertical=True, width=40)




            def stopMtr(sender, data):
                dpg.set_value(pref+"/speedVal", 0.0)
            dpg.add_button(label="Stop motor", callback=stopMtr, height=100, width=100)


            with dpg.group():
                with dpg.group(horizontal=True):
                    dpg.add_text("NT Tag")
                    dpg.add_input_text(default_value="motor" + str(self.__class__.__count), width=100)

                motors = [
                    "Spark",
                    "SparkMax",
                    "TalonSRX"
                ]

                with dpg.group(horizontal=True):
                    dpg.add_text("Type: ")
                    dpg.add_combo(motors, width=100)


                # dpg.add_button(label="Start proc", callback=)
                dpg.add_button(label="End proc", callback=lambda s, d: dpg.delete_item(pref+"/group"))


        with dpg.value_registry():
            dpg.add_double_value(tag=pref+"/speedVal", default_value=0)


        self.__class__.__count += 1







    def tick(self):
        val = dpg.get_value(self.pref+"/speedVal")
        if val is not None:
            table.putNumber("speed", val)

            self.__datay[(self.__class__.__i%len(self.__datax))] = (val*0.9/2+0.5)
            self.__class__.__i+=1


        dpg.set_value(self.pref+'/series_tag', [self.__datax, self.__datay])

