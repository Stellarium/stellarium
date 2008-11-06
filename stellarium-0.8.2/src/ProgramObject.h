#ifndef PROGRAM_OBJECT_H
#define PROGRAM_OBJECT_H

#include <list>

class Shader;

class ProgramObject
{
public:
	ProgramObject();
	~ProgramObject();

	void enable() const;
	void disable() const;

	void attachShader(const Shader* shader);
private:
	std::list<const Shader*> mShaders;

	unsigned int mProgramObject;
};

#endif // PROGRAM_OBJECT_H