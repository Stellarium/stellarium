#ifndef PLANESDIALOG_HPP
#define PLANESDIALOG_HPP

#include "StelDialog.hpp"

class Ui_planesDialog;
class Planes;

class PlanesDialog : public StelDialog
{
	Q_OBJECT

public:
	PlanesDialog();
	~PlanesDialog() override;

public slots:
	void retranslate() override;
	void updateFromPlugin();
	void setStatus(const QString& status);

protected:
	void createDialogContent() override;

private slots:
	void setEnabledFlag(bool enabled);
	void setShowLabels(bool enabled);
	void setShowButton(bool enabled);
	void setLabelMode(int index);
	void setProvider(int index);
	void setRadius(int radiusNm);
	void setFetchInterval(int seconds);
	void triggerRefresh();
	void openExternalLink(const QString& url);

private:
	void updateComboTexts();
	void setAboutHtml();

	Planes* planes;
	Ui_planesDialog* ui;
};

#endif
