#ifndef SYSTEMDISPLAYINFO_HPP
#define SYSTEMDISPLAYINFO_HPP

#include <QSystemDisplayInfo>

//! @class SystemDisplayInfo
//! Just a QML-accessible version of SystemDisplayInfo
class SystemDisplayInfo : public QtMobility::QSystemDisplayInfo
{
    Q_OBJECT
public:
    explicit SystemDisplayInfo(QObject *parent = 0);
	virtual ~SystemDisplayInfo() {}

	Q_INVOKABLE static int displayBrightness();
	Q_INVOKABLE static int colorDepth();

	Q_INVOKABLE QSystemDisplayInfo::DisplayOrientation orientation();
	Q_INVOKABLE float contrast();
	Q_INVOKABLE int getDPIWidth();
	Q_INVOKABLE int getDPIHeight();
	Q_INVOKABLE int physicalHeight();
	Q_INVOKABLE int physicalWidth();
	Q_INVOKABLE QSystemDisplayInfo::BacklightState backlightStatus();

signals:
	void orientationChanged ( QSystemDisplayInfo::DisplayOrientation orientation );

public slots:
	//! Tell the SystemDisplayInfo that the orientation has switched. This lets us detect it
	//! from the accelerometers directly, allowing us to display OS-handled switching
	//! (which would introduce a delay and unpleasant screen blanking; this is especially
	//! undesireable if we're using accelerometer-control mode)
	//! @param orientation the new orientation
	void changeOrientation(QSystemDisplayInfo::DisplayOrientation orientation);

private:
	QSystemDisplayInfo::DisplayOrientation currOrientation;
};

#endif // SYSTEMDISPLAYINFO_HPP
