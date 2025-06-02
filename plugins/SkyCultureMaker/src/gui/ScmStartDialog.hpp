#ifndef SCMSTARTDIALOG_HPP
#define SCMSTARTDIALOG_HPP

#include <QObject>
#include "StelDialog.hpp"
#include "../SkyCultureMaker.hpp"

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
	Ui_scmStartDialog *ui = nullptr;
	SkyCultureMaker *maker = nullptr;
};

#endif	// SCMSTARTDIALOG_HPP
