//
//  GlyphInfo.h
//  TF
//
//  Created by phenix on 12/12/26.
//  Copyright (c) 2012 Nubee Pte Ltd. All rights reserved.
//

#ifndef TF_GlyphInfo_h
#define TF_GlyphInfo_h

struct SGlyphInfo {
	char	advance;
	char	width;
	char	height;
	char	offsetX;
	char	offsetY;
	char	padding;
    unsigned short   index;
};

const int kCHAR_GRID_MAX = 32;

typedef const void* FontHandle;

bool SysFont_Initialize(void);
bool SysFont_Create(const char *filename, int fontsize, FontHandle& font);
void SysFont_Destroy(const FontHandle& font);
void SysFont_Terminate(void);
bool SysFont_GetGlyph(const FontHandle& font, unsigned int codepoint, SGlyphInfo& entry);
void*SysFont_GetBitmap();

#endif//TF_GlyphInfo_h
