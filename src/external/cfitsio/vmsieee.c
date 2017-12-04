#include <cvtdef.h>
#include <cvtmsg.h>

unsigned long CVT$CONVERT_FLOAT();

/* IEEVPAKR -- Pack a native floating point vector into an IEEE one.
*/
void ieevpr (unsigned int *native, unsigned int *ieee, int *nelem)
{
        unsigned long status;
        unsigned long options;
        unsigned int *unanval;
        int nanval = -1;
        int     i,n;

        unanval = (unsigned int *) &nanval;
        options = CVT$M_BIG_ENDIAN;
        
        n = *nelem;
        status = CVT$_NORMAL;

        for (i = 0; i < n ; i++) {

            status = CVT$CONVERT_FLOAT (&native[i], CVT$K_VAX_F,
                                        &ieee[i], CVT$K_IEEE_S, 
                                        options);
           if (status != CVT$_NORMAL) {
                 ieee[i] = *unanval;
           }
        }       

}
/* IEEVPAKD -- Pack a native double floating point vector into an IEEE one.
*/
void ieevpd (unsigned long *native, unsigned long *ieee, int *nelem)
{
        unsigned long status;
        unsigned long options;
        unsigned long *unanval;
        long nanval = -1;
        int     i,n;

        unanval = (unsigned long *) &nanval;
        options = CVT$M_BIG_ENDIAN;
        
        n = *nelem * 2;
        status = CVT$_NORMAL;

        for (i = 0; i < n ; i=i+2) {

            status = CVT$CONVERT_FLOAT (&native[i], CVT$K_VAX_D,
                                        &ieee[i], CVT$K_IEEE_T, 
                                        options);
           if (status != CVT$_NORMAL) {
                 ieee[i]   = *unanval;
                 ieee[i+1] = *unanval;
           }
        }       

}
/* IEEVUPKR -- Unpack an ieee vector into native single floating point vector.
*/
void ieevur (unsigned int *ieee, unsigned int *native, int *nelem)
{
        unsigned long status;
        unsigned long options;
        unsigned int *unanval;
        int nanval = -1;
        int     j,n;

        unanval = (unsigned int *) &nanval;
        options = CVT$M_ERR_UNDERFLOW+CVT$M_BIG_ENDIAN;
        
        n = *nelem;

        status = CVT$_NORMAL;

        for (j = 0; j < n ; j++) {
           status = CVT$CONVERT_FLOAT (&ieee[j], CVT$K_IEEE_S,
                                        &native[j], CVT$K_VAX_F, 
                                        options);
           if (status != CVT$_NORMAL)
              switch(status) {
              case CVT$_INVVAL:
              case CVT$_NEGINF:
              case CVT$_OVERFLOW:
              case CVT$_POSINF:
                 native[j]   = *unanval;
                 break;
              default:
                 native[j] = 0;             
              }
        }
}
/* IEEVUPKD -- Unpack an ieee vector into native double floating point vector.
*/
void ieevud (unsigned long *ieee, unsigned long *native, int *nelem)
{
        unsigned long status;
        unsigned long options;
        unsigned long *unanval;
        long nanval = -1;
        int     j,n;

        unanval = (unsigned long *) &nanval;
        options = CVT$M_BIG_ENDIAN + CVT$M_ERR_UNDERFLOW; 
        
        n = *nelem * 2;

        status = CVT$_NORMAL;

        for (j = 0; j < n ; j=j+2) {
           status = CVT$CONVERT_FLOAT (&ieee[j], CVT$K_IEEE_T,
                                        &native[j], CVT$K_VAX_D, 
                                        options);
           if (status != CVT$_NORMAL)
              switch(status) {
              case CVT$_INVVAL:
              case CVT$_NEGINF:
              case CVT$_OVERFLOW:
              case CVT$_POSINF:
                 native[j]   = *unanval;
                 native[j+1] = *unanval;
                 break;
              default:
                 native[j]   = 0;             
                 native[j+1] = 0;             
              }
        }
}
