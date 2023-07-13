#pragma once
#include "base/allocators.h"
#include "base/str.h"
#include "base/utils.h"



struct gfx_UniformBlock;
struct gfx_Shader;


enum gfx_TexPxType {
    gfx_texPxType_RGBA8,
    gfx_texPxType_R8
};
// NOTE: remember to mirror with texture loads

struct gfx_Texture {
    U32 id = 0;
    U8* data = nullptr;

    int width = 0;
    int height = 0;

    gfx_TexPxType pixelLayout;
};




enum gfx_VType {
    gfx_vtype_NONE,
    gfx_vtype_POS2F_UV,
    gfx_vtype_POS3F_UV
};

struct gfx_Pass; // >:(
typedef void (*gfx_ShaderUniformBindFunc)(gfx_Pass* pass, gfx_UniformBlock* uniforms);
struct gfx_Shader {
    U32 id = 0;
    gfx_VType layout = gfx_vtype_NONE;
    gfx_ShaderUniformBindFunc uniformBindFunc = nullptr;
    gfx_ShaderUniformBindFunc passUniformBindFunc = nullptr;
};

struct gfx_IndexBuffer {
    U32 id = 0;
    U32 count = 0;
    U32* data = nullptr;
};

struct gfx_VertexArray {
    U32 id = 0;
    U32 vbId = 0;

    gfx_VType layoutType = gfx_vtype_NONE;
    // U32 count = 0;
    void* data = nullptr;
};


// NOTE: consider different formats for framebuffers
// RN rgba and depth attachments are assumed
struct gfx_Framebuffer {
    gfx_Texture* texture;

    U32 fbId = 0;
    U32 depthId = 0;
};





// CLEANUP: better name?
struct gfx_UniformBlock {

    // CALL UNIS =========================

    V2f dstStart = V2f();
    V2f dstEnd = V2f();

    V2f srcStart = V2f();
    V2f srcEnd = V2f(1, 1);

    V2f clipStart = V2f(0, 0);
    V2f clipEnd = V2f();

    gfx_Texture* texture = nullptr;
    gfx_Texture* fontTexture = nullptr;
    V4f color = V4f(0, 0, 0, 1);

    Mat4f model = Mat4f(1);

    gfx_VertexArray* va = nullptr;
    gfx_IndexBuffer* ib = nullptr;

    // PASS UNIS ========================

    Mat4f vp = Mat4f(1);

    //

    gfx_UniformBlock* _next;
};
// NOTE: consider making into a union
// NOTE: figure out a way to make this work without editing gfx src.


// CLEANUP: make public members clearer
// CLEANUP: all ctors or no?
// target == nullptr indicates drawing to the screen
struct gfx_Pass {

    gfx_VertexArray* curVa;
    gfx_IndexBuffer* curIb;


    bool isClearPass = false; // TODO: assert fail on adding calls to clear passes / refactor

    gfx_Framebuffer* target = nullptr;
    gfx_Shader* shader = nullptr;
    gfx_UniformBlock passUniforms = gfx_UniformBlock();

    gfx_UniformBlock* startCall = nullptr;
    gfx_UniformBlock* endCall = nullptr;
};






void gfx_init();
// CLEANUP: this
gfx_VertexArray* gfx_getQuadVA();
gfx_IndexBuffer* gfx_getQuadIB();

gfx_Shader* gfx_registerShader(gfx_VType vertLayout, const char* vertPath, const char* fragPath, BumpAlloc* scratch);
gfx_Texture* gfx_registerTexture(U8* data, int width, int height, gfx_TexPxType pixelLayout);

// TODO: multisample/antialias framebuffers
gfx_Framebuffer* gfx_registerFramebuffer();

gfx_VertexArray* gfx_registerVertexArray(gfx_VType layout, void* data, U32 dataSize, bool dynamic);
gfx_IndexBuffer* gfx_registerIndexBuffer(U32* data, U32 dataCount);

void gfx_resizeFramebuffer(gfx_Framebuffer* fb, int nw, int nh);

// NOTE: use these for binding VAs/IBs otherwise it'll shit itself
// NOTE: VERTEX ARRAY GOES FIRST DONT ASK
void gfx_bindVertexArray(gfx_Pass* pass, gfx_VertexArray* va);
void gfx_bindIndexBuffer(gfx_Pass* pass, gfx_IndexBuffer* ib);

gfx_Pass* gfx_registerPass();

// CLEANUP: this?
// target = nullptr indicates drawing to screen
gfx_Pass* gfx_registerClearPass(V4f color, gfx_Framebuffer* target);

gfx_UniformBlock* gfx_registerCall(gfx_Pass* pass);

// draws all passes, clears passes and calls after.
void gfx_drawPasses(U32 scWidth, U32 scHeight);


#ifdef GFX_IMPL

#include "base/arr.h"
#include "GLAD/gl.h"
#include "stb_image/stb_image.h"
#include <stdio.h>

#define MAX_DRAW_CALL_COUNT 2048
#define MAX_PASS_COUNT 16
#define RES_SIZE 1000000


struct gfx_Globs {

    BumpAlloc resArena = BumpAlloc();

    BumpAlloc passArena = BumpAlloc();
    gfx_Pass* passes = nullptr;
    U32 passCount = 0;
    // TODO: move passes to arenas outside of gfx

    gfx_IndexBuffer* quadIb = nullptr;
    gfx_VertexArray* quadVa = nullptr;
};
static gfx_Globs globs = gfx_Globs();


// NOTE: requires opengl already setup
void gfx_init() {

    bump_allocate(&globs.resArena, RES_SIZE);
    bump_allocate(&globs.passArena, MAX_PASS_COUNT * sizeof(gfx_Pass) + MAX_DRAW_CALL_COUNT * sizeof(gfx_UniformBlock));

    // CLEANUP: consider begin_frame func
    // preps passes to start
    globs.passes = BUMP_PUSH_ARR(&globs.passArena, MAX_PASS_COUNT, gfx_Pass);
    globs.passCount = 0;



    U32 ibData[] = {
        0, 1, 2,
        2, 3, 0
    };
    globs.quadIb = gfx_registerIndexBuffer(ibData, sizeof(ibData) / sizeof(U32));

    F32 vbData[] = {
        -1, -1,    0, 0, // BL
        -1,  1,    0, 1, // UL
        1,  1,    1, 1, // UR
        1, -1,    1, 0  // BR
    };
    globs.quadVa = gfx_registerVertexArray(gfx_vtype_POS2F_UV, vbData, sizeof(vbData), false);
}


gfx_VertexArray* gfx_getQuadVA() { return globs.quadVa; }
gfx_IndexBuffer* gfx_getQuadIB() { return globs.quadIb; }


gfx_Shader* gfx_registerShader(gfx_VType vertLayout, const char* vertPath, const char* fragPath, BumpAlloc* scratch) {
    int err = 0;


    U64 vertSize;
    U8* vertSrc = loadFileToBuffer(vertPath, true, &vertSize, scratch);
    if(!vertSrc) {
        printf("Shader source file at \"%s\" could not be found\n", vertPath);
        return nullptr;
    }

    U64 fragSize;
    U8* fragSrc = loadFileToBuffer(fragPath, true, &fragSize, scratch);
    if(!fragSrc) {
        printf("Shader source file at \"%s\" could not be found\n", fragPath);
        return nullptr;
    }

    //compile src into shader code
    U32 v = glCreateShader(GL_VERTEX_SHADER);
    int smallVertSize = (int)vertSize;
    glShaderSource(v, 1, (const char* const*)&vertSrc, &smallVertSize);
    glCompileShader(v);

    U32 f = glCreateShader(GL_FRAGMENT_SHADER);
    int smallFragSize = (int)fragSize; // CLEANUP: these could overflow
    glShaderSource(f, 1, (const char* const*)&fragSrc, &smallFragSize);
    glCompileShader(f);



    bool compFailed = false;
    char* logBuffer = BUMP_PUSH_ARR(scratch, 512, char);

    glGetShaderiv(v, GL_COMPILE_STATUS, &err);
    if(!err) {
        glGetShaderInfoLog(v, 512, NULL, logBuffer);
        printf("Compiling shader \"%s\" failied: %s", vertPath, logBuffer);
        compFailed = true;
    };

    glGetShaderiv(f, GL_COMPILE_STATUS, &err);
    if(!err) {
        glGetShaderInfoLog(f, 512, NULL, logBuffer);
        printf("Compiling shader \"%s\" failied: %s", fragPath, logBuffer);
        compFailed = true;
    };


    gfx_Shader* s = nullptr;
    if(!compFailed) {
        s = BUMP_PUSH_NEW(&globs.resArena, gfx_Shader);

        s->id = glCreateProgram();
        glAttachShader(s->id, v);
        glAttachShader(s->id, f);

        glLinkProgram(s->id);
        glValidateProgram(s->id);

        s->layout = vertLayout;
    }

    glDeleteShader(v);
    glDeleteShader(f);
    return s;
}


gfx_Texture* gfx_registerTexture(U8* data, int width, int height, gfx_TexPxType pixelLayout) {


    gfx_Texture* t = BUMP_PUSH_NEW(&globs.resArena, gfx_Texture);
    t->data = data;
    t->width = width;
    t->height = height;
    t->pixelLayout = pixelLayout;

    glGenTextures(1, &t->id);
    glBindTexture(GL_TEXTURE_2D, t->id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // NOTE: for pixely looks again, use -> GL_NEAREST as the mag/min filter


    if (pixelLayout == gfx_texPxType_RGBA8) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, t->width, t->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, t->data); }
    else if (pixelLayout == gfx_texPxType_R8) {
        // They lied??????????????????????????
        // glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // disable byte-alignment restriction, who knows what thisl do
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, t->width, t->height, 0, GL_RED, GL_UNSIGNED_BYTE, t->data); }
    else {
        ASSERT("INVALID PIXEL LAYOUT ON TEX LOAD" == ""); }

    return t;
}

gfx_Framebuffer* gfx_registerFramebuffer() {

    gfx_Framebuffer* f = BUMP_PUSH_NEW(&globs.resArena, gfx_Framebuffer);

    glGenFramebuffers(1, &f->fbId);
    glBindFramebuffer(GL_FRAMEBUFFER, f->fbId);

    f->texture = gfx_registerTexture(nullptr, 1, 1, gfx_texPxType_RGBA8);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, f->texture->id);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, f->texture->id, 0);

    glGenRenderbuffers(1, &f->depthId);
    glBindRenderbuffer(GL_RENDERBUFFER, f->depthId);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, 1, 1);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, f->depthId);

    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        printf("Framebuffer creation failed.");
        ASSERT(false);
    }

    return f;
}

// resizes texture and depth components GPU side
void gfx_resizeFramebuffer(gfx_Framebuffer* fb, int nw, int nh) {

    glBindTexture(GL_TEXTURE_2D, fb->texture->id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, nw, nh, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    glBindRenderbuffer(GL_RENDERBUFFER, fb->depthId);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, nw, nh);

    fb->texture->width = nw;
    fb->texture->height = nh;
}



// TODO: gl out of memory errs

gfx_VertexArray* gfx_registerVertexArray(gfx_VType layout, void* data, U32 dataSize, bool dynamic) {

    gfx_VertexArray* va = BUMP_PUSH_NEW(&globs.resArena, gfx_VertexArray);
    *va = gfx_VertexArray();

    // VERTEX BUFFER AND ARR ====================================================
    {
        glGenVertexArrays(1, &va->id);
        glBindVertexArray(va->id);

        glGenBuffers(1, &va->vbId);
        glBindBuffer(GL_ARRAY_BUFFER, va->vbId);

        va->data = data;
        glBufferData(GL_ARRAY_BUFFER, dataSize, data, dynamic? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);

    }

    // ATTRIBS ================================================================
    {
        if(layout == gfx_vtype_POS2F_UV) {
            va->layoutType = gfx_vtype_POS2F_UV;
            U32 vertSize = 4 * sizeof(F32);
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, vertSize, (void*)(0));
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, vertSize, (void*)(2 * sizeof(F32)));
            glEnableVertexAttribArray(1);
        }
        else if(layout == gfx_vtype_POS3F_UV) {
            va->layoutType = gfx_vtype_POS3F_UV;
            U32 vertSize = 5 * sizeof(F32);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, vertSize, (void*)(0));
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, vertSize, (void*)(3 * sizeof(F32)));
            glEnableVertexAttribArray(1);
        }
        else { ASSERT(false); }

    }
    glBindVertexArray(0);

    return va;
}

gfx_IndexBuffer* gfx_registerIndexBuffer(U32* data, U32 dataCount) {
    gfx_IndexBuffer* ib = BUMP_PUSH_NEW(&globs.resArena, gfx_IndexBuffer);
    *ib = gfx_IndexBuffer();


    glGenBuffers(1, &ib->id);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ib->id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, dataCount * sizeof(U32), data, GL_STATIC_DRAW);
    ib->data = data;
    ib->count = dataCount;

    return ib;
}

// CLEANUP: make binding more consistent
void gfx_bindVertexArray(gfx_Pass* pass, gfx_VertexArray* va) {
    ASSERT(va);
    ASSERT(va->layoutType == pass->shader->layout);
    glBindVertexArray(va->id);
    pass->curVa = va;
}
void gfx_bindIndexBuffer(gfx_Pass* pass, gfx_IndexBuffer* ib) {
    ASSERT(ib);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ib->id);
    pass->curIb = ib;
}






// Begins new pass
gfx_Pass* gfx_registerPass() {
    ASSERT(globs.passCount < MAX_PASS_COUNT - 1);
    gfx_Pass* p = ARR_APPEND(globs.passes, globs.passCount, gfx_Pass());
    *p = gfx_Pass();
    return p;
}

gfx_Pass* gfx_registerClearPass(V4f color, gfx_Framebuffer* target) {
    gfx_Pass* out = gfx_registerPass();
    out->isClearPass = true;
    out->target = target;
    out->passUniforms.color = color;
    return out;
}



// Adds and returns new call within the current pass
gfx_UniformBlock* gfx_registerCall(gfx_Pass* pass) {
    ASSERT(pass);

    gfx_UniformBlock* block = BUMP_PUSH_NEW(&globs.passArena, gfx_UniformBlock);
    *block = gfx_UniformBlock();
    block->_next = nullptr;

    if(!pass->startCall) {
        pass->startCall = block;
        pass->endCall = block;
    }
    else {
        pass->endCall->_next = block;
        pass->endCall = block;
    }

    return block;
}




// Draws all passes,
// Clears after
void gfx_drawPasses(U32 scWidth, U32 scHeight) {

    for(int i = 0; i < globs.passCount; i++) {
        gfx_Pass* pass = &globs.passes[i];


        if(pass->target) {
            glBindFramebuffer(GL_FRAMEBUFFER, pass->target->fbId);

            glViewport(0, 0,
            pass->target->texture->width,
            pass->target->texture->height);
        }
        else {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glViewport(0, 0, scWidth, scHeight);
        }

        if(pass->isClearPass) {
            glClearColor(
                pass->passUniforms.color.x,
                pass->passUniforms.color.y,
                pass->passUniforms.color.z,
                pass->passUniforms.color.w);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        }
        else {
            ASSERT(pass->shader);
            glUseProgram(pass->shader->id);
            ASSERT(pass->shader->passUniformBindFunc);
            pass->shader->passUniformBindFunc(pass, &pass->passUniforms);

            gfx_UniformBlock* curBlock = pass->startCall;
            while(curBlock) {

                ASSERT(pass->shader->uniformBindFunc);
                pass->shader->uniformBindFunc(pass, curBlock);

                ASSERT(pass->curIb);
                ASSERT(pass->curVa);

                glDrawElements(GL_TRIANGLES, pass->curIb->count, GL_UNSIGNED_INT, nullptr);

                curBlock = curBlock->_next;
            }
        }
    }

    // reset passes and calls for next frame
    globs.passCount = 0;
    bump_clear(&globs.passArena);
    globs.passes = BUMP_PUSH_ARR(&globs.passArena, MAX_PASS_COUNT, gfx_Pass);

    glBindVertexArray(0);
}


#endif
