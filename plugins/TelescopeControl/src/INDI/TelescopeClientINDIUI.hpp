#ifndef TELESCOPECLIENTINDIUI_HPP
#define TELESCOPECLIENTINDIUI_HPP

#include <QWidget>

#include "INDIConnection.hpp"

namespace Ui {
class TelescopeClientINDIUI;
}

class TelescopeClientINDIUI : public QWidget
{
    Q_OBJECT

public:
    explicit TelescopeClientINDIUI(QWidget *parent = 0);
    ~TelescopeClientINDIUI();

private slots:
    void onGetDevicesPushButtonClicked();
    void onDevicesChanged();

private:
    Ui::TelescopeClientINDIUI *ui;
    INDIConnection mConnection;
};

#endif // TELESCOPECLIENTINDIUI_HPP
