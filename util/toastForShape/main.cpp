#include <QtCore/QCoreApplication>
#include "StelToast.hpp"
#include "StelJsonParser.hpp"

int main(int argc, char *argv[])
{
	SphericalRegionP reg = SphericalRegionP::loadFromQVariant(StelJsonParser::parse(argv[0]).toList());
	//SphericalRegionP reg = SphericalRegionP::loadFromQVariant(StelJsonParser::parse("[[21.286982057125858, 82.117482778097937], [341.20801035028205, 82.141350178494122], [308.54824444399429, 86.661406614892087], [54.398199236987082, 86.613098160666411]]").toList());
	ToastGrid grid(10);
	QVariantList l;
	for (int i=0;i<1024;++i)
	{
		for (int j=0;j<1024;++j)
		{
			SphericalConvexPolygon poly(grid.getPolygon(10,i,j));
			if (reg->contains(poly))
			{
				QVariantMap m;
				m["i"]=i;
				m["j"]=j;
				m["tile"]= poly.toQVariant();
				l << m;
			}
		}
	}
	qDebug() << StelJsonParser::write(l);

	return 0;
}
