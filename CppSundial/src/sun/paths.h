#pragma once

#include "base/allocators.h"
#include "base/utils.h"
#include "base/arr.h"

#include "graphics.h"
#include "blue/blue.h"
#include "colors.h"

#include "glfw/glfw3.h"
#include <memory.h>

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

void sun_pathsBuild(sun_PathInfo* info, gfx_Framebuffer* fb) {
    blu_Area* a = blu_areaMake("paths", blu_areaFlags_CLICKABLE);
    sun_areaAddFB(a, fb);
    float width = fb->texture->width;
    float height = fb->texture->height;
    float aspect = width/height;
    blu_WidgetInteraction inter = blu_interactionFromWidget(a);
    V2f mousePos = inter.mousePos;

    // POINT INSERTION
    if(inter.pressed && inter.button == blu_mouseButton_LEFT) {
        int x = info->pathPtCount;
        info->path[x+0] = (x? (info->path[x-1]) : V2f{ 0, 0 });
        info->path[x+1] = (x? 2 * info->path[x-1] - info->path[x-2] : V2f{0, 0});
        V2f mp = mousePosToCameraPos(mousePos, info->camPos, {width, height}, {info->camHeight*aspect, info->camHeight});
        info->path[x+2] = mp;
        info->path[x+3] = mp;
        info->pathPtCount+=4;
        info->pathDirty = true;
    }
    else if(inter.held && inter.button == blu_mouseButton_LEFT) {
        int x = info->pathPtCount;
        V2f mp = mousePosToCameraPos(mousePos, info->camPos, {width, height}, {info->camHeight*aspect, info->camHeight});
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

        if(inter.held && inter.button == blu_mouseButton_LEFT) {
            info->path[i] = mousePosToCameraPos(
                mousePos,
                info->camPos,
                { width, height },
                { info->camHeight*aspect, info->camHeight });
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


    // DRAW PATHS ==========================================================================================
    gfx_Pass* p = gfx_registerClearPass(col_darkBlue, fb);
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
        V4f pathColor = col_white;
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
        pathColor = col_darkGray;
        LineVert* pts = BUMP_PUSH_ARR(globs.scratch, renderPointCount + 2, LineVert);
        int ptCount = 0;
        ARR_APPEND(pts, ptCount, (LineVert{ V4f(0, 0, 0, 1), pathColor }));
        for(int i = 0; i < info->pathPtCount; i++) {
            ARR_APPEND(pts, ptCount, (LineVert{V4f(info->path[i].x, info->path[i].y, 0, 1), pathColor}));
        }
        ARR_APPEND(pts, ptCount, (LineVert{ V4f(0, 0, 0, 1), pathColor }));
        gfx_updateSSBO(info->connectSSBO, pts, (ptCount) * sizeof(LineVert), false);
        // CLEANUP: for some reason the first segment doesn't appear until you move the camera

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