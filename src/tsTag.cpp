/******************************************************************************
Copyright (c) 2013 jbyu. All rights reserved.
******************************************************************************/

#include "tsTag.h"
#include "tsReader.h"

using namespace std;

namespace tinyswf {

#ifdef SWF_DEBUG
    std::map<uint32_t, const char*> TagHeader::_tagNames;

    int TagHeader::initialize() {
        _tagNames[TAG_END]="END";
	    _tagNames[TAG_SHOW_FRAME]="SHOW_FRAME";
	    _tagNames[TAG_DEFINE_SHAPE]="DEFINE_SHAPE";
	    _tagNames[TAG_PLACE_OBJECT]="PLACE_OBJECT";
	    _tagNames[TAG_REMOVE_OBJECT]="REMOVE_OBJECT";
	    _tagNames[TAG_DEFINE_BITS]="DEFINE_BITS";
	    _tagNames[TAG_DEFINE_BUTTON]="DEFINE_BUTTON";
	    _tagNames[TAG_JPEG_TABLES]="JPEG_TABLES";
	    _tagNames[TAG_SET_BACKGROUND_COLOR]="SET_BACKGROUND_COLOR";
	    _tagNames[TAG_DEFINE_FONT]="DEFINE_FONT";
	    _tagNames[TAG_DEFINE_TEXT]="DEFINE_TEXT";
	    _tagNames[TAG_DO_ACTION]="DO_ACTION";
	    _tagNames[TAG_DEFINE_FONT_INFO]="DEFINE_FONT_INFO";
	    _tagNames[TAG_DEFINE_SOUND]="DEFINE_SOUND";
	    _tagNames[TAG_START_SOUND]="START_SOUND";
	    _tagNames[TAG_DEFINE_BUTTON_SOUND]="DEFINE_BUTTON_SOUND";
	    _tagNames[TAG_SOUND_STREAM_HEAD]="SOUND_STREAM_HEAD";
	    _tagNames[TAG_SOUND_STREAM_BLOCK]="SOUND_STREAM_BLOCK";
	    _tagNames[TAG_DEFINE_BITS_LOSSLESS]="DEFINE_BITS_LOSSLESS";
	    _tagNames[TAG_DEFINE_BITS_JPEG2]="DEFINE_BITS_JPEG2";
	    _tagNames[TAG_DEFINE_SHAPE2]="DEFINE_SHAPE2";
	    _tagNames[TAG_DEFINE_BUTTON_CXFORM]="DEFINE_BUTTON_CXFORM";
	    _tagNames[TAG_PROTECT]="PROTECT";
	    _tagNames[TAG_PLACE_OBJECT2]="PLACE_OBJECT2";
	    _tagNames[TAG_REMOVE_OBJECT2]="REMOVE_OBJECT2";
	    _tagNames[TAG_DEFINE_SHAPE3]="DEFINE_SHAPE3";
	    _tagNames[TAG_DEFINE_TEXT2]="DEFINE_TEXT2";
	    _tagNames[TAG_DEFINE_BUTTON2]="DEFINE_BUTTON2";
	    _tagNames[TAG_DEFINE_BITS_JPEG3]="DEFINE_BITS_JPEG3";
	    _tagNames[TAG_DEFINE_BITS_LOSSLESS2]="DEFINE_BITS_LOSSLESS2";
	    _tagNames[TAG_DEFINE_EDIT_TEXT]="DEFINE_EDIT_TEXT";
	    _tagNames[TAG_DEFINE_SPRITE]="DEFINE_SPRITE";
	    _tagNames[TAG_FRAME_LABEL]="FRAME_LABEL";
	    _tagNames[TAG_SOUND_STREAM_HEAD2]="SOUND_STREAM_HEAD2";
	    _tagNames[TAG_DEFINE_MORPH_SHAPE]="DEFINE_MORPH_SHAPE";
	    _tagNames[TAG_DEFINE_FONT2]="DEFINE_FONT2";
	    _tagNames[TAG_EXPORT_ASSETS]="EXPORT_ASSETS";
	    _tagNames[TAG_IMPORT_ASSETS]="IMPORT_ASSETS";
	    _tagNames[TAG_ENABLE_DEBUGGER]="ENABLE_DEBUGGER";
	    _tagNames[TAG_DO_INIT_ACTION]="DO_INIT_ACTION";
	    _tagNames[TAG_DEFINE_VIDEO_STREAM]="DEFINE_VIDEO_STREAM";
	    _tagNames[TAG_VIDEO_FRAME]="VIDEO_FRAME";
	    _tagNames[TAG_DEFINE_FONT_INFO2]="DEFINE_FONT_INFO2";
	    _tagNames[TAG_ENABLE_DEBUGGER2]="ENABLE_DEBUGGER2";
	    _tagNames[TAG_SCRIPT_LIMITS]="SCRIPT_LIMITS";
	    _tagNames[TAG_SET_TAB_INDEX]="SET_TAB_INDEX";
	    _tagNames[TAG_FILE_ATTRIBUTES]="FILE_ATTRIBUTES";
	    _tagNames[TAG_PLACE_OBJECT3]="PLACE_OBJECT3";
	    _tagNames[TAG_IMPORT_ASSETS2]="IMPORT_ASSETS2";
	    _tagNames[TAG_DEFINE_FONT_ALIGN_ZONES]="DEFINE_FONT_ALIGN_ZONES";
	    _tagNames[TAG_DEFINE_CSM_TEXT_SETTINGS]="DEFINE_CSM_TEXT_SETTINGS";
	    _tagNames[TAG_DEFINE_FONT3]="DEFINE_FONT3";
	    _tagNames[TAG_SYMBOL_CLASS]="SYMBOL_CLASS";
	    _tagNames[TAG_METADATA]="METADATA";
	    _tagNames[TAG_DEFINE_SCALING_GRID]="DEFINE_SCALING_GRID";
	    _tagNames[TAG_DO_ABC]="DO_ABC";
	    _tagNames[TAG_DEFINE_SHAPE4]="DEFINE_SHAPE4";
	    _tagNames[TAG_DEFINE_MORPH_SHAPE2]="DEFINE_MORPH_SHAPE2";
	    _tagNames[TAG_DEFINE_SCENE_AND_FRAME_LABEL_DATA]="DEFINE_SCENE_AND_FRAME_LABEL_DATA";
	    _tagNames[TAG_DEFINE_BINARY_DATA]="DEFINE_BINARY_DATA";
	    _tagNames[TAG_DEFINE_FONT_NAME]="DEFINE_FONT_NAME";
	    _tagNames[TAG_DEFINE_START_SOUND2]="DEFINE_START_SOUND2";
	    _tagNames[TAG_DEFINE_BITS_JPEG4]="DEFINE_BITS_JPEG4";
	    _tagNames[TAG_DEFINE_FONT4]="DEFINE_FONT4";
        return 0;
    }
    const int kTagInitialize = TagHeader::initialize();
#endif

	bool TagHeader::read( Reader& reader ) {
		uint16_t tagcode_and_length = reader.get<uint16_t>();
		
		_code = (tagcode_and_length >> 6 );
		_length = (tagcode_and_length & 0x3F );
		if( _length == 0x3f )	// if long tag read an additional 32 bit length value
			_length = reader.get<uint32_t>();
				
		return true;
	}
	
	void TagHeader::print() {
		SWF_TRACE("%s\tcode:%2d, length:%d\n", name(), _code , _length);
	}


void MATRIX::transform(RECT& result, const RECT& p) const {
	POINT in[4] = {
		{p.xmin, p.ymin},
		{p.xmin, p.ymax},
		{p.xmax, p.ymin},
		{p.xmax, p.ymax}
	};
	RECT rect = {FLT_MAX,FLT_MAX,-FLT_MAX,-FLT_MAX};
	for (int i = 0; i < 4; ++i) {
		POINT other;
		transform(other, in[i]);
		if (other.x < rect.xmin)
			rect.xmin = other.x;
		else if (other.x > rect.xmax)
			rect.xmax = other.x;
		if (other.y < rect.ymin)
			rect.ymin = other.y;
		else if (other.y > rect.ymax)
			rect.ymax = other.y;
	}
	result = rect;
}

}//namespace