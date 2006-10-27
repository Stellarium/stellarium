#ifndef GEODESICGRIDDRAWER_H_
#define GEODESICGRIDDRAWER_H_

#include "geodesic_grid.h"
#include "stelmodule.h"

class SFont;

class GeodesicGridDrawer : public StelModule
{
public:
	GeodesicGridDrawer(int level);
	virtual ~GeodesicGridDrawer();

	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	virtual void init(const InitParser& conf, LoadingBar& lb);
	virtual string getModuleID() const {return "geodesic_grid_drawer";}
	virtual double draw(Projector *prj, const Navigator *nav, ToneReproductor *eye);
	virtual void update(double deltaTime) {;}
	virtual void updateI18n() {;}
	virtual void updateSkyCulture(LoadingBar& lb) {;}
	
private:
	GeodesicGrid* grid;
	GeodesicSearchResult* searchResult;
	SFont* font;
};

#endif /*GEODESICGRIDDRAWER_H_*/
