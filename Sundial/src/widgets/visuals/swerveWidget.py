import dearpygui.dearpygui as dpg
from typing import Any
import math
from widgets.widget import widget
import utils.tags as tags
from utils.tables import telemTable



halfWheelSpacing: float = 70.0
wheelRadius = 35

backColor = [0, 0, 0]
wheelColor = [255, 255, 255]

wheelWidth = 30
wheelHeight = 70

scWidth = 500
scHeight = 300

treadSpacing = 15
lineThickness = 3
maxWheelSpeed = 1
treadCount = 5

treadPositions: list[float] = [ 0, 0, 0, 0 ]


steeringTags = [
    tags.FLSteering + tags.ENCODER_READING,
    tags.FRSteering + tags.ENCODER_READING,
    tags.BLSteering + tags.ENCODER_READING,
    tags.BRSteering + tags.ENCODER_READING
    ]

driveTags = [
    tags.FLDrive + tags.MOTOR_SPEED_CONTROL,
    tags.FRDrive + tags.MOTOR_SPEED_CONTROL,
    tags.BLDrive + tags.MOTOR_SPEED_CONTROL,
    tags.BRDrive + tags.MOTOR_SPEED_CONTROL
    ]


translations = [
    [-halfWheelSpacing, -halfWheelSpacing],
    [ halfWheelSpacing, -halfWheelSpacing],
    [-halfWheelSpacing,  halfWheelSpacing],
    [ halfWheelSpacing,  halfWheelSpacing]
]

def drawPolyRect(start, end, color, fill, thickness):
    points = [
        [start[0], start[1]],
        [start[0], end[1]],
        [end[0], end[1]],
        [end[0], start[1]]
    ]
    dpg.draw_polygon(points, thickness=thickness, fill=fill, color=color)



class swerveWidget(widget):



    def __init__(self):


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
                        self.rotationNodes = [ ]
                        for x in range(0, 4):
                            with dpg.draw_node() as wheelNode:
                                self.wheelNodes.append(wheelNode)

                                with dpg.draw_node() as rotNode:
                                    self.rotationNodes.append(rotNode)

                                    drawPolyRect([-wheelWidth/2, wheelHeight/2], [wheelWidth/2, -wheelHeight/2], wheelColor, [0, 0, 0, 0], lineThickness)
                                    dpg.draw_circle((0, 0), wheelRadius, color=wheelColor, thickness=lineThickness)
                                    with dpg.draw_node() as treadNode:
                                        self.treadNodes.append(treadNode)
                                        for y in range(0, treadCount):
                                            dpg.draw_line([-wheelWidth/2, y*treadSpacing-wheelHeight/2], [wheelWidth/2, y*treadSpacing-wheelHeight/2], color=wheelColor, thickness=lineThickness)


                                    # upper mask
                                    drawPolyRect([-wheelWidth/2-3, (-wheelHeight/2)-2], [wheelWidth/2+3, (-wheelHeight/2)-wheelWidth-5], backColor, backColor, lineThickness)
                                    # lower
                                    drawPolyRect([-wheelWidth/2-3, (wheelHeight/2)+1], [wheelWidth/2+3, (wheelHeight/2)+wheelWidth+5], backColor, backColor, lineThickness)



        dpg.apply_transform(self.rootNode, dpg.create_translation_matrix([scWidth / 2, scHeight / 2])) # type: ignore
        for i in range(0, 4):
            dpg.apply_transform(self.wheelNodes[i], dpg.create_translation_matrix(translations[i]))





    def tick(self) -> None:
        for i in range(0, 4):

            # TREAD MOVEMENT ============================
            val = telemTable.getValue(driveTags[i], 0.0)
            assert(type(val) is float)

            treadPositions[i] -= val * maxWheelSpeed
            treadPositions[i] = treadPositions[i] % treadSpacing
            dpg.apply_transform(self.treadNodes[i], dpg.create_translation_matrix([0, treadPositions[i]]))


            # TURNING MOVEMENT ==========================
            val = telemTable.getValue(steeringTags[i], 0.0)
            assert(type(val) is float)

            val *= 2 * math.pi
            dpg.apply_transform(self.rotationNodes[i], dpg.create_rotation_matrix(val, [0, 0, 1]))


        width = dpg.get_item_width(self.windowTag)
        height = dpg.get_item_height(self.windowTag)
        if width is None or height is None: return


        dpg.apply_transform(self.rootNode, dpg.create_translation_matrix([width / 2, height / 2]))
        dpg.set_item_width(self.drawListTag, width=width)
        dpg.set_item_height(self.drawListTag, height=height)

        dpg.apply_transform(self.backgroundNodeTag, dpg.create_scale_matrix([width, height]))
