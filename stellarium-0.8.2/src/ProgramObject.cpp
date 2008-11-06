#include "ProgramObject.h"
#include "Shader.h"
#include "glew.h"
#include <stdexcept>

using namespace std;

ProgramObject::ProgramObject()
{
	if (glewInit() != GLEW_OK)
		throw runtime_error("Error in GLEW init");

	mProgramObject = glCreateProgram(); 
	if (!mProgramObject)
		throw runtime_error("Can't create program object");
}

ProgramObject::~ProgramObject()
{
	disable();
	mShaders.clear();
	if (mProgramObject)
		glDeleteObjectARB(mProgramObject); 
}

void ProgramObject::enable() const
{
	if (!mShaders.empty())
	{
		glUseProgram(mProgramObject);
		int i = 0;
		for(list<const Shader*>::const_iterator it = mShaders.begin(); it != mShaders.end(); ++it)
		{
			(*it)->setParams(mProgramObject, i);
			i += (*it)->getActiveTexCount();
		}
	}
}

void ProgramObject::disable() const
{
	glUseProgram(0);
	glActiveTexture(GL_TEXTURE0);
}

void ProgramObject::attachShader(const Shader* shader)
{
	mShaders.push_back(shader);

	glAttachShader(mProgramObject, shader->getVertShader());
	glAttachShader(mProgramObject, shader->getFragShader()); 

	glLinkProgramARB(mProgramObject); 
	int status;
	glGetProgramivARB(mProgramObject, GL_LINK_STATUS, &status);
	if (!status)
		throw runtime_error("Can't link program object");
}