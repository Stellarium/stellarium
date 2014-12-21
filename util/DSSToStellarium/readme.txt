A set of pyton scripts used to process DSS plates for Stellarium

Dependencies:
 - astropy: (sudo pip install astropy)
 - PIL (sudo apt-get install python-imaging)
 - numpy
 
-----------------------------------------------------------
1- Pre-process zipped plates to extract only necessary data
-----------------------------------------------------------
python prepareAllPlates.py

input: a directory with all original plates .tgz files (change in code)
output: a directory "preparedPlates" containing for each plate a single full resolution jpg image + a FITS headers necessary for sky-to-pixel projection
usage: you must change the location of your input DSS plates path directly in the script


-----------------------------------------------------------
2- Generate the survey
-----------------------------------------------------------
python generateFullPlate.py

input: the "preparedPlates" directory created in step 1 must be present
output: a "results" directory containing the survey ready for display in Stellarium


-----------------------------------------------------------
3- Display the survey
-----------------------------------------------------------
Get the Stellarium branch bzr+ssh://bazaar.launchpad.net/~stellarium/stellarium/toastImages
Change the base location of the survey to match the path where the results/ directory is stored. This can be done in ToastMgr.cpp line 29
Compile and run the soft. Remove ground and atmosphere (press G and A), the survey should be visible in the south hemisphere.

