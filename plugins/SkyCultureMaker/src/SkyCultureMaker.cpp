#include "StelProjector.hpp"
#include "StelPainter.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelLocaleMgr.hpp"
#include "StelModuleMgr.hpp"
#include "SkyCultureMaker.hpp"
#include "StelGui.hpp"
#include "StelGuiItems.hpp"

#include <QApplication>
#include <QDebug>
#include <QMouseEvent>
#include <QPixmap>

/*************************************************************************
 This method is the one called automatically by the StelModuleMgr just
 after loading the dynamic library
*************************************************************************/
StelModule *SkyCultureMakerStelPluginInterface::getStelModule() const
{
	return new SkyCultureMaker();
}

StelPluginInfo SkyCultureMakerStelPluginInterface::getPluginInfo() const
{
	// Allow to load the resources when used as a static plugin
	Q_INIT_RESOURCE(SkyCultureMaker);

	StelPluginInfo info;
	info.id = "SkyCultureMaker";
	info.displayedName = "Sky Culture Maker";
	info.authors = "Vincent Gerlach (RivinHD), Luca-Philipp Grumbach (xLPMG), Fabian Hofer (Integer-Ctrl), Richard Hofmann (ZeyxRew), Mher Mnatsakanyan (MherMnatsakanyan03)";
	info.contact = "Contact us using our GitHub usernames, via an Issue or the Discussion tab in the Stellarium repository.";
	info.description = "Plugin to draw and export sky cultures in Stellarium.";
	info.version = SKYCULTUREMAKER_PLUGIN_VERSION;
	info.license = SKYCULTUREMAKER_PLUGIN_LICENSE;
	return info;
}

/*************************************************************************
 Constructor
*************************************************************************/
SkyCultureMaker::SkyCultureMaker()
	: isLineDrawEnabled(false)
	, drawState(None)
{
	startPoint.set(0, 0, 0);
	endPoint.set(0, 0, 0);

	setObjectName("SkyCultureMaker");
	font.setPixelSize(25);
}

/*************************************************************************
 Destructor
*************************************************************************/
SkyCultureMaker::~SkyCultureMaker()
{
}

/*************************************************************************
 Reimplementation of the getCallOrder method
*************************************************************************/
double SkyCultureMaker::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName == StelModule::ActionDraw)
		return StelApp::getInstance().getModuleMgr().getModule("NebulaMgr")->getCallOrder(actionName) + 10.;
	if (actionName == StelModule::ActionHandleMouseClicks)
		return -11;
	return 0;
}

/*************************************************************************
 Init our module
*************************************************************************/
void SkyCultureMaker::init()
{
	qDebug() << "init called for SkyCultureMaker";

	StelApp &app = StelApp::getInstance();

	addAction(actionIdLine, groupId, N_("Sky Culture Maker Line"), "enabledDrawLine");

	// Add a toolbar button
	try
	{
		QPixmap iconLineDisabled(":/SkyCultureMaker/bt_LineDraw_Off.png");
		QPixmap iconLineEnabled(":/SkyCultureMaker/bt_LineDraw_On.png");
		qDebug() << (iconLineDisabled.isNull() ? "Failed to load image: bt_LineDraw_Off.png"
						      : "Loaded image: bt_LineDraw_Off.png");
		qDebug() << (iconLineEnabled.isNull() ? "Failed to load image: bt_LineDraw_On.png"
						      : "Loaded image: bt_LineDraw_On.png");

		StelGui *gui = dynamic_cast<StelGui *>(app.getGui());
		if (gui != Q_NULLPTR)
		{
			toolbarButton = new StelButton(Q_NULLPTR,
						       iconLineEnabled,
						       iconLineDisabled,
						       QPixmap(":/graphicGui/miscGlow32x32.png"),
						       actionIdLine,
						       false);
			gui->getButtonBar()->addButton(toolbarButton, "065-pluginsGroup");
		}
	}
	catch (std::runtime_error &e)
	{
		qWarning() << "Unable create toolbar button for SkyCultureMaker plugin: " << e.what();
	}
}

/*************************************************************************
 Draw our module. This should print "Hello world!" in the main window
*************************************************************************/
void SkyCultureMaker::draw(StelCore *core)
{
	if (isLineDrawEnabled)
	{
		drawLine(core);
	}
}

bool SkyCultureMaker::handleMouseMoves(int x, int y, Qt::MouseButtons)
{
	if (drawState & (hasStart | hasFloatingEnd))
	{
		const StelProjectorP prj = StelApp::getInstance().getCore()->getProjection(StelCore::FrameAltAz);
		prj->unProject(x, y, endPoint);
		drawState = hasFloatingEnd;
		return true;
	}

	return false;
}

void SkyCultureMaker::drawLine(StelCore *core)
{
	if (!(drawState & (hasEnd | hasFloatingEnd)))
	{
		return;
	}

	StelPainter painter(core->getProjection(StelCore::FrameAltAz));
	painter.setBlending(true);
	painter.setLineSmooth(true);
	Vec3f color = {1.f, 0.5f, 0.5f};
	bool alpha = (drawState == hasEnd ? 1.0f : 0.5f);
	painter.setColor(color, alpha);
	painter.drawGreatCircleArc(startPoint, endPoint);
}

void SkyCultureMaker::handleMouseClicks(class QMouseEvent *event)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
	qreal x = event->position().x(), y = event->position().y();
#else
	qreal x = event->x(), y = event->y();
#endif

	if (event->type() == QEvent::MouseButtonPress && event->button() == Qt::RightButton)
	{
		const StelProjectorP prj = StelApp::getInstance().getCore()->getProjection(StelCore::FrameAltAz);
		Vec3d point;
		prj->unProject(x, y, point);

		if (drawState & (hasStart | hasFloatingEnd))
		{
			endPoint = point;
			drawState = hasEnd;
		}
		else
		{
			startPoint = point;
			drawState = hasStart;
		}

		event->setAccepted(true);
		return;
	}
	else if (event->type() == QEvent::MouseButtonDblClick && event->button() == Qt::RightButton)
	{
		// Reset line drawing
		drawState = None;
		event->setAccepted(true);
		return;
	}

	event->setAccepted(false);
}

void SkyCultureMaker::setIsLineDrawEnabled(bool b)
{
	if (b != isLineDrawEnabled)
	{
		isLineDrawEnabled = b;
		emit eventIsLineDrawEnabled(b);
	}
}
