/******************************************************************************
 Copyright (c) 2013 jbyu. All rights reserved.
 ******************************************************************************/

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import <CoreText/CoreText.h>
#import <string>
#import "FontCache.h"

static bool isValidFontName(const char *fontName) {
    bool ret = false;
    
    NSString *fontNameNS = [NSString stringWithUTF8String:fontName];
    
    for (NSString *familiName in [UIFont familyNames]) {
        if ([familiName isEqualToString:fontNameNS]) {
            ret = true;
            goto out;
        }
        
        for (NSString *font in [UIFont fontNamesForFamilyName: familiName]) {
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

bool FontData::initialize(void) {
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

bool FontData::createFont(const char *font_name, int size, Handle& font) {
	if (! isValidFontName(font_name)) {
		font_name = "Heiti TC";
	}
    CFStringRef name = CFStringCreateWithCString (NULL, font_name, kCFStringEncodingUTF8);
    //CGFontRef cgfont = CGFontCreateWithFontName (CFSTR("Helvetica"));
    //CTFontRef ctFont = CTFontCreateWithGraphicsFont(cgfont, 24,NULL);
    CTFontRef output = CTFontCreateWithName(name, size, NULL);
    CFRelease(name);
    if (output) {
        font = output;
        return true;
    }
    return false;
}

void FontData::destroyFont(const Handle& font) {
    //CGFontRelease( (CGFontRef) font );
    CFRelease( (CTFontRef) font);
}

void FontData::terminate(void) {
    CGContextRelease(spContext);
    delete [] spBitmapBuffer;
    spContext = NULL;
    spBitmapBuffer = NULL;
}

bool FontData::getGlyph(const Handle& font, wchar_t codepoint, GlyphInfo& entry) {
    CTFontRef tFont = (CTFontRef)font;
    UniChar characters[] = { (UniChar)codepoint };
    CGGlyph glyphs[] = {0};
    CGPoint positions[] = { CGPointMake(0, 0) };
    CGRect rects[1];
    
    // convert unicode to glyphs
    CTFontGetGlyphsForCharacters(tFont, characters, glyphs, 1);
    if (0 == glyphs[0])
        return false;
    
    CTFontGetBoundingRectsForGlyphs(tFont, kCTFontOrientationDefault, glyphs, rects, 1);
    double adv = CTFontGetAdvancesForGlyphs(tFont, kCTFontOrientationDefault, glyphs, NULL, 1);
    float ascent = CTFontGetAscent (tFont);
    float dscent = CTFontGetDescent (tFont);

    // offset glyph from bottom left to top left
    positions[0].x = - floorf( rects->origin.x );
    positions[0].y = - ceilf( rects->origin.y ) + 2.f;
    //positions[0].y = kCHAR_GRID_MAX - rects->size.height - rects->origin.y;
    
    entry.advance = (char)ceilf(adv);
    entry.width =   (char)ceilf(rects->size.width);
    entry.height = kGLYPH_WIDTH;
    //entry.height =  (char)ceilf(rects->size.height);
    entry.offsetX = (char)floorf(rects->origin.x);
    entry.offsetY = (char)ceilf(positions[0].y - dscent) + 1;
    if (128 > codepoint) {
        entry.advance = (char)ceilf(adv+0.5f);
        entry.width =   (char)ceilf(rects->size.width+0.5f);
        //entry.offsetY = dscent*0.5f;
    }
 
    //CGContextTranslateCTM(spContext, 0.0f, kCHAR_GRID_MAX);
    //CGContextScaleCTM(spContext, 1.0f, -1.0f); //NOTE: NSString draws in UIKit referential i.e. renders upside-down compared to CGBitmapContext referential
    UIGraphicsPushContext(spContext);
    CGContextClearRect (spContext, CGRectMake(0,0, kGLYPH_WIDTH, kGLYPH_WIDTH) );
    CTFontDrawGlyphs( tFont, glyphs, positions, 1, spContext );
    //CGContextFillRect(spContext, CGRectMake(0,31,16,1));
    //CGContextFillRect(spContext, CGRectMake(0,0,16,1));
    CGContextFlush(spContext);
    UIGraphicsPopContext();
    return true;
}

void* FontData::getBitmap() {
    return spBitmapBuffer;
}



