
#include <stdio.h>
#include <math.h>

#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"
#include "stb_image/stb_image.h"
#include "stb_truetype/stb_truetype.h"
#include "GLAD/gl.h"

#include "base/arr.h"
#include "base/utils.h"
#include "graphics.h"
#include "blue/blue.h"
#include "network/network.h"

#include "colors.h"




// NOTE: I didn't want to put it in gfx because of the stb dependency
gfx_Texture* loadTextureFromFile(const char* path) {

    // CLEANUP: image data is performing an allocaiton that I don't like
    stbi_set_flip_vertically_on_load(1); // NOTE: opengl specific
    S32 bpp, w, h;
    U8* data = stbi_load(path, &w, &h, &bpp, 4);
    ASSERT(data);

    return gfx_registerTexture(data, w, h, gfx_texPxType_RGBA8);
}


float windowScrollDelta = 0;
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    windowScrollDelta -= (F32)yoffset;
}


int main() {

    BumpAlloc lifetimeArena;
    BumpAlloc frameArena;
    bump_allocate(&lifetimeArena, 1000000);
    bump_allocate(&frameArena, 1000000);


    GLFWwindow* window = nullptr;
    GLFWcursor* hoverCursor = nullptr;
    // GLFW INIT AND WINDOW CREATION ===============================================================================
    {
        assert(glfwInit());
        glfwSetErrorCallback([](int error, const char* description) { printf("%s\n", description); });

        glfwWindowHint(GLFW_MAXIMIZED, true);
        glfwWindowHint(GLFW_RESIZABLE, true);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
        glfwWindowHint(GLFW_SAMPLES, 4);

        window = glfwCreateWindow(512, 512, "Sundial", NULL, NULL);
        ASSERT(window);

        glfwMakeContextCurrent(window);
        glfwSwapInterval(1);


        hoverCursor = glfwCreateStandardCursor(GLFW_HAND_CURSOR);
        glfwSetScrollCallback(window, scroll_callback);
        // glfwSetKeyCallback(window, updateInput);
        // glfwSetCursorPosCallback(window, updateMousePos);
        // glfwSetMouseButtonCallback(window, updateMouseButton);
    }

    // OPENGL/GLAD INIT
    {
        gladLoadGL(glfwGetProcAddress);
        glLoadIdentity();

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glDepthFunc(GL_LESS | GL_EQUAL);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glEnable(GL_MULTISAMPLE);

        glEnable(GL_DEBUG_OUTPUT);
        glDebugMessageCallback(
        [](GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const char* message, const void* userParam) {
            printf("%s\n", message);
        }, 0);
    }

    gfx_init();



    gfx_Texture* solidTex = loadTextureFromFile("res/textures/solid.png");
    gfx_Texture* testTex = loadTextureFromFile("res/textures/helloTex.png");
    blu_init(solidTex);
    blu_loadFont("C:/windows/fonts/consola.ttf");


    net_init();


    gfx_Shader* blueShader;
    {
        blueShader = gfx_registerShader(gfx_vtype_POS2F_UV, "res/shaders/blue.vert", "res/shaders/blue.frag", &frameArena);

        // TODO: remove lambdas
        blueShader->passUniformBindFunc = [](gfx_Shader* shader, gfx_UniformBlock* uniforms) {
            int loc;

            loc = glGetUniformLocation(shader->id, "uVP");
            glUniformMatrix4fv(loc, 1, false, &(uniforms->vp)[0]);
        };

        blueShader->uniformBindFunc = [](gfx_Shader* shader, gfx_UniformBlock* uniforms) {
            int loc;

            loc = glGetUniformLocation(shader->id, "uDstStart");
            glUniform2f(loc, uniforms->dstStart.x, uniforms->dstStart.y);
            loc = glGetUniformLocation(shader->id, "uDstEnd");
            glUniform2f(loc, uniforms->dstEnd.x, uniforms->dstEnd.y);

            loc = glGetUniformLocation(shader->id, "uSrcStart");
            glUniform2f(loc, uniforms->srcStart.x, uniforms->srcStart.y);
            loc = glGetUniformLocation(shader->id, "uSrcEnd");
            glUniform2f(loc, uniforms->srcEnd.x, uniforms->srcEnd.y);

            loc = glGetUniformLocation(shader->id, "uColor");
            glUniform4f(loc, uniforms->color.x, uniforms->color.y, uniforms->color.z, uniforms->color.w);

            loc = glGetUniformLocation(shader->id, "uTexture");
            glUniform1i(loc, 0);
            glActiveTexture(GL_TEXTURE0 + 0);
            glBindTexture(GL_TEXTURE_2D, uniforms->texture->id);

            loc = glGetUniformLocation(shader->id, "uFontTexture");
            glUniform1i(loc, 1);
            glActiveTexture(GL_TEXTURE0 + 1);
            glBindTexture(GL_TEXTURE_2D, uniforms->fontTexture->id);

            // CLEANUP: better texture abstraction
        };
    }




    V2f windowPos = V2f(350, 100);
    float clipPos = 0;
    float clipMax = 1400;
    float clipSize = 0;

    F64 prevTime = glfwGetTime();
    while(!glfwWindowShouldClose(window)) {
        F64 time = glfwGetTime();
        F64 dt = time - prevTime;
        prevTime = time;

        net_update();


        // UI ///////////////////////////////////////////////////////////////////////
        {
            F64 mx, my;
            glfwGetCursorPos(window, &mx, &my);
            bool leftPressed = (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)? true : false;

            blu_beginFrame();


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
                clipSize = a->calculatedSizes[blu_axis_Y];
                blu_areaAddDisplayStr(a, STR("left"));
                a->style.sizes[blu_axis_X] = { blu_sizeKind_PX, 300 };
                a->style.childLayoutAxis = blu_axis_X;
                blu_parentScope(a) {

                    a = blu_areaMake(STR("clip"), blu_areaFlags_VIEW_OFFSET | blu_areaFlags_CLICKABLE);
                    blu_Area* clip = a;
                    blu_areaAddDisplayStr(a, STR("blip"));
                    a->style.sizes[blu_axis_X] = { blu_sizeKind_REMAINDER, 0 };
                    a->style.childLayoutAxis = blu_axis_Y;
                    clipPos += blu_interactionFromWidget(a).scrollDelta * 40;
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
                    if(clipMax > clipSize) {
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
                            a->style.sizes[blu_axis_Y] = { blu_sizeKind_PERCENT, clipSize / clipMax };
                            clipPos += blu_interactionFromWidget(a).dragDelta.y / clipSize * clipMax;

                        }
                    } else { clipPos = 0; }


                    clipPos = max(clipPos, 0);
                    clipPos = min(clipPos, clipMax - (clipSize));
                    clip->viewOffset = { 0, clipPos };

                    if(spacer) {
                        spacer->style.sizes[blu_axis_Y] = { blu_sizeKind_PX, (clipPos / clipMax) * clipSize };
                    }
                } // end left



                //*
                a = blu_areaMake(STR("window"),
                        blu_areaFlags_DRAW_BACKGROUND |
                        blu_areaFlags_DRAW_TEXT |
                        blu_areaFlags_FLOATING |
                        blu_areaFlags_CLICKABLE);

                blu_Area* win = a;
                blu_areaAddDisplayStr(a, STR("window"));
                a->style.sizes[blu_axis_X] = { blu_sizeKind_PX, 350 };
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
                        windowPos += blu_interactionFromWidget(a).dragDelta;
                        blu_parentScope(a) {

                            a = blu_areaMake(STR("space"), 0);
                            a->style.sizes[blu_axis_X].kind = blu_sizeKind_REMAINDER;


                            blu_styleScope {
                                U32 stdSize = 30;
                                U32 wideSize = 40;

                                a = blu_areaMake(STR("b1"), blu_areaFlags_DRAW_BACKGROUND | blu_areaFlags_DRAW_TEXT | blu_areaFlags_HOVER_ANIM | blu_areaFlags_CLICKABLE);
                                blu_areaAddDisplayStr(a, STR("AAAAA"));
                                a->style.backgroundColor = v4f_lerp(a->style.backgroundColor, col_white, a->target_hoverAnim);
                                a->style.textColor = v4f_lerp(a->style.textColor, col_darkBlue, a->target_hoverAnim);
                                a->style.sizes[blu_axis_X] = { blu_sizeKind_PX, lerp(stdSize, wideSize, a->target_hoverAnim) };

                                a = blu_areaMake(STR("b2"), blu_areaFlags_DRAW_BACKGROUND | blu_areaFlags_DRAW_TEXT | blu_areaFlags_HOVER_ANIM | blu_areaFlags_CLICKABLE);
                                blu_areaAddDisplayStr(a, STR("O"));
                                a->style.backgroundColor = v4f_lerp(a->style.backgroundColor, col_white, a->target_hoverAnim);
                                a->style.textColor = v4f_lerp(a->style.textColor, col_darkBlue, a->target_hoverAnim);
                                a->style.sizes[blu_axis_X] = { blu_sizeKind_PX, lerp(stdSize, wideSize, a->target_hoverAnim) };

                                a = blu_areaMake(STR("b3"), blu_areaFlags_DRAW_BACKGROUND | blu_areaFlags_DRAW_TEXT | blu_areaFlags_HOVER_ANIM | blu_areaFlags_CLICKABLE);
                                blu_areaAddDisplayStr(a, STR("X"));
                                a->style.backgroundColor = v4f_lerp(a->style.backgroundColor, col_red, a->target_hoverAnim);
                                a->style.textColor = v4f_lerp(a->style.textColor, col_darkBlue, a->target_hoverAnim);
                                a->style.sizes[blu_axis_X] = { blu_sizeKind_PX, lerp(stdSize, wideSize, a->target_hoverAnim) };
                            }
                        } // end title bar


                    }
                } // end window
                win->offset = windowPos;
                //*/





            } // end main style


            blu_input(V2f((F32)mx, (F32)my), leftPressed, windowScrollDelta);
            windowScrollDelta = 0;
        } // end UI




        // RENDER UI //////////////////////////////////////////////////////////////////////////
        {
            int w, h;
            glfwGetFramebufferSize(window, &w, &h);
            Mat4f vp;
            matrixOrtho(0, w, h, 0, 0.0001, 10000, vp);

            glViewport(0, 0, w, h);
            blu_layout(V2f(w, h));

            gfx_Pass* p = gfx_registerPass();
            p->shader = blueShader;
            p->passUniforms = gfx_UniformBlock();
            p->passUniforms.vp = vp;

            blu_createPass(p);
        }

        gfx_clear(V4f(0, 0, 0, 1));
        gfx_drawPasses();

        bump_clear(&frameArena);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }


    net_cleanup();
    glfwDestroyWindow(window);
    glfwTerminate();

    // getchar();
}
