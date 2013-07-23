#ifdef WIN32

#include "FontCache.h"
#include "cocos2d.h"

#define STBTT_malloc(x,u)  malloc(x)
#define STBTT_free(x,u)    free(x)
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

using namespace cocos2d;

static unsigned char *spSTBTT_BITMAP = NULL;

struct SYSFONT {
    stbtt_fontinfo  info;
    uint32_t  mmapSize;
    int  descent;
	float scale;
	float height;
	TEXTMETRIC metric;
	HFONT	hFont;
};

HDC ghDC;

bool FontData::initialize(void) {
    spSTBTT_BITMAP = new unsigned char[kGLYPH_WIDTH * kGLYPH_WIDTH];
    HDC hdc = GetDC(NULL);
    ghDC   = CreateCompatibleDC(hdc);
	ReleaseDC(NULL, hdc);
	return true;
}

bool FontData::createFont(const char *fontname, int fontsize, Handle& handle) {
	SYSFONT *font = new SYSFONT;

	//int nHeight = -MulDiv(fontsize, GetDeviceCaps(ghDC, LOGPIXELSY), 72);
	font->hFont = CreateFont(fontsize, 0, 0, 0,
		FW_DONTCARE, FALSE, FALSE, FALSE, 
		DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, VARIABLE_PITCH,
		fontname);
	SelectObject(ghDC, font->hFont);
	GetTextMetrics(ghDC, &font->metric);

	DWORD tag = 0x66637474;//ttcf
	font->mmapSize = GetFontData(ghDC, tag, 0, NULL, 0);
	if (-1 == font->mmapSize) {
		tag = 0;
		font->mmapSize = GetFontData(ghDC, tag, 0, NULL, 0);
	}
	void *pData = new char[font->mmapSize];
	GetFontData(ghDC, tag, 0, pData, font->mmapSize);

	font->height = fontsize;
    int start = stbtt_GetFontOffsetForIndex( (const unsigned char *)pData, 0);
    if (stbtt_InitFont(&font->info, (const unsigned char *)pData, start)) {
	    // Store normalized line height. The real line height is got by multiplying the lineh by font size.
    	int ascent, descent, lineGap;
	    stbtt_GetFontVMetrics(&font->info, &ascent, &descent, &lineGap);
	    //fh = ascent - descent;
	    //fnt->ascender = (float)ascent / (float)fh;
	    //fnt->descender = (float)descent / (float)fh;
	    //fnt->lineh = (float)(fh + lineGap) / (float)fh;
       	float scale = stbtt_ScaleForPixelHeight(&font->info, font->height);
        font->descent = (int)ceilf(descent*scale);
        font->scale = scale;
        handle = font;
        return true;
    }
    return false;
}

void FontData::destroyFont(const Handle& handle) {
    SYSFONT *font = (SYSFONT*)handle;
	DeleteObject(font->hFont);
	delete [] font->info.data;
    delete font;
	font = NULL;
}

void FontData::terminate(void) { 
	delete [] spSTBTT_BITMAP; 
	spSTBTT_BITMAP = NULL;
	//DeleteObject(ghBitmap);
	DeleteDC(ghDC);
}

void* FontData::getBitmap() { return spSTBTT_BITMAP; }

bool FontData::getGlyph(const Handle& handle, wchar_t codepoint, GlyphInfo& entry) {
    SYSFONT *sysfont = (SYSFONT*)handle;
    stbtt_fontinfo *font = &sysfont->info;
	int advance, lsb, x0, y0, x1, y1;
	int glyph = stbtt_FindGlyphIndex(font, codepoint);
    if (0 >= glyph)
        return false;

    float scale = sysfont->scale;
    int descent = sysfont->descent;
    //if (128 > codepoint) descent /= 2;

	stbtt_GetGlyphHMetrics(font, glyph, &advance, &lsb);
	stbtt_GetGlyphBitmapBox(font, glyph, scale, scale, &x0,&y0,&x1,&y1);

    int width = x1 - x0;
	int height = y1 - y0;
    entry.advance = ceilf(advance*scale);
    entry.offsetX = x0;
	entry.offsetY = sysfont->height + y0 + descent;
    entry.width = width;
    entry.height = height;

    memset(spSTBTT_BITMAP, 0, kGLYPH_WIDTH * kGLYPH_WIDTH);
    stbtt_MakeGlyphBitmap(font, spSTBTT_BITMAP, kGLYPH_WIDTH, kGLYPH_WIDTH, kGLYPH_WIDTH, scale, scale, glyph);
    /*
    spSTBTT_BITMAP[0]=255;
    spSTBTT_BITMAP[1]=255;
    spSTBTT_BITMAP[2]=255;
    spSTBTT_BITMAP[3]=255;
    */
    return true;
}

#endif//WIN32