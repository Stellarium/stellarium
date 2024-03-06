This directory contains the QtWebApp HTTP server (only the httpserver module for now).
It currently has some fixes for crashing threading issues (on stopping the server), 
and changes to avoid using QSettings for configuration.
The template system has a new tag type to enable a different approach to translation than separate .html files.

Copyright Stefan Frings (http://www.stefanfrings.de)
Published under the GNU LGPL.