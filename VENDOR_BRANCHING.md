# Vendor branching : importing external artifacts

*In short: if you import anything from outside Stellarium, do not bluntly copy-paste. The following procedure looks daunting, but that is only appearance.*

## Introduction

Stellarium is constantly benefiting from open source artifacts and being enriched with text/data/source files that are **copy-pasted** from places outside of Stellarium.

The basic problem with the copy-pasting of external artifacts is **code (or data) rot** (and also: "*copy-paste is evil*"). More in detail, **external changes** will not magically appear in Stellarium.

Sometimes this can be solved by using package managers that automate the importing of external "stuff" (typically code) and allow to fine tune which version is to be imported; Python's `pip -e` is a great example of this. But package managers do not allow to modify the imported code out of the box *and* benefit from external updates.

Sometimes this problenm is thensolved by manual labor: porting the external changes in Stellarium, a laborous approach prone to bugs.

### Examples:

Here are some existing artifacts that have been copy-pasted in Stellarium over the years:

- geonames data ([external changes](https://www.geonames.org/recent-changes.html)), https://github.com/Stellarium/stellarium-data/tags
- [quasar data](https://github.com/Stellarium/stellarium/blob/master/plugins/Quasars/util/quasars.tsv)
- Almagest data (minor fixes, of course - this is essentially frozen data)
- HTC algorithms (Helene, Telesto, and Calypso (Lagrangian satellites of Dione) - taken from ftp://ftp.imcce.fr/pub/ephem/satel/htc20/htc20.f ? )
- various libraries under [src/external](https://github.com/Stellarium/stellarium/tree/master/src/external):
	- the [gsatellite directory](https://github.com/Stellarium/stellarium/tree/master/plugins/Satellites/src/gsatellite) seems to contain a lot of external code that has been modified locally.
- The SPG4/SDPG4 algorithm (see also [WP](https://en.wikipedia.org/wiki/Simplified_perturbations_models) updated 2020-03-12) (used in the [satellite plugin](https://github.com/Stellarium/stellarium/blob/e75b00e6c249747c198fe0e2badd77a4adab9415/plugins/Satellites/src/Satellites.hpp#L56-L57) )
- and, of course, how could we forget: the [various ephemeris algorithms](https://github.com/Stellarium/stellarium/commits/master/src/core/planetsephems) (a good example is `jpleph.cpp`). Their true soource is [JPL](https://ssd.jpl.nasa.gov/planets/eph_export.html) Some random googling shows that the problem exists elsewhere too (e.g. Celestia):
	- https://github.com/Bill-Gray/jpl_eph/blob/master/jpleph.h
	- [Stanford JSOC](http://jsoc.stanford.edu/cvs/JSOC/proj/timed/apps/Attic/jpleph.c?hidecvsroot=1&search=None&hideattic=1&sortby=rev&logsort=date&rev=1.1&content-type=text%2Fvnd.viewcvs-markup&diff_format=h)
	- https://apollo.astro.amu.edu.pl/PAD/pmwiki.php?n=Dybol.JPLEph 
	- [Celestia](http://celestia.simulatorlabbs.com/CelSL/src/celephem/)
- [SOFA sourcecode](https://www.iausofa.org/) (*Standards Of Fundamental Astronomy*), also mentioned in [Planet.cpp](https://github.com/Stellarium/stellarium/blob/ba80d33d4bc83d72fc15cca53f798cd9439482cf/src/core/modules/Planet.cpp#L1648): see the [recent changes](https://www.iausofa.org/current.html) (especially the [C library](https://www.iausofa.org/current_C.html), and the [archive](https://www.iausofa.org/archive.html))

Potential candidates, other examples

- [meshwarp sample code](http://paulbourke.net/dataformats/meshwarp/) : yes, the sample code.
- [time ephemerides](http://timeephem.sourceforge.net/index.php)
- although less likely, copy-pasted snippets from Qt examples *could* be candidates ([example](https://github.com/Stellarium/stellarium/blob/2db52c18bc87aaefa00d3d4a280969349634af8f/src/gui/StelGuiItems.cpp#L352))

Although some examples above are unlikely to ever change - or be very ephemeral (sic) - the reasoning is always: prevent rather than cure, and exercise a lot, until it becomes second nature. Just keep Murphy's Law in mind...

### The problem

Notice how it is not always obvious to trace back the original files. In the case of ephemeride routines, these reside in "ancient", core Stellarium files, it is extremely likely that they were copied (and *maybe* recent changes are incorporated in Stellarium). One aspect of the vendor branch procedure details how to include such meta information so as to help find back the source at all times.

The external version of such artifacts will often continue to evolve, but these modifications obviously will not be magically reflected in our repository, thus *sometimes, if not often* leading to artifacts slowly getting totally outdated (hence the term "code rot"). 

Importing external artifacts therefore requires a specific (and in this case a *simple* as well as generic) approach: the **Vendor Branch** mechanism. Other approaches exist, such as subtrees and submodules (in case Git is used for the external artifacts), but have inconveniences and are not discussed here; the proposed approach is *simple to apply to novice programmers*. The approach is valid in any VCS, such as Git, Mercurial, ClearCase, TFS, cvs, you name it.

Importing external artefacts in vendor branches will also provide insight in what changed in those artefacts, just by inspecting the "diff" between successive vendor versions. Normally such changes should also be announced the vendor.

## The solution
1. Keep external information alive (and updated) on a **separate** branch, called a "**vendor branch**" (e.g. ``vendor/geonames``). Every copied dataset/tool/sourcecode lives in its own vendor branch. A vendor branch tracks a *pristine* copy/mirror of the external data.
1. Optionally, the external information is stored in a separate directory (e.g. ``external/<vendor>/<toolname>``). But the approach works just as fine for individual files.
1. A ``VENDOR`` file explains where the external/original information can be found, so that it can be updated when necessary.
1. **vendor tags** describe which version of the vendor artifacts has been imported (e.g. ``vendor/geonames/2021-08-21``)
1. The vendor branch is merged to wherever it is needed (normally a feature branch, which will eventually be merged to ``master``)
1. If needed (which is often the case), external information is modified *locally* (usually via a feature/master branch, but *never* in its vendor branch)
1. Ideally, issues in the vendor artifacts should be reported to the vendor, so that updates can then be imported via a fresh *vendor drop*. Alternatively, a developer can solve issues locally, but *never* in the vendor branch.

## Managing vendor artifacts

Basically, two use cases exist: 

- (1) import a new artifact from scratch, or 
- (2) update an artifact because it changed in the remote location. 

(A third case exists: existing data is to be made vendor-branch compatible.)

## case 1 - initialising a vendor branch: importing new artifacts from scratch

1. checkout the feature branch that will import the data in the project
1. if needed (in order to ingest many files), create a directory for the vendor artifacts, e.g. ``external/geonames``
1. create and checkout a vendor branch, e.g. ``vendor/geonames``
1. unzip/copy/import/... the external data. This is called a *vendor drop*.
1. Make sure that file/directory *names* do not contain version information as a kind of implicit versioning scheme. Rename when needed.
1. Commit the vendor data
1. Tag the vendor branch, e.g. ``vendor/geonames/1.0``. If the vendor does not provide a clear version number, use the UTC date/time of the drop, formatted as ISO: ``vendor/geonames/2021-09-09T1200``
1. Switch to the feature/master/whatever local branch
1. Merge the vendor branch 
1. Create relevant metadata that helps finding back the source: detailed source location data, and if needed, instructions how to find back the data. Avoid top-level (domain) adresses, try to make life easy for anyone wanting to update the data. Make sure to include the keyword "VENDOR" somewhere so that it can be found if needed. 
EITHER 
	- create (add) a ``VENDOR`` file *in* the vendor directory (or a similar name, in the unlikely chance that the file name is already in use), OR 
	- edit an existing vendor file (e.g. when only 1 file has been copied, rather than a directory)
1. If needed, do whatever is needed to transform the data *in the feature branch*. Try to provide a scripted way to transform data, rather than rely on manual operations, so that this can be run again whenever the external data is re-imported.

For prolific vendors (offering many independent tools/data, typically offered as separate packages), it might be necessary to create subdirectory and separate (sub)branches per product: `vendor/<vendor name>/<vendor product>`.

## case 2 - updating existing vendored artifacts (a "vendor drop")

Now it becomes easy to update external information. Whenever the external information changes:

1. Checkout the vendor branch
1. Perform a fresh **vendor drop**: 
	1. empty the vendor folder OR delete the vendor file

		``git ls-files -z | xargs -0 rm -f``
	2. replace/explode/unzip/untar/...
1. Commit: 

	``git add -A && git commit``  
	(see ``git help git-rm`` for details, search for "vendor".)

1. Tag the vendor branch with a vendor tag (`vendor/<vendor name>/<vendor product>/<tag>`) where `<tag>` is either a tag available from the vendor, or otherwise the ISO date/time of the vendor drop.
1. Merge the updated vendor branch to ``master`` (or via an intermediate feature/bugfix branch, often in order to update local stuff)
1. Deal with conflicts when needed. Such conflicts are expected to arise when the vendor changed something that was also changed locally.

This is also needed when the external data disappears: in that case, mention that the external data is no longer available to the public to avoid developers searching for it (or even worse, continue with a copy that still exists elsewhere!! Such a copy does not belong on the vendor branch).

## Notes

### When conflicts are no longer manageable

In some cases, merging vendor updates may become too difficult if not impossible. Nevertheless, vendor drops will continue to provide the means to port changes, by allowing the developer to study what changed in the vendor branch, and apply those changes manually.

### Importing vendor code written in other languages

If the artifact is not usable without extensive rewriting (e.g. Fortran code in a C++ project), it might still make sense to vendor the original file and keep it as a neutral (uncompiled) text file that will serve as an excellent reference.

This also applies for cases where explanatory/example (pseudo) sourcecode is offered; this code is essential documentation that the vendor will update occasionally. 

A typical example is the set of [JPL DExxx development ephemerides](https://ssd.jpl.nasa.gov/planets/eph_export.html), that are accompanied by important documentation *that is to be stored in the vendor branch too*:

- https://ssd.jpl.nasa.gov/ftp/eph/planets/ascii/ascii_format.txt
- https://ssd.jpl.nasa.gov/ftp/eph/planets/fortran/ (complete!) (includes https://ssd.jpl.nasa.gov/ftp/eph/planets/fortran/userguide.txt)
- https://ssd.jpl.nasa.gov/ftp/eph/planets/other_readers.txt

Should any of those "other readers" be used in any way in Stellarium, then these belong in separate vendor branches.

### How to deal with module files that are updated externally and still not vendored internally

The basic idea is to create a vendor branch with the new artifact, and do some manual "undo-redo" work.

### Version information in vendor file/directory paths

Sometimes, developers add directories/files with path names containing version information. An example is the FAQ file of [this landscape](http://www.alienbasecamp.com/Stellarium/sun.htm). 

This is a bad practice, usually done by developers that do not have access to a good VCS.

In such cases, the version information should be removed before committing the vendor drop; this will present the additional advantage that the history of such a file becomes available. This is a delicate step, as the "pristine copy" requirement is no longer fulfilled. If filename information is part of the correct functioning of the imported artifacts (e.g. it is also referenced in vendor scripts, in linked data, etc), then you are facing a difficult problem. In such a case (for now), it might be better to not rename the vendor file paths at all and accept that file history must be discovered by other means (e.g. a manual diff).

### Using data from other users

The vendor branch approach also works for users that want to keep track of other user's configuration data or scripts while at the same time apply local changes. This works better for file formats that are easily merged because being insensitive to line numbers, such as YAML; a bad example is the numbering format chosen in the [Ocular config files](https://github.com/Stellarium/stellarium/blob/ba80d33d4bc83d72fc15cca53f798cd9439482cf/plugins/Oculars/resources/default_ocular.ini).

