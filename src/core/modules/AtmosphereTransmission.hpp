/* Stellarium - GPL-2.0-or-later
 * CPU-only sampling of exported CalcMySky spectral optical depths.
 * Geometry follows CalcMySky's texture-coordinates.frag and the experimental
 * ShowMySky_directSolarTransmission_v1 API. No rendering context is required.
 */
#ifndef ATMOSPHERE_TRANSMISSION_HPP
#define ATMOSPHERE_TRANSMISSION_HPP

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>

class QString;

class AtmosphereTransmission
{
public:
	//! Read native CalcMySky optical depths and generated color coefficients.
	//! Does not load scattering textures or require a patched ShowMySky library.
	bool loadModel(const QString& directory);

	// Little-endian format: magic[8], version/u32, width/u32, height/u32,
	// wavelength count/u32, planet radius/f32, atmosphere height/f32,
	// normalized linear RGB weights[count][3], optical depth[height][width][count].
	bool load(const char* bytes, std::size_t size)
	{
		depths.clear(); weights.clear();
		if (!bytes || size < 32 || std::memcmp(bytes, "AtmoExt\0", 8)) return false;
		std::size_t pos = 8;
		const auto integer = [&]() {
			const auto p = reinterpret_cast<const unsigned char*>(bytes + pos);
			pos += 4;
			return uint32_t(p[0]) | uint32_t(p[1])<<8 | uint32_t(p[2])<<16 | uint32_t(p[3])<<24;
		};
		const auto real = [&]() { const auto bits = integer(); float f; std::memcpy(&f, &bits, 4); return f; };
		if (integer() != 1) return false;
		width = integer(); height = integer(); count = integer();
		radius = real(); thickness = real();
		if (width < 2 || width > 4096 || height < 2 || height > 4096 || count < 1 || count > 256 ||
		    !std::isfinite(radius) || !std::isfinite(thickness) || radius <= 0 || thickness <= 0) return false;
		const std::size_t samples = std::size_t(width)*height*count;
		if (size != 32 + 4*(3*count + samples)) return false;
		weights.resize(count);
		for (auto& weight : weights)
			for (auto& component : weight) { component = real(); if (!std::isfinite(component)) return false; }
		depths.resize(samples);
		for (auto& depth : depths)
		{
			depth = real();
			if (!std::isfinite(depth) || depth < 0) { depths.clear(); return false; }
		}
		return true;
	}

	bool sample(double altitude, double elevation, std::array<float, 3>& rgb) const
	{
		constexpr double halfPi = 1.57079632679489661923;
		if (depths.empty() || !std::isfinite(altitude) || !std::isfinite(elevation) ||
		    elevation < -halfPi || elevation > halfPi) return false;
		const double h = std::clamp(altitude, 0., thickness), r = radius+h;
		const double rho = std::sqrt(h*(h+2*radius));
		const double horizon = std::sqrt(thickness*(thickness+2*radius));
		const double mu = std::sin(elevation);
		if (mu < -rho/r) { rgb = {0,0,0}; return true; }
		const double dMin = thickness-h;
		const double d = std::sqrt(r*r*mu*mu+dMin*(2*r+dMin))-r*mu;
		const double x = std::clamp((d-dMin)/(horizon+rho-dMin),0.,1.)*(width-1);
		const double y = std::clamp(rho/horizon,0.,1.)*(height-1);
		const unsigned x0 = unsigned(x), y0 = unsigned(y);
		const unsigned x1 = std::min(x0+1,width-1), y1 = std::min(y0+1,height-1);
		std::array<double,3> sum = {0,0,0};
		for (unsigned n = 0; n < count; ++n)
		{
			const auto at = [&](unsigned xx,unsigned yy) { return depths[(std::size_t(yy)*width+xx)*count+n]; };
			const double lo = at(x0,y0)*(1-(x-x0))+at(x1,y0)*(x-x0);
			const double hi = at(x0,y1)*(1-(x-x0))+at(x1,y1)*(x-x0);
			const double transmission = std::exp(-std::max(0.,lo*(1-(y-y0))+hi*(y-y0)));
			for (unsigned c = 0; c < 3; ++c) sum[c] += weights[n][c]*transmission;
		}
		for (unsigned c = 0; c < 3; ++c) rgb[c] = std::clamp(float(sum[c]),0.f,1.f);
		return true;
	}

private:
	unsigned width = 0, height = 0, count = 0;
	double radius = 0, thickness = 0;
	std::vector<std::array<float,3>> weights;
	std::vector<float> depths;
};
#endif
