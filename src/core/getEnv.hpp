#include <QtGlobal>

//! reterive QString value for passed environment variable name
const QString getEnv(const char* envName) {
#if QT_VERSION >= 0x050A00
    if (qEnvironmentVariableIsSet(envName))
    {
        return qEnvironmentVariable(envName);
    }
#else
    QByteArray envValue = qgetenv(envName);
    if (envValue.length()>0)
    {
        return QString::fromLocal8Bit(envValue);
    }
#endif
    return QString();
}
