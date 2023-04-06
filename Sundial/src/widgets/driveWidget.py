import dearpygui.dearpygui as dpg
import math
from widgets.widget import widget
from utils.tables import *



wheelWidth = 60
wheelHeight = 60
backColor = [0, 0, 0]
wheelColor = [255, 255, 255]
scWidth = 500
scHeight = 300
treadSpacing = 25
lineThickness = 3
maxWheelSpeed = 10 # per frame

treadPositions: list[float] = [
    0, 0, 0, 0
]

halfWheelSpacing: float = 70.0


class driveWidget(widget):



    def __init__(self):


        self.ntTags = [ "", "", "", "" ]


        with dpg.window(label="Drive", width=scWidth, height=scHeight, no_scrollbar=True) as window:
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

                with dpg.draw_layer(label="wheel_layer"):
                    with dpg.draw_node() as root:
                        self.rootNode: int|str = root # type: ignore

                        self.wheelNodes = [ ]
                        self.treadNodes = [ ]
                        self.maskNodes = [ ]
                        for x in range(0, 4):
                            with dpg.draw_node() as wheelNode:
                                self.wheelNodes.append(wheelNode)
                                dpg.draw_rectangle([-wheelWidth/2, wheelHeight/2], [wheelWidth/2, -wheelHeight/2], color=wheelColor, thickness=lineThickness)

                                with dpg.draw_node() as treadNode:
                                    self.treadNodes.append(treadNode)
                                    for y in range(0, 5):

                                        flipped = [False, True, True, False][x]
                                        if not flipped:
                                            startY = (-wheelHeight/2) - wheelWidth # bc its a 45 deg angle, width == the dist up
                                            dpg.draw_line([-wheelWidth/2, startY+y*treadSpacing], [wheelWidth/2, startY+y*treadSpacing+wheelWidth], color=wheelColor, thickness=lineThickness)
                                        else:
                                            startY = (-wheelHeight/2)
                                            dpg.draw_line([-wheelWidth/2, startY+y*treadSpacing], [wheelWidth/2, startY+y*treadSpacing-wheelWidth], color=wheelColor, thickness=lineThickness)


                                with dpg.draw_node() as maskNode:
                                    self.maskNodes.append(maskNode)
                                    # upper mask
                                    dpg.draw_rectangle([-wheelWidth/2-3, (-wheelHeight/2)-2], [wheelWidth/2+3, (-wheelHeight/2)-wheelWidth-5], fill=backColor, color=backColor)
                                    # lower
                                    dpg.draw_rectangle([-wheelWidth/2-3, (wheelHeight/2)+1], [wheelWidth/2+3, (wheelHeight/2)+wheelWidth+5], fill=backColor, color=backColor)



        dpg.apply_transform(self.rootNode, dpg.create_translation_matrix([scWidth / 2, scHeight / 2])) # type: ignore

        translations = [
            [-halfWheelSpacing, -halfWheelSpacing],
            [ halfWheelSpacing, -halfWheelSpacing],
            [-halfWheelSpacing,  halfWheelSpacing],
            [ halfWheelSpacing,  halfWheelSpacing]
        ]

        for i in range(0, 4):
            dpg.apply_transform(self.wheelNodes[i], dpg.create_translation_matrix(translations[i]))






    def tick(self) -> None:
        for i in range(0, 4):

            val = telemTable.getValue(self.ntTags[i], None)
            if type(val) is not float: continue

            treadPositions[i] -= val * maxWheelSpeed
            treadPositions[i] = treadPositions[i] % treadSpacing
            dpg.apply_transform(self.treadNodes[i], dpg.create_translation_matrix([0, treadPositions[i]]))


        width = dpg.get_item_width(self.windowTag)
        height = dpg.get_item_height(self.windowTag)
        if width is None or height is None: return


        dpg.apply_transform(self.rootNode, dpg.create_translation_matrix([width / 2, height / 2]))
        dpg.set_item_width(self.drawListTag, width=width)
        dpg.set_item_height(self.drawListTag, height=height)

        dpg.apply_transform(self.backgroundNodeTag, dpg.create_scale_matrix([width, height]))





