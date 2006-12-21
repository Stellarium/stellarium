#ifndef CEGUIGUI_H_
#define CEGUIGUI_H_

#include "StelModule.hpp"

class CeguiGui : public StelModule
{
public:
	CeguiGui();
	virtual ~CeguiGui();
	
	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	virtual void init(const InitParser& conf, LoadingBar& lb);
	virtual void update(double deltaTime); // Increment/decrement smoothly the vision field and position
	virtual string getModuleID() const {return "ceguigui";}
	virtual bool isExternal() {return true;}
	virtual double draw(Projector *prj, const Navigator *nav, ToneReproducer *eye);
	virtual bool handleKeys(SDLKey key, SDLMod mod, Uint16 unicode, Uint8 state);
	virtual bool handleMouseMoves(Uint16 x, Uint16 y);
	virtual bool handleMouseClicks(Uint16 x, Uint16 y, Uint8 button, Uint8 state);
};

#endif /*CEGUIGUI_H_*/
