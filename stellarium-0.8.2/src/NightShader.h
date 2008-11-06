#ifndef NIGHT_SHADER_H
#define NIGHT_SHADER_H

#include "Shader.h"
#include "vecmath.h"

class NightShader : public Shader
{
public:
	virtual ~NightShader();

	virtual void setParams(unsigned int programObject, int startActiveTex) const;
	virtual int getActiveTexCount() const;

	void setLightPos(const Vec3f& lightPos);
	void setDayTexMap(unsigned int dayTexMap);
	void setNightTexMap(unsigned int nightTexMap);
	void setSpecularTexMap(unsigned int specularTexMap);

	static NightShader* instance();
private:
	NightShader();

	Vec3f mLightPos;
	unsigned int mDayTexMap;
	unsigned int mNightTexMap;
	unsigned int mSpecularTexMap;
};

#endif // NIGHT_SHADER_H