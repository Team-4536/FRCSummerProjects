#pragma once
#include "base/utils.h"
#include "base/config.h"
#include "base/str.h"
#include "base/allocators.h"
#include "graphics.h"



/*
THE PLAN: build an immediate mode UI library that is as portable as as possible, for use in future projects

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
    [X] scrolling
    [X] clipping
    [X] cursor changes
    [X] rounding
    [X] borders
    [X] tooltips
    [X] drag drop
    [ ] disabling
    [ ] dropdowns
    [ ] padding
    [ ] drop shadows
    [ ] text input
    [ ] text hotkeys

    [ ] cleanup
    [ ] batching

    [ ] documentation and examples
    [ ] testing?

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
    blu_sizeKind_CHILDSUM,
};

enum blu_AreaFlags { // TODO: name these better
    blu_areaFlags_NONE =            (0 << 0),
    blu_areaFlags_DRAW_BACKGROUND = (1 << 0),
    blu_areaFlags_DRAW_TEXT =       (1 << 1),
    blu_areaFlags_DRAW_TEXTURE =    (1 << 2),
    blu_areaFlags_FLOATING =        (1 << 3),
    blu_areaFlags_HOVER_ANIM =      (1 << 4),
    blu_areaFlags_CENTER_TEXT =     (1 << 5),
    blu_areaFlags_CLICKABLE =       (1 << 6),
    blu_areaFlags_SCROLLABLE =      (1 << 7),
    blu_areaFlags_VIEW_OFFSET =     (1 << 8),
    blu_areaFlags_DROP_EVENTS =     (1 << 9),
};

struct blu_Size {
    blu_SizeKind kind = blu_sizeKind_NONE;
    F32 value = 0;
};

enum blu_Cursor {
    blu_cursor_norm,
    blu_cursor_hand,
    blu_cursor_resizeH,
    blu_cursor_resizeV,
    blu_cursor_type,
};




// NOTE:
// when editing, be sure to update
//  - flags enum
//  - style struct
//  - add functions
//  - add function header
//  - style build code in areaMake
// gad damn this language

struct blu_Style {
    U32 overrideFlags = 0;

    blu_Axis childLayoutAxis = blu_axis_X;
    blu_Size sizes[blu_axis_COUNT] = { { blu_sizeKind_NONE, 0 }, { blu_sizeKind_NONE, 0 } };
    V4f backgroundColor = V4f();
    V4f textColor = V4f();
    V2f textPadding = V2f();
    F32 animationStrength = 0;
    F32 cornerRadius = 0;
    V4f borderColor = V4f();
    F32 borderSize = 0;
};

enum blu_StyleFlags {
    blu_styleFlags_none             = (0 << 0),
    blu_styleFlags_childLayoutAxis  = (1 << 1),
    blu_styleFlags_sizeX            = (1 << 2),
    blu_styleFlags_sizeY            = (1 << 3),
    blu_styleFlags_backgroundColor  = (1 << 4),
    blu_styleFlags_textColor        = (1 << 5),
    blu_styleFlags_textPadding      = (1 << 6),
    blu_styleFlags_animationStrength= (1 << 7),
    blu_styleFlags_cornerRadius     = (1 << 8),
    blu_styleFlags_borderSize       = (1 << 9),
    blu_styleFlags_borderColor      = (1 << 10),
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
    // CLEANUP: replace double buffer/ benchmark vs a copy version
    // CLEANUP: better way of iterating over all elements

    blu_Area* ctorParent = nullptr; // where to go after popParent() is called (because sometimes you push to another tree (like the cursor tree))
    blu_Area* parent = nullptr; // actual parent in the tree
    blu_Area* firstChild[2] = { 0 };
    blu_Area* lastChild = nullptr;
    blu_Area* nextSibling[2] = { 0 };
    blu_Area* prevSibling = nullptr;

    // hashing //////////////////////////////////////
    blu_Area* hashNext;
    U64 hashKey;

    U64 lastTouchedIdx = 0;

    // builder provided /////////////////////////////
    U32 flags = 0;
    str displayString = { nullptr, 0 };

    blu_Style style;

    gfx_Texture* texture = nullptr;

    V2f offset = { 0, 0 };
    V2f viewOffset = { 0, 0 };

    blu_Cursor cursor = blu_cursor_norm;

    // CLEANUP: do these need to be reset every frame? (not rn)
    // masks which events are recieved
    U32 dropTypeMask = 0;
    // masks which events are given (which other elems can recieve)
    U32 dropType = 0;
    void* dropVal = nullptr;

    // layout pass data //////////////////////////////
    F32 calculatedSizes[blu_axis_COUNT];
    Rect2f rect;
    Rect2f clipRect;

    // persistant shit for input / anim //////////////
    F32 target_hoverAnim = 0;

    bool prevHovered = false;
    bool prevPressed = false;
    F32 scrollDelta = 0;
};


struct blu_WidgetInteraction {
    bool hovered = false;
    bool held = false;
    bool clicked = false;

    V2f dragDelta = V2f();
    V2f mousePos = V2f();

    float scrollDelta = 0;

    // another elem has been dropped on this one
    bool dropped = false;
    void* dropVal = nullptr;
    U32 dropType = 0;
};




void blu_init(gfx_Texture* solidTex);
void blu_loadFont(const char* path);
void blu_beginFrame(); // cull, reset globals
// build code goes here
void blu_layout(V2f scSize); // calculate layout shit
void blu_input(V2f npos, bool lmbState, float scrollDelta, blu_Cursor* outCursor);  // set current and update prev input // CLEANUP: merge with begin?
void blu_makeDrawCalls(gfx_Pass* normalPass);

blu_Area* blu_areaMake(str s, U32 flags);
blu_Area* blu_areaMake(const char* string, U32 flags);
void blu_pushParent(blu_Area* parent);
void blu_popParent();

blu_Area* blu_getCursorParent();

void blu_areaAddDisplayStr(blu_Area* area, str s); // CLEANUP: inconsistent, ctor functions or no
void blu_areaAddDisplayStr(blu_Area* area, const char* s);

void blu_pushStyle(blu_Style s);
void blu_popStyle();
void blu_style_style(blu_Style* style, blu_Style* target = nullptr);

void blu_style_sizeX(blu_Size size, blu_Style* target = nullptr);
void blu_style_sizeY(blu_Size size, blu_Style* target = nullptr);
void blu_style_childLayoutAxis(blu_Axis axis, blu_Style* target = nullptr);
void blu_style_backgroundColor(V4f color, blu_Style* target = nullptr);
void blu_style_textColor(V4f color, blu_Style* target = nullptr);
void blu_style_textPadding(V2f padding, blu_Style* target = nullptr);
void blu_style_animationStrength(F32 s, blu_Style* target = nullptr);
void blu_style_cornerRadius(F32 radius, blu_Style* target = nullptr);
void blu_style_borderSize(F32 size, blu_Style* target = nullptr);
void blu_style_borderColor(V4f color, blu_Style* target = nullptr);

blu_WidgetInteraction blu_interactionFromWidget(blu_Area* area);


#define blu_deferLoop(begin, end) for(int _i_ = ((begin), 0); !_i_; _i_ += 1, (end))
#define blu_styleScope(style) blu_deferLoop(blu_pushStyle(style), blu_popStyle())
#define blu_parentScope(parent) blu_deferLoop(blu_pushParent(parent), blu_popParent())


#define BLU_FONT_SIZE 20 // in px

#ifdef BLU_IMPL

#include "base/hashtable.h"
#include "base/arr.h"
#include "stb_truetype/stb_truetype.h"
#include <stdio.h>


// CLEANUP: these
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

    blu_Area* ogParent = nullptr;
    blu_Area* cursorParent = nullptr;
    blu_Area* currentParent = nullptr;

    blu_StyleStackNode* currentStyle = nullptr;
    blu_StyleStackNode* ogStyle = nullptr;



    blu_Area** hash = nullptr; // array of pointers to the actual area structs, for hash-based access

    BumpAlloc areaArena = { };
    blu_Area* firstFree = nullptr;

    BumpAlloc frameArena = { };


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
    blu_Area* prevDragged = nullptr;
    V2f dragDelta = V2f();
};

static blu_Globs globs = blu_Globs();


// CLEANUP: gen solidtex at startup
void blu_init(gfx_Texture* solidTex) {

    globs.solidTex = solidTex;

    bump_allocate(&globs.areaArena, BLU_MAX_AREA_COUNT * sizeof(blu_Area));
    bump_allocate(&globs.frameArena, BLU_MAX_ARENA_SIZE);
    globs.hash = (blu_Area**)arr_allocate0(sizeof(blu_Area*), BLU_AREA_HASH_COUNT);
}

void blu_loadFont(const char* path) {

    U64 size;
    U8* fontBuffer = loadFileToBuffer(path, false, &size, &globs.frameArena);
    ASSERT(fontBuffer);

    const U32 texSize = 512;
    U8* imgbuf = BUMP_PUSH_ARR(&globs.frameArena, texSize*texSize, U8);
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


    bump_clear(&globs.frameArena);
}




// NOTE: asserts if element already exists
void _blu_hashInsert(U64 key, blu_Area* a) {
    U64 idx = key % BLU_AREA_HASH_COUNT;
    blu_Area* elem = globs.hash[idx];

    if(elem) { // remember the shit about a default ptr so no segfualt
        while(elem->hashNext) {
            if(elem->hashKey == key) { ASSERT(false); return; }
            elem = elem->hashNext;
        }
        elem->hashNext = a;
    }
    else {
        globs.hash[idx] = a; }
}

blu_Area* _blu_hashGet(U64 key) {
    blu_Area* elem = globs.hash[key % BLU_AREA_HASH_COUNT];
    while(true) {
        if(elem == nullptr) { return nullptr; }
        if(elem->hashKey == key) { return elem; }

        elem = elem->hashNext;
    }
}

void _blu_hashRemove(U64 key) {
    blu_Area** nextPtr = &globs.hash[key % BLU_AREA_HASH_COUNT];
    while(*nextPtr) {
        if((*nextPtr)->hashKey == key) {
            *nextPtr = nullptr;
            return;
        }
        nextPtr = &(*nextPtr)->hashNext;
    }
}





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
    a->cursor = blu_cursor_norm;
}



blu_Area* blu_areaMake(const char* string, U32 flags) {
    return blu_areaMake(str_make(string), flags);
}
blu_Area* blu_areaMake(str string, U32 flags) {

    U64 hashKey = hash_hashStr(string);
    if(globs.currentParent) {
        hashKey += globs.currentParent->hashKey; }

    blu_Area* area = _blu_hashGet(hashKey);


    // CONSTRUCTION /////////////////////////////////////////////////////////////
    if(!area) {
        area = globs.firstFree;
        if(!area) { // push new to arena if free list is empty
            area = BUMP_PUSH_NEW(&globs.areaArena, blu_Area); }
        else { // pop from free list otherwise
            globs.firstFree = area->nextFree;
            *area = blu_Area();
        }

        _blu_hashInsert(hashKey, area);
        area->hashKey = hashKey;
    }
    else {
        ASSERT(area->lastTouchedIdx != globs.frameIndex);
        _blu_areaReset(area);
    }


    // APPLY CURRENTLY BUILT STYLE ////////////////////////////////////////////////
    // CLEANUP: could optimize?
    area->style = blu_Style();
    blu_StyleStackNode* st = globs.ogStyle;
    while(st) {
        blu_style_style(&st->data, &area->style);
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
    area->flags = flags;
    area->lastTouchedIdx = globs.frameIndex;

    blu_areaAddDisplayStr(area, string);

    return area;
}

void blu_areaAddDisplayStr(blu_Area* area, const char* s) {
    blu_areaAddDisplayStr(area, str_make(s));
}
void blu_areaAddDisplayStr(blu_Area* area, str s) {
    str nstr = str_copy(s, &globs.frameArena);
    area->displayString = nstr;
}



void blu_pushParent(blu_Area* parent) {
    parent->ctorParent = globs.currentParent;
    globs.currentParent = parent;
}
void blu_popParent() {
    ASSERT(globs.currentParent->ctorParent != nullptr);
    globs.currentParent = globs.currentParent->ctorParent;
}





blu_Area* blu_getCursorParent() {
    ASSERT(globs.cursorParent->firstChild[globs.linkSide] == nullptr);
    return globs.cursorParent;
}



// walk previous frames tree to remove old areas
void _blu_cullRecurse(blu_Area* elem) {
    while(elem){
        if(elem->lastTouchedIdx < globs.frameIndex) {
            // push to free list
            elem->nextFree = globs.firstFree;
            globs.firstFree = elem;
            _blu_hashRemove(elem->hashKey);
        }
        _blu_cullRecurse(elem->firstChild[!globs.linkSide]);
        elem = elem->nextSibling[!globs.linkSide];
    }
}

// reset globals, cull old areas, free memory
void blu_beginFrame() {
    // U64 byteCount = (U64)(globs.frameArena.offset) - (U64)(globs.frameArena.start);
    // printf("%llu / %llu\n", byteCount, globs.frameArena.reserved);
    bump_clear(&globs.frameArena);

    _blu_cullRecurse(globs.ogParent);


    globs.frameIndex++;
    globs.linkSide = globs.frameIndex%2;


    globs.ogStyle = nullptr;
    globs.currentStyle = nullptr;
    globs.currentParent = nullptr;
    globs.cursorParent = nullptr;


    globs.ogParent = blu_areaMake("HELP ME PLEASE GOD PLEASE", 0);
    globs.cursorParent = blu_areaMake("WOAHHHH CURSOR PARENT HOLY SHIT", 0);

    globs.currentParent = globs.ogParent;
}











float _blu_sizeOfString(str s, int axis) {
    if(axis == blu_axis_Y) { return globs.fontHeight; }

    float size = 0;
    for(int i = 0; i < s.length; i++) {
        size += globs.fontGlyphs[s.chars[i] - BLU_FONT_FIRST].advance; // CLEANUP: out of bounds char rendering
    }
    return size;
}


// double underscore indicates only being used in one place
// would have just inlined it but this language doesn't have nested function defs

// NOTE: noop on parent
// fills in PX and TEXT based sizes
void __blu_calculateStandaloneSizesRecurse(blu_Area* parent, int axis) {

    blu_Area* elem = parent->firstChild[globs.linkSide];
    while(elem) {
        if(elem->style.sizes[axis].kind == blu_sizeKind_PX) {
            elem->calculatedSizes[axis] = elem->style.sizes[axis].value;
        }
        else if(elem->style.sizes[axis].kind == blu_sizeKind_TEXT) {

            // CLEANUP: what about no text
            // TODO: text wrapping/truncation

            float pad = elem->style.textPadding.x * 2;
            if(axis == blu_axis_Y) { pad = elem->style.textPadding.y * 2; }
            elem->calculatedSizes[axis] = _blu_sizeOfString(elem->displayString, axis) + pad;
        }

        __blu_calculateStandaloneSizesRecurse(elem, axis);
        elem = elem->nextSibling[globs.linkSide];
    }
}

// NOTE: does operate on parent
// fills in pct based sizes
void __blu_calculateUpwardsSizesRecurse(blu_Area* elem, int axis) {
    while(elem) {

        if(elem->style.sizes[axis].kind == blu_sizeKind_PERCENT) {
            elem->calculatedSizes[axis] = elem->parent->calculatedSizes[axis] * elem->style.sizes[axis].value; }

        __blu_calculateUpwardsSizesRecurse(elem->firstChild[globs.linkSide], axis);
        elem = elem->nextSibling[globs.linkSide];
    }
}



void __blu_calculateDownwardsSizesRecurse(blu_Area* elem, int axis) {
    while(elem) {
        __blu_calculateDownwardsSizesRecurse(elem->firstChild[globs.linkSide], axis);

        if(elem->style.sizes[axis].kind == blu_sizeKind_CHILDSUM) {
            float sum = 0;

            blu_Area* child = elem->firstChild[globs.linkSide];
            while(child) {
                sum += child->calculatedSizes[axis];
                child = child->nextSibling[globs.linkSide];
            }
            elem->calculatedSizes[axis] = sum;
            // printf("%f", sum);
        }

        elem = elem->nextSibling[globs.linkSide];
    }
}


// NOTE: does not operate on parent
// fills in remainder sizes
void __blu_solveChildRemaindersRecurse(blu_Area* parent, int axis) {

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
        __blu_solveChildRemaindersRecurse(elem, axis);
    }
}

// NOTE: does not operate on parent object
void __blu_calculateChildRectsRecurse(blu_Area* parent, Rect2f clip) {

    bool x = parent->style.childLayoutAxis == blu_axis_X;
    V2f off = { 0, 0 };

    if(parent->flags & blu_areaFlags_VIEW_OFFSET) {
        off = -parent->viewOffset; }

    Rect2f parentClip = clip;
    blu_Area* elem = parent->firstChild[globs.linkSide];
    while(elem) {

        V2f elemOffset = off;

        if(elem->flags & blu_areaFlags_FLOATING) {
            elemOffset = elem->offset; }
        else {
            off += {
                (elem->calculatedSizes[blu_axis_X] * x),
                (elem->calculatedSizes[blu_axis_Y] * (!x))
            };
        }

        V2f calcdPos = parent->rect.start + elemOffset;
        elem->rect = Rect2f {
            { calcdPos },
            {
                calcdPos.x + elem->calculatedSizes[blu_axis_X],
                calcdPos.y + elem->calculatedSizes[blu_axis_Y]
            }
        };

        clip.start.x = max(parentClip.start.x, elem->rect.start.x);
        clip.start.y = max(parentClip.start.y, elem->rect.start.y);
        clip.end.x = min(parentClip.end.x, elem->rect.end.x);
        clip.end.y = min(parentClip.end.y, elem->rect.end.y);
        elem->clipRect = clip;

        __blu_calculateChildRectsRecurse(elem, clip);
        elem = elem->nextSibling[globs.linkSide];
    }
}

void blu_layout(V2f scSize) {

    globs.ogParent->calculatedSizes[blu_axis_X] = scSize.x;
    globs.ogParent->calculatedSizes[blu_axis_Y] = scSize.y;
    globs.ogParent->rect = {
        { 0, 0 },
        { scSize.x, scSize.y }
    };
    globs.ogParent->clipRect = globs.ogParent->rect;

    globs.cursorParent->calculatedSizes[blu_axis_X] = 300;
    globs.cursorParent->calculatedSizes[blu_axis_Y] = 300;
    globs.cursorParent->rect = {
        globs.inputMousePos,
        globs.inputMousePos + scSize
    };
    globs.currentParent->clipRect = globs.ogParent->rect;



    for(int axis = 0; axis < blu_axis_COUNT; axis++) {
        // sorry :p
        __blu_calculateStandaloneSizesRecurse(globs.ogParent, axis);
        __blu_calculateUpwardsSizesRecurse(globs.ogParent, axis);
        __blu_calculateDownwardsSizesRecurse(globs.ogParent, axis);
        __blu_solveChildRemaindersRecurse(globs.ogParent, axis);

        __blu_calculateStandaloneSizesRecurse(globs.cursorParent, axis);
        __blu_calculateUpwardsSizesRecurse(globs.cursorParent, axis);
        __blu_calculateDownwardsSizesRecurse(globs.cursorParent, axis);
        __blu_solveChildRemaindersRecurse(globs.cursorParent, axis);
    }

    __blu_calculateChildRectsRecurse(globs.ogParent, globs.ogParent->clipRect);
    __blu_calculateChildRectsRecurse(globs.cursorParent, globs.ogParent->clipRect);
}
















// NOTE: start is UL of text, not baseline
// CLEANUP: that lol ^^^^^^^^^^^^^^^^^^^^^
void _blu_renderString(str string, V2f start, Rect2f clip, V4f color, gfx_Pass* pass) {


    V2f cursor = start + V2f(0, +globs.fontAscent);
    for(int i = 0; i < string.length; i++) {

        U8 c = string.chars[i] - BLU_FONT_FIRST;

        // TODO: INVALID CHAR RENDERING
        // if(c < 0 || c > BLU_FONT_CHARCOUNT) {
            // c = 'I'; }
        ASSERT(c >= 0 && c < BLU_FONT_CHARCOUNT);

        blu_Glyph data = globs.fontGlyphs[c];

        gfx_UniformBlock* block = gfx_registerCall(pass);

        block->color = color;
        block->fontTexture = globs.fontTex;
        block->texture = globs.solidTex;

        block->srcStart = data.startUV;
        block->srcEnd = data.endUV;
        block->dstStart = V2f(cursor.x + data.xBearing, cursor.y - data.yBearing);
        block->dstEnd = block->dstStart + V2f(data.width, data.height);

        block->clipStart = clip.start;
        block->clipEnd = clip.end;

        cursor.x += data.advance;
    }
}


void _blu_genRenderCallsRecurse(blu_Area* area, gfx_Pass* pass) {

    while(area) {

        if(area->flags & blu_areaFlags_DRAW_BACKGROUND) {

            gfx_UniformBlock* block = gfx_registerCall(pass);

            block->dstStart = area->rect.start;
            block->dstEnd = area->rect.end;
            block->color = area->style.backgroundColor;
            block->fontTexture = globs.solidTex;
            block->texture = globs.solidTex;
            block->clipStart = area->clipRect.start;
            block->clipEnd = area->clipRect.end;
            block->cornerRadius = area->style.cornerRadius;
            block->borderSize = area->style.borderSize;
            block->borderColor = area->style.borderColor;
        }
        if (area->flags & blu_areaFlags_DRAW_TEXT) {

            V2f off = area->style.textPadding;

            if(area->flags & blu_areaFlags_CENTER_TEXT) {
                float size = area->calculatedSizes[blu_axis_X];
                float strSize = _blu_sizeOfString(area->displayString, blu_axis_X);
                off.x = (size - strSize) / 2;
            }

            _blu_renderString(
                area->displayString,
                off + area->rect.start,
                area->clipRect,
                area->style.textColor,
                pass);
        }
        if (area->flags & blu_areaFlags_DRAW_TEXTURE) {

            gfx_UniformBlock* block = gfx_registerCall(pass);

            block->dstStart = area->rect.start;
            block->dstEnd = area->rect.end;
            block->color = V4f(1, 1, 1, 1);
            block->texture = area->texture;
            block->fontTexture = globs.solidTex;
            block->clipStart = area->clipRect.start;
            block->clipEnd = area->clipRect.end;
            block->cornerRadius = area->style.cornerRadius;
            block->borderSize = area->style.borderSize;
            block->borderColor = area->style.borderColor;
        }


        _blu_genRenderCallsRecurse(area->firstChild[globs.linkSide], pass);
        area = area->nextSibling[globs.linkSide];
    }
}
void blu_makeDrawCalls(gfx_Pass* normalPass) {
    _blu_genRenderCallsRecurse(globs.ogParent, normalPass);
    _blu_genRenderCallsRecurse(globs.cursorParent, normalPass);
}










#define _BLU_DEFINE_STYLE_ADD(varName, type) \
    void blu_style_##varName(type varName, blu_Style* target) { \
        if(target == nullptr) { target = &globs.currentStyle->data; } \
        target->overrideFlags |= blu_styleFlags_##varName; \
        target->varName = varName; \
    } \

void blu_style_sizeX(blu_Size size, blu_Style* target) {
    if(target == nullptr) { target = &globs.currentStyle->data; }
    target->overrideFlags |= blu_styleFlags_sizeX;
    target->sizes[blu_axis_X] = size;
}
void blu_style_sizeY(blu_Size size, blu_Style* target) {
    if(target == nullptr) { target = &globs.currentStyle->data; }
    target->overrideFlags |= blu_styleFlags_sizeY;
    target->sizes[blu_axis_Y] = size;
}

_BLU_DEFINE_STYLE_ADD(childLayoutAxis, blu_Axis)
_BLU_DEFINE_STYLE_ADD(backgroundColor, V4f)
_BLU_DEFINE_STYLE_ADD(textColor, V4f)
_BLU_DEFINE_STYLE_ADD(textPadding, V2f)
_BLU_DEFINE_STYLE_ADD(animationStrength, F32)
_BLU_DEFINE_STYLE_ADD(cornerRadius, F32)
_BLU_DEFINE_STYLE_ADD(borderColor, V4f)
_BLU_DEFINE_STYLE_ADD(borderSize, F32)

#undef _BLU_DEFINE_STYLE_ADD



void blu_pushStyle(blu_Style style) {

    blu_StyleStackNode* s = BUMP_PUSH_NEW(&globs.frameArena, blu_StyleStackNode);
    s->data = style;

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


void blu_style_style(blu_Style* style, blu_Style* target) {
    if(target == nullptr) { target = &globs.currentStyle->data; }

    if(style->overrideFlags & blu_styleFlags_childLayoutAxis) {
        target->childLayoutAxis = style->childLayoutAxis; }
    if(style->overrideFlags & blu_styleFlags_sizeX) {
        target->sizes[blu_axis_X] = style->sizes[blu_axis_X]; }
    if(style->overrideFlags & blu_styleFlags_sizeY) {
        target->sizes[blu_axis_Y] = style->sizes[blu_axis_Y]; }
    if(style->overrideFlags & blu_styleFlags_backgroundColor) {
        target->backgroundColor = style->backgroundColor; }
    if(style->overrideFlags & blu_styleFlags_textColor) {
        target->textColor = style->textColor; }
    if(style->overrideFlags & blu_styleFlags_textPadding) {
        target->textPadding = style->textPadding; }
    if(style->overrideFlags & blu_styleFlags_animationStrength) {
        target->animationStrength = style->animationStrength; }
    if(style->overrideFlags & blu_styleFlags_cornerRadius) {
        target->cornerRadius = style->cornerRadius; }
    if(style->overrideFlags & blu_styleFlags_borderColor) {
        target->borderColor = style->borderColor; }
    if(style->overrideFlags & blu_styleFlags_borderSize) {
        target->borderSize = style->borderSize; }

    target->overrideFlags |= style->overrideFlags;
}






// CLEANUP: move
bool _blu_rectContainsPoint(Rect2f r, V2f point) {
    if(r.start.x <= point.x && r.end.x > point.x) {
        if(r.start.y <= point.y && r.end.y > point.y) {
            return true;
        }
    }
    return false;
}

// clears hover, pressed
// updates hover animation
void __blu_clearInteractionsRecurse(blu_Area* elem) {
    while(elem) {
        elem->prevHovered = false;
        elem->prevPressed = false;
        F32 scrollDelta = 0;

        __blu_clearInteractionsRecurse(elem->firstChild[globs.linkSide]);
        elem = elem->nextSibling[globs.linkSide];
    }
}

void __blu_updateInteractionsRecurse(blu_Area* elem) {
    while(elem) {

        if(elem->flags & blu_areaFlags_HOVER_ANIM) {
            bool hover = elem->prevHovered;
            elem->target_hoverAnim += (hover - elem->target_hoverAnim) * elem->style.animationStrength;
        } else { elem->target_hoverAnim = 0; }

        __blu_updateInteractionsRecurse(elem->firstChild[globs.linkSide]);
        elem = elem->nextSibling[globs.linkSide];
    }
}

// return indicates if parent has been blocked
bool __blu_genInteractionsRecurse(blu_Area* area, blu_Area* dragged, bool* outBlockSiblings, blu_Cursor* outCursor) {
    *outBlockSiblings = false;

    blu_Area* elem = area->lastChild;
    while(elem) {
        bool b;
        if(__blu_genInteractionsRecurse(elem, dragged, &b, outCursor)) { return true; }
        if(b) { break; }
        elem = elem->prevSibling;
    }


    bool containsMouse = _blu_rectContainsPoint(area->clipRect, globs.inputMousePos);
    if(!containsMouse) { return false; }

    bool clickable = (area->flags & blu_areaFlags_CLICKABLE) && (!dragged);
    if(clickable) { *outBlockSiblings = true; }

    bool recievesDrop = (dragged && dragged != area) && (area->flags & blu_areaFlags_DROP_EVENTS) && (area->dropTypeMask & dragged->dropType);
    if(!recievesDrop && !clickable) { return false; }

    *outCursor = area->cursor;
    area->prevHovered = true;
    area->prevPressed = globs.inputCurLButton && !globs.inputPrevLButton;
    if(area->prevPressed) { globs.dragged = area; }
    return true;
}

// return indicates blocking parent from scroll events
bool __blu_genScrollInteractionsRecurse(blu_Area* area, bool* outBlockSiblings, float scrollDelta) {
    *outBlockSiblings = false;

    blu_Area* elem = area->lastChild;
    while(elem) {
        bool b;
        if(__blu_genScrollInteractionsRecurse(elem, &b, scrollDelta)) { return true; }
        if(b) { break; }
        elem = elem->prevSibling;
    }

    if(!_blu_rectContainsPoint(area->clipRect, globs.inputMousePos)) { return false; }

    bool scrollable = (area->flags & blu_areaFlags_SCROLLABLE);
    if(!scrollable) { return false; }

    *outBlockSiblings = true;

    area->scrollDelta = scrollDelta;
    return true;
}

void blu_input(V2f npos, bool lmbState, float scrollDelta, blu_Cursor* outCursor) {

    globs.prevDragged = globs.dragged;
    if(!globs.inputCurLButton && globs.inputPrevLButton) {
        globs.dragged = nullptr;
    }

    globs.dragDelta = npos - globs.inputMousePos;
    globs.inputMousePos = npos;

    globs.inputPrevLButton = globs.inputCurLButton;
    globs.inputCurLButton = lmbState;


    *outCursor = blu_cursor_norm;
    if(globs.dragged) { *outCursor = globs.dragged->cursor; }

    __blu_clearInteractionsRecurse(globs.ogParent);
    bool x;
    __blu_genInteractionsRecurse(globs.ogParent, globs.dragged, &x, outCursor);
    if(globs.dragged == nullptr) {
        __blu_genScrollInteractionsRecurse(globs.ogParent, &x, scrollDelta);
    }
    __blu_updateInteractionsRecurse(globs.ogParent);
}



blu_WidgetInteraction blu_interactionFromWidget(blu_Area* area) {
    // ASSERT(area->flags & blu_areaFlags_CLICKABLE);

    blu_WidgetInteraction out = blu_WidgetInteraction();


    if(globs.prevDragged != area && globs.prevDragged) {
        if(area->prevHovered) {
            if(globs.prevDragged->dropType & area->dropTypeMask) {
                if(globs.inputPrevLButton && !globs.inputCurLButton) {
                    out.dropped = true;
                }

                out.dropType = globs.prevDragged->dropType;
                out.dropVal = globs.prevDragged->dropVal;
            }
        }
    }

    out.hovered = area->prevHovered;

    if(globs.dragged && globs.dragged != area) { return out; }


    out.scrollDelta = area->scrollDelta;

    if(out.hovered || globs.dragged == area) {
        out.mousePos = globs.inputMousePos - area->rect.start;
        out.hovered = true;
    }

    if(globs.dragged == area) {
        out.held = true;
        out.dragDelta = globs.dragDelta; // TODO: this isn't actually a drag delta, its a pos change

        if(!globs.inputCurLButton && globs.inputPrevLButton) {
            out.clicked = true; }
    }


    return out;
}



#endif