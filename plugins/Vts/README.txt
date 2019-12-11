# How to compile with docker on any OS
cd plugins/Vts
# Build the docker image
docker build -t vts-stellarium-centos7-builder .
# Build stellarium + Vts plugin and create the package
docker run -it -v "$PWD/../..:/app/" vts-stellarium-centos7-builder ./plugins/Vts/create-package.sh

This should create a Stellarium.tgz in the top-level stellarium directory.
It needs to be decompressed in the <path-to-vts>/Apps/Stellarium directory

# How to compile on centos 7

- Install basic devel tools:
    sudo yum group install "DevelopmentTools"
- Install Qt from the Qt online installer (not the os version).  Tested with Qt 5.9.7 and Qt 5.9.8.
- set the PATH to point to the installed qt bin.
- Run the script: ./plugins/Vts/create-package

This should create a Stellarium.tgz in the top-level stellarium directory.
It needs to be decompressed in the <path-to-vts>/Apps/Stellarium directory
