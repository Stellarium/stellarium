#ifndef SHADER_H
#define SHADER_H

#include <string>
#include "glew.h"

class Shader
{
public:
	virtual ~Shader() {}

	virtual void enable() = 0;
	virtual void disable() = 0;
};

class SourceShader : public Shader
{
public:
	SourceShader(const std::string& vertShader, const std::string& fragShader);
	virtual ~SourceShader();

	virtual void enable();
	virtual void disable();
protected:
	virtual void setParams() = 0;

	GLhandleARB getProgramObject()const;
private:
	GLhandleARB mVertShader; 
	GLhandleARB mFragShader;
	GLhandleARB mProgramObject;
};

#endif // SHADER_H