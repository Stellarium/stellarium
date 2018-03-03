/*
 * Stellarium Scenery3d Plug-in
 *
 * Copyright (C) 2014 Simon Parzer, Peter Neubauer, Georg Zotti, Andrei Borza, Florian Schaukowitsch
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */
 
 
/*
This is a shader for phong/per-pixel lighting.
*/
 
//macros that can be set by ShaderManager (simple true/false flags)
#define SHADOWS 1
#define SINGLE_SHADOW_FRUSTUM 0
#define BUMP 1
#define HEIGHT 1
#define GEOMETRY_SHADER 0

//matrices
uniform mat4 u_mModelView;
#if ! GEOMETRY_SHADER
uniform mat4 u_mProjection;
#endif
uniform mat3 u_mNormal;

uniform vec3 u_vLightDirectionView; //in view space, from point to light

#if SHADOWS
//shadow transforms
uniform mat4 u_mShadow0;
#if !SINGLE_SHADOW_FRUSTUM 
uniform mat4 u_mShadow1;
uniform mat4 u_mShadow2;
uniform mat4 u_mShadow3;
#endif
#endif

attribute vec4 a_vertex;
attribute vec3 a_normal;
attribute vec2 a_texcoord;
#if BUMP
attribute vec4 a_tangent;
#endif

#if GEOMETRY_SHADER
#define VAR_TEXCOORD v_texcoordGS
#define VAR_NORMAL v_normalGS
#define VAR_LIGHTVEC v_lightVecGS
#define VAR_VIEWPOS v_viewPosGS
#define VAR_SHADOWCOORD0 v_shadowCoord0GS
#define VAR_SHADOWCOORD1 v_shadowCoord1GS
#define VAR_SHADOWCOORD2 v_shadowCoord2GS
#define VAR_SHADOWCOORD3 v_shadowCoord3GS
#else
#define VAR_TEXCOORD v_texcoord
#define VAR_NORMAL v_normal
#define VAR_LIGHTVEC v_lightVec
#define VAR_VIEWPOS v_viewPos
#define VAR_SHADOWCOORD0 v_shadowCoord0
#define VAR_SHADOWCOORD1 v_shadowCoord1
#define VAR_SHADOWCOORD2 v_shadowCoord2
#define VAR_SHADOWCOORD3 v_shadowCoord3
#endif

varying vec3 VAR_NORMAL; //normal in view space
varying vec2 VAR_TEXCOORD;
varying vec3 VAR_LIGHTVEC; //light vector, in VIEW or TBN space according to bump settings
varying vec3 VAR_VIEWPOS; //position of fragment in view space

#if SHADOWS
//varying arrays seem to cause some problems, so we use 4 vecs for now...
varying vec4 VAR_SHADOWCOORD0;
#if !SINGLE_SHADOW_FRUSTUM 
varying vec4 VAR_SHADOWCOORD1;
varying vec4 VAR_SHADOWCOORD2;
varying vec4 VAR_SHADOWCOORD3;
#endif
#endif

void main(void)
{
	//transform normal
	VAR_NORMAL = normalize(u_mNormal * a_normal);
	
	//pass on tex coord
	VAR_TEXCOORD = a_texcoord;
	
	//calc vertex pos in view space
	vec4 viewPos = u_mModelView * a_vertex;
	VAR_VIEWPOS = viewPos.xyz;
	
	#if SHADOWS
	//calculate shadowmap coords
	VAR_SHADOWCOORD0 = u_mShadow0 * a_vertex;
	#if !SINGLE_SHADOW_FRUSTUM 
	VAR_SHADOWCOORD1 = u_mShadow1 * a_vertex;
	VAR_SHADOWCOORD2 = u_mShadow2 * a_vertex;
	VAR_SHADOWCOORD3 = u_mShadow3 * a_vertex;
	#endif
	#endif
	
	#if BUMP
	//create View-->TBN matrix
	vec3 t = normalize(u_mNormal * a_tangent.xyz);
	//bitangent recreated from normal and tangent instead passed as attribute for a bit more orthonormality
	vec3 b = cross(VAR_NORMAL, t) * a_tangent.w; //w coordinate stores handedness of tangent space
	
	mat3 TBN = mat3(t.x, b.x, VAR_NORMAL.x,
					t.y, b.y, VAR_NORMAL.y,
					t.z, b.z, VAR_NORMAL.z);
	VAR_LIGHTVEC = TBN * u_vLightDirectionView;
	VAR_VIEWPOS = TBN * VAR_VIEWPOS;
	#else
	VAR_LIGHTVEC = u_vLightDirectionView;
	#endif
	
	//calc final position
	#if GEOMETRY_SHADER
	gl_Position = a_vertex; //pass on unchanged
	#else
	gl_Position = u_mProjection * viewPos;
	#endif
}