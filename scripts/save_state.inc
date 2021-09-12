//
// Name:        Save and restore display state
// Version:     0.19.1
// License:     Public Domain
// Authors:     Matthew Gates (matthewg42) (Original author)
//              Alexander Wolf (alex-w)
//              Georg Zotti (gzotti)
//              Brian W. Mulligan (astrobit)
// Description: Provide functions for saving and restoring state (view settings etc.)
//              for use in other scripts via the include feature.  A state is 
//              associated with some string identifier.  This is arbitrary.  You can 
//              have as many states as you like subject to whatever memory 
//              restrictions there are...
//              note - you can pass options to the restoreState function to get it to
//              ignore some parts of the saved state.  See below for details.
// Change Log:
// 2009-05-09   Matthew Gates (matthewg42)
//     * First version
// 2010-04-14   Matthew Gates (matthewg42)
//     * save language
// 2012-02-19   Matthew Gates (matthewg42)
//     * add galactic line states
// 2012-02-20   Matthew Gates (matthewg42)
//     * remove reference to StelSkyImageMgr.getFlagShow and StelSkyImageMgr.setFlagShow
// 2012-07-27   Alexander Wolf (alex-w)
//     * add ecliptic j2000 grid state
// 2014-09-10   Georg Zotti (gzotti)
//    * Update Galactic Plane to Galactic Equator
// 2019-08-14   Brian W. Mulligan (astrobit)
//    * Fix to work under stellarium 0.19
//    * Add change log, version, and author list to header

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
	savedStates[stateName]["SolarSystemFlagLabels"] = SolarSystem.getFlagLabels();
	savedStates[stateName]["SolarSystemLabelsAmount"] = SolarSystem.getLabelsAmount();
	savedStates[stateName]["SolarSystemFlagLightTravelTime"] = SolarSystem.getFlagLightTravelTime();
	savedStates[stateName]["SolarSystemFlagMoonScale"] = SolarSystem.getFlagMoonScale();
	savedStates[stateName]["SolarSystemMoonScale"] = SolarSystem.getMoonScale();
	savedStates[stateName]["MeteorMgrZHR"] = SporadicMeteorMgr.getZHR();
	savedStates[stateName]["MeteorMgrFlagShow"] = SporadicMeteorMgr.getFlagShow();
	savedStates[stateName]["GridLinesMgrFlagAzimuthalGrid"] = GridLinesMgr.getFlagAzimuthalGrid();
	savedStates[stateName]["GridLinesMgrFlagEquatorGrid"] = GridLinesMgr.getFlagEquatorGrid();
	savedStates[stateName]["GridLinesMgrFlagEquatorLine"] = GridLinesMgr.getFlagEquatorLine();
	savedStates[stateName]["GridLinesMgrFlagEclipticLine"] = GridLinesMgr.getFlagEclipticLine();
	savedStates[stateName]["GridLinesMgrFlagMeridianLine"] = GridLinesMgr.getFlagMeridianLine();
	savedStates[stateName]["GridLinesMgrFlagEquatorJ2000Grid"] = GridLinesMgr.getFlagEquatorJ2000Grid();
	savedStates[stateName]["GridLinesMgrFlagEclipticJ2000Grid"] = GridLinesMgr.getFlagEclipticJ2000Grid();
	savedStates[stateName]["GridLinesMgrFlagGalacticGrid"] = GridLinesMgr.getFlagGalacticGrid();
	savedStates[stateName]["GridLinesMgrFlagGalacticEquatorLine"] = GridLinesMgr.getFlagGalacticEquatorLine();
	savedStates[stateName]["ConstellationMgrFlagLines"] = ConstellationMgr.getFlagLines();
	savedStates[stateName]["ConstellationMgrFlagLabels"] = ConstellationMgr.getFlagLabels(); 
	savedStates[stateName]["ConstellationMgrFlagBoundaries"] = ConstellationMgr.getFlagBoundaries(); 
	savedStates[stateName]["ConstellationMgrFlagArt"] = ConstellationMgr.getFlagArt();
	savedStates[stateName]["ConstellationMgrArtIntensity"] = ConstellationMgr.getArtIntensity();
	savedStates[stateName]["ConstellationMgrFlagIsolateSelected"] = ConstellationMgr.getFlagIsolateSelected();
	savedStates[stateName]["StarMgrFlagLabels"] = StarMgr.getFlagLabels(); 
	savedStates[stateName]["StarMgrFlagStars"] = StarMgr.getFlagStars(); 
	savedStates[stateName]["StarMgrLabelsAmount"] = StarMgr.getLabelsAmount(); 
	savedStates[stateName]["StarMgrFlagSciNames"] = StarMgr.getFlagSciNames(); 
	savedStates[stateName]["NebulaMgrFlagHints"] = NebulaMgr.getFlagHints();
	savedStates[stateName]["NebulaMgrFlagShow"] = NebulaMgr.getFlagShow();
//	savedStates[stateName]["NebulaMgrFlagDisplayNoTexture"] = NebulaMgr.getFlagDisplayNoTexture();
	savedStates[stateName]["NebulaMgrLabelsAmount"] = NebulaMgr.getLabelsAmount();
	savedStates[stateName]["NebulaMgrHintsAmount"] = NebulaMgr.getHintsAmount();
	savedStates[stateName]["LandscapeMgrFlagCardinalsPoints"] = LandscapeMgr.getFlagCardinalsPoints();
	savedStates[stateName]["LandscapeMgrFlagLandscape"] = LandscapeMgr.getFlagLandscape();
	savedStates[stateName]["LandscapeMgrFlagAtmosphere"] = LandscapeMgr.getFlagAtmosphere();
	savedStates[stateName]["LandscapeMgrFlagFog"] = LandscapeMgr.getFlagFog();
	savedStates[stateName]["LandscapeMgrCurrentLandscapeID"] = LandscapeMgr.getCurrentLandscapeID();
	//savedStates[stateName]["LandscapeMgrAtmosphereBortleLightPollution"] = LandscapeMgr.getAtmosphereBortleLightPollution();
	savedStates[stateName]["StelSkyDrawerRelativeStarScale"] = StelSkyDrawer.getRelativeStarScale();
	savedStates[stateName]["StelSkyDrawerAbsoluteStarScale"] = StelSkyDrawer.getAbsoluteStarScale();
	savedStates[stateName]["StelSkyDrawerTwinkleAmount"] = StelSkyDrawer.getTwinkleAmount();
	savedStates[stateName]["StelSkyDrawerFlagTwinkle"] = StelSkyDrawer.getFlagTwinkle();
	//savedStates[stateName]["StelSkyDrawerFlagPointStar"] = StelSkyDrawer.getFlagPointStar();
	//savedStates[stateName]["StelSkyDrawerBortleScale"] = StelSkyDrawer.getBortleScale();
	savedStates[stateName]["StelSkyDrawerFlagLuminanceAdaptation"] = StelSkyDrawer.getFlagLuminanceAdaptation();
	savedStates[stateName]["MountMode"] = core.getMountMode();
	savedStates[stateName]["NightMode"] = core.getNightMode();
	savedStates[stateName]["ProjectionMode"] = core.getProjectionMode();
	savedStates[stateName]["SkyCulture"] = core.getSkyCulture();
	savedStates[stateName]["FlagGravityLabels"] = core.getFlagGravityLabels();
	savedStates[stateName]["DiskViewport"] = core.getDiskViewport();
	savedStates[stateName]["ObserverLocation"] = core.getObserverLocation();

	savedStates[stateName]["TimeIsNow"] = core.isRealTime();
	savedStates[stateName]["StateTime"] = core.getJDay();
	savedStates[stateName]["StateTimeRate"] = core.getTimeRate();
	savedStates[stateName]["AzimuthAngle"] = core.getViewAzimuthAngle();
	savedStates[stateName]["AltitudeAngle"] = core.getViewAltitudeAngle();
	savedStates[stateName]["RaAngle"] = core.getViewRaAngle();
	savedStates[stateName]["DecAngle"] = core.getViewDecAngle();

	savedStates[stateName]["AppLanguage"] = core.getAppLanguage();
	savedStates[stateName]["SkyLanguage"] = core.getSkyLanguage();

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
	SolarSystem.setFlagLabels(savedStates[stateName]["SolarSystemFlagLabels"]);
	SolarSystem.setLabelsAmount(savedStates[stateName]["SolarSystemLabelsAmount"]);
	SolarSystem.setFlagLightTravelTime(savedStates[stateName]["SolarSystemFlagLightTravelTime"]);
	SolarSystem.setFlagMoonScale(savedStates[stateName]["SolarSystemFlagMoonScale"]);
	SolarSystem.setMoonScale(savedStates[stateName]["SolarSystemMoonScale"]);
	SporadicMeteorMgr.setZHR(savedStates[stateName]["MeteorMgrZHR"]);
	SporadicMeteorMgr.setFlagShow(savedStates[stateName]["MeteorMgrFlagShow"]);
	GridLinesMgr.setFlagAzimuthalGrid(savedStates[stateName]["GridLinesMgrFlagAzimuthalGrid"]);
	GridLinesMgr.setFlagEquatorGrid(savedStates[stateName]["GridLinesMgrFlagEquatorGrid"]);
	GridLinesMgr.setFlagEquatorLine(savedStates[stateName]["GridLinesMgrFlagEquatorLine"]);
	GridLinesMgr.setFlagEclipticLine(savedStates[stateName]["GridLinesMgrFlagEclipticLine"]);
	GridLinesMgr.setFlagMeridianLine(savedStates[stateName]["GridLinesMgrFlagMeridianLine"]);
	GridLinesMgr.setFlagEquatorJ2000Grid(savedStates[stateName]["GridLinesMgrFlagEquatorJ2000Grid"]);
	GridLinesMgr.setFlagEclipticJ2000Grid(savedStates[stateName]["GridLinesMgrFlagEclipticJ2000Grid"]);
	GridLinesMgr.setFlagGalacticGrid(savedStates[stateName]["GridLinesMgrFlagGalacticGrid"]);
	GridLinesMgr.setFlagGalacticEquatorLine(savedStates[stateName]["GridLinesMgrFlagGalacticEquatorLine"]);
	ConstellationMgr.setFlagLines(savedStates[stateName]["ConstellationMgrFlagLines"]);
	ConstellationMgr.setFlagLabels(savedStates[stateName]["ConstellationMgrFlagLabels"]);
	ConstellationMgr.setFlagBoundaries(savedStates[stateName]["ConstellationMgrFlagBoundaries"]);
	ConstellationMgr.setFlagArt(savedStates[stateName]["ConstellationMgrFlagArt"]);
	ConstellationMgr.setArtIntensity(savedStates[stateName]["ConstellationMgrArtIntensity"]);
	ConstellationMgr.setFlagIsolateSelected(savedStates[stateName]["ConstellationMgrFlagIsolateSelected"]);
	StarMgr.setFlagLabels(savedStates[stateName]["StarMgrFlagLabels"]);
	StarMgr.setFlagStars(savedStates[stateName]["StarMgrFlagStars"]);
	StarMgr.setLabelsAmount(savedStates[stateName]["StarMgrLabelsAmount"]);
	StarMgr.setFlagSciNames(savedStates[stateName]["StarMgrFlagSciNames"]);
	NebulaMgr.setFlagHints(savedStates[stateName]["NebulaMgrFlagHints"]);
	NebulaMgr.setFlagShow(savedStates[stateName]["NebulaMgrFlagShow"]);
//	NebulaMgr.setFlagDisplayNoTexture(savedStates[stateName]["NebulaMgrFlagDisplayNoTexture"]);
	NebulaMgr.setLabelsAmount(savedStates[stateName]["NebulaMgrLabelsAmount"]);
	NebulaMgr.setHintsAmount(savedStates[stateName]["NebulaMgrHintsAmount"]);
	LandscapeMgr.setFlagCardinalsPoints(savedStates[stateName]["LandscapeMgrFlagCardinalsPoints"]);
	LandscapeMgr.setFlagLandscape(savedStates[stateName]["LandscapeMgrFlagLandscape"]);
	LandscapeMgr.setFlagAtmosphere(savedStates[stateName]["LandscapeMgrFlagAtmosphere"]);
	LandscapeMgr.setFlagFog(savedStates[stateName]["LandscapeMgrFlagFog"]);
	LandscapeMgr.setCurrentLandscapeID(savedStates[stateName]["LandscapeMgrCurrentLandscapeID"]);
	//LandscapeMgr.setAtmosphereBortleLightPollution(savedStates[stateName]["LandscapeMgrAtmosphereBortleLightPollution"]);
	StelSkyDrawer.setRelativeStarScale(savedStates[stateName]["StelSkyDrawerRelativeStarScale"]);
	StelSkyDrawer.setAbsoluteStarScale(savedStates[stateName]["StelSkyDrawerAbsoluteStarScale"]);
	StelSkyDrawer.setTwinkleAmount(savedStates[stateName]["StelSkyDrawerTwinkleAmount"]);
	StelSkyDrawer.setFlagTwinkle(savedStates[stateName]["StelSkyDrawerFlagTwinkle"]);
	//StelSkyDrawer.setFlagPointStar(savedStates[stateName]["StelSkyDrawerFlagPointStar"]);
	//StelSkyDrawer.setBortleScale(savedStates[stateName]["StelSkyDrawerBortleScale"]);
	StelSkyDrawer.setFlagLuminanceAdaptation(savedStates[stateName]["StelSkyDrawerFlagLuminanceAdaptation"]);
	core.setMountMode(savedStates[stateName]["MountMode"]);
	core.setNightMode(savedStates[stateName]["NightMode"]);
	core.setProjectionMode(savedStates[stateName]["ProjectionMode"]);
	core.setSkyCulture(savedStates[stateName]["SkyCulture"]);
	core.setFlagGravityLabels(savedStates[stateName]["FlagGravityLabels"]);
	core.setDiskViewport(savedStates[stateName]["DiskViewport"]);
	core.setObserverLocation(savedStates[stateName]["ObserverLocation"]);
	core.setAppLanguage(savedStates[stateName]["AppLanguage"]);
	core.setSkyLanguage(savedStates[stateName]["SkyLanguage"]);

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

