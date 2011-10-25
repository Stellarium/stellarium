attribute vec3 tang;
attribute vec3 pvec;
uniform vec3 lpos;

varying vec3 var_ldir;
varying vec3 var_vdir;
varying vec3 var_norm;

varying vec3 pos;

void main()
{
	gl_Position = gl_ProjectionMatrix * vec4(pvec.x, pvec.y, pvec.z, 1.0);

	vec3 vpos = (gl_ModelViewMatrix * gl_Vertex).xyz;
	vec3 normal = normalize(gl_NormalMatrix * gl_Normal);
	vec3 tangent = normalize(gl_NormalMatrix * tang);
	vec3 binormal = cross(normal, tangent);

//	vec3 lpos = gl_LightSource[0].position.xyz;
	vec3 ldir = normalize(lpos - vpos);

	gl_TexCoord[0] = gl_MultiTexCoord0;

	mat3 tbnv = mat3(tangent.x, binormal.x, normal.x,
			tangent.y, binormal.y, normal.y,
			tangent.z, binormal.z, normal.z);

	var_ldir = tbnv * ldir;
	var_vdir = -(tbnv * pvec);
	pos = gl_Vertex.xyz;

	var_norm = normal;
}
