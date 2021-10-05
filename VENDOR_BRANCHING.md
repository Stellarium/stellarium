# Vendor branching : importing external artifacts

Stellarium will often be enriched with text/data/source files that are **copy-pasted** from places outside of Stellarium.

Examples:

- geonames data (https://www.geonames.org/recent-changes.html), https://github.com/Stellarium/stellarium-data/tags
- quasar data (https://github.com/Stellarium/stellarium/blob/master/plugins/Quasars/util/quasars.tsv)
- Almagest data (minor fixes, of course - this is essentially frozen data)
- various libraries (https://github.com/Stellarium/stellarium/tree/master/src/external)

The problem with copy-pasting external data is **code (or data) rot** (and also: "*copy-paste is evil!*").

The external version of such artifacts will often continue to evolve, but these modifications obviously will not be reflected in our repository, thus leading to artifacts slowly getting totally outdated (hence the term "code rot"). 

Importing external artifacts therefore requires a specific (and in this case a *simple* as wel as generic) approach: the **Vendor Branch** mechanism. Other approaches exist, such as subtrees and submodules (in case Git is used for the external artifacts), but have inconveniences and are not discussed here; the proposed approach is *simple to apply to novice programmers*.

## The idea
1. keep external information alive (and updated) on a **separate** branch, called a "**vendor branch**" (eg ``vendor/geonames``). Every dataset/tool lives in its own vendor branch. This branch tracks a pristine Git copy/mirror of the external data.
1. optionally, the external information is stored in a separate directory (eg ``external/<vendor>/<toolname>``)
1. a ``VENDOR`` file explains where the external/original information can be found, so that it can be updated when necessary.
1. **vendor tags** describe which version of the vendor artifacts have been imported (eg ``vendor/geonames/2021-08-21``)
1. that branch is merged to wherever it is needed (normally the feature branch, which will eventually be merged to ``master``)
1. if needed, modify the external information *locally* (usually via a feature/master branch, but *never* in its vendor branch)
1. ideally, issues in the vendor artifacts should be reported to the vendor, so that updates can then be imported via a vendor drop. alternatively, the developer can solve issues locally, but *never* in the vendor branch.

## managing vendor artifacts

Basically, two use cases exist: you import a new artifact from scratch, or you want to update an artifact because it changed in the remote location. (A third case exists: existing data is to be made vendor-branch compatible.)

## 1 importing new artifacts from scratch

1. checkout the feature branch that will import the data in the project
1. if needed (in order to ingest many files), create a directory for the vendor artifacts, e.g. ``external/geonames``
1. create and checkout a vendor branch, e.g. ``vendor/geonames``
1. unzip/copy/import/... the external data. This is called a *vendor drop*.
1. make sure that file/directory *names* do not contain version information as a kind of implicit versioning scheme! rename when needed.
1. commit the vendor data
1. tag the vendor branch, e.g. ``vendor/geonames/1.0``. if the vendor does not provide a clear version number, use the ISO date of the drop : ``vendor/geonames/2021-09-09T1200``
1. **switch to the feature/master/whatever local branch**
1. merge the vendor branch 
1. EITHER create (add) a ``VENDOR`` file *in* the vendor directory (or a similar name, in the unlikely chance that the file name is already in use) OR edit an existing vendor file to include detailed source location data and, if needed, instructions how to find back the data. Avoid top-level (domain) adresses, try to make life easy for anyone wanting to update the data. Make sure to include the keyword "VENDOR" somewhere so that it can be found if needed.
1. if needed, do whatever is needed to transform the data *in the feature branch*. try to provide a scripted way to transform data, rather than rely on manual operations, so that this can be run again whenever the external data is re-imported.

For prolific vendors (offering many independent tools/data), it might be necessary to create subdirectory and separate branches per product.

## 2 updating existing artifacts

Now it becomes easy to update external information. Whenever the external information changes:

1. checkout the vendor branch
1. perform a fresh **vendor drop**: 
	1. empty the vendor folder OR delete the vendor file ; you can use this: ``git ls-files -z | xargs -0 rm -f``
	2. replace/explode/unzip/untar/...
1. commit: ``git add -A && git commit``  (see ``git help git-rm`` for details, search for "vendor".)
1. vendor tag the vendor branch
1. merge the updated vendor branch to ``master`` (or via an intermediate feature/bugfix branch, often in order to update local stuff)
1. deal with conflicts when needed. such conflicts are expected to arise when the vendor changed something, and that was also changed locally

This is also needed when the external data dsappears: in that case, mention that the external data is no longer available to the public to avoid developers searching for it (or even worse, continue with a copy that still exists elsewhere!! Such a copy does not belong on the vendor branch).

## Notes

### version information in vendor file/directory paths

Sometimes, developers add directories/files with path names containing version information, such as a FAQ file in [this landscape](http://www.alienbasecamp.com/Stellarium/sun.htm). 

This is a bad practice, often done by developers that do not have access to a good VCS.

In such cases, the version information should be removed before committing the vendor drop; this will present the additional advantage that the history of such a file becomes available. This is a delicate step, as the "pristine copy" is no longer fulfilled. If filename information is part of the correct functioning of the imported artifacts (eg it is also referenced in vendor scripts, in linked data, etc), then you are facing a difficult problem. In such a case, it might be better to not rename the vendor file paths at all and accept that file history must be discovered by other means (eg a manual diff).

