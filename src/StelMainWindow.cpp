#include <QMainWindow>


//! Sets up the menus for the main window.
static void setupMenus(QMainWindow* window, QApplication* app)
{
	QMenuBar* menuBar = window->menuBar();
	menuBar->setNativeMenuBar(true);

	QMenu* fileMenu = menuBar->addMenu("File");
	QAction* dummyFileAction = new QAction("Dummy File Action");
	fileMenu->addAction(dummyFileAction);
	QAction* quitAction = new QAction("Quit");
	quitAction->setMenuRole(QAction::QuitRole);
	QObject::connect(quitAction, &QAction::triggered, app, &QApplication::quit);
	fileMenu->addAction(quitAction);

	QMenu* editMenu = menuBar->addMenu("View");

	auto addAction = [&](const QString& label, const QString& shortcut, const QString& actionName) {
		QAction* action = new QAction(label);
		action->setShortcut(QKeySequence(shortcut));
		QObject::connect(action, &QAction::triggered, [actionName](){
			if (StelAction* a = StelApp::getInstance().getStelActionManager()->findAction(actionName))
				a->trigger();
		});
		editMenu->addAction(action);
	};

	addAction("Location...", "F6", "actionShow_Location_Window_Global");
	addAction("Date/Time...", "F5", "actionShow_DateTime_Window_Global");
	addAction("Search...", "F3", "actionShow_Search_Window_Global");
	addAction("Sky and Viewing Options...", "F4", "actionShow_View_Window_Global");
	addAction("Configuration...", "F2", "actionShow_Configuration_Window_Global");
	addAction("Shortcuts...", "F7", "actionShow_Shortcuts_Window_Global");
	addAction("Astronomical Calculations...", "F10", "actionShow_AstroCalc_Window_Global");
	addAction("Observing List...", "Alt+B", "actionShow_ObservingList_Window_Global");

#ifdef ENABLE_SCRIPT_CONSOLE
	addAction("Script Console...", "F12", "actionShow_ScriptConsole_Window_Global");
#endif

	QMenu* helpMenu = menuBar->addMenu("Help");
	QAction* helpAction = new QAction("Help...");
	helpAction->setShortcut(QKeySequence("F1"));
	QObject::connect(helpAction, &QAction::triggered, [](){
		if (StelAction* a = StelApp::getInstance().getStelActionManager()->findAction("actionShow_Help_Window_Global"))
			a->trigger();
	});
	helpMenu->addAction(helpAction);

	QAction* aboutAction = new QAction("About");
	aboutAction->setMenuRole(QAction::AboutRole);
	QObject::connect(aboutAction, &QAction::triggered, window, [](){
		// Optional: Connect to a real About dialog
	});
	helpMenu->addAction(aboutAction);
}