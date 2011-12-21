uniform sampler2D tex;
uniform sampler2D smap;

varying vec4 diffuse;
varying vec4 SM_tex_coord;

void main(void)
{
	vec4 texColor = texture(tex, gl_TexCoord[0].st);

	vec3 tex_coords = SM_tex_coord.xyz/SM_tex_coord.w;
	
	float depth = texture(smap, tex_coords.xy).x;
	
	//ESM
	float overdark = 80.0f;
	float lit = exp(overdark * (depth - tex_coords.z));
	lit = clamp(lit, 0.0, 1.0);
 
	vec4 color = texColor * (gl_LightSource[0].ambient + diffuse * lit);
	gl_FragColor = vec4(color.xyz, 1.0);
}