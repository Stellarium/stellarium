#ifndef BUMP_SHADER_H
#define BUMP_SHADER_H

#include "Shader.h"
#include "vecmath.h"

class BumpShader : public Shader
{
public:
	virtual ~BumpShader();

	virtual void setParams(unsigned int programObject, int startActiveTex) const;
	virtual int getActiveTexCount() const;

	void setTexMap(unsigned int texMap);
	void setNormMap(unsigned int normMap);
	void setLightPos(const Vec3f& lightPos);

	static BumpShader* instance();
private:
	BumpShader();

	Vec3f mLightPos;
	unsigned int mTexMap;
	unsigned int mNormMap;
};

#endif // BUMP_SHADER_H