#ifndef SCM_SKY_CULTURE_DIALOG_HPP
#define SCM_SKY_CULTURE_DIALOG_HPP

#include <QObject>
#include <QString>
#include <optional>
#include "StelDialogSeparate.hpp"
#include "../SkyCultureMaker.hpp"
#include "../ScmConstellation.hpp"

class Ui_scmSkyCultureDialog;

class ScmSkyCultureDialog : public StelDialogSeparate
{

protected:
	void createDialogContent() override;

public:
	ScmSkyCultureDialog(SkyCultureMaker *maker);
	~ScmSkyCultureDialog() override;

	/**
	 * @brief Sets the constellations to be displayed in the dialog.
	 *
	 * @param constellations The vector of constellations to be set.
	 */
	void setConstellations(std::vector<scm::ScmConstellation> *constellations);

public slots:
	void retranslate() override;
	void close() override;

private slots:
	void saveSkyCulture();
	void constellationDialog();
	void removeSelectedConstellation();
	void updateRemoveConstellationButton();

private:
	Ui_scmSkyCultureDialog *ui = nullptr;
	SkyCultureMaker *maker = nullptr;

	/// The name of the sky culture.
	QString name = "";

	/// The vector of constellations to be displayed in the dialog.
	std::vector<scm::ScmConstellation> *constellations = nullptr;

	/**
	 * @brief Gets the display name from a constellation.
	 *
	 * @param constellation The constellation to get the display name from.
	 * @return The display name of the constellation.
	 */
	QString getDisplayNameFromConstellation(const scm::ScmConstellation &constellation) const;

	void setIdFromName(QString &name);
};

#endif	// SCM_SKY_CULTURE_DIALOG_HPP
