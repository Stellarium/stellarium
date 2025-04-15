#ifndef SKYCOORDS_HPP
#define SKYCOORDS_HPP

#include <QPair>
#include <QJsonArray>

constexpr double D2R = M_PI / 180.0;
constexpr double R2D = 180.0 / M_PI;

class SkyCoords
{
public:
	// 将图像像素坐标转换为天球坐标（RA, Dec）
	static QPair<double, double> pixelToRaDec(int X, int Y,
		double CRPIX1, double CRPIX2,
		double CRVAL1, double CRVAL2,
		double CD1_1, double CD1_2,
		double CD2_1, double CD2_2);

	// 从一组天球角点坐标中计算中心点（RA, Dec）
	static QPair<double, double> calculateCenter(const QJsonArray& corners);
};

#endif // SKYCOORDS_HPP
