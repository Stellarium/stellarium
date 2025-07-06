# Sky Culture Maker Plugin

## Introduction

*Sky Culture Maker* is a plugin for **Stellarium**, designed to simplify the creation and customization of sky cultures. With this tool, users can easily define new sky cultures, draw constellations, and visualize their arrangements directly within Stellarium.

The plugin provides an intuitive interface for both amateur astronomers and advanced users to:
- Create sky cultures directly in Stellarium with real-time visualization
- Define new constellations by drawing directly in the Stellarium sky view  
- Convert `.fab` data into JSON
- Contribute to educational projects, preserve cultural astronomical knowledge, or create personalized star maps  

This documentation explains how to activate and use the Sky Culture Maker plugin, as well as its functionalities.

---

## Table of Contents

1. **Installation**  
    - Plugin Installation and Prerequisites  
    - Activating the Plugin in Stellarium  
2. **Getting Started**  
    - Overview of the User Interface  
    - Creating a New Sky Culture  
    - Drawing Constellations  
    - Exporting Your Sky Culture  
3. **File Structure and Output**  
    - Directory Layout of Generated Sky Cultures  
    - File Types and Their Purpose  
4. **Best Practices**  
    - Recommended Naming Conventions  
    - Tips for Accurate Constellation Design  
5. **Future Roadmap**
6. **Credits**  
7. **License Information**  

<br>

---

## 1. Installation

### 1.1 Plugin Installation and Prerequisites

To use Sky Culture Maker, ensure the following dependencies are installed:

**For converting**  
- [Tidy HTML](https://www.html-tidy.org/) (used for formatting HTML descriptions)  

### 1.2 Activating the Plugin in Stellarium

After building or installing the plugin:

1. Launch Stellarium.  
2. Navigate to **Settings → Plugins**.  
3. Locate **Sky Culture Maker** in the plugin list.  
4. Enable the plugin and restart Stellarium for changes to take effect.  

The Sky Culture Maker icon should now appear in the Stellarium toolbar. 

<br>

<img src="./resources/bt_SCM_On.png" alt="drawing" width="100"/>

---

## 2. Getting Started

Sky Culture Maker provides a seamless workflow to create custom sky cultures directly within Stellarium. This section will guide you through the initial steps to get familiar with the interface and start creating your first sky culture.

### Overview of the User Interface

Once the plugin is activated, you can access it through the Stellarium toolbar. By clicking on **Create**, the main window will open, providing several tabs, each dedicated to different aspects of sky culture creation:

- **Overview:** Manage constellations, remove existing ones, and save your sky culture. The license must be specified here.  
- **Description:** Enter essential metadata like the sky culture's name, description, and author. See the [Stellarium Guide](https://stellarium.org/files/guide.pdf) for details on recommended attributes.  
- **Boundaries:** (Work in progress) Define boundaries for constellations or cultural sky regions.  
- **Common Names:** (Work in progress) Specify alternative names or traditional star names.  
- **Geolocation:** (Work in progress) Assign geographic or cultural origin information.  

---

### Creating a New Sky Culture

1. Open the Sky Culture Maker interface.  
2. Click **Create New Sky Culture**.  
3. Fill in the required fields under the **Description** tab (name, author, etc.).  
4. Set the license in the **Overview** tab.  

Once the basic information is complete, you can begin drawing constellations.

---

### Drawing Constellations

The constellation editor provides tools for interactive design:

- **Pen Tool:** Right click on stars to draw constellation lines between points.  
- **Erase Tool:** Remove lines or incorrect segments from the constellation.  
- **Undo:** Revert the most recent drawing action.  

For each constellation, the following information must be provided:

- **ID:** Unique identifier for the constellation  
- **Name:** Display name for the constellation  
- **Native Name (optional):** Name in the original language or script  
- **Pronunciation (optional):** Phonetic pronunciation guide  
- **Constellation Image (optional):** Upload an image to be anchored to the constellation  

If your constellation consists of multiple separated areas, disable the pen tool after completing one area to unlock the drawing cursor from the most recent star.

---

#### Exporting the Sky Culture

Once your sky culture is saved, it can be exported as a JSON package. The exported files are stored in Stellarium's `skycultures` directory.

The `description.md` file contains all general information provided in the *Overview* tab.  
All created constellations, along with their properties, are exported into the `index.json` file.  

These files allow your sky culture to be loaded and displayed within Stellarium or shared with others.

---

#### Converting a Sky Culture

To convert a sky culture:

1. Click on **Convert** in the main window.
2. Select an archive file containing the sky culture files.  
    **Supported formats:** `.zip`, `.rar`, `.tar`, `.7z`  
    > ⚠️ **Note:** Only regular `.tar` archives are supported.  
    > Compressed formats like `.tar.gz` or `.tar.bz2` are **not** supported.

3. The converted files will be saved in Stellarium's sky culture folder.

> **Case Sensitivity:**  
> The constellation names are **case-sensitive**.  
> For example, `chinese_chenzhuo` is **not** the same as `chinese_Chenzhuo`.

> <span style="color:#b22222"><strong>Important:</strong></span>  
> The conversion process **requires Qt6 or later**.  
> If you are using an older version of Qt, the converter will **not** be built and the conversion feature will be **disabled**.

---

## 3. File Structure and Output

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
		- datatypes (e.f. Classification, Lines, StarPoint)



Generated sky culture files follow the Stellarium standards for directories, metadata, and constellation definitions.

---

## 4. Best Practices

### Recommended Naming Conventions

- Follow the standards outlined in the [Stellarium Guide](https://stellarium.org/files/guide.pdf)  
- Use consistent and culturally appropriate names for constellations  
- Ensure unique IDs for each constellation  

### Tips for Accurate Constellation Design

- When drawing constellations with separate, disconnected regions, disable the pen tool after completing one area to prevent lines from unintentionally connecting distant stars.  
- Use high-contrast colors for constellation lines to ensure visibility in Stellarium's sky view.  
- Anchor images carefully to avoid visual misalignment during zooming or panning.  
- Zooming in on the sky allows for more precise use of the pen tool. The star selection becomes less sensitive to nearby stars, making it easier to accurately connect the intended points.

---

## 5 Credits
- Vincent Gerlach (RivinHD)
- Luca-Philipp Grumbach (xLPMG)
- Fabian Hofer (Integer-Ctrl)
- Mher Mnatsakanyan (MherMnatsakanyan03)
- Richard Hofmann (ZeyxRew)

---

## 6. **License Information**
Copyright (C) 2025 The Sky Culture Maker Contributors

This plugin is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This plugin is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this plugin; if not, see https://www.gnu.org/licenses/.