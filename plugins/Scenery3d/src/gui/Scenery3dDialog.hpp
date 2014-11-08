#ifndef _SCENERY3DDIALOG_HPP_
#define _SCENERY3DDIALOG_HPP_

#include "StelDialog.hpp"
#include "ui_scenery3dDialog.h"

class Scenery3dDialog : public StelDialog
{
    Q_OBJECT
public:
    Scenery3dDialog(QObject* parent = NULL);
	~Scenery3dDialog();

    virtual void languageChanged();
    virtual void createDialogContent();

public slots:
        void retranslate();

private slots:
    void scenery3dChanged(QListWidgetItem* item);
    void renderingShadowmapChanged(void);
    void renderingBumpChanged(void);
    void renderingShadowsFilterChanged(void);
    void renderingShadowsFilterHQChanged(void);
    //! Update the widget to make sure it is synchrone if a value was changed programmatically
    //! This function should be called repeatedly with e.g. a timer
    void updateFromProgram();

private:
    Ui_scenery3dDialogForm* ui;
};

#endif
