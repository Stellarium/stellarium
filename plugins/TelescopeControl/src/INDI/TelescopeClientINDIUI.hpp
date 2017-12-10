#ifndef TELESCOPECLIENTINDIUI_HPP
#define TELESCOPECLIENTINDIUI_HPP

#include <QWidget>

namespace Ui {
class TelescopeClientINDIUI;
}

class TelescopeClientINDIUI : public QWidget
{
    Q_OBJECT

public:
    explicit TelescopeClientINDIUI(QWidget *parent = 0);
    ~TelescopeClientINDIUI();

private:
    Ui::TelescopeClientINDIUI *ui;
};

#endif // TELESCOPECLIENTINDIUI_HPP
