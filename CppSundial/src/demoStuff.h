#pragma once

#include "graphics.h"
#include "base/utils.h"
#include "blue/blue.h"




#ifdef DEMO_IMPL

static struct DemoGlobs {

    gfx_Shader* sceneShader = nullptr;
    gfx_Framebuffer* sceneFB = nullptr;

    gfx_VertexArray* sceneVA = nullptr;
    gfx_IndexBuffer* sceneIB = nullptr;

    // TODO: v3s
    V4f camPos = { 0.5, 0.5, 2, 0 };
    V2i viewPortSize = { 0, 0 };

    V2f windowPos = V2f(350, 100);

    float clipSize = 0;
    float clipPos = 0;
    float clipMax = 1400;

} demoGlobs;


// TODO: make deps clearer

void demo_init(BumpAlloc* frameArena) {
    demoGlobs = DemoGlobs();
    demoGlobs.sceneFB = gfx_registerFramebuffer();

    float vaData[] = {
        0, 0, 0,        0, 0,
        0, 1, 0,        0, 0,
        1, 1, 0,        0, 0,
        1, 0, 0,        0, 0
    };
    demoGlobs.sceneVA = gfx_registerVertexArray(gfx_vtype_POS3F_UV, vaData, sizeof(vaData), false);

    U32 ibData[] = {
        0, 1, 2,
        2, 3, 0 };
    demoGlobs.sceneIB = gfx_registerIndexBuffer(ibData, sizeof(ibData) / sizeof(U32));



    {
        demoGlobs.sceneShader = gfx_registerShader(gfx_vtype_POS3F_UV, "res/shaders/scene.vert", "res/shaders/scene.frag", frameArena);

        demoGlobs.sceneShader->passUniformBindFunc = [](gfx_Pass* pass, gfx_UniformBlock* uniforms) {
            int loc;

            loc = glGetUniformLocation(pass->shader->id, "uVP");
            glUniformMatrix4fv(loc, 1, false, &(uniforms->vp)[0]);
        };

        demoGlobs.sceneShader->uniformBindFunc = [](gfx_Pass* pass, gfx_UniformBlock* uniforms) {
            int loc;

            loc = glGetUniformLocation(pass->shader->id, "uColor");
            glUniform4f(loc, uniforms->color.x, uniforms->color.y, uniforms->color.z, uniforms->color.w);

            loc = glGetUniformLocation(pass->shader->id, "uModel");
            glUniformMatrix4fv(loc, 1, false, &(uniforms->model)[0]);

            gfx_bindVertexArray(pass, uniforms->va);
            gfx_bindIndexBuffer(pass, uniforms->ib);
        };
    }

}


blu_Area* demo_makeFancyButton(str name, int stdWidth, int hoverWidth, V4f hoverBack, V4f hoverText) {

    blu_Area* a = blu_areaMake(name,
        blu_areaFlags_DRAW_BACKGROUND |
        blu_areaFlags_DRAW_TEXT |
        blu_areaFlags_HOVER_ANIM |
        blu_areaFlags_CLICKABLE);

    blu_areaAddDisplayStr(a, name);
    a->style.backgroundColor = v4f_lerp(a->style.backgroundColor, hoverBack, a->target_hoverAnim);
    a->style.textColor = v4f_lerp(a->style.textColor, hoverText, a->target_hoverAnim);
    a->style.sizes[blu_axis_X] = { blu_sizeKind_PX, lerp(stdWidth, hoverWidth, a->target_hoverAnim) };

    return a;
}


void demo_makeUI(BumpAlloc& frameArena, float dt, GLFWwindow* window) {

    blu_Area* a;


    blu_styleScope
    {
    blu_style_add_backgroundColor(col_darkBlue);
    blu_style_add_textColor(col_white);
    blu_style_add_sizeX({ blu_sizeKind_PERCENT, 1 });
    blu_style_add_sizeY({ blu_sizeKind_PERCENT, 1 });
    blu_style_add_textPadding(V2f(3, 3));
    blu_style_add_animationStrength(0.1f);




        a = blu_areaMake(STR("left"), blu_areaFlags_DRAW_BACKGROUND | blu_areaFlags_CLICKABLE);
        demoGlobs.clipSize = a->calculatedSizes[blu_axis_Y];
        blu_areaAddDisplayStr(a, STR("left"));
        a->style.sizes[blu_axis_X] = { blu_sizeKind_PX, 300 };
        a->style.childLayoutAxis = blu_axis_X;
        blu_parentScope(a) {

            a = blu_areaMake(STR("clip"), blu_areaFlags_VIEW_OFFSET | blu_areaFlags_CLICKABLE);
            blu_Area* clip = a;
            blu_areaAddDisplayStr(a, STR("blip"));
            a->style.sizes[blu_axis_X] = { blu_sizeKind_REMAINDER, 0 };
            a->style.childLayoutAxis = blu_axis_Y;
            demoGlobs.clipPos += blu_interactionFromWidget(a).scrollDelta * 40;
            blu_parentScope(a) {

                blu_styleScope {
                blu_style_add_sizeY({ blu_sizeKind_TEXT, 0 });
                blu_style_add_childLayoutAxis(blu_axis_X);
                blu_style_add_backgroundColor(col_darkGray);

                    a = blu_areaMake(STR("FPS"), blu_areaFlags_DRAW_TEXT);
                    char* buf = BUMP_PUSH_ARR(&frameArena, 32, char);
                    gcvt(1/dt, 6, buf);
                    blu_areaAddDisplayStr(a, str_join(STR("FPS: "), STR(buf), &frameArena));


                    a = blu_areaMake(STR("connectionPar"), blu_areaFlags_DRAW_BACKGROUND);
                    blu_areaAddDisplayStr(a, STR("conPar"));
                    blu_parentScope(a) {
                        a = blu_areaMake(STR("networkConnLabel"), blu_areaFlags_DRAW_TEXT);
                        blu_areaAddDisplayStr(a, STR("Network status: "));
                        a->style.sizes[blu_axis_X] = { blu_sizeKind_TEXT, 0 };

                        a = blu_areaMake(STR("connSpacer"), 0);
                        blu_areaAddDisplayStr(a, STR("conspace"));
                        a->style.sizes[blu_axis_X] = { blu_sizeKind_REMAINDER, 0 };

                        a = blu_areaMake(STR("connection"), blu_areaFlags_DRAW_BACKGROUND);
                        a->style.backgroundColor = col_red;
                        a->style.sizes[blu_axis_X] = { blu_sizeKind_PX, 100 };
                        if(net_getConnected()) {
                            a->style.backgroundColor = col_green; }
                    } // end connection parent


                    net_Prop** tracked;
                    U32 tCount = 0;
                    net_getTracked(&tracked, &tCount);
                    for(int i = 0; i < tCount; i++) {
                        net_Prop* prop = tracked[i];

                        a = blu_areaMake(prop->name, 0);

                        blu_parentScope(a) {
                            blu_styleScope {
                            blu_style_add_sizeX({ blu_sizeKind_PERCENT, 0.5 });
                                a = blu_areaMake(str_join(prop->name, STR("label"), &frameArena), blu_areaFlags_DRAW_TEXT);
                                blu_areaAddDisplayStr(a, prop->name);

                                a = blu_areaMake(str_join(prop->name, STR("value"), &frameArena), blu_areaFlags_DRAW_TEXT);
                                a->style.backgroundColor = col_darkGray;
                                if(prop->type == net_propType_S32) {
                                    blu_areaAddDisplayStr(a, str_format(&frameArena, STR("%i"), (prop->data->s32))); }
                            }
                        }
                    }

                } // end text styling

            } // end of clip



            blu_Area* spacer = nullptr;
            if(demoGlobs.clipMax > demoGlobs.clipSize) {
                a = blu_areaMake(STR("scrollpar"), blu_areaFlags_DRAW_BACKGROUND);
                a->style.backgroundColor = col_darkGray;
                a->style.sizes[blu_axis_X] = { blu_sizeKind_PX, 10 };
                a->style.childLayoutAxis = blu_axis_Y;

                blu_parentScope(a) {

                    a = blu_areaMake(STR("scspace"), 0);
                    spacer = a;

                    a = blu_areaMake(STR("scroll"),
                        blu_areaFlags_DRAW_BACKGROUND |
                        blu_areaFlags_CLICKABLE);
                    a->style.backgroundColor = col_lightGray;
                    a->style.sizes[blu_axis_Y] = { blu_sizeKind_PERCENT, demoGlobs.clipSize / demoGlobs.clipMax };
                    demoGlobs.clipPos += blu_interactionFromWidget(a).dragDelta.y / demoGlobs.clipSize * demoGlobs.clipMax;

                }
            } else { demoGlobs.clipPos = 0; }


            demoGlobs.clipPos = max(demoGlobs.clipPos, 0);
            demoGlobs.clipPos = min(demoGlobs.clipPos, demoGlobs.clipMax - (demoGlobs.clipSize));
            clip->viewOffset = { 0, demoGlobs.clipPos };

            if(spacer) {
                spacer->style.sizes[blu_axis_Y] = { blu_sizeKind_PX, (demoGlobs.clipPos / demoGlobs.clipMax) * demoGlobs.clipSize };
            }
        } // end left



        {
            a = blu_areaMake(STR("fbdisplay"), blu_areaFlags_DRAW_TEXTURE);
            a->style.sizes[blu_axis_X] = { blu_sizeKind_REMAINDER, 0 };
            a->texture = demoGlobs.sceneFB->texture;

            int w = (int)a->calculatedSizes[blu_axis_X];
            int h = (int)a->calculatedSizes[blu_axis_Y];

            if(w != demoGlobs.sceneFB->texture->width || h != demoGlobs.sceneFB->texture->height) {
                gfx_resizeFramebuffer(demoGlobs.sceneFB, w, h); }


            gfx_Pass* p = gfx_registerPass();
            p->isClearPass = true;
            p->target = demoGlobs.sceneFB;
            p->passUniforms.color = V4f(0, 0, 0, 1);



            Mat4f proj = Mat4f(1.0f);
            if(h != 0) {
                matrixPerspective(90, (float)w / h, 0.01, 1000000, proj); }

            // TODO: 3d-ify utils
            demoGlobs.camPos.x += (glfwGetKey(window, GLFW_KEY_D) - glfwGetKey(window, GLFW_KEY_A)) * dt;
            demoGlobs.camPos.y += (glfwGetKey(window, GLFW_KEY_W) - glfwGetKey(window, GLFW_KEY_S)) * dt;
            demoGlobs.camPos.z += (glfwGetKey(window, GLFW_KEY_Q) - glfwGetKey(window, GLFW_KEY_E)) * dt;

            Mat4f view;
            matrixTranslation(V2f(demoGlobs.camPos.x, demoGlobs.camPos.y), demoGlobs.camPos.z, view);
            matrixInverse(view, view);

            p = gfx_registerPass();
            p->target = demoGlobs.sceneFB;
            p->shader = demoGlobs.sceneShader;
            p->passUniforms.vp = view * proj;

            gfx_UniformBlock* b = gfx_registerCall(p);
            b->color = V4f(1, 1, 1, 1);
            b->model = Mat4f(1.0f);
            b->ib = demoGlobs.sceneIB;
            b->va = demoGlobs.sceneVA;
        }




        //*
        a = blu_areaMake(STR("window"),
                blu_areaFlags_DRAW_BACKGROUND |
                blu_areaFlags_DRAW_TEXT |
                blu_areaFlags_FLOATING |
                blu_areaFlags_CLICKABLE);

        blu_Area* win = a;
        blu_areaAddDisplayStr(a, STR("window"));
        a->style.sizes[blu_axis_X] = { blu_sizeKind_PX, 250 };
        a->style.sizes[blu_axis_Y] = { blu_sizeKind_PX, 250 };
        a->style.childLayoutAxis = blu_axis_Y;

        blu_parentScope(a) {
            blu_styleScope {
            blu_style_add_sizeX({ blu_sizeKind_TEXT, 0 });
            blu_style_add_sizeY({ blu_sizeKind_TEXT, 0 });
            blu_style_add_backgroundColor(col_darkGray);

                a = blu_areaMake(STR("titlebar"), blu_areaFlags_DRAW_BACKGROUND | blu_areaFlags_CLICKABLE | blu_areaFlags_DRAW_TEXT);
                blu_areaAddDisplayStr(a, STR("titlebar"));
                a->style.sizes[blu_axis_X] = { blu_sizeKind_PERCENT, 1 };
                demoGlobs.windowPos += blu_interactionFromWidget(a).dragDelta;
                blu_parentScope(a) {

                    a = blu_areaMake(STR("space"), 0);
                    a->style.sizes[blu_axis_X].kind = blu_sizeKind_REMAINDER;

                    blu_styleScope {
                    blu_style_add_backgroundColor(col_darkGray);
                    blu_style_add_textColor(col_white);
                        U32 stdSize = 30;
                        U32 wideSize = 40;

                        demo_makeFancyButton(STR("-"), stdSize, wideSize, col_white, col_darkBlue);
                        demo_makeFancyButton(STR("O"), stdSize, wideSize, col_white, col_darkBlue);
                        demo_makeFancyButton(STR("X"), stdSize, wideSize, col_red, col_darkBlue);
                    }
                } // end title bar


            }
        } // end window
        win->offset = demoGlobs.windowPos;
        //*/
    } // end main style
}

void demo_updateScene(float dt) {

}


#endif