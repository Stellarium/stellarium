#include "StelSplashScreen.hpp"
#include "StelFileMgr.hpp"
#include "StelTranslator.hpp"
#include "core/StelApp.hpp"
#include "core/StelActionMgr.hpp"
#include "core/StelLocaleMgr.hpp"
#include "StelMainWindow.hpp"
#include "StelMainView.hpp"
#include <QCloseEvent>
#include <QCoreApplication>
#include <QFontDatabase>
#include <QTranslator>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QThreadPool>
#include <QScreen>
#include <QGuiApplication>
#include <QSettings>

#ifdef ENABLE_SCRIPTING
#include "StelScriptOutput.hpp"
#endif

//! @class CustomQTranslator
//! Provides custom i18n support.
class CustomQTranslator : public QTranslator
{
	using QTranslator::translate;
public:
	bool isEmpty() const override { return false; }

	//! Overrides QTranslator::translate().
	//! Calls StelTranslator::qtranslate().
	//! @param context Qt context string - IGNORED.
	//! @param sourceText the source message.
	//! @param comment optional parameter
	QString translate(const char *context, const char *sourceText, const char *disambiguation = Q_NULLPTR, int n = -1) const override
	{
		Q_UNUSED(context)
		Q_UNUSED(n)
		return StelTranslator::globalTranslator->qtranslate(sourceText, disambiguation);
	}
};

StelMainWindow::StelMainWindow(QSettings* confSettings, QWidget* parent)
	: QMainWindow(parent)
{
    Q_INIT_RESOURCE(mainRes);
    Q_INIT_RESOURCE(guiRes);

    setWindowIcon(QIcon(":/mainWindow/icon.bmp"));
    enableScripting(confSettings);
    initWorkspace(confSettings);
    initFonts(confSettings);
    initTranslations();
	mainView = new StelMainView(confSettings);
	setCentralWidget(mainView);
	applyVideoSettings(confSettings);
    connect(&StelApp::getInstance().getLocaleMgr(), &StelLocaleMgr::appLanguageChanged, this, &StelMainWindow::initTitleI18n);
	initTitleI18n();
    setupMenus();
	SplashScreen::finish(this);
    qDebug() << "Max thread count (Global Pool): " << QThreadPool::globalInstance()->maxThreadCount();
}

StelMainWindow::~StelMainWindow() = default;

// Update the translated title
void StelMainWindow::initTitleI18n()
{
	setWindowTitle(StelUtils::getApplicationName());
}

void StelMainWindow::closeEvent(QCloseEvent* event)
{
    mainView -> deinit();
}

void StelMainWindow::initWorkspace(QSettings* confSettings)
{
	StelFileMgr::setObsListDir(confSettings->value("main/observinglists_dir", StelFileMgr::getUserDir()).toString());
}

void StelMainWindow::enableScripting(QSettings* confSettings)
{
	#ifdef ENABLE_SCRIPTING
	QString outputFile = StelFileMgr::getUserDir()+"/output.txt";
	if (confSettings->value("main/use_separate_output_file", false).toBool())
		outputFile = StelFileMgr::getUserDir()+"/output-"+QDateTime::currentDateTime().toString("yyyyMMdd-HHmmss")+".txt";
	StelScriptOutput::init(outputFile);
	#endif
}

void StelMainWindow::initFonts(QSettings* confSettings)
{
    // Add the Noto & DejaVu fonts that we use everywhere in the program
    const QStringList customFonts = { "NotoSans-Regular.ttf", "NotoSansMono-Regular.ttf", "NotoSansSC-Regular.otf", "DejaVuSans.ttf", "DejaVuSansMono.ttf" };
    for (auto &font: std::as_const(customFonts))
    {
        QString customFont = StelFileMgr::findFile(QString("data/%1").arg(font));
        if (!customFont.isEmpty())
            QFontDatabase::addApplicationFont(customFont);
    }

    QString fileFont = confSettings->value("gui/base_font_file", "").toString();
    if (!fileFont.isEmpty())
    {
        const QString& afName = StelFileMgr::findFile(QString("data/%1").arg(fileFont));
        if (!afName.isEmpty() && !afName.contains("file not found"))
            QFontDatabase::addApplicationFont(afName);
        else
            qWarning().noquote() << "ERROR while loading custom font:" << QDir::toNativeSeparators(fileFont);
    }

#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
    // Set OS-dependent fallback fonts to retrieve missing glyphs if the main font does not provide them
#ifdef Q_OS_WIN
    QFontDatabase::addApplicationFallbackFontFamily(QChar::Script_Cuneiform, "Segoe UI Historic");
#endif
#endif

    // Set the default application font and font size.
    // Note that style sheet will possibly override this setting.
    QString baseFont = confSettings->value("gui/base_font_name", "Noto Sans").toString();
    QFont tmpFont(baseFont);
    tmpFont.setPixelSize(confSettings->value("gui/gui_font_size", DEFAULT_FONT_SIZE).toInt());
    QGuiApplication::setFont(tmpFont);

    if (qApp->property("onetime_fontinfo").toBool())
    {
        qInfo() << "=======================================================================";
        StelApp::getInstance().dumpFontInfo();
        qInfo() << "=======================================================================";
    }
}

void StelMainWindow::initTranslations()
{
    // Initialize translator feature
    StelTranslator::init(StelFileMgr::getInstallationDir() + "/data/languages.tab");

    // Use our custom translator for Qt translations as well
    CustomQTranslator trans;
    qApp->installTranslator(&trans);
}

void StelMainWindow::applyVideoSettings(QSettings* confSettings)
{
	int screen = confSettings->value("video/screen_number", 0).toInt();
	auto screens = QGuiApplication::screens();
	if (screen < 0 || screen >= screens.count())
		screen = 0;

	QScreen* qscreen = screens.at(screen);
	QRect screenGeom = qscreen->geometry();
	qreal pixelRatio = qscreen->devicePixelRatio();

	QSize virtSize(
		confSettings->value("video/screen_w", screenGeom.width()).toInt(),
		confSettings->value("video/screen_h", screenGeom.height()).toInt()
	);

	QSize size(
		std::lround(virtSize.width() / pixelRatio),
		std::lround(virtSize.height() / pixelRatio)
	);

	resize(size);

	bool fullscreen = confSettings->value("video/fullscreen", true).toBool();
	if (fullscreen)
	{
		move(screenGeom.topLeft() + QPoint(1, 1));
		setGeometry(geometry() & screenGeom);
		showFullScreen();
	}
	else
	{
		int x = confSettings->value("video/screen_x", 0).toInt();
		int y = confSettings->value("video/screen_y", 0).toInt();
		move(screenGeom.x() + x / pixelRatio,
		     screenGeom.y() + y / pixelRatio);
		show();
	}
}

struct MenuNode
{
    QString title;
    QStringList groups;
    QList<MenuNode> children;
};


static const QList<MenuNode> MENU_TREE = {
    {
        QObject::tr("&Window"),
        { QObject::tr("Windows") },
        {}
    },
    {
        QObject::tr("&View"),
        { QObject::tr("Display Options") },
        {
            {
                QObject::tr("Movement and Selection"),
                { QObject::tr("Movement and Selection") },
                {}
            },
            {
                QObject::tr("Field of View"),
                { QObject::tr("Field of View") },
                {}
            }
        }
    },
    {
        QObject::tr("&Object"),
        { QObject::tr("Selected object information") },
        {
            {
                QObject::tr("Exoplanets"),
                { QObject::tr("Exoplanets") },
                {}
            },
            {
                QObject::tr("Meteor Showers"),
                { QObject::tr("Meteor Showers") },
                {}
            },
            {
                QObject::tr("Satellites"),
                { QObject::tr("Satellites") },
                {}
            }
        }
    },
    {
        QObject::tr("&Time"),
        { QObject::tr("Date and Time") },
        {
            {
                QObject::tr("Specific"),
                { QObject::tr("Specific Time") },
                {}
            }
        },
    },
    {
        QObject::tr("&Script"),
        { QObject::tr("Scripts") },
        {}
    },
    {
        QObject::tr("&Misc"),
        { QObject::tr("Miscellaneous") },
        {}
    }
};

static void addGroupsToMenu(
    QMenu* menu,
    StelActionMgr* mgr,
    const QStringList& groups,
    QSet<QString>& usedGroups)
{
    for (const QString& group : groups)
    {
        QList<StelAction*> actions = mgr->getActionList(group);
        if (actions.isEmpty())
            continue;

        usedGroups.insert(group);

        std::sort(actions.begin(), actions.end(),
            [](StelAction* a, StelAction* b) {
                return a->getText().localeAwareCompare(b->getText()) < 0;
            });

        for (StelAction* a : actions)
        {
            QAction* qa = a->getQAction();
            qa->setText(a->getText());
            menu->addAction(qa);
        }
    }
}

static QMenu* buildMenuTree(
    QMenuBar* mb,
    QMenu* parent,
    const MenuNode& node,
    StelActionMgr* mgr,
    QSet<QString>& usedGroups)
{
    QMenu* menu = parent
        ? parent->addMenu(node.title)
        : mb->addMenu(node.title);

    addGroupsToMenu(menu, mgr, node.groups, usedGroups);

    for (const MenuNode& child : node.children)
        buildMenuTree(nullptr, menu, child, mgr, usedGroups);

    return menu;
}

static void populateMenuFromGroup(
    QMenu* menu,
    StelActionMgr* mgr,
    const QString& group)
{
    QList<StelAction*> actions = mgr->getActionList(group);
    if (actions.isEmpty())
        return;

    std::sort(actions.begin(), actions.end(),
        [](StelAction* a, StelAction* b) {
            return a->getText().localeAwareCompare(b->getText()) < 0;
        });

    for (StelAction* a : actions)
    {
        QAction* qa = a->getQAction();
        qa->setText(a->getText());
        menu->addAction(qa);
    }
}


void StelMainWindow::setupMenus()
{
    QMenuBar* mb = menuBar();
    mb->clear();
    mb->setNativeMenuBar(true);

    StelActionMgr* mgr = StelApp::getInstance().getStelActionManager();
    if (!mgr)
        return;

    QSet<QString> usedGroups;

    for (const MenuNode& node : MENU_TREE)
    {
        QMenu* menu = mb->addMenu(node.title);

        for (const MenuNode& child : node.children)
        {
            QMenu* sub = menu->addMenu(child.title);

            for (const QString& group : child.groups)
            {
                populateMenuFromGroup(sub, mgr, group);
                usedGroups.insert(group);
            }
        }

        for (const QString& group : node.groups)
        {
            populateMenuFromGroup(menu, mgr, group);
            usedGroups.insert(group);
        }
    }

    QMenu* pluginsMenu = mb->addMenu(tr("&Plugin"));

    for (const QString& group : mgr->getGroupList())
    {
        if (usedGroups.contains(group))
            continue;

        QList<StelAction*> actions = mgr->getActionList(group);
        if (actions.isEmpty())
            continue;

        QMenu* groupMenu = pluginsMenu->addMenu(group);
        populateMenuFromGroup(groupMenu, mgr, group);
    }

    if (pluginsMenu->isEmpty())
        pluginsMenu->menuAction()->setVisible(false);

    if (StelAction* helpAction =
            StelApp::getInstance()
                .getStelActionManager()
                ->findAction("actionShow_Help_Window_Global"))
    {
        QMenu* helpMenu = mb->addMenu("Help");

        QAction* qa = helpAction->getQAction();
        qa->setText(tr("Stellarium Help"));
        helpMenu->addAction(qa);
    }

}

