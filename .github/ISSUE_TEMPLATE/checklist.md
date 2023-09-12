---
name: Checklist for release
about: Creating a checklist for release to help us improve quality of software

---

<!--
READ THIS AND FILL IN THIS TEMPLATE!

This is checklist for maintainer of Stellarium (a release master)

-->

## Step 1: two weeks before release
### Core
- [ ] Update the list of planetary nomenclature

### Environment
- [ ] Weak freeze the master branch (accept bug fixes and typo fixes only)
- [ ] Extract all lines for translation
- [ ] Send notification for translators and developers (via maillist)
- [ ] Publish first release candidate (RC1)
- [ ] Assign this issue to the milestone of upcoming release

### Dependencies
- [ ] Update CalcMySky/ShowMySky version
- [ ] Update QXlsx version
- [ ] Update INDI version

## Step 2: one week before release

### Core
- [ ] Update the default list of locations
- [ ] Update orbital elements for minor bodies of Solar system

### Plugins
- [ ] Update the default list of satellites
- [ ] Update the default catalog of pulsars
- [ ] Update the default catalog of exoplanets
- [ ] Update the default list of comets (`data\ssystem_minor.ini`)

### Environment
- [ ] Update the Stellarium User Guide
- [ ] Strong freeze the master branch (accept bug fixes only, which don't touch translatable data)
- [ ] Publish second release candidate (RC2)

## Step 3: immediately before release

### Core
- [ ] Update the default list of locations
- [ ] Update orbital elements for minor bodies of Solar system
- [ ] Update the list of contributors
- [ ] Update the list of financial supporters

### Plugins
- [ ] Update the default list of satellites
- [ ] Update the default catalog of pulsars
- [ ] Update the default catalog of exoplanets
- [ ] Update the discovery circumstances for minor planets (Solar System Editor)
- [ ] Update the discovery circumstances for comets (Solar System Editor)

### Environment
- [ ] Update the ChangeLog file
- [ ] Update the link to Stellarium User Guide in README.md file
- [ ] Update the version number in CMakeLists.txt file
- [ ] Set `CMAKE_BUILD_TYPE Release` in CMakeLists.txt file
- [ ] Set `STELLARIUM_RELEASE_BUILD 1` in CMakeLists.txt file
- [ ] Update metainfo for new release (`util\metainfo\metainfo.sh`)
- [ ] Update translations of desktop info (`util\desktop\desktoppo.py`)
- [ ] Update translations for landscapes descriptions (`util\landscapes\translate.sh`)
- [ ] Update translations for skycultures descriptions (`util\skycultures\translate.sh`)
- [ ] Update translations for scenery3d descriptions (`util\scenery3d\translate.sh`)

## Step 4: release

### Environment
- [ ] Add tag on the GitHub
- [ ] Fill the release notes on the GitHub
- [ ] Upload packages on the GitHub
- [ ] Close milestone on the GitHub
- [ ] Remove label `state: published` for all items from closed milestone on the GitHub
               
## Step 5: immediately after release

### Environment
- [ ] Update API documentation on the website (`make apidoc`)
- [ ] Update release data on the website
- [ ] Update translations on the website
- [ ] Add news about new release on the website
- [ ] Update the catalog of pulsars on the website
- [ ] Update the catalog of exoplanets on the website
- [ ] Set `STELLARIUM_RELEASE_BUILD 0` in CMakeLists.txt file
- [ ] Close this issue
