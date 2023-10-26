#pragma once

#include "blue/blue.h"
#include "network/network.h"
#include "colors.h"

#include "sun/sunUtils.h"
#include "sun/sun.h"
extern SunGlobs globs;

void sun_swerveBuild(sun_SwerveInfo* info, gfx_Framebuffer* target) {

    blu_Area* a = blu_areaMake("swerveDisplay", blu_areaFlags_DRAW_TEXTURE);
    a->texture = target->texture;

    sun_areaAddFB(a, target);

    gfx_Pass* p = gfx_registerClearPass(col_darkBlue, target);

    p = gfx_registerPass();
    p->target = target;
    p->shader = globs.sceneShader2d;

    float aspect = ((float)target->texture->width/target->texture->height);
    float size = 4;
    float width = size * aspect;
    float height = size;

    if(aspect < 1) {
        width = 4;
        height = width * (1/aspect);
    }
    matrixOrtho(-width/2, width/2, -height/2, height/2, 0, 10000, p->passUniforms.vp);


    V2f translations[] = {
        { -1, 1 },
        { 1, 1},
        { -1, -1},
        { 1, -1 }
    };

    net_PropSample* rotationProps[] = {
        net_getSample(STR("FLSteerPos"), net_propType_F64, &globs.table),
        net_getSample(STR("FRSteerPos"), net_propType_F64, &globs.table),
        net_getSample(STR("BLSteerPos"), net_propType_F64, &globs.table),
        net_getSample(STR("BRSteerPos"), net_propType_F64, &globs.table)
    };
    net_PropSample* distProps[] = {
        net_getSample(STR("FLDrivePos"), net_propType_F64, &globs.table),
        net_getSample(STR("FRDrivePos"), net_propType_F64, &globs.table),
        net_getSample(STR("BLDrivePos"), net_propType_F64, &globs.table),
        net_getSample(STR("BRDrivePos"), net_propType_F64, &globs.table)
    };

    net_PropSample* angleSample = net_getSample(STR("yaw"), net_propType_F64, &globs.table);
    float angle = 90;
    if(angleSample) { angle += (F32)(angleSample->f64); }

    Mat4f temp;

    for(int i = 0; i < 4; i++) {

        Mat4f t;
        matrixTranslation(translations[i].x, translations[i].y, 0, t);

        if(rotationProps[i]) {
            matrixZRotation(-(F32)rotationProps[i]->f64, temp);
            t = temp * t;
        }

        matrixZRotation(-angle, temp);
        t = t * temp;

        // wheel
        gfx_UniformBlock* b = gfx_registerCall(p);
        b->texture = globs.wheelTex;
        b->model = t;

        // tread
        matrixScale(0.25, 0.95, 0, temp);

        float pos = 0;
        if(distProps[i]) {
            pos = (F32)distProps[i]->f64 * 0.1f;
        }

        b = gfx_registerCall(p);
        b->texture = globs.treadTex;
        b->model = temp * t;
        b->srcStart = V2f(0, pos);
        b->srcEnd = V2f(1, pos - 1);


        // forward arrow
        b = gfx_registerCall(p);
        b->texture = globs.solidTex;
        matrixTranslation(0, 0.75, 0, temp);
        t = temp * t;
        matrixScale(0.1, 0.1, 0, temp);
        t = temp * t;
        b->model = t;

    }

    gfx_UniformBlock* b = gfx_registerCall(p);
    b->texture = globs.arrowTex;
    matrixZRotation(-angle, b->model);

    matrixScale((F32)globs.arrowTex->width / globs.arrowTex->height, 1, 1, temp);
    b->model = temp * b->model;
}