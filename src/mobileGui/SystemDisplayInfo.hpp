#ifndef SYSTEMDISPLAYINFO_HPP
#define SYSTEMDISPLAYINFO_HPP

#include <QSystemDisplayInfo>

//! @class SystemDisplayInfo
//! A QML-accessible version of SystemDisplayInfo
//! Also stores the device's density scale for layout scaling
class SystemDisplayInfo : public QtMobility::QSystemDisplayInfo
{
	Q_OBJECT
	Q_PROPERTY(DpiBucket dpiBucket READ dpiBucket)
	Q_PROPERTY(float density READ density NOTIFY densityChanged)
	Q_ENUMS(DpiBucket)
public:
#define LOW_DPI_DENSITY 0.75
#define MEDIUM_DPI_DENSITY 1.00
#define HIGH_DPI_DENSITY 1.50
#define XHIGH_DPI_DENSITY 2.00

	enum DpiBucket
	{
		INVALID_DPI = -1,
		LOW_DPI = 120,    //density = 0.75
		MEDIUM_DPI = 160, //density = 1.00
		HIGH_DPI = 240,   //density = 1.50
		XHIGH_DPI = 320   //density = 2.00
	};

    explicit SystemDisplayInfo(QObject *parent = 0);
	virtual ~SystemDisplayInfo() {}


	//These are from QSystemDisplayInfo, just made invokable:

	Q_INVOKABLE static int displayBrightness();
	Q_INVOKABLE static int colorDepth();

	Q_INVOKABLE QSystemDisplayInfo::DisplayOrientation orientation();
	Q_INVOKABLE float contrast();
	Q_INVOKABLE int getDPIWidth();
	Q_INVOKABLE int getDPIHeight();
	Q_INVOKABLE int physicalHeight();
	Q_INVOKABLE int physicalWidth();
	Q_INVOKABLE QSystemDisplayInfo::BacklightState backlightStatus();

	//These are new:

	//! Convert from density-independent pixels (dp) to screen pixels
	//! @param dp a measurement in dp
	//! @return   the dp given, converted to pixels
	Q_INVOKABLE int dpToPixels(int dp); //convert from Dp to pixels

	//! @return the abstract density bucket to which the screen belongs
	DpiBucket dpiBucket();

	//! @return the conversion factor from density-independent pixels to
	//!         screen pixels. Higher for denser screens.
	float density();

signals:
	void orientationChanged ( QSystemDisplayInfo::DisplayOrientation orientation );
	void densityChanged();

public slots:
	//! Tell the SystemDisplayInfo that the orientation has switched. This lets us detect it
	//! from the accelerometers directly, allowing us to display OS-handled switching
	//! (which would introduce a delay and unpleasant screen blanking; this is especially
	//! undesireable if we're using accelerometer-control mode)
	//! @param orientation the new orientation
	void changeOrientation(QSystemDisplayInfo::DisplayOrientation orientation);

private:
	QSystemDisplayInfo::DisplayOrientation currOrientation;
	float m_density; //factor to multiply dp by
	DpiBucket m_dpiBucket; //current DPI bucket
};

#endif // SYSTEMDISPLAYINFO_HPP
