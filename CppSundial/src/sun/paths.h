#pragma once

#include "base/allocators.h"
#include "base/utils.h"

#include "graphics.h"
#include "blue/blue.h"
#include "colors.h"

#include "glfw/glfw3.h"
#include <memory.h>
#include <stdio.h>

#include "sun/sunUtils.h"
#include "sun/sun.h"
extern SunGlobs globs;


void sun_pathsInit(sun_PathInfo* info) {
    *info = sun_PathInfo();
    bump_allocate(&info->resArena, 10000);
    info->pathSSBO = gfx_registerSSBO(nullptr, 0, false);
    info->connectSSBO = gfx_registerSSBO(nullptr, 0, false);

    info->pathPtCount = 8;
    V2f* p = BUMP_PUSH_ARR(&info->resArena, 8, V2f);
    p[0] = V2f(0, 0);
    p[1] = V2f(1, 1);
    p[2] = V2f(3, 1);
    p[3] = V2f(3, 0);

    p[4] = V2f(3, 0);
    p[5] = V2f(3, -1);
    p[6] = V2f(1, -1);
    p[7] = V2f(0, 1);
    info->path = p;
}
void sun_pathsDeinit(sun_PathInfo* info) {
    bump_free(&info->resArena);
    gfx_freeSSBO(info->pathSSBO);
    gfx_freeSSBO(info->connectSSBO);
}



// ptCount is inclusive of tangent points, curve interpreted as a cubic bezier
V2f bezierSample(V2f* pts, U32 ptCount, float t) {
    U32 segCount = ptCount / 4;
    U32 segIdx = (U32)(t*segCount);
    if(segIdx >= segCount) {
        return pts[ptCount-1];
    }

    float pct = fmodf(t*segCount, 1);
    V2f* p = &pts[segIdx*4];
    V2f a = v2f_lerp(p[0], p[1], pct);
    V2f b = v2f_lerp(p[1], p[2], pct);
    V2f c = v2f_lerp(p[2], p[3], pct);

    V2f d = v2f_lerp(a, b, pct);
    V2f e = v2f_lerp(b, c, pct);
    return v2f_lerp(d, e, pct);
}

V2f mousePosToCameraPos(V2f mousePos, V2f cameraPos, V2f res, V2f cameraSize) {
    V2f pos = V2f{ mousePos.x, res.y-mousePos.y };
    pos /= res; // 0-1 range inside of FB, UP+
    pos -= V2f(0.5); // -0.5 to 0.5 range
    pos *= cameraSize;
    pos += cameraPos;
    return pos;
}

void savePath(V2f* path, U32 ptCount) {
    FILE* f = fopen("path.txt", "w");

    for(int i = 0; i < ptCount; i++) {
        V2f pt = path[i];
        str formatted = str_format(globs.scratch, STR("%f, %f\n"), pt.x, pt.y);
        fwrite(formatted.chars, 1, formatted.length, f);
    }
    fclose(f);
}

// return indicates success
bool loadPath(V2f** outPoints, U32* outCount, BumpAlloc* arena) {
    *outPoints = ((V2f*)arena->end);
    *outCount = 0;

    U64 size;
    U8* f = loadFileToBuffer("path.txt", false, &size, globs.scratch);
    if(!f) { return false; }

    str* lines;
    U32 lineCount;
    str_split({f, size}, '\n', globs.scratch, &lineCount, &lines);
    for(int i = 0; i < lineCount; i++) {
        str* components;
        U32 compCount;
        str_split(lines[i], ',', globs.scratch, &compCount, &components);
        if(compCount != 2) { return false; }

        BUMP_PUSH_NEW(arena, V2f);
        V2f pt = { str_toFloat(components[0]), str_toFloat(components[1]) };
        ARR_APPEND(*outPoints, *outCount, pt);
    }

    return true;
}


void sun_pathsBuild(sun_PathInfo* info, gfx_Framebuffer* fb) {
    blu_Area* a = blu_areaMake("paths", blu_areaFlags_CLICKABLE);
    sun_areaAddFB(a, fb);
    float width = fb->texture->width;
    float height = fb->texture->height;
    float aspect = width/height;
    blu_WidgetInteraction inter = blu_interactionFromWidget(a);
    V2f mousePos = inter.mousePos;
    bool showCoords = inter.hovered;
    V2f mp = mousePosToCameraPos(mousePos, info->camPos, {width, height}, {info->camHeight*aspect, info->camHeight});

    // POINT INSERTION
    if(inter.pressed && inter.button == blu_mouseButton_LEFT) {
        int x = info->pathPtCount;
        info->path[x+0] = (x? (info->path[x-1]) : V2f{ 0, 0 });
        info->path[x+1] = (x? 2 * info->path[x-1] - info->path[x-2] : V2f{0, 0});
        info->path[x+2] = mp;
        info->path[x+3] = mp;
        info->pathPtCount+=4;
        info->pathDirty = true;
    }
    else if(inter.held && inter.button == blu_mouseButton_LEFT) {
        int x = info->pathPtCount;
        info->path[x-2] = mp;
        info->pathDirty = true;
    }

    V2f move = {
        (float)glfwGetKey(globs.window, GLFW_KEY_D) - glfwGetKey(globs.window, GLFW_KEY_A),
        (float)glfwGetKey(globs.window, GLFW_KEY_W) - glfwGetKey(globs.window, GLFW_KEY_S)
    };
    float scale = glfwGetKey(globs.window, GLFW_KEY_E) - glfwGetKey(globs.window, GLFW_KEY_Q);
    info->camHeight += scale * info->camHeight * globs.dt;
    info->camPos += info->camHeight * globs.dt * move * 0.5;

    Mat4f proj;
    float halfHeight = info->camHeight / 2;
    matrixOrtho(-aspect * halfHeight, aspect * halfHeight, -halfHeight, halfHeight, 0, 100, proj);
    Mat4f view;
    matrixTranslation(info->camPos.x, info->camPos.y, 0, view);
    matrixInverse(view, view);
    Mat4f vp = view * proj;

    // PT RENDER AND DRAGGING AND REMOVAL =========================================================================
    for(int i = 0; i < info->pathPtCount; i++) {
        bool isControl = (i%4 == 1) || (i%4 == 2);
        a = blu_areaMake(str_format(globs.scratch, STR("%i"), i),
            blu_areaFlags_DRAW_BACKGROUND |
            blu_areaFlags_FLOATING |
            blu_areaFlags_CLICKABLE |
            blu_areaFlags_HOVER_ANIM);
        inter = blu_interactionFromWidget(a);
        float size = lerp(10, 14, a->target_hoverAnim);
        a->style.sizes[blu_axis_X] = { blu_sizeKind_PX, size };
        a->style.sizes[blu_axis_Y] = { blu_sizeKind_PX, size };
        a->style.backgroundColor = isControl? col_purple : col_green;
        if(i == info->pathPtCount-1) {
            a->style.backgroundColor = col_red;
        }

        if(inter.hovered) { showCoords = true; }

        if(inter.held && inter.button == blu_mouseButton_LEFT) {
            info->path[i] = mp;
            info->pathDirty = true;
        }
        if(inter.clicked && inter.button == blu_mouseButton_RIGHT) {
            if(!isControl) {
                U32 start = i%4 == 3? i-3 : i;
                memmove(&info->path[start], &info->path[start+4], (info->pathPtCount - start) * sizeof(V2f));
                info->pathPtCount-=4;
                info->pathDirty = true;
            }
        }

        V2f pt = info->path[i];
        V4f temp = V4f(pt.x, pt.y, 0, 1) * vp; // move to screenspace (-1 to 1 inside of FB area)
        pt = V2f{ temp.x, temp.y } / 2 + V2f(0.5); // shift 0 to 1 range
        pt *= V2f{ width, height }; // move to fb space
        a->offset = { pt.x, height-pt.y }; // invert Y bc ui uses down+
        a->offset -= (a->rect.end - a->rect.start) / 2;
    }


    // TODO: fix float formatting
    // TODO: distance measurement tool
    if(showCoords) {
        a = blu_areaMake("coords", blu_areaFlags_FLOATING | blu_areaFlags_DRAW_TEXT);
        a->style.textColor = col_darkGray;
        blu_style_style(&globs.textSizeStyle, &a->style);
        a->textScale = 0.75;
        a->offset = mousePos + V2f { 15, 0 };
        str s = str_format(globs.scratch, STR("%f, %f"), mp.x, mp.y);
        blu_areaAddDisplayStr(a, s);
    }

    {
        a = blu_areaMake("menuParent", blu_areaFlags_FLOATING);
        a->style.sizes[blu_axis_X] = { blu_sizeKind_PERCENT, 1 };
        a->style.sizes[blu_axis_Y] = { blu_sizeKind_PERCENT, 1 };
        a->style.childLayoutAxis = blu_axis_Y;
        a->offset = { 5, 5 };

        blu_parentScope(a) {
            blu_styleScope(globs.textSizeStyle) {
            blu_style_style(&globs.borderStyle);

                if(sun_makeButton(STR("Save"), col_lightGray).clicked) {
                    savePath(info->path, info->pathPtCount);
                }
                if(sun_makeButton(STR("Load"), col_lightGray).clicked) {
                    bool e = loadPath(&info->path, &info->pathPtCount, &info->resArena);
                    assert(e);
                    info->pathDirty = true;
                }
            }
        }
    }


    gfx_Pass* p = gfx_registerClearPass(col_darkBlue, fb);
    // DRAW FIELD =======================================================================
    {
        p = gfx_registerPass();
        p->target = fb;
        p->shader = globs.sceneShader2d;
        p->passUniforms.vp = vp;

        gfx_UniformBlock* b = gfx_registerCall(p);
        b->texture = globs.fieldTex;
        b->model = Mat4f(1.0);
        matrixScale(16.4846, 8.1026, 1, b->model); // taken from field.h

        U32 sampleCount = 30;
        for(float t = 0; t < 1; t += 1.0/sampleCount) {
            b = gfx_registerCall(p);
            b->texture = globs.solidTex;
            b->color = V4f(0, 1, 0, 1);
            V2f sample = bezierSample(info->path, info->pathPtCount, t);
            matrixTranslation(sample.x, sample.y, 0, b->model);
        }
    }

    // DRAW PATHS ==========================================================================================
    p = gfx_registerPass();
    p->target = fb;
    p->shader = globs.lineShader;
    p->passUniforms.resolution = { width, height };
    p->drawIndexed = false;
    p->passUniforms.vp = vp;

    U32 renderPointCount = info->pathPtCount/4 * PATH_BEZIERSUBDIVCOUNT;
    // Only recalculate curve data if this flag gets set
    // TODO: this will miss one segment between two curves which looks really nasty. plz fix
    if(info->pathDirty) {
        // spline
        V4f pathColor = col_darkGray;
        if(renderPointCount > 0) {
            LineVert* pts = BUMP_PUSH_ARR(globs.scratch, renderPointCount + 2, LineVert);
            int ptCount = 0;
            V2f pt = info->path[0] + info->path[0] - info->path[1];
            ARR_APPEND(pts, ptCount, (LineVert{ V4f(pt.x, pt.y, 0, 1), pathColor })); // 1st miter to point to 1st control
            for(float i = 0; i <= renderPointCount; i += 1) {
                V2f sample = bezierSample(info->path, info->pathPtCount, i/(float)renderPointCount);
                ARR_APPEND(pts, ptCount, (LineVert{ V4f(sample.x, sample.y, 0, 1), pathColor }));
            }
            pt = info->path[info->pathPtCount-1] + info->path[info->pathPtCount-1] - info->path[info->pathPtCount-2];
            ARR_APPEND(pts, ptCount, (LineVert{ V4f(pt.x, pt.y, 0, 1), pathColor })); // end miter points to last control point
            gfx_updateSSBO(info->pathSSBO, pts, (ptCount) * sizeof(LineVert), false);
        }

        // connection between points
        if(info->pathPtCount > 1) {
            pathColor = col_white;
            LineVert* pts = BUMP_PUSH_ARR(globs.scratch, renderPointCount + 2, LineVert);
            int ptCount = 0;
            V2f pt = info->path[0] * 2 - info->path[1];
            ARR_APPEND(pts, ptCount, (LineVert{ V4f(pt.x, pt.y, 0, 1), pathColor }));
            for(int i = 0; i < info->pathPtCount; i++) {
                ARR_APPEND(pts, ptCount, (LineVert{V4f(info->path[i].x, info->path[i].y, 0, 1), pathColor}));
            }
            pt = info->path[info->pathPtCount-1] * 2 - info->path[info->pathPtCount-2];
            ARR_APPEND(pts, ptCount, (LineVert{ V4f(pt.x, pt.y, 0, 1), pathColor }));
            gfx_updateSSBO(info->connectSSBO, pts, (ptCount) * sizeof(LineVert), false);
            // CLEANUP: for some reason the first segment doesn't appear until you move the camera
        }
        info->pathDirty = false;
    }

    gfx_UniformBlock* b;
    if(info->pathPtCount > 0) {
        b = gfx_registerCall(p);
        b->thickness = 1.2;
        b->ssbo = info->connectSSBO;
        b->vertCount = 6*(info->pathPtCount-1);
    }
    if(renderPointCount+1 > 0) {
        b = gfx_registerCall(p);
        b->thickness = 2.3;
        b->ssbo = info->pathSSBO;
        b->vertCount = 6*(renderPointCount+1-1);
    }
}