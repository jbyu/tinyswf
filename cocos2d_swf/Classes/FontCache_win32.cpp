#ifdef WIN32

#include "FontCache.h"
#include "cocos2d.h"

//#define USE_STB_TRUETYPE	1

#ifdef USE_STB_TRUETYPE
#define STBTT_malloc(x,u)  malloc(x)
#define STBTT_free(x,u)    free(x)
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
#endif

using namespace cocos2d;
using namespace tinyswf;

static HDC ghDC;

static unsigned char *spSTBTT_BITMAP = NULL;
const int kSize = kGLYPH_WIDTH * kGLYPH_WIDTH;

struct SYSFONT {
#ifdef USE_STB_TRUETYPE
    stbtt_fontinfo  info;
	float scale;
#endif
	float ascent;
    float descent;
	float line_height;
	float extra;
	HFONT hFont;
};

bool OSFont::initialize(void) {
    spSTBTT_BITMAP = new unsigned char[kSize*2];
    HDC hdc = GetDC(NULL);
    ghDC   = CreateCompatibleDC(hdc);
	ReleaseDC(NULL, hdc);
	return true;
}

enum FONT_STYLE {
	FLAG_ITALIC			= 0x02,
	FLAG_BOLD			= 0x01
};

inline FIXED FixedFromFloat(float d) {
	long l = (long) (d * 65536L);
	return *(FIXED *)&l;
}
inline void SetMat(LPMAT2 lpMat) {
	lpMat->eM11 = FixedFromFloat(1);
	lpMat->eM12 = FixedFromFloat(0);
	lpMat->eM21 = FixedFromFloat(0);
	lpMat->eM22 = FixedFromFloat(1);
}

OSFont::Handle OSFont::create(const char *fontname, float fontsize, int style) {
	SYSFONT *font = new SYSFONT;
	float height = fontsize * GetDeviceCaps(ghDC, LOGPIXELSY) / 72.f;

	bool bItalic = (FLAG_ITALIC & style) > 0;
	int weight = FW_NORMAL;
	if (FLAG_BOLD & style) {
		weight = FW_BOLD;
	}

	font->hFont = CreateFont((int)height, 0, 0, 0,
		weight, bItalic, FALSE, FALSE, 
		DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, VARIABLE_PITCH,
		fontname);
	SelectObject(ghDC, font->hFont);
	TEXTMETRIC metric;
	GetTextMetrics(ghDC, &metric);

	font->ascent = (float)metric.tmAscent;
	font->descent = (float)metric.tmDescent;
	font->line_height = float(metric.tmHeight + metric.tmExternalLeading);
	font->extra = float(metric.tmExternalLeading/2);
    return font;

#ifdef USE_STB_TRUETYPE
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
		start = stbtt_FindMatchingFont((const unsigned char *)pData, fontname, STBTT_MACSTYLE_DONTCARE);
	}
	CCASSERT(0 <= start, "no such font!");
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
#endif
    return NULL;
}

void OSFont::destroy(const Handle& handle) {
    SYSFONT *font = (SYSFONT*)handle;
#ifdef USE_STB_TRUETYPE
	//delete [] font->info.data;
#else 
	DeleteObject(font->hFont);
#endif
    delete font;
	font = NULL;
}

void OSFont::terminate(void) { 
	delete [] spSTBTT_BITMAP; 
	spSTBTT_BITMAP = NULL;
	DeleteDC(ghDC);
}

void* OSFont::getGlyphBitmap() { return spSTBTT_BITMAP; }

float OSFont::getLineHeight(const Handle& handle) {
    SYSFONT *sysfont = (SYSFONT*)handle;
	return sysfont->line_height;
}

bool OSFont::makeGlyph(const Handle& handle, wchar_t codepoint, GlyphInfo& entry) {
#ifdef USE_STB_TRUETYPE
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

#else
    memset(spSTBTT_BITMAP, 0, kSize);

    SYSFONT *sysfont = (SYSFONT*)handle;
	SelectObject(ghDC, sysfont->hFont);

	MAT2 mat2;
	SetMat(&mat2);
	GLYPHMETRICS gm;
	DWORD dwNeedSize = GetGlyphOutlineW(ghDC, codepoint, GGO_GRAY8_BITMAP, &gm, 0, NULL, &mat2);

	CCASSERT(dwNeedSize <= kSize, "exceed buffer");
	BYTE *dst = spSTBTT_BITMAP+1;
	BYTE *src = spSTBTT_BITMAP + kSize;
	GetGlyphOutlineW(ghDC, codepoint, GGO_GRAY8_BITMAP, &gm, dwNeedSize, src, &mat2);

	int bytesPerRow = dwNeedSize / gm.gmBlackBoxY;
	for (UINT y = 0; y < gm.gmBlackBoxY; ++y) {  
		for (int x = 0; x < bytesPerRow; ++x) {
			int value = src[x] * 4;
			dst[x] = (0xff < value) ? 0xff : value;
		}
		src += bytesPerRow;
		dst += kGLYPH_WIDTH;
	}

	entry.advance = (float)gm.gmCellIncX;
    entry.offsetX = (float)gm.gmptGlyphOrigin.x;
	entry.offsetY = sysfont->ascent - gm.gmptGlyphOrigin.y + sysfont->extra;
	entry.width = gm.gmBlackBoxX+4;
	entry.height = gm.gmBlackBoxY+4;
	//entry.width = kGLYPH_WIDTH;
	//entry.height = kGLYPH_WIDTH;

#endif
#if 0
	const int offset = kSize - kGLYPH_WIDTH;
	for(int i = 0; i < kGLYPH_WIDTH; ++i) {
		spSTBTT_BITMAP[i]=255;
	    spSTBTT_BITMAP[i+offset]=255;
	}
#endif    
    return true;
}

#endif//WIN32