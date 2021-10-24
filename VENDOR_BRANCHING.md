# Vendor branching : importing external artifacts

*In short: if you import anything from outside Stellarium, do not bluntly copy-paste. The following procedure looks daunting, but that is only appearance.*

## Introduction

Stellarium is constantly benefiting from open source artifacts and enriched with text/data/source files that are **copy-pasted** from places outside of Stellarium.

Sometimes this can be solved by using package managers that automate the importig of external "stuff" (typically code) and allow to fine tune which version is to be imported; Python's `pip` is a great example of this. But package managers do not allow to customise the imported code out of the box (in the `pip` example, one needs to use `-e` to start with).

The basic problem with copy-pasting external artifacts is **code (or data) rot** (and also: "*copy-paste is evil*"). E.g. external artifact changes will not magically appear in Stellarium.

### Examples:

- geonames data (https://www.geonames.org/recent-changes.html), https://github.com/Stellarium/stellarium-data/tags
- quasar data (https://github.com/Stellarium/stellarium/blob/master/plugins/Quasars/util/quasars.tsv)
- Almagest data (minor fixes, of course - this is essentially frozen data)
- HTC algorithms (Helene, Telesto, and Calypso (Lagrangian satellites of Dione) - taken from ftp://ftp.imcce.fr/pub/ephem/satel/htc20/htc20.f ? )
- various libraries (https://github.com/Stellarium/stellarium/tree/master/src/external)
- The SPG4/SDPG4 algorithm (https://en.wikipedia.org/wiki/Simplified_perturbations_models that was updated 2020-03-12) (used in https://github.com/Stellarium/stellarium/blob/e75b00e6c249747c198fe0e2badd77a4adab9415/plugins/Satellites/src/Satellites.hpp#L56-L57)
- and, of course, how could we forget: the various ephemeris algorithms, see https://github.com/Stellarium/stellarium/commits/master/src/core/planetsephems (a good example is `jpleph.cpp`). Some random googling in an attempt to find the originals reveals these (also in use in other software such as Celestia), just to illustrate the issue:
	- https://github.com/Bill-Gray/jpl_eph/blob/master/jpleph.h
	- http://jsoc.stanford.edu/cvs/JSOC/proj/timed/apps/Attic/jpleph.c?hidecvsroot=1&search=None&hideattic=1&sortby=rev&logsort=date&rev=1.1&content-type=text%2Fvnd.viewcvs-markup&diff_format=h
	- https://apollo.astro.amu.edu.pl/PAD/pmwiki.php?n=Dybol.JPLEph 
	- http://celestia.simulatorlabbs.com/CelSL/src/celephem/
- the [gsatellite directory](https://github.com/Stellarium/stellarium/tree/master/plugins/Satellites/src/gsatellite) seems to contain a lot of external code that has been modified locally.
- possibly most of the code stored under `[src/external](https://github.com/Stellarium/stellarium/tree/master/src/external)/`
- although less likely, copy-pasted snippets from Qt examples *could* be candidates ([example](https://github.com/Stellarium/stellarium/blob/2db52c18bc87aaefa00d3d4a280969349634af8f/src/gui/StelGuiItems.cpp#L352))

Potential candidates, other examples

- [SOFA sourcecode](https://www.iausofa.org/) (*Standards Of Fundamental Astronomy*), also mentioned in [Planet.cpp](https://github.com/Stellarium/stellarium/blob/ba80d33d4bc83d72fc15cca53f798cd9439482cf/src/core/modules/Planet.cpp#L1648)
- [meshwarp sample code](http://paulbourke.net/dataformats/meshwarp/) : yes, the sample code.
- [time ephemerides](http://timeephem.sourceforge.net/index.php)

Although some examples abve are unlikely to ever change - or be very ephemeral (sic) - the reasoning is always: prevent rather than cure, and exercise a lot, until it becomes second nature. Just keep Murphy's Law in mind...

### The problem

Notice how it is not always obvious to trace back original file. In the case of ephemerides, these reside in "ancient", core Stellarium files, it is extremely likely that they were copied (and *maybe* recent changes are incorporated in Stellarium). One aspect of the vendor branch procedure details how to include such information so as to help find back the source at all time.

The external version of such artifacts will often continue to evolve, but these modifications obviously will not be magically reflected in our repository, thus *sometimes, if not often* leading to artifacts slowly getting totally outdated (hence the term "code rot"). 

Importing external artifacts therefore requires a specific (and in this case a *simple* as wel as generic) approach: the **Vendor Branch** mechanism. Other approaches exist, such as subtrees and submodules (in case Git is used for the external artifacts), but have inconveniences and are not discussed here; the proposed approach is *simple to apply to novice programmers*.

## The idea
1. Keep external information alive (and updated) on a **separate** branch, called a "**vendor branch**" (eg ``vendor/geonames``). Every dataset/tool lives in its own vendor branch. A vendor branch tracks a *pristine* copy/mirror of the external data.
1. Optionally, the external information is stored in a separate directory (eg ``external/<vendor>/<toolname>``). But the approach works just as fine for individual files.
1. A ``VENDOR`` file explains where the external/original information can be found, so that it can be updated when necessary.
1. **vendor tags** describe which version of the vendor artifacts have been imported (eg ``vendor/geonames/2021-08-21``)
1. The vendor branch is merged to wherever it is needed (normally a feature branch, which will eventually be merged to ``master``)
1. If needed, external information is modified *locally* (usually via a feature/master branch, but *never* in its vendor branch)
1. Ideally, issues in the vendor artifacts should be reported to the vendor, so that updates can then be imported via a *vendor drop*. Alternatively, the developer can solve issues locally, but *never* in the vendor branch.

## Managing vendor artifacts

Basically, two use cases exist: (1) import a new artifact from scratch, or (2) update an artifact because it changed in the remote location. (A third case exists: existing data is to be made vendor-branch compatible.)

## case 1: initialising a vendor branch: importing new artifacts from scratch

1. checkout the feature branch that will import the data in the project
1. if needed (in order to ingest many files), create a directory for the vendor artifacts, e.g. ``external/geonames``
1. create and checkout a vendor branch, e.g. ``vendor/geonames``
1. unzip/copy/import/... the external data. This is called a *vendor drop*.
1. make sure that file/directory *names* do not contain version information as a kind of implicit versioning scheme. rename when needed.
1. commit the vendor data
1. tag the vendor branch, e.g. ``vendor/geonames/1.0``. if the vendor does not provide a clear version number, use the ISO date of the drop : ``vendor/geonames/2021-09-09T1200``
1. switch to the feature/master/whatever local branch
1. merge the vendor branch 
1. EITHER create (add) a ``VENDOR`` file *in* the vendor directory (or a similar name, in the unlikely chance that the file name is already in use) OR edit an existing vendor file to include detailed source location data and, if needed, instructions how to find back the data. Avoid top-level (domain) adresses, try to make life easy for anyone wanting to update the data. Make sure to include the keyword "VENDOR" somewhere so that it can be found if needed.
1. if needed, do whatever is needed to transform the data *in the feature branch*. try to provide a scripted way to transform data, rather than rely on manual operations, so that this can be run again whenever the external data is re-imported.

For prolific vendors (offering many independent tools/data), it might be necessary to create subdirectory and separate branches per product.

## case 2: updating existing vendored artifacts (a "vendor drop")

Now it becomes easy to update external information. Whenever the external information changes:

1. checkout the vendor branch
1. perform a fresh **vendor drop**: 
	1. empty the vendor folder OR delete the vendor file ; you can use this: ``git ls-files -z | xargs -0 rm -f``
	2. replace/explode/unzip/untar/...
1. commit: ``git add -A && git commit``  (see ``git help git-rm`` for details, search for "vendor".)
1. vendor tag the vendor branch
1. merge the updated vendor branch to ``master`` (or via an intermediate feature/bugfix branch, often in order to update local stuff)
1. deal with conflicts when needed. such conflicts are expected to arise when the vendor changed something, and that was also changed locally

This is also needed when the external data disappears: in that case, mention that the external data is no longer available to the public to avoid developers searching for it (or even worse, continue with a copy that still exists elsewhere!! Such a copy does not belong on the vendor branch).

## Notes

### How to deal with module files that are updated externally and still not vendored internally

The basic idea is to create a vendor branch with the new artifact, and do some manual "undo-redo" work.

### Version information in vendor file/directory paths

Sometimes, developers add directories/files with path names containing version information, such as a FAQ file in [this landscape](http://www.alienbasecamp.com/Stellarium/sun.htm). 

This is a bad practice, often done by developers that do not have access to a good VCS.

In such cases, the version information should be removed before committing the vendor drop; this will present the additional advantage that the history of such a file becomes available. This is a delicate step, as the "pristine copy" is no longer fulfilled. If filename information is part of the correct functioning of the imported artifacts (eg it is also referenced in vendor scripts, in linked data, etc), then you are facing a difficult problem. In such a case, it might be better to not rename the vendor file paths at all and accept that file history must be discovered by other means (eg a manual diff).

### Using data from other users

This approach also works for users that want to keep track of other user's configuration data or scripts while at the same time apply local changes.

