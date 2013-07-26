/******************************************************************************
Copyright (c) 2013 jbyu. All rights reserved.
******************************************************************************/
#include "CCFlash.h"
#include "FontCache.h"
#include "cocos2d.h"

#pragma comment(lib, "libtinyswf.lib")

using namespace cocos2d;
using namespace tinyswf;

const GLuint kStencilMask = 0xffffffff;

const char textureShader_vert[] = "	\n\
attribute vec4 a_position;			\n\
uniform mat4 CC_TMatrix;			\n\
									\n\
#ifdef GL_ES						\n\
varying mediump vec2 v_texCoord;	\n\
#else								\n\
varying vec2 v_texCoord;			\n\
#endif								\n\
									\n\
void main()							\n\
{									\n\
    gl_Position = CC_MVPMatrix * a_position;	\n\
	v_texCoord = (CC_TMatrix * a_position).xy;		\n\
}									\n\
";
const char textureShader_frag[] = "		\n\
#ifdef GL_ES							\n\
precision highp float;					\n\
#endif									\n\
										\n\
uniform sampler2D CC_Texture0;			\n\
varying vec2 v_texCoord;				\n\
uniform	vec4 u_AddColor;				\n\
uniform	vec4 u_MultColor;				\n\
										\n\
void main()								\n\
{										\n\
	gl_FragColor = texture2D(CC_Texture0, v_texCoord) * u_MultColor + u_AddColor;	\n\
}										\n\
";

CCFlashRenderer::CCFlashRenderer()
	:miMaskLevel(0)
{
	// flash texture shader
    mpTextureShader = new GLProgram();
	mpTextureShader->initWithVertexShaderByteArray(textureShader_vert, textureShader_frag);
	mpTextureShader->addAttribute(kAttributeNamePosition, kVertexAttrib_Position);
    mpTextureShader->link();
    mpTextureShader->updateUniforms();
	miAddColorLocation = glGetUniformLocation( mpTextureShader->getProgram(), "u_AddColor");
	miMultColorLocation = glGetUniformLocation( mpTextureShader->getProgram(), "u_MultColor");
	miTextureMatrixLocation = glGetUniformLocation( mpTextureShader->getProgram(), "CC_TMatrix");
    CHECK_GL_ERROR_DEBUG();

	// default color only shader
	mpDefaultShader = ShaderCache::getInstance()->programForKey(kShader_Position_uColor);
	mpDefaultShader->retain();
	miColorLocation = glGetUniformLocation( mpDefaultShader->getProgram(), "u_color");
    CHECK_GL_ERROR_DEBUG();
}

CCFlashRenderer::~CCFlashRenderer() {
	CC_SAFE_RELEASE_NULL(mpDefaultShader);
	CC_SAFE_RELEASE_NULL(mpTextureShader);
	FlashTextureCache::iterator it = moCache.begin();
	while(moCache.end() != it) {
		Texture2D* tex = it->second;
		TextureCache::getInstance()->removeTexture(tex);
		++it;
	}
}

void CCFlashRenderer::maskBegin(void) {
	if (0 == miMaskLevel) {
		glEnable(GL_STENCIL_TEST);
	}
	// we set the stencil buffer to 'm_mask_level+1' 
	// where we draw any polygon and stencil buffer is 'm_mask_level'
	glStencilFunc(GL_EQUAL, miMaskLevel++, kStencilMask);
	glStencilOp(GL_KEEP, GL_KEEP, GL_INCR); 
	glColorMask(0, 0, 0, 0);
}

void CCFlashRenderer::maskEnd(void) {	     
	// we draw only where the stencil is m_mask_level (where the current mask was drawn)
	glStencilFunc(GL_EQUAL, miMaskLevel, kStencilMask);
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);	
	glColorMask(1, 1, 1, 1);
}

void CCFlashRenderer::maskOffBegin(void) {	     
	SWF_ASSERT(0 < miMaskLevel);
	// we set the stencil buffer back to 'm_mask_level' 
	// where the stencil buffer m_mask_level + 1
	glStencilFunc(GL_EQUAL, miMaskLevel--, kStencilMask);
	glStencilOp(GL_KEEP, GL_KEEP, GL_DECR); 
	glColorMask(0, 0, 0, 0);
}

void CCFlashRenderer::maskOffEnd(void) {	     
	maskEnd();
	if (0 == miMaskLevel) {
		glDisable(GL_STENCIL_TEST); 
	}
}

inline void MATRIX3f2kmMat4(kmMat4 &matrix, const tinyswf::MATRIX3f& mtx) {
    matrix.mat[0] = mtx.f[0];
    matrix.mat[1] = mtx.f[1];
    matrix.mat[2] = mtx.f[2];
    matrix.mat[3] = 0;
    matrix.mat[4] = mtx.f[3];
    matrix.mat[5] = mtx.f[4];
    matrix.mat[6] = mtx.f[5];
    matrix.mat[7] = 0;
    matrix.mat[8] = 0;
    matrix.mat[9] = 0;
    matrix.mat[10]= 1;
    matrix.mat[11]= 0;
    matrix.mat[12]= mtx.f[6];
    matrix.mat[13]= mtx.f[7];
    matrix.mat[14]= mtx.f[8];
    matrix.mat[15]= 1;
}

void CCFlashRenderer::applyTransform(const tinyswf::MATRIX3f& mtx) {
    kmMat4 matrix;
	MATRIX3f2kmMat4(matrix, mtx);
    kmGLLoadMatrix(&matrixMV);
	kmGLMultMatrix(&matrix);
}

void CCFlashRenderer::drawLineStrip(const tinyswf::VertexArray& vertices, const tinyswf::CXFORM& cxform, const tinyswf::LineStyle& style) {
	tinyswf::COLOR4f color = cxform.mult * style.getColor();
	color += cxform.add;
	glLineWidth(style.getWidth());

	ccGLEnableVertexAttribs( kVertexAttribFlag_Position );
    mpDefaultShader->use();
	mpDefaultShader->setUniformsForBuiltins();
	mpDefaultShader->setUniformLocationWith4fv(miColorLocation, (GLfloat*) &color.r, 1);
	glVertexAttribPointer(kVertexAttrib_Position, 2, GL_FLOAT, GL_FALSE, 0, &vertices.front().x);
	glDrawArrays(GL_LINE_STRIP, 0, vertices.size());
    CC_INCREMENT_GL_DRAWS(1);
}

void CCFlashRenderer::drawTriangles(const tinyswf::VertexArray& vertices, const tinyswf::CXFORM& cxform, const tinyswf::FillStyle& style, const tinyswf::Asset& asset) {
#define PRIMITIVE_MODE	GL_TRIANGLES
//#define PRIMITIVE_MODE	GL_POINTS

	ccGLEnableVertexAttribs( kVertexAttribFlag_Position );
	if (0 == asset.handle) {
		tinyswf::COLOR4f color = cxform.mult * style.getColor();
		color += cxform.add;

	    mpDefaultShader->use();
		mpDefaultShader->setUniformsForBuiltins();
		mpDefaultShader->setUniformLocationWith4fv(miColorLocation, (GLfloat*) &color.r, 1);

		glVertexAttribPointer(kVertexAttrib_Position, 2, GL_FLOAT, GL_FALSE, 0, &vertices.front().x);
		glDrawArrays(PRIMITIVE_MODE, 0, vertices.size());
	    CC_INCREMENT_GL_DRAWS(1);
		return;
	}

    kmMat4 matrix;
	MATRIX3f2kmMat4(matrix, style.getBitmapMatrix());

    mpTextureShader->use();
	mpTextureShader->setUniformsForBuiltins();
	mpTextureShader->setUniformLocationWithMatrix4fv(miTextureMatrixLocation, matrix.mat, 1);
	mpTextureShader->setUniformLocationWith4fv(miMultColorLocation, (GLfloat*) &cxform.mult.r, 1);
	mpTextureShader->setUniformLocationWith4fv(miAddColorLocation, (GLfloat*) &cxform.add.r, 1);

	Texture2D *texture = (Texture2D *)asset.handle;
	ccGLBindTexture2D( texture->getName() );
	if (style.type() & 0x1) {
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
	} else {
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
	}

	glVertexAttribPointer(kVertexAttrib_Position, 2, GL_FLOAT, GL_FALSE, 0, &vertices.front().x);
	glDrawArrays(PRIMITIVE_MODE, 0, vertices.size());
    CC_INCREMENT_GL_DRAWS(1);
}

void CCFlashRenderer::drawImportAsset( const tinyswf::RECT& rect, const tinyswf::CXFORM& cxform, const tinyswf::Asset& asset ) {
#if 0
    glDisable(GL_TEXTURE_2D);
    glBegin(GL_QUADS);
    glColor4f(1, 0, 0, 1);
		glTexCoord2f(0,0);
        glVertex2f(rect.xmin, rect.ymin);

    glColor4f(0, 1, 0, 1);
		glTexCoord2f(1,0);
        glVertex2f(rect.xmax, rect.ymin);

    glColor4f(0, 0, 1, 1);
    	glTexCoord2f(1,1);
        glVertex2f(rect.xmax, rect.ymax);

    glColor4f(1, 1, 0, 1);
		glTexCoord2f(0,1);
        glVertex2f(rect.xmin, rect.ymax);
	glEnd();
#endif
}
    
void CCFlashRenderer::drawBegin(void) {
	kmMat4 inverse;
	kmMat4Scaling(&inverse, 1,-1,1);
	kmGLMatrixMode(KM_GL_PROJECTION);
	kmGLPushMatrix();
	kmGLMultMatrix(&inverse);

	kmGLMatrixMode(KM_GL_MODELVIEW);
	kmGLPushMatrix();
	kmGLGetMatrix(KM_GL_MODELVIEW, &matrixMV);
	
	ccGLBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void CCFlashRenderer::drawEnd(void) {
	kmGLMatrixMode(KM_GL_PROJECTION);
	kmGLPopMatrix();
	kmGLMatrixMode(KM_GL_MODELVIEW);
	kmGLPopMatrix();
}

Texture2D* CCFlashRenderer::getTexture( const char *filename , int &width, int&height, int&x, int&y) {
    Texture2D* ret = 0;
    FlashTextureCache::iterator it = moCache.find(filename);
    if (moCache.end() != it)
        return it->second;

	ret = TextureCache::getInstance()->addImage(filename);
	width = ret->getPixelsWide();
	height = ret->getPixelsHigh();
	x = y = 0;

    moCache[filename] = ret;
    return ret;
}

//-----------------------------------------------------------------------------

tinyswf::Asset myLoadAssetCallback( const char *name, bool import ) {
	tinyswf::Asset asset = {import, 0, 0, 0};
	if (import) {
		asset.handle = 1;
		return asset;
	}

	if (strstr(name,".png")) {
		int x,y,w,h;
		CCFlashRenderer *renderer = (CCFlashRenderer*) tinyswf::Renderer::getInstance();
		asset.handle = (uint32_t)renderer->getTexture(name, w,h,x,y);
		const float invW = 1.f / w;
		const float invH = 1.f / h;
		asset.param[0] = tinyswf::SWF_TWIPS * invW;
		asset.param[1] = tinyswf::SWF_TWIPS * invH;
		asset.param[2] = x * invW;
		asset.param[3] = y * invH;
	} else if (strstr(name,".wav"))  {
		//asset.handle = tinyswf::Speaker::getSpeaker()->getSound(name);
	}
	return asset;
}

//-----------------------------------------------------------------------------
void myURLCallback( tinyswf::MovieClip&, bool isFSCommand, const char *url, const char *target )
{
	CCLOG("fs:%d, url:%s, targt:%s\n", isFSCommand, url, target);
}

CCFlash::~CCFlash() {
	delete mpSWF;
}

bool CCFlash::initWithFile(const char* filename) {
    unsigned long size = 0;
    unsigned char* pBuffer = FileUtils::getInstance()->getFileData(filename, "rb", &size);
    tinyswf::Reader reader((char*)pBuffer, size);
    mpSWF = new tinyswf::SWF;
    bool ret = mpSWF->read(reader);
    CC_SAFE_DELETE_ARRAY(pBuffer);

	this->setTouchEnabled(true);

	mpSWF->setGetURL( myURLCallback );

#if 1
	MATRIX3f& mtx = mpSWF->getCurrentMatrix();
    Point origin = Director::getInstance()->getVisibleOrigin();
	mtx.f[6] = origin.x;
	mtx.f[7] = origin.y;
#endif
	return true;
}

CCFlash * CCFlash::create(const char*filename) {
    CCFlash *node = new CCFlash();
    if(node && node->initWithFile(filename)) {
        node->autorelease();
        return node;
    }
    CC_SAFE_DELETE(node);
    return NULL;
}

void CCFlash::draw() {
	mpSWF->draw();
}

void CCFlash::update(float delta) {
	CCNode::update(delta);
	mpSWF->update(delta);
}

bool CCFlash::setString(const char* name, const char *text) {
	return mpSWF->setString(name, text);
}

void CCFlash::setColor(const Color3B &color)
{
    LayerRGBA::setColor(color);
    //updateColor();
}

void CCFlash::setOpacity(GLubyte opacity)
{
    LayerRGBA::setOpacity(opacity);
    //updateColor();
}

void CCFlash::registerWithTouchDispatcher()
{
    Director* pDirector = Director::getInstance();
    pDirector->getTouchDispatcher()->addTargetedDelegate(this, this->getTouchPriority(), true);
}

bool CCFlash::ccTouchBegan(Touch* touch, cocos2d::Event* event)
{
    CC_UNUSED_PARAM(event);
	Point pt = touch->getLocationInView();
	mpSWF->notifyMouse(1, pt.x, pt.y, true); 
    return true;
}

void CCFlash::ccTouchEnded(Touch *touch, cocos2d::Event* event)
{
    CC_UNUSED_PARAM(event);
	Point pt = touch->getLocationInView();
	mpSWF->notifyMouse(0, pt.x, pt.y, true); 
}

void CCFlash::ccTouchCancelled(Touch *touch, cocos2d::Event* event)
{
	ccTouchEnded(touch, event);
}

void CCFlash::ccTouchMoved(Touch* touch, cocos2d::Event* event)
{
    CC_UNUSED_PARAM(event);
	Point pt = touch->getLocationInView();
	mpSWF->notifyMouse(1, pt.x, pt.y, true); 
}
