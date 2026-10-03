/*
 * Stellarium
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */


#include <QtTest>
#include <limits>
#include "StelPsf.hpp"

class TestStelPsf : public QObject
{
	Q_OBJECT
private slots:
	void glowTransition();
	void boundedGlowPeak();
	void resolvedDisc();
	void reflectiveGlow();
	void parameterBounds();
	void glowBounds();
};

void TestStelPsf::glowTransition()
{
	QCOMPARE(StelPsf::glowOnset(1.f), 0.f);
	QCOMPARE(StelPsf::glowOnset(1.25f), 0.15625f);
	QCOMPARE(StelPsf::glowOnset(1.5f), 0.5f);
	QCOMPARE(StelPsf::glowOnset(1.75f), 0.84375f);
	QCOMPARE(StelPsf::glowOnset(2.f), 1.f);
	const float justAbove = StelPsf::glowOnset(std::nextafter(1.f, 2.f));
	QVERIFY(justAbove > 0.f && justAbove < 1.e-6f);
	float previous = 0.f;
	for (int i = 0; i <= 3000; ++i)
	{
		const float alpha = StelPsf::glowOnset(i / 1000.f);
		QVERIFY(alpha >= previous && alpha <= 1.f);
		previous = alpha;
	}
}

void TestStelPsf::boundedGlowPeak()
{
	QCOMPARE(StelPsf::glowPeak(0.f), 0.f);
	QCOMPARE(StelPsf::glowPeak(1000.f), 500.f);
	QVERIFY(StelPsf::glowPeak(1.f) > 0.999f);
	float previous = 0.f;
	for (float peak : {1.f, 10.f, 100.f, 1000.f, 1.e10f, std::numeric_limits<float>::max()})
	{
		const float glow = StelPsf::glowPeak(peak);
		QVERIFY(std::isfinite(glow) && glow >= previous && glow <= 1000.f);
		previous = glow;
	}

	// Venus/Jupiter, 2012-03-14 18:30 UTC, Stockholm, 60-degree field,
	// default PSF and star scales. The former limit of 100 reduced these
	// measured input contrasts to 2.2 (atmosphere) and 1.6 (no atmosphere).
	QVERIFY(StelPsf::glowPeak(907.32f) / StelPsf::glowPeak(71.63f) > 5.f);
	QVERIFY(StelPsf::glowPeak(3637.14f) / StelPsf::glowPeak(156.69f) > 5.f);

	// A resolved Venus or Moon must not bloom over its mesh even if zoom
	// raises the integrated point-source exposure by many orders of magnitude.
	QCOMPARE(StelPsf::reflectiveGlowOnset(StelPsf::glowPeak(1.e10f, 4.f), 4.f, 1.5f), 0.f);
	for (float radius : {0.5f, 1.5f, 5.f})
	{
		const float end = std::max(2.f, radius);
		const float unresolved = StelPsf::glowPeak(1.e10f, 1.f, radius);
		const float resolved = StelPsf::glowPeak(1.e10f, end, radius);
		QVERIFY(resolved <= 100.f);
		QVERIFY(std::abs(StelPsf::glowPeak(1.e10f, std::nextafter(1.f, end), radius) - unresolved) < 0.001f);
		QVERIFY(std::abs(StelPsf::glowPeak(1.e10f, std::nextafter(end, 1.f), radius) - resolved) < 0.001f);
		previous = unresolved;
		for (int i = 0; i <= 2000; ++i)
		{
			const float glow = StelPsf::glowPeak(1.e10f, i * end / 1000.f, radius);
			QVERIFY(glow <= previous && glow >= resolved);
			previous = glow;
		}
	}
}

void TestStelPsf::resolvedDisc()
{
	for (float radius : {0.5f, 1.5f, 5.f})
		for (float scale : {0.5f, 1.f, 2.f, 4.f})
		{
			const float end = std::max(2.f, radius * scale);
			QCOMPARE(StelPsf::pointFade(0.f, radius, scale), 1.f);
			QCOMPARE(StelPsf::pointFade(1.f, radius, scale), 1.f);
			QCOMPARE(StelPsf::pointFade((1.f + end) / 2.f, radius, scale), 0.5f);
			QCOMPARE(StelPsf::pointFade(end, radius, scale), 0.f);
			QVERIFY(StelPsf::pointFade(std::nextafter(1.f, end), radius, scale) > 1.f - 1.e-6f);
			QVERIFY(StelPsf::pointFade(std::nextafter(end, 1.f), radius, scale) < 1.e-6f);
			float previous = 1.f;
			for (int i = 0; i <= 2000; ++i)
			{
				const float fade = StelPsf::pointFade(i * end / 1000.f, radius, scale);
				QVERIFY(fade <= previous && fade >= 0.f);
				previous = fade;
			}
		}
}

void TestStelPsf::reflectiveGlow()
{
	QCOMPARE(StelPsf::reflectiveGlowOnset(10.f, 0.f, 1.5f), 1.f);
	for (float radius : {0.5f, 1.5f, 5.f})
		for (float disc : {0.01f, 1.f, 10.f, 100.f})
		{
			const float linked = std::pow(disc * 3.14159265358979323846f / radius, 2.5f);
			QCOMPARE(StelPsf::reflectiveGlowOnset(linked, disc, radius), 0.f);
			QVERIFY(std::abs(StelPsf::reflectiveGlowOnset(1.5f * linked, disc, radius) - 0.5f) < 1.e-6f);
			QCOMPARE(StelPsf::reflectiveGlowOnset(2.f * linked, disc, radius), 1.f);
			float previous = 1.f;
			for (int i = 0; i <= 2000; ++i)
			{
				const float fade = StelPsf::reflectiveGlowOnset(linked, disc * i / 1000.f, radius);
				QVERIFY(fade <= previous && fade >= 0.f);
				previous = fade;
			}
		}
}

void TestStelPsf::parameterBounds()
{
	QCOMPARE(StelPsf::boundedFlareDecay(-1., 0.1f), 0.f);
	QCOMPARE(StelPsf::boundedFlareDecay(0., 0.1f), 0.f);
	QCOMPARE(StelPsf::boundedFlareDecay(0.001, 0.1f), 0.05f);
	QCOMPARE(StelPsf::boundedFlareDecay(0.01, 0.1f), 0.05f);
	QCOMPARE(StelPsf::boundedFlareDecay(0.1, 0.5f), 0.1f);
	QCOMPARE(StelPsf::boundedFlareDecay(10., 0.1f), 1.f);
	QCOMPARE(StelPsf::boundedFlareDecay(std::numeric_limits<double>::quiet_NaN(), 0.2f), 0.2f);
	QCOMPARE(StelPsf::boundedFlareDecay(std::numeric_limits<double>::infinity(), 0.1f), 1.f);
	QCOMPARE(StelPsf::boundedFlareDecay(-std::numeric_limits<double>::infinity(), 0.1f), 0.f);
}

void TestStelPsf::glowBounds()
{
	constexpr double threshold = 1. / (255. * 12.92);
	for (float peak : {1.1f, 2.f, 10.f, 1000.f, 1.e6f})
		for (float radius : {0.5f, 1.5f, 5.f})
			for (float decay : {0.05f, 0.1f, 1.f})
				for (float strength : {1.e-5f, 0.001f, 0.5f, 1.f, 20.f})
				{
					const float bound = StelPsf::glowRadius(peak, strength, radius, decay);
					if (peak * strength <= threshold)
					{
						QCOMPARE(bound, 0.f);
						continue;
					}
					QVERIFY(std::isfinite(bound) && bound > 0.f);
					QVERIFY(bound < std::pow(peak, 0.4f) * radius / decay);
					// Evaluate the shader profile independently in double precision.
					const double a = decay / double(radius);
					const double b = radius / (3.14159265358979323846 - decay);
					const double s = (std::pow(double(peak), 0.4) / bound - a) * b;
					const double radiance = std::min(std::pow(std::max(0., s), 2.5), double(peak)) * strength;
					QVERIFY(std::abs(radiance / threshold - 1.) < 0.005);
				}
	QCOMPARE(StelPsf::glowRadius(2.f, 0.f, 1.5f, 0.1f), 0.f);
	QCOMPARE(StelPsf::glowRadius(2.f, 1.f, 1.5f, 0.f), 0.f);
	QCOMPARE(StelPsf::glowRadius(2.f, 1.f, 1.5f, 4.f), 0.f);
	QCOMPARE(StelPsf::glowRadius(std::numeric_limits<float>::infinity(), 1.f, 1.5f, 0.1f), 0.f);
	QCOMPARE(StelPsf::glowRadius(2.f, std::numeric_limits<float>::quiet_NaN(), 1.5f, 0.1f), 0.f);
	// No radius floor at the former 0.001 alpha clamp, or at the core radius.
	float previous = 0.f;
	for (int i = 0; i <= 2000; ++i)
	{
		const float bound = StelPsf::glowRadius(10.f, i * 1.e-6f, 1.5f, 0.1f);
		QVERIFY(bound >= previous);
		previous = bound;
	}
}

QTEST_GUILESS_MAIN(TestStelPsf)
#include "testStelPsf.moc"
