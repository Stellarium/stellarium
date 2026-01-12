#include <QObject>
#include <QApplication>
#include <QMainWindow>
#include <QScreen>
#include <QSettings>
#include <QTimer>
#include <QWindow>


class WindowHelper : public QObject
{
    Q_OBJECT

public:
    explicit WindowHelper(QMainWindow* mainWindow, QSettings* settings);

    void applyInitialConfig();
    void setFullScreen(bool enable);
    void saveWindowPosition();
    void restoreWindowPosition();
    void setCursorTimeoutEnabled(bool enable, int timeoutSeconds);
    void hideCursor();
    void updateTitle(const QString& appName);
    void saveWindowSize(const QSize& size);
    void restoreWindowSize();

    QRectF getPhysicalSize(const QRectF& virtualRect) const;

signals:
    void fullScreenChanged(bool);
    void flagCursorTimeoutChanged(bool);

private slots:
    void handleCursorTimeout();

private:
    QMainWindow* mainWindow;
    QSettings* settings;
    QTimer* cursorTimer;
    bool cursorTimeoutEnabled;
};
