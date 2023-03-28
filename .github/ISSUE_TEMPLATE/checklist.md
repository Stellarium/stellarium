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
- [ ] Actualize the list of planetary nomenclature

### Environment
- [ ] Weak freeze the master branch (accept bug fixes and typo fixes only)
- [ ] Extract all lines for translation
- [ ] Send notification for translators and developers (via maillist)
- [ ] Publish first release candidate (RC1)
- [ ] Assign this issue to the milestone of upcoming release

### Dependencies
- [ ] Actualize CalcMySky/ShowMySky version
- [ ] Actualize QXlsx version
- [ ] Actualize INDI version

## Step 2: one week before release

### Core
- [ ] Actualize the default list of locations
- [ ] Actualize orbital elements for minor bodies of Solar system

### Plugins
- [ ] Actualize the default list of satellites
- [ ] Actualize the default catalog of pulsars
- [ ] Actualize the default catalog of exoplanets

### Environment
- [ ] Actualize the Stellarium User Guide
- [ ] Strong freeze the master branch (accept bug fixes only, which don't touch translatable data)
- [ ] Publish second release candidate (RC2)

## Step 3: immediately before release

### Core
- [ ] Actualize the default list of locations
- [ ] Actualize orbital elements for minor bodies of Solar system
- [ ] Actualize the list of contributors
- [ ] Actualize the list of financial supporters

### Plugins
- [ ] Actualize the default list of satellites
- [ ] Actualize the default catalog of pulsars
- [ ] Actualize the default catalog of exoplanets

### Environment
- [ ] Actualize the ChangeLog file
- [ ] Actualize the link to Stellarium User Guide in README.md file
- [ ] Actualize the version number in CMakeLists.txt file
- [ ] Set `CMAKE_BUILD_TYPE Release` in CMakeLists.txt file
- [ ] Set `STELLARIUM_RELEASE_BUILD 1` in CMakeLists.txt file
- [ ] Actualize metainfo for new release (`util\metainfo\metainfo.sh`)
- [ ] Actualize translations of desktop info (`util\desktop\desktoppo.py`)
- [ ] Actualize translations for landscapes descriptions (`util\landscapes\translate.sh`)
- [ ] Actualize translations for skycultures descriptions (`util\skycultures\translate.sh`)
- [ ] Actualize translations for scenery3d descriptions (`util\scenery3d\translate.sh`)

## Step 4: release

### Environment
- [ ] Add tag on the GitHub
- [ ] Fill the release notes on the GitHub
- [ ] Upload packages on the GitHub
- [ ] Close milestone on the GitHub
- [ ] Remove label `state: published` for all items from closed milestone on the GitHub
               
## Step 5: immediately after release

### Environment
- [ ] Actualize API documentation on the website (`make apidoc`)
- [ ] Actualize release data on the website
- [ ] Actualize translations on the website
- [ ] Add news about new release on the website
- [ ] Actualize the catalog of pulsars on the website
- [ ] Actualize the catalog of exoplanets on the website
- [ ] Set `STELLARIUM_RELEASE_BUILD 0` in CMakeLists.txt file
- [ ] Close this issue
