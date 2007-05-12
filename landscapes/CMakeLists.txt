ADD_SUBDIRECTORY( guereins ) 
ADD_SUBDIRECTORY( trees ) 
ADD_SUBDIRECTORY( moon ) 
ADD_SUBDIRECTORY( hurricane ) 
ADD_SUBDIRECTORY( ocean ) 


########### install files ###############

INSTALL_FILES( /share/@PACKAGE@/landscapes FILES  )



#original Makefile.am contents follow:

#pkgdatadir = $(datadir)/@PACKAGE@/landscapes
#SUBDIRS = guereins trees moon hurricane ocean
