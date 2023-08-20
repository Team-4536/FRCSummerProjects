#pragma once

#include "blue/blue.h"
#include "network/network.h"
#include "colors.h"

#include "sun/sunUtils.h"
#include "sun/sun.h"
extern SunGlobs globs;

void sun_powerIndicatorsBuild(sun_PowerIndicatorInfo* info) {

    blu_Area* a = sun_makeScrollArea(&info->scrollPosition);
    blu_parentScope(a) {
        blu_styleScope(blu_Style()) {
        blu_style_sizeX({ blu_sizeKind_PERCENT, 1 });
        blu_style_sizeY({ blu_sizeKind_PERCENT, 1 });

            for (int i = 0; i < POWER_INDICATOR_COUNT; i++) {
                net_PropSample* s = nullptr;
                if(info->keys[i].str.length > 0) {
                    net_Prop* p = net_getProp(info->keys[i].str, net_propType_F64, &globs.table);
                    if(p) { s = p->firstPt; }
                }
                bool fadeText = (!s || !net_getConnected(&globs.table));

                str indexStr = str_format(globs.scratch, STR("%i"), i);
                a = blu_areaMake(indexStr, blu_areaFlags_DRAW_BACKGROUND | blu_areaFlags_HOVER_ANIM | blu_areaFlags_CLICKABLE | blu_areaFlags_DROP_EVENTS);
                blu_style_style(&globs.borderStyle, &a->style);
                blu_style_childLayoutAxis(blu_axis_X, &a->style);
                blu_style_backgroundColor(col_darkBlue, &a->style);
                blu_style_sizeY({ blu_sizeKind_PX, 40 }, &a->style);


                a->dropTypeMask = dropMasks_NT_PROP;
                blu_WidgetInteraction inter = blu_interactionFromWidget(a);
                if(inter.dropped) {
                    NTKey* k = &info->keys[i];
                    *k = NTKey();
                    net_Prop* p = (net_Prop*)inter.dropVal;
                    ASSERT(p->name.length < 255);
                    k->str = str_copy(p->name, k->chars);
                }


                if(inter.hovered && inter.dropType) {
                    float t = a->target_hoverAnim;
                    blu_style_backgroundColor(v4f_lerp(a->style.backgroundColor, col_lightGray, t), &a->style);
                }
                else if(inter.hovered && !inter.dropType && info->keys[i].str.length > 0) {
                    fadeText = true;
                    blu_parentScope(a) {
                        a = blu_areaMake("X", blu_areaFlags_DRAW_TEXT | blu_areaFlags_CENTER_TEXT);
                        blu_areaAddDisplayStr(a, "X");
                        blu_style_sizeX({ blu_sizeKind_PERCENT, 1 });
                        if(inter.clicked) {
                            info->keys[i] = NTKey();
                        }
                    }
                }

                blu_parentScope(a) {

                    float w = a->calculatedSizes[blu_axis_X];
                    a = blu_areaMake("indicator", blu_areaFlags_DRAW_BACKGROUND | blu_areaFlags_FLOATING);
                    a->offset = { 0, 0 };
                    a->style.sizes[blu_axis_X] = { blu_sizeKind_PX, 0 };

                    blu_style_backgroundColor(col_red, &a->style);
                    if(fadeText) { a->style.backgroundColor *= col_disconnect; }

                    if(s) {
                        float val = (F32)s->f64;
                        float leftShift = 0;
                        float barSize = val * (w/2);

                        if(val < 0) {
                            leftShift = barSize;
                            barSize *= -1;
                        }

                        a->offset = { w/2 + leftShift, 0 };
                        blu_style_sizeX({ blu_sizeKind_PX, barSize }, &a->style);
                    }


                    a = blu_areaMake("key", blu_areaFlags_DRAW_TEXT | blu_areaFlags_FLOATING);
                    a->offset = V2f(0, 0);
                    blu_areaAddDisplayStr(a, info->keys[i].str);
                    if(fadeText) { a->style.textColor *= col_disconnect; }


                    if(s) {
                        int val = (int)(s->f64 * 100);
                        a = blu_areaMake("value", blu_areaFlags_DRAW_TEXT | blu_areaFlags_CENTER_TEXT | blu_areaFlags_FLOATING);
                        blu_areaAddDisplayStr(a, str_format(globs.scratch, STR("%i%%"), val));
                        if(fadeText) { a->style.textColor *= col_disconnect; }
                    }


                } // end per elem parent
            } // end loop
        }
    } // end scroll view
}
