import dearpygui.dearpygui as dpg
from widgets.widget import widget
import utils.tags as tags
from utils.tables import telemTable
import math


scWidth = 500
scHeight = 500
backColor = [ 0, 0, 0, 255 ]
botColor = [ 255, 255, 255, 255 ]

botSizeX = 0.711
botSizeY = 0.711

fieldWidth = 16.4846
fieldHeight = 8.1026




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

        self.camX = 0
        self.camY = 0
        self.scale = 0.02

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

                    with dpg.draw_node() as camScaleTag:
                        self.camScaleTag: int|str = camScaleTag

                        with dpg.draw_node() as camTranslateTag:
                            self.camTranslateTag: int|str = camTranslateTag

                            dpg.draw_line([0, 1], [0, -1])
                            dpg.draw_line([1, 0], [-1, 0])

                            dpg.draw_rectangle([-fieldWidth/2, fieldHeight/2], [fieldWidth/2, -fieldHeight/2])

                            with dpg.draw_node() as botTranslateTag:
                                self.botTranslateTag: int|str = botTranslateTag

                                with dpg.draw_node() as botRotateTag:
                                    self.botRotateTag: int|str = botRotateTag

                                    drawPolyRect(
                                        [-botSizeX/2, -botSizeY/2],
                                        [botSizeX/2, botSizeY/2], botColor, backColor, 4)


        dpg.apply_transform(self.rootNode, dpg.create_translation_matrix([scWidth / 2, scHeight / 2])) # type: ignore


    def tick(self) -> None:

        width = dpg.get_item_width(self.windowTag)
        height = dpg.get_item_height(self.windowTag)
        if width is None or height is None: return


        x: float = telemTable.getNumber("PosX", 0.0) # type: ignore
        y: float = telemTable.getNumber("PosY", 0.0) # type: ignore
        dpg.apply_transform(self.botTranslateTag, dpg.create_translation_matrix([x, -y]))

        angle: float = telemTable.getNumber("Yaw", 0.0) # type: ignore
        angle *= math.pi / 180
        dpg.apply_transform(self.botRotateTag, dpg.create_rotation_matrix(angle, [0, 0, 1]))



        sc = dpg.is_key_down(dpg.mvKey_E) - dpg.is_key_down(dpg.mvKey_Q)
        self.scale += sc * self.scale * 0.01 # TODO: time correction
        self.scale = max(self.scale, 0.01)
        self.scale = min(self.scale, 1)
        dpg.apply_transform(self.camScaleTag, dpg.create_scale_matrix([1/self.scale, 1/self.scale]))


        self.camX += (dpg.is_key_down(dpg.mvKey_D) - dpg.is_key_down(dpg.mvKey_A)) * self.scale * 3
        self.camY += (dpg.is_key_down(dpg.mvKey_W) - dpg.is_key_down(dpg.mvKey_S)) * self.scale * 3
        dpg.apply_transform(self.camTranslateTag, dpg.create_translation_matrix([-self.camX, self.camY]))
        #                                                                                     dumbass down positive drawing system



        dpg.apply_transform(self.rootNode, dpg.create_translation_matrix([width / 2, height / 2]))
        dpg.set_item_width(self.drawListTag, width=width)
        dpg.set_item_height(self.drawListTag, height=height)

        dpg.apply_transform(self.backgroundNodeTag, dpg.create_scale_matrix([width, height]))





