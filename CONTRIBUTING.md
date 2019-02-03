# Contributing to Stellarium
Hello, and thank you for your interest in contributing to Stellarium! 
Please take a minute to review our Contribution Guidelines, as this will 
result in getting your issue resolved or pull request merged faster.
Contributions are always welcome, no matter how large or small. :)

## Reporting Issues
Before reporting an issue around graphics artifacts like missing menu buttons, 
strange colors, Moon not rendered, or similar, *please make absolutely sure* 
you are running the latest graphics drivers for your graphics card. 
95% of graphics problems are solved this way.

Before reporting an issue, please *absolutely make sure* to check the recent 
open and also [closed issues](https://github.com/Stellarium/stellarium/issues?q=is%3Aissue+is%3Aclosed) 
whether it has been reported and solved/closed already! Don't report a new 
issue in this case, you may join the discussion.

If you want to report a bug, please make sure to update and verify the bug 
still exists in the current version.  In this case please also check 
[current beta builds](https://github.com/Stellarium/stellarium-data/releases/tag/beta) (i.e., the latest development version).

Also look into the [wiki](https://github.com/Stellarium/stellarium/wiki/Common-Problems-for-the-current-version) 
and [FAQ](https://github.com/Stellarium/stellarium/wiki/FAQ) and check if you are 
attempting to report a known issue. Don't report in this case.

Also look into the User Guide before reporting unexpected but correct behaviour. 
Don't report in this case.

If you miss translations, please help us with your language and join our 
translators at [Transifex](https://www.transifex.com/stellarium/stellarium/dashboard/).

If the bug still persists, or you're find any typos, errors, or new feature suggestions 
feel free to open a new issue.

When opening an issue to report a problem, please try and provide a minimal steps that 
reproduces the issue, and also include details of the operating
system, Stellarium versions and Graphics Card info you are using.

## Pull Requests (Contributing code)
So you're interested in contributing code to Stellarium? Excellent!

Most contributions to Stellarium are done via pull requests from GitHub users'
forks of the [Stellarium repository](https://github.com/Stellarium/stellarium). 
If you're new to this style of development, you'll want to read over our
[development workflow](https://github.com/Stellarium/stellarium/wiki/Git-Contributor-Workflow).

You may also/instead be interested in contributing to an stellarium affiliated stuff, like 
landscapes, sky cultures, DSO or planetary textures and scripts.

Once you open a pull request (which should be opened against the ``master``
branch, not against any of the other branches), please make sure that you
include the following:

- **Code**: the code you are adding, which should follow as much as possible
  our [coding guidelines](http://stellarium.org/doc/head/codingStyle.html).

- **Tests**: these are usually tests to ensure that code that previously
  failed now works (regression tests) or tests that cover as much as possible
  of the new functionality to make sure it doesn't break in future, and also
  returns consistent results on all platforms (since we run these tests on many
  platforms/configurations). 

- **Documentation**: if you are adding new functionality, be sure to include a 
description in the main documentation (in ``docs/``) or doxygen description/comments 
for code in the ``*.hpp`` files (commenting the code may be extremelly helpful!).

Thanks!

\- *The Stellarium development team*
