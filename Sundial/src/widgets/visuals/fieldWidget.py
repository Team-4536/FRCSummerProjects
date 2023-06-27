import dearpygui.dearpygui as dpg
from widgets.widget import widget
import utils.tags as tags
from utils.tables import telemTable
import math


scWidth = 500
scHeight = 500

botSizeX = 50
botSizeY = 50

backColor = [ 0, 0, 0, 255 ]

scale = 1/100



# CLEANUP: this is duplicated from swerveWidget
def drawPolyRect(start, end, color, fill, thickness):
    points = [
        [start[0], start[1]],
        [start[0], end[1]],
        [end[0], end[1]],
        [end[0], start[1]]
    ]
    dpg.draw_polygon(points, thickness=thickness, fill=fill, color=color)



class fieldWidget(widget):

    def __init__(self):


        with dpg.window(label="Field", width=scWidth, height=scHeight, no_scrollbar=True) as window:
            self.windowTag: int|str = window # type: ignore


            with dpg.theme() as item_theme:
                with dpg.theme_component(dpg.mvAll):
                    dpg.add_theme_color(dpg.mvThemeCol_FrameBg, (200, 200, 100), category=dpg.mvThemeCat_Core)
                    dpg.add_theme_style(dpg.mvStyleVar_WindowPadding, 0, 0, category=dpg.mvThemeCat_Core)

                dpg.bind_item_theme(window, item_theme) # type: ignore



            with dpg.drawlist(width=scWidth, height=scHeight) as drawList:
                self.drawListTag: int|str = drawList # type: ignore

                with dpg.draw_node() as backgroundNodeTag:
                    self.backgroundNodeTag: int|str = backgroundNodeTag # type: ignore
                    dpg.draw_rectangle([-1, -1], [1, 1], color=backColor, fill=backColor)


                with dpg.draw_node() as root:
                    self.rootNode: int|str = root # type: ignore

                    with dpg.draw_node() as botNodeTag:
                        self.botNodeTag: int|str = botNodeTag
                        with dpg.draw_node() as botRotTag:
                            self.botRotTag: int|str = botRotTag
                            drawPolyRect([-botSizeX/2, -botSizeY/2], [botSizeX/2, botSizeY/2], [255, 255, 255, 255], backColor, 4)


        dpg.apply_transform(self.rootNode, dpg.create_translation_matrix([scWidth / 2, scHeight / 2])) # type: ignore


    def tick(self) -> None:

        width = dpg.get_item_width(self.windowTag)
        height = dpg.get_item_height(self.windowTag)
        if width is None or height is None: return


        x: float = telemTable.getNumber("PosX", 0.0) # type: ignore
        y: float = telemTable.getNumber("PosY", 0.0) # type: ignore
        dpg.apply_transform(self.botNodeTag, dpg.create_translation_matrix([x / scale, -y / scale]))

        angle: float = telemTable.getNumber("Yaw", 0.0) # type: ignore
        angle *= math.pi / 180
        dpg.apply_transform(self.botRotTag, dpg.create_rotation_matrix(angle, [0, 0, 1]))

        dpg.apply_transform(self.rootNode, dpg.create_translation_matrix([width / 2, height / 2]))
        dpg.set_item_width(self.drawListTag, width=width)
        dpg.set_item_height(self.drawListTag, height=height)

        dpg.apply_transform(self.backgroundNodeTag, dpg.create_scale_matrix([width, height]))





