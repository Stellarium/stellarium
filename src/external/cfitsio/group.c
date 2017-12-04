/*  This file, group.c, contains the grouping convention suport routines.  */

/*  The FITSIO software was written by William Pence at the High Energy    */
/*  Astrophysic Science Archive Research Center (HEASARC) at the NASA      */
/*  Goddard Space Flight Center.                                           */
/*                                                                         */
/*  The group.c module of CFITSIO was written by Donald G. Jennings of     */
/*  the INTEGRAL Science Data Centre (ISDC) under NASA contract task       */
/*  66002J6. The above copyright laws apply. Copyright guidelines of The   */
/*  University of Geneva might also apply.                                 */

/*  The following routines are designed to create, read, and manipulate    */
/*  FITS Grouping Tables as defined in the FITS Grouping Convention paper  */
/*  by Jennings, Pence, Folk and Schlesinger. The development of the       */
/*  grouping structure was partially funded under the NASA AISRP Program.  */ 
    
#include "fitsio2.h"
#include "group.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#if defined(WIN32) || defined(__WIN32__)
#include <direct.h>   /* defines the getcwd function on Windows PCs */
#endif

#if defined(unix) || defined(__unix__)  || defined(__unix) || defined(HAVE_UNISTD_H)
#include <unistd.h>  /* needed for getcwd prototype on unix machines */
#endif

#define HEX_ESCAPE '%'

/*---------------------------------------------------------------------------
 Change record:

D. Jennings, 18/06/98, version 1.0 of group module delivered to B. Pence for
                       integration into CFITSIO 2.005

D. Jennings, 17/11/98, fixed bug in ffgtcpr(). Now use fits_find_nextkey()
                       correctly and insert auxiliary keyword records 
		       directly before the TTYPE1 keyword in the copied
		       group table.

D. Jennings, 22/01/99, ffgmop() now looks for relative file paths when 
                       the MEMBER_LOCATION information is given in a 
		       grouping table.

D. Jennings, 01/02/99, ffgtop() now looks for relatve file paths when 
                       the GRPLCn keyword value is supplied in the member
		       HDU header.

D. Jennings, 01/02/99, ffgtam() now trys to construct relative file paths
                       from the member's file to the group table's file
		       (and visa versa) when both the member's file and
		       group table file are of access type FILE://.

D. Jennings, 05/05/99, removed the ffgtcn() function; made obsolete by
                       fits_get_url().

D. Jennings, 05/05/99, updated entire module to handle partial URLs and
                       absolute URLs more robustly. Host dependent directory
		       paths are now converted to true URLs before being
		       read from/written to grouping tables.

D. Jennings, 05/05/99, added the following new functions (note, none of these
                       are directly callable by the application)

		       int fits_path2url()
		       int fits_url2path()
		       int fits_get_cwd()
		       int fits_get_url()
		       int fits_clean_url()
		       int fits_relurl2url()
		       int fits_encode_url()
		       int fits_unencode_url()
		       int fits_is_url_absolute()

-----------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
int ffgtcr(fitsfile *fptr,      /* FITS file pointer                         */
	   char    *grpname,    /* name of the grouping table                */
	   int      grouptype,  /* code specifying the type of
				   grouping table information:
				   GT_ID_ALL_URI  0 ==> defualt (all columns)
				   GT_ID_REF      1 ==> ID by reference
				   GT_ID_POS      2 ==> ID by position
				   GT_ID_ALL      3 ==> ID by ref. and position
				   GT_ID_REF_URI 11 ==> (1) + URI info 
				   GT_ID_POS_URI 12 ==> (2) + URI info       */
	   int      *status    )/* return status code                        */

/* 
   create a grouping table at the end of the current FITS file. This
   function makes the last HDU in the file the CHDU, then calls the
   fits_insert_group() function to actually create the new grouping table.
*/

{
  int hdutype;
  int hdunum;


  if(*status != 0) return(*status);


  *status = fits_get_num_hdus(fptr,&hdunum,status);

  /* If hdunum is 0 then we are at the beginning of the file and
     we actually haven't closed the first header yet, so don't do
     anything more */

  if (0 != hdunum) {

      *status = fits_movabs_hdu(fptr,hdunum,&hdutype,status);
  }

  /* Now, the whole point of the above two fits_ calls was to get to
     the end of file.  Let's ignore errors at this point and keep
     going since any error is likely to mean that we are already at the 
     EOF, or the file is fatally corrupted.  If we are at the EOF then
     the next fits_ call will be ok.  If it's corrupted then the
     next call will fail, but that's not big deal at this point.
  */

  if (0 != *status ) *status = 0;

  *status = fits_insert_group(fptr,grpname,grouptype,status);

  return(*status);
}

/*---------------------------------------------------------------------------*/
int ffgtis(fitsfile *fptr,      /* FITS file pointer                         */
	   char    *grpname,    /* name of the grouping table                */
	   int      grouptype,  /* code specifying the type of
				   grouping table information:
				   GT_ID_ALL_URI  0 ==> defualt (all columns)
				   GT_ID_REF      1 ==> ID by reference
				   GT_ID_POS      2 ==> ID by position
				   GT_ID_ALL      3 ==> ID by ref. and position
				   GT_ID_REF_URI 11 ==> (1) + URI info 
				   GT_ID_POS_URI 12 ==> (2) + URI info       */
	   int      *status)     /* return status code                       */
	   
/* 
   insert a grouping table just after the current HDU of the current FITS file.
   This is the same as fits_create_group() only it allows the user to select
   the place within the FITS file to add the grouping table.
*/

{

  int tfields  = 0;
  int hdunum   = 0;
  int hdutype  = 0;
  int extver;
  int i;
  
  long pcount  = 0;

  char *ttype[6];
  char *tform[6];

  char ttypeBuff[102];  
  char tformBuff[54];  

  char  extname[] = "GROUPING";
  char  keyword[FLEN_KEYWORD];
  char  keyvalue[FLEN_VALUE];
  char  comment[FLEN_COMMENT];
    
  do
    {

      /* set up the ttype and tform character buffers */

      for(i = 0; i < 6; ++i)
	{
	  ttype[i] = ttypeBuff+(i*17);
	  tform[i] = tformBuff+(i*9);
	}

      /* define the columns required according to the grouptype parameter */

      *status = ffgtdc(grouptype,0,0,0,0,0,0,ttype,tform,&tfields,status);

      /* create the grouping table using the columns defined above */

      *status = fits_insert_btbl(fptr,0,tfields,ttype,tform,NULL,
				 NULL,pcount,status);

      if(*status != 0) continue;

      /*
	 retrieve the hdu position of the new grouping table for
	 future use
      */

      fits_get_hdu_num(fptr,&hdunum);

      /*
	 add the EXTNAME and EXTVER keywords to the HDU just after the 
	 TFIELDS keyword; for now the EXTVER value is set to 0, it will be 
	 set to the correct value later on
      */

      fits_read_keyword(fptr,"TFIELDS",keyvalue,comment,status);

      fits_insert_key_str(fptr,"EXTNAME",extname,
			  "HDU contains a Grouping Table",status);
      fits_insert_key_lng(fptr,"EXTVER",0,"Grouping Table vers. (this file)",
			  status);

      /* 
	 if the grpname parameter value was defined (Non NULL and non zero
	 length) then add the GRPNAME keyword and value
      */

      if(grpname != NULL && strlen(grpname) > 0)
	fits_insert_key_str(fptr,"GRPNAME",grpname,"Grouping Table name",
			    status);

      /* 
	 add the TNULL keywords and values for each integer column defined;
	 integer null values are zero (0) for the MEMBER_POSITION and 
	 MEMBER_VERSION columns.
      */

      for(i = 0; i < tfields && *status == 0; ++i)
	{	  
	  if(fits_strcasecmp(ttype[i],"MEMBER_POSITION") == 0 ||
	     fits_strcasecmp(ttype[i],"MEMBER_VERSION")  == 0)
	    {
	      sprintf(keyword,"TFORM%d",i+1);
	      *status = fits_read_key_str(fptr,keyword,keyvalue,comment,
					  status);
	 
	      sprintf(keyword,"TNULL%d",i+1);

	      *status = fits_insert_key_lng(fptr,keyword,0,"Column Null Value",
					    status);
	    }
	}

      /*
	 determine the correct EXTVER value for the new grouping table
	 by finding the highest numbered grouping table EXTVER value
	 the currently exists
      */

      for(extver = 1;
	  (fits_movnam_hdu(fptr,ANY_HDU,"GROUPING",extver,status)) == 0; 
	  ++extver);

      if(*status == BAD_HDU_NUM) *status = 0;

      /*
	 move back to the new grouping table HDU and update the EXTVER
	 keyword value
      */

      fits_movabs_hdu(fptr,hdunum,&hdutype,status);

      fits_modify_key_lng(fptr,"EXTVER",extver,"&",status);

    }while(0);


  return(*status);
}

/*---------------------------------------------------------------------------*/
int ffgtch(fitsfile *gfptr,     /* FITS pointer to group                     */
	   int       grouptype, /* code specifying the type of
				   grouping table information:
				   GT_ID_ALL_URI  0 ==> defualt (all columns)
				   GT_ID_REF      1 ==> ID by reference
				   GT_ID_POS      2 ==> ID by position
				   GT_ID_ALL      3 ==> ID by ref. and position
				   GT_ID_REF_URI 11 ==> (1) + URI info 
				   GT_ID_POS_URI 12 ==> (2) + URI info       */
	   int      *status)     /* return status code                       */


/* 
   Change the grouping table structure of the grouping table pointed to by
   gfptr. The grouptype code specifies the new structure of the table. This
   operation only adds or removes grouping table columns, it does not add
   or delete group members (i.e., table rows). If the grouping table already
   has the desired structure then no operations are performed and function   
   simply returns with a (0) success status code. If the requested structure
   change creates new grouping table columns, then the column values for all
   existing members will be filled with the appropriate null values.
*/

{
  int xtensionCol, extnameCol, extverCol, positionCol, locationCol, uriCol;
  int ncols    = 0;
  int colnum   = 0;
  int nrows    = 0;
  int grptype  = 0;
  int i,j;

  long intNull  = 0;
  long tfields  = 0;
  
  char *tform[6];
  char *ttype[6];

  unsigned char  charNull[1] = {'\0'};

  char ttypeBuff[102];  
  char tformBuff[54];  

  char  keyword[FLEN_KEYWORD];
  char  keyvalue[FLEN_VALUE];
  char  comment[FLEN_COMMENT];


  if(*status != 0) return(*status);

  do
    {
      /* set up the ttype and tform character buffers */

      for(i = 0; i < 6; ++i)
	{
	  ttype[i] = ttypeBuff+(i*17);
	  tform[i] = tformBuff+(i*9);
	}

      /* retrieve positions of all Grouping table reserved columns */

      *status = ffgtgc(gfptr,&xtensionCol,&extnameCol,&extverCol,&positionCol,
		       &locationCol,&uriCol,&grptype,status);

      if(*status != 0) continue;

      /* determine the total number of grouping table columns */

      *status = fits_read_key_lng(gfptr,"TFIELDS",&tfields,comment,status);

      /* define grouping table columns to be added to the configuration */

      *status = ffgtdc(grouptype,xtensionCol,extnameCol,extverCol,positionCol,
		       locationCol,uriCol,ttype,tform,&ncols,status);

      /*
	delete any grouping tables columns that exist but do not belong to
	new desired configuration; note that we delete before creating new
	columns for (file size) efficiency reasons
      */

      switch(grouptype)
	{

	case GT_ID_ALL_URI:

	  /* no columns to be deleted in this case */

	  break;

	case GT_ID_REF:

	  if(positionCol != 0) 
	    {
	      *status = fits_delete_col(gfptr,positionCol,status);
	      --tfields;
	      if(uriCol      > positionCol)  --uriCol;
	      if(locationCol > positionCol) --locationCol;
	    }
	  if(uriCol      != 0)
	    { 
	    *status = fits_delete_col(gfptr,uriCol,status);
	      --tfields;
	      if(locationCol > uriCol) --locationCol;
	    }
	  if(locationCol != 0) 
	    *status = fits_delete_col(gfptr,locationCol,status);

	  break;

	case  GT_ID_POS:

	  if(xtensionCol != 0) 
	    {
	      *status = fits_delete_col(gfptr,xtensionCol,status);
	      --tfields;
	      if(extnameCol  > xtensionCol)  --extnameCol;
	      if(extverCol   > xtensionCol)  --extverCol;
	      if(uriCol      > xtensionCol)  --uriCol;
	      if(locationCol > xtensionCol)  --locationCol;
	    }
	  if(extnameCol  != 0) 
	    {
	      *status = fits_delete_col(gfptr,extnameCol,status);
	      --tfields;
	      if(extverCol   > extnameCol)  --extverCol;
	      if(uriCol      > extnameCol)  --uriCol;
	      if(locationCol > extnameCol)  --locationCol;
	    }
	  if(extverCol   != 0)
	    { 
	      *status = fits_delete_col(gfptr,extverCol,status);
	      --tfields;
	      if(uriCol      > extverCol)  --uriCol;
	      if(locationCol > extverCol)  --locationCol;
	    }
	  if(uriCol      != 0)
	    { 
	      *status = fits_delete_col(gfptr,uriCol,status);
	      --tfields;
	      if(locationCol > uriCol)  --locationCol;
	    }
	  if(locationCol != 0)
	    { 
	      *status = fits_delete_col(gfptr,locationCol,status);
	      --tfields;
	    }
	  
	  break;

	case  GT_ID_ALL:

	  if(uriCol      != 0) 
	    {
	      *status = fits_delete_col(gfptr,uriCol,status);
	      --tfields;
	      if(locationCol > uriCol)  --locationCol;
	    }
	  if(locationCol != 0)
	    { 
	      *status = fits_delete_col(gfptr,locationCol,status);
	      --tfields;
	    }

	  break;

	case GT_ID_REF_URI:

	  if(positionCol != 0)
	    { 
	      *status = fits_delete_col(gfptr,positionCol,status);
	      --tfields;
	    }

	  break;

	case  GT_ID_POS_URI:

	  if(xtensionCol != 0) 
	    {
	      *status = fits_delete_col(gfptr,xtensionCol,status);
	      --tfields;
	      if(extnameCol > xtensionCol)  --extnameCol;
	      if(extverCol  > xtensionCol)  --extverCol;
	    }
	  if(extnameCol  != 0)
	    { 
	      *status = fits_delete_col(gfptr,extnameCol,status);
	      --tfields;
	      if(extverCol > extnameCol)  --extverCol;
	    }
	  if(extverCol   != 0)
	    { 
	      *status = fits_delete_col(gfptr,extverCol,status);
	      --tfields;
	    }

	  break;

	default:

	  *status = BAD_OPTION;
	  ffpmsg("Invalid value for grouptype parameter specified (ffgtch)");
	  break;

	}

      /*
	add all the new grouping table columns that were not there
	previously but are called for by the grouptype parameter
      */

      for(i = 0; i < ncols && *status == 0; ++i)
	*status = fits_insert_col(gfptr,tfields+i+1,ttype[i],tform[i],status);

      /* 
	 add the TNULL keywords and values for each new integer column defined;
	 integer null values are zero (0) for the MEMBER_POSITION and 
	 MEMBER_VERSION columns. Insert a null ("/0") into each new string
	 column defined: MEMBER_XTENSION, MEMBER_NAME, MEMBER_URI_TYPE and
	 MEMBER_LOCATION. Note that by convention a null string is the
	 TNULL value for character fields so no TNULL is required.
      */

      for(i = 0; i < ncols && *status == 0; ++i)
	{	  
	  if(fits_strcasecmp(ttype[i],"MEMBER_POSITION") == 0 ||
	     fits_strcasecmp(ttype[i],"MEMBER_VERSION")  == 0)
	    {
	      /* col contains int data; set TNULL and insert 0 for each col */

	      *status = fits_get_colnum(gfptr,CASESEN,ttype[i],&colnum,
					status);
	      
	      sprintf(keyword,"TFORM%d",colnum);

	      *status = fits_read_key_str(gfptr,keyword,keyvalue,comment,
					  status);
	 
	      sprintf(keyword,"TNULL%d",colnum);

	      *status = fits_insert_key_lng(gfptr,keyword,0,
					    "Column Null Value",status);

	      for(j = 1; j <= nrows && *status == 0; ++j)
		*status = fits_write_col_lng(gfptr,colnum,j,1,1,&intNull,
					     status);
	    }
	  else if(fits_strcasecmp(ttype[i],"MEMBER_XTENSION") == 0 ||
		  fits_strcasecmp(ttype[i],"MEMBER_NAME")     == 0 ||
		  fits_strcasecmp(ttype[i],"MEMBER_URI_TYPE") == 0 ||
		  fits_strcasecmp(ttype[i],"MEMBER_LOCATION") == 0)
	    {

	      /* new col contains character data; insert NULLs into each col */

	      *status = fits_get_colnum(gfptr,CASESEN,ttype[i],&colnum,
					status);

	      for(j = 1; j <= nrows && *status == 0; ++j)
	    /* WILL THIS WORK FOR VAR LENTH CHAR COLS??????*/
		*status = fits_write_col_byt(gfptr,colnum,j,1,1,charNull,
					     status);
	    }
	}

    }while(0);

  return(*status);
}

/*---------------------------------------------------------------------------*/
int ffgtrm(fitsfile *gfptr,  /* FITS file pointer to group                   */
	   int       rmopt,  /* code specifying if member
				elements are to be deleted:
				OPT_RM_GPT ==> remove only group table
				OPT_RM_ALL ==> recursively remove members
				and their members (if groups)                */
	   int      *status) /* return status code                           */
	    
/*
  remove a grouping table, and optionally all its members. Any groups 
  containing the grouping table are updated, and all members (if not 
  deleted) have their GRPIDn and GRPLCn keywords updated accordingly. 
  If the (deleted) members are members of another grouping table then those
  tables are also updated. The CHDU of the FITS file pointed to by gfptr must 
  be positioned to the grouping table to be deleted.
*/

{
  int hdutype;

  long i;
  long nmembers = 0;

  HDUtracker HDU;
  

  if(*status != 0) return(*status);

  /*
     remove the grouping table depending upon the rmopt parameter
  */

  switch(rmopt)
    {

    case OPT_RM_GPT:

      /*
	 for this option, the grouping table is deleted, but the member
	 HDUs remain; in this case we only have to remove each member from
	 the grouping table by calling fits_remove_member() with the
	 OPT_RM_ENTRY option
      */

      /* get the number of members contained by this table */

      *status = fits_get_num_members(gfptr,&nmembers,status);

      /* loop over all grouping table members and remove them */

      for(i = nmembers; i > 0 && *status == 0; --i)
	*status = fits_remove_member(gfptr,i,OPT_RM_ENTRY,status);
      
	break;

    case OPT_RM_ALL:

      /*
	for this option the entire Group is deleted -- this includes all
	members and their members (if grouping tables themselves). Call 
	the recursive form of this function to perform the removal.
      */

      /* add the current grouping table to the HDUtracker struct */

      HDU.nHDU = 0;

      *status = fftsad(gfptr,&HDU,NULL,NULL);

      /* call the recursive group remove function */

      *status = ffgtrmr(gfptr,&HDU,status);

      /* free the memory allocated to the HDUtracker struct */

      for(i = 0; i < HDU.nHDU; ++i)
	{
	  free(HDU.filename[i]);
	  free(HDU.newFilename[i]);
	}

      break;

    default:
      
      *status = BAD_OPTION;
      ffpmsg("Invalid value for the rmopt parameter specified (ffgtrm)");
      break;

     }

  /*
     if all went well then unlink and delete the grouping table HDU
  */

  *status = ffgmul(gfptr,0,status);

  *status = fits_delete_hdu(gfptr,&hdutype,status);
      
  return(*status);
}

/*---------------------------------------------------------------------------*/
int ffgtcp(fitsfile *infptr,  /* input FITS file pointer                     */
	   fitsfile *outfptr, /* output FITS file pointer                    */
	   int        cpopt,  /* code specifying copy options:
				OPT_GCP_GPT (0) ==> copy only grouping table
				OPT_GCP_ALL (2) ==> recusrively copy members 
				                    and their members (if 
						    groups)                  */
	   int      *status)  /* return status code                          */

/*
  copy a grouping table, and optionally all its members, to a new FITS file.
  If the cpopt is set to OPT_GCP_GPT (copy grouping table only) then the 
  existing members have their GRPIDn and GRPLCn keywords updated to reflect 
  the existance of the new group, since they now belong to another group. If 
  cpopt is set to OPT_GCP_ALL (copy grouping table and members recursively) 
  then the original members are not updated; the new grouping table is 
  modified to include only the copied member HDUs and not the original members.

  Note that the recursive version of this function, ffgtcpr(), is called
  to perform the group table copy. In the case of cpopt == OPT_GCP_GPT
  ffgtcpr() does not actually use recursion.
*/

{
  int i;

  HDUtracker HDU;


  if(*status != 0) return(*status);

  /* make sure infptr and outfptr are not the same pointer */

  if(infptr == outfptr) *status = IDENTICAL_POINTERS;
  else
    {

      /* initialize the HDUtracker struct */
      
      HDU.nHDU = 0;
      
      *status = fftsad(infptr,&HDU,NULL,NULL);
      
      /* 
	 call the recursive form of this function to copy the grouping table. 
	 If the cpopt is OPT_GCP_GPT then there is actually no recursion
	 performed
      */

      *status = ffgtcpr(infptr,outfptr,cpopt,&HDU,status);
  
      /* free memory allocated for the HDUtracker struct */

      for(i = 0; i < HDU.nHDU; ++i) 
	{
	  free(HDU.filename[i]);
	  free(HDU.newFilename[i]);
	}
    }

  return(*status);
}

/*---------------------------------------------------------------------------*/
int ffgtmg(fitsfile *infptr,  /* FITS file ptr to source grouping table      */
	   fitsfile *outfptr, /* FITS file ptr to target grouping table      */
	   int       mgopt,   /* code specifying merge options:
				 OPT_MRG_COPY (0) ==> copy members to target
				                      group, leaving source 
						      group in place
				 OPT_MRG_MOV  (1) ==> move members to target
				                      group, source group is
						      deleted after merge    */
	   int      *status)   /* return status code                         */
     

/*
  merge two grouping tables by combining their members into a single table. 
  The source grouping table must be the CHDU of the fitsfile pointed to by 
  infptr, and the target grouping table must be the CHDU of the fitsfile to by 
  outfptr. All members of the source grouping table shall be copied to the
  target grouping table. If the mgopt parameter is OPT_MRG_COPY then the source
  grouping table continues to exist after the merge. If the mgopt parameter
  is OPT_MRG_MOV then the source grouping table is deleted after the merge, 
  and all member HDUs are updated accordingly.
*/
{
  long i ;
  long nmembers = 0;

  fitsfile *tmpfptr = NULL;


  if(*status != 0) return(*status);

  do
    {

      *status = fits_get_num_members(infptr,&nmembers,status);

      for(i = 1; i <= nmembers && *status == 0; ++i)
	{
	  *status = fits_open_member(infptr,i,&tmpfptr,status);
	  *status = fits_add_group_member(outfptr,tmpfptr,0,status);

	  if(*status == HDU_ALREADY_MEMBER) *status = 0;

	  if(tmpfptr != NULL)
	    {
	      fits_close_file(tmpfptr,status);
	      tmpfptr = NULL;
	    }
	}

      if(*status != 0) continue;

      if(mgopt == OPT_MRG_MOV) 
	*status = fits_remove_group(infptr,OPT_RM_GPT,status);

    }while(0);

  if(tmpfptr != NULL)
    {
      fits_close_file(tmpfptr,status);
    }

  return(*status);
}

/*---------------------------------------------------------------------------*/
int ffgtcm(fitsfile *gfptr,  /* FITS file pointer to grouping table          */
	   int       cmopt,  /* code specifying compact options
				OPT_CMT_MBR      (1) ==> compact only direct 
			                                 members (if groups)
				OPT_CMT_MBR_DEL (11) ==> (1) + delete all 
				                         compacted groups    */
	   int      *status) /* return status code                           */
    
/*
  "Compact" a group pointed to by the FITS file pointer gfptr. This 
  is achieved by flattening the tree structure of a group and its 
  (grouping table) members. All members HDUs of a grouping table which is 
  itself a member of the grouping table gfptr are added to gfptr. Optionally,
  the grouping tables which are "compacted" are deleted. If the grouping 
  table contains no members that are themselves grouping tables then this 
  function performs a NOOP.
*/

{
  long i;
  long nmembers = 0;

  char keyvalue[FLEN_VALUE];
  char comment[FLEN_COMMENT];

  fitsfile *mfptr = NULL;


  if(*status != 0) return(*status);

  do
    {
      if(cmopt != OPT_CMT_MBR && cmopt != OPT_CMT_MBR_DEL)
	{
	  *status = BAD_OPTION;
	  ffpmsg("Invalid value for cmopt parameter specified (ffgtcm)");
	  continue;
	}

      /* reteive the number of grouping table members */

      *status = fits_get_num_members(gfptr,&nmembers,status);

      /*
	loop over all the grouping table members; if the member is a 
	grouping table then merge its members with the parent grouping 
	table 
      */

      for(i = 1; i <= nmembers && *status == 0; ++i)
	{
	  *status = fits_open_member(gfptr,i,&mfptr,status);

	  if(*status != 0) continue;

	  *status = fits_read_key_str(mfptr,"EXTNAME",keyvalue,comment,status);

	  /* if no EXTNAME keyword then cannot be a grouping table */

	  if(*status == KEY_NO_EXIST) 
	    {
	      *status = 0;
	      continue;
	    }
	  prepare_keyvalue(keyvalue);

	  if(*status != 0) continue;

	  /* if EXTNAME == "GROUPING" then process member as grouping table */

	  if(fits_strcasecmp(keyvalue,"GROUPING") == 0)
	    {
	      /* merge the member (grouping table) into the grouping table */

	      *status = fits_merge_groups(mfptr,gfptr,OPT_MRG_COPY,status);

	      *status = fits_close_file(mfptr,status);
	      mfptr = NULL;

	      /* 
		 remove the member from the grouping table now that all of
		 its members have been transferred; if cmopt is set to
		 OPT_CMT_MBR_DEL then remove and delete the member
	      */

	      if(cmopt == OPT_CMT_MBR)
		*status = fits_remove_member(gfptr,i,OPT_RM_ENTRY,status);
	      else
		*status = fits_remove_member(gfptr,i,OPT_RM_MBR,status);
	    }
	  else
	    {
	      /* not a grouping table; just close the opened member */

	      *status = fits_close_file(mfptr,status);
	      mfptr = NULL;
	    }
	}

    }while(0);

  return(*status);
}

/*--------------------------------------------------------------------------*/
int ffgtvf(fitsfile *gfptr,       /* FITS file pointer to group             */
	   long     *firstfailed, /* Member ID (if positive) of first failed
				     member HDU verify check or GRPID index
				     (if negitive) of first failed group
				     link verify check.                     */
	   int      *status)      /* return status code                     */

/*
 check the integrity of a grouping table to make sure that all group members 
 are accessible and all the links to other grouping tables are valid. The
 firstfailed parameter returns the member ID of the first member HDU to fail
 verification if positive or the first group link to fail if negative; 
 otherwise firstfailed contains a return value of 0.
*/

{
  long i;
  long nmembers = 0;
  long ngroups  = 0;

  char errstr[FLEN_VALUE];

  fitsfile *fptr = NULL;


  if(*status != 0) return(*status);

  *firstfailed = 0;

  do
    {
      /*
	attempt to open all the members of the grouping table. We stop
	at the first member which cannot be opened (which implies that it
	cannot be located)
      */

      *status = fits_get_num_members(gfptr,&nmembers,status);

      for(i = 1; i <= nmembers && *status == 0; ++i)
	{
	  *status = fits_open_member(gfptr,i,&fptr,status);
	  fits_close_file(fptr,status);
	}

      /*
	if the status is non-zero from the above loop then record the
	member index that caused the error
      */

      if(*status != 0)
	{
	  *firstfailed = i;
	  sprintf(errstr,"Group table verify failed for member %ld (ffgtvf)",
		  i);
	  ffpmsg(errstr);
	  continue;
	}

      /*
	attempt to open all the groups linked to this grouping table. We stop
	at the first group which cannot be opened (which implies that it
	cannot be located)
      */

      *status = fits_get_num_groups(gfptr,&ngroups,status);

      for(i = 1; i <= ngroups && *status == 0; ++i)
	{
	  *status = fits_open_group(gfptr,i,&fptr,status);
	  fits_close_file(fptr,status);
	}

      /*
	if the status from the above loop is non-zero, then record the
	GRPIDn index of the group that caused the failure
      */

      if(*status != 0)
	{
	  *firstfailed = -1*i;
	  sprintf(errstr,
		  "Group table verify failed for GRPID index %ld (ffgtvf)",i);
	  ffpmsg(errstr);
	  continue;
	}

    }while(0);

  return(*status);
}

/*---------------------------------------------------------------------------*/
int ffgtop(fitsfile *mfptr,  /* FITS file pointer to the member HDU          */
	   int       grpid,  /* group ID (GRPIDn index) within member HDU    */
	   fitsfile **gfptr, /* FITS file pointer to grouping table HDU      */
	   int      *status) /* return status code                           */

/*
  open the grouping table that contains the member HDU. The member HDU must
  be the CHDU of the FITS file pointed to by mfptr, and the grouping table
  is identified by the Nth index number of the GRPIDn keywords specified in 
  the member HDU's header. The fitsfile gfptr pointer is positioned with the
  appropriate FITS file with the grouping table as the CHDU. If the group
  grouping table resides in a file other than the member then an attempt
  is first made to open the file readwrite, and failing that readonly.
 
  Note that it is possible for the GRPIDn/GRPLCn keywords in a member 
  header to be non-continuous, e.g., GRPID1, GRPID2, GRPID5, GRPID6. In 
  such cases, the grpid index value specified in the function call shall
  identify the (grpid)th GRPID value. In the above example, if grpid == 3,
  then the group specified by GRPID5 would be opened.
*/
{
  int i;
  int found;

  long ngroups   = 0;
  long grpExtver = 0;

  char keyword[FLEN_KEYWORD];
  char keyvalue[FLEN_FILENAME];
  char *tkeyvalue;
  char location[FLEN_FILENAME];
  char location1[FLEN_FILENAME];
  char location2[FLEN_FILENAME];
  char comment[FLEN_COMMENT];

  char *url[2];


  if(*status != 0) return(*status);

  do
    {
      /* set the grouping table pointer to NULL for error checking later */

      *gfptr = NULL;

      /*
	make sure that the group ID requested is valid ==> cannot be
	larger than the number of GRPIDn keywords in the member HDU header
      */

      *status = fits_get_num_groups(mfptr,&ngroups,status);

      if(grpid > ngroups)
	{
	  *status = BAD_GROUP_ID;
	  sprintf(comment,
		  "GRPID index %d larger total GRPID keywords %ld (ffgtop)",
		  grpid,ngroups);
	  ffpmsg(comment);
	  continue;
	}

      /*
	find the (grpid)th group that the member HDU belongs to and read
	the value of the GRPID(grpid) keyword; fits_get_num_groups()
	automatically re-enumerates the GRPIDn/GRPLCn keywords to fill in
	any gaps
      */

      sprintf(keyword,"GRPID%d",grpid);

      *status = fits_read_key_lng(mfptr,keyword,&grpExtver,comment,status);

      if(*status != 0) continue;

      /*
	if the value of the GRPIDn keyword is positive then the member is
	in the same FITS file as the grouping table and we only have to
	reopen the current FITS file. Else the member and grouping table
	HDUs reside in different files and another FITS file must be opened
	as specified by the corresponding GRPLCn keyword
	
	The DO WHILE loop only executes once and is used to control the
	file opening logic.
      */

      do
	{
	  if(grpExtver > 0) 
	    {
	      /*
		the member resides in the same file as the grouping
		 table, so just reopen the grouping table file
	      */

	      *status = fits_reopen_file(mfptr,gfptr,status);
	      continue;
	    }

	  else if(grpExtver == 0)
	    {
	      /* a GRPIDn value of zero (0) is undefined */

	      *status = BAD_GROUP_ID;
	      sprintf(comment,"Invalid value of %ld for GRPID%d (ffgtop)",
		      grpExtver,grpid);
	      ffpmsg(comment);
	      continue;
	    }

	  /* 
	     The GRPLCn keyword value is negative, which implies that
	     the grouping table must reside in another FITS file;
	     search for the corresponding GRPLCn keyword 
	  */
	  
	  /* set the grpExtver value positive */
  
	  grpExtver = -1*grpExtver;

	  /* read the GRPLCn keyword value */

	  sprintf(keyword,"GRPLC%d",grpid);
	  /* SPR 1738 */
	  *status = fits_read_key_longstr(mfptr,keyword,&tkeyvalue,comment,
				      status);
	  if (0 == *status) {
	    strcpy(keyvalue,tkeyvalue);
	    free(tkeyvalue);
	  }
	  

	  /* if the GRPLCn keyword was not found then there is a problem */

	  if(*status == KEY_NO_EXIST)
	    {
	      *status = BAD_GROUP_ID;

	      sprintf(comment,"Cannot find GRPLC%d keyword (ffgtop)",
		      grpid);
	      ffpmsg(comment);

	      continue;
	    }

	  prepare_keyvalue(keyvalue);

	  /*
	    if the GRPLCn keyword value specifies an absolute URL then
	    try to open the file; we cannot attempt any relative URL
	    or host-dependent file path reconstruction
	  */

	  if(fits_is_url_absolute(keyvalue))
	    {
	      ffpmsg("Try to open group table file as absolute URL (ffgtop)");

	      *status = fits_open_file(gfptr,keyvalue,READWRITE,status);

	      /* if the open was successful then continue */

	      if(*status == 0) continue;

	      /* if READWRITE failed then try opening it READONLY */

	      ffpmsg("OK, try open group table file as READONLY (ffgtop)");
	      
	      *status = 0;
	      *status = fits_open_file(gfptr,keyvalue,READONLY,status);

	      /* continue regardless of the outcome */

	      continue;
	    }

	  /*
	    see if the URL gives a file path that is absolute on the
	    host machine 
	  */

	  *status = fits_url2path(keyvalue,location1,status);

	  *status = fits_open_file(gfptr,location1,READWRITE,status);

	  /* if the file opened then continue */

	  if(*status == 0) continue;

	  /* if READWRITE failed then try opening it READONLY */

	  ffpmsg("OK, try open group table file as READONLY (ffgtop)");
	  
	  *status = 0;
	  *status = fits_open_file(gfptr,location1,READONLY,status);

	  /* if the file opened then continue */

	  if(*status == 0) continue;

	  /*
	    the grouping table location given by GRPLCn must specify a 
	    relative URL. We assume that this URL is relative to the 
	    member HDU's FITS file. Try to construct a full URL location 
	    for the grouping table's FITS file and then open it
	  */

	  *status = 0;
		  
	  /* retrieve the URL information for the member HDU's file */
		  
	  url[0] = location1; url[1] = location2;
		  
	  *status = fits_get_url(mfptr,url[0],url[1],NULL,NULL,NULL,status);

	  /*
	    It is possible that the member HDU file has an initial
	    URL it was opened with and a real URL that the file actually
	    exists at (e.g., an HTTP accessed file copied to a local
	    file). For each possible URL try to construct a
	  */
		  
	  for(i = 0, found = 0, *gfptr = NULL; i < 2 && !found; ++i)
	    {
	      
	      /* the url string could be empty */
	      
	      if(*url[i] == 0) continue;
	      
	      /* 
		 create a full URL from the partial and the member
		 HDU file URL
	      */
	      
	      *status = fits_relurl2url(url[i],keyvalue,location,status);
	      
	      /* if an error occured then contniue */
	      
	      if(*status != 0) 
		{
		  *status = 0;
		  continue;
		}
	      
	      /*
		if the location does not specify an access method
		then turn it into a host dependent path
	      */

	      if(! fits_is_url_absolute(location))
		{
		  *status = fits_url2path(location,url[i],status);
		  strcpy(location,url[i]);
		}
	      
	      /* try to open the grouping table file READWRITE */
	      
	      *status = fits_open_file(gfptr,location,READWRITE,status);
	      
	      if(*status != 0)
		{    
		  /* try to open the grouping table file READONLY */
		  
		  ffpmsg("opening file as READWRITE failed (ffgtop)");
		  ffpmsg("OK, try to open file as READONLY (ffgtop)");
		  *status = 0;
		  *status = fits_open_file(gfptr,location,READONLY,status);
		}
	      
	      /* either set the found flag or reset the status flag */
	      
	      if(*status == 0) 
		found = 1;
	      else
		*status = 0;
	    }

	}while(0); /* end of file opening loop */

      /* if an error occured with the file opening then exit */

      if(*status != 0) continue;
  
      if(*gfptr == NULL)
	{
	  ffpmsg("Cannot open or find grouping table FITS file (ffgtop)");
	  *status = GROUP_NOT_FOUND;
	  continue;
	}

      /* search for the grouping table in its FITS file */

      *status = fits_movnam_hdu(*gfptr,ANY_HDU,"GROUPING",(int)grpExtver,
				status);

      if(*status != 0) *status = GROUP_NOT_FOUND;

    }while(0);

  if(*status != 0 && *gfptr != NULL) 
    {
      fits_close_file(*gfptr,status);
      *gfptr = NULL;
    }

  return(*status);
}
/*---------------------------------------------------------------------------*/
int ffgtam(fitsfile *gfptr,   /* FITS file pointer to grouping table HDU     */
	   fitsfile *mfptr,   /* FITS file pointer to member HDU             */
	   int       hdupos,  /* member HDU position IF in the same file as
			         the grouping table AND mfptr == NULL        */
	   int      *status)  /* return status code                          */
 
/*
  add a member HDU to an existing grouping table. The fitsfile pointer gfptr
  must be positioned with the grouping table as the CHDU. The member HDU
  may either be identifed with the fitsfile *mfptr (which must be positioned
  to the member HDU) or the hdupos parameter (the HDU number of the member 
  HDU) if both reside in the same FITS file. The hdupos value is only used
  if the mfptr parameter has a value of NULL (0). The new member HDU shall 
  have the appropriate GRPIDn and GRPLCn keywords created in its header.

  Note that if the member HDU to be added to the grouping table is already
  a member of the group then it will not be added a sceond time.
*/

{
  int xtensionCol,extnameCol,extverCol,positionCol,locationCol,uriCol;
  int memberPosition = 0;
  int grptype        = 0;
  int hdutype        = 0;
  int useLocation    = 0;
  int nkeys          = 6;
  int found;
  int i;

  int memberIOstate;
  int groupIOstate;
  int iomode;

  long memberExtver = 0;
  long groupExtver  = 0;
  long memberID     = 0;
  long nmembers     = 0;
  long ngroups      = 0;
  long grpid        = 0;

  char memberAccess1[FLEN_VALUE];
  char memberAccess2[FLEN_VALUE];
  char memberFileName[FLEN_FILENAME];
  char memberLocation[FLEN_FILENAME];
  char grplc[FLEN_FILENAME];
  char *tgrplc;
  char memberHDUtype[FLEN_VALUE];
  char memberExtname[FLEN_VALUE];
  char memberURI[] = "URL";

  char groupAccess1[FLEN_VALUE];
  char groupAccess2[FLEN_VALUE];
  char groupFileName[FLEN_FILENAME];
  char groupLocation[FLEN_FILENAME];
  char tmprootname[FLEN_FILENAME], grootname[FLEN_FILENAME];
  char cwd[FLEN_FILENAME];

  char *keys[] = {"GRPNAME","EXTVER","EXTNAME","TFIELDS","GCOUNT","EXTEND"};
  char *tmpPtr[1];

  char keyword[FLEN_KEYWORD];
  char card[FLEN_CARD];

  unsigned char charNull[]  = {'\0'};

  fitsfile *tmpfptr = NULL;

  int parentStatus = 0;

  if(*status != 0) return(*status);

  do
    {
      /*
	make sure the grouping table can be modified before proceeding
      */

      fits_file_mode(gfptr,&iomode,status);

      if(iomode != READWRITE)
	{
	  ffpmsg("cannot modify grouping table (ffgtam)");
	  *status = BAD_GROUP_ATTACH;
	  continue;
	}

      /*
	 if the calling function supplied the HDU position of the member
	 HDU instead of fitsfile pointer then get a fitsfile pointer
      */

      if(mfptr == NULL)
	{
	  *status = fits_reopen_file(gfptr,&tmpfptr,status);
	  *status = fits_movabs_hdu(tmpfptr,hdupos,&hdutype,status);

	  if(*status != 0) continue;
	}
      else
	tmpfptr = mfptr;

      /*
	 determine all the information about the member HDU that will
	 be needed later; note that we establish the default values for
	 all information values that are not explicitly found
      */

      *status = fits_read_key_str(tmpfptr,"XTENSION",memberHDUtype,card,
				  status);

      if(*status == KEY_NO_EXIST) 
	{
	  strcpy(memberHDUtype,"PRIMARY");
	  *status = 0;
	}
      prepare_keyvalue(memberHDUtype);

      *status = fits_read_key_lng(tmpfptr,"EXTVER",&memberExtver,card,status);

      if(*status == KEY_NO_EXIST) 
	{
	  memberExtver = 1;
	  *status      = 0;
	}

      *status = fits_read_key_str(tmpfptr,"EXTNAME",memberExtname,card,
				  status);

      if(*status == KEY_NO_EXIST) 
	{
	  memberExtname[0] = 0;
	  *status          = 0;
	}
      prepare_keyvalue(memberExtname);

      fits_get_hdu_num(tmpfptr,&memberPosition);

      /*
	Determine if the member HDU's FITS file location needs to be
	taken into account when building its grouping table reference

	If the member location needs to be used (==> grouping table and member
	HDU reside in different files) then create an appropriate URL for
	the member HDU's file and grouping table's file. Note that the logic
	for this is rather complicated
      */

      /* SPR 3463, don't do this 
	 if(tmpfptr->Fptr == gfptr->Fptr)
	 {  */
	  /*
	    member HDU and grouping table reside in the same file, no need
	    to use the location information */
	  
      /* printf ("same file\n");
	   
	   useLocation     = 0;
	   memberIOstate   = 1;
	   *memberFileName = 0;
	}
      else
      { */ 
	  /*
	     the member HDU and grouping table FITS file location information 
	     must be used.

	     First determine the correct driver and file name for the group
	     table and member HDU files. If either are disk files then
	     construct an absolute file path for them. Finally, if both are
	     disk files construct relative file paths from the group(member)
	     file to the member(group) file.

	  */

	  /* set the USELOCATION flag to true */

	  useLocation = 1;

	  /* 
	     get the location, access type and iostate (RO, RW) of the
	     member HDU file
	  */

	  *status = fits_get_url(tmpfptr,memberFileName,memberLocation,
				 memberAccess1,memberAccess2,&memberIOstate,
				 status);

	  /*
	     if the memberFileName string is empty then use the values of
	     the memberLocation string. This corresponds to a file where
	     the "real" file is a temporary memory file, and we must assume
	     the the application really wants the original file to be the
	     group member
	   */

	  if(strlen(memberFileName) == 0)
	    {
	      strcpy(memberFileName,memberLocation);
	      strcpy(memberAccess1,memberAccess2);
	    }

	  /* 
	     get the location, access type and iostate (RO, RW) of the
	     grouping table file
	  */

	  *status = fits_get_url(gfptr,groupFileName,groupLocation,
				 groupAccess1,groupAccess2,&groupIOstate,
				 status);
	  
	  if(*status != 0) continue;

	  /*
	    the grouping table file must be writable to continue
	  */

	  if(groupIOstate == 0)
	    {
	      ffpmsg("cannot modify grouping table (ffgtam)");
	      *status = BAD_GROUP_ATTACH;
	      continue;
	    }

	  /*
	    determine how to construct the resulting URLs for the member and
	    group files
	  */

	  if(fits_strcasecmp(groupAccess1,"file://")  &&
	                                   fits_strcasecmp(memberAccess1,"file://"))
	    {
              *cwd = 0;
	      /* 
		 nothing to do in this case; both the member and group files
		 must be of an access type that already gives valid URLs;
		 i.e., URLs that we can pass directly to the file drivers
	      */
	    }
	  else
	    {
	      /*
		 retrieve the Current Working Directory as a Unix-like
		 URL standard string
	      */

	      *status = fits_get_cwd(cwd,status);

	      /*
		 create full file path for the member HDU FITS file URL
		 if it is of access type file://
	      */
	      
	      if(fits_strcasecmp(memberAccess1,"file://") == 0)
		{
		  if(*memberFileName == '/')
		    {
		      strcpy(memberLocation,memberFileName);
		    }
		  else
		    {
		      strcpy(memberLocation,cwd);
		      strcat(memberLocation,"/");
		      strcat(memberLocation,memberFileName);
		    }
		  
		  *status = fits_clean_url(memberLocation,memberFileName,
					   status);
		}

	      /*
		 create full file path for the grouping table HDU FITS file URL
		 if it is of access type file://
	      */

	      if(fits_strcasecmp(groupAccess1,"file://") == 0)
		{
		  if(*groupFileName == '/')
		    {
		      strcpy(groupLocation,groupFileName);
		    }
		  else
		    {
		      strcpy(groupLocation,cwd);
		      strcat(groupLocation,"/");
		      strcat(groupLocation,groupFileName);
		    }
		  
		  *status = fits_clean_url(groupLocation,groupFileName,status);
		}

	      /*
		if both the member and group files are disk files then 
		create a relative path (relative URL) strings with 
		respect to the grouping table's file and the grouping table's 
		file with respect to the member HDU's file
	      */
	      
	      if(fits_strcasecmp(groupAccess1,"file://") == 0 &&
		                      fits_strcasecmp(memberAccess1,"file://") == 0)
		{
		  fits_url2relurl(memberFileName,groupFileName,
				                  groupLocation,status);
		  fits_url2relurl(groupFileName,memberFileName,
				                  memberLocation,status);

		  /*
		     copy the resulting partial URL strings to the
		     memberFileName and groupFileName variables for latter
		     use in the function
		   */
		    
		  strcpy(memberFileName,memberLocation);
		  strcpy(groupFileName,groupLocation);		  
		}
	    }
	  /* beo done */
	  /* }  */
      

      /* retrieve the grouping table's EXTVER value */

      *status = fits_read_key_lng(gfptr,"EXTVER",&groupExtver,card,status);

      /* 
	 if useLocation is true then make the group EXTVER value negative
	 for the subsequent GRPIDn/GRPLCn matching
      */
      /* SPR 3463 change test;  WDP added test for same filename */
      /* Now, if either the Fptr values are the same, or the root filenames
         are the same, then assume these refer to the same file.
      */
      fits_parse_rootname(tmpfptr->Fptr->filename, tmprootname, status);
      fits_parse_rootname(gfptr->Fptr->filename, grootname, status);

      if((tmpfptr->Fptr != gfptr->Fptr) && 
          strncmp(tmprootname, grootname, FLEN_FILENAME))
	   groupExtver = -1*groupExtver;

      /* retrieve the number of group members */

      *status = fits_get_num_members(gfptr,&nmembers,status);
	      
    do {

      /*
	 make sure the member HDU is not already an entry in the
	 grouping table before adding it
      */

      *status = ffgmf(gfptr,memberHDUtype,memberExtname,memberExtver,
		      memberPosition,memberFileName,&memberID,status);

      if(*status == MEMBER_NOT_FOUND) *status = 0;
      else if(*status == 0)
	{  
	  parentStatus = HDU_ALREADY_MEMBER;
    ffpmsg("Specified HDU is already a member of the Grouping table (ffgtam)");
	  continue;
	}
      else continue;

      /*
	 if the member HDU is not already recorded in the grouping table
	 then add it 
      */

      /* add a new row to the grouping table */

      *status = fits_insert_rows(gfptr,nmembers,1,status);
      ++nmembers;

      /* retrieve the grouping table column IDs and structure type */

      *status = ffgtgc(gfptr,&xtensionCol,&extnameCol,&extverCol,&positionCol,
		       &locationCol,&uriCol,&grptype,status);

      /* fill in the member HDU data in the new grouping table row */

      *tmpPtr = memberHDUtype; 

      if(xtensionCol != 0)
	fits_write_col_str(gfptr,xtensionCol,nmembers,1,1,tmpPtr,status);

      *tmpPtr = memberExtname; 

      if(extnameCol  != 0)
	{
	  if(strlen(memberExtname) != 0)
	    fits_write_col_str(gfptr,extnameCol,nmembers,1,1,tmpPtr,status);
	  else
	    /* WILL THIS WORK FOR VAR LENTH CHAR COLS??????*/
	    fits_write_col_byt(gfptr,extnameCol,nmembers,1,1,charNull,status);
	}

      if(extverCol   != 0)
	fits_write_col_lng(gfptr,extverCol,nmembers,1,1,&memberExtver,
			   status);

      if(positionCol != 0)
	fits_write_col_int(gfptr,positionCol,nmembers,1,1,
			   &memberPosition,status);

      *tmpPtr = memberFileName; 

      if(locationCol != 0)
	{
	  /* Change the test for SPR 3463 */
	  /* Now, if either the Fptr values are the same, or the root filenames
	     are the same, then assume these refer to the same file.
	  */
	  fits_parse_rootname(tmpfptr->Fptr->filename, tmprootname, status);
	  fits_parse_rootname(gfptr->Fptr->filename, grootname, status);

	  if((tmpfptr->Fptr != gfptr->Fptr) && 
	          strncmp(tmprootname, grootname, FLEN_FILENAME))
	    fits_write_col_str(gfptr,locationCol,nmembers,1,1,tmpPtr,status);
	  else
	    /* WILL THIS WORK FOR VAR LENTH CHAR COLS??????*/
	    fits_write_col_byt(gfptr,locationCol,nmembers,1,1,charNull,status);
	}

      *tmpPtr = memberURI;

      if(uriCol      != 0)
	{

	  /* Change the test for SPR 3463 */
	  /* Now, if either the Fptr values are the same, or the root filenames
	     are the same, then assume these refer to the same file.
	  */
	  fits_parse_rootname(tmpfptr->Fptr->filename, tmprootname, status);
	  fits_parse_rootname(gfptr->Fptr->filename, grootname, status);

	  if((tmpfptr->Fptr != gfptr->Fptr) && 
	          strncmp(tmprootname, grootname, FLEN_FILENAME))
	    fits_write_col_str(gfptr,uriCol,nmembers,1,1,tmpPtr,status);
	  else
	    /* WILL THIS WORK FOR VAR LENTH CHAR COLS??????*/
	    fits_write_col_byt(gfptr,uriCol,nmembers,1,1,charNull,status);
	}
    } while(0);

      if(0 != *status) continue;
      /*
	 add GRPIDn/GRPLCn keywords to the member HDU header to link
	 it to the grouing table if the they do not already exist and
	 the member file is RW
      */

      fits_file_mode(tmpfptr,&iomode,status);
 
     if(memberIOstate == 0 || iomode != READWRITE) 
	{
	  ffpmsg("cannot add GRPID/LC keywords to member HDU: (ffgtam)");
	  ffpmsg(memberFileName);
	  continue;
	}

      *status = fits_get_num_groups(tmpfptr,&ngroups,status);

      /* 
	 look for the GRPID/LC keywords in the member HDU; if the keywords
	 for the back-link to the grouping table already exist then no
	 need to add them again
       */

      for(i = 1, found = 0; i <= ngroups && !found && *status == 0; ++i)
	{
	  sprintf(keyword,"GRPID%d",(int)ngroups);
	  *status = fits_read_key_lng(tmpfptr,keyword,&grpid,card,status);

	  if(grpid == groupExtver)
	    {
	      if(grpid < 0)
		{

		  /* have to make sure the GRPLCn keyword matches too */

		  sprintf(keyword,"GRPLC%d",(int)ngroups);
		  /* SPR 1738 */
		  *status = fits_read_key_longstr(mfptr,keyword,&tgrplc,card,
						  status);
		  if (0 == *status) {
		    strcpy(grplc,tgrplc);
		    free(tgrplc);
		  }
		  
		  /*
		     always compare files using absolute paths
                     the presence of a non-empty cwd indicates
                     that the file names may require conversion
                     to absolute paths
                  */

                  if(0 < strlen(cwd)) {
                    /* temp buffer for use in assembling abs. path(s) */
                    char tmp[FLEN_FILENAME];

                    /* make grplc absolute if necessary */
                    if(!fits_is_url_absolute(grplc)) {
		      fits_path2url(grplc,groupLocation,status);

		      if(groupLocation[0] != '/')
			{
			  strcpy(tmp, cwd);
			  strcat(tmp,"/");
			  strcat(tmp,groupLocation);
			  fits_clean_url(tmp,grplc,status);
			}
                    }

                    /* make groupFileName absolute if necessary */
                    if(!fits_is_url_absolute(groupFileName)) {
		      fits_path2url(groupFileName,groupLocation,status);

		      if(groupLocation[0] != '/')
			{
			  strcpy(tmp, cwd);
			  strcat(tmp,"/");
			  strcat(tmp,groupLocation);
                          /*
                             note: use groupLocation (which is not used
                             below this block), to store the absolute
                             file name instead of using groupFileName.
                             The latter may be needed unaltered if the
                             GRPLC is written below
                          */

			  fits_clean_url(tmp,groupLocation,status);
			}
                    }
                  }
		  /*
		    see if the grplc value and the group file name match
		  */

		  if(strcmp(grplc,groupLocation) == 0) found = 1;
		}
	      else
		{
		  /* the match is found with GRPIDn alone */
		  found = 1;
		}
	    }
	}

      /*
	 if FOUND is true then no need to continue
      */

      if(found)
	{
	  ffpmsg("HDU already has GRPID/LC keywords for group table (ffgtam)");
	  continue;
	}

      /*
	 add the GRPID/LC keywords to the member header for this grouping
	 table
	 
	 If NGROUPS == 0 then we must position the header pointer to the
	 record where we want to insert the GRPID/LC keywords (the pointer
	 is already correctly positioned if the above search loop activiated)
      */

      if(ngroups == 0)
	{
	  /* 
	     no GRPIDn/GRPLCn keywords currently exist in header so try
	     to position the header pointer to a desirable position
	  */
	  
	  for(i = 0, *status = KEY_NO_EXIST; 
	                       i < nkeys && *status == KEY_NO_EXIST; ++i)
	    {
	      *status = 0;
	      *status = fits_read_card(tmpfptr,keys[i],card,status);
	    }
	      
	  /* all else fails: move write pointer to end of header */
	      
	  if(*status == KEY_NO_EXIST)
	    {
	      *status = 0;
	      fits_get_hdrspace(tmpfptr,&nkeys,&i,status);
	      ffgrec(tmpfptr,nkeys,card,status);
	    }
	  
	  /* any other error status then abort */
	  
	  if(*status != 0) continue;
	}
      
      /* 
	 now that the header pointer is positioned for the GRPID/LC 
	 keyword insertion increment the number of group links counter for 
	 the member HDU 
      */

      ++ngroups;

      /*
	 if the member HDU and grouping table reside in the same FITS file
	 then there is no need to add a GRPLCn keyword
      */
      /* SPR 3463 change test */
      /* Now, if either the Fptr values are the same, or the root filenames
	 are the same, then assume these refer to the same file.
      */
      fits_parse_rootname(tmpfptr->Fptr->filename, tmprootname, status);
      fits_parse_rootname(gfptr->Fptr->filename, grootname, status);

      if((tmpfptr->Fptr == gfptr->Fptr) || 
	          strncmp(tmprootname, grootname, FLEN_FILENAME) == 0)
	{
	  /* add the GRPIDn keyword only */

	  sprintf(keyword,"GRPID%d",(int)ngroups);
	  fits_insert_key_lng(tmpfptr,keyword,groupExtver,
			      "EXTVER of Group containing this HDU",status);
	}
      else 
	{
	  /* add the GRPIDn and GRPLCn keywords */

	  sprintf(keyword,"GRPID%d",(int)ngroups);
	  fits_insert_key_lng(tmpfptr,keyword,groupExtver,
			      "EXTVER of Group containing this HDU",status);

	  sprintf(keyword,"GRPLC%d",(int)ngroups);
	  /* SPR 1738 */
	  fits_insert_key_longstr(tmpfptr,keyword,groupFileName,
			      "URL of file containing Group",status);
	  fits_write_key_longwarn(tmpfptr,status);

	}

    }while(0);

  /* close the tmpfptr pointer if it was opened in this function */

  if(mfptr == NULL)
    {
      *status = fits_close_file(tmpfptr,status);
    }

  *status = 0 == *status ? parentStatus : *status;

  return(*status);
}

/*---------------------------------------------------------------------------*/
int ffgtnm(fitsfile *gfptr,    /* FITS file pointer to grouping table        */
	   long     *nmembers, /* member count  of the groping table         */
	   int      *status)   /* return status code                         */

/*
  return the number of member HDUs in a grouping table. The fitsfile pointer
  gfptr must be positioned with the grouping table as the CHDU. The number
  of grouping table member HDUs is just the NAXIS2 value of the grouping
  table.
*/

{
  char keyvalue[FLEN_VALUE];
  char comment[FLEN_COMMENT];
  

  if(*status != 0) return(*status);

  *status = fits_read_keyword(gfptr,"EXTNAME",keyvalue,comment,status);
  
  if(*status == KEY_NO_EXIST)
    *status = NOT_GROUP_TABLE;
  else
    {
      prepare_keyvalue(keyvalue);

      if(fits_strcasecmp(keyvalue,"GROUPING") != 0)
	{
	  *status = NOT_GROUP_TABLE;
	  ffpmsg("Specified HDU is not a Grouping table (ffgtnm)");
	}

      *status = fits_read_key_lng(gfptr,"NAXIS2",nmembers,comment,status);
    }

  return(*status);
}

/*--------------------------------------------------------------------------*/
int ffgmng(fitsfile *mfptr,   /* FITS file pointer to member HDU            */
	   long     *ngroups, /* total number of groups linked to HDU       */
	   int      *status)  /* return status code                         */

/*
  return the number of groups to which a HDU belongs, as defined by the number
  of GRPIDn/GRPLCn keyword records that appear in the HDU header. The 
  fitsfile pointer mfptr must be positioned with the member HDU as the CHDU. 
  Each time this function is called, the indicies of the GRPIDn/GRPLCn
  keywords are checked to make sure they are continuous (ie no gaps) and
  are re-enumerated to eliminate gaps if gaps are found to be present.
*/

{
  int offset;
  int index;
  int newIndex;
  int i;
  
  long grpid;

  char *inclist[] = {"GRPID#"};
  char keyword[FLEN_KEYWORD];
  char newKeyword[FLEN_KEYWORD];
  char card[FLEN_CARD];
  char comment[FLEN_COMMENT];
  char *tkeyvalue;

  if(*status != 0) return(*status);

  *ngroups = 0;

  /* reset the member HDU keyword counter to the beginning */

  *status = ffgrec(mfptr,0,card,status);
  
  /*
    search for the number of GRPIDn keywords in the member HDU header
    and count them with the ngroups variable
  */
  
  while(*status == 0)
    {
      /* read the next GRPIDn keyword in the series */

      *status = fits_find_nextkey(mfptr,inclist,1,NULL,0,card,status);
      
      if(*status != 0) continue;
      
      ++(*ngroups);
    }

  if(*status == KEY_NO_EXIST) *status = 0;
      
  /*
     read each GRPIDn/GRPLCn keyword and adjust their index values so that
     there are no gaps in the index count
  */

  for(index = 1, offset = 0, i = 1; i <= *ngroups && *status == 0; ++index)
    {	  
      sprintf(keyword,"GRPID%d",index);

      /* try to read the next GRPIDn keyword in the series */

      *status = fits_read_key_lng(mfptr,keyword,&grpid,card,status);

      /* if not found then increment the offset counter and continue */

      if(*status == KEY_NO_EXIST) 
	{
	  *status = 0;
	  ++offset;
	}
      else
	{
	  /* 
	     increment the number_keys_found counter and see if the index
	     of the keyword needs to be updated
	  */

	  ++i;

	  if(offset > 0)
	    {
	      /* compute the new index for the GRPIDn/GRPLCn keywords */
	      newIndex = index - offset;

	      /* update the GRPIDn keyword index */

	      sprintf(newKeyword,"GRPID%d",newIndex);
	      fits_modify_name(mfptr,keyword,newKeyword,status);

	      /* If present, update the GRPLCn keyword index */

	      sprintf(keyword,"GRPLC%d",index);
	      sprintf(newKeyword,"GRPLC%d",newIndex);
	      /* SPR 1738 */
	      *status = fits_read_key_longstr(mfptr,keyword,&tkeyvalue,comment,
					      status);
	      if (0 == *status) {
		fits_delete_key(mfptr,keyword,status);
		fits_insert_key_longstr(mfptr,newKeyword,tkeyvalue,comment,status);
		fits_write_key_longwarn(mfptr,status);
		free(tkeyvalue);
	      }
	      

	      if(*status == KEY_NO_EXIST) *status = 0;
	    }
	}
    }

  return(*status);
}

/*---------------------------------------------------------------------------*/
int ffgmop(fitsfile *gfptr,  /* FITS file pointer to grouping table          */
	   long      member, /* member ID (row num) within grouping table    */
	   fitsfile **mfptr, /* FITS file pointer to member HDU              */
	   int      *status) /* return status code                           */

/*
  open a grouping table member, returning a pointer to the member's FITS file
  with the CHDU set to the member HDU. The grouping table must be the CHDU of
  the FITS file pointed to by gfptr. The member to open is identified by its
  row number within the grouping table (first row/member == 1).

  If the member resides in a FITS file different from the grouping
  table the member file is first opened readwrite and if this fails then
  it is opened readonly. For access type of FILE:// the member file is
  searched for assuming (1) an absolute path is given, (2) a path relative
  to the CWD is given, and (3) a path relative to the grouping table file
  but not relative to the CWD is given. If all of these fail then the
  error FILE_NOT_FOUND is returned.
*/

{
  int xtensionCol,extnameCol,extverCol,positionCol,locationCol,uriCol;
  int grptype,hdutype;
  int dummy;

  long hdupos = 0;
  long extver = 0;

  char  xtension[FLEN_VALUE];
  char  extname[FLEN_VALUE];
  char  uri[FLEN_VALUE];
  char  grpLocation1[FLEN_FILENAME];
  char  grpLocation2[FLEN_FILENAME];
  char  mbrLocation1[FLEN_FILENAME];
  char  mbrLocation2[FLEN_FILENAME];
  char  mbrLocation3[FLEN_FILENAME];
  char  cwd[FLEN_FILENAME];
  char  card[FLEN_CARD];
  char  nstr[] = {'\0'};
  char *tmpPtr[1];


  if(*status != 0) return(*status);

  do
    {
      /*
	retrieve the Grouping Convention reserved column positions within
	the grouping table
      */

      *status = ffgtgc(gfptr,&xtensionCol,&extnameCol,&extverCol,&positionCol,
		       &locationCol,&uriCol,&grptype,status);

      if(*status != 0) continue;

      /*
	 extract the member information from grouping table
      */

      tmpPtr[0] = xtension;

      if(xtensionCol != 0)
	{

	  *status = fits_read_col_str(gfptr,xtensionCol,member,1,1,nstr,
				      tmpPtr,&dummy,status);

	  /* convert the xtension string to a hdutype code */

	  if(fits_strcasecmp(xtension,"PRIMARY")       == 0) hdutype = IMAGE_HDU; 
	  else if(fits_strcasecmp(xtension,"IMAGE")    == 0) hdutype = IMAGE_HDU; 
	  else if(fits_strcasecmp(xtension,"TABLE")    == 0) hdutype = ASCII_TBL; 
	  else if(fits_strcasecmp(xtension,"BINTABLE") == 0) hdutype = BINARY_TBL; 
	  else hdutype = ANY_HDU; 
	}

      tmpPtr[0] = extname;

      if(extnameCol  != 0)
	  *status = fits_read_col_str(gfptr,extnameCol,member,1,1,nstr,
				      tmpPtr,&dummy,status);

      if(extverCol   != 0)
	  *status = fits_read_col_lng(gfptr,extverCol,member,1,1,0,
				      (long*)&extver,&dummy,status);

      if(positionCol != 0)
	  *status = fits_read_col_lng(gfptr,positionCol,member,1,1,0,
				      (long*)&hdupos,&dummy,status);

      tmpPtr[0] = mbrLocation1;

      if(locationCol != 0)
	*status = fits_read_col_str(gfptr,locationCol,member,1,1,nstr,
				    tmpPtr,&dummy,status);
      tmpPtr[0] = uri;

      if(uriCol != 0)
	*status = fits_read_col_str(gfptr,uriCol,member,1,1,nstr,
				    tmpPtr,&dummy,status);

      if(*status != 0) continue;

      /* 
	 decide what FITS file the member HDU resides in and open the file
	 using the fitsfile* pointer mfptr; note that this logic is rather
	 complicated and is based primiarly upon if a URL specifier is given
	 for the member file in the grouping table
      */

      switch(grptype)
	{

	case GT_ID_POS:
	case GT_ID_REF:
	case GT_ID_ALL:

	  /*
	     no location information is given so we must assume that the
	     member HDU resides in the same FITS file as the grouping table;
	     if the grouping table was incorrectly constructed then this
	     assumption will be false, but there is nothing to be done about
	     it at this point
	  */

	  *status = fits_reopen_file(gfptr,mfptr,status);
	  
	  break;

	case GT_ID_REF_URI:
	case GT_ID_POS_URI:
	case GT_ID_ALL_URI:

	  /*
	    The member location column exists. Determine if the member 
	    resides in the same file as the grouping table or in a
	    separate file; open the member file in either case
	  */

	  if(strlen(mbrLocation1) == 0)
	    {
	      /*
		 since no location information was given we must assume
		 that the member is in the same FITS file as the grouping
		 table
	      */

	      *status = fits_reopen_file(gfptr,mfptr,status);
	    }
	  else
	    {
	      /*
		make sure the location specifiation is "URL"; we cannot
		decode any other URI types at this time
	      */

	      if(fits_strcasecmp(uri,"URL") != 0)
		{
		  *status = FILE_NOT_OPENED;
		  sprintf(card,
		  "Cannot open member HDU file with URI type %s (ffgmop)",
			  uri);
		  ffpmsg(card);

		  continue;
		}

	      /*
		The location string for the member is not NULL, so it 
		does not necessially reside in the same FITS file as the
		grouping table. 

		Three cases are attempted for opening the member's file
		in the following order:

		1. The URL given for the member's file is absolute (i.e.,
		access method supplied); try to open the member

		2. The URL given for the member's file is not absolute but
		is an absolute file path; try to open the member as a file
		after the file path is converted to a host-dependent form

		3. The URL given for the member's file is not absolute
	        and is given as a relative path to the location of the 
		grouping table's file. Create an absolute URL using the 
		grouping table's file URL and try to open the member.
		
		If all three cases fail then an error is returned. In each
		case the file is first opened in read/write mode and failing
		that readonly mode.
		
		The following DO loop is only used as a mechanism to break
		(continue) when the proper file opening method is found
	       */

	      do
		{
		  /*
		     CASE 1:

		     See if the member URL is absolute (i.e., includes a
		     access directive) and if so open the file
		   */

		  if(fits_is_url_absolute(mbrLocation1))
		    {
		      /*
			 the URL must specify an access method, which 
			 implies that its an absolute reference
			 
			 regardless of the access method, pass the whole
			 URL to the open function for processing
		       */
		      
		      ffpmsg("member URL is absolute, try open R/W (ffgmop)");

		      *status = fits_open_file(mfptr,mbrLocation1,READWRITE,
					       status);

		      if(*status == 0) continue;

		      *status = 0;

		      /* 
			 now try to open file using full URL specs in 
			 readonly mode 
		      */ 

		      ffpmsg("OK, now try to open read-only (ffgmop)");

		      *status = fits_open_file(mfptr,mbrLocation1,READONLY,
					       status);

		      /* break from DO loop regardless of status */

		      continue;
		    }

		  /*
		     CASE 2:

		     If we got this far then the member URL location 
		     has no access type ==> FILE:// Try to open the member 
		     file using the URL as is, i.e., assume that it is given 
		     as absolute, if it starts with a '/' character
		   */

		  ffpmsg("Member URL is of type FILE (ffgmop)");

		  if(*mbrLocation1 == '/')
		    {
		      ffpmsg("Member URL specifies abs file path (ffgmop)");

		      /* 
			 convert the URL path to a host dependent path
		      */

		      *status = fits_url2path(mbrLocation1,mbrLocation2,
					      status);

		      ffpmsg("Try to open member URL in R/W mode (ffgmop)");

		      *status = fits_open_file(mfptr,mbrLocation2,READWRITE,
					       status);

		      if(*status == 0) continue;

		      *status = 0;

		      /* 
			 now try to open file using the URL as an absolute 
			 path in readonly mode 
		      */
 
		      ffpmsg("OK, now try to open read-only (ffgmop)");

		      *status = fits_open_file(mfptr,mbrLocation2,READONLY,
					       status);

		      /* break from the Do loop regardless of the status */

		      continue;
		    }
		  
		  /* 
		     CASE 3:

		     If we got this far then the URL does not specify an
		     absoulte file path or URL with access method. Since 
		     the path to the group table's file is (obviously) valid 
		     for the CWD, create a full location string for the
		     member HDU using the grouping table URL as a basis

		     The only problem is that the grouping table file might
		     have two URLs, the original one used to open it and
		     the one that points to the real file being accessed
		     (i.e., a file accessed via HTTP but transferred to a
		     local disk file). Have to attempt to build a URL to
		     the member HDU file using both of these URLs if
		     defined.
		  */

		  ffpmsg("Try to open member file as relative URL (ffgmop)");

		  /* get the URL information for the grouping table file */

		  *status = fits_get_url(gfptr,grpLocation1,grpLocation2,
					 NULL,NULL,NULL,status);

		  /* 
		     if the "real" grouping table file URL is defined then
		     build a full url for the member HDU file using it
		     and try to open the member HDU file
		  */

		  if(*grpLocation1)
		    {
		      /* make sure the group location is absolute */

		      if(! fits_is_url_absolute(grpLocation1) &&
			                              *grpLocation1 != '/')
			{
			  fits_get_cwd(cwd,status);
			  strcat(cwd,"/");
			  strcat(cwd,grpLocation1);
			  strcpy(grpLocation1,cwd);
			}

		      /* create a full URL for the member HDU file */

		      *status = fits_relurl2url(grpLocation1,mbrLocation1,
						mbrLocation2,status);

		      if(*status != 0) continue;

		      /*
			if the URL does not have an access method given then
			translate it into a host dependent file path
		      */

		      if(! fits_is_url_absolute(mbrLocation2))
			{
			  *status = fits_url2path(mbrLocation2,mbrLocation3,
						  status);
			  strcpy(mbrLocation2,mbrLocation3);
			}

		      /* try to open the member file READWRITE */

		      *status = fits_open_file(mfptr,mbrLocation2,READWRITE,
					       status);

		      if(*status == 0) continue;

		      *status = 0;
		  
		      /* now try to open in readonly mode */ 

		      ffpmsg("now try to open file as READONLY (ffgmop)");

		      *status = fits_open_file(mfptr,mbrLocation2,READONLY,
					       status);

		      if(*status == 0) continue;

		      *status = 0;
		    }

		  /* 
		     if we got this far then either the "real" grouping table
		     file URL was not defined or all attempts to open the
		     resulting member HDU file URL failed.

		     if the "original" grouping table file URL is defined then
		     build a full url for the member HDU file using it
		     and try to open the member HDU file
		  */

		  if(*grpLocation2)
		    {
		      /* make sure the group location is absolute */

		      if(! fits_is_url_absolute(grpLocation2) &&
			                              *grpLocation2 != '/')
			{
			  fits_get_cwd(cwd,status);
			  strcat(cwd,"/");
			  strcat(cwd,grpLocation2);
			  strcpy(grpLocation2,cwd);
			}

		      /* create an absolute URL for the member HDU file */

		      *status = fits_relurl2url(grpLocation2,mbrLocation1,
						mbrLocation2,status);
		      if(*status != 0) continue;

		      /*
			if the URL does not have an access method given then
			translate it into a host dependent file path
		      */

		      if(! fits_is_url_absolute(mbrLocation2))
			{
			  *status = fits_url2path(mbrLocation2,mbrLocation3,
						  status);
			  strcpy(mbrLocation2,mbrLocation3);
			}

		      /* try to open the member file READWRITE */

		      *status = fits_open_file(mfptr,mbrLocation2,READWRITE,
					       status);

		      if(*status == 0) continue;

		      *status = 0;
		  
		      /* now try to open in readonly mode */ 

		      ffpmsg("now try to open file as READONLY (ffgmop)");

		      *status = fits_open_file(mfptr,mbrLocation2,READONLY,
					       status);

		      if(*status == 0) continue;

		      *status = 0;
		    }

		  /*
		     if we got this far then the member HDU file could not
		     be opened using any method. Log the error.
		  */

		  ffpmsg("Cannot open member HDU FITS file (ffgmop)");
		  *status = MEMBER_NOT_FOUND;
		  
		}while(0);
	    }

	  break;

	default:

	  /* no default action */
	  
	  break;
	}
	  
      if(*status != 0) continue;

      /*
	 attempt to locate the member HDU within its FITS file as determined
	 and opened above
      */

      switch(grptype)
	{

	case GT_ID_POS:
	case GT_ID_POS_URI:

	  /*
	    try to find the member hdu in the the FITS file pointed to
	    by mfptr based upon its HDU posistion value. Note that is 
	    impossible to verify if the HDU is actually the correct HDU due 
	    to a lack of information.
	  */
	  
	  *status = fits_movabs_hdu(*mfptr,(int)hdupos,&hdutype,status);

	  break;

	case GT_ID_REF:
	case GT_ID_REF_URI:

	  /*
	     try to find the member hdu in the FITS file pointed to
	     by mfptr based upon its XTENSION, EXTNAME and EXTVER keyword 
	     values
	  */

	  *status = fits_movnam_hdu(*mfptr,hdutype,extname,extver,status);

	  if(*status == BAD_HDU_NUM) 
	    {
	      *status = MEMBER_NOT_FOUND;
	      ffpmsg("Cannot find specified member HDU (ffgmop)");
	    }

	  /*
	     if the above function returned without error then the
	     mfptr is pointed to the member HDU
	  */

	  break;

	case GT_ID_ALL:
	case GT_ID_ALL_URI:

	  /*
	     if the member entry has reference information then use it
             (ID by reference is safer than ID by position) else use
	     the position information
	  */

	  if(strlen(xtension) > 0 && strlen(extname) > 0 && extver > 0)
	    {
	      /* valid reference info exists so use it */
	      
	      /* try to find the member hdu in the grouping table's file */

	      *status = fits_movnam_hdu(*mfptr,hdutype,extname,extver,status);

	      if(*status == BAD_HDU_NUM) 
		{
		  *status = MEMBER_NOT_FOUND;
		  ffpmsg("Cannot find specified member HDU (ffgmop)");
		}
	    }
	  else
	      {
		  *status = fits_movabs_hdu(*mfptr,(int)hdupos,&hdutype,
					    status);
		  if(*status == END_OF_FILE) *status = MEMBER_NOT_FOUND;
	      }

	  /*
	     if the above function returned without error then the
	     mfptr is pointed to the member HDU
	  */

	  break;

	default:

	  /* no default action */

	  break;
	}
      
    }while(0);

  if(*status != 0 && *mfptr != NULL) 
    {
      fits_close_file(*mfptr,status);
    }

  return(*status);
}

/*---------------------------------------------------------------------------*/
int ffgmcp(fitsfile *gfptr,  /* FITS file pointer to group                   */
	   fitsfile *mfptr,  /* FITS file pointer to new member
				FITS file                                    */
	   long      member, /* member ID (row num) within grouping table    */
	   int       cpopt,  /* code specifying copy options:
				OPT_MCP_ADD  (0) ==> add copied member to the
 				                     grouping table
				OPT_MCP_NADD (1) ==> do not add member copy to
				                     the grouping table
				OPT_MCP_REPL (2) ==> replace current member
				                     entry with member copy  */
	   int      *status) /* return status code                           */
	   
/*
  copy a member HDU of a grouping table to a new FITS file. The grouping table
  must be the CHDU of the FITS file pointed to by gfptr. The copy of the
  group member shall be appended to the end of the FITS file pointed to by
  mfptr. If the cpopt parameter is set to OPT_MCP_ADD then the copy of the 
  member is added to the grouping table as a new member, if OPT_MCP_NADD 
  then the copied member is not added to the grouping table, and if 
  OPT_MCP_REPL then the copied member is used to replace the original member.
  The copied member HDU also has its EXTVER value updated so that its
  combination of XTENSION, EXTNAME and EXVTER is unique within its new
  FITS file.
*/

{
  int numkeys = 0;
  int keypos  = 0;
  int hdunum  = 0;
  int hdutype = 0;
  int i;
  
  char *incList[] = {"GRPID#","GRPLC#"};
  char  extname[FLEN_VALUE];
  char  card[FLEN_CARD];
  char  comment[FLEN_COMMENT];
  char  keyname[FLEN_CARD];
  char  value[FLEN_CARD];

  fitsfile *tmpfptr = NULL;


  if(*status != 0) return(*status);

  do
    {
      /* open the member HDU to be copied */

      *status = fits_open_member(gfptr,member,&tmpfptr,status);

      if(*status != 0) continue;

      /*
	if the member is a grouping table then copy it with a call to
	fits_copy_group() using the "copy only the grouping table" option

	if it is not a grouping table then copy the hdu with fits_copy_hdu()
	remove all GRPIDn and GRPLCn keywords, and update the EXTVER keyword
	value
      */

      /* get the member HDU's EXTNAME value */

      *status = fits_read_key_str(tmpfptr,"EXTNAME",extname,comment,status);

      /* if no EXTNAME value was found then set the extname to a null string */

      if(*status == KEY_NO_EXIST) 
	{
	  extname[0] = 0;
	  *status    = 0;
	}
      else if(*status != 0) continue;

      prepare_keyvalue(extname);

      /* if a grouping table then copy with fits_copy_group() */

      if(fits_strcasecmp(extname,"GROUPING") == 0)
	*status = fits_copy_group(tmpfptr,mfptr,OPT_GCP_GPT,status);
      else
	{
	  /* copy the non-grouping table HDU the conventional way */

	  *status = fits_copy_hdu(tmpfptr,mfptr,0,status);

	  ffgrec(mfptr,0,card,status);

	  /* delete all the GRPIDn and GRPLCn keywords in the copied HDU */

	  while(*status == 0)
	    {
	      *status = fits_find_nextkey(mfptr,incList,2,NULL,0,card,status);
	      *status = fits_get_hdrpos(mfptr,&numkeys,&keypos,status);  
	      /* SPR 1738 */
	      *status = fits_read_keyn(mfptr,keypos-1,keyname,value,
				       comment,status);
	      *status = fits_read_record(mfptr,keypos-1,card,status);
	      *status = fits_delete_key(mfptr,keyname,status);
	    }

	  if(*status == KEY_NO_EXIST) *status = 0;
	  if(*status != 0) continue;
	}

      /* 
	 if the member HDU does not have an EXTNAME keyword then add one
	 with a default value
      */

      if(strlen(extname) == 0)
	{
	  if(fits_get_hdu_num(tmpfptr,&hdunum) == 1)
	    {
	      strcpy(extname,"PRIMARY");
	      *status = fits_write_key_str(mfptr,"EXTNAME",extname,
					   "HDU was Formerly a Primary Array",
					   status);
	    }
	  else
	    {
	      strcpy(extname,"DEFAULT");
	      *status = fits_write_key_str(mfptr,"EXTNAME",extname,
					   "default EXTNAME set by CFITSIO",
					   status);
	    }
	}

      /* 
	 update the member HDU's EXTVER value (add it if not present)
      */

      fits_get_hdu_num(mfptr,&hdunum);
      fits_get_hdu_type(mfptr,&hdutype,status);

      /* set the EXTVER value to 0 for now */

      *status = fits_modify_key_lng(mfptr,"EXTVER",0,NULL,status);

      /* if the EXTVER keyword was not found then add it */

      if(*status == KEY_NO_EXIST)
	{
	  *status = 0;
	  *status = fits_read_key_str(mfptr,"EXTNAME",extname,comment,
				      status);
	  *status = fits_insert_key_lng(mfptr,"EXTVER",0,
					"Extension version ID",status);
	}

      if(*status != 0) continue;

      /* find the first available EXTVER value for the copied HDU */
 
      for(i = 1; fits_movnam_hdu(mfptr,hdutype,extname,i,status) == 0; ++i);

      *status = 0;

      fits_movabs_hdu(mfptr,hdunum,&hdutype,status);

      /* reset the copied member HDUs EXTVER value */

      *status = fits_modify_key_lng(mfptr,"EXTVER",(long)i,NULL,status);    

      /*
	perform member copy operations that are dependent upon the cpopt
	parameter value
      */

      switch(cpopt)
	{
	case OPT_MCP_ADD:

	  /*
	    add the copied member to the grouping table, leaving the
	    entry for the original member in place
	  */

	  *status = fits_add_group_member(gfptr,mfptr,0,status);

	  break;

	case OPT_MCP_NADD:

	  /*
	    nothing to do for this copy option
	  */

	  break;

	case OPT_MCP_REPL:

	  /*
	    remove the original member from the grouping table and add the
	    copied member in its place
	  */

	  *status = fits_remove_member(gfptr,member,OPT_RM_ENTRY,status);
	  *status = fits_add_group_member(gfptr,mfptr,0,status);

	  break;

	default:

	  *status = BAD_OPTION;
	  ffpmsg("Invalid value specified for the cmopt parameter (ffgmcp)");

	  break;
	}

    }while(0);
      
  if(tmpfptr != NULL) 
    {
      fits_close_file(tmpfptr,status);
    }

  return(*status);
}		     

/*---------------------------------------------------------------------------*/
int ffgmtf(fitsfile *infptr,   /* FITS file pointer to source grouping table */
	   fitsfile *outfptr,  /* FITS file pointer to target grouping table */
	   long      member,   /* member ID within source grouping table     */
	   int       tfopt,    /* code specifying transfer opts:
				  OPT_MCP_ADD (0) ==> copy member to dest.
				  OPT_MCP_MOV (3) ==> move member to dest.   */
	   int      *status)   /* return status code                         */

/*
  transfer a group member from one grouping table to another. The source
  grouping table must be the CHDU of the fitsfile pointed to by infptr, and 
  the destination grouping table must be the CHDU of the fitsfile to by 
  outfptr. If the tfopt parameter is OPT_MCP_ADD then the member is made a 
  member of the target group and remains a member of the source group. If
  the tfopt parameter is OPT_MCP_MOV then the member is deleted from the 
  source group after the transfer to the destination group. The member to be
  transfered is identified by its row number within the source grouping table.
*/

{
  fitsfile *mfptr = NULL;


  if(*status != 0) return(*status);

  if(tfopt != OPT_MCP_MOV && tfopt != OPT_MCP_ADD)
    {
      *status = BAD_OPTION;
      ffpmsg("Invalid value specified for the tfopt parameter (ffgmtf)");
    }
  else
    {
      /* open the member of infptr to be transfered */

      *status = fits_open_member(infptr,member,&mfptr,status);
      
      /* add the member to the outfptr grouping table */
      
      *status = fits_add_group_member(outfptr,mfptr,0,status);
      
      /* close the member HDU */
      
      *status = fits_close_file(mfptr,status);
      
      /* 
	 if the tfopt is "move member" then remove it from the infptr 
	 grouping table
      */

      if(tfopt == OPT_MCP_MOV)
	*status = fits_remove_member(infptr,member,OPT_RM_ENTRY,status);
    }
  
  return(*status);
}

/*---------------------------------------------------------------------------*/
int ffgmrm(fitsfile *gfptr,  /* FITS file pointer to group table             */
	   long      member, /* member ID (row num) in the group             */
	   int       rmopt,  /* code specifying the delete option:
				OPT_RM_ENTRY ==> delete the member entry
				OPT_RM_MBR   ==> delete entry and member HDU */
	   int      *status)  /* return status code                          */

/*
  remove a member HDU from a grouping table. The fitsfile pointer gfptr must
  be positioned with the grouping table as the CHDU, and the member to 
  delete is identified by its row number in the table (first member == 1).
  The rmopt parameter determines if the member entry is deleted from the
  grouping table (in which case GRPIDn and GRPLCn keywords in the member 
  HDU's header shall be updated accordingly) or if the member HDU shall 
  itself be removed from its FITS file.
*/

{
  int found;
  int hdutype   = 0;
  int index;
  int iomode    = 0;

  long i;
  long ngroups      = 0;
  long nmembers     = 0;
  long groupExtver  = 0;
  long grpid        = 0;

  char grpLocation1[FLEN_FILENAME];
  char grpLocation2[FLEN_FILENAME];
  char grpLocation3[FLEN_FILENAME];
  char cwd[FLEN_FILENAME];
  char keyword[FLEN_KEYWORD];
  /* SPR 1738 This can now be longer */
  char grplc[FLEN_FILENAME];
  char *tgrplc;
  char keyvalue[FLEN_VALUE];
  char card[FLEN_CARD];
  char *editLocation;
  char mrootname[FLEN_FILENAME], grootname[FLEN_FILENAME];

  fitsfile *mfptr  = NULL;


  if(*status != 0) return(*status);

  do
    {
      /*
	make sure the grouping table can be modified before proceeding
      */

      fits_file_mode(gfptr,&iomode,status);

      if(iomode != READWRITE)
	{
	  ffpmsg("cannot modify grouping table (ffgtam)");
	  *status = BAD_GROUP_DETACH;
	  continue;
	}

      /* open the group member to be deleted and get its IOstatus*/

      *status = fits_open_member(gfptr,member,&mfptr,status);
      *status = fits_file_mode(mfptr,&iomode,status);

      /*
	 if the member HDU is to be deleted then call fits_unlink_member()
	 to remove it from all groups to which it belongs (including
	 this one) and then delete it. Note that if the member is a
	 grouping table then we have to recursively call fits_remove_member()
	 for each member of the member before we delete the member itself.
      */

      if(rmopt == OPT_RM_MBR)
	{
	    /* cannot delete a PHDU */
	    if(fits_get_hdu_num(mfptr,&hdutype) == 1)
		{
		    *status = BAD_HDU_NUM;
		    continue;
		}

	  /* determine if the member HDU is itself a grouping table */

	  *status = fits_read_key_str(mfptr,"EXTNAME",keyvalue,card,status);

	  /* if no EXTNAME is found then the HDU cannot be a grouping table */ 

	  if(*status == KEY_NO_EXIST) 
	    {
	      keyvalue[0] = 0;
	      *status = 0;
	    }
	  prepare_keyvalue(keyvalue);

	  /* Any other error is a reason to abort */

	  if(*status != 0) continue;

	  /* if the EXTNAME == GROUPING then the member is a grouping table */
	  
	  if(fits_strcasecmp(keyvalue,"GROUPING") == 0)
	    {
	      /* remove each of the grouping table members */
	      
	      *status = fits_get_num_members(mfptr,&nmembers,status);
	      
	      for(i = nmembers; i > 0 && *status == 0; --i)
		*status = fits_remove_member(mfptr,i,OPT_RM_ENTRY,status);
	      
	      if(*status != 0) continue;
	    }

	  /* unlink the member HDU from all groups that contain it */

	  *status = ffgmul(mfptr,0,status);

	  if(*status != 0) continue;
 
	  /* reset the grouping table HDU struct */

	  fits_set_hdustruc(gfptr,status);

	  /* delete the member HDU */

	  if(iomode != READONLY)
	    *status = fits_delete_hdu(mfptr,&hdutype,status);
	}
      else if(rmopt == OPT_RM_ENTRY)
	{
	  /* 
	     The member HDU is only to be removed as an entry from this
	     grouping table. Actions are (1) find the GRPIDn/GRPLCn 
	     keywords that link the member to the grouping table, (2)
	     remove the GRPIDn/GRPLCn keyword from the member HDU header
	     and (3) remove the member entry from the grouping table
	  */

	  /*
	    there is no need to seach for and remove the GRPIDn/GRPLCn
	    keywords from the member HDU if it has not been opened
	    in READWRITE mode
	  */

	  if(iomode == READWRITE)
	    {	  	      
	      /* 
		 determine the group EXTVER value of the grouping table; if
		 the member HDU and grouping table HDU do not reside in the 
		 same file then set the groupExtver value to its negative 
	      */
	      
	      *status = fits_read_key_lng(gfptr,"EXTVER",&groupExtver,card,
					  status);
	      /* Now, if either the Fptr values are the same, or the root filenames
	         are the same, then assume these refer to the same file.
	      */
	      fits_parse_rootname(mfptr->Fptr->filename, mrootname, status);
	      fits_parse_rootname(gfptr->Fptr->filename, grootname, status);

	      if((mfptr->Fptr != gfptr->Fptr) && 
	          strncmp(mrootname, grootname, FLEN_FILENAME))
                       groupExtver = -1*groupExtver;
	      
	      /*
		retrieve the URLs for the grouping table; note that it is 
		possible that the grouping table file has two URLs, the 
		one used to open it and the "real" one pointing to the 
		actual file being accessed
	      */
	      
	      *status = fits_get_url(gfptr,grpLocation1,grpLocation2,NULL,
				     NULL,NULL,status);
	      
	      if(*status != 0) continue;
	      
	      /*
		if either of the group location strings specify a relative
		file path then convert them into absolute file paths
	      */

	      *status = fits_get_cwd(cwd,status);
	      
	      if(*grpLocation1 != 0 && *grpLocation1 != '/' &&
		 !fits_is_url_absolute(grpLocation1))
		{
		  strcpy(grpLocation3,cwd);
		  strcat(grpLocation3,"/");
		  strcat(grpLocation3,grpLocation1);
		  fits_clean_url(grpLocation3,grpLocation1,status);
		}
	      
	      if(*grpLocation2 != 0 && *grpLocation2 != '/' &&
		 !fits_is_url_absolute(grpLocation2))
		{
		  strcpy(grpLocation3,cwd);
		  strcat(grpLocation3,"/");
		  strcat(grpLocation3,grpLocation2);
		  fits_clean_url(grpLocation3,grpLocation2,status);
		}
	      
	      /*
		determine the number of groups to which the member HDU 
		belongs
	      */
	      
	      *status = fits_get_num_groups(mfptr,&ngroups,status);
	      
	      /* reset the HDU keyword position counter to the beginning */
	      
	      *status = ffgrec(mfptr,0,card,status);
	      
	      /*
		loop over all the GRPIDn keywords in the member HDU header 
		and find the appropriate GRPIDn and GRPLCn keywords that 
		identify it as belonging to the group
	      */
	      
	      for(index = 1, found = 0; index <= ngroups && *status == 0 && 
		    !found; ++index)
		{	  
		  /* read the next GRPIDn keyword in the series */
		  
		  sprintf(keyword,"GRPID%d",index);
		  
		  *status = fits_read_key_lng(mfptr,keyword,&grpid,card,
					      status);
		  if(*status != 0) continue;
		  
		  /* 
		     grpid value == group EXTVER value then we could have a 
		     match
		  */
		  
		  if(grpid == groupExtver && grpid > 0)
		    {
		      /*
			if GRPID is positive then its a match because 
			both the member HDU and grouping table HDU reside
			in the same FITS file
		      */
		      
		      found = index;
		    }
		  else if(grpid == groupExtver && grpid < 0)
		    {
		      /* 
			 have to look at the GRPLCn value to determine a 
			 match because the member HDU and grouping table 
			 HDU reside in different FITS files
		      */
		      
		      sprintf(keyword,"GRPLC%d",index);
		      
		      /* SPR 1738 */
		      *status = fits_read_key_longstr(mfptr,keyword,&tgrplc,
						      card, status);
		      if (0 == *status) {
			strcpy(grplc,tgrplc);
			free(tgrplc);
		      }
		      		      
		      if(*status == KEY_NO_EXIST)
			{
			  /* 
			     no GRPLCn keyword value found ==> grouping
			     convention not followed; nothing we can do 
			     about it, so just continue
			  */
			  
			  sprintf(card,"No GRPLC%d found for GRPID%d",
				  index,index);
			  ffpmsg(card);
			  *status = 0;
			  continue;
			}
		      else if (*status != 0) continue;
		      
		      /* construct the URL for the GRPLCn value */
		      
		      prepare_keyvalue(grplc);
		      
		      /*
			if the grplc value specifies a relative path then
			turn it into a absolute file path for comparison
			purposes
		      */
		      
		      if(*grplc != 0 && !fits_is_url_absolute(grplc) &&
			 *grplc != '/')
			{
			    /* No, wrong, 
			       strcpy(grpLocation3,cwd);
			       should be */
			    *status = fits_file_name(mfptr,grpLocation3,status);
			    /* Remove everything after the last / */
			    if (NULL != (editLocation = strrchr(grpLocation3,'/'))) {
				*editLocation = '\0';
			    }
				
			  strcat(grpLocation3,"/");
			  strcat(grpLocation3,grplc);
			  *status = fits_clean_url(grpLocation3,grplc,
						   status);
			}
		      
		      /*
			if the absolute value of GRPIDn is equal to the
			EXTVER value of the grouping table and (one of the 
			possible two) grouping table file URL matches the
			GRPLCn keyword value then we hava a match
		      */
		      
		      if(strcmp(grplc,grpLocation1) == 0  || 
			 strcmp(grplc,grpLocation2) == 0) 
			found = index; 
		    }
		}

	      /*
		if found == 0 (false) after the above search then we assume 
		that it is due to an inpromper updating of the GRPIDn and 
		GRPLCn keywords in the member header ==> nothing to delete 
		in the header. Else delete the GRPLCn and GRPIDn keywords 
		that identify the member HDU with the group HDU and 
		re-enumerate the remaining GRPIDn and GRPLCn keywords
	      */

	      if(found != 0)
		{
		  sprintf(keyword,"GRPID%d",found);
		  *status = fits_delete_key(mfptr,keyword,status);
		  
		  sprintf(keyword,"GRPLC%d",found);
		  *status = fits_delete_key(mfptr,keyword,status);
		  
		  *status = 0;
		  
		  /* call fits_get_num_groups() to re-enumerate the GRPIDn */
		  
		  *status = fits_get_num_groups(mfptr,&ngroups,status);
		} 
	    }

	  /*
	     finally, remove the member entry from the current grouping table
	     pointed to by gfptr
	  */

	  *status = fits_delete_rows(gfptr,member,1,status);
	}
      else
	{
	  *status = BAD_OPTION;
	  ffpmsg("Invalid value specified for the rmopt parameter (ffgmrm)");
	}

    }while(0);

  if(mfptr != NULL) 
    {
      fits_close_file(mfptr,status);
    }

  return(*status);
}

/*---------------------------------------------------------------------------
                 Grouping Table support functions
  ---------------------------------------------------------------------------*/
int ffgtgc(fitsfile *gfptr,  /* pointer to the grouping table                */
	   int *xtensionCol, /* column ID of the MEMBER_XTENSION column      */
	   int *extnameCol,  /* column ID of the MEMBER_NAME column          */
	   int *extverCol,   /* column ID of the MEMBER_VERSION column       */
	   int *positionCol, /* column ID of the MEMBER_POSITION column      */
	   int *locationCol, /* column ID of the MEMBER_LOCATION column      */
	   int *uriCol,      /* column ID of the MEMBER_URI_TYPE column      */
	   int *grptype,     /* group structure type code specifying the
				grouping table columns that are defined:
				GT_ID_ALL_URI  (0) ==> all columns defined   
				GT_ID_REF      (1) ==> reference cols only   
				GT_ID_POS      (2) ==> position col only     
				GT_ID_ALL      (3) ==> ref & pos cols        
				GT_ID_REF_URI (11) ==> ref & loc cols        
				GT_ID_POS_URI (12) ==> pos & loc cols        */
	   int *status)      /* return status code                           */
/*
   examine the grouping table pointed to by gfptr and determine the column
   index ID of each possible grouping column. If a column is not found then
   an index of 0 is returned. the grptype parameter returns the structure
   of the grouping table ==> what columns are defined.
*/

{

  char keyvalue[FLEN_VALUE];
  char comment[FLEN_COMMENT];


  if(*status != 0) return(*status);

  do
    {
      /*
	if the HDU does not have an extname of "GROUPING" then it is not
	a grouping table
      */

      *status = fits_read_key_str(gfptr,"EXTNAME",keyvalue,comment,status);
  
      if(*status == KEY_NO_EXIST) 
	{
	  *status = NOT_GROUP_TABLE;
	  ffpmsg("Specified HDU is not a Grouping Table (ffgtgc)");
	}
      if(*status != 0) continue;

      prepare_keyvalue(keyvalue);

      if(fits_strcasecmp(keyvalue,"GROUPING") != 0)
	{
	  *status = NOT_GROUP_TABLE;
	  continue;
	}

      /*
        search for the MEMBER_XTENSION, MEMBER_NAME, MEMBER_VERSION,
	MEMBER_POSITION, MEMBER_LOCATION and MEMBER_URI_TYPE columns
	and determine their column index ID
      */

      *status = fits_get_colnum(gfptr,CASESEN,"MEMBER_XTENSION",xtensionCol,
				status);

      if(*status == COL_NOT_FOUND)
	{
	  *status      = 0;
 	  *xtensionCol = 0;
	}

      if(*status != 0) continue;

      *status = fits_get_colnum(gfptr,CASESEN,"MEMBER_NAME",extnameCol,status);

      if(*status == COL_NOT_FOUND)
	{
	  *status     = 0;
	  *extnameCol = 0;
	}

      if(*status != 0) continue;

      *status = fits_get_colnum(gfptr,CASESEN,"MEMBER_VERSION",extverCol,
				status);

      if(*status == COL_NOT_FOUND)
	{
	  *status    = 0;
	  *extverCol = 0;
	}

      if(*status != 0) continue;

      *status = fits_get_colnum(gfptr,CASESEN,"MEMBER_POSITION",positionCol,
				status);

      if(*status == COL_NOT_FOUND)
	{
	  *status      = 0;
	  *positionCol = 0;
	}

      if(*status != 0) continue;

      *status = fits_get_colnum(gfptr,CASESEN,"MEMBER_LOCATION",locationCol,
				status);

      if(*status == COL_NOT_FOUND)
	{
	  *status      = 0;
	  *locationCol = 0;
	}

      if(*status != 0) continue;

      *status = fits_get_colnum(gfptr,CASESEN,"MEMBER_URI_TYPE",uriCol,
				status);

      if(*status == COL_NOT_FOUND)
	{
	  *status = 0;
	  *uriCol = 0;
	}

      if(*status != 0) continue;

      /*
	 determine the type of grouping table structure used by this
	 grouping table and record it in the grptype parameter
      */

      if(*xtensionCol && *extnameCol && *extverCol && *positionCol &&
	 *locationCol && *uriCol) 
	*grptype = GT_ID_ALL_URI;
      
      else if(*xtensionCol && *extnameCol && *extverCol &&
	      *locationCol && *uriCol) 
	*grptype = GT_ID_REF_URI;

      else if(*xtensionCol && *extnameCol && *extverCol && *positionCol)
	*grptype = GT_ID_ALL;
      
      else if(*xtensionCol && *extnameCol && *extverCol)
	*grptype = GT_ID_REF;
      
      else if(*positionCol && *locationCol && *uriCol) 
	*grptype = GT_ID_POS_URI;
      
      else if(*positionCol)
	*grptype = GT_ID_POS;
      
      else
	*status = NOT_GROUP_TABLE;
      
    }while(0);

  /*
    if the table contained more than one column with a reserved name then
    this cannot be considered a vailid grouping table
  */

  if(*status == COL_NOT_UNIQUE) 
    {
      *status = NOT_GROUP_TABLE;
      ffpmsg("Specified HDU has multipule Group table cols defined (ffgtgc)");
    }

  return(*status);
}

/*****************************************************************************/
int ffgtdc(int   grouptype,     /* code specifying the type of
				   grouping table information:
				   GT_ID_ALL_URI  0 ==> defualt (all columns)
				   GT_ID_REF      1 ==> ID by reference
				   GT_ID_POS      2 ==> ID by position
				   GT_ID_ALL      3 ==> ID by ref. and position
				   GT_ID_REF_URI 11 ==> (1) + URI info 
				   GT_ID_POS_URI 12 ==> (2) + URI info       */
	   int   xtensioncol, /* does MEMBER_XTENSION already exist?         */
	   int   extnamecol,  /* does MEMBER_NAME aleady exist?              */
	   int   extvercol,   /* does MEMBER_VERSION already exist?          */
	   int   positioncol, /* does MEMBER_POSITION already exist?         */
	   int   locationcol, /* does MEMBER_LOCATION already exist?         */
	   int   uricol,      /* does MEMBER_URI_TYPE aleardy exist?         */
	   char *ttype[],     /* array of grouping table column TTYPE names
				 to define (if *col var false)               */
	   char *tform[],     /* array of grouping table column TFORM values
				 to define (if*col variable false)           */
	   int  *ncols,       /* number of TTYPE and TFORM values returned   */
	   int  *status)      /* return status code                          */

/*
  create the TTYPE and TFORM values for the grouping table according to the
  value of the grouptype parameter and the values of the *col flags. The
  resulting TTYPE and TFORM are returned in ttype[] and tform[] respectively.
  The number of TTYPE and TFORMs returned is given by ncols. Both the TTYPE[]
  and TTFORM[] arrays must contain enough pre-allocated strings to hold
  the returned information.
*/

{

  int i = 0;

  char  xtension[]  = "MEMBER_XTENSION";
  char  xtenTform[] = "8A";
  
  char  name[]      = "MEMBER_NAME";
  char  nameTform[] = "32A";

  char  version[]   = "MEMBER_VERSION";
  char  verTform[]  = "1J";
  
  char  position[]  = "MEMBER_POSITION";
  char  posTform[]  = "1J";

  char  URI[]       = "MEMBER_URI_TYPE";
  char  URITform[]  = "3A";

  char  location[]  = "MEMBER_LOCATION";
  /* SPR 01720, move from 160A to 256A */
  char  locTform[]  = "256A";


  if(*status != 0) return(*status);

  switch(grouptype)
    {
      
    case GT_ID_ALL_URI:

      if(xtensioncol == 0)
	{
	  strcpy(ttype[i],xtension);
	  strcpy(tform[i],xtenTform);
	  ++i;
	}
      if(extnamecol == 0)
	{
	  strcpy(ttype[i],name);
	  strcpy(tform[i],nameTform);
	  ++i;
	}
      if(extvercol == 0)
	{
	  strcpy(ttype[i],version);
	  strcpy(tform[i],verTform);
	  ++i;
	}
      if(positioncol == 0)
	{
	  strcpy(ttype[i],position);
	  strcpy(tform[i],posTform);
	  ++i;
	}
      if(locationcol == 0)
	{
	  strcpy(ttype[i],location);
	  strcpy(tform[i],locTform);
	  ++i;
	}
      if(uricol == 0)
	{
	  strcpy(ttype[i],URI);
	  strcpy(tform[i],URITform);
	  ++i;
	}
      break;
      
    case GT_ID_REF:
      
      if(xtensioncol == 0)
	{
	  strcpy(ttype[i],xtension);
	  strcpy(tform[i],xtenTform);
	  ++i;
	}
      if(extnamecol == 0)
	{
	  strcpy(ttype[i],name);
	  strcpy(tform[i],nameTform);
	  ++i;
	}
      if(extvercol == 0)
	{
	  strcpy(ttype[i],version);
	  strcpy(tform[i],verTform);
	  ++i;
	}
      break;
      
    case GT_ID_POS:
      
      if(positioncol == 0)
	{
	  strcpy(ttype[i],position);
	  strcpy(tform[i],posTform);
	  ++i;
	}	  
      break;
      
    case GT_ID_ALL:
      
      if(xtensioncol == 0)
	{
	  strcpy(ttype[i],xtension);
	  strcpy(tform[i],xtenTform);
	  ++i;
	}
      if(extnamecol == 0)
	{
	  strcpy(ttype[i],name);
	  strcpy(tform[i],nameTform);
	  ++i;
	}
      if(extvercol == 0)
	{
	  strcpy(ttype[i],version);
	  strcpy(tform[i],verTform);
	  ++i;
	}
      if(positioncol == 0)
	{
	  strcpy(ttype[i],position);
	  strcpy(tform[i], posTform);
	  ++i;
	}	  
      
      break;
      
    case GT_ID_REF_URI:
      
      if(xtensioncol == 0)
	{
	  strcpy(ttype[i],xtension);
	  strcpy(tform[i],xtenTform);
	  ++i;
	}
      if(extnamecol == 0)
	{
	  strcpy(ttype[i],name);
	  strcpy(tform[i],nameTform);
	  ++i;
	}
      if(extvercol == 0)
	{
	  strcpy(ttype[i],version);
	  strcpy(tform[i],verTform);
	  ++i;
	}
      if(locationcol == 0)
	{
	  strcpy(ttype[i],location);
	  strcpy(tform[i],locTform);
	  ++i;
	}
      if(uricol == 0)
	{
	  strcpy(ttype[i],URI);
	  strcpy(tform[i],URITform);
	  ++i;
	}
      break;
      
    case GT_ID_POS_URI:
      
      if(positioncol == 0)
	{
	  strcpy(ttype[i],position);
	  strcpy(tform[i],posTform);
	  ++i;
	}
      if(locationcol == 0)
	{
	  strcpy(ttype[i],location);
	  strcpy(tform[i],locTform);
	  ++i;
	}
      if(uricol == 0)
	{
	  strcpy(ttype[i],URI);
	  strcpy(tform[i],URITform);
	  ++i;
	}
      break;
      
    default:
      
      *status = BAD_OPTION;
      ffpmsg("Invalid value specified for the grouptype parameter (ffgtdc)");

      break;

    }

  *ncols = i;
  
  return(*status);
}

/*****************************************************************************/
int ffgmul(fitsfile *mfptr,   /* pointer to the grouping table member HDU    */
           int       rmopt,   /* 0 ==> leave GRPIDn/GRPLCn keywords,
				 1 ==> remove GRPIDn/GRPLCn keywords         */
	   int      *status) /* return status code                          */

/*
   examine all the GRPIDn and GRPLCn keywords in the member HDUs header
   and remove the member from the grouping tables referenced; This
   effectively "unlinks" the member from all of its groups. The rmopt 
   specifies if the GRPIDn/GRPLCn keywords are to be removed from the
   member HDUs header after the unlinking.
*/

{
  int memberPosition = 0;
  int iomode;

  long index;
  long ngroups      = 0;
  long memberExtver = 0;
  long memberID     = 0;

  char mbrLocation1[FLEN_FILENAME];
  char mbrLocation2[FLEN_FILENAME];
  char memberHDUtype[FLEN_VALUE];
  char memberExtname[FLEN_VALUE];
  char keyword[FLEN_KEYWORD];
  char card[FLEN_CARD];

  fitsfile *gfptr = NULL;


  if(*status != 0) return(*status);

  do
    {
      /* 
	 determine location parameters of the member HDU; note that
	 default values are supplied if the expected keywords are not
	 found
      */

      *status = fits_read_key_str(mfptr,"XTENSION",memberHDUtype,card,status);

      if(*status == KEY_NO_EXIST) 
	{
	  strcpy(memberHDUtype,"PRIMARY");
	  *status = 0;
	}
      prepare_keyvalue(memberHDUtype);

      *status = fits_read_key_lng(mfptr,"EXTVER",&memberExtver,card,status);

      if(*status == KEY_NO_EXIST) 
	{
	  memberExtver = 1;
	  *status      = 0;
	}

      *status = fits_read_key_str(mfptr,"EXTNAME",memberExtname,card,status);

      if(*status == KEY_NO_EXIST) 
	{
	  memberExtname[0] = 0;
	  *status          = 0;
	}
      prepare_keyvalue(memberExtname);

      fits_get_hdu_num(mfptr,&memberPosition);

      *status = fits_get_url(mfptr,mbrLocation1,mbrLocation2,NULL,NULL,
			     NULL,status);

      if(*status != 0) continue;

      /*
	 open each grouping table linked to this HDU and remove the member 
	 from the grouping tables
      */

      *status = fits_get_num_groups(mfptr,&ngroups,status);

      /* loop over each group linked to the member HDU */

      for(index = 1; index <= ngroups && *status == 0; ++index)
	{
	  /* open the (index)th group linked to the member HDU */ 

	  *status = fits_open_group(mfptr,index,&gfptr,status);

	  /* if the group could not be opened then just skip it */

	  if(*status != 0)
	    {
	      *status = 0;
	      sprintf(card,"Cannot open the %dth group table (ffgmul)",
		      (int)index);
	      ffpmsg(card);
	      continue;
	    }

	  /*
	    make sure the grouping table can be modified before proceeding
	  */
	  
	  fits_file_mode(gfptr,&iomode,status);

	  if(iomode != READWRITE)
	    {
	      sprintf(card,"The %dth group cannot be modified (ffgtam)",
		      (int)index);
	      ffpmsg(card);
	      continue;
	    }

	  /* 
	     try to find the member's row within the grouping table; first 
	     try using the member HDU file's "real" URL string then try
	     using its originally opened URL string if either string exist
	   */
	     
	  memberID = 0;
 
	  if(strlen(mbrLocation1) != 0)
	    {
	      *status = ffgmf(gfptr,memberHDUtype,memberExtname,memberExtver,
			      memberPosition,mbrLocation1,&memberID,status);
	    }

	  if(*status == MEMBER_NOT_FOUND && strlen(mbrLocation2) != 0)
	    {
	      *status = 0;
	      *status = ffgmf(gfptr,memberHDUtype,memberExtname,memberExtver,
			      memberPosition,mbrLocation2,&memberID,status);
	    }

	  /* if the member was found then delete it from the grouping table */

	  if(*status == 0)
	    *status = fits_delete_rows(gfptr,memberID,1,status);

	  /*
	     continue the loop over all member groups even if an error
	     was generated
	  */

	  if(*status == MEMBER_NOT_FOUND)
	    {
	      ffpmsg("cannot locate member's entry in group table (ffgmul)");
	    }
	  *status = 0;

	  /*
	     close the file pointed to by gfptr if it is non NULL to
	     prepare for the next loop iterration
	  */

	  if(gfptr != NULL)
	    {
	      fits_close_file(gfptr,status);
	      gfptr = NULL;
	    }
	}

      if(*status != 0) continue;

      /*
	 if rmopt is non-zero then find and delete the GRPIDn/GRPLCn 
	 keywords from the member HDU header
      */

      if(rmopt != 0)
	{
	  fits_file_mode(mfptr,&iomode,status);

	  if(iomode == READONLY)
	    {
	      ffpmsg("Cannot modify member HDU, opened READONLY (ffgmul)");
	      continue;
	    }

	  /* delete all the GRPIDn/GRPLCn keywords */

	  for(index = 1; index <= ngroups && *status == 0; ++index)
	    {
	      sprintf(keyword,"GRPID%d",(int)index);
	      fits_delete_key(mfptr,keyword,status);
	      
	      sprintf(keyword,"GRPLC%d",(int)index);
	      fits_delete_key(mfptr,keyword,status);

	      if(*status == KEY_NO_EXIST) *status = 0;
	    }
	}
    }while(0);

  /* make sure the gfptr has been closed */

  if(gfptr != NULL)
    { 
      fits_close_file(gfptr,status);
    }

return(*status);
}

/*--------------------------------------------------------------------------*/
int ffgmf(fitsfile *gfptr, /* pointer to grouping table HDU to search       */
	   char *xtension,  /* XTENSION value for member HDU                */
	   char *extname,   /* EXTNAME value for member HDU                 */
	   int   extver,    /* EXTVER value for member HDU                  */
	   int   position,  /* HDU position value for member HDU            */
	   char *location,  /* FITS file location value for member HDU      */
	   long *member,    /* member HDU ID within group table (if found)  */
	   int  *status)    /* return status code                           */

/*
   try to find the entry for the member HDU defined by the xtension, extname,
   extver, position, and location parameters within the grouping table
   pointed to by gfptr. If the member HDU is found then its ID (row number)
   within the grouping table is returned in the member variable; if not
   found then member is returned with a value of 0 and the status return
   code will be set to MEMBER_NOT_FOUND.

   Note that the member HDU postion information is used to obtain a member
   match only if the grouping table type is GT_ID_POS_URI or GT_ID_POS. This
   is because the position information can become invalid much more
   easily then the reference information for a group member.
*/

{
  int xtensionCol,extnameCol,extverCol,positionCol,locationCol,uriCol;
  int mposition = 0;
  int grptype;
  int dummy;
  int i;

  long nmembers = 0;
  long mextver  = 0;
 
  char  charBuff1[FLEN_FILENAME];
  char  charBuff2[FLEN_FILENAME];
  char  tmpLocation[FLEN_FILENAME];
  char  mbrLocation1[FLEN_FILENAME];
  char  mbrLocation2[FLEN_FILENAME];
  char  mbrLocation3[FLEN_FILENAME];
  char  grpLocation1[FLEN_FILENAME];
  char  grpLocation2[FLEN_FILENAME];
  char  cwd[FLEN_FILENAME];

  char  nstr[] = {'\0'};
  char *tmpPtr[2];

  if(*status != 0) return(*status);

  *member = 0;

  tmpPtr[0] = charBuff1;
  tmpPtr[1] = charBuff2;


  if(*status != 0) return(*status);

  /*
    if the passed LOCATION value is not an absolute URL then turn it
    into an absolute path
  */

  if(location == NULL)
    {
      *tmpLocation = 0;
    }

  else if(*location == 0)
    {
      *tmpLocation = 0;
    }

  else if(!fits_is_url_absolute(location))
    {
      fits_path2url(location,tmpLocation,status);

      if(*tmpLocation != '/')
	{
	  fits_get_cwd(cwd,status);
	  strcat(cwd,"/");
	  strcat(cwd,tmpLocation);
	  fits_clean_url(cwd,tmpLocation,status);
	}
    }

  else
    strcpy(tmpLocation,location);

  /*
     retrieve the Grouping Convention reserved column positions within
     the grouping table
  */

  *status = ffgtgc(gfptr,&xtensionCol,&extnameCol,&extverCol,&positionCol,
		   &locationCol,&uriCol,&grptype,status);

  /* retrieve the number of group members */

  *status = fits_get_num_members(gfptr,&nmembers,status);
	      
  /* 
     loop over all grouping table rows until the member HDU is found 
  */

  for(i = 1; i <= nmembers && *member == 0 && *status == 0; ++i)
    {
      if(xtensionCol != 0)
	{
	  fits_read_col_str(gfptr,xtensionCol,i,1,1,nstr,tmpPtr,&dummy,status);
	  if(fits_strcasecmp(tmpPtr[0],xtension) != 0) continue;
	}
	  
      if(extnameCol  != 0)
	{
	  fits_read_col_str(gfptr,extnameCol,i,1,1,nstr,tmpPtr,&dummy,status);
	  if(fits_strcasecmp(tmpPtr[0],extname) != 0) continue;
	}
	  
      if(extverCol   != 0)
	{
	  fits_read_col_lng(gfptr,extverCol,i,1,1,0,
			    (long*)&mextver,&dummy,status);
	  if(extver != mextver) continue;
	}
      
      /* note we only use postionCol if we have to */

      if(positionCol != 0 && 
	            (grptype == GT_ID_POS || grptype == GT_ID_POS_URI))
	{
	  fits_read_col_int(gfptr,positionCol,i,1,1,0,
			    &mposition,&dummy,status);
	  if(position != mposition) continue;
	}
      
      /*
	if no location string was passed to the function then assume that
	the calling application does not wish to use it as a comparision
	critera ==> if we got this far then we have a match
      */

      if(location == NULL)
	{
	  ffpmsg("NULL Location string given ==> ingore location (ffgmf)");
	  *member = i;
	  continue;
	}

      /*
	if the grouping table MEMBER_LOCATION column exists then read the
	location URL for the member, else set the location string to
	a zero-length string for subsequent comparisions
      */

      if(locationCol != 0)
	{
	  fits_read_col_str(gfptr,locationCol,i,1,1,nstr,tmpPtr,&dummy,status);
	  strcpy(mbrLocation1,tmpPtr[0]);
	  *mbrLocation2 = 0;
	}
      else
	*mbrLocation1 = 0;

      /* 
	 if the member location string from the grouping table is zero 
	 length (either implicitly or explicitly) then assume that the 
	 member HDU is in the same file as the grouping table HDU; retrieve
	 the possible URL values of the grouping table HDU file 
       */

      if(*mbrLocation1 == 0)
	{
	  /* retrieve the possible URLs of the grouping table file */
	  *status = fits_get_url(gfptr,mbrLocation1,mbrLocation2,NULL,NULL,
				 NULL,status);

	  /* if non-NULL, make sure the first URL is absolute or a full path */
	  if(*mbrLocation1 != 0 && !fits_is_url_absolute(mbrLocation1) &&
	     *mbrLocation1 != '/')
	    {
	      fits_get_cwd(cwd,status);
	      strcat(cwd,"/");
	      strcat(cwd,mbrLocation1);
	      fits_clean_url(cwd,mbrLocation1,status);
	    }

	  /* if non-NULL, make sure the first URL is absolute or a full path */
	  if(*mbrLocation2 != 0 && !fits_is_url_absolute(mbrLocation2) &&
	     *mbrLocation2 != '/')
	    {
	      fits_get_cwd(cwd,status);
	      strcat(cwd,"/");
	      strcat(cwd,mbrLocation2);
	      fits_clean_url(cwd,mbrLocation2,status);
	    }
	}

      /*
	if the member location was specified, then make sure that it is
	either an absolute URL or specifies a full path
      */

      else if(!fits_is_url_absolute(mbrLocation1) && *mbrLocation1 != '/')
	{
	  strcpy(mbrLocation2,mbrLocation1);

	  /* get the possible URLs for the grouping table file */
	  *status = fits_get_url(gfptr,grpLocation1,grpLocation2,NULL,NULL,
				 NULL,status);
	  
	  if(*grpLocation1 != 0)
	    {
	      /* make sure the first grouping table URL is absolute */
	      if(!fits_is_url_absolute(grpLocation1) && *grpLocation1 != '/')
		{
		  fits_get_cwd(cwd,status);
		  strcat(cwd,"/");
		  strcat(cwd,grpLocation1);
		  fits_clean_url(cwd,grpLocation1,status);
		}
	      
	      /* create an absoute URL for the member */

	      fits_relurl2url(grpLocation1,mbrLocation1,mbrLocation3,status);
	      
	      /* 
		 if URL construction succeeded then copy it to the
		 first location string; else set the location string to 
		 empty
	      */

	      if(*status == 0)
		{
		  strcpy(mbrLocation1,mbrLocation3);
		}

	      else if(*status == URL_PARSE_ERROR)
		{
		  *status       = 0;
		  *mbrLocation1 = 0;
		}
	    }
	  else
	    *mbrLocation1 = 0;

	  if(*grpLocation2 != 0)
	    {
	      /* make sure the second grouping table URL is absolute */
	      if(!fits_is_url_absolute(grpLocation2) && *grpLocation2 != '/')
		{
		  fits_get_cwd(cwd,status);
		  strcat(cwd,"/");
		  strcat(cwd,grpLocation2);
		  fits_clean_url(cwd,grpLocation2,status);
		}
	      
	      /* create an absolute URL for the member */

	      fits_relurl2url(grpLocation2,mbrLocation2,mbrLocation3,status);
	      
	      /* 
		 if URL construction succeeded then copy it to the
		 second location string; else set the location string to 
		 empty
	      */

	      if(*status == 0)
		{
		  strcpy(mbrLocation2,mbrLocation3);
		}

	      else if(*status == URL_PARSE_ERROR)
		{
		  *status       = 0;
		  *mbrLocation2 = 0;
		}
	    }
	  else
	    *mbrLocation2 = 0;
	}

      /*
	compare the passed member HDU file location string with the
	(possibly two) member location strings to see if there is a match
       */

      if(strcmp(mbrLocation1,tmpLocation) != 0 && 
	 strcmp(mbrLocation2,tmpLocation) != 0   ) continue;
  
      /* if we made it this far then a match to the member HDU was found */
      
      *member = i;
    }

  /* if a match was not found then set the return status code */

  if(*member == 0 && *status == 0) 
    {
      *status = MEMBER_NOT_FOUND;
      ffpmsg("Cannot find specified member HDU (ffgmf)");
    }

  return(*status);
}

/*--------------------------------------------------------------------------
                        Recursive Group Functions
  --------------------------------------------------------------------------*/
int ffgtrmr(fitsfile   *gfptr,  /* FITS file pointer to group               */
	    HDUtracker *HDU,    /* list of processed HDUs                   */
	    int        *status) /* return status code                       */
	    
/*
  recursively remove a grouping table and all its members. Each member of
  the grouping table pointed to by gfptr it processed. If the member is itself
  a grouping table then ffgtrmr() is recursively called to process all
  of its members. The HDUtracker struct *HDU is used to make sure a member
  is not processed twice, thus avoiding an infinite loop (e.g., a grouping
  table contains itself as a member).
*/

{
  int i;
  int hdutype;

  long nmembers = 0;

  char keyvalue[FLEN_VALUE];
  char comment[FLEN_COMMENT];
  
  fitsfile *mfptr = NULL;


  if(*status != 0) return(*status);

  /* get the number of members contained by this grouping table */

  *status = fits_get_num_members(gfptr,&nmembers,status);

  /* loop over all group members and delete them */

  for(i = nmembers; i > 0 && *status == 0; --i)
    {
      /* open the member HDU */

      *status = fits_open_member(gfptr,i,&mfptr,status);

      /* if the member cannot be opened then just skip it and continue */

      if(*status == MEMBER_NOT_FOUND) 
	{
	  *status = 0;
	  continue;
	}

      /* Any other error is a reason to abort */
      
      if(*status != 0) continue;

      /* add the member HDU to the HDUtracker struct */

      *status = fftsad(mfptr,HDU,NULL,NULL);

      /* status == HDU_ALREADY_TRACKED ==> HDU has already been processed */

      if(*status == HDU_ALREADY_TRACKED) 
	{
	  *status = 0;
	  fits_close_file(mfptr,status);
	  continue;
	}
      else if(*status != 0) continue;

      /* determine if the member HDU is itself a grouping table */

      *status = fits_read_key_str(mfptr,"EXTNAME",keyvalue,comment,status);

      /* if no EXTNAME is found then the HDU cannot be a grouping table */ 

      if(*status == KEY_NO_EXIST) 
	{
	  *status     = 0;
	  keyvalue[0] = 0;
	}
      prepare_keyvalue(keyvalue);

      /* Any other error is a reason to abort */
      
      if(*status != 0) continue;

      /* 
	 if the EXTNAME == GROUPING then the member is a grouping table 
	 and we must call ffgtrmr() to process its members
      */

      if(fits_strcasecmp(keyvalue,"GROUPING") == 0)
	  *status = ffgtrmr(mfptr,HDU,status);  

      /* 
	 unlink all the grouping tables that contain this HDU as a member 
	 and then delete the HDU (if not a PHDU)
      */

      if(fits_get_hdu_num(mfptr,&hdutype) == 1)
	      *status = ffgmul(mfptr,1,status);
      else
	  {
	      *status = ffgmul(mfptr,0,status);
	      *status = fits_delete_hdu(mfptr,&hdutype,status);
	  }

      /* close the fitsfile pointer */

      fits_close_file(mfptr,status);
    }

  return(*status);
}
/*--------------------------------------------------------------------------*/
int ffgtcpr(fitsfile   *infptr,  /* input FITS file pointer                 */
	    fitsfile   *outfptr, /* output FITS file pointer                */
	    int         cpopt,   /* code specifying copy options:
				    OPT_GCP_GPT (0) ==> cp only grouping table
				    OPT_GCP_ALL (2) ==> recusrively copy 
				    members and their members (if groups)   */
	    HDUtracker *HDU,     /* list of already copied HDUs             */
	    int        *status)  /* return status code                      */

/*
  copy a Group to a new FITS file. If the cpopt parameter is set to 
  OPT_GCP_GPT (copy grouping table only) then the existing members have their 
  GRPIDn and GRPLCn keywords updated to reflect the existance of the new group,
  since they now belong to another group. If cpopt is set to OPT_GCP_ALL 
  (copy grouping table and members recursively) then the original members are 
  not updated; the new grouping table is modified to include only the copied 
  member HDUs and not the original members.

  Note that this function is recursive. When copt is OPT_GCP_ALL it will call
  itself whenever a member HDU of the current grouping table is itself a
  grouping table (i.e., EXTNAME = 'GROUPING').
*/

{

  int i;
  int nexclude     = 8;
  int hdutype      = 0;
  int groupHDUnum  = 0;
  int numkeys      = 0;
  int keypos       = 0;
  int startSearch  = 0;
  int newPosition  = 0;

  long nmembers    = 0;
  long tfields     = 0;
  long newTfields  = 0;

  char keyword[FLEN_KEYWORD];
  char keyvalue[FLEN_VALUE];
  char card[FLEN_CARD];
  char comment[FLEN_CARD];
  char *tkeyvalue;

  char *includeList[] = {"*"};
  char *excludeList[] = {"EXTNAME","EXTVER","GRPNAME","GRPID#","GRPLC#",
			 "THEAP","TDIM#","T????#"};

  fitsfile *mfptr = NULL;


  if(*status != 0) return(*status);

  do
    {
      /*
	create a new grouping table in the FITS file pointed to by outptr
      */

      *status = fits_get_num_members(infptr,&nmembers,status);

      *status = fits_read_key_str(infptr,"GRPNAME",keyvalue,card,status);

      if(*status == KEY_NO_EXIST)
	{
	  keyvalue[0] = 0;
	  *status     = 0;
	}
      prepare_keyvalue(keyvalue);

      *status = fits_create_group(outfptr,keyvalue,GT_ID_ALL_URI,status);
     
      /* save the new grouping table's HDU position for future use */

      fits_get_hdu_num(outfptr,&groupHDUnum);

      /* update the HDUtracker struct with the grouping table's new position */
      
      *status = fftsud(infptr,HDU,groupHDUnum,NULL);

      /*
	Now populate the copied grouping table depending upon the 
	copy option parameter value
      */

      switch(cpopt)
	{

	  /*
	    for the "copy grouping table only" option we only have to
	    add the members of the original grouping table to the new
	    grouping table
	  */

	case OPT_GCP_GPT:

	  for(i = 1; i <= nmembers && *status == 0; ++i)
	    {
	      *status = fits_open_member(infptr,i,&mfptr,status);
	      *status = fits_add_group_member(outfptr,mfptr,0,status);

	      fits_close_file(mfptr,status);
	      mfptr = NULL;
	    }

	  break;

	case OPT_GCP_ALL:
      
	  /*
	    for the "copy the entire group" option
 	  */

	  /* loop over all the grouping table members */

	  for(i = 1; i <= nmembers && *status == 0; ++i)
	    {
	      /* open the ith member */

	      *status = fits_open_member(infptr,i,&mfptr,status);

	      if(*status != 0) continue;

	      /* add it to the HDUtracker struct */

	      *status = fftsad(mfptr,HDU,&newPosition,NULL);

	      /* if already copied then just add the member to the group */

	      if(*status == HDU_ALREADY_TRACKED)
		{
		  *status = 0;
		  *status = fits_add_group_member(outfptr,NULL,newPosition,
						  status);
		  fits_close_file(mfptr,status);
                  mfptr = NULL;
		  continue;
		}
	      else if(*status != 0) continue;

	      /* see if the member is a grouping table */

	      *status = fits_read_key_str(mfptr,"EXTNAME",keyvalue,card,
					  status);

	      if(*status == KEY_NO_EXIST)
		{
		  keyvalue[0] = 0;
		  *status     = 0;
		}
	      prepare_keyvalue(keyvalue);

	      /*
		if the member is a grouping table then copy it and all of
		its members using ffgtcpr(), else copy it using
		fits_copy_member(); the outptr will point to the newly
		copied member upon return from both functions
	      */

	      if(fits_strcasecmp(keyvalue,"GROUPING") == 0)
		*status = ffgtcpr(mfptr,outfptr,OPT_GCP_ALL,HDU,status);
	      else
		*status = fits_copy_member(infptr,outfptr,i,OPT_MCP_NADD,
					   status);

	      /* retrieve the position of the newly copied member */

	      fits_get_hdu_num(outfptr,&newPosition);

	      /* update the HDUtracker struct with member's new position */
	      
	      if(fits_strcasecmp(keyvalue,"GROUPING") != 0)
		*status = fftsud(mfptr,HDU,newPosition,NULL);

	      /* move the outfptr back to the copied grouping table HDU */

	      *status = fits_movabs_hdu(outfptr,groupHDUnum,&hdutype,status);

	      /* add the copied member HDU to the copied grouping table */

	      *status = fits_add_group_member(outfptr,NULL,newPosition,status);

	      /* close the mfptr pointer */

	      fits_close_file(mfptr,status);
	      mfptr = NULL;
	    }

	  break;

	default:
	  
	  *status = BAD_OPTION;
	  ffpmsg("Invalid value specified for cmopt parameter (ffgtcpr)");
	  break;
	}

      if(*status != 0) continue; 

      /* 
	 reposition the outfptr to the grouping table so that the grouping
	 table is the CHDU upon return to the calling function
      */

      fits_movabs_hdu(outfptr,groupHDUnum,&hdutype,status);

      /*
	 copy all auxiliary keyword records from the original grouping table
	 to the new grouping table; they are copied in their original order
	 and inserted just before the TTYPE1 keyword record
      */

      *status = fits_read_card(outfptr,"TTYPE1",card,status);
      *status = fits_get_hdrpos(outfptr,&numkeys,&keypos,status);
      --keypos;

      startSearch = 8;

      while(*status == 0)
	{
	  ffgrec(infptr,startSearch,card,status);

	  *status = fits_find_nextkey(infptr,includeList,1,excludeList,
				      nexclude,card,status);

	  *status = fits_get_hdrpos(infptr,&numkeys,&startSearch,status);

	  --startSearch;
	  /* SPR 1738 */
	  if (strncmp(card,"GRPLC",5)) {
	    /* Not going to be a long string so we're ok */
	    *status = fits_insert_record(outfptr,keypos,card,status);
	  } else {
	    /* We could have a long string */
	    *status = fits_read_record(infptr,startSearch,card,status);
	    card[9] = '\0';
	    *status = fits_read_key_longstr(infptr,card,&tkeyvalue,comment,
					    status);
	    if (0 == *status) {
	      fits_insert_key_longstr(outfptr,card,tkeyvalue,comment,status);
	      fits_write_key_longwarn(outfptr,status);
	      free(tkeyvalue);
	    }
	  }
	  
	  ++keypos;
	}
      
	  
      if(*status == KEY_NO_EXIST) 
	*status = 0;
      else if(*status != 0) continue;

      /*
	 search all the columns of the original grouping table and copy
	 those to the new grouping table that were not part of the grouping
	 convention. Note that is legal to have additional columns in a
	 grouping table. Also note that the order of the columns may
	 not be the same in the original and copied grouping table.
      */

      /* retrieve the number of columns in the original and new group tables */

      *status = fits_read_key_lng(infptr,"TFIELDS",&tfields,card,status);
      *status = fits_read_key_lng(outfptr,"TFIELDS",&newTfields,card,status);

      for(i = 1; i <= tfields; ++i)
	{
	  sprintf(keyword,"TTYPE%d",i);
	  *status = fits_read_key_str(infptr,keyword,keyvalue,card,status);
	  
	  if(*status == KEY_NO_EXIST)
	    {
	      *status = 0;
              keyvalue[0] = 0;
	    }
	  prepare_keyvalue(keyvalue);

	  if(fits_strcasecmp(keyvalue,"MEMBER_XTENSION") != 0 &&
	     fits_strcasecmp(keyvalue,"MEMBER_NAME")     != 0 &&
	     fits_strcasecmp(keyvalue,"MEMBER_VERSION")  != 0 &&
	     fits_strcasecmp(keyvalue,"MEMBER_POSITION") != 0 &&
	     fits_strcasecmp(keyvalue,"MEMBER_LOCATION") != 0 &&
	     fits_strcasecmp(keyvalue,"MEMBER_URI_TYPE") != 0   )
	    {
 
	      /* SPR 3956, add at the end of the table */
	      *status = fits_copy_col(infptr,outfptr,i,newTfields+1,1,status);
	      ++newTfields;
	    }
	}

    }while(0);

  if(mfptr != NULL) 
    {
      fits_close_file(mfptr,status);
    }

  return(*status);
}

/*--------------------------------------------------------------------------
                HDUtracker struct manipulation functions
  --------------------------------------------------------------------------*/
int fftsad(fitsfile   *mfptr,       /* pointer to an member HDU             */
	   HDUtracker *HDU,         /* pointer to an HDU tracker struct     */
	   int        *newPosition, /* new HDU position of the member HDU   */
	   char       *newFileName) /* file containing member HDU           */

/*
  add an HDU to the HDUtracker struct pointed to by HDU. The HDU is only 
  added if it does not already reside in the HDUtracker. If it already
  resides in the HDUtracker then the new HDU postion and file name are
  returned in  newPosition and newFileName (if != NULL)
*/

{
  int i;
  int hdunum;
  int status = 0;

  char filename1[FLEN_FILENAME];
  char filename2[FLEN_FILENAME];

  do
    {
      /* retrieve the HDU's position within the FITS file */

      fits_get_hdu_num(mfptr,&hdunum);
      
      /* retrieve the HDU's file name */
      
      status = fits_file_name(mfptr,filename1,&status);
      
      /* parse the file name and construct the "standard" URL for it */
      
      status = ffrtnm(filename1,filename2,&status);
      
      /* 
	 examine all the existing HDUs in the HDUtracker an see if this HDU
	 has already been registered
      */

      for(i = 0; 
       i < HDU->nHDU &&  !(HDU->position[i] == hdunum 
			   && strcmp(HDU->filename[i],filename2) == 0);
	  ++i);

      if(i != HDU->nHDU) 
	{
	  status = HDU_ALREADY_TRACKED;
	  if(newPosition != NULL) *newPosition = HDU->newPosition[i];
	  if(newFileName != NULL) strcpy(newFileName,HDU->newFilename[i]);
	  continue;
	}

      if(HDU->nHDU == MAX_HDU_TRACKER) 
	{
	  status = TOO_MANY_HDUS_TRACKED;
	  continue;
	}

      HDU->filename[i] = (char*) malloc(FLEN_FILENAME * sizeof(char));

      if(HDU->filename[i] == NULL)
	{
	  status = MEMORY_ALLOCATION;
	  continue;
	}

      HDU->newFilename[i] = (char*) malloc(FLEN_FILENAME * sizeof(char));

      if(HDU->newFilename[i] == NULL)
	{
	  status = MEMORY_ALLOCATION;
	  free(HDU->filename[i]);
	  continue;
	}

      HDU->position[i]    = hdunum;
      HDU->newPosition[i] = hdunum;

      strcpy(HDU->filename[i],filename2);
      strcpy(HDU->newFilename[i],filename2);
 
       ++(HDU->nHDU);

    }while(0);

  return(status);
}
/*--------------------------------------------------------------------------*/
int fftsud(fitsfile   *mfptr,       /* pointer to an member HDU             */
	   HDUtracker *HDU,         /* pointer to an HDU tracker struct     */
	   int         newPosition, /* new HDU position of the member HDU   */
	   char       *newFileName) /* file containing member HDU           */

/*
  update the HDU information in the HDUtracker struct pointed to by HDU. The 
  HDU to update is pointed to by mfptr. If non-zero, the value of newPosition
  is used to update the HDU->newPosition[] value for the mfptr, and if
  non-NULL the newFileName value is used to update the HDU->newFilename[]
  value for mfptr.
*/

{
  int i;
  int hdunum;
  int status = 0;

  char filename1[FLEN_FILENAME];
  char filename2[FLEN_FILENAME];


  /* retrieve the HDU's position within the FITS file */
  
  fits_get_hdu_num(mfptr,&hdunum);
  
  /* retrieve the HDU's file name */
  
  status = fits_file_name(mfptr,filename1,&status);
  
  /* parse the file name and construct the "standard" URL for it */
      
  status = ffrtnm(filename1,filename2,&status);

  /* 
     examine all the existing HDUs in the HDUtracker an see if this HDU
     has already been registered
  */

  for(i = 0; i < HDU->nHDU && 
      !(HDU->position[i] == hdunum && strcmp(HDU->filename[i],filename2) == 0);
      ++i);

  /* if previously registered then change newPosition and newFileName */

  if(i != HDU->nHDU) 
    {
      if(newPosition  != 0) HDU->newPosition[i] = newPosition;
      if(newFileName  != NULL) 
	{
	  strcpy(HDU->newFilename[i],newFileName);
	}
    }
  else
    status = MEMBER_NOT_FOUND;
 
  return(status);
}

/*---------------------------------------------------------------------------*/

void prepare_keyvalue(char *keyvalue) /* string containing keyword value     */

/*
  strip off all single quote characters "'" and blank spaces from a keyword
  value retrieved via fits_read_key*() routines

  this is necessary so that a standard comparision of keyword values may
  be made
*/

{

  int i;
  int length;

  /*
    strip off any leading or trailing single quotes (`) and (') from
    the keyword value
  */

  length = strlen(keyvalue) - 1;

  if(keyvalue[0] == '\'' && keyvalue[length] == '\'')
    {
      for(i = 0; i < length - 1; ++i) keyvalue[i] = keyvalue[i+1];
      keyvalue[length-1] = 0;
    }
  
  /*
    strip off any trailing blanks from the keyword value; note that if the
    keyvalue consists of nothing but blanks then no blanks are stripped
  */

  length = strlen(keyvalue) - 1;

  for(i = 0; i < length && keyvalue[i] == ' '; ++i);

  if(i != length)
    {
      for(i = length; i >= 0 && keyvalue[i] == ' '; --i) keyvalue[i] = '\0';
    }
}

/*---------------------------------------------------------------------------
        Host dependent directory path to/from URL functions
  --------------------------------------------------------------------------*/
int fits_path2url(char *inpath,  /* input file path string                  */
		  char *outpath, /* output file path string                 */
		  int  *status)
  /*
     convert a file path into its Unix-style equivelent for URL 
     purposes. Note that this process is platform dependent. This
     function supports Unix, MSDOS/WIN32, VMS and Macintosh platforms. 
     The plaform dependant code is conditionally compiled depending upon
     the setting of the appropriate C preprocessor macros.
   */
{
  char buff[FLEN_FILENAME];

#if defined(WINNT) || defined(__WINNT__)

  /*
    Microsoft Windows NT case. We assume input file paths of the form:

    //disk/path/filename

     All path segments may be null, so that a single file name is the
     simplist case.

     The leading "//" becomes a single "/" if present. If no "//" is present,
     then make sure the resulting URL path is relative, i.e., does not
     begin with a "/". In other words, the only way that an absolute URL
     file path may be generated is if the drive specification is given.
  */

  if(*status > 0) return(*status);

  if(inpath[0] == '/')
    {
      strcpy(buff,inpath+1);
    }
  else
    {
      strcpy(buff,inpath);
    }

#elif defined(MSDOS) || defined(__WIN32__) || defined(WIN32)

  /*
     MSDOS or Microsoft windows/NT case. The assumed form of the
     input path is:

     disk:\path\filename

     All path segments may be null, so that a single file name is the
     simplist case.

     All back-slashes '\' become slashes '/'; if the path starts with a
     string of the form "X:" then it is replaced with "/X/"
  */

  int i,j,k;
  int size;
  if(*status > 0) return(*status);

  for(i = 0, j = 0, size = strlen(inpath), buff[0] = 0; 
                                           i < size; j = strlen(buff))
    {
      switch(inpath[i])
	{

	case ':':

	  /*
	     must be a disk desiginator; add a slash '/' at the start of
	     outpath to designate that the path is absolute, then change
	     the colon ':' to a slash '/'
	   */

	  for(k = j; k >= 0; --k) buff[k+1] = buff[k];
	  buff[0] = '/';
	  strcat(buff,"/");
	  ++i;
	  
	  break;

	case '\\':

	  /* just replace the '\' with a '/' IF its not the first character */

	  if(i != 0 && buff[(j == 0 ? 0 : j-1)] != '/')
	    {
	      buff[j] = '/';
	      buff[j+1] = 0;
	    }

	  ++i;

	  break;

	default:

	  /* copy the character from inpath to buff as is */

	  buff[j]   = inpath[i];
	  buff[j+1] = 0;
	  ++i;

	  break;
	}
    }

#elif defined(VMS) || defined(vms) || defined(__vms)

  /*
     VMS case. Assumed format of the input path is:

     node::disk:[path]filename.ext;version

     Any part of the file path may be missing, so that in the simplist
     case a single file name/extension is given.

     all brackets "[", "]" and dots "." become "/"; dashes "-" become "..", 
     all single colons ":" become ":/", all double colons "::" become
     "FILE://"
   */

  int i,j,k;
  int done;
  int size;

  if(*status > 0) return(*status);
     
  /* see if inpath contains a directory specification */

  if(strchr(inpath,']') == NULL) 
    done = 1;
  else
    done = 0;

  for(i = 0, j = 0, size = strlen(inpath), buff[0] = 0; 
                           i < size && j < FLEN_FILENAME - 8; j = strlen(buff))
    {
      switch(inpath[i])
	{

	case ':':

	  /*
	     must be a logical/symbol separator or (in the case of a double
	     colon "::") machine node separator
	   */

	  if(inpath[i+1] == ':')
	    {
	      /* insert a "FILE://" at the start of buff ==> machine given */

	      for(k = j; k >= 0; --k) buff[k+7] = buff[k];
	      strncpy(buff,"FILE://",7);
	      i += 2;
	    }
	  else if(strstr(buff,"FILE://") == NULL)
	    {
	      /* insert a "/" at the start of buff ==> absolute path */

	      for(k = j; k >= 0; --k) buff[k+1] = buff[k];
	      buff[0] = '/';
	      ++i;
	    }
	  else
	    ++i;

	  /* a colon always ==> path separator */

	  strcat(buff,"/");

	  break;
  
	case ']':

	  /* end of directory spec, file name spec begins after this */

	  done = 1;

	  buff[j]   = '/';
	  buff[j+1] = 0;
	  ++i;

	  break;

	case '[':

	  /* 
	     begin directory specification; add a '/' only if the last char 
	     is not '/' 
	  */

	  if(i != 0 && buff[(j == 0 ? 0 : j-1)] != '/')
	    {
	      buff[j]   = '/';
	      buff[j+1] = 0;
	    }

	  ++i;

	  break;

	case '.':

	  /* 
	     directory segment separator or file name/extension separator;
	     we decide which by looking at the value of done
	  */

	  if(!done)
	    {
	    /* must be a directory segment separator */
	      if(inpath[i-1] == '[')
		{
		  strcat(buff,"./");
		  ++j;
		}
	      else
		buff[j] = '/';
	    }
	  else
	    /* must be a filename/extension separator */
	    buff[j] = '.';

	  buff[j+1] = 0;

	  ++i;

	  break;

	case '-':

	  /* 
	     a dash is the same as ".." in Unix speak, but lets make sure
	     that its not part of the file name first!
	   */

	  if(!done)
	    /* must be part of the directory path specification */
	    strcat(buff,"..");
	  else
	    {
	      /* the dash is part of the filename, so just copy it as is */
	      buff[j] = '-';
	      buff[j+1] = 0;
	    }

	  ++i;

	  break;

	default:

	  /* nothing special, just copy the character as is */

	  buff[j]   = inpath[i];
	  buff[j+1] = 0;

	  ++i;

	  break;

	}
    }

  if(j > FLEN_FILENAME - 8)
    {
      *status = URL_PARSE_ERROR;
      ffpmsg("resulting path to URL conversion too big (fits_path2url)");
    }

#elif defined(macintosh)

  /*
     MacOS case. The assumed form of the input path is:

     disk:path:filename

     It is assumed that all paths are absolute with disk and path specified,
     unless no colons ":" are supplied with the string ==> a single file name
     only. All colons ":" become slashes "/", and if one or more colon is 
     encountered then the path is specified as absolute.
  */

  int i,j,k;
  int firstColon;
  int size;

  if(*status > 0) return(*status);

  for(i = 0, j = 0, firstColon = 1, size = strlen(inpath), buff[0] = 0; 
                                                   i < size; j = strlen(buff))
    {
      switch(inpath[i])
	{

	case ':':

	  /*
	     colons imply path separators. If its the first colon encountered
	     then assume that its the disk designator and add a slash to the
	     beginning of the buff string
	   */
	  
	  if(firstColon)
	    {
	      firstColon = 0;

	      for(k = j; k >= 0; --k) buff[k+1] = buff[k];
	      buff[0] = '/';
	    }

	  /* all colons become slashes */

	  strcat(buff,"/");

	  ++i;
	  
	  break;

	default:

	  /* copy the character from inpath to buff as is */

	  buff[j]   = inpath[i];
	  buff[j+1] = 0;

	  ++i;

	  break;
	}
    }

#else 

  /*
     Default Unix case.

     Nothing special to do here except to remove the double or more // and 
     replace them with single /
   */

  int ii = 0;
  int jj = 0;

  if(*status > 0) return(*status);

  while (inpath[ii]) {
      if (inpath[ii] == '/' && inpath[ii+1] == '/') {
	  /* do nothing */
      } else {
	  buff[jj] = inpath[ii];
	  jj++;
      }
      ii++;
  }
  buff[jj] = '\0';
  /* printf("buff is %s\ninpath is %s\n",buff,inpath); */
  /* strcpy(buff,inpath); */

#endif

  /*
    encode all "unsafe" and "reserved" URL characters
  */

  *status = fits_encode_url(buff,outpath,status);

  return(*status);
}

/*---------------------------------------------------------------------------*/
int fits_url2path(char *inpath,  /* input file path string  */
		  char *outpath, /* output file path string */
		  int  *status)
  /*
     convert a Unix-style URL into a platform dependent directory path. 
     Note that this process is platform dependent. This
     function supports Unix, MSDOS/WIN32, VMS and Macintosh platforms. Each
     platform dependent code segment is conditionally compiled depending 
     upon the setting of the appropriate C preprocesser macros.
   */
{
  char buff[FLEN_FILENAME];
  int absolute;

#if defined(MSDOS) || defined(__WIN32__) || defined(WIN32)
  char *tmpStr, *saveptr;
#elif defined(VMS) || defined(vms) || defined(__vms)
  int i;
  char *tmpStr, *saveptr;
#elif defined(macintosh)
  char *tmpStr, *saveptr;
#endif

  if(*status != 0) return(*status);

  /*
    make a copy of the inpath so that we can manipulate it
  */

  strcpy(buff,inpath);

  /*
    convert any encoded characters to their unencoded values
  */

  *status = fits_unencode_url(inpath,buff,status);

  /*
    see if the URL is given as absolute w.r.t. the "local" file system
  */

  if(buff[0] == '/') 
    absolute = 1;
  else
    absolute = 0;

#if defined(WINNT) || defined(__WINNT__)

  /*
    Microsoft Windows NT case. We create output paths of the form

    //disk/path/filename

     All path segments but the last may be null, so that a single file name 
     is the simplist case.     
  */

  if(absolute)
    {
      strcpy(outpath,"/");
      strcat(outpath,buff);
    }
  else
    {
      strcpy(outpath,buff);
    }

#elif defined(MSDOS) || defined(__WIN32__) || defined(WIN32)

  /*
     MSDOS or Microsoft windows/NT case. The output path will be of the
     form

     disk:\path\filename

     All path segments but the last may be null, so that a single file name 
     is the simplist case.
  */

  /*
    separate the URL into tokens at each slash '/' and process until
    all tokens have been examined
  */

  for(tmpStr = ffstrtok(buff,"/",&saveptr), outpath[0] = 0;
                                 tmpStr != NULL; tmpStr = ffstrtok(NULL,"/",&saveptr))
    {
      strcat(outpath,tmpStr);

      /* 
	 if the absolute flag is set then process the token as a disk 
	 specification; else just process it as a directory path or filename
      */

      if(absolute)
	{
	  strcat(outpath,":\\");
	  absolute = 0;
	}
      else
	strcat(outpath,"\\");
    }

  /* remove the last "\" from the outpath, it does not belong there */

  outpath[strlen(outpath)-1] = 0;

#elif defined(VMS) || defined(vms) || defined(__vms)

  /*
     VMS case. The output path will be of the form:

     node::disk:[path]filename.ext;version

     Any part of the file path may be missing execpt filename.ext, so that in 
     the simplist case a single file name/extension is given.

     if the path is specified as relative starting with "./" then the first
     part of the VMS path is "[.". If the path is relative and does not start
     with "./" (e.g., "a/b/c") then the VMS path is constructed as
     "[a.b.c]"
   */
     
  /*
    separate the URL into tokens at each slash '/' and process until
    all tokens have been examined
  */

  for(tmpStr = ffstrtok(buff,"/",&saveptr), outpath[0] = 0; 
                                 tmpStr != NULL; tmpStr = ffstrtok(NULL,"/",&saveptr))
    {

      if(fits_strcasecmp(tmpStr,"FILE:") == 0)
	{
	  /* the next token should contain the DECnet machine name */

	  tmpStr = ffstrtok(NULL,"/",&saveptr);
	  if(tmpStr == NULL) continue;

	  strcat(outpath,tmpStr);
	  strcat(outpath,"::");

	  /* set the absolute flag to true for the next token */
	  absolute = 1;
	}

      else if(strcmp(tmpStr,"..") == 0)
	{
	  /* replace all Unix-like ".." with VMS "-" */

	  if(strlen(outpath) == 0) strcat(outpath,"[");
	  strcat(outpath,"-.");
	}

      else if(strcmp(tmpStr,".") == 0 && strlen(outpath) == 0)
	{
	  /*
	    must indicate a relative path specifier
	  */

	  strcat(outpath,"[.");
	}
  
      else if(strchr(tmpStr,'.') != NULL)
	{
	  /* 
	     must be up to the file name; turn the last "." path separator
	     into a "]" and then add the file name to the outpath
	  */
	  
	  i = strlen(outpath);
	  if(i > 0 && outpath[i-1] == '.') outpath[i-1] = ']';

	  strcat(outpath,tmpStr);
	}

      else
	{
	  /*
	    process the token as a a directory path segement
	  */

	  if(absolute)
	    {
	      /* treat the token as a disk specifier */
	      absolute = 0;
	      strcat(outpath,tmpStr);
	      strcat(outpath,":[");
	    }
	  else if(strlen(outpath) == 0)
	    {
	      /* treat the token as the first directory path specifier */
	      strcat(outpath,"[");
	      strcat(outpath,tmpStr);
	      strcat(outpath,".");
	    }
	  else
	    {
	      /* treat the token as an imtermediate path specifier */
	      strcat(outpath,tmpStr);
	      strcat(outpath,".");
	    }
	}
    }

#elif defined(macintosh)

  /*
     MacOS case. The output path will be of the form

     disk:path:filename

     All path segments but the last may be null, so that a single file name 
     is the simplist case.
  */

  /*
    separate the URL into tokens at each slash '/' and process until
    all tokens have been examined
  */

  for(tmpStr = ffstrtok(buff,"/",&saveptr), outpath[0] = 0;
                                 tmpStr != NULL; tmpStr = ffstrtok(NULL,"/",&saveptr))
    {
      strcat(outpath,tmpStr);
      strcat(outpath,":");
    }

  /* remove the last ":" from the outpath, it does not belong there */

  outpath[strlen(outpath)-1] = 0;

#else

  /*
     Default Unix case.

     Nothing special to do here
   */

  strcpy(outpath,buff);

#endif

  return(*status);
}

/****************************************************************************/
int fits_get_cwd(char *cwd,  /* IO current working directory string */
		 int  *status)
  /*
     retrieve the string containing the current working directory absolute
     path in Unix-like URL standard notation. It is assumed that the CWD
     string has a size of at least FLEN_FILENAME.

     Note that this process is platform dependent. This
     function supports Unix, MSDOS/WIN32, VMS and Macintosh platforms. Each
     platform dependent code segment is conditionally compiled depending 
     upon the setting of the appropriate C preprocesser macros.
   */
{

  char buff[FLEN_FILENAME];


  if(*status != 0) return(*status);

#if defined(macintosh)

  /*
     MacOS case. Currently unknown !!!!
  */

  *buff = 0;

#else
  /*
    Good old getcwd() seems to work with all other platforms
  */

  getcwd(buff,FLEN_FILENAME);

#endif

  /*
    convert the cwd string to a URL standard path string
  */

  fits_path2url(buff,cwd,status);

  return(*status);
}

/*---------------------------------------------------------------------------*/
int  fits_get_url(fitsfile *fptr,       /* I ptr to FITS file to evaluate    */
		  char     *realURL,    /* O URL of real FITS file           */
		  char     *startURL,   /* O URL of starting FITS file       */
		  char     *realAccess, /* O true access method of FITS file */
		  char     *startAccess,/* O "official" access of FITS file  */
		  int      *iostate,    /* O can this file be modified?      */
		  int      *status)
/*
  For grouping convention purposes, determine the URL of the FITS file
  associated with the fitsfile pointer fptr. The true access type (file://,
  mem://, shmem://, root://), starting "official" access type, and iostate 
  (0 ==> readonly, 1 ==> readwrite) are also returned.

  It is assumed that the url string has enough room to hold the resulting
  URL, and the the accessType string has enough room to hold the access type.
*/
{
  int i;
  int tmpIOstate = 0;

  char infile[FLEN_FILENAME];
  char outfile[FLEN_FILENAME];
  char tmpStr1[FLEN_FILENAME];
  char tmpStr2[FLEN_FILENAME];
  char tmpStr3[FLEN_FILENAME];
  char tmpStr4[FLEN_FILENAME];
  char *tmpPtr;


  if(*status != 0) return(*status);

  do
    {
      /* 
	 retrieve the member HDU's file name as opened by ffopen() 
	 and parse it into its constitutent pieces; get the currently
	 active driver token too
       */
	  
      *tmpStr1 = *tmpStr2 = *tmpStr3 = *tmpStr4 = 0;

      *status = fits_file_name(fptr,tmpStr1,status);

      *status = ffiurl(tmpStr1,NULL,infile,outfile,NULL,tmpStr2,tmpStr3,
		       tmpStr4,status);

      if((*tmpStr2) || (*tmpStr3) || (*tmpStr4)) tmpIOstate = -1;
 
      *status = ffurlt(fptr,tmpStr3,status);

      strcpy(tmpStr4,tmpStr3);

      *status = ffrtnm(tmpStr1,tmpStr2,status);
      strcpy(tmpStr1,tmpStr2);

      /*
	for grouping convention purposes (only) determine the URL of the
	actual FITS file being used for the given fptr, its true access 
	type (file://, mem://, shmem://, root://) and its iostate (0 ==>
	read only, 1 ==> readwrite)
      */

      /*
	The first set of access types are "simple" in that they do not
	use any redirection to temporary memory or outfiles
       */

      /* standard disk file driver is in use */
      
      if(fits_strcasecmp(tmpStr3,"file://")              == 0)         
	{
	  tmpIOstate = 1;
	  
	  if(strlen(outfile)) strcpy(tmpStr1,outfile);
	  else *tmpStr2 = 0;

	  /*
	    make sure no FILE:// specifier is given in the tmpStr1
	    or tmpStr2 strings; the convention calls for local files
	    to have no access specification
	  */

	  if((tmpPtr = strstr(tmpStr1,"://")) != NULL)
	    {
	      strcpy(infile,tmpPtr+3);
	      strcpy(tmpStr1,infile);
	    }

	  if((tmpPtr = strstr(tmpStr2,"://")) != NULL)
	    {
	      strcpy(infile,tmpPtr+3);
	      strcpy(tmpStr2,infile);
	    }
	}

      /* file stored in conventional memory */
	  
      else if(fits_strcasecmp(tmpStr3,"mem://")          == 0)          
	{
	  if(tmpIOstate < 0)
	    {
	      /* file is a temp mem file only */
	      ffpmsg("cannot make URL from temp MEM:// file (fits_get_url)");
	      *status = URL_PARSE_ERROR;
	    }
	  else
	    {
	      /* file is a "perminate" mem file for this process */
	      tmpIOstate = 1;
	      *tmpStr2 = 0;
	    }
	}

      /* file stored in conventional memory */
 
     else if(fits_strcasecmp(tmpStr3,"memkeep://")      == 0)      
	{
	  strcpy(tmpStr3,"mem://");
	  *tmpStr4 = 0;
	  *tmpStr2 = 0;
	  tmpIOstate = 1;
	}

      /* file residing in shared memory */

      else if(fits_strcasecmp(tmpStr3,"shmem://")        == 0)        
	{
	  *tmpStr4   = 0;
	  *tmpStr2   = 0;
	  tmpIOstate = 1;
	}
      
      /* file accessed via the ROOT network protocol */

      else if(fits_strcasecmp(tmpStr3,"root://")         == 0)         
	{
	  *tmpStr4   = 0;
	  *tmpStr2   = 0;
	  tmpIOstate = 1;
	}
  
      /*
	the next set of access types redirect the contents of the original
	file to an special outfile because the original could not be
	directly modified (i.e., resides on the network, was compressed).
	In these cases the URL string takes on the value of the OUTFILE,
	the access type becomes file://, and the iostate is set to 1 (can
	read/write to the file).
      */

      /* compressed file uncompressed and written to disk */

      else if(fits_strcasecmp(tmpStr3,"compressfile://") == 0) 
	{
	  strcpy(tmpStr1,outfile);
	  strcpy(tmpStr2,infile);
	  strcpy(tmpStr3,"file://");
	  strcpy(tmpStr4,"file://");
	  tmpIOstate = 1;
	}

      /* HTTP accessed file written locally to disk */

      else if(fits_strcasecmp(tmpStr3,"httpfile://")     == 0)     
	{
	  strcpy(tmpStr1,outfile);
	  strcpy(tmpStr3,"file://");
	  strcpy(tmpStr4,"http://");
	  tmpIOstate = 1;
	}
      
      /* FTP accessd file written locally to disk */

      else if(fits_strcasecmp(tmpStr3,"ftpfile://")      == 0)      
	{
	  strcpy(tmpStr1,outfile);
	  strcpy(tmpStr3,"file://");
	  strcpy(tmpStr4,"ftp://");
	  tmpIOstate = 1;
	}
      
      /* file from STDIN written to disk */

      else if(fits_strcasecmp(tmpStr3,"stdinfile://")    == 0)    
	{
	  strcpy(tmpStr1,outfile);
	  strcpy(tmpStr3,"file://");
	  strcpy(tmpStr4,"stdin://");
	  tmpIOstate = 1;
	}

      /* 
	 the following access types use memory resident files as temporary
	 storage; they cannot be modified or be made group members for 
	 grouping conventions purposes, but their original files can be.
	 Thus, their tmpStr3s are reset to mem://, their iostate
	 values are set to 0 (for no-modification), and their URL string
	 values remain set to their original values
       */

      /* compressed disk file uncompressed into memory */

      else if(fits_strcasecmp(tmpStr3,"compress://")     == 0)     
	{
	  *tmpStr1 = 0;
	  strcpy(tmpStr2,infile);
	  strcpy(tmpStr3,"mem://");
	  strcpy(tmpStr4,"file://");
	  tmpIOstate = 0;
	}
      
      /* HTTP accessed file transferred into memory */

      else if(fits_strcasecmp(tmpStr3,"http://")         == 0)         
	{
	  *tmpStr1 = 0;
	  strcpy(tmpStr3,"mem://");
	  strcpy(tmpStr4,"http://");
	  tmpIOstate = 0;
	}
      
      /* HTTP accessed compressed file transferred into memory */

      else if(fits_strcasecmp(tmpStr3,"httpcompress://") == 0) 
	{
	  *tmpStr1 = 0;
	  strcpy(tmpStr3,"mem://");
	  strcpy(tmpStr4,"http://");
	  tmpIOstate = 0;
	}
      
      /* FTP accessed file transferred into memory */
      
      else if(fits_strcasecmp(tmpStr3,"ftp://")          == 0)          
	{
	  *tmpStr1 = 0;
	  strcpy(tmpStr3,"mem://");
	  strcpy(tmpStr4,"ftp://");
	  tmpIOstate = 0;
	}
      
      /* FTP accessed compressed file transferred into memory */

      else if(fits_strcasecmp(tmpStr3,"ftpcompress://")  == 0)  
	{
	  *tmpStr1 = 0;
	  strcpy(tmpStr3,"mem://");
	  strcpy(tmpStr4,"ftp://");
	  tmpIOstate = 0;
	}	
      
      /*
	The last set of access types cannot be used to make a meaningful URL 
	strings from; thus an error is generated
       */

      else if(fits_strcasecmp(tmpStr3,"stdin://")        == 0)        
	{
	  *status = URL_PARSE_ERROR;
	  ffpmsg("cannot make vaild URL from stdin:// (fits_get_url)");
	  *tmpStr1 = *tmpStr2 = 0;
	}

      else if(fits_strcasecmp(tmpStr3,"stdout://")       == 0)       
	{
	  *status = URL_PARSE_ERROR;
	  ffpmsg("cannot make vaild URL from stdout:// (fits_get_url)");
	  *tmpStr1 = *tmpStr2 = 0;
	}

      else if(fits_strcasecmp(tmpStr3,"irafmem://")      == 0)      
	{
	  *status = URL_PARSE_ERROR;
	  ffpmsg("cannot make vaild URL from irafmem:// (fits_get_url)");
	  *tmpStr1 = *tmpStr2 = 0;
	}

      if(*status != 0) continue;

      /*
	 assign values to the calling parameters if they are non-NULL
      */

      if(realURL != NULL)
	{
	  if(strlen(tmpStr1) == 0)
	    *realURL = 0;
	  else
	    {
	      if((tmpPtr = strstr(tmpStr1,"://")) != NULL)
		{
		  tmpPtr += 3;
		  i = (long)tmpPtr - (long)tmpStr1;
		  strncpy(realURL,tmpStr1,i);
		}
	      else
		{
		  tmpPtr = tmpStr1;
		  i = 0;
		}

	      *status = fits_path2url(tmpPtr,realURL+i,status);
	    }
	}

      if(startURL != NULL)
	{
	  if(strlen(tmpStr2) == 0)
	    *startURL = 0;
	  else
	    {
	      if((tmpPtr = strstr(tmpStr2,"://")) != NULL)
		{
		  tmpPtr += 3;
		  i = (long)tmpPtr - (long)tmpStr2;
		  strncpy(startURL,tmpStr2,i);
		}
	      else
		{
		  tmpPtr = tmpStr2;
		  i = 0;
		}

	      *status = fits_path2url(tmpPtr,startURL+i,status);
	    }
	}

      if(realAccess  != NULL)  strcpy(realAccess,tmpStr3);
      if(startAccess != NULL)  strcpy(startAccess,tmpStr4);
      if(iostate     != NULL) *iostate = tmpIOstate;

    }while(0);

  return(*status);
}

/*--------------------------------------------------------------------------
                           URL parse support functions
  --------------------------------------------------------------------------*/

/* simple push/pop/shift/unshift string stack for use by fits_clean_url */
typedef char* grp_stack_data; /* type of data held by grp_stack */

typedef struct grp_stack_item_struct {
  grp_stack_data data; /* value of this stack item */
  struct grp_stack_item_struct* next; /* next stack item */
  struct grp_stack_item_struct* prev; /* previous stack item */
} grp_stack_item;

typedef struct grp_stack_struct {
  size_t stack_size; /* number of items on stack */
  grp_stack_item* top; /* top item */
} grp_stack;

static char* grp_stack_default = NULL; /* initial value for new instances
                                          of grp_stack_data */

/* the following functions implement the group string stack grp_stack */
static void delete_grp_stack(grp_stack** mystack);
static grp_stack_item* grp_stack_append(
  grp_stack_item* last, grp_stack_data data
);
static grp_stack_data grp_stack_remove(grp_stack_item* last);
static grp_stack* new_grp_stack(void);
static grp_stack_data pop_grp_stack(grp_stack* mystack);
static void push_grp_stack(grp_stack* mystack, grp_stack_data data);
static grp_stack_data shift_grp_stack(grp_stack* mystack);
/* static void unshift_grp_stack(grp_stack* mystack, grp_stack_data data); */

int fits_clean_url(char *inURL,  /* I input URL string                      */
		   char *outURL, /* O output URL string                     */
		   int  *status)
/*
  clean the URL by eliminating any ".." or "." specifiers in the inURL
  string, and write the output to the outURL string.

  Note that this function must have a valid Unix-style URL as input; platform
  dependent path strings are not allowed.
 */
{
  grp_stack* mystack; /* stack to hold pieces of URL */
  char* tmp;
  char *saveptr;

  if(*status) return *status;

  mystack = new_grp_stack();
  *outURL = 0;

  do {
    /* handle URL scheme and domain if they exist */
    tmp = strstr(inURL, "://");
    if(tmp) {
      /* there is a URL scheme, so look for the end of the domain too */
      tmp = strchr(tmp + 3, '/');
      if(tmp) {
        /* tmp is now the end of the domain, so
         * copy URL scheme and domain as is, and terminate by hand */
        size_t string_size = (size_t) (tmp - inURL);
        strncpy(outURL, inURL, string_size);
        outURL[string_size] = 0;

        /* now advance the input pointer to just after the domain and go on */
        inURL = tmp;
      } else {
        /* '/' was not found, which means there are no path-like
         * portions, so copy whole inURL to outURL and we're done */
        strcpy(outURL, inURL);
        continue; /* while(0) */
      }
    }

    /* explicitly copy a leading / (absolute path) */
    if('/' == *inURL) strcat(outURL, "/");

    /* now clean the remainder of the inURL. push URL segments onto
     * stack, dealing with .. and . as we go */
    tmp = ffstrtok(inURL, "/",&saveptr); /* finds first / */
    while(tmp) {
      if(!strcmp(tmp, "..")) {
        /* discard previous URL segment, if there was one. if not,
         * add the .. to the stack if this is *not* an absolute path
         * (for absolute paths, leading .. has no effect, so skip it) */
        if(0 < mystack->stack_size) pop_grp_stack(mystack);
        else if('/' != *inURL) push_grp_stack(mystack, tmp);
      } else {
        /* always just skip ., but otherwise add segment to stack */
        if(strcmp(tmp, ".")) push_grp_stack(mystack, tmp);
      }
      tmp = ffstrtok(NULL, "/",&saveptr); /* get the next segment */
    }

    /* stack now has pieces of cleaned URL, so just catenate them
     * onto output string until stack is empty */
    while(0 < mystack->stack_size) {
      tmp = shift_grp_stack(mystack);
      strcat(outURL, tmp);
      strcat(outURL, "/");
    }
    outURL[strlen(outURL) - 1] = 0; /* blank out trailing / */
  } while(0);
  delete_grp_stack(&mystack);
  return *status;
}

/* free all stack contents using pop_grp_stack before freeing the
 * grp_stack itself */
static void delete_grp_stack(grp_stack** mystack) {
  if(!mystack || !*mystack) return;
  while((*mystack)->stack_size) pop_grp_stack(*mystack);
  free(*mystack);
  *mystack = NULL;
}

/* append an item to the stack, handling the special case of the first
 * item appended */
static grp_stack_item* grp_stack_append(
  grp_stack_item* last, grp_stack_data data
) {
  /* first create a new stack item, and copy data to it */
  grp_stack_item* new_item = (grp_stack_item*) malloc(sizeof(grp_stack_item));
  new_item->data = data;
  if(last) {
    /* attach this item between the "last" item and its "next" item */
    new_item->next = last->next;
    new_item->prev = last;
    last->next->prev = new_item;
    last->next = new_item;
  } else {
    /* stack is empty, so "next" and "previous" both point back to it */
    new_item->next = new_item;
    new_item->prev = new_item;
  }
  return new_item;
}

/* remove an item from the stack, handling the special case of the last
 * item removed */
static grp_stack_data grp_stack_remove(grp_stack_item* last) {
  grp_stack_data retval = last->data;
  last->prev->next = last->next;
  last->next->prev = last->prev;
  free(last);
  return retval;
}

/* create new stack dynamically, and give it valid initial values */
static grp_stack* new_grp_stack(void) {
  grp_stack* retval = (grp_stack*) malloc(sizeof(grp_stack));
  if(retval) {
    retval->stack_size = 0;
    retval->top = NULL;
  }
  return retval;
}

/* return the value at the top of the stack and remove it, updating
 * stack_size. top->prev becomes the new "top" */
static grp_stack_data pop_grp_stack(grp_stack* mystack) {
  grp_stack_data retval = grp_stack_default;
  if(mystack && mystack->top) {
    grp_stack_item* newtop = mystack->top->prev;
    retval = grp_stack_remove(mystack->top);
    mystack->top = newtop;
    if(0 == --mystack->stack_size) mystack->top = NULL;
  }
  return retval;
}

/* add to the stack after the top element. the added element becomes
 * the new "top" */
static void push_grp_stack(grp_stack* mystack, grp_stack_data data) {
  if(!mystack) return;
  mystack->top = grp_stack_append(mystack->top, data);
  ++mystack->stack_size;
  return;
}

/* return the value at the bottom of the stack and remove it, updating
 * stack_size. "top" pointer is unaffected */
static grp_stack_data shift_grp_stack(grp_stack* mystack) {
  grp_stack_data retval = grp_stack_default;
  if(mystack && mystack->top) {
    retval = grp_stack_remove(mystack->top->next); /* top->next == bottom */
    if(0 == --mystack->stack_size) mystack->top = NULL;
  }
  return retval;
}

/* add to the stack after the top element. "top" is unaffected, except
 * in the special case of an initially empty stack */
/* static void unshift_grp_stack(grp_stack* mystack, grp_stack_data data) {
   if(!mystack) return;
   if(mystack->top) grp_stack_append(mystack->top, data);
   else mystack->top = grp_stack_append(NULL, data);
   ++mystack->stack_size;
   return;
   } */

/*--------------------------------------------------------------------------*/
int fits_url2relurl(char     *refURL, /* I reference URL string             */
		    char     *absURL, /* I absoulute URL string to process  */
		    char     *relURL, /* O resulting relative URL string    */
		    int      *status)
/*
  create a relative URL to the file referenced by absURL with respect to the
  reference URL refURL. The relative URL is returned in relURL.

  Both refURL and absURL must be absolute URL strings; i.e. either begin
  with an access method specification "XXX://" or with a '/' character
  signifiying that they are absolute file paths.

  Note that it is possible to make a relative URL from two input URLs
  (absURL and refURL) that are not compatable. This function does not
  check to see if the resulting relative URL makes any sence. For instance,
  it is impossible to make a relative URL from the following two inputs:

  absURL = ftp://a.b.c.com/x/y/z/foo.fits
  refURL = /a/b/c/ttt.fits

  The resulting relURL will be:

  ../../../ftp://a.b.c.com/x/y/z/foo.fits 

  Which is syntically correct but meaningless. The problem is that a file
  with an access method of ftp:// cannot be expressed a a relative URL to
  a local disk file.
*/

{
  int i,j;
  int refcount,abscount;
  int refsize,abssize;
  int done;


  if(*status != 0) return(*status);

  /* initialize the relative URL string */
  relURL[0] = 0;

  do
    {
      /*
	refURL and absURL must be absolute to process
      */

      if(!(fits_is_url_absolute(refURL) || *refURL == '/') ||
	 !(fits_is_url_absolute(absURL) || *absURL == '/'))
	{
	  *status = URL_PARSE_ERROR;
	  ffpmsg("Cannot make rel. URL from non abs. URLs (fits_url2relurl)");
	  continue;
	}

      /* determine the size of the refURL and absURL strings */

      refsize = strlen(refURL);
      abssize = strlen(absURL);

      /* process the two URL strings and build the relative URL between them */
		

      for(done = 0, refcount = 0, abscount = 0; 
	  !done && refcount < refsize && abscount < abssize; 
	  ++refcount, ++abscount)
	{
	  for(; abscount < abssize && absURL[abscount] == '/'; ++abscount);
	  for(; refcount < refsize && refURL[refcount] == '/'; ++refcount);

	  /* find the next path segment in absURL */ 
	  for(i = abscount; absURL[i] != '/' && i < abssize; ++i);
	  
	  /* find the next path segment in refURL */
	  for(j = refcount; refURL[j] != '/' && j < refsize; ++j);
	  
	  /* do the two path segments match? */
	  if(i == j && 
	     strncmp(absURL+abscount, refURL+refcount,i-refcount) == 0)
	    {
	      /* they match, so ignore them and continue */
	      abscount = i; refcount = j;
	      continue;
	    }
	  
	  /* We found a difference in the paths in refURL and absURL.
	     For every path segment remaining in the refURL string, append
	     a "../" path segment to the relataive URL relURL.
	  */

	  for(j = refcount; j < refsize; ++j)
	    if(refURL[j] == '/') strcat(relURL,"../");
	  
	  /* copy all remaining characters of absURL to the output relURL */

	  strcat(relURL,absURL+abscount);
	  
	  /* we are done building the relative URL */
	  done = 1;
	}

    }while(0);

  return(*status);
}
/*--------------------------------------------------------------------------*/
int fits_relurl2url(char     *refURL, /* I reference URL string             */
		    char     *relURL, /* I relative URL string to process   */
		    char     *absURL, /* O absolute URL string              */
		    int      *status)
/*
  create an absolute URL from a relative url and a reference URL. The 
  reference URL is given by the FITS file pointed to by fptr.

  The construction of the absolute URL from the partial and reference URl
  is performed using the rules set forth in:
 
  http://www.w3.org/Addressing/URL/URL_TOC.html
  and
  http://www.w3.org/Addressing/URL/4_3_Partial.html

  Note that the relative URL string relURL must conform to the Unix-like
  URL syntax; host dependent partial URL strings are not allowed.
*/
{
  int i;

  char tmpStr[FLEN_FILENAME];

  char *tmpStr1, *tmpStr2;


  if(*status != 0) return(*status);
  
  do
    {

      /*
	make a copy of the reference URL string refURL for parsing purposes
      */

      strcpy(tmpStr,refURL);

      /*
	if the reference file has an access method of mem:// or shmem://
	then we cannot use it as the basis of an absolute URL construction
	for a partial URL
      */
	  
      if(fits_strncasecmp(tmpStr,"MEM:",4)   == 0 ||
                	                fits_strncasecmp(tmpStr,"SHMEM:",6) == 0)
	{
	  ffpmsg("ref URL has access mem:// or shmem:// (fits_relurl2url)");
	  ffpmsg("   cannot construct full URL from a partial URL and ");
	  ffpmsg("   MEM/SHMEM base URL");
	  *status = URL_PARSE_ERROR;
	  continue;
	}

      if(relURL[0] != '/')
	{
	  /*
	    just append the relative URL string to the reference URL
	    string (minus the reference URL file name) to form the 
	    absolute URL string
	  */
	      
	  tmpStr1 = strrchr(tmpStr,'/');
	  
	  if(tmpStr1 != NULL) tmpStr1[1] = 0;
	  else                tmpStr[0]  = 0;
	  
	  strcat(tmpStr,relURL);
	}
      else
	{
	  /*
	    have to parse the refURL string for the first occurnace of the 
	    same number of '/' characters as contained in the beginning of
	    location that is not followed by a greater number of consective 
	    '/' charaters (yes, that is a confusing statement); this is the 
	    location in the refURL string where the relURL string is to
	    be appended to form the new absolute URL string
	   */
	  
	  /*
	    first, build up a slash pattern string that has one more
	    slash in it than the starting slash pattern of the
	    relURL string
	  */
	  
	  strcpy(absURL,"/");
	  
	  for(i = 0; relURL[i] == '/'; ++i) strcat(absURL,"/");
	  
	  /*
	    loop over the refURL string until the slash pattern stored
	    in absURL is no longer found
	  */

	  for(tmpStr1 = tmpStr, i = strlen(absURL); 
	      (tmpStr2 = strstr(tmpStr1,absURL)) != NULL;
	      tmpStr1 = tmpStr2 + i);
	  
	  /* reduce the slash pattern string by one slash */
	  
	  absURL[i-1] = 0;
	  
	  /* 
	     search for the slash pattern in the remaining portion
	     of the refURL string
	  */

	  tmpStr2 = strstr(tmpStr1,absURL);
	  
	  /* if no slash pattern match was found */
	  
	  if(tmpStr2 == NULL)
	    {
	      /* just strip off the file name from the refURL  */
	      
	      tmpStr2 = strrchr(tmpStr1,'/');
	      
	      if(tmpStr2 != NULL) tmpStr2[0] = 0;
	      else                tmpStr[0]  = 0;
	    }
	  else
	    {
	      /* set a string terminator at the slash pattern match */
	      
	      *tmpStr2 = 0;
	    }
	  
	  /* 
	    conatenate the relURL string to the refURL string to form
	    the absURL
	   */

	  strcat(tmpStr,relURL);
	}

      /*
	normalize the absURL by removing any ".." or "." specifiers
	in the string
      */

      *status = fits_clean_url(tmpStr,absURL,status);

    }while(0);

  return(*status);
}
/*--------------------------------------------------------------------------*/
int fits_encode_url(char *inpath,  /* I URL  to be encoded                  */ 
		    char *outpath, /* O output encoded URL                  */
		    int *status)
     /*
       encode all URL "unsafe" and "reserved" characters using the "%XX"
       convention, where XX stand for the two hexidecimal digits of the
       encode character's ASCII code.

       Note that the output path is at least as large as, if not larger than
       the input path, so that OUTPATH should be passed to this function
       with room for growth. If not a runtime error could result. It is
       assumed that OUTPATH has been allocated with enough room to hold
       the resulting encoded URL.

       This function was adopted from code in the libwww.a library available
       via the W3 consortium <URL: http://www.w3.org>
     */
{
  unsigned char a;
  
  char *p;
  char *q;
  char *hex = "0123456789ABCDEF";
  
unsigned const char isAcceptable[96] =
{/* 0x0 0x1 0x2 0x3 0x4 0x5 0x6 0x7 0x8 0x9 0xA 0xB 0xC 0xD 0xE 0xF */
  
    0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0xF,0xE,0x0,0xF,0xF,0xC, 
                                           /* 2x  !"#$%&'()*+,-./   */
    0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0x8,0x0,0x0,0x0,0x0,0x0,
                                           /* 3x 0123456789:;<=>?   */
    0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF, 
                                           /* 4x @ABCDEFGHIJKLMNO   */
    0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0x0,0x0,0x0,0x0,0xF,
                                           /* 5X PQRSTUVWXYZ[\]^_   */
    0x0,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,
                                           /* 6x `abcdefghijklmno   */
    0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0x0,0x0,0x0,0x0,0x0  
                                           /* 7X pqrstuvwxyz{\}~DEL */
};

  if(*status != 0) return(*status);
  
  /* loop over all characters in inpath until '\0' is encountered */

  for(q = outpath, p = inpath; *p; p++)
    {
      a = (unsigned char)*p;

      /* if the charcter requires encoding then process it */

      if(!( a>=32 && a<128 && (isAcceptable[a-32])))
	{
	  /* add a '%' character to the outpath */
	  *q++ = HEX_ESCAPE;
	  /* add the most significant ASCII code hex value */
	  *q++ = hex[a >> 4];
	  /* add the least significant ASCII code hex value */
	  *q++ = hex[a & 15];
	}
      /* else just copy the character as is */
      else *q++ = *p;
    }

  /* null terminate the outpath string */

  *q++ = 0; 
  
  return(*status);
}

/*---------------------------------------------------------------------------*/
int fits_unencode_url(char *inpath,  /* I input URL with encoding            */
		      char *outpath, /* O unencoded URL                      */
		      int  *status)
     /*
       unencode all URL "unsafe" and "reserved" characters to their actual
       ASCII representation. All tokens of the form "%XX" where XX is the
       hexidecimal code for an ASCII character, are searched for and
       translated into the actuall ASCII character (so three chars become
       1 char).

       It is assumed that OUTPATH has enough room to hold the unencoded
       URL.

       This function was adopted from code in the libwww.a library available
       via the W3 consortium <URL: http://www.w3.org>
     */

{
    char *p;
    char *q;
    char  c;

    if(*status != 0) return(*status);

    p = inpath;
    q = outpath;

    /* 
       loop over all characters in the inpath looking for the '%' escape
       character; if found the process the escape sequence
    */

    while(*p != 0) 
      {
	/* 
	   if the character is '%' then unencode the sequence, else
	   just copy the character from inpath to outpath
        */

        if (*p == HEX_ESCAPE)
	  {
            if((c = *(++p)) != 0)
	      { 
		*q = (
		      (c >= '0' && c <= '9') ?
		      (c - '0') : ((c >= 'A' && c <= 'F') ?
				   (c - 'A' + 10) : (c - 'a' + 10))
		      )*16;

		if((c = *(++p)) != 0)
		  {
		    *q = *q + (
			       (c >= '0' && c <= '9') ? 
		               (c - '0') : ((c >= 'A' && c <= 'F') ? 
					    (c - 'A' + 10) : (c - 'a' + 10))
			       );
		    p++, q++;
		  }
	      }
	  } 
	else
	  *q++ = *p++; 
      }
 
    /* terminate the outpath */
    *q = 0;

    return(*status);   
}
/*---------------------------------------------------------------------------*/

int fits_is_url_absolute(char *url)
/*
  Return a True (1) or False (0) value indicating whether or not the passed
  URL string contains an access method specifier or not. Note that this is
  a boolean function and it neither reads nor returns the standard error
  status parameter
*/
{
  char *tmpStr1, *tmpStr2;

  char reserved[] = {':',';','/','?','@','&','=','+','$',','};

  /*
    The rule for determing if an URL is relative or absolute is that it (1)
    must have a colon ":" and (2) that the colon must appear before any other
    reserved URL character in the URL string. We first see if a colon exists,
    get its position in the string, and then check to see if any of the other
    reserved characters exists and if their position in the string is greater
    than that of the colons. 
   */

  if( (tmpStr1 = strchr(url,reserved[0])) != NULL                       &&
     ((tmpStr2 = strchr(url,reserved[1])) == NULL || tmpStr2 > tmpStr1) &&
     ((tmpStr2 = strchr(url,reserved[2])) == NULL || tmpStr2 > tmpStr1) &&
     ((tmpStr2 = strchr(url,reserved[3])) == NULL || tmpStr2 > tmpStr1) &&
     ((tmpStr2 = strchr(url,reserved[4])) == NULL || tmpStr2 > tmpStr1) &&
     ((tmpStr2 = strchr(url,reserved[5])) == NULL || tmpStr2 > tmpStr1) &&
     ((tmpStr2 = strchr(url,reserved[6])) == NULL || tmpStr2 > tmpStr1) &&
     ((tmpStr2 = strchr(url,reserved[7])) == NULL || tmpStr2 > tmpStr1) &&
     ((tmpStr2 = strchr(url,reserved[8])) == NULL || tmpStr2 > tmpStr1) &&
     ((tmpStr2 = strchr(url,reserved[9])) == NULL || tmpStr2 > tmpStr1)   )
    {
      return(1);
    }
  else
    {
      return(0);
    }
}
