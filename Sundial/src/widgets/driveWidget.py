import dearpygui.dearpygui as dpg
import Tags
import math
from widgets.widget import widget




wheelWidth = 60
wheelHeight = 60
backColor = [0, 0, 0]
wheelColor = [255, 255, 255]
scWidth = 500
scHeight = 300
treadSpacing = 25
lineThickness = 3
maxWheelSpeed = 10 # per frame

treadPositions = [
    0, 0, 0, 0
]

driveWindowTag = "driveWindow"


class driveWidget(widget):



    def __init__(self):




        with dpg.window(label="Drive", width=scWidth, height=scHeight, tag=driveWindowTag, no_scrollbar=True) as window:



            with dpg.theme() as item_theme:
                with dpg.theme_component(dpg.mvAll):
                    dpg.add_theme_color(dpg.mvThemeCol_FrameBg, (200, 200, 100), category=dpg.mvThemeCat_Core)
                    dpg.add_theme_style(dpg.mvStyleVar_WindowPadding, 0, 0, category=dpg.mvThemeCat_Core)

                dpg.bind_item_theme(window, item_theme) # type: ignore



            with dpg.drawlist(width=scWidth, height=scHeight, tag="driveDraw"):

                with dpg.draw_node(tag="backgroundNode"):
                    dpg.draw_rectangle([-1, -1], [1, 1], color=backColor, fill=backColor)

                with dpg.draw_layer(label="wheel_layer"):
                    with dpg.draw_node(tag="root node"):

                        for x in range(0, 4):
                            with dpg.draw_node(tag=("wheelNode" + str(x))):
                                dpg.draw_rectangle([-wheelWidth/2, wheelHeight/2], [wheelWidth/2, -wheelHeight/2], color=wheelColor, thickness=lineThickness)

                                with dpg.draw_node(tag=("treadNode" + str(x))):
                                    for y in range(0, 5):

                                        flipped = [False, True, True, False][x]
                                        if not flipped:
                                            startY = (-wheelHeight/2) - wheelWidth # bc its a 45 deg angle, width == the dist up
                                            dpg.draw_line([-wheelWidth/2, startY+y*treadSpacing], [wheelWidth/2, startY+y*treadSpacing+wheelWidth], color=wheelColor, thickness=lineThickness)
                                        else:
                                            startY = (-wheelHeight/2)
                                            dpg.draw_line([-wheelWidth/2, startY+y*treadSpacing], [wheelWidth/2, startY+y*treadSpacing-wheelWidth], color=wheelColor, thickness=lineThickness)


                                with dpg.draw_node(tag=("maskNode" + str(x))):
                                    # upper mask
                                    dpg.draw_rectangle([-wheelWidth/2-3, (-wheelHeight/2)-2], [wheelWidth/2+3, (-wheelHeight/2)-wheelWidth-5], fill=backColor, color=backColor)
                                    # lower
                                    dpg.draw_rectangle([-wheelWidth/2-3, (wheelHeight/2)+1], [wheelWidth/2+3, (wheelHeight/2)+wheelWidth+5], fill=backColor, color=backColor)



        dpg.apply_transform("root node", dpg.create_translation_matrix([scWidth / 2, scHeight / 2]))

        translations = [
            [-70.0, -70],
            [ 70, -70],
            [-70,  70],
            [ 70,  70]
        ]

        for i in range(0, 4):
            dpg.apply_transform(("wheelNode" + str(i)), dpg.create_translation_matrix(translations[i]))

        # dpg.apply_transform("planet node 1", dpg.create_rotation_matrix(math.pi*planet1_angle/180.0 , [0, 0, -1])*dpg.create_translation_matrix([planet1_distance, 0]))
        # dpg.apply_transform("planet 1, moon node", dpg.create_rotation_matrix(math.pi*planet1_moonangle/180.0 , [0, 0, -1])*dpg.create_translation_matrix([planet1_moondistance, 0]))
        # dpg.apply_transform("planet node 2", dpg.create_rotation_matrix(math.pi*planet2_angle/180.0 , [0, 0, -1])*dpg.create_translation_matrix([planet2_distance, 0]))
        # dpg.apply_transform("planet 2, moon 1 node", dpg.create_rotation_matrix(math.pi*planet2_moon1distance/180.0 , [0, 0, -1])*dpg.create_translation_matrix([planet2_moon1distance, 0]))
        # dpg.apply_transform("planet 2, moon 2 node", dpg.create_rotation_matrix(math.pi*planet2_moon2angle/180.0 , [0, 0, -1])*dpg.create_translation_matrix([planet2_moon2distance, 0]))


    def tick(self) -> None:
        names = [Tags.FLPWR, Tags.FRPWR, Tags.BLPWR, Tags.BRPWR]
        for i in range(0, 4):
            treadPositions[i] -= dpg.get_value(names[i]) * maxWheelSpeed
            treadPositions[i] = treadPositions[i] % treadSpacing
            dpg.apply_transform(("treadNode" + str(i)), dpg.create_translation_matrix([0, treadPositions[i]]))

        width = dpg.get_item_width(driveWindowTag)
        height = dpg.get_item_height(driveWindowTag)

        if width is None or height is None:
            return

        dpg.apply_transform("root node", dpg.create_translation_matrix([width / 2, height / 2]))
        dpg.set_item_width("driveDraw", width=width)
        dpg.set_item_height("driveDraw", height=height)

        dpg.apply_transform("backgroundNode", dpg.create_scale_matrix([width, height]))





