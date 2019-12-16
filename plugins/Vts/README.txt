# This is a plugin allowing Stellarium to be used in VTS
# See https://timeloop.fr/vts/

# How to compile with docker on any OS
cd plugins/Vts
# Build the docker image
docker build -t vts-stellarium-centos7-builder .
# Build stellarium + Vts plugin and create the package
docker run -it -v "$PWD/../..:/app/" vts-stellarium-centos7-builder ./plugins/Vts/create-package.sh

This should create a Stellarium.tgz in the top-level stellarium directory.
It needs to be decompressed in the <path-to-vts>/Apps/Stellarium directory
