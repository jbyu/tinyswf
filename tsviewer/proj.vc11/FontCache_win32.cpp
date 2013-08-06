#include "stdafx.h"
#include <Windows.h>

#ifdef WIN32
#include "FontCache.h"

#define STBTT_malloc(x,u)  malloc(x)
#define STBTT_free(x,u)    free(x)
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

using namespace tinyswf;

static HDC ghDC;

static unsigned char *spSTBTT_BITMAP = NULL;

struct SYSFONT {
    stbtt_fontinfo  info;
	float ascent;
    float descent;
	float line_height;
	float scale;
	//TEXTMETRIC metric;
	//HFONT	hFont;
};


bool OSFont::initialize(void) {
    spSTBTT_BITMAP = new unsigned char[kGLYPH_WIDTH * kGLYPH_WIDTH];
    HDC hdc = GetDC(NULL);
    ghDC   = CreateCompatibleDC(hdc);
	ReleaseDC(NULL, hdc);
	return true;
}

enum FONT_STYLE {
	FLAG_ITALIC			= 0x02,
	FLAG_BOLD			= 0x01
};

OSFont::Handle OSFont::create(const char *fontname, float fontsize, int style) {
	SYSFONT *font = new SYSFONT;
	float height = fontsize * GetDeviceCaps(ghDC, LOGPIXELSY) / 72.f;

	bool bItalic = (FLAG_ITALIC & style) > 0;
	int weight = FW_NORMAL;
	if (FLAG_BOLD & style) {
		weight = FW_BOLD;
	}

	HFONT hFont = CreateFont((int)height, 0, 0, 0,
		weight, bItalic, FALSE, FALSE, 
		DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, VARIABLE_PITCH,
		fontname);
	SelectObject(ghDC, hFont);
	//GetTextMetrics(ghDC, &font->metric);

	DWORD tag = 0x66637474;//ttcf
	int size = GetFontData(ghDC, tag, 0, NULL, 0);
	if (-1 == size) {
		tag = 0;
		size = GetFontData(ghDC, tag, 0, NULL, 0);
	}
	void *pData = new char[size];
	GetFontData(ghDC, tag, 0, pData, size);
	DeleteObject(hFont);

    int start = stbtt_GetFontOffsetForIndex( (const unsigned char *)pData, 0);
	if (0 != tag) {
		//start = stbtt_FindMatchingFont((const unsigned char *)pData, fontname, STBTT_MACSTYLE_DONTCARE);
	}
	//CCASSERT(0 <= start, "no such font!");
    if (stbtt_InitFont(&font->info, (const unsigned char *)pData, start)) {
	    // Store normalized line height. The real line height is got by multiplying the lineh by font size.
    	int ascent, descent, lineGap;
	    stbtt_GetFontVMetrics(&font->info, &ascent, &descent, &lineGap);
	    //fh = ascent - descent;
	    //fnt->ascender = (float)ascent / (float)fh;
	    //fnt->descender = (float)descent / (float)fh;
	    //fnt->lineh = (float)(fh + lineGap) / (float)fh;
       	float scale = stbtt_ScaleForPixelHeight(&font->info, height);
		font->ascent = ascent * scale;
        font->descent = descent * scale;
		font->line_height = (ascent - descent + lineGap) * scale;
        font->scale = scale;
        return font;
    }
	delete font;
    return NULL;
}

void OSFont::destroy(const Handle& handle) {
    SYSFONT *font = (SYSFONT*)handle;
	delete [] font->info.data;
    delete font;
	font = NULL;
}

void OSFont::terminate(void) { 
	delete [] spSTBTT_BITMAP; 
	spSTBTT_BITMAP = NULL;
	//DeleteObject(ghBitmap);
	DeleteDC(ghDC);
}

void* OSFont::getGlyphBitmap() { return spSTBTT_BITMAP; }

float OSFont::getLineHeight(const Handle& handle) {
    SYSFONT *sysfont = (SYSFONT*)handle;
	return sysfont->line_height;
}

bool OSFont::makeGlyph(const Handle& handle, wchar_t codepoint, GlyphInfo& entry) {
    SYSFONT *sysfont = (SYSFONT*)handle;
    stbtt_fontinfo *font = &sysfont->info;
	int advance, lsb, x0, y0, x1, y1;
	int glyph = stbtt_FindGlyphIndex(font, codepoint);
    if (0 >= glyph)
        return false;

    float scale = sysfont->scale;
    
	stbtt_GetGlyphHMetrics(font, glyph, &advance, &lsb);
	stbtt_GetGlyphBitmapBox(font, glyph, scale, scale, &x0,&y0,&x1,&y1);

    int width = x1 - x0 +1;
	int height = y1 - y0+1;
    entry.advance = advance * scale;
    entry.offsetX = float(x0);
	entry.offsetY = sysfont->ascent + y0;
	entry.width = width;
    entry.height = height;

    memset(spSTBTT_BITMAP, 0, kGLYPH_WIDTH * kGLYPH_WIDTH);
    stbtt_MakeGlyphBitmap(font, spSTBTT_BITMAP, kGLYPH_WIDTH, kGLYPH_WIDTH, kGLYPH_WIDTH, scale, scale, glyph);

#if 0
	const int offset = kGLYPH_WIDTH * (kGLYPH_WIDTH - 1);
	for(int i = 0; i < kGLYPH_WIDTH; ++i) {
		spSTBTT_BITMAP[i]=255;
	    spSTBTT_BITMAP[i+offset]=255;
	}
#endif    
    return true;
}

#endif//WIN32