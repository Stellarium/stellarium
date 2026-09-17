/* Stellarium - GPL-2.0-or-later */
#include <QtTest>
#include <QFile>
#include <QTemporaryDir>
#include <limits>
#include "core/modules/AtmosphereTransmission.hpp"

class TestAtmosphereTransmission : public QObject
{
	Q_OBJECT
private slots:
	void nativeAndPackedAgree();
	void horizonColors();
	void invalidData();
	void incompleteNativeModel();
};

void TestAtmosphereTransmission::nativeAndPackedAgree()
{
	AtmosphereTransmission native, packed;
	QVERIFY(native.loadModel(QStringLiteral(TRANSMISSION_TEST_DATA "/default")));
	QFile file(QStringLiteral(TRANSMISSION_TEST_DATA "/lightweight-extinction.dat"));
	QVERIFY(file.open(QFile::ReadOnly));
	const auto bytes = file.readAll();
	QVERIFY(packed.load(bytes.constData(), bytes.size()));
	// Independent stored export versus the native-model reader, including the
	// depressed horizon at altitude, sea level and above the atmosphere.
	for (const double altitude : {-100., 0., 1000., 10000., 120000., 150000.})
		for (int i = 0; i <= 2000; ++i)
		{
			const double elevation = -M_PI_2 + M_PI*i/2000.;
			std::array<float,3> a, b;
			QVERIFY(native.sample(altitude, elevation, a));
			QVERIFY(packed.sample(altitude, elevation, b));
			for (int c = 0; c < 3; ++c)
				QVERIFY2(std::abs(a[c]-b[c]) < 2.e-6f, qPrintable(QString("altitude=%1 elevation=%2 channel=%3").arg(altitude).arg(elevation).arg(c)));
		}
}

void TestAtmosphereTransmission::horizonColors()
{
	AtmosphereTransmission model;
	QVERIFY(model.loadModel(QStringLiteral(TRANSMISSION_TEST_DATA "/default")));
	std::array<float,3> bottom, top, zenith, ground;
	// Half-degree solar/lunar disk, just above the horizon. Channel ratios must
	// redden toward its bottom even after normalizing away overall attenuation.
	QVERIFY(model.sample(0., 0.25*M_PI/180., bottom));
	QVERIFY(model.sample(0., 0.75*M_PI/180., top));
	QVERIFY(model.sample(0., M_PI_2, zenith));
	QVERIFY(model.sample(0., -0.01, ground));
	QVERIFY(bottom[0] > 0.f);
	QVERIFY(bottom[1]/bottom[0] < top[1]/top[0]);
	// The blue component can already be clipped to zero at both limbs.
	QVERIFY(bottom[2]/bottom[0] <= top[2]/top[0]);
	QVERIFY(top[1]/top[0] < zenith[1]/zenith[0]);
	for (float c : ground) QCOMPARE(c, 0.f);
	QVERIFY(!model.sample(0., std::numeric_limits<double>::quiet_NaN(), top));
	QVERIFY(!model.sample(std::numeric_limits<double>::infinity(), 0., top));
	QVERIFY(!model.sample(0., M_PI, top));
}

void TestAtmosphereTransmission::invalidData()
{
	AtmosphereTransmission model;
	std::array<float,3> rgb;
	QVERIFY(!model.sample(0., 0., rgb));
	QVERIFY(!model.load(nullptr, 100));
	QFile file(QStringLiteral(TRANSMISSION_TEST_DATA "/lightweight-extinction.dat"));
	QVERIFY(file.open(QFile::ReadOnly));
	auto bytes = file.readAll();
	QVERIFY(model.load(bytes.constData(), bytes.size()));
	QVERIFY(!model.load(bytes.constData(), bytes.size()-1));
	QVERIFY(!model.sample(0., 0., rgb));
	QVERIFY(model.load(bytes.constData(), bytes.size()));
	QTemporaryDir directory;
	QVERIFY(directory.isValid());
	QVERIFY(!model.loadModel(directory.path()));
	QVERIFY(!model.sample(0., 0., rgb));
	// Negative optical depth, in the last float of the packed little-endian data.
	bytes[bytes.size()-4] = 0;
	bytes[bytes.size()-3] = 0;
	bytes[bytes.size()-2] = char(0x80);
	bytes[bytes.size()-1] = char(0xbf);
	QVERIFY(!model.load(bytes.constData(), bytes.size()));
	QVERIFY(!model.sample(0., 0., rgb));
}


void TestAtmosphereTransmission::incompleteNativeModel()
{
	QTemporaryDir directory;
	QVERIFY(directory.isValid());
	const QDir target(directory.path());
	const QDir source(QStringLiteral(TRANSMISSION_TEST_DATA "/default"));
	for (int set = 0; set < 4; ++set)
	{
		const QString subdir = QString("shaders/zero-order-scattering/%1").arg(set);
		QVERIFY(target.mkpath(subdir));
		const QString shader = subdir + "/render.frag";
		const QString texture = QString("transmittance-wlset%1.f32").arg(set);
		QVERIFY(QFile::copy(source.filePath(shader), target.filePath(shader)));
		QVERIFY(QFile::copy(source.filePath(texture), target.filePath(texture)));
	}
	AtmosphereTransmission model;
	QVERIFY(model.loadModel(directory.path()));
	QFile broken(target.filePath("transmittance-wlset1.f32"));
	QVERIFY(broken.open(QFile::ReadWrite));
	QVERIFY(broken.resize(broken.size()-1));
	broken.close();
	QVERIFY(!model.loadModel(directory.path()));
	std::array<float,3> rgb;
	QVERIFY(!model.sample(0., 0., rgb));
}

QTEST_GUILESS_MAIN(TestAtmosphereTransmission)
#include "testAtmosphereTransmission.moc"
