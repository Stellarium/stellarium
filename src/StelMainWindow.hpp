#ifndef STELMAINWINDOW_HPP
#define STELMAINWINDOW_HPP

#include <QMainWindow>

class QSettings;
class QApplication;
class StelMainView;

class StelMainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit StelMainWindow(QSettings* confSettings,
	                        QWidget* parent = nullptr);
	~StelMainWindow() override;

	StelMainView* getMainView() const;


	void setupMenus();

public slots:
    void initTitleI18n();

protected:
	void closeEvent(QCloseEvent* event) override;

private:
    void initWorkspace(QSettings* confSettings);
    void initFonts(QSettings* confSettings);
    void initTranslations();
    void enableScripting(QSettings* confSettings);
	void setupUi(QSettings* confSettings);
	void applyVideoSettings(QSettings* confSettings);

private:
	StelMainView* mainView { nullptr };
};

#endif
