#ifndef SKYCULTUREMAKER_HPP
#define SKYCULTUREMAKER_HPP

#include "StelModule.hpp"
#include "VecMath.hpp"
#include "StelTranslator.hpp"
#include "StelCore.hpp"

#include <QFont>

class QPixmap;
class StelButton;

//! This is an example of a plug-in which can be dynamically loaded into stellarium
class SkyCultureMaker : public StelModule
{
	Q_OBJECT
	Q_PROPERTY(
	    bool enabledDrawLine READ getIsLineDrawEnabled WRITE setIsLineDrawEnabled NOTIFY eventIsLineDrawEnabled)
public:
	SkyCultureMaker();
	~SkyCultureMaker() override;

	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	void init() override;
	// Activate only if update() does something.
	// void update(double deltaTime) override {}
	void draw(StelCore *core) override;
	double getCallOrder(StelModuleActionName actionName) const override;

	//! Handle mouse clicks. Please note that most of the interactions will be done through the GUI module.
	//! @return set the event as accepted if it was intercepted
	void handleMouseClicks(class QMouseEvent *) override;

	//! Handle mouse moves. Please note that most of the interactions will be done through the GUI module.
	//! @return true if the event was intercepted
	bool handleMouseMoves(int x, int y, Qt::MouseButtons b);

signals:
	void eventIsLineDrawEnabled(bool b);

public slots:
	bool getIsLineDrawEnabled() const
	{
		return isLineDrawEnabled;
	}

	void setIsLineDrawEnabled(bool b);

private:
	const QString groupId = N_("Sky Culture Maker");
	const QString actionIdLine = "actionShow_SkyCultureMaker_Line";
	enum Drawing
	{
		None = 0,
		hasStart = 1,
		hasFloatingEnd = 2,
		hasEnd = 4,
	};

	/// Indicates that line drawing can be done (QT Signal)
	bool isLineDrawEnabled;

	/// The button to activate line drawing.
	StelButton *toolbarButton;

	/// Font used for displaying our text
	QFont font;

	/// The start point of the line.
	Vec3d startPoint;

	/// The end point of the line.
	Vec3d endPoint;

	/// Indicates that the startPoint has been set.
	Drawing drawState;

	/// Draws the line between the start and the current end point.
	/// @param core The core used for drawing the line.
	void drawLine(StelCore *core);
};

#include <QObject>
#include "StelPluginInterface.hpp"

//! This class is used by Qt to manage a plug-in interface
class SkyCultureMakerStelPluginInterface : public QObject, public StelPluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID StelPluginInterface_iid)
	Q_INTERFACES(StelPluginInterface)
public:
	StelModule *getStelModule() const override;
	StelPluginInfo getPluginInfo() const override;
	QObjectList getExtensionList() const override
	{
		return QObjectList();
	}
};

#endif /* SKYCULTUREMAKER_HPP */
