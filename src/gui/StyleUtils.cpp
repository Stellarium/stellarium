#include "StyleUtils.hpp"
#include <QApplication>
#include <QEvent>
#include <QStyle>

void fullyRefreshWidgetStyle(QWidget* widget)
{
    if (!widget) return;

    widget->setUpdatesEnabled(false);

    widget->style()->unpolish(widget);
    widget->style()->polish(widget);

    QEvent styleChangeEvent(QEvent::StyleChange);
    QApplication::sendEvent(widget, &styleChangeEvent);

    widget->updateGeometry();
    widget->update();

    widget->setUpdatesEnabled(true);
}

void fullyRefreshWidgetStyleRecursive(QWidget* widget)
{
    if (!widget) return;

    fullyRefreshWidgetStyle(widget);

    const auto children = widget->findChildren<QWidget*>();
    for (QWidget* child : children)
        fullyRefreshWidgetStyle(child);
}