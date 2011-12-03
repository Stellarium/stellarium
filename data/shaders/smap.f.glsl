uniform sampler2D texture;
uniform sampler2D smap;

varying vec4 diffuse;
varying vec4 SM_tex_coord;

void main(void)
{
	vec4 texColor = texture2D(texture, gl_TexCoord[0].st);

	vec3 tex_coords = SM_tex_coord.xyz/SM_tex_coord.w;
	
	float depth = texture2D(smap, tex_coords.xy).x;
	
	//Shadow factor: 1.0 = light, 0.0 = shadow
    float factor = 0.0;
	
	if(depth >= tex_coords.z)
	{
		//In light!
		factor = 1.0;
	}
	
	gl_FragColor = texColor * (gl_LightSource[0].ambient + diffuse * factor);
}