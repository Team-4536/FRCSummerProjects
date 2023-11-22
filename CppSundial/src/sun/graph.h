#pragma once

#include "sun/sunUtils.h"
#include "base/utils.h"
#include "graphics.h"

#include "colors.h"
#include "blue/blue.h"
#include "network/network.h"
#include "memory.h"

#include "sun/sun.h"
extern SunGlobs globs;

void sun_graph2dInit(sun_Graph2dInfo* info) {
    *info = sun_Graph2dInfo();

    info->colors[0] = col_green;
    info->keys[0].str = str_copy(STR("FLSteerSpeed"), info->keys[0].chars);

    info->colors[1] = col_red;
    info->keys[1].str = str_copy(STR("FLSteerPos"), info->keys[1].chars);

    info->colors[2] = col_purple;
    info->colors[3] = col_yellow;
    info->colors[4] = col_black;
    info->colors[5] = col_white;

    for(int i = 0; i < GRAPH2D_LINECOUNT; i++) {
        info->lineVerts[i] = gfx_registerSSBO(nullptr, 0, true);
    }
    info->gridVerts = gfx_registerSSBO(nullptr, 0, true);
    info->timeVerts = gfx_registerSSBO(nullptr, 0, true);
}
void sun_graph2dDeinit(sun_Graph2dInfo* info) {
    for(int i = 0; i < GRAPH2D_LINECOUNT; i++) {
        gfx_freeSSBO(info->lineVerts[i]);
    }
    gfx_freeSSBO(info->gridVerts);
    gfx_freeSSBO(info->timeVerts);
}


void sun_graph2dBuild(sun_Graph2dInfo* info, gfx_Framebuffer* target) {

    blu_Area* a = blu_areaMake("graph2d", blu_areaFlags_DRAW_BACKGROUND);
    blu_style_childLayoutAxis(blu_axis_Y, &a->style);

    blu_parentScope(a) {

        a = blu_areaMake("upperBit", blu_areaFlags_DRAW_TEXTURE | blu_areaFlags_CLICKABLE | blu_areaFlags_SCROLLABLE);
        blu_style_sizeY({blu_sizeKind_REMAINDER, 0}, &a->style);
        sun_areaAddFB(a, target);
        float width = target->texture->width;
        float height = target->texture->height;

        gfx_registerClearPass(col_darkBlue, target);

        gfx_Pass* p = gfx_registerPass();
        p->target = target;
        p->shader = globs.lineShader;
        p->passUniforms.resolution = a->rect.end - a->rect.start;
        p->drawIndexed = false;

        blu_WidgetInteraction inter = blu_interactionFromWidget(a);

        if(height != 0 && inter.hovered) {
            float viewSize = info->top - info->bottom;
            V2f change = V2f(inter.dragDelta.y * (viewSize / height));

            float scaleChange = inter.scrollDelta * viewSize * 0.05;
            float pct = (inter.mousePos.y / height);

            change += V2f(pct * scaleChange, (1-pct) * -scaleChange);

            info->top += change.x;
            info->bottom += change.y;
        }
        matrixOrtho(0, 1, info->bottom, info->top, 0, 100, p->passUniforms.vp);

        float pointGap = 1 / (float)GRAPH2D_VCOUNT;
        float sampleGap = GRAPH2D_SAMPLE_WINDOW / GRAPH2D_VCOUNT;


        blu_styleScope(blu_Style()) {
        blu_style_sizeX({ blu_sizeKind_TEXT, 0 });
        blu_style_sizeY({ blu_sizeKind_TEXT, 0 });
            {
                // time lines
                int lineCount = 10;
                int vertCount = lineCount * 2;
                LineVert* gridPts = BUMP_PUSH_ARR(globs.scratch, vertCount+4, LineVert);
                float ypts[] = { info->bottom, info->top };

                for(int i = 0; i < lineCount; i++) {
                    LineVert* v = &gridPts[i*2+1];
                    bool even = i%2 == 0;

                    float x = ((i-fmodf(globs.curTime, 1)) / GRAPH2D_SAMPLE_WINDOW);
                    V4f color = v4f_lerp(col_darkGray, col_darkBlue, (1-x)/2); // gradient
                    v[0] = { V4f(x, ypts[even], 0, 1), color };
                    v[1] = { V4f(x, ypts[!even], 0, 1), color };
                }

                // hover line
                float x = 100;
                if(inter.hovered) { x = inter.mousePos.x / width; }
                gridPts[vertCount+1] = { V4f(x, ypts[1], 0, 1), col_darkGray };
                gridPts[vertCount+2] = { V4f(x, ypts[0], 0, 1), col_darkGray };
                gridPts[vertCount+3] = { V4f(0, 0, 0, 1), col_darkGray };

                gfx_updateSSBO(info->timeVerts, gridPts, sizeof(LineVert) * (vertCount+4), true);
                gfx_UniformBlock* b = gfx_registerCall(p);
                b->thickness = 1;
                b->ssbo = info->timeVerts;
                b->vertCount = 6*(vertCount+2-1);

                // time labels
                float timeOffset = fmodf(globs.curTime, 1);

                for(int i = 0; i < 10; i++) {
                    a = blu_areaMake(str_format(globs.scratch, STR("time label %i"), i),
                        blu_areaFlags_FLOATING | blu_areaFlags_DRAW_TEXT | blu_areaFlags_CENTER_TEXT);
                    a->offset =  { (1-((i+timeOffset) / GRAPH2D_SAMPLE_WINDOW)) * width - a->calculatedSizes[blu_axis_X] / 2, height - BLU_FONT_SIZE };
                    a->textScale = 0.7f;
                    blu_areaAddDisplayStr(a, str_format(globs.scratch, STR("%i"), (int)(globs.curTime-i)));
                }
                if(inter.hovered) {
                    a = blu_areaMake("hoverLabel", blu_areaFlags_FLOATING | blu_areaFlags_DRAW_TEXT | blu_areaFlags_CENTER_TEXT);
                    a->offset =  { inter.mousePos.x - a->calculatedSizes[blu_axis_X] / 2, height - BLU_FONT_SIZE };
                    a->textScale = 0.75f;
                    blu_areaAddDisplayStr(a, str_format(globs.scratch, STR("%f"), globs.curTime - GRAPH2D_SAMPLE_WINDOW * (1-(inter.mousePos.x / width))));
                }
            }

            // value lines
            {
                int lineCount = 14;
                int vertCount = lineCount * 2;
                LineVert* gridPts = BUMP_PUSH_ARR(globs.scratch, vertCount+2, LineVert);
                float xpts[] = { -0.1, 1.1 };

                float lineSpacing = 0;
                float lineOffset = 0;
                { // find line spacing/ offset based on zoom
                    float lineGap = (info->top - info->bottom) / 6;
                    assert(lineGap > 0);

                    float dec = lineGap; // get the base ten decimal/exponents
                    int exp = 0;
                    while(dec < 1) { dec *= 10; exp--; }
                    while(dec > 10) { dec /= 10; exp++; }

                    int roundingTargets[] = { 5, 2, 1 };
                    for(int i = 0; i < 3; i++) {
                        if(dec > roundingTargets[i]) {
                            dec = roundingTargets[i];
                            break;
                        }
                    }
                    lineSpacing = dec * powf(10, exp);
                    lineOffset = fmodf((info->top + info->bottom) / 2, lineSpacing);
                }
                // TODO: fix warnings lol
                float spaceToPx = height / (info->top - info->bottom); // conversion between drawing space and pix
                int centerIdx = (int)((info->top+info->bottom)/2 / lineSpacing);

                for(int i = 0; i < lineCount; i++) {
                    LineVert* v = &gridPts[i*2+1];
                    bool even = i%2 == 0;
                    float y = (i - (lineCount/2) + centerIdx) * lineSpacing;
                    v[0].pos = V4f(xpts[even], y, 0, 1);
                    v[0].color = col_darkGray;
                    v[1].pos = V4f(xpts[!even], y, 0, 1);
                    v[1].color = col_darkGray;
                }
                gridPts[0] = { V4f(), col_darkGray };
                gridPts[vertCount+1] = { V4f(), col_darkGray };
                gfx_updateSSBO(info->gridVerts, gridPts, sizeof(LineVert) * (vertCount+2), true);

                gfx_UniformBlock* b = gfx_registerCall(p);
                b->color = col_darkGray;
                b->thickness = 1;
                b->ssbo = info->gridVerts;
                b->vertCount = 6*(vertCount-1);

                // labels
                {
                    for(int i = 0; i < lineCount; i++) {
                        int k = i - lineCount/2;
                        a = blu_areaMake(str_format(globs.scratch, STR("value label %i"), i),
                            blu_areaFlags_FLOATING | blu_areaFlags_DRAW_TEXT | blu_areaFlags_CENTER_TEXT);
                        a->textScale = 0.7f;
                        float label = (k + centerIdx) * lineSpacing;
                        blu_areaAddDisplayStrF(a, "%f", label);

                        a->offset.x = width - a->calculatedSizes[blu_axis_X];
                        a->offset.y = height / 2 + (-k*lineSpacing + lineOffset) * spaceToPx;
                        a->offset.y -= a->calculatedSizes[blu_axis_Y] / 2;
                    }
                }
            }
        } // end grid things




        LineVert* pts = BUMP_PUSH_ARR(globs.scratch, GRAPH2D_VCOUNT, LineVert);
        for(int i = 0; i < GRAPH2D_LINECOUNT; i++) {
            if(info->keys[i].str.length == 0) { continue; }

            // TODO: int/bool graphing

            for(int j = 0; j < GRAPH2D_VCOUNT; j++) {
                V4f color = info->colors[i];

                float sHeight = 0;
                // TODO: inline traversal instead of restarting constantly
                float sampleTime = globs.curTime - sampleGap*j;
                net_PropSample* sample = net_getSample(info->keys[i].str, net_propType_F64, sampleTime, &globs.table);
                if(sample) { sHeight = (float)sample->f64; }
                else { color *= col_disconnect; }

                sample = net_getSample(STR("/connected"), net_propType_BOOL, sampleTime, &globs.table);
                bool connected = false;
                if(sample && sample->boo) { connected = true; }
                if(sample && !connected) { color *= col_disconnect; }

                pts[j] = { V4f(1-(j*pointGap), sHeight, 0, 1), color };
            }

            gfx_updateSSBO(info->lineVerts[i], pts, sizeof(LineVert) * GRAPH2D_VCOUNT, true);

            gfx_UniformBlock* b = gfx_registerCall(p);
            b->color = info->colors[i];
            b->thickness = 2;
            b->ssbo = info->lineVerts[i];
            b->vertCount = 6*(GRAPH2D_VCOUNT-1);
        }

        a = blu_areaMake("lowerBit", blu_areaFlags_FLOATING);
        blu_style_sizeY({ blu_sizeKind_PERCENT, 1 }, &a->style);
        blu_style_childLayoutAxis(blu_axis_Y, &a->style);
        blu_parentScope(a) {
            blu_styleScope(blu_Style()) {
            blu_style_sizeY({ blu_sizeKind_TEXT, 0 });
            blu_style_sizeX({ blu_sizeKind_CHILDSUM, 0 });
            blu_style_backgroundColor(col_darkGray);
            blu_style_cornerRadius(5);
            blu_style_childLayoutAxis(blu_axis_X);

                for(int i = 0; i < GRAPH2D_LINECOUNT; i++) {
                    if(!info->keys[i].chars) { continue; }

                    a = blu_areaMake(str_format(globs.scratch, STR("thing %i"), i),
                        blu_areaFlags_DRAW_BACKGROUND | blu_areaFlags_DROP_EVENTS | blu_areaFlags_CLICKABLE | blu_areaFlags_HOVER_ANIM);
                    inter = blu_interactionFromWidget(a);

                    bool fadeChildren = false;
                    if(!net_getConnected(&globs.table)) { fadeChildren = true; }
                    if(inter.hovered && !inter.dropType) { fadeChildren = true; }

                    float hoverTarget = a->target_hoverAnim;
                    a->style.backgroundColor = v4f_lerp(a->style.backgroundColor, col_lightGray, hoverTarget);
                    if(fadeChildren) { a->style.backgroundColor *= col_disconnect; }

                    a->dropTypeMask = dropMasks_NT_PROP;
                    if(inter.dropped) {
                        net_Prop* prop = ((net_Prop*)(inter.dropVal));
                        if(prop->type == net_propType_F64) {
                            ASSERT(prop->name.length < 255);
                            info->keys[i].str = str_copy(prop->name, info->keys[i].chars);
                        }
                    }

                    if(inter.clicked && !inter.dropType) {
                        memset(&info->keys[i], 0, sizeof(NTKey));
                    }

                    blu_parentScope(a) {

                        a = blu_areaMake("color", blu_areaFlags_DRAW_BACKGROUND);
                        a->style.sizes[blu_axis_X] = { blu_sizeKind_PX, 14 };
                        a->style.backgroundColor = info->colors[i];
                        if(fadeChildren) { a->style.backgroundColor *= col_disconnect; }

                        a = blu_areaMake("text", blu_areaFlags_DRAW_TEXT);
                        blu_areaAddDisplayStr(a, info->keys[i].str);
                        a->style.sizes[blu_axis_X] = { blu_sizeKind_TEXT, 0 };
                        if(fadeChildren) { a->style.textColor *= col_disconnect; }


                        if(inter.hovered && info->keys[i].str.length == 0) {
                            a = blu_areaMake("spacer", 0);
                            a->style.sizes[blu_axis_X] = { blu_sizeKind_PX, hoverTarget * 10 };
                        }
                        if(inter.hovered && !inter.dropType) {

                            a = blu_areaMake("X", blu_areaFlags_DRAW_TEXT);
                            blu_areaAddDisplayStr(a, "X");
                            a->style.sizes[blu_axis_X] = { blu_sizeKind_TEXT, 0 };
                        }

                    }

                }

            } // end styling for list
        }
    } // end area
};