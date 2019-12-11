FROM centos:7.7.1908

RUN yum groups mark install "Development Tools"
RUN yum groups mark convert "Development Tools"
RUN yum -y groupinstall "Development Tools"
RUN yum -y install wget
RUN yum -y install qt5-qtbase-devel
RUN yum -y install qt5-qtscript-devel
RUN yum -y install qt5-linguist
RUN yum -y install cmake

WORKDIR /app
