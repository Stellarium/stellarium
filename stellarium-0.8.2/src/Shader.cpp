#include "Shader.h"
#include "glew.h"
#include <stdexcept>

using namespace std;

Shader::Shader(const string& vertShader, const string& fragShader)
{
	if (glewInit() != GLEW_OK)
		throw runtime_error("Error in GLEW init");

	mVertShader = glCreateShader(GL_VERTEX_SHADER_ARB);
	if (!mVertShader)
		throw runtime_error("Can't create vertex shader");
	mFragShader = glCreateShader(GL_FRAGMENT_SHADER_ARB);
	if (!mFragShader)
		throw runtime_error("Can't create fragment shader");

	const char* shaderstring = vertShader.c_str();
	glShaderSourceARB(mVertShader, 1, &shaderstring, NULL);
	shaderstring = fragShader.c_str();
	glShaderSourceARB(mFragShader, 1, &shaderstring, NULL);
	
	glCompileShader(mVertShader);
	GLint  status = 0;
	glGetShaderiv(mVertShader, GL_COMPILE_STATUS, &status);
	if (!status)
		throw runtime_error("Can't compile vertex shader");
	glCompileShader(mFragShader); 
	glGetShaderiv(mFragShader, GL_COMPILE_STATUS, &status);
	if (!status)
		throw runtime_error("Can't compile fragment shader");
}

Shader::~Shader()
{
	if (mVertShader)
		glDeleteObjectARB(mVertShader);
    if (mFragShader)
		glDeleteObjectARB(mFragShader);
}

unsigned int Shader::getVertShader() const
{
	return mVertShader;
}

unsigned int Shader::getFragShader() const
{
	return mFragShader;
}