#include "NoiseShader.h"
#include "glew.h"

namespace
{
	const char* cVertShaderSource = 
		" varying vec2 TexCoord;\
		varying vec3 Vertex;\
		varying vec3 Normal;\
		varying vec3 View;\
		void main()\
		{   \
			vec3 position = vec3(gl_ModelViewMatrix * gl_Vertex);\
			Normal = normalize(gl_NormalMatrix * gl_Normal);\
			View = normalize(-position);\
 			Vertex = gl_Vertex.xyz; \
			TexCoord = gl_MultiTexCoord0.st;\
			gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\
		}\
		";

	/*const char* cFragShaderSource = 
		"uniform sampler3D NoiseTexture;                                                            \
		uniform float Time;                                                                         \
		varying vec2 TexCoord;                                                                      \
		varying vec3 Vertex; \
		varying vec3 Normal;\
		varying vec3 View;\
		const vec3 Light = vec3(0.0, 0.0, 1.0);\
		const float NoiseScale = 3.0;\
		const float NoiseContribution = 0.7;\
		float fBm(const vec3 p){\
			float noise = 0.0;\
			float scale = 0.0;\
			for (int k = 0; k < 5; k++){\
				float val = pow(2.0, float(k)); \
				noise += (2.0 * texture3D(NoiseTexture, p * val).a-0.05) / val; \
				scale += 1.0 / val;} \
			return noise / scale;} \
void main(void)\
{\
	vec3 surface = vec3(0.89,0.36,0.0);	\
	vec3 light = normalize(Light);\
	vec3 view = normalize(View);\
	vec3 normal = normalize(Normal);	\
	vec3 refl = reflect(-light, normal);\
	vec3 period_variable1 = (0.75 * vec3(abs(0.1*Time), abs(0.1*Time), abs(0.1*Time)));\
	vec3 color = surface + 0.4 * vec3(fBm(Vertex * (NoiseScale + abs(0.5*(period_variable1-floor(period_variable1)-0.5)))))- 0.35*vec3(fBm(Vertex * (NoiseScale + 0.73345256 + abs(period_variable1-floor(period_variable1)-0.5))));\
	gl_FragColor = vec4(color, 1.0);\
}\
";
}*/
const char* cFragShaderSource = 
" varying vec2 TexCoord;\
  varying vec3 Vertex;\
  varying vec3 Normal;\
  varying vec3 View;\
void main(void)\
{\
    vec3 color = vec3(1,0,0);\
	gl_FragColor = vec4(color, 1.0);\
}\
";
}
NoiseShader::NoiseShader() : 
	Shader(cVertShaderSource, cFragShaderSource), mTexMap(0)
{
}

NoiseShader::~NoiseShader()
{
}

void NoiseShader::setParams(unsigned int programObject, int startActiveTex) const
{
}	

int NoiseShader::getActiveTexCount() const
{
	return 0;
}

void NoiseShader::setTexMap(unsigned int texMap)
{
	mTexMap = texMap;
}

NoiseShader* NoiseShader::instance()
{
	static NoiseShader shader;
	return &shader;
}