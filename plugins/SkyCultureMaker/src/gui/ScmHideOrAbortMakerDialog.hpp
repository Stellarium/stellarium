#ifndef SCMHIDEORABORTMAKERDIALOG_HPP
#define SCMHIDEORABORTMAKERDIALOG_HPP

#include "SkyCultureMaker.hpp"
#include "StelDialog.hpp"
#include <QObject>

class UI_scmHideOrAbortMakerDialog;

class ScmHideOrAbortMakerDialog : public StelDialog
{
protected:
	void createDialogContent() override;

public:
	ScmHideOrAbortMakerDialog(SkyCultureMaker *maker);
	~ScmHideOrAbortMakerDialog() override;

public slots:
	void retranslate() override;

private slots:
	void hideScmCreationProcess();
	void abortScmCreationProcess();
	void closeDialog();

private:
	UI_scmHideOrAbortMakerDialog *ui  = nullptr;
	SkyCultureMaker *maker = nullptr;
};

#endif // SCMHIDEORABORTMAKERDIALOG_HPP
