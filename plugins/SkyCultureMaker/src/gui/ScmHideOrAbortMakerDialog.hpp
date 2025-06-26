#ifndef SCMHIDEORABORTMAKERDIALOG_HPP
#define SCMHIDEORABORTMAKERDIALOG_HPP

#include "SkyCultureMaker.hpp"
#include "StelDialogSeparate.hpp"
#include <QObject>

class Ui_scmHideOrAbortMakerDialog;

class ScmHideOrAbortMakerDialog : public StelDialogSeparate
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
	void cancelDialog();

private:
	Ui_scmHideOrAbortMakerDialog *ui  = nullptr;
	SkyCultureMaker *maker = nullptr;
};

#endif // SCMHIDEORABORTMAKERDIALOG_HPP
