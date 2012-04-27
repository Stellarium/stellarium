#ifndef JAVAWRAPPER_HPP
#define JAVAWRAPPER_HPP

#ifdef ANDROID

// Provides calls to JNI in a C++/Qt-friendly manner.
// Right now, static calls. No instantiation.

#include <QString>
#include <jni.h>

class JavaWrapper
{
public:
	static int getDensityDpi();
	static float getDensity();
	static QString getLocaleString();
	static QString getExternalStorageDirectory();
	static bool isExternalStorageReady();

private:
	static void attachJni(JNIEnv* &env, jclass & javaClass);
};

#endif // ANDROID

#endif // JAVAWRAPPER_HPP
