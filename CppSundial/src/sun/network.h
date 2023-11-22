#pragma once

#include "blue/blue.h"
#include "colors.h"
#include "network/network.h"

#include "sun/sunUtils.h"
#include "sun/sun.h"
extern SunGlobs globs;

void sun_networkBuild(sun_NetInfo* info) {
    blu_Area* a;

    blu_styleScope(blu_Style()) {
        blu_style_sizeX({ blu_sizeKind_PERCENT, 1 });
        blu_style_sizeY({ blu_sizeKind_REMAINDER, 0 });
        a = sun_makeScrollArea(&info->clipPos);
    }

    blu_parentScope(a) {
        blu_styleScope(blu_Style()) {
        blu_style_sizeY({ blu_sizeKind_TEXT, 0 });
        blu_style_childLayoutAxis(blu_axis_X);
        blu_style_backgroundColor(col_darkGray);

            for(int i = 0; i < globs.table.propCount; i++) {
                net_Prop* prop = &globs.table.props[i];
                if(!prop->firstPt) { continue; }

                str quotedName = str_format(globs.scratch, STR("\"%s\""), prop->name);

                blu_Area* parent = blu_areaMakeF(blu_areaFlags_DRAW_BACKGROUND | blu_areaFlags_HOVER_ANIM | blu_areaFlags_CLICKABLE, "%i", i);
                blu_style_style(&globs.borderStyle, &parent->style);
                F32 t = parent->target_hoverAnim;
                parent->dropType = dropMasks_NT_PROP;
                parent->dropVal = prop;

                if(blu_interactionFromWidget(parent).held) {
                    t = 1;
                    blu_parentScope(blu_getCursorParent()) {
                        blu_styleScope(blu_Style()) {
                        blu_style_style(&globs.borderStyle);
                        blu_style_borderSize(2);
                        blu_style_sizeX({ blu_sizeKind_TEXT, 0 });
                        blu_style_sizeY({ blu_sizeKind_TEXT, 0 });
                        blu_style_cornerRadius(4);

                            a = blu_areaMake("drag indicator", blu_areaFlags_DRAW_BACKGROUND | blu_areaFlags_DRAW_TEXT);
                            blu_areaAddDisplayStr(a, quotedName);
                        }
                    }
                }
                parent->style.backgroundColor = v4f_lerp(col_darkBlue, col_darkGray, t);

                blu_parentScope(parent) {
                    blu_styleScope(blu_Style()) {
                    blu_style_sizeX({ blu_sizeKind_PERCENT, 0.5 });
                    blu_style_style(&globs.borderStyle);

                        a = blu_areaMake("label", blu_areaFlags_DRAW_TEXT);
                        blu_areaAddDisplayStr(a, quotedName);

                        bool connected = net_getConnected(&globs.table);
                        if(!connected) {
                            a->style.backgroundColor *= col_disconnect;
                            a->style.textColor *= col_disconnect;
                        }

                        a = blu_areaMake("value", blu_areaFlags_DRAW_TEXT | blu_areaFlags_DRAW_BACKGROUND);
                        a->style.backgroundColor = col_darkGray;

                        BumpAlloc* scratch = globs.scratch;
                        if(prop->type == net_propType_S32) {
                            blu_areaAddDisplayStrF(a, "%i", prop->firstPt->s32); }
                        else if(prop->type == net_propType_F64) {
                            blu_areaAddDisplayStrF(a, "%G", prop->firstPt->f64); }
                        else if(prop->type == net_propType_STR) {
                            blu_areaAddDisplayStrF(a, "\"%s\"", prop->firstPt->str); }
                        else if(prop->type == net_propType_BOOL) {
                            blu_areaAddDisplayStrF(a, "%s", prop->firstPt->boo? "true" : "false");
                            a->style.backgroundColor = prop->firstPt->boo? col_green : col_red;
                            a->style.cornerRadius = 2;
                            a->style.textColor = col_darkBlue;
                        }

                        if(!connected) {
                            a->style.backgroundColor *= col_disconnect;
                            a->style.textColor *= col_disconnect;
                        }
                    }
                }
            }
        } // end text styling
    } // end of clip
}