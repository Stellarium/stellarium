uniform mat4 tex_mat;

varying vec4 SM_tex_coord;
varying vec3 vecLight;
varying vec3 vecPos;
varying vec3 vecNormal;

void main(void)
{
	//Shadow texture coords in projected light space
	SM_tex_coord = tex_mat * gl_Vertex;
	
	gl_TexCoord[0] = gl_MultiTexCoord0;
	gl_Position = ftransform();
	
	//vec3 normal = normalize(gl_NormalMatrix * gl_Normal);
	//vec3 tangent = normalize(gl_NormalMatrix * tang);
	//vec3 binormal = cross(normal, tangent);
	
	vec3 lightDir = normalize(gl_LightSource[0].position.xyz);
	
	//Multiplication by inverse Normal Matrix fixes a bug - need to investigate this
	vecLight = normalize(inverse(gl_NormalMatrix) * lightDir);
	vecPos = gl_Vertex.xyz;
	vecNormal = normalize(gl_NormalMatrix * gl_Normal);
}