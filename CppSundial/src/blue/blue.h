#pragma once

#include "utils.h"
#include "base/config.h"
#include "base/str.h"
#include "base/allocators.h"
#include "graphics.h"



/*
THE PLAN: build an immediate mode UI library that is as portable as as possible, for use in personal projects

THE CHECKLIST:

[X] core/systems:
    [X] hashmap/basic data utils header/lib
    [X] figure out the build system
    [X] custom stirngs apperantly

[ ] the library:
    [X] frame correlation system
    [X] render system
    [X] layout
        [X] remainder
        [X] floating
    [X] themeing system that is slick af
    [X] fonts????
        [X] text center
    [X] input
        [X] ordering
    [X] custom textures
    [X] animations
    [ ] scrolling
    [ ] clipping
    [ ] tooltips/dropdowns
    [ ] text input

    [ ] hover cursor change
    [ ] rounding
    [ ] borders
    [ ] drop shadows
    [ ] batching
    [ ] cleanup
        [ ] hide engine guts

TESTING CHECKLIST:
[ ] test hashmap collsions
[ ] test out of memory things
[ ] make a fuzzing thing
*/


enum blu_Axis{
    blu_axis_X,
    blu_axis_Y,
    blu_axis_COUNT
};

enum blu_SizeKind {
    blu_sizeKind_NONE,
    blu_sizeKind_PX, // pixel count
    blu_sizeKind_PERCENT, // percent of parent
    blu_sizeKind_TEXT, // large enough to fit text
    blu_sizeKind_REMAINDER,
};

enum blu_AreaFlags {
    blu_areaFlags_NONE =            (0 << 0),
    blu_areaFlags_DRAW_BACKGROUND = (1 << 0),
    blu_areaFlags_DRAW_TEXT =       (1 << 1),
    blu_areaFlags_DRAW_TEXTURE =    (1 << 2),
    blu_areaFlags_FLOATING =        (1 << 3),
    blu_areaFlags_HOVER_ANIM =      (1 << 4),
    blu_areaFlags_CENTER_TEXT =     (1 << 5),
    blu_areaFlags_CLICKABLE =       (1 << 6),
};

struct blu_Size {
    blu_SizeKind kind = blu_sizeKind_NONE;
    F32 value = 0;
};




// NOTE:
// when editing, be sure to update
//  - flags enum
//  - style struct
//  - add functions
//  - add function header
//  - style build code in areaMake
// gad damn this language

enum blu_StyleFlags {
    blu_styleFlags_none             = (0 << 0),
    blu_styleFlags_childLayoutAxis  = (1 << 1),
    blu_styleFlags_sizeX            = (1 << 2),
    blu_styleFlags_sizeY            = (1 << 3),
    blu_styleFlags_backgroundColor  = (1 << 4),
    blu_styleFlags_textColor        = (1 << 5),
    blu_styleFlags_textPadding      = (1 << 6),
    blu_styleFlags_animationStrength= (1 << 7),
};

struct blu_Style {
    U32 overrideFlags = 0; // NOTE: override flags don't do shit inside the areas, they are only used to construct the final styles

    blu_Axis childLayoutAxis = blu_axis_X;
    blu_Size sizes[blu_axis_COUNT] = { { blu_sizeKind_NONE, 0 }, { blu_sizeKind_NONE, 0 } };
    V4f backgroundColor = V4f();
    V4f textColor = V4f();
    V2f textPadding = V2f();
    F32 animationStrength = 0;
};
struct blu_StyleStackNode {
    blu_Style data = blu_Style();
    blu_StyleStackNode* next;
    blu_StyleStackNode* prev;
};




struct blu_Area {

    // free list
    blu_Area* nextFree = nullptr;

    // tree links, [2] means double buffered
    // TODO: replace double buffer/ benchmark vs a copy version
    // TODO: better way of iterating over all elements
    blu_Area* parent = nullptr;
    blu_Area* firstChild[2] = { 0 };
    blu_Area* lastChild = nullptr;
    blu_Area* nextSibling[2] = { 0 };
    blu_Area* prevSibling = nullptr;


    // hashing
    blu_Area* hashNext;
    U64 hashKey; // TODO: parent based hashing/salting/whatever

    U64 lastTouchedIdx = 0;


    // builder provided
    U32 flags = 0;
    str displayString = { nullptr, 0 };

    blu_Style style;

    gfx_Texture* texture = nullptr;

    V2f offset = { 0, 0 };

    // layout pass data
    F32 calculatedSizes[blu_axis_COUNT];
    F32 calculatedOffsets[blu_axis_COUNT];
    F32 calculatedPosition[blu_axis_COUNT];


    // persistant shit for input / anim
    F32 target_hoverAnim = 1;


    // input
    bool prevHovered = false;
    bool prevPressed = false;
};


// TODO: name?
struct blu_WidgetInputs {
    bool hovered = false;
    bool held = false;
    bool clicked = false;

    bool dragged = false;
    V2f dragDelta = V2f();
};




void blu_init(gfx_Texture* solidTex);
void blu_loadFont(const char* path);


blu_Area* blu_areaMake(str string, U32 flags);
void blu_areaAddDisplayStr(blu_Area* area, str s);

void blu_pushParent(blu_Area* parent);
void blu_popParent();

void blu_beginFrame(); // cull
void blu_input(V2f npos, bool lmbState); // set current and update prev input // CLEANUP: merge with begin?
void blu_layout(V2f scSize); // calculate layout shit
void blu_createPass(gfx_Pass* normalPass);


void blu_style_add_childLayoutAxis(blu_Axis axis);
void blu_style_add_sizeX(blu_Size size);
void blu_style_add_sizeY(blu_Size size);
void blu_style_add_backgroundColor(V4f color);
void blu_style_add_textColor(V4f color);
void blu_style_add_textPadding(V2f padding);
void blu_style_add_animationStrength(F32 s);

void blu_pushStyle();
void blu_popStyle();

blu_WidgetInputs blu_interactionFromWidget(blu_Area* area);



#define blu_deferLoop(begin, end) for(int _i_ = ((begin), 0); !_i_; _i_ += 1, (end))
#define blu_styleScope blu_deferLoop(blu_pushStyle(), blu_popStyle())
#define blu_parentScope(parent) blu_deferLoop(blu_pushParent(parent), blu_popParent())


#ifdef BLU_IMPL

#include "base/hashtable.h"
#include "base/arr.h"
#include "stb_truetype/stb_truetype.h"


// CLEANUP: these
#define BLU_FONT_SIZE 20 // in px
#define BLU_FONT_FIRST 32 // fist char in font, 32 = space
#define BLU_FONT_CHARCOUNT 96


#define BLU_AREA_HASH_COUNT 10000
#define BLU_MAX_AREA_COUNT 10000
#define BLU_MAX_ARENA_SIZE 1000000



// NOTE: units in pixels, except the UVs
// metrics are up positive
struct blu_Glyph {
    V2f startUV;
    V2f endUV;

    float width;
    float height;

    float xBearing;
    float yBearing;

    float advance;
};


struct blu_Globs {

    blu_Area* currentParent = nullptr; // TODO: replace with currentArea
    blu_Area* ogParent = nullptr;

    blu_Area** hash = nullptr; // array of pointers to the actual area structs, for hash-based access

    BumpAlloc areaArena = { };
    blu_Area* firstFree = nullptr;

    BumpAlloc frameArena = { };

    blu_StyleStackNode* currentStyle = nullptr; // TODO: (?) make it so current style doesn't have to be rebuilt per area
    blu_StyleStackNode* ogStyle = nullptr;

    U64 frameIndex = 0;
    bool linkSide = 0;


    gfx_Texture* solidTex = 0;


    gfx_Texture* fontTex = 0;
    float fontAscent = 0;
    float fontDecent = 0;
    float fontLinegap = 0;
    float fontHeight = 0;

    blu_Glyph fontGlyphs[BLU_FONT_CHARCOUNT];



    V2f inputMousePos = V2f();
    bool inputCurLButton = false;
    bool inputPrevLButton = false;
    blu_Area* dragged = nullptr;
    V2f dragDelta = V2f();
};

static blu_Globs globs = blu_Globs();


// CLEANUP: gen solidtex at startup
void blu_init(gfx_Texture* solidTex) {

    globs.solidTex = solidTex;

    bump_allocate(globs.areaArena, BLU_MAX_AREA_COUNT * sizeof(blu_Area));
    bump_allocate(globs.frameArena, BLU_MAX_ARENA_SIZE);
    globs.hash = (blu_Area**)arr_allocate0(sizeof(blu_Area*), BLU_AREA_HASH_COUNT);
}

void blu_loadFont(const char* path) {

    FILE* fontFile = fopen(path, "rb");
    fseek(fontFile, 0, SEEK_END);
    U64 size = ftell(fontFile); /* how long is the file ? */
    fseek(fontFile, 0, SEEK_SET); /* reset */
    U8* fontBuffer = BUMP_PUSH_ARR(globs.frameArena, size, U8);
    fread(fontBuffer, size, 1, fontFile);
    fclose(fontFile);

    const U32 texSize = 512;
    U8* imgbuf = BUMP_PUSH_ARR(globs.frameArena, texSize*texSize, U8);
    stbtt_bakedchar glyphInfo[BLU_FONT_CHARCOUNT];

    // TODO: put stbtt allocations into arenas
    // TODO: get rid of this lol
    stbtt_BakeFontBitmap(fontBuffer, 0, BLU_FONT_SIZE, imgbuf, texSize, texSize, 32, BLU_FONT_CHARCOUNT, glyphInfo); // no guarantee this fits!
    globs.fontTex = gfx_registerTexture(imgbuf, texSize, texSize, gfx_texPxType_R8);

    stbtt_fontinfo fInfo;
    stbtt_InitFont(&fInfo, fontBuffer, 0);

    int a, d, l;
    stbtt_GetFontVMetrics(&fInfo, &a, &d, &l);
    float scale = stbtt_ScaleForPixelHeight(&fInfo, BLU_FONT_SIZE);
    globs.fontAscent = a * scale;
    globs.fontDecent = -d * scale; // note ascent and decent are both distances that are positive
    globs.fontLinegap = l * scale;
    globs.fontHeight = globs.fontAscent + globs.fontDecent;


    for(int i = 0; i < BLU_FONT_CHARCOUNT; i++) {
        blu_Glyph* g = &globs.fontGlyphs[i];
        stbtt_bakedchar* info = &glyphInfo[i];

        g->startUV = V2f(info->x0, info->y0) / texSize;
        g->endUV = V2f(info->x1, info->y1) / texSize;
        g->width = info->x1 - info->x0;
        g->height = info->y1 - info->y0;
        g->xBearing = info->xoff;
        g->yBearing = -info->yoff; // STB uses down-positive metrics
        g->advance = info->xadvance;
    }


    bump_clear(globs.frameArena);
}




// NOTE: does nothing if element at key already exists
void blu_hashInsert(U64 key, blu_Area* a) {

    U64 idx = key % BLU_AREA_HASH_COUNT;
    blu_Area* elem = globs.hash[idx];

    if(elem) { // remember the shit about a default ptr so no segfualt
        while(elem->hashNext) {
            if(elem->hashKey == key) { return; }
            elem = elem->hashNext;
        }
        elem->hashNext = a;
    }
    else {
        globs.hash[idx] = a; }
}

blu_Area* blu_hashGet(U64 key) {

    blu_Area* elem = globs.hash[key % BLU_AREA_HASH_COUNT];
    while(true) {
        if(elem == nullptr) { return nullptr; }
        if(elem->hashKey == key) { return elem; }

        elem = elem->hashNext;
    }

}

void blu_hashRemove(U64 key) {

    blu_Area** nextPtr = &globs.hash[key % BLU_AREA_HASH_COUNT];

    while(*nextPtr) {

        if((*nextPtr)->hashKey == key) {
            *nextPtr = nullptr;
            return;
        }

        nextPtr = &(*nextPtr)->hashNext;
    }
}



// CLEANUP: update and reset happen post-construction because it was easy, change please
void _blu_areaReset(blu_Area* a) {
    a->firstChild[globs.linkSide] = nullptr;
    a->lastChild = nullptr;
    a->nextSibling[globs.linkSide] = nullptr;
    a->prevSibling = nullptr;

    // NOTE: this could be bad for perf, but who cares
    a->flags = 0;
    a->displayString = { nullptr, 0 };
    a->texture = nullptr;
    a->offset = { 0, 0 };
}

void _blu_areaUpdate(blu_Area* a) {
    bool hover = a->prevHovered;

    if(a->flags & blu_areaFlags_HOVER_ANIM) {
        a->target_hoverAnim += (hover - a->target_hoverAnim) * a->style.animationStrength; }
    else { a->target_hoverAnim = 0; }
}


blu_Area* blu_areaMake(str string, U32 flags) {

    U64 hashKey = hash_hashStr(string);
    blu_Area* area = blu_hashGet(hashKey);


    if(area) { // assert to check a component hasn't been created twice per frame
        ASSERT(area->lastTouchedIdx != globs.frameIndex); }


    // CONSTRUCTION //////////////////
    if(!area) {

        // GET NEW AREA STRUCT ////////////////////
        area = globs.firstFree;
        if(!area) { // push new to arena if free list is empty
            area = BUMP_PUSH_NEW(globs.areaArena, blu_Area); }
        else { // pop from free list otherwise
            globs.firstFree = area->nextFree;
            *area = blu_Area();
        }
        ///////////////////////////////////////////

        blu_hashInsert(hashKey, area);

        area->hashKey = hashKey;
    }
    // IF OLD, RESET CURRENT TREE LINKS //////////////////
    else {
        _blu_areaReset(area);
    }

    area->flags = flags;

    area->lastTouchedIdx = globs.frameIndex;



    // APPLY CURRENTLY BUILT STYLE ////////////////////////////////////////////////
    // TODO: make recalculation happen less
    area->style = blu_Style();

    blu_StyleStackNode* st = globs.ogStyle;
    while(st) {

        if(st->data.overrideFlags & blu_styleFlags_childLayoutAxis) {
            area->style.childLayoutAxis = st->data.childLayoutAxis; }

        if(st->data.overrideFlags & blu_styleFlags_sizeX) {
            area->style.sizes[blu_axis_X] = st->data.sizes[blu_axis_X]; }

        if(st->data.overrideFlags & blu_styleFlags_sizeY) {
            area->style.sizes[blu_axis_Y] = st->data.sizes[blu_axis_Y]; }

        if(st->data.overrideFlags & blu_styleFlags_backgroundColor) {
            area->style.backgroundColor = st->data.backgroundColor; }

        if(st->data.overrideFlags & blu_styleFlags_textColor) {
            area->style.textColor = st->data.textColor; }

        if(st->data.overrideFlags & blu_styleFlags_textPadding) {
            area->style.textPadding = st->data.textPadding; }

        if(st->data.overrideFlags & blu_styleFlags_animationStrength) {
            area->style.animationStrength = st->data.animationStrength; }

        st = st->next;
    }


    // SET TREE LINKS ///////////////////////////////////////////////////////////

    blu_Area* parent = globs.currentParent;
    if(parent != nullptr) {
        if(parent->firstChild[globs.linkSide] == nullptr) { parent->firstChild[globs.linkSide] = area; }
        else { parent->lastChild->nextSibling[globs.linkSide] = area; }
        area->prevSibling = parent->lastChild;
        parent->lastChild = area;
    }
    area->parent = parent;

    // SET TREE LINKS ///////////////////////////////////////////////////////////

    _blu_areaUpdate(area);

    return area;
}

void blu_areaAddDisplayStr(blu_Area* area, str s) {
    str nstr = str_copy(s, globs.frameArena);
    area->displayString = nstr;
}

// CLEANUP: are wo going with opaque or no

void blu_pushParent(blu_Area* parent) {
    globs.currentParent = parent;
}

void blu_popParent() {
    ASSERT(globs.currentParent->parent != nullptr);
    globs.currentParent = globs.currentParent->parent;
}





void blu_areaRelease(blu_Area* area) {
    area->nextFree = globs.firstFree;
    globs.firstFree = area;
}

void _blu_cullRecursive(blu_Area* area) {

    if(!area) { return; }

    for(blu_Area* elem = area->firstChild[!globs.linkSide]; elem; elem = elem->nextSibling[!globs.linkSide]){
        _blu_cullRecursive(elem);
    }

    if(area->lastTouchedIdx < globs.frameIndex) {
        blu_areaRelease(area);
        blu_hashRemove(area->hashKey);
    }
}

void blu_beginFrame() {

    // U64 byteCount = (U64)(globs.frameArena.offset) - (U64)(globs.frameArena.start);
    // printf("%llu / %llu\n", byteCount, globs.frameArena.reserved);
    bump_clear(globs.frameArena);
    _blu_cullRecursive(globs.ogParent);

    globs.frameIndex++;
    globs.linkSide = globs.frameIndex%2;

    globs.ogStyle = nullptr;
    globs.currentStyle = nullptr;

    globs.currentParent = nullptr;
    blu_Area* np = blu_areaMake(str_make("HELP ME PLEASE GOD PLEASE"), 0);
    blu_pushParent(np);
    globs.ogParent = np;
}






float _blu_sizeOfString(str s) {
    float size = 0;
    for(int i = 0; i < s.length; i++) {
        size += globs.fontGlyphs[s.chars[i] - BLU_FONT_FIRST].advance; // CLEANUP: char oob assertion
    }

    return size;
}



void _blu_solveRemainders(blu_Area* parent, int axis) {



    float wantedSize = parent->calculatedSizes[axis];
    float totalSize = 0;
    U32 remainderCount = 0;

    blu_Area* elem = parent->firstChild[globs.linkSide];
    for(; elem; elem = elem->nextSibling[globs.linkSide]) {
        if((elem->flags & blu_areaFlags_FLOATING)) { continue; }

        float elemSize = elem->calculatedSizes[axis];
        if(elem->style.sizes[axis].kind == blu_sizeKind_REMAINDER) {
            remainderCount++;
        }
        else {
            if(parent->style.childLayoutAxis == axis) {
                totalSize += elemSize; }
            else {
                if(elem->calculatedSizes[axis] > totalSize) {
                    totalSize = elemSize; }
            }
        }
    }

    float error = wantedSize - totalSize;
    if(error > 0) {
        float remSize = error / remainderCount;

        elem = parent->firstChild[globs.linkSide];
        while(elem) {
            if(elem->style.sizes[axis].kind == blu_sizeKind_REMAINDER) {
                elem->calculatedSizes[axis] = remSize; }
            elem = elem->nextSibling[globs.linkSide];
        }
    }

    elem = parent->firstChild[globs.linkSide];
    for(; elem; elem = elem->nextSibling[globs.linkSide]) {
        _blu_solveRemainders(elem, axis);
    }
}


void _blu_calculateOffsetsAndRect(blu_Area* parent) {

    bool x = parent->style.childLayoutAxis == blu_axis_X;
    V2f off = { 0, 0 };

    blu_Area* elem = parent->firstChild[globs.linkSide];
    while(elem) {

        if((elem->flags & blu_areaFlags_FLOATING)) {
            elem->calculatedPosition[blu_axis_X] = parent->calculatedPosition[blu_axis_X] + elem->offset.x;
            elem->calculatedPosition[blu_axis_Y] = parent->calculatedPosition[blu_axis_Y] + elem->offset.y;
        }
        else {
            elem->calculatedPosition[blu_axis_X] = parent->calculatedPosition[blu_axis_X] + off.x;
            elem->calculatedPosition[blu_axis_Y] = parent->calculatedPosition[blu_axis_Y] + off.y;

            off += {
                (elem->calculatedSizes[blu_axis_X] * x),
                (elem->calculatedSizes[blu_axis_Y] * (!x))
            };
        }

        _blu_calculateOffsetsAndRect(elem);
        elem = elem->nextSibling[globs.linkSide];
    }
}

void blu_layout(V2f scSize) {

    globs.ogParent->calculatedSizes[blu_axis_X] = scSize.x;
    globs.ogParent->calculatedSizes[blu_axis_Y] = scSize.y;


    U64 visitSize = BLU_MAX_AREA_COUNT * sizeof(blu_Area*);
    blu_Area** visitStack = BUMP_PUSH_ARR(globs.frameArena, BLU_MAX_AREA_COUNT, blu_Area*);
    U32 visitStackDepth = 0;

    for(int axis = 0; axis < blu_axis_COUNT; axis++) {

        // STANDALONE SIZES ==================================================================

        ARR_APPEND(visitStack, visitStackDepth, globs.ogParent);
        while(visitStackDepth > 0) {
            blu_Area* elem = ARR_POP(visitStack, visitStackDepth);

            if(elem->style.sizes[axis].kind == blu_sizeKind_PX) {
                elem->calculatedSizes[axis] = elem->style.sizes[axis].value; }

            else if(elem->style.sizes[axis].kind == blu_sizeKind_TEXT) {
                if(axis == blu_axis_Y) {
                    elem->calculatedSizes[axis] = globs.fontHeight + 2*elem->style.textPadding.y;
                    // CLEANUP: what about no text
                    // TODO: (?) vertical text layout?
                    // TODO: text wrapping/cutoff
                }
                else if(axis == blu_axis_X) {

                    float s = elem->style.textPadding.x * 2;
                    s += _blu_sizeOfString(elem->displayString);

                    elem->calculatedSizes[axis] = s;
                }
            }


            blu_Area* child = elem->firstChild[globs.linkSide];
            while(child) {
                ASSERT(visitStackDepth < BLU_MAX_AREA_COUNT);
                ARR_APPEND(visitStack, visitStackDepth, child);
                child = child->nextSibling[globs.linkSide];
            }
        }

        // UPWARD DEPENDANT SIZES ===========================================================

        visitStackDepth = 0;
        ARR_APPEND(visitStack, visitStackDepth, globs.ogParent);
        while(visitStackDepth > 0) {
            blu_Area* elem = ARR_POP(visitStack, visitStackDepth);


            if(elem->style.sizes[axis].kind == blu_sizeKind_PERCENT) {
                elem->calculatedSizes[axis] = elem->parent->calculatedSizes[axis] * elem->style.sizes[axis].value; }


            blu_Area* child = elem->firstChild[globs.linkSide];
            while(child) {
                ASSERT(visitStackDepth < BLU_MAX_AREA_COUNT);
                ARR_APPEND(visitStack, visitStackDepth, child);
                child = child->nextSibling[globs.linkSide];
            }
        }


        _blu_solveRemainders(globs.ogParent, axis);
    } // end axis loop

    _blu_calculateOffsetsAndRect(globs.ogParent);

    bump_pop(globs.frameArena, visitSize);
} // end layout









// NOTE: start is UL of text, not baseline
// CLEANUP: that lol ^^^^^^^^^^^^^^^^^^^^^
void _blu_renderString(str string, V2f start, V4f color, gfx_Pass* pass) {


    V2f cursor = start + V2f(0, +globs.fontAscent);
    for(int i = 0; i < string.length; i++) {

        U8 c = string.chars[i] - BLU_FONT_FIRST;
        assert(c >= 0 && c < BLU_FONT_CHARCOUNT);
        blu_Glyph data = globs.fontGlyphs[c];

        gfx_UniformBlock* block = gfx_registerCall(pass);

        block->color = color;
        block->fontTexture = globs.fontTex;
        block->texture = globs.solidTex;

        block->srcStart = data.startUV;
        block->srcEnd = data.endUV;
        block->dstStart = V2f(cursor.x + data.xBearing, cursor.y - data.yBearing);
        block->dstEnd = block->dstStart + V2f(data.width, data.height);

        cursor.x += data.advance;
    }
}


void _blu_genRenderCallsRecurse(blu_Area* area, gfx_Pass* pass) {

    while(area) {

            // CLEANUP:
            V2f pos = V2f(
                area->calculatedPosition[blu_axis_X],
                area->calculatedPosition[blu_axis_Y]
                );
            V2f size = V2f(
                area->calculatedSizes[blu_axis_X],
                area->calculatedSizes[blu_axis_Y]);

        if(area->flags & blu_areaFlags_DRAW_BACKGROUND) {

            gfx_UniformBlock* block = gfx_registerCall(pass);

            block->dstStart = pos;
            block->dstEnd = pos + size;
            block->color = area->style.backgroundColor;
            block->fontTexture = globs.solidTex;
            block->texture = globs.solidTex;
        }
        if (area->flags & blu_areaFlags_DRAW_TEXT) {

            V2f off = V2f(0, 0);

            if(area->flags & blu_areaFlags_CENTER_TEXT) {
                float size = area->calculatedSizes[blu_axis_X];
                float strSize = _blu_sizeOfString(area->displayString);
                off.x = (size - strSize) / 2;
                off.x -= area->style.textPadding.x;
            }

            _blu_renderString(area->displayString, off + pos + area->style.textPadding, area->style.textColor, pass);
        }

        if (area->flags & blu_areaFlags_DRAW_TEXTURE) {

            gfx_UniformBlock* block = gfx_registerCall(pass);

            block->dstStart = pos;
            block->dstEnd = pos + size;
            block->color = V4f(1, 1, 1, 1);
            block->texture = area->texture;
            block->fontTexture = globs.solidTex;
        }


        _blu_genRenderCallsRecurse(area->firstChild[globs.linkSide], pass);
        area = area->nextSibling[globs.linkSide];
    }
}
// CLEANUP: rename
void blu_createPass(gfx_Pass* normalPass) {
    _blu_genRenderCallsRecurse(globs.ogParent, normalPass);
}












#define _BLU_DEFINE_STYLE_ADD(varName, type) \
    void blu_style_add_##varName(type varName) { \
        globs.currentStyle->data.overrideFlags |= blu_styleFlags_##varName; \
        globs.currentStyle->data.varName = varName; \
    } \

void blu_style_add_sizeX(blu_Size size) {
    globs.currentStyle->data.overrideFlags |= blu_styleFlags_sizeX;
    globs.currentStyle->data.sizes[blu_axis_X] = size;
}
void blu_style_add_sizeY(blu_Size size) {
    globs.currentStyle->data.overrideFlags |= blu_styleFlags_sizeY;
    globs.currentStyle->data.sizes[blu_axis_Y] = size;
}

_BLU_DEFINE_STYLE_ADD(childLayoutAxis, blu_Axis)
_BLU_DEFINE_STYLE_ADD(backgroundColor, V4f)
_BLU_DEFINE_STYLE_ADD(textColor, V4f)
_BLU_DEFINE_STYLE_ADD(textPadding, V2f)
_BLU_DEFINE_STYLE_ADD(animationStrength, F32)




void blu_pushStyle() {

    blu_StyleStackNode* s = BUMP_PUSH_NEW(globs.frameArena, blu_StyleStackNode);

    if(!globs.ogStyle) {
        globs.currentStyle = s;
        globs.ogStyle = s;
        s->prev = globs.currentStyle;
    }
    else {
        globs.currentStyle->next = s;
        s->prev = globs.currentStyle;
        globs.currentStyle = s;
    }
}

void blu_popStyle() {
    ASSERT(globs.currentStyle);
    ASSERT(globs.currentStyle->prev);
    globs.currentStyle->prev->next = nullptr;
    globs.currentStyle = globs.currentStyle->prev;
}






// return indicates if parent has been blocked
// never touch this code again
bool _blu_genInteractionsRecurse(blu_Area* area, bool covered) {


    blu_Area* elem = area->lastChild;
    while(elem) {

        if(_blu_genInteractionsRecurse(elem, covered)) {
            covered = true; }

        elem = elem->prevSibling;
    }

    bool containsMouse = false;
    if(!covered) {
        V2f pos = globs.inputMousePos;
        if(area->calculatedPosition[blu_axis_X] <= pos.x &&
            area->calculatedPosition[blu_axis_X] + area->calculatedSizes[blu_axis_X] > pos.x) {
            if(area->calculatedPosition[blu_axis_Y] <= pos.y &&
                area->calculatedPosition[blu_axis_Y] + area->calculatedSizes[blu_axis_Y] > pos.y) {

                containsMouse = true;
            }
        }
    }


    bool clickable = area->flags & blu_areaFlags_CLICKABLE;
    if(clickable) {
        area->prevHovered = containsMouse;
        area->prevPressed = globs.inputCurLButton && !globs.inputPrevLButton && containsMouse;

        if(area->prevPressed) {
            globs.dragged = area; }
    }
    else {
        area->prevHovered = false;
        area->prevPressed = false;
    }

    return (clickable && containsMouse) || covered;
}

void blu_input(V2f npos, bool lmbState) {

    if(!globs.inputCurLButton && globs.inputPrevLButton) {
        globs.dragged = nullptr;
    }

    globs.dragDelta = npos - globs.inputMousePos;
    globs.inputMousePos = npos;

    globs.inputPrevLButton = globs.inputCurLButton;
    globs.inputCurLButton = lmbState;


    bool x;
    _blu_genInteractionsRecurse(globs.ogParent, globs.dragged != nullptr);
}


blu_WidgetInputs blu_interactionFromWidget(blu_Area* area) {
    ASSERT(area->flags & blu_areaFlags_CLICKABLE);

    blu_WidgetInputs out;

    if(globs.dragged && globs.dragged != area) {
        return out; }


    out.hovered = area->prevHovered;

    if(globs.dragged == area) {
        out.held = true;
        out.dragDelta = globs.dragDelta;

        if(!globs.inputCurLButton && globs.inputPrevLButton) {
            out.clicked = true; }
    }

    return out;
}




#endif