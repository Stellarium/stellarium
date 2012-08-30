const highp float pi = 3.1415926535897931;
const highp float ln10 = 2.3025850929940459;

// Variable for the xyYTo RGB conversion
uniform highp float alphaWaOverAlphaDa;
uniform highp float oneOverGamma;
uniform highp float term2TimesOneOverMaxdLpOneOverGamma;
uniform highp float brightnessScale;

// Variables for the color computation
uniform highp vec3 sunPos;
uniform mediump float term_x, Ax, Bx, Cx, Dx, Ex;
uniform mediump float term_y, Ay, By, Cy, Dy, Ey;

// The current projection matrix
uniform mediump mat4 projectionMatrix;

// Contains the 2d position of the point on the screen (before multiplication by the projection matrix)
attribute mediump vec2 vertex;

// Contains the r,g,b,Y (luminosity) components.
attribute highp vec4 color;

// The output variable passed to the fragment shader
varying mediump vec4 resultSkyColor;

void main()
{
	gl_Position = projectionMatrix*vec4(vertex, 0., 1.);

	// Must be a separate variable due to Intel drivers
	vec4 tempColor = color;

	///////////////////////////////////////////////////////////////////////////
	// First compute the xy color component
	// color contains the unprojected vertex position in r,g,b
	// + the Y (luminance) component of the color in the alpha channel
	if (tempColor[3]>0.01)
	{
		highp float cosDistSunq = sunPos[0]*tempColor[0] + sunPos[1]*tempColor[1] + sunPos[2]*tempColor[2];
		highp float distSun=acos(cosDistSunq);
		highp float oneOverCosZenithAngle = (tempColor[2]==0.) ? 9999999999999. : 1. / tempColor[2];

		cosDistSunq*=cosDistSunq;
		tempColor[0] = term_x * (1. + Ax * exp(Bx*oneOverCosZenithAngle))* (1. + Cx * exp(Dx*distSun) + Ex * cosDistSunq);
		tempColor[1] = term_y * (1. + Ay * exp(By*oneOverCosZenithAngle))* (1. + Cy * exp(Dy*distSun) + Ey * cosDistSunq);
		if (tempColor[0] < 0. || tempColor[1] < 0.)
		{
			tempColor[0] = 0.25;
			tempColor[1] = 0.25;
		}
	}
	else
	{
		tempColor[0] = 0.25;
		tempColor[1] = 0.25;
	}
	tempColor[2]=tempColor[3];
	tempColor[3]=1.;


	///////////////////////////////////////////////////////////////////////////
	// Now we have the xyY components in color, need to convert to RGB

	// 1. Hue conversion
	// if log10Y>0.6, photopic vision only (with the cones, colors are seen)
	// else scotopic vision if log10Y<-2 (with the rods, no colors, everything blue),
	// else mesopic vision (with rods and cones, transition state)
	if (tempColor[2] <= 0.01)
	{
		// special case for s = 0 (x=0.25, y=0.25)
		tempColor[2] *= 0.5121445;
		tempColor[2] = pow(tempColor[2]*pi*0.0001, alphaWaOverAlphaDa*oneOverGamma)* term2TimesOneOverMaxdLpOneOverGamma;
		tempColor[0] = 0.787077*tempColor[2];
		tempColor[1] = 0.9898434*tempColor[2];
		tempColor[2] *= 1.9256125;
		resultSkyColor = tempColor*brightnessScale;
	}
	else
	{
		if (tempColor[2]<3.9810717055349722)
		{
			// Compute s, ratio between scotopic and photopic vision
			float op = (log(tempColor[2])/ln10 + 2.)/2.6;
			float s = op * op *(3. - 2. * op);
			// Do the blue shift for scotopic vision simulation (night vision) [3]
			// The "night blue" is x,y(0.25, 0.25)
			tempColor[0] = (1. - s) * 0.25 + s * tempColor[0];	// Add scotopic + photopic components
			tempColor[1] = (1. - s) * 0.25 + s * tempColor[1];	// Add scotopic + photopic components
			// Take into account the scotopic luminance approximated by V [3] [4]
			float V = tempColor[2] * (1.33 * (1. + tempColor[1] / tempColor[0] + tempColor[0] * (1. - tempColor[0] - tempColor[1])) - 1.68);
			tempColor[2] = 0.4468 * (1. - s) * V + s * tempColor[2];
		}

		// 2. Adapt the luminance value and scale it to fit in the RGB range [2]
		// tempColor[2] = std::pow(adaptLuminanceScaled(tempColor[2]), oneOverGamma);
		tempColor[2] = pow(tempColor[2]*pi*0.0001, alphaWaOverAlphaDa*oneOverGamma)* term2TimesOneOverMaxdLpOneOverGamma;

		// Convert from xyY to XZY
		// Use a XYZ to Adobe RGB (1998) matrix which uses a D65 reference white
		mediump vec3 tmp = vec3(tempColor[0] * tempColor[2] / tempColor[1], tempColor[2], (1. - tempColor[0] - tempColor[1]) * tempColor[2] / tempColor[1]);
		resultSkyColor = vec4(2.04148*tmp.x-0.564977*tmp.y-0.344713*tmp.z, -0.969258*tmp.x+1.87599*tmp.y+0.0415557*tmp.z, 0.0134455*tmp.x-0.118373*tmp.y+1.01527*tmp.z, 1.);
		resultSkyColor*=brightnessScale;
	}
}
