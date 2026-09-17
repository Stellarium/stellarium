/* Stellarium - GPL-2.0-or-later
 * Read the same precomputed direct transmission used by ShowMySky.
 */
#include "AtmosphereTransmission.hpp"

#include <QDir>
#include <QFile>
#include <QRegularExpression>
#include <QtEndian>

bool AtmosphereTransmission::loadModel(const QString& directory)
{
	depths.clear();
	weights.clear();
	// Build separately so a truncated or unsupported model cannot leave usable
	// partial data behind. These are native CalcMySky files, not a new export.
	AtmosphereTransmission model;
	const QDir dir(directory);
	const auto files = dir.entryList({"transmittance-wlset*.f32"}, QDir::Files);
	if (files.isEmpty() || files.size() > 64) return false;
	model.count = 4 * files.size();
	model.weights.resize(model.count);
	std::array<double,3> white = {0., 0., 0.};
	for (int set = 0; set < files.size(); ++set)
	{
		QFile shader(dir.filePath(QString("shaders/zero-order-scattering/%1/render.frag").arg(set)));
		if (!shader.open(QFile::ReadOnly) || shader.size() > 4*1024*1024) return false;
		const QString source = QString::fromUtf8(shader.readAll());
		// CalcMySky embeds its exact solar spectrum, geometry and CIE quadrature
		// in this shader, for both radiance and luminance atmosphere models.
		const auto constant = [&](const QString& type, const QString& name, int size, std::vector<double>& values) {
			const QString value = size == 1 ? "([^;]+)" : type + "\\s*\\(([^)]+)\\)";
			const QRegularExpression expression("\\bconst\\s+" + type + "\\s+" + name + "\\s*=\\s*" + value + "\\s*;");
			const auto match = expression.match(source);
			if (!match.hasMatch()) return false;
			const auto fields = match.captured(1).split(',');
			if (fields.size() != size) return false;
			values.clear();
			for (const auto& field : fields)
			{
				bool ok;
				const double number = field.trimmed().toDouble(&ok);
				if (!ok || !std::isfinite(number)) return false;
				values.push_back(number);
			}
			return true;
		};
		std::vector<double> radiusValue, heightValue, solar, toXYZ;
		if (!constant("float", "earthRadius", 1, radiusValue) ||
		    !constant("float", "atmosphereHeight", 1, heightValue) ||
		    !constant("vec4", "solarIrradianceAtTOA", 4, solar) ||
		    !constant("mat4", "radianceToLuminance", 16, toXYZ)) return false;
		if (radiusValue[0] <= 0. || heightValue[0] <= 0.) return false;
		if (set && (model.radius != radiusValue[0] || model.thickness != heightValue[0])) return false;
		model.radius = radiusValue[0];
		model.thickness = heightValue[0];
		for (int c = 0; c < 4; ++c)
		{
			if (solar[c] < 0.) return false;
			const double x = solar[c]*toXYZ[4*c], y = solar[c]*toXYZ[4*c+1], z = solar[c]*toXYZ[4*c+2];
			const std::array<double,3> rgb = {3.2406*x-1.5372*y-0.4986*z,
			                               -0.9689*x+1.8758*y+0.0415*z,
			                               0.0557*x-0.2040*y+1.0570*z};
			for (unsigned channel = 0; channel < 3; ++channel)
			{
				model.weights[4*set+c][channel] = rgb[channel];
				white[channel] += rgb[channel];
			}
		}
		QFile texture(dir.filePath(QString("transmittance-wlset%1.f32").arg(set)));
		if (!texture.open(QFile::ReadOnly)) return false;
		const auto header = texture.read(4);
		if (header.size() != 4) return false;
		const unsigned w = qFromLittleEndian<quint16>(header.constData());
		const unsigned h = qFromLittleEndian<quint16>(header.constData()+2);
		if (w < 2 || w > 4096 || h < 2 || h > 4096 ||
		    texture.size() != 4 + qint64(w)*h*16 ||
		    std::size_t(w)*h*model.count > 64*1024*1024) return false;
		if (set && (model.width != w || model.height != h)) return false;
		model.width = w;
		model.height = h;
		if (!set) model.depths.resize(std::size_t(w)*h*model.count);
		const auto bytes = texture.readAll();
		if (bytes.size() != qint64(w)*h*16) return false;
		for (std::size_t pixel = 0; pixel < std::size_t(w)*h; ++pixel)
			for (unsigned c = 0; c < 4; ++c)
			{
				const auto bits = qFromLittleEndian<quint32>(bytes.constData()+16*pixel+4*c);
				float depth;
				std::memcpy(&depth, &bits, sizeof(depth));
				if (!std::isfinite(depth) || depth < 0.f) return false;
				model.depths[pixel*model.count+4*set+c] = depth;
			}
	}
	for (unsigned c = 0; c < 3; ++c)
	{
		if (!std::isfinite(white[c]) || white[c] <= 0.) return false;
		for (auto& weight : model.weights)
		{
			weight[c] /= white[c];
			if (!std::isfinite(weight[c])) return false;
		}
	}
	*this = std::move(model);
	return true;
}
