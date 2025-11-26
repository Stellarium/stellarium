# Sky Culture Maker Plugin

## Introduction

*Sky Culture Maker* is a plugin for **Stellarium**, designed to simplify the creation and customization of sky cultures. With this tool, users can easily define new sky cultures, draw constellations, and visualize their arrangements directly within Stellarium.

The plugin provides an intuitive interface for both amateur astronomers and advanced users to:
- Create sky cultures directly in Stellarium with real-time visualization
- Define new constellations by drawing directly in the Stellarium sky view  
- Convert `.fab` data into JSON

This documentation explains how to activate and use the Sky Culture Maker plugin, as well as its functionalities.

---

## Table of Contents

1. **Installation**  
    - Prerequisites  
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

The Sky Culture Maker plugin is embedded in the Stellarium project. Therefore, no additional installation is necessary.
 
### 1.1 Prerequisites

The Sky Culture Maker currently consists of two main features. The first is the Maker, which is used to create new sky cultures within Stellarium. The second is a converter that transforms old `.fab` sky culture files into the new JSON format.

**Maker**

- Stellarium already comes with everything you need.

**Converter**

- [Tidy HTML](https://www.html-tidy.org/) (used for formatting HTML descriptions)
- [gettext](https://www.gnu.org/software/gettext/)
- [Zlib](https://www.zlib.net)

### 1.2 Activating the Plugin in Stellarium

After building or installing Stellarium:

1. Launch Stellarium.  
2. Navigate to **Settings → Plugins**.  
3. Locate **Sky Culture Maker** in the plugin list.  
4. Enable the plugin and restart Stellarium for changes to take effect.  

The Sky Culture Maker icon should now appear in the Stellarium toolbar. 

<br>

<img src="./resources/bt_SCM_On.png" alt="drawing" width="100"/>

---

## 2. Getting Started

Sky Culture Maker provides a seamless workflow to create custom sky cultures directly within Stellarium. This section will guide you through the initial steps to get familiar with the interface and start creating your first sky culture and how to use the converter.

After activating the plugin, you can start it via the Stellarium toolbar. After starting the Sky Culture Maker, the Start dialog window will appear. There, you will find the options **Create**, **Convert**, and **Cancel**. Let's look at **Create** first.

### Maker

Clicking **Create** will start the Maker and open the three windows.
1. The _Location window_ so that the user can navigate to the correct location where to create the sky culture
2. The _Date/time window_ so that the user can choose the correct date and time of the sky
3. The main window of the maker, which provides several tabs dedicated to different aspects of sky culture creation.

- **Overview:** Manage constellations, remove existing ones, and save your sky culture. The sky culture name and license must be specified here.  
- **Description**: Enter essential information such as authors, cultural descriptions, and constellations. Refer to the Stellarium Guide for details on the description fields.  **Important**: All fields must be filled out, and a classification must be selected from the dropdown menu at the bottom of the description tab.
- **Boundaries:** (Planned for the future) Define boundaries for constellations or cultural sky regions.  
- **Common Names:** (Planned for the future) Specify alternative names or traditional star names.  
- **Geolocation:** (Planned for the future) Assign geographic or cultural origin information.  

---

#### Creating a New Sky Culture

1. Open the Sky Culture Maker interface.  
2. Click **Create New Sky Culture**.  
3. Fill in the required fields under the **Description** tab (name, author, etc.).  
4. Set the license in the **Overview** tab.  

Once the basic information is complete, you can begin drawing constellations.

---

#### Drawing Constellations

The constellation editor provides tools for interactive design:

- **Pen Tool:** Right click on stars to draw constellation lines between points. You can also use the search function to locate specific stars by name. Once found, those stars can be selected directly to create lines between them.
- **Erase Tool:** Remove lines or incorrect segments from the constellation.  
- **Undo:** Revert the most recent drawing action.  

For each constellation, the following information must be provided:

- **ID:** Unique identifier for the constellation . If not set it will automatically be generated based on the **Name**
- **Name:** Display name for the constellation  
- **Native Name (optional):** Name in the original language or script  
- **Pronunciation (optional):** Phonetic pronunciation guide  
- **Constellation Image (optional):** Upload an image to be anchored to the constellation  

If your constellation consists of multiple separated areas, disable the pen tool after completing one area to unlock the drawing cursor from the most recent star.


#### Uploading and Anchoring an Image

You can upload a constellation image and align it with specific stars in the sky. Follow these steps to add and anchor your artwork:

1. Open the **Artwork** tab within the Constellation Editor.  
2. Click on **Upload Image** and select the desired constellation image from your files.  
3. After uploading, three anchor points will appear at the center of the image. You can use these points to align the image precisely with the stars.  

To bind an anchor to a star:

- Click on one of the anchor points. The selected anchor will turn green to indicate it is active.  
- Select a star of your choice in the sky.  
- Click **Bind Star** to link the anchor to the selected star.  

When an anchor is successfully bound to a star, it will be highlighted in a brighter color. Selecting the anchor again will automatically highlight the corresponding star.

**Note:**  
If an anchor is already bound to a star, clicking **Bind Star** again will overwrite the existing binding with the currently selected star.


---

#### Exporting the Sky Culture

Once your sky culture is saved, it can be exported as a JSON package. The exported files are stored in Stellarium's `skycultures` directory.

The `description.md` file contains all general information provided in the *Overview* tab.  
All created constellations, along with their properties, are exported into the `index.json` file.  

These files allow your sky culture to be loaded and displayed within Stellarium or shared with others.

---

### Converting a Sky Culture (Converter)

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

After creating or converting a sky culture, the files are stored in Stellarium's `skycultures` directory by default.

If the user does not have permission to write to Stellarium's directory, a dialog will prompt the user to select an alternative existing folder where the sky culture can be saved.

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
- To avoid the pen tool snapping to nearby stars while drawing, press and hold **CTRL**. This allows for free placement of lines without automatic star locking.


---

## 5 Credits
- Vincent Gerlach ([RivinHD@GitHub](https://github.com/RivinHD))
- Luca-Philipp Grumbach ([xLPMG@GitHub](https://github.com/xLPMG))
- Fabian Hofer ([Integer-Ctrl@GitHub](https://github.com/Integer-Ctrl))
- Mher Mnatsakanyan ([MherMnatsakanyan03@GitHub](https://github.com/MherMnatsakanyan03))
- Richard Hofmann ([ZeyxRew@GitHub](https://github.com/ZeyxRew))

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