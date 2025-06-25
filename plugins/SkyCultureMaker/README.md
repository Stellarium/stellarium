# Sky Culture Maker Plugin

## Introduction
Sky Culture Maker is a plugin for Stellarium, designed to simplify the creation and customization of sky cultures. With this tool, users can easily define new sky cultures, draw constellations, and visualize their arrangements directly within Stellarium.
The plugin provides an intuitive interface for both amateur astronomers and advanced users to:
	•	Create and edit sky cultures without extensive manual configuration
	•	Define new constellations by simply drawing them in the sky
	•	Importing fab data
	•	Contribute to educational projects, cultural astronomy preservation, or personal visualization preferences
This documentation provides detailed instructions on how to install, use, and customize the Sky Culture Maker plugin.

## Table of Contents

1. **Installation**
    - Plugin Installation and Prerequisites
    - Activating the Plugin in Stellarium
2. **Getting Started**
    - Overview of the User Interface
    - Creating a New Sky Culture
3. **File Structure and Output**
    - Directory Layout of Generated Sky Cultures
    - File Types and Their Purpose
4. **Best Practices**
    - Recommended Naming Conventions
    - Tips for Accurate Constellation Design
5. **Credits**
6. **License Information**


## 1 Installation

### 1.1 Plugin Installation and prerequisits
TODO package requirements

### 1.2 Activating the Plugin in Stellarium
this can be done like every other plugin in Stellarem. Navigate to settings and the to the plugin selection.  Click SkyCultureMaker and restart the program

## 2 Getting Started
The Sky Culture Maker plugin provides an intuitive way to create sky cultures directly within Stellarium. This section will guide you through the initial steps to get familiar with the interface and start creating your first sky culture.

### Creating a New Sky Culture
Follow these steps to create your first sky culture:

#### Open the Plugin Interface
Ensure the Sky Culture Maker plugin is activated (see Installation section if needed).
Access the plugin via the Stellarium toolbar.

#### Create a New Sky Culture
Click on Creating
The main window will be shown with following tabs:
- Overview: Includes the interface for creating/removing constellations and saving the sky culture. License has to be set here. 
- Description: Enter basic information about the sky culture. Information of the attributes can be found in the [Stellarium guide](https://stellarium.org/files/guide.pdf)
- Boundaries (still in progress)
- Common names (still in progress)
- Geolocation (still in progress)

#### Creating Constellations
Use the pen to create constellation lines by clicking on points and stars in the sky.
Erasing deletes drawn lines from the constellation.
Undo reverts drawn lines.

For each constellation a name and ID has to be set. Further attributes like native name and pronounciation are available.
In addition, an image can be uploaded an anchored to the constellation.

#### Export the Sky Culture
TODO

## 3. File Structure and Output**


SkyCultureMaker/
- CMakeLists.txt, README.md, icons.svg 
- resources/
	- icons
	- SkyCultureMaker.qrc
- src/
	- CMakeLists.txt
	- main classes 
	- gui/
		- dialogue & windows (Scm*.cpp/hpp)
		- UI-definitions(.ui)
	- types/
		- elp and datatypes (e.f. Classification, Lines, StarPoint)

## 4. Best Practices 

###  Recommended Naming Conventions
Follow the naming conventions and standards mentioned in the [Stellarium guide](https://stellarium.org/files/guide.pdf)

###  Tips for Accurate Constellation Design
When a constellation has two separate areas, disable the pen after finishing the first area to unlock the drawer from the most recent star.


## 5 Credits
- Vincent Gerlach (RivinHD)
- Luca-Philipp Grumbach (xLPMG)
- Fabian Hofer (Integer-Ctrl)
- Mher Mnatsakanyan (MherMnatsakanyan03)
- Richard Hofmann (ZeyxRew)

6. **License Information**


## Development Notes

- makeCulturesList():
- https://github.com/Integer-Ctrl/stellarium/blob/master/src/core/StelSkyCultureMgr.cpp#L163
- läd ALLE skycultures in den Stellarium SC Manager
