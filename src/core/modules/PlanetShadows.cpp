#include "PlanetShadows.hpp"

#include <QOpenGLContext>
#include <QOpenGLShader>
#include "StelPainter.hpp"
#include "StelApp.hpp"
#include "StelTextureMgr.hpp"
#include "StelTexture.hpp"
#include "StelPainter.hpp"

PlanetShadows* PlanetShadows::instance = NULL;

PlanetShadows *PlanetShadows::getInstance()
{
	if(instance == NULL)
		instance = new PlanetShadows();

	return instance;
}

void PlanetShadows::cleanup()
{
	delete instance;
	instance = NULL;
}

bool PlanetShadows::isSupported()
{
	return supported;
}

bool PlanetShadows::isActive()
{
	return current != 0;
}

void PlanetShadows::setData(const char *data, int size, int count)
{
	if(!supported)
		return;

	infoCount = count;
	infoSize = size;
	// XXX: this does not work with OpenGL ES2 / ANGLE.
	texture->loadFromMemory(data, size, size, GL_RGBA, GL_FLOAT, GL_RGBA32F);
}

void PlanetShadows::setCurrent(int current)
{
	this->current = current;
}

void PlanetShadows::setRings(StelTextureSP rings_texture, float min, float max)
{
	this->rings_texture = rings_texture;
	rings_min = min;
	rings_max = max;
}

void PlanetShadows::setMoon(StelTextureSP moon_texture)
{
	this->moon_texture = moon_texture;
}

void PlanetShadows::setupShading(StelPainter* sPainter, float radius, float oneMinusOblateness, bool isRing)
{
	if(!supported)
		return;

	StelPainterLight& light = sPainter->getLight();

	Vec4f lightPos = light.getPosition();
	const StelProjectorP& projector = sPainter->getProjector();


	Vec3f lightPos3;
	lightPos3.set(lightPos[0], lightPos[1], lightPos[2]);
	Vec3f tmpv(0.f);
	projector->getModelViewTransform()->forward(tmpv); // -posCenterEye
	projector->getModelViewTransform()->getApproximateLinearTransfo().transpose().multiplyWithoutTranslation(Vec3d(lightPos3[0], lightPos3[1], lightPos3[2]));
	projector->getModelViewTransform()->backward(lightPos3);
	lightPos3.normalize();

	Vec4f& diffuse = light.getDiffuse();
	Vec4f& ambient = light.getAmbient();

	shaderProgram->bind();

	shaderProgram->setUniformValue(shaderVars.lightPos, lightPos3[0], lightPos3[1], lightPos3[2]);
	shaderProgram->setUniformValue(shaderVars.diffuseLight, diffuse[0], diffuse[1], diffuse[2], diffuse[3]);
	shaderProgram->setUniformValue(shaderVars.ambientLight, ambient[0], ambient[1], ambient[2], ambient[3]);
	shaderProgram->setUniformValue(shaderVars.radius, radius);
	shaderProgram->setUniformValue(shaderVars.oneMinusOblateness, oneMinusOblateness);
	shaderProgram->setUniformValue(shaderVars.texture, 0);

	shaderProgram->setUniformValue(shaderVars.info, 1);
	shaderProgram->setUniformValue(shaderVars.infoCount, infoCount);
	shaderProgram->setUniformValue(shaderVars.infoSize, infoSize);
	shaderProgram->setUniformValue(shaderVars.current, current);

	shaderProgram->setUniformValue(shaderVars.isRing, isRing);

	const bool ring = (!rings_texture.isNull());
	if(ring)
		rings_texture->bind(2);

	shaderProgram->setUniformValue(shaderVars.ring, ring);
	shaderProgram->setUniformValue(shaderVars.outerRadius, rings_max);
	shaderProgram->setUniformValue(shaderVars.innerRadius, rings_min);
	shaderProgram->setUniformValue(shaderVars.ringS, ring ? 2 : 0);

	const bool moon = !moon_texture.isNull();
	if(moon)
		moon_texture->bind(3);
	shaderProgram->setUniformValue(shaderVars.isMoon, moon);
	shaderProgram->setUniformValue(shaderVars.earthShadow, moon ? 3: 0);

	texture->bind(1);
}

QOpenGLShaderProgram* PlanetShadows::setupGeneralUniforms(const QMatrix4x4 &projection)
{
	if(!supported)
		return NULL;

	GLenum er = glGetError();
	if (er!=GL_NO_ERROR)
	{
		if (er==GL_INVALID_OPERATION)
			qFatal("Invalid openGL operation detected in ~StelPainter()");
	}
	shaderProgram->setUniformValue(shaderVars.projectionMatrix, projection);
	er = glGetError();
	if (er!=GL_NO_ERROR)
	{
		if (er==GL_INVALID_OPERATION)
			qFatal("Invalid openGL operation detected in ~StelPainter()");
	}
	return shaderProgram;
}

PlanetShadows::~PlanetShadows()
{
	delete shaderProgram;
}

PlanetShadows::PlanetShadows()
	: shaderProgram(NULL)
	, shaderVars(ShaderVars())
	, rings_min(0.f)
	, rings_max(0.f)
	, current(0)
	, infoCount(0)
	, infoSize(0)
{
	//std::string extensions = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
	supported = false; // extensions.find("GL_ARB_texture_float") != std::string::npos;

	if(supported)
	{
		initShader();

		StelTexture::StelTextureParams params(false, GL_NEAREST, GL_CLAMP_TO_EDGE);
		texture = StelApp::getInstance().getTextureManager().createMemoryTexture(params);
	}
}

void PlanetShadows::initShader()
{
	// Basic texture shader program
	QOpenGLShader vshader(QOpenGLShader::Vertex);
	const char *vsrc =
		"attribute highp vec3 vertex;\n"
		"attribute highp vec3 unprojectedVertex;\n"
		"attribute mediump vec2 texCoord;\n"
		"uniform mediump mat4 projectionMatrix;\n"
		"uniform highp vec3 lightPos;\n"
		"uniform highp float oneMinusOblateness;\n"
		"uniform highp float radius;\n"
		"varying mediump vec2 texc;\n"
		"varying mediump float lambert;\n"
		"varying highp vec3 P;\n"
		"\n"
		"void main()\n"
		"{\n"
		"    gl_Position = projectionMatrix * vec4(vertex, 1.);\n"
		"    texc = texCoord;\n"
		"    // Must be a separate variable due to Intel drivers\n"
		"    vec3 normal = unprojectedVertex / radius;\n"
		"    float c = lightPos.x * normal.x * oneMinusOblateness +\n"
		"              lightPos.y * normal.y * oneMinusOblateness +\n"
		"              lightPos.z * normal.z / oneMinusOblateness;\n"
		"    lambert = clamp(c, 0.0, 0.5);\n"
		"\n"
		"    P = unprojectedVertex;\n"
		"}\n"
		"\n";
	vshader.compileSourceCode(vsrc);
	if (!vshader.log().isEmpty()) { qWarning() << "PlanetShadows: Warnings while compiling vshader: " << vshader.log(); }

	QOpenGLShader fshader(QOpenGLShader::Fragment);
	const char *fsrc =
		"varying mediump vec2 texc;\n"
		"varying mediump float lambert;\n"
		"uniform sampler2D tex;\n"
		"uniform mediump vec4 ambientLight;\n"
		"uniform mediump vec4 diffuseLight;\n"
		"\n"
		"varying highp vec3 P;\n"
		"\n"
		"uniform sampler2D info;\n"
		"uniform int current;\n"
		"uniform int infoCount;\n"
		"uniform float infoSize;\n"
		"\n"
		"uniform bool ring;\n"
		"uniform highp float outerRadius;\n"
		"uniform highp float innerRadius;\n"
		"uniform sampler2D ringS;\n"
		"uniform bool isRing;\n"
		"\n"
		"uniform bool isMoon;\n"
		"uniform sampler2D earthShadow;\n"
		"\n"
		"bool visible(vec3 normal, vec3 light)\n"
		"{\n"
		"    return (dot(light, normal) > 0.0);\n"
		"}\n"
		"\n"
		"void main()\n"
		"{\n"
		"    float final_illumination = 1.0;\n"
		"    vec4 diffuse = diffuseLight;\n"
		"    vec4 data = texture2D(info, vec2(0.0, current) / infoSize);\n"
		"    float RS = data.w;\n"
		"    vec3 Lp = data.xyz;\n"
		"\n"
		"    vec3 P3;\n"
		"\n"
		"    if(isRing)\n"
		"        P3 = P;\n"
		"    else\n"
		"    {\n"
		"        data = texture2D(info, vec2(current, current) / infoSize);\n"
		"        P3 = normalize(P) * data.w;\n"
		"    }\n"
		"\n"
		"    float L = length(Lp - P3);\n"
		"    RS = L * tan(asin(RS / L));\n"
		"    float R = atan(RS / L); //RS / L;\n"
		"\n"
		"    if((lambert > 0.0) || isRing)\n"
		"    {\n"
		"        if(ring && !isRing)\n"
		"        {\n"
		"            vec3 ray = normalize(Lp);\n"
		"            vec3 normal = normalize(vec3(0.0, 0.0, 1.0));\n"
		"            float u = - dot(P3, normal) / dot(ray, normal);\n"
		"\n"
		"            if(u > 0.0 && u < 1e10)\n"
		"            {\n"
		"                float ring_radius = length(P3 + u * ray);\n"
		"\n"
		"                if(ring_radius > innerRadius && ring_radius < outerRadius)\n"
		"                {\n"
		"                    ring_radius = (ring_radius - innerRadius) / (outerRadius - innerRadius);\n"
		"                    data = texture2D(ringS, vec2(ring_radius, 0.5));\n"
		"\n"
		"                    final_illumination = 1.0 - data.w;\n"
		"                }\n"
		"            }\n"
		"        }\n"
		"\n"
		"        for(int i = 1; i < infoCount; i++)\n"
		"        {\n"
		"            if(current == i && !isRing)\n"
		"                continue;\n"
		"\n"
		"            data = texture2D(info, vec2(i, current) / infoSize);\n"
		"            vec3 C = data.rgb;\n"
		"            float radius = data.a;\n"
		"\n"
		"            float l = length(C - P3);\n"
		"            radius = l * tan(asin(radius / l));\n"
		"            float r = atan(radius / l); //radius / l;\n"
		"            float d = acos(min(1.0, dot(normalize(Lp - P3), normalize(C - P3)))); //length( (Lp - P3) / L - (C - P3) / l );\n"
		"\n"
		"            float illumination = 1.0;\n"
		"\n"
		"            // distance too far\n"
		"            if(d >= R + r)\n"
		"            {\n"
		"                illumination = 1.0;\n"
		"            }\n"
		"            // umbra\n"
		"            else if(r >= R + d)\n"
		"            {\n"
		"                if(isMoon)\n"
		"                    illumination = d / (r - R) * 0.6;\n"
		"                else"
		"                    illumination = 0.0;\n"
		"            }\n"
		"            // penumbra completely inside\n"
		"            else if(d + r <= R)\n"
		"            {\n"
		"                illumination = 1.0 - r * r / (R * R);\n"
		"            }\n"
		"            // penumbra partially inside\n"
		"            else\n"
		"            {\n"
		"                if(isMoon)\n"
		"                    illumination = ((d - abs(R-r)) / (R + r - abs(R-r))) * 0.4 + 0.6;\n"
		"                else\n"
		"                {\n"
		"                    float x = (R * R + d * d - r * r) / (2.0 * d);\n"
		"\n"
		"                    float alpha = acos(x / R);\n"
		"                    float beta = acos((d - x) / r);\n"
		"\n"
		"                    float AR = R * R * (alpha - 0.5 * sin(2.0 * alpha));\n"
		"                    float Ar = r * r * (beta - 0.5 * sin(2.0 * beta));\n"
		"                    float AS = R * R * 2.0 * asin(1.0);\n"
		"\n"
		"                    illumination = 1.0 - (AR + Ar) / AS;\n"
		"                }\n"
		"            }\n"
		"\n"
		"            if(illumination < final_illumination)\n"
		"                final_illumination = illumination;\n"
		"        }\n"
		"    }\n"
		"\n"
		"    vec4 litColor = (isRing ? 1.0 : lambert) * final_illumination * diffuse + ambientLight;\n"
		"    if(isMoon && final_illumination < 1.0)\n"
		"    {\n"
		"        vec4 shadowColor = texture2D(earthShadow, vec2(final_illumination, 0.5));\n"
		"        gl_FragColor = mix(texture2D(tex, texc) * litColor, shadowColor, shadowColor.a);\n"
		"    }\n"
		"    else\n"
		"        gl_FragColor = texture2D(tex, texc) * litColor;\n"
		"}\n"
		"\n";
	fshader.compileSourceCode(fsrc);
	if (!fshader.log().isEmpty()) { qWarning() << "PlanetShadows: Warnings while compiling fshader: " << fshader.log(); }

	shaderProgram = new QOpenGLShaderProgram(QOpenGLContext::currentContext());
	shaderProgram->addShader(&vshader);
	shaderProgram->addShader(&fshader);
	StelPainter::linkProg(shaderProgram, "planetShaderProgram");
	shaderVars.projectionMatrix = shaderProgram->uniformLocation("projectionMatrix");
	shaderVars.texCoord = shaderProgram->attributeLocation("texCoord");
	shaderVars.unprojectedVertex = shaderProgram->attributeLocation("unprojectedVertex");
	shaderVars.vertex = shaderProgram->attributeLocation("vertex");
	shaderVars.texture = shaderProgram->uniformLocation("tex");

	shaderVars.lightPos = shaderProgram->uniformLocation("lightPos");
	shaderVars.diffuseLight = shaderProgram->uniformLocation("diffuseLight");
	shaderVars.ambientLight = shaderProgram->uniformLocation("ambientLight");
	shaderVars.radius = shaderProgram->uniformLocation("radius");
	shaderVars.oneMinusOblateness = shaderProgram->uniformLocation("oneMinusOblateness");
	shaderVars.info = shaderProgram->uniformLocation("info");
	shaderVars.infoCount = shaderProgram->uniformLocation("infoCount");
	shaderVars.infoSize = shaderProgram->uniformLocation("infoSize");
	shaderVars.current = shaderProgram->uniformLocation("current");
	shaderVars.isRing = shaderProgram->uniformLocation("isRing");
	shaderVars.ring = shaderProgram->uniformLocation("ring");
	shaderVars.outerRadius = shaderProgram->uniformLocation("outerRadius");
	shaderVars.innerRadius = shaderProgram->uniformLocation("innerRadius");
	shaderVars.ringS = shaderProgram->uniformLocation("ringS");
	shaderVars.isMoon = shaderProgram->uniformLocation("isMoon");
	shaderVars.earthShadow = shaderProgram->uniformLocation("earthShadow");
}
