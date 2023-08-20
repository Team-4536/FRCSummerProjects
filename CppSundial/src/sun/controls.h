#pragma once

#include "base/allocators.h"
#include "base/config.h"
#include "base/arr.h"
#include "base/utils.h"
#include "network/network.h"
#include "blue/blue.h"
#include "colors.h"

#include "sun/sunUtils.h"
#include "sun/sun.h"

#include <memory.h>
#include <cstring>
#include <stdlib.h>

extern SunGlobs globs;

// expects table to be empty
void loadReplay(net_Table* table) {
    U64 size;
    U8* f = loadFileToBuffer("test.log", true, &size, globs.scratch);
    ASSERT(f);

    U8* c = f;
    U8* lineStart = c;
    while((c - f) < size) {
        if(*c != '\n') { c++; continue; }

        str line = { lineStart, c-lineStart };
        U32 spCount;
        str* split;
        str_split(line, ' ', globs.scratch, &spCount, &split);
        ASSERT(spCount > 0);

        // [prop/update] [name]
        // prop:                [type]
        // update:              [timestamp] [value]

        if(split[0].length == 1 && split[0].chars[0] == 'p') {
            net_Prop* p = ARR_APPEND(table->props, table->propCount, net_Prop());
            p->name = str_copy(split[1], globs.ctrlInfo.replayArena);

            str typeStr = split[2];
            if(str_compare(typeStr, STR("s32"))) { p->type = net_propType_S32; }
            else if(str_compare(typeStr, STR("f64"))) { p->type = net_propType_F64; }
            else if(str_compare(typeStr, STR("bool"))) { p->type = net_propType_BOOL; }
            else if(str_compare(typeStr, STR("str"))) { p->type = net_propType_STR; }
            else { ASSERT(false); }
        }
        else if(split[0].length == 1 && split[0].chars[0] == 'u') {
            net_PropSample* sample = BUMP_PUSH_NEW(globs.ctrlInfo.replayArena, net_PropSample);

            net_Prop* p = net_getProp(split[1], table);
            ASSERT(p);
            sample->next = p->firstPt;
            p->firstPt = sample;

            sample->timeStamp = atof((const char*)split[2].chars);

            const char* arg = (const char*)split[3].chars;
            if(p->type == net_propType_S32) { sample->s32 = atoi(arg); }
            else if(p->type == net_propType_F64) { sample->f64 = atof(arg); }
            else if(p->type == net_propType_STR) { sample->str = str_copy(split[3], globs.ctrlInfo.replayArena); }
            else if(p->type == net_propType_BOOL) {
                bool end;
                if(str_compare(split[3], STR("true"))) { sample->boo = true; }
                else if(str_compare(split[3], STR("false"))) { sample->boo = false; }
                else { ASSERT(false); }
            }
            else { ASSERT(false); }
        }

        c++;
        lineStart = c;
    }
}

// TODO: add pause/play button
void sun_controlsBuild(sun_ControlsInfo* info) {
    blu_Area* a;

    a = blu_areaMake("controlInfo", 0);
    blu_style_childLayoutAxis(blu_axis_X, &a->style);
    blu_style_sizeY({ blu_sizeKind_REMAINDER, 0 }, &a->style);
    blu_parentScope(a) {
        blu_styleScope(blu_Style()) {
        blu_style_childLayoutAxis(blu_axis_Y);


            a = blu_areaMake("left", blu_areaFlags_DRAW_BACKGROUND);
            blu_style_sizeX({ blu_sizeKind_PX, 75}, &a->style);
            blu_parentScope(a) {
                blu_styleScope(blu_Style()) {
                    blu_style_sizeX({blu_sizeKind_PERCENT, 1});
                    blu_style_sizeY({blu_sizeKind_REMAINDER, 1});

                    bool resetTable = false;
                    if(sun_makeButton(STR("LIVE"), !info->usingSockets?col_darkGray:col_darkBlue, col_lightGray).clicked) {
                        info->usingSockets = true;
                        resetTable = true;
                    }

                    if(sun_makeButton(STR("REPLAY"), info->usingSockets?col_darkGray:col_darkBlue, col_lightGray).clicked) {
                        info->usingSockets = false;
                        resetTable = true;
                    }

                    if(resetTable) {
                        memset(&globs.ctrlInfo.replayTable.props, 0, sizeof(globs.ctrlInfo.replayTable.props));
                        globs.ctrlInfo.replayTable.propCount = 0;
                        bump_clear(globs.ctrlInfo.replayArena);

                        globs.curTime = 0;
                        globs.table = globs.ctrlInfo.replayTable;
                    }
                    // CLEANUP: only trigger on state change, and move to after UI build ends
                }
            }

            a = blu_areaMake("right", blu_areaFlags_DRAW_BACKGROUND);
            blu_style_sizeX({ blu_sizeKind_REMAINDER, 0}, &a->style);

            blu_parentScope(a) {
                blu_styleScope(blu_Style()) {
                blu_style_sizeY({ blu_sizeKind_TEXT, 0 });
                blu_style_sizeX({ blu_sizeKind_REMAINDER, 0 });
                blu_style_childLayoutAxis(blu_axis_X);


                    if(info->usingSockets) {
                        a = blu_areaMake(STR("connectionStatus"), blu_areaFlags_DRAW_BACKGROUND);
                        blu_style_sizeX({ blu_sizeKind_REMAINDER, 0}, &a->style);
                        blu_parentScope(a) {
                            a = blu_areaMake(STR("label"), blu_areaFlags_DRAW_TEXT);
                            blu_areaAddDisplayStr(a, STR("Network status: "));
                            blu_style_sizeX({ blu_sizeKind_TEXT, 0 }, &a->style);

                            a = blu_areaMake(STR("box"), blu_areaFlags_DRAW_BACKGROUND);
                            a->style.backgroundColor = col_red;
                            a->style.sizes[blu_axis_X] = { blu_sizeKind_REMAINDER, 0 };
                            a->style.cornerRadius = 2;
                            if(net_getConnected(&globs.table)) {
                                a->style.backgroundColor = col_green; }
                        } // end connection parent


                        a = blu_areaMake("simSwitchParent", 0);
                        a->style.sizes[blu_axis_X] = { blu_sizeKind_PERCENT, .2 };
                        blu_parentScope(a) {
                            blu_styleScope(blu_Style()) {
                            blu_style_sizeX({ blu_sizeKind_REMAINDER, 0 });
                            blu_style_style(&globs.borderStyle);

                                bool clicked = false;
                                if(sun_makeButton(STR("sim"), info->sim? col_darkGray:col_darkBlue, col_lightGray).clicked) {
                                    info->sim = true;
                                }
                                if(sun_makeButton(STR("real"), !info->sim? col_darkGray:col_darkBlue, col_lightGray).clicked) {
                                    info->sim = false;
                                }
                            }
                        }

                    }
                    else {
                        if(sun_makeButton(STR("load"), col_lightGray).clicked) {
                            bump_clear(globs.ctrlInfo.replayArena);
                            memset(&globs.ctrlInfo.replayTable.props, 0, sizeof(globs.ctrlInfo.replayTable.props));
                            globs.ctrlInfo.replayTable.propCount = 0;

                            loadReplay(&globs.ctrlInfo.replayTable);

                            globs.curTime = 0;
                            globs.table = globs.ctrlInfo.replayTable;
                            globs.ctrlInfo.refreshTableFlag = true;
                        }

                        a = blu_areaMake("nav", blu_areaFlags_DRAW_BACKGROUND |
                                                blu_areaFlags_HOVER_ANIM |
                                                blu_areaFlags_CLICKABLE |
                                                blu_areaFlags_DRAW_TEXT |
                                                blu_areaFlags_CENTER_TEXT);
                        a->style.backgroundColor = v4f_lerp(col_darkBlue, col_darkGray, a->target_hoverAnim);
                        blu_WidgetInteraction i = blu_interactionFromWidget(a);
                        if(i.held) {
                            a->cursor = blu_cursor_resizeH;
                            globs.curTime += i.dragDelta.x * 0.01f;
                            globs.ctrlInfo.refreshTableFlag = true;
                        }
                        blu_areaAddDisplayStr(a, str_format(globs.scratch, STR("%f"), globs.curTime));
                    }
                }
            } // end right side
        }
    } // end control info
};