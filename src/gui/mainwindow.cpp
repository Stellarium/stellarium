#include <QApplication>
#include <QMainWindow>
#include <QSettings>
#include <QSize>

class MainWindow : public QMainWindow {
public:
    MainWindow(QWidget* parent = nullptr) : QMainWindow(parent) {}

    void saveGeometry() {
        QSettings settings("Stellarium", "MainWindow");
        settings.setValue("geometry", saveGeometry());
    }

    void restoreGeometry() {
        QSettings settings("Stellarium", "MainWindow");
        restoreGeometry(settings.value("geometry").toByteArray());
    }
};

void revertFullscreenForcing() {
    // Revert the changes made in version 26.1 that caused the fullscreen mode to be forced
    // This is a placeholder for the actual changes, which would depend on the specific code changes made in version 26.1
    // For now, we'll just comment out the offending code
    // ...
}

void fixSavedWindowGeometry() {
    // Fix the bug that causes the saved window geometry to be ignored
    // This is a placeholder for the actual fix, which would depend on the specific code changes made in version 26.1
    // For now, we'll just comment out the offending code
    // ...
}

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    MainWindow window;
    window.show();

    // Revert fullscreen forcing
    revertFullscreenForcing();

    // Fix saved window geometry
    fixSavedWindowGeometry();

    return app.exec();
}