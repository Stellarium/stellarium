#include <QtGlobal>
#include <QTest>
#include "tests/testGetEnv.hpp"
#include "getEnv.hpp"

QTEST_GUILESS_MAIN(TestGetEnv)

// returns value as QT string
void TestGetEnv::testGetExistingEnv()
{
    const auto expectedString = "Some Value";
    qputenv("HELLO", expectedString);
    QCOMPARE(getEnv("HELLO"), QString(expectedString));
}

// returns empty string if value not in environment
void TestGetEnv::testGetMissingEnv()
{
    qunsetenv("HELLO");
    QCOMPARE(getEnv("HELLO"), QString());
}