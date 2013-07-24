/******************************************************************************
Copyright (c) 2013 jbyu. All rights reserved.
******************************************************************************/
#ifndef __CCFLASH_H__
#define __CCFLASH_H__

#include "tinyswf.h"
#include "kazmath/mat4.h"
#include "base_nodes/CCNode.h"

namespace cocos2d {
	class Texture2D;
	class GLProgram;
}

tinyswf::Asset myLoadAssetCallback( const char *name, bool import );

class CCFlash : public cocos2d::Node {
	tinyswf::SWF *mpSWF;

public:
	virtual ~CCFlash();

    virtual void draw();

    virtual void update(float delta);

	virtual bool initWithFile(const char* filename);

	bool setString(const char* name, const char *text);

    static CCFlash* create(const char* filename);
};

class CCFlashRenderer : public tinyswf::Renderer {
    typedef std::map<std::string, cocos2d::Texture2D*> TextureCache;
    TextureCache moCache;
	cocos2d::GLProgram *mpFontShader;
	cocos2d::GLProgram *mpDefaultShader;
	cocos2d::GLProgram *mpTextureShader;
	int miFontUVScaleLocation;
	int miFontColorLocation;
	int miColorLocation;
	int miAddColorLocation;
	int miMultColorLocation;
	int miTextureMatrixLocation;
	int miMaskLevel;
	kmMat4 matrixMV;

public:
    CCFlashRenderer();
    virtual ~CCFlashRenderer();

	void maskBegin(void);
	void maskEnd(void);
	void maskOffBegin(void);
	void maskOffEnd(void);

    void applyTransform(const tinyswf::MATRIX3f& mtx );
    
	void drawLineStrip(const tinyswf::VertexArray& vertices, const tinyswf::CXFORM& cxform, const tinyswf::LineStyle& style);

	void drawTriangles(const tinyswf::VertexArray& vertices, const tinyswf::CXFORM& cxform, const tinyswf::FillStyle& style, const tinyswf::Asset& asset );

    void drawImportAsset( const tinyswf::RECT& rect, const tinyswf::CXFORM& cxform, const tinyswf::Asset& asset );
    
    void formatText(tinyswf::VertexArray& vertices, const tinyswf::RECT& rect, const tinyswf::TextStyle& style, const std::wstring& text);
	void drawText(const tinyswf::VertexArray& vertices, const tinyswf::RECT& rect, const tinyswf::TextStyle& style, const std::wstring& text);

    void drawBegin(void);
    void drawEnd(void);

    cocos2d::Texture2D* getTexture(const char *filename , int &width, int&height, int&x, int&y);
};


#endif//__CCFLASH_H__