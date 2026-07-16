#include <QtTest>
#include <QApplication>
#include <QMainWindow>
#include <QSettings>
#include <QSize>

class MainWindowTest : public QObject {
    Q_OBJECT

public:
    MainWindowTest() {}

private slots:
    void testSaveRestoreGeometry();
};

void MainWindowTest::testSaveRestoreGeometry() {
    QApplication app(0);

    MainWindow window;
    window.resize(800, 600);
    window.saveGeometry();

    QSettings settings("Stellarium", "MainWindow");
    settings.setValue("geometry", window.saveGeometry());

    window.close();
    app.quit();

    app.exec();

    window.show();
    window.restoreGeometry();

    QCOMPARE(window.size(), QSize(800, 600));
}

QTEST_MAIN(MainWindowTest)