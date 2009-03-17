#ifndef NOISE_SHADER_H
#define NOISE_SHADER_H

#include "Shader.h"
#include "vecmath.h"

class NoiseShader : public Shader
{
public:
	virtual ~NoiseShader();

	virtual void setParams(unsigned int programObject, int startActiveTex) const;
	virtual int getActiveTexCount() const;

	void setTexMap(unsigned int texMap);

	static NoiseShader* instance();
private:
	NoiseShader();

	unsigned int mTexMap;
};

#endif // BUMP_SHADER_H