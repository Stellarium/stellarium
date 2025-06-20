#ifndef SCMSTARTDIALOG_HPP
#define SCMSTARTDIALOG_HPP

#include "SkyCultureMaker.hpp"
#include "StelDialog.hpp"
#include <QObject>

#ifdef SCM_CONVERTER_ENABLED_CPP
# include "SkyCultureConverter.hpp"
# include "unarr.h"
#endif

class Ui_scmStartDialog;

class ScmStartDialog : public StelDialog
{
protected:
	void createDialogContent() override;

public:
	ScmStartDialog(SkyCultureMaker *maker);
	~ScmStartDialog() override;

public slots:
	void retranslate() override;

private slots:
	void startScmCreationProcess();
	void closeDialog();

private:
	Ui_scmStartDialog *ui  = nullptr;
	SkyCultureMaker *maker = nullptr;
};

#endif // SCMSTARTDIALOG_HPP
