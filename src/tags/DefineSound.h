/******************************************************************************
Copyright (c) 2013 jbyu. All rights reserved.
******************************************************************************/

#ifndef __DEFINE_SOUND_H__
#define __DEFINE_SOUND_H__

#include "tsTag.h"

namespace tinyswf {

class DefineSoundTag : public ITag
{
    uint16_t _soundId;

public:
	DefineSoundTag( TagHeader& h ) 
	: ITag( h )
	{}
		
	virtual ~DefineSoundTag()
    {}

	virtual bool read( Reader& reader, SWF& swf, MovieFrames& ) {
		_soundId = reader.get<uint16_t>();
        reader.skip( length()-2 );
		return false;//delete tag
	}

	virtual void print() {
    	_header.print();
		SWF_TRACE("id=%d\n", _soundId);
	}

	static ITag* create( TagHeader& header ) {
		return new DefineSoundTag( header );
	}				
};

//-----------------------------------------------------------------------------
class StartSoundTag : public ITag
{
    uint16_t _soundId;
    Asset _asset;
    bool _syncStop;
    bool _noMultiple;
    bool _loop;

public:
	StartSoundTag( TagHeader& h ) 
	: ITag( h )
	{}
		
	virtual ~StartSoundTag()
    {}

    virtual bool read( Reader& reader, SWF& swf, MovieFrames&) {
		_soundId = reader.get<uint16_t>();
        // read SOUNDINFO
        reader.getbits(2);//reserved
	    _syncStop           = reader.getbits(1)!=0;
        _noMultiple         = reader.getbits(1)!=0;
        uint32_t hasEnvelope= reader.getbits(1);
        _loop               = reader.getbits(1)!=0;
        uint32_t hasOutPoint= reader.getbits(1);
        uint32_t hasInPoint = reader.getbits(1);

        reader.skip( length()-3 );

        _asset = swf.getAsset(_soundId);
		return true; // keep tag
	}

	virtual void print() {
    	_header.print();
		SWF_TRACE("id=%d, stop:%d, noMultiple:%d, loop:%d\n", _soundId, _syncStop, _noMultiple, _loop);
	}

    virtual void setup(MovieClip&, bool skipAction)
    {
        Speaker *speaker = tinyswf::Speaker::getSpeaker();
        if (NULL==speaker) return;
        speaker->playSound(_asset.handle, _syncStop, _noMultiple, _loop);
    }

	static ITag* create( TagHeader& header ) {
		return new StartSoundTag( header );
	}				
};

}//namespace
#endif	// __DEFINE_SOUND_H__