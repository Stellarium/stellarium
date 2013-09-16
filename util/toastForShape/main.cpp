#include <QtCore/QCoreApplication>
#include "StelToast.hpp"
#include "StelJsonParser.hpp"

int main(int argc, char *argv[])
{
	Q_ASSERT(argc==3);
	QByteArray ar = argv[1];
	bool ok;
	int level = ar.toInt(&ok);
	if (!ok)
	{
		qFatal("Argument 1 must be a valid level number");
	}
	int length = sqrt(pow(4, level));
	SphericalRegionP reg = SphericalRegionP::loadFromQVariant(StelJsonParser::parse(argv[2]).toList());
	//SphericalRegionP reg = SphericalRegionP::loadFromQVariant(StelJsonParser::parse("[[21.286982057125858, 82.117482778097937], [341.20801035028205, 82.141350178494122], [308.54824444399429, 86.661406614892087], [54.398199236987082, 86.613098160666411]]").toList());
	ToastGrid grid(level);
	QVariantList l;
	for (int i=0;i<length;++i)
	{
		for (int j=0;j<length;++j)
		{
			SphericalConvexPolygon poly(grid.getPolygon(level,i,j));
			if (reg->intersects(poly))
			{
				QVariantMap m;
				m["i"]=i;
				m["j"]=j;
				m["tile"]= poly.toQVariant();
				l << m;
			}
		}
	}
	printf("%s", StelJsonParser::write(l).constData());

	return 0;
}
