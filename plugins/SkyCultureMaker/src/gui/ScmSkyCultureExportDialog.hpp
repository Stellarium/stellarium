#ifndef SCM_SKY_CULTURE_EXPORT_DIALOG_HPP
#define SCM_SKY_CULTURE_EXPORT_DIALOG_HPP

#include "SkyCultureMaker.hpp"
#include "StelDialogSeparate.hpp"
#include <QObject>

class Ui_scmSkyCultureExportDialog;

class ScmSkyCultureExportDialog : public StelDialogSeparate
{
protected:
	void createDialogContent() override;

public:
	ScmSkyCultureExportDialog(SkyCultureMaker *maker);
	~ScmSkyCultureExportDialog() override;

public slots:
	void retranslate() override;

private:
	Ui_scmSkyCultureExportDialog *ui  = nullptr;
	SkyCultureMaker *maker = nullptr;

	void exportSkyCulture();
};

#endif // SCM_SKY_CULTURE_EXPORT_DIALOG_HPP
