/******************************************************************************
Copyright (c) 2013 jbyu. All rights reserved.
******************************************************************************/
#ifndef __CCFLASH_H__
#define __CCFLASH_H__

#include "tinyswf.h"
#include "cocos2d.h"

tinyswf::Asset myLoadAssetCallback( const char *name, bool import );

class CCFlash : public cocos2d::LayerRGBA {
	tinyswf::SWF *mpSWF;

public:
	virtual ~CCFlash();

    virtual void draw();

    virtual void update(float delta);

	virtual bool initWithFile(const char* filename);

	bool setString(const char* name, const char *text);

    virtual void registerWithTouchDispatcher();

    virtual bool ccTouchBegan(cocos2d::Touch* touch, cocos2d::Event* event) override;
    virtual void ccTouchEnded(cocos2d::Touch* touch, cocos2d::Event* event) override;
    virtual void ccTouchCancelled(cocos2d::Touch *touch, cocos2d::Event* event) override;
    virtual void ccTouchMoved(cocos2d::Touch* touch, cocos2d::Event* event) override;

    virtual void setColor(const cocos2d::Color3B &color) override;
    virtual void setOpacity(GLubyte opacity) override;

    static CCFlash* create(const char* filename);
};

class CCFlashRenderer : public tinyswf::Renderer {
    typedef std::map<std::string, cocos2d::Texture2D*> FlashTextureCache;
    FlashTextureCache moCache;
	cocos2d::GLProgram *mpDefaultShader;
	cocos2d::GLProgram *mpTextureShader;
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
    
    void drawBegin(void);
    void drawEnd(void);

    cocos2d::Texture2D* getTexture(const char *filename , int &width, int&height, int&x, int&y);
};


#endif//__CCFLASH_H__