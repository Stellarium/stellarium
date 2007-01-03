#include "CeguiGui.hpp"
#include "SDL_opengl.h"
#include "StelApp.hpp"
#include <CEGUI.h>
#include <renderers/OpenGLGUIRenderer/openglrenderer.h>

CeguiGui::CeguiGui()
{
}

CeguiGui::~CeguiGui()
{
}

#include <CEGUIDefaultResourceProvider.h>
class StellariumResourceProvider : public CEGUI::DefaultResourceProvider
{
	void loadRawDataContainer(const CEGUI::String& filename, CEGUI::RawDataContainer& output, const CEGUI::String& resourceGroup)
	{
		CEGUI::DefaultResourceProvider::loadRawDataContainer(StelApp::getInstance().getDataFilePath(filename.c_str()), output, resourceGroup);
	}
};

void CeguiGui::init(const InitParser& conf, LoadingBar& lb)
{
// 	glEnable(GL_CULL_FACE);
// 	glDisable(GL_FOG);
// 	glClearColor(0.0f,0.0f,0.0f,1.0f);
// 	glViewport(0,0, 800,600);
// 	CEGUI::OpenGLRenderer* renderer = new CEGUI::OpenGLRenderer(0,800,600);
// 	new CEGUI::System(renderer);
// 	
// 	using namespace CEGUI;
// 	try
// 	{
// 		// load in the scheme file, which auto-loads the TaharezLook imageset
// 		CEGUI::SchemeManager::getSingleton().loadScheme("../datafiles/schemes/TaharezLook.scheme");
// 		
// 		// load in a font.  The first font loaded automatically becomes the default font.
// 		CEGUI::FontManager::getSingleton().createFont("../datafiles/fonts/Commonwealth-10.font");
// 
// 		Window* myRoot = WindowManager::getSingleton().loadWindowLayout("test.layout");
// 		System::getSingleton().setGUISheet(myRoot);
// 	}
// 	catch (CEGUI::Exception& e)
// 	{
// 		fprintf(stderr,"CEGUI Exception occured: %s\n", e.getMessage().c_str());
// 	}

	using namespace CEGUI;

	CEGUI::OpenGLRenderer* myRenderer = new CEGUI::OpenGLRenderer(0);
	StellariumResourceProvider* resourceProvider = new StellariumResourceProvider();
	new CEGUI::System(myRenderer, resourceProvider);
	
	CEGUI::SchemeManager::getSingleton().loadScheme("../datafiles/schemes/VanillaSkin.scheme");
	CEGUI::FontManager::getSingleton().createFont("../datafiles/fonts/DejaVuSans-10.font");
	
	Window* root;
	try{
		root = WindowManager::getSingleton().loadWindowLayout("../datafiles/layouts/main.layout");
	}
	catch (CEGUI::Exception e)
	{
		cerr << e.getMessage() << endl;
	}

}

void CeguiGui::update(double deltaTime)
{
	CEGUI::System::getSingleton().injectTimePulse(deltaTime);
}

double CeguiGui::draw(Projector *prj, const Navigator *nav, ToneReproducer *eye)
{
	CEGUI::System::getSingleton().renderGUI();
	return 0.;
}

bool CeguiGui::handleKeys(SDLKey key, SDLMod mod, Uint16 unicode, Uint8 state)
{
	if (state==SDL_KEYDOWN)
		return CEGUI::System::getSingleton().injectKeyDown(key);
	if (state==SDL_KEYUP)
		return CEGUI::System::getSingleton().injectKeyUp(key);
	return false;
}

bool CeguiGui::handleMouseMoves(Uint16 x, Uint16 y)
{
	return CEGUI::System::getSingleton().injectMousePosition(static_cast<float>(x),static_cast<float>(y));
}

bool CeguiGui::handleMouseClicks(Uint16 x, Uint16 y, Uint8 button, Uint8 state)
{
	if (state==SDL_MOUSEBUTTONDOWN)
	{
		switch ( button )
		{
		// handle real mouse buttons
		case SDL_BUTTON_LEFT:
			return CEGUI::System::getSingleton().injectMouseButtonDown(CEGUI::LeftButton);
		case SDL_BUTTON_MIDDLE:
			return CEGUI::System::getSingleton().injectMouseButtonDown(CEGUI::MiddleButton);
		case SDL_BUTTON_RIGHT:
			return CEGUI::System::getSingleton().injectMouseButtonDown(CEGUI::RightButton);
		
		// handle the mouse wheel
		case SDL_BUTTON_WHEELDOWN:
			return CEGUI::System::getSingleton().injectMouseWheelChange( -1 );
		case SDL_BUTTON_WHEELUP:
			return CEGUI::System::getSingleton().injectMouseWheelChange( +1 );
		}
	}
	if (state==SDL_MOUSEBUTTONUP)
	{
		switch ( button )
		{
		case SDL_BUTTON_LEFT:
			return CEGUI::System::getSingleton().injectMouseButtonUp(CEGUI::LeftButton);
		case SDL_BUTTON_MIDDLE:
			return CEGUI::System::getSingleton().injectMouseButtonUp(CEGUI::MiddleButton);
		case SDL_BUTTON_RIGHT:
			return CEGUI::System::getSingleton().injectMouseButtonUp(CEGUI::RightButton);
		}
	}
	return false;
}
