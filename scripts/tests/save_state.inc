//
// Name:        Save and restore display state
// License:     Public Domain
// Author:      Matthew Gates
// Description: Provide functions for saving and restoring state (view settings etc.)
//              for use in other scripts via the include feature.  A state is 
//              associated with some string identifier.  This is arbitrary.  You can 
//              have as many states as you like subject to whatever memory 
//              restrictions there are...
//              note - you can pass options to the restoreState function to get it to
//              ignore some parts of the saved state.  See below for details.

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
	savedStates[stateName]["MountMode"] = core.getMountMode();
	savedStates[stateName]["TimeIsNow"] = core.isRealTime();
	savedStates[stateName]["StateTime"] = core.getJDay();
	savedStates[stateName]["StateTimeRate"] = core.getTimeRate();
	savedStates[stateName]["AzimuthAngle"] = core.getViewAzimuthAngle();
	savedStates[stateName]["AltitudeAngle"] = core.getViewAltitudeAngle();
	savedStates[stateName]["RaAngle"] = core.getViewRaAngle();
	savedStates[stateName]["DecAngle"] = core.getViewDecAngle();

	core.debug("saveState() - state saved with ID: " + stateName);
}

// The options parameter is a hash of option values. Supported options:
// "time": "ignore"		do not restore the simulation time and time rate
// "time": "force"		always restore the saved time and time rate
// "time": "smart"		(default) if at saveState, the time was the same as the system
//				time and normal time rate, set this when restore is called.
//				However, if the time rate was not like this, restore
//				the time and time rate as it was when the saveState was called
//
// "view": true 		(default) restore the viewing direction and FOV.  Note that this 
//				will be the alt/azi direction if the saved mount mode is azimuthal,
//				else it will be the equarotial coordinates
// "view": false		Do not restore the viewing direction and FOV.
//
// Examples:
// restoreState("state1"); // use "smart" time behaviour
// restoreState("state2", {"time":"ignore"}); // don't restore the time and time rate
// restoreState("state2", {"time":"force"});  // always restore the time and time rate
function restoreState(stateName, options)
{
	// prevent type error if there are no options...
	if (!options)
	{
		options = {"a":1};
	}

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
	core.setMountMode(savedStates[stateName]["MountMode"]);

	// optional restoration options...
	if (!options["time"])
	{
		options["time"] = "smart";
	}

	if (options["time"] == "smart")
	{
		if (savedStates[stateName]["TimeIsNow"])
		{
			core.setRealTime();
		}
		else
		{
			core.setJDay(savedStates[stateName]["StateTime"]);
			core.setTimeRate(savedStates[stateName]["StateTimeRate"]);
		}
	}
	else if (options["time"] == "force")
	{
		core.setJDay(savedStates[stateName]["StateTime"]);
		core.setTimeRate(savedStates[stateName]["StateTimeRate"]);
	}

	if (options["view"])
	{
		if (savedStates[stateName]["MountMode"] == "azimuthal")
		{
			core.moveToAltAzi(savedStates[stateName]["AltitudeAngle"], savedStates[stateName]["AzimuthAngle"] , 0);
		}
		else
		{
			core.moveToRaDec(savedStates[stateName]["RaAngle"], savedStates[stateName]["DecAngle"] , 0);
		}
	}

	core.debug("restoreState() - state restored with ID: " + stateName);
}


