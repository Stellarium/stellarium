uniform sampler2D tex;
uniform sampler2D smap;
uniform sampler2D nmap;

varying vec4 SM_tex_coord;
varying vec3 vecLight;
varying vec3 vecPosition;
varying vec3 vecNormal;

void main(void)
{
	//Derived position
	vec3 p_dx = dFdx(vecPosition);
	vec3 p_dy = dFdy(vecPosition);
	//Compute derivations of the texture coordinate
	//Derivations give a flat TBN basis for each point of a polygon.
	//In order to get a smooth one we have to re-orthogonalize it based on a given (smooth) vertex normal. 
	vec2 tc_dx = dFdx(gl_TexCoord[0].st);
	vec2 tc_dy = dFdy(gl_TexCoord[0].st);
	// compute initial tangent and bi-tangent
	vec3 t = normalize(tc_dy.y * p_dx - tc_dx.y * p_dy );
	vec3 b = normalize(tc_dy.x * p_dx - tc_dx.x * p_dy ); // sign inversion
	// get new tangent from a given mesh normal
	vec3 n = normalize(vecNormal);
	vec3 x = cross(n, t);
	t = cross(x, n);
	t = normalize(t);
	// get updated bi-tangent
	x = cross(b, n);
	b = cross(n, x);
	b = normalize(b);
	mat3 tbn = mat3(t, b, n);

	vecLight = tbn * vecLight;

	vec3 normal = texture(nmap, gl_TexCoord[0].st).xyz * 2.0 - 1.0;
	normal = normalize(normal);
	
	vec3 ldir = normalize(vecLight);
	
	vec4 texColor = texture(tex, gl_TexCoord[0].st);
	
	vec4 diffuse = gl_LightSource[0].diffuse * max(0.0, dot(normal, ldir));
	
	vec3 tex_coords = SM_tex_coord.xyz/SM_tex_coord.w;
	float depth = texture(smap, tex_coords.xy).x;
	
	float factor = 0.0;
	if(depth > (tex_coords.z + 0.00001))
	{
		//In light!
		factor = 1.0;
	}
	
	////ESM
	//float overdark = 80.0f;
	//float lit = exp(overdark * (depth - tex_coords.z));
	//lit = clamp(lit, 0.0, 1.0);
 
 
	vec4 color = texColor * (gl_LightSource[0].ambient + diffuse * factor);
	gl_FragColor = vec4(color.xyz, 1.0);
}