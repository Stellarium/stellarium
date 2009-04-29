//
// Name:        Save and restore display state
// License:     Public Domain
// Author:      Matthew Gates
// Description: Provide functions for saving and restoring state (view settings etc.)
//              for use in other scripts via the include feature.  A state is 
//              associated with some string identifier.  This is arbitrary.  You can 
//              have as many states as you like subject to whatever memory 
//              restrictions there are...
//

// declare a global variable to store saved state
var savedStates = new Array();

// call this function with some string ID
function saveState(stateName)
{
	savedStates[stateName] = new Array();
	savedStates[stateName]["SolarSystemFlagPlanets"] = SolarSystem.getFlagPlanets();
	savedStates[stateName]["SolarSystemFlagHints"] = SolarSystem.getFlagHints();
	savedStates[stateName]["SolarSystemFlagOrbits"] = SolarSystem.getFlagOrbits();
	savedStates[stateName]["SolarSystemFlagMoonScale"] = SolarSystem.getFlagMoonScale();
	savedStates[stateName]["SolarSystemFlagTrails"] = SolarSystem.getFlagTrails();
	savedStates[stateName]["MeteorMgrZHR"] = MeteorMgr.getZHR();
	savedStates[stateName]["GridLinesMgrFlagAzimuthalGrid"] = GridLinesMgr.getFlagAzimuthalGrid();
	savedStates[stateName]["GridLinesMgrFlagEquatorGrid"] = GridLinesMgr.getFlagEquatorGrid();
	savedStates[stateName]["GridLinesMgrFlagEquatorLine"] = GridLinesMgr.getFlagEquatorLine();
	savedStates[stateName]["GridLinesMgrFlagEclipticLine"] = GridLinesMgr.getFlagEclipticLine();
	savedStates[stateName]["GridLinesMgrFlagMeridianLine"] = GridLinesMgr.getFlagMeridianLine();
	savedStates[stateName]["GridLinesMgrFlagEquatorJ2000Grid"] = GridLinesMgr.getFlagEquatorJ2000Grid();
	savedStates[stateName]["ConstellationMgrFlagLines"] = ConstellationMgr.getFlagLines();
	savedStates[stateName]["ConstellationMgrFlagLabels"] = ConstellationMgr.getFlagLabels(); 
	savedStates[stateName]["ConstellationMgrFlagBoundaries"] = ConstellationMgr.getFlagBoundaries(); 
	savedStates[stateName]["ConstellationMgrFlagArt"] = ConstellationMgr.getFlagArt();
	savedStates[stateName]["StarMgrFlagLabels"] = StarMgr.getFlagLabels(); 
	savedStates[stateName]["SolarSystemFlagLabels"] = SolarSystem.getFlagLabels();
	savedStates[stateName]["NebulaMgrFlagHints"] = NebulaMgr.getFlagHints();
	savedStates[stateName]["LandscapeMgrFlagCardinalsPoints"] = LandscapeMgr.getFlagCardinalsPoints();
	savedStates[stateName]["LandscapeMgrFlagLandscape"] = LandscapeMgr.getFlagLandscape();
	savedStates[stateName]["LandscapeMgrFlagAtmosphere"] = LandscapeMgr.getFlagAtmosphere();
	savedStates[stateName]["LandscapeMgrFlagFog"] = LandscapeMgr.getFlagFog();

	core.debug("saveState() - state saved with ID: " + stateName);
}

function restoreState(stateName)
{
	if (!savedStates[stateName])
	{
		core.debug("restoreState() - WARNING no such saved state named: " + stateName);
		return;
	}

	SolarSystem.setFlagPlanets(savedStates[stateName]["SolarSystemFlagPlanets"]);
	SolarSystem.setFlagHints(savedStates[stateName]["SolarSystemFlagHints"]);
	SolarSystem.setFlagOrbits(savedStates[stateName]["SolarSystemFlagOrbits"]);
	SolarSystem.setFlagMoonScale(savedStates[stateName]["SolarSystemFlagMoonScale"]);
	SolarSystem.setFlagTrails(savedStates[stateName]["SolarSystemFlagTrails"]);
	MeteorMgr.setZHR(savedStates[stateName]["MeteorMgrZHR"]);
	GridLinesMgr.setFlagAzimuthalGrid(savedStates[stateName]["GridLinesMgrFlagAzimuthalGrid"]);
	GridLinesMgr.setFlagEquatorGrid(savedStates[stateName]["GridLinesMgrFlagEquatorGrid"]);
	GridLinesMgr.setFlagEquatorLine(savedStates[stateName]["GridLinesMgrFlagEquatorLine"]);
	GridLinesMgr.setFlagEclipticLine(savedStates[stateName]["GridLinesMgrFlagEclipticLine"]);
	GridLinesMgr.setFlagMeridianLine(savedStates[stateName]["GridLinesMgrFlagMeridianLine"]);
	GridLinesMgr.setFlagEquatorJ2000Grid(savedStates[stateName]["GridLinesMgrFlagEquatorJ2000Grid"]);
	ConstellationMgr.setFlagLines(savedStates[stateName]["ConstellationMgrFlagLines"]);
	ConstellationMgr.setFlagLabels(savedStates[stateName]["ConstellationMgrFlagLabels"]);
	ConstellationMgr.setFlagBoundaries(savedStates[stateName]["ConstellationMgrFlagBoundaries"]);
	ConstellationMgr.setFlagArt(savedStates[stateName]["ConstellationMgrFlagArt"]);
	StarMgr.setFlagLabels(savedStates[stateName]["StarMgrFlagLabels"]);
	SolarSystem.setFlagLabels(savedStates[stateName]["SolarSystemFlagLabels"]);
	NebulaMgr.setFlagHints(savedStates[stateName]["NebulaMgrFlagHints"]);
	LandscapeMgr.setFlagCardinalsPoints(savedStates[stateName]["LandscapeMgrFlagCardinalsPoints"]);
	LandscapeMgr.setFlagLandscape(savedStates[stateName]["LandscapeMgrFlagLandscape"]);
	LandscapeMgr.setFlagAtmosphere(savedStates[stateName]["LandscapeMgrFlagAtmosphere"]);
	LandscapeMgr.setFlagFog(savedStates[stateName]["LandscapeMgrFlagFog"]);

	core.debug("restoreState() - state restored with ID: " + stateName);
}


