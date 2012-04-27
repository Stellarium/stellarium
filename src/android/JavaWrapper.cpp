#ifdef ANDROID

#include "JavaWrapper.hpp"
#include <jni.h>
#include <QtGlobal>
#include <QDebug>

extern JavaVM *m_javaVM;
extern jobject objptr;
extern jobject customClassPtr;

void JavaWrapper::attachJni(JNIEnv* &env, jclass &javaClass)
{
	if (m_javaVM->AttachCurrentThread(&env, NULL)<0)
	{
		qCritical()<<"JavaWrapper::attachJni - AttachCurrentThread failed";
		javaClass = NULL;
		return;
	}

	javaClass = env->GetObjectClass(customClassPtr);
}

int JavaWrapper::getDensityDpi()
{
	JNIEnv* env = NULL;
	jclass javaClass = NULL;

	if (m_javaVM->AttachCurrentThread(&env, NULL)<0)
	{
		qCritical()<<"JavaWrapper::attachJni - AttachCurrentThread failed";
		javaClass = NULL;
		return 160;
	}

	javaClass = env->GetObjectClass(customClassPtr);

	if(javaClass)
	{
		jmethodID getDensityDpiId = env->GetStaticMethodID(javaClass,"getDensityDpi", "()I");
		int retval = static_cast<int>(env->CallStaticIntMethod(javaClass, getDensityDpiId));
		m_javaVM->DetachCurrentThread();
		return retval;
	}
	else
	{
		qCritical() << "JavaWrapper::getDensityDpi - Java class failed to load!";
		m_javaVM->DetachCurrentThread();
		return 160;
	}
}

float JavaWrapper::getDensity()
{
	JNIEnv* env = NULL;
	jclass javaClass = NULL;
	if (m_javaVM->AttachCurrentThread(&env, NULL)<0)
	{
		qCritical()<<"JavaWrapper::attachJni - AttachCurrentThread failed";
		javaClass = NULL;
		return 1;
	}

	javaClass = env->GetObjectClass(customClassPtr);

	if(javaClass)
	{
		jmethodID getDensityId = env->GetStaticMethodID(javaClass,"getDensity", "()F");
		float retval = static_cast<float>(env->CallStaticFloatMethod(javaClass, getDensityId));
		m_javaVM->DetachCurrentThread();
		return retval;
	}
	else
	{
		qCritical() << "JavaWrapper::getDensity - Java class failed to load!";
		m_javaVM->DetachCurrentThread();
		return 1;
	}
}

QString JavaWrapper::getLocaleString()
{
	JNIEnv* env = NULL;
	jclass javaClass = NULL;
	if (m_javaVM->AttachCurrentThread(&env, NULL)<0)
	{
		qCritical()<<"JavaWrapper::attachJni - AttachCurrentThread failed";
		javaClass = NULL;
		return QString("C");
	}

	javaClass = env->GetObjectClass(customClassPtr);

	if(javaClass)
	{
		jmethodID getLocaleStringId = env->GetStaticMethodID(javaClass,"getLocaleString", "()Ljava/lang/String;");
		jstring javaString = static_cast<jstring>(env->CallStaticObjectMethod(javaClass, getLocaleStringId));

		jboolean isCopy;
		const char* cString = env->GetStringUTFChars(javaString,&isCopy);

		if(!cString)
		{
			qCritical() << "JavaWrapper::getLocaleString - String failed to load!";
			return QString("C");
		}

		QString qtString(cString);
		env->ReleaseStringUTFChars(javaString, cString);
		m_javaVM->DetachCurrentThread();
		return qtString;
	}
	else
	{
		qCritical() << "JavaWrapper::getLocaleString - Java class failed to load!";
		m_javaVM->DetachCurrentThread();
		return QString("C");
	}
}

QString JavaWrapper::getExternalStorageDirectory()
{
	JNIEnv* env = NULL;
	jclass javaClass = NULL;
	if (m_javaVM->AttachCurrentThread(&env, NULL)<0)
	{
		qCritical()<<"JavaWrapper::getExternalStorageDirectory - AttachCurrentThread failed";
		javaClass = NULL;
		return QString("/mnt/sdcard");
	}

	javaClass = env->GetObjectClass(customClassPtr);

	if(javaClass)
	{
		jmethodID getExternalStorageDirectory = env->GetStaticMethodID(javaClass,"getExternalStorageDirectory", "()Ljava/lang/String;");
		jstring javaString = static_cast<jstring>(env->CallStaticObjectMethod(javaClass, getExternalStorageDirectory));

		jboolean isCopy;
		const char* cString = env->GetStringUTFChars(javaString,&isCopy);

		if(!cString)
		{
			qCritical() << "JavaWrapper::getExternalStorageDirectory - String failed to load!";
			return QString("/mnt/sdcard");
		}

		QString qtString(cString);
		env->ReleaseStringUTFChars(javaString, cString);
		m_javaVM->DetachCurrentThread();
		return qtString;
	}
	else
	{
		qCritical() << "JavaWrapper::getExternalStorageDirectory - Java class failed to load!";
		m_javaVM->DetachCurrentThread();
		return QString("/mnt/sdcard");
	}
}

bool JavaWrapper::isExternalStorageReady()
{
	JNIEnv* env = NULL;
	jclass javaClass = NULL;
	if (m_javaVM->AttachCurrentThread(&env, NULL)<0)
	{
		qCritical()<<"JavaWrapper::isExternalStorageReady - AttachCurrentThread failed";
		javaClass = NULL;
		return false;
	}

	javaClass = env->GetObjectClass(customClassPtr);

	if(javaClass)
	{
		jmethodID isExternalStorageReady = env->GetStaticMethodID(javaClass,"isExternalStorageReady", "()Z");
		bool result = static_cast<bool>(env->CallStaticObjectMethod(javaClass, isExternalStorageReady));
		m_javaVM->DetachCurrentThread();
		return result;
	}
	else
	{
		qCritical() << "JavaWrapper::getExternalStorageDirectory - Java class failed to load!";
		m_javaVM->DetachCurrentThread();
		return false;
	}
}

#endif // ANDROID
