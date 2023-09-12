#pragma once

#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"
#include "base/utils.h"
#include "graphics.h"
#include "network/network.h"
#include "blue/blue.h"
#include "colors.h"
#include "sun/sunUtils.h"
#include "sun/sun.h"

extern SunGlobs globs;

// TODO: make controls not apply unless field is "selected"
// TODO: robot follow mode

void sun_fieldBuild(sun_FieldInfo* info, gfx_Framebuffer* fb) {

    V4f mVec = { 0, 0, 0, 0 };

    F32 moveSpeed = 3;
    GLFWwindow* window = globs.window;
    float dt = globs.dt;
    mVec.x += (glfwGetKey(window, GLFW_KEY_D) - glfwGetKey(window, GLFW_KEY_A)) * moveSpeed * dt;
    mVec.z -= (glfwGetKey(window, GLFW_KEY_W) - glfwGetKey(window, GLFW_KEY_S)) * moveSpeed * dt;
    mVec.y += (glfwGetKey(window, GLFW_KEY_E) - glfwGetKey(window, GLFW_KEY_Q)) * moveSpeed * dt;

    Mat4f ry;
    Mat4f rx;
    matrixYRotation(info->camTransform.ry, ry);
    matrixXRotation(info->camTransform.rx, rx);
    mVec = mVec * (rx * ry);

    info->camTarget.x += mVec.x;
    info->camTarget.y += mVec.y;
    info->camTarget.z += mVec.z;

    net_PropSample* posX = net_getSample(STR("posX"), net_propType_F64, &globs.table);
    net_PropSample* posY = net_getSample(STR("posY"), net_propType_F64, &globs.table);
    net_PropSample* yaw = net_getSample(STR("yaw"), net_propType_F64, &globs.table);

    net_PropSample* estX = net_getSample(STR("estX"), net_propType_F64, &globs.table);
    net_PropSample* estY = net_getSample(STR("estY"), net_propType_F64, &globs.table);

    net_PropSample* targetX = net_getSample(STR("targetX"), net_propType_F64, &globs.table);
    net_PropSample* targetY = net_getSample(STR("targetY"), net_propType_F64, &globs.table);
    net_PropSample* targetA = net_getSample(STR("targetAngle"), net_propType_F64, &globs.table);

    Transform robotTransform = Transform();
    if(posX && posY && yaw) {
        robotTransform.x = (F32)posX->f64;
        robotTransform.z = -(F32)posY->f64;
        robotTransform.ry = -(F32)yaw->f64;
    }

    Transform estimateTransform = Transform();
    if(estX && estY && yaw) {
        estimateTransform.x = (F32)estX->f64;
        estimateTransform.z = -(F32)estY->f64;
        estimateTransform.ry = -(F32)yaw->f64;
    }

    Transform targetTransform = Transform();
    if(targetX && targetY && targetA) {
        targetTransform.x = (F32)targetX->f64;
        targetTransform.z = -(F32)targetY->f64;
        targetTransform.ry = -(F32)targetA->f64;
    }

    blu_Area* a = blu_areaMake("fieldDisplay", blu_areaFlags_DRAW_TEXTURE | blu_areaFlags_CLICKABLE);
    a->texture = fb->texture;
    sun_areaAddFB(a, fb);

    V2f delta = blu_interactionFromWidget(a).dragDelta;
    info->camTarget.ry -= delta.x * 0.5f;
    info->camTarget.rx -= delta.y * 0.5f;

    // CLEANUP: transform lerp func, also quaternions would be cool
    float followStrength = 0.1f;
    info->camTransform.x = lerp(info->camTransform.x, info->camTarget.x, followStrength);
    info->camTransform.y = lerp(info->camTransform.y, info->camTarget.y, followStrength);
    info->camTransform.z = lerp(info->camTransform.z, info->camTarget.z, followStrength);
    info->camTransform.rx = lerp(info->camTransform.rx, info->camTarget.rx, followStrength);
    info->camTransform.ry = lerp(info->camTransform.ry, info->camTarget.ry, followStrength);

    blu_parentScope(a) {
        blu_styleScope(blu_Style()) {
        blu_style_sizeX({ blu_sizeKind_TEXT, 0 });
        blu_style_sizeY({ blu_sizeKind_TEXT, 0 });
        blu_style_backgroundColor(col_darkGray * V4f(1, 1, 1, 0.75f));
        blu_style_style(&globs.borderStyle);

            a = blu_areaMake("buttonParent", blu_areaFlags_FLOATING);
            blu_style_sizeX({ blu_sizeKind_PERCENT, 1 }, &a->style);
            blu_style_childLayoutAxis(blu_axis_X, &a->style);
            a->offset = V2f(5, 5);
            blu_parentScope(a) {

                if(sun_makeButton(STR("Home"), col_white).clicked) {
                    info->camTarget = {
                        0, 6, 0,
                        -90, 0, 0,
                        1, 1, 1
                        };
                }
                if(sun_makeButton(STR("Blue"), col_white).clicked) {
                    info->camTarget = {
                        -4, 7, 0,
                        -75, -90, 0,
                        1, 1, 1
                        };
                }
                if(sun_makeButton(STR("Red"), col_white).clicked) {
                    info->camTarget = {
                        4, 7, 0,
                        -75, 90, 0,
                        1, 1, 1
                        };
                }
            }

            a = blu_areaMake(STR("FPS"), blu_areaFlags_DRAW_TEXT | blu_areaFlags_FLOATING);
            a->offset = V2f(5, fb->texture->height - BLU_FONT_SIZE - 5);
            str n = str_format(globs.scratch, STR("FPS: %f"), 1/globs.dt);
            blu_areaAddDisplayStr(a, n);
        }
    }

    Mat4f view = matrixTransform(info->camTransform);
    matrixInverse(view, view);
    Mat4f proj = Mat4f(1.0f);
    matrixPerspective(90, (float)fb->texture->width / fb->texture->height, 0.01, 1000000, proj);

    gfx_Pass* p = gfx_registerClearPass(col_darkBlue, fb);
    p = gfx_registerPass();
    p->target = fb;
    p->shader = globs.sceneShader3d;
    p->passUniforms.vp = view * proj;

    gfx_UniformBlock* b;

    b = gfx_registerCall(p);
    b->color = V4f(1, 1, 1, 1);
    b->ib = globs.fieldIB;
    b->va = globs.fieldVA;
    b->texture = globs.fieldTex;

    Transform field = Transform(); // TODO: get a field model
    field.sx = 16.4846 / 2;
    field.sz = 8.1026 / 2;
    b->model = matrixTransform(field);

    b = gfx_registerCall(p);
    b->color = col_purple * V4f(1, 1, 1, 0.5);
    b->ib = globs.robotIB;
    b->va = globs.robotVA;
    b->model = matrixTransform(targetTransform);
    b->texture = globs.solidTex;

    b = gfx_registerCall(p);
    b->color = V4f(1, 0, 0, 0.5);
    b->ib = globs.robotIB;
    b->va = globs.robotVA;
    b->model = matrixTransform(estimateTransform);
    b->texture = globs.solidTex;

    b = gfx_registerCall(p);
    b->color = V4f(1, 0, 0, 1);
    b->ib = globs.robotIB;
    b->va = globs.robotVA;
    b->model = matrixTransform(robotTransform);
    b->texture = globs.solidTex;
}