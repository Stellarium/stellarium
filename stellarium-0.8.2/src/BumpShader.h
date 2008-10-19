#ifndef BUMP_SHADER_H
#define BUMP_SHADER_H

#include "Shader.h"
#include "vecmath.h"

class BumpShader : public SourceShader
{
public:
	BumpShader();
	virtual ~BumpShader();

	void setTexMap(GLuint texMap);
	void setBumpMap(GLuint bumpMap);
	void setLightPos(const Vec3f& lightPos);
protected:
	virtual void setParams();
private:
	Vec3f mLightPos;
	GLuint mTexMap;
	GLuint mBumpMap;

	GLint mLightPosLoc;
	GLint mTexMapLoc;
	GLint mBumpMapLoc;
};

class BumpShaderPtr
{
public:
	BumpShaderPtr();

	BumpShader* operator ->();
private:
	BumpShaderPtr(const BumpShaderPtr&);
	BumpShaderPtr& operator =(const BumpShaderPtr&);

	static BumpShader* sBumpShader;
};

#endif // BUMP_SHADER_H