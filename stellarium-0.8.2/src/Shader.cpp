#include "Shader.h"
#include <stdexcept>

using namespace std;

SourceShader::SourceShader(const string& vertShader, const string& fragShader)
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

	mProgramObject = glCreateProgram(); 
	if (!mProgramObject)
		throw runtime_error("Can't create program object");

	glAttachShader(mProgramObject, mVertShader);
	glAttachShader(mProgramObject, mFragShader); 

	glLinkProgramARB(mProgramObject); 
	glGetProgramivARB(mProgramObject, GL_LINK_STATUS, &status);
	if (!status)
		throw runtime_error("Can't link program object");
}

SourceShader::~SourceShader()
{
	disable();
	if (mVertShader)
		glDeleteObjectARB(mVertShader);
    if (mFragShader)
		glDeleteObjectARB(mFragShader);
    if (mProgramObject)
		glDeleteObjectARB(mProgramObject); 
}

void SourceShader::enable()
{
	glUseProgram(mProgramObject);
	setParams();
}

void SourceShader::disable()
{
	glUseProgram(0);
    glActiveTexture(GL_TEXTURE0);
}

GLhandleARB SourceShader::getProgramObject()const
{
	return mProgramObject;
}