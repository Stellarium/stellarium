#include "WindowHelper.hpp"

WindowHelper::WindowHelper(QMainWindow* mainWindow, QSettings* settings)
    : mainWindow(mainWindow), settings(settings), cursorTimer(new QTimer(this)), cursorTimeoutEnabled(false)
{
    cursorTimer->setSingleShot(true);
    connect(cursorTimer, &QTimer::timeout, this, &WindowHelper::handleCursorTimeout);
}

void WindowHelper::applyInitialConfig()
{
    mainWindow->setWindowIcon(QIcon(":/mainWindow/icon.bmp"));
    mainWindow->setObjectName("MainWindow");
    mainWindow->setAttribute(Qt::WA_AcceptTouchEvents);
    mainWindow->setMouseTracking(true);

    if (settings->value("video/fullscreen", false).toBool())
        mainWindow->showFullScreen();
    else
        mainWindow->show();
}

void WindowHelper::setFullScreen(bool enable)
{
    if (enable)
        mainWindow->showFullScreen();
    else
        mainWindow->showNormal();

    settings->setValue("video/fullscreen", enable);
    emit fullScreenChanged(enable);
}

void WindowHelper::saveWindowPosition()
{
    if (!mainWindow->isFullScreen())
    {
        settings->setValue("video/screen_x", mainWindow->x());
        settings->setValue("video/screen_y", mainWindow->y());
    }
}

void WindowHelper::restoreWindowPosition()
{
    int screen = settings->value("video/screen_number", 0).toInt();
    int x = settings->value("video/screen_x", 0).toInt();
    int y = settings->value("video/screen_y", 0).toInt();

    if (screen >= 0 && screen < QApplication::screens().size())
    {
        QRect geom = QApplication::screens().at(screen)->geometry();
        mainWindow->move(x + geom.x(), y + geom.y());
    }
}

void WindowHelper::saveWindowSize(const QSize& size)
{
    settings->setValue("video/screen_w", size.width());
    settings->setValue("video/screen_h", size.height());
}

void WindowHelper::restoreWindowSize()
{
    int w = settings->value("video/screen_w", 1024).toInt();
    int h = settings->value("video/screen_h", 768).toInt();
    mainWindow->resize(w, h);
}

void WindowHelper::setCursorTimeoutEnabled(bool enable, int timeoutSeconds)
{
    cursorTimeoutEnabled = enable;

    if (!enable)
    {
        if (QGuiApplication::overrideCursor() && QGuiApplication::overrideCursor()->shape() == Qt::BlankCursor)
            QGuiApplication::restoreOverrideCursor();
        cursorTimer->stop();
    }
    else
    {
        cursorTimer->start(timeoutSeconds * 1000);
    }

    settings->setValue("gui/flag_mouse_cursor_timeout", enable);
    emit flagCursorTimeoutChanged(enable);
}

void WindowHelper::handleCursorTimeout()
{
    if (cursorTimeoutEnabled)
        QGuiApplication::setOverrideCursor(Qt::BlankCursor);
}

void WindowHelper::updateTitle(const QString& appName)
{
    mainWindow->setWindowTitle(appName);
}

QRectF WindowHelper::getPhysicalSize(const QRectF& virtualRect) const
{
    QWindow* win = mainWindow->windowHandle();
    if (win)
    {
        qreal ratio = win->devicePixelRatio();
        return QRectF(virtualRect.x(), virtualRect.y(), virtualRect.width() * ratio, virtualRect.height() * ratio);
    }
    return virtualRect;
}