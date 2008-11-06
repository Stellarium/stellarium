#ifndef CLOUD_SHADER_H
#define CLOUD_SHADER_H

#include "Shader.h"
#include "vecmath.h"

class CloudShader : public Shader
{
public:
	virtual ~CloudShader();

	virtual void setParams(unsigned int programObject, int startActiveTex) const;
	virtual int getActiveTexCount() const;

	void setLightPos(const Vec3f& lightPos);
	void setTexMap(unsigned int texMap);
	void setShadowMap(unsigned int shadowMap);
	void setNormMap(unsigned int normMap);

	static CloudShader* instance();
private:
	CloudShader();

	Vec3f mLightPos;
	unsigned int mTexMap;
	unsigned int mShadowMap;
	unsigned int mNormMap;
};

#endif // CLOUD_SHADER_H