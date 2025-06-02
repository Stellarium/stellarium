#ifndef SCM_SKY_CULTURE_DIALOG_HPP
#define SCM_SKY_CULTURE_DIALOG_HPP

#include <QObject>
#include <QString>
#include <optional>
#include "StelDialogSeparate.hpp"
#include "../SkyCultureMaker.hpp"

class Ui_scmSkyCultureDialog;

class ScmSkyCultureDialog : public StelDialogSeparate
{

protected:
	void createDialogContent() override;

public:
	ScmSkyCultureDialog(SkyCultureMaker *maker);
	~ScmSkyCultureDialog() override;

public slots:
	void retranslate() override;
	void close() override;

private slots:
	void saveSkyCulture();
	void constellationDialog();
	void removeConstellation();

private:
	Ui_scmSkyCultureDialog *ui = nullptr;
	SkyCultureMaker *maker = nullptr;

	QString name;
	std::vector<QString> constellationList;

	void updateSkyCultureSave(bool saved);
	void setIdFromName(QString &name);
};

#endif	// SCM_SKY_CULTURE_DIALOG_HPP
