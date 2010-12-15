#ifndef _SCENERY3DDIALOG_HPP_
#define _SCENERY3DDIALOG_HPP_

#include "StelDialog.hpp"
#include "ui_scenery3dDialog.h"

class Scenery3dDialog : public StelDialog
{
    Q_OBJECT
public:
    Scenery3dDialog();

    virtual void languageChanged();
    virtual void createDialogContent();
private:
    Ui_scenery3dDialogForm* ui;
};

#endif
