uniform mat4 tex_mat;

varying vec4 diffuse;
varying vec4 SM_tex_coord;

void main(void)
{
	//Shadow texture coords in projected light space
	SM_tex_coord = tex_mat * gl_Vertex;
	
	gl_TexCoord[0] = gl_MultiTexCoord0;
	gl_Position = ftransform();

	vec3 normal = gl_NormalMatrix * gl_Normal;
	vec3 lightVector = normalize(gl_LightSource[0].position.xyz);

	diffuse = gl_LightSource[0].diffuse * max(0.0, dot(normal, lightVector));
}