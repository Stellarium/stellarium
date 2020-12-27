The scripts in this folder may be useful for those who want to
organize sustainable translation process of the sky culture description translation.

Prerequisites:

Installed po4a package.

I extract.sh:

Usage:

1. Update po4a.config with the new cultures if needed.
2. Run from this folder

   sh extract.sh

3. The sky cultures POT file will be stored in
   po/stellarium-skycultures-descriptions of the Stellarium code folder.

II translate.sh

1. Put your translations into po/stellarium-skycultures-descriptions
2. Run from this folder

sh translate.sh <your_locale>

if you need to translate the descriptions for one locale only.

Or run

sh translate.sh

if you need to translate the descriptions for all available locales.
3. The translated descriptions will be created in the skycultures folder, ready
   for you to create a pull request.

  * Both of these files are free software. They come without any warranty, to
  * the extent permitted by applicable law. You can redistribute it
  * and/or modify them under the terms of the Do What The Fuck You Want
  * To Public License, Version 2, as published by Sam Hocevar. See
  * http://sam.zoy.org/wtfpl/COPYING for more details.
