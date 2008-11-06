#ifndef SHADER_H
#define SHADER_H

#include <string>

class Shader
{
public:
	Shader(const std::string& vertShader, const std::string& fragShader);
	virtual ~Shader();

	unsigned int getVertShader() const;
	unsigned int getFragShader() const;

	virtual void setParams(unsigned int programObject, int startActiveTex) const = 0;
	virtual int getActiveTexCount() const = 0;
private:
	unsigned int mVertShader; 
	unsigned int mFragShader;
};

#endif // SHADER_H