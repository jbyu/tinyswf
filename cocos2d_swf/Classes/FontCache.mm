/******************************************************************************
 Copyright (c) 2013 jbyu. All rights reserved.
 ******************************************************************************/

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import <CoreText/CoreText.h>
#import <string>
#import "FontCache.h"

bool isValidFontName(const char *fontName) {
    bool ret = false;
    
    NSString *fontNameNS = [NSString stringWithUTF8String:fontName];
    for (NSString *familiName in [UIFont familyNames]) {
		//NSLog(@"font-family:%@", familiName);
        if ([familiName isEqualToString:fontNameNS]) {
            ret = true;
            goto out;
		}
        
        for (NSString *font in [UIFont fontNamesForFamilyName: familiName]) {
			//NSLog(@"font:%@", font);
            if ([font isEqualToString: fontNameNS]) {
                ret = true;
                goto out;
            }
        }
    }
out:
    return ret;
}

static CGContextRef spContext;

static char *spBitmapBuffer = NULL;

struct SYSFONT {
	CTFontRef hFont;
	float ascent;
	float descent;
	float line_height;
};

bool OSFont::initialize(void) {
    spBitmapBuffer = new char[kGLYPH_WIDTH * kGLYPH_WIDTH];
    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceGray();
    spContext = CGBitmapContextCreate(spBitmapBuffer, kGLYPH_WIDTH, kGLYPH_WIDTH, 8, kGLYPH_WIDTH, colorSpace, kCGBitmapByteOrderDefault);
    CGColorSpaceRelease(colorSpace);
    if (spContext) {
        CGContextSetRGBFillColor(spContext, 1, 1, 1, 1);
        return true;
    }
    return false;
}

OSFont::Handle OSFont::create(const char *font_name, float fontsize, int style) {
	CCLOG("create OS Font: %s", font_name);
	if (! isValidFontName(font_name)) {
		font_name = "HiraKakuProN-W6";
	}
    //CGFontRef cgfont = CGFontCreateWithFontName (CFSTR("Helvetica"));
    //CTFontRef ctFont = CTFontCreateWithGraphicsFont(cgfont, 24,NULL);
    CFStringRef name = CFStringCreateWithCString (NULL, font_name, kCFStringEncodingUTF8);
    CTFontRef hFont = CTFontCreateWithName(name, fontsize, NULL);
    CFRelease(name);
		
    if (hFont) {
		SYSFONT *font = new SYSFONT;
		font->hFont = hFont;
	    font->ascent = CTFontGetAscent(hFont);
		font->descent = CTFontGetDescent(hFont);
		font->line_height = font->ascent + font->descent + CTFontGetLeading(hFont);
	    font->ascent += CTFontGetLeading(hFont) * 0.5f;
        return font;
    }
    return NULL;
}

void OSFont::destroy(const Handle& handle) {
	SYSFONT *font = (SYSFONT*)handle;
    //CGFontRelease( (CGFontRef) font );
    CFRelease(font->hFont);
	delete font;
}

void OSFont::terminate(void) {
    CGContextRelease(spContext);
    delete [] spBitmapBuffer;
    spContext = NULL;
    spBitmapBuffer = NULL;
}

bool OSFont::makeGlyph(const Handle& handle, wchar_t codepoint, GlyphInfo& entry) {
	SYSFONT *font = (SYSFONT*)handle;
    UniChar characters[] = { (UniChar)codepoint };
    CGGlyph glyphs[] = {0};
    CGPoint positions[] = { CGPointMake(0, 0) };
    CGRect rects[1];
    
    // convert unicode to glyphs
    CTFontGetGlyphsForCharacters(font->hFont, characters, glyphs, 1);
    if (0 == glyphs[0])
        return false;
    
    CTFontGetBoundingRectsForGlyphs(font->hFont, kCTFontOrientationHorizontal, glyphs, rects, 1);
    double adv = CTFontGetAdvancesForGlyphs(font->hFont, kCTFontOrientationHorizontal, glyphs, NULL, 1);

    // offset glyph from bottom left to top left
    positions[0].x = - rects->origin.x;
    positions[0].y = (kGLYPH_WIDTH - rects->size.height) - rects->origin.y;
   
    entry.advance = (adv);
    entry.offsetX = (rects->origin.x);
	entry.offsetY = (font->ascent - rects->size.height) - rects->origin.y;
	
    entry.width = (char)ceilf(rects->size.width)+4;
    entry.height = (char)ceilf(rects->size.height)+4;
	//entry.height = kGLYPH_WIDTH;
	
    //CGContextTranslateCTM(spContext, 0.0f, kCHAR_GRID_MAX);
    //CGContextScaleCTM(spContext, 1.0f, -1.0f);
	//NOTE: NSString draws in UIKit referential i.e. renders upside-down compared to CGBitmapContext referential
    UIGraphicsPushContext(spContext);
    CGContextClearRect (spContext, CGRectMake(0,0, kGLYPH_WIDTH, kGLYPH_WIDTH) );
    CTFontDrawGlyphs( font->hFont, glyphs, positions, 1, spContext );
#if 0
	const int i = kGLYPH_WIDTH -1;
    CGContextFillRect(spContext, CGRectMake(0,i,i,1));
    CGContextFillRect(spContext, CGRectMake(0,0,i,1));
#endif
    CGContextFlush(spContext);
    UIGraphicsPopContext();
    return true;
}

float OSFont::getLineHeight(const Handle& handle) {
    SYSFONT *sysfont = (SYSFONT*)handle;
	return sysfont->line_height;
}

void* OSFont::getGlyphBitmap() {
    return spBitmapBuffer;
}

