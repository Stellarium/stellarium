
# How to compile on centos 7

- Install basic devel tools:
    sudo yum group install "DevelopmentTools"
- Install Qt from the Qt online installer (not the os version).  Tested with Qt 5.9.7 and Qt 5.9.8.
- set the PATH to point to the installed qt bin.
- Run the script: ./plugins/Vts/create-package <path-to-vts>

This should compile and copy the files into <path-to-vts>/Apps/Stellarium
