/*
 *  Unit SGP4SDP4
 *           Author:  Dr TS Kelso 
 * Original Version:  1991 Oct 30
 * Current Revision:  1992 Sep 03
 *          Version:  1.50 
 *        Copyright:  1991-1992, All Rights Reserved 
 *
 *   Ported to C by:  Neoklis Kyriazis  April 10  2001
 */

#include "sgp4sdp4.h"

/* SGP4 */
/* This function is used to calculate the position and velocity */
/* of near-earth (period < 225 minutes) satellites. tsince is   */
/* time since epoch in minutes, tle is a pointer to a tle_t     */
/* structure with Keplerian orbital elements and pos and vel    */
/* are vector_t structures returning ECI satellite position and */
/* velocity. Use Convert_Sat_State() to convert to km and km/s.*/
void
SGP4(double tsince, tle_t *tle, vector_t *pos, vector_t *vel, double* phase)
{
  static double
    aodp,aycof,c1,c4,c5,cosio,d2,d3,d4,delmo,omgcof,
    eta,omgdot,sinio,xnodp,sinmo,t2cof,t3cof,t4cof,t5cof,
    x1mth2,x3thm1,x7thm1,xmcof,xmdot,xnodcf,xnodot,xlcof;
 
  double
    cosuk,sinuk,rfdotk,vx,vy,vz,ux,uy,uz,xmy,xmx,
    cosnok,sinnok,cosik,sinik,rdotk,xinck,xnodek,uk,
    rk,cos2u,sin2u,u,sinu,cosu,betal,rfdot,rdot,r,pl,
    elsq,esine,ecose,epw,cosepw,x1m5th,xhdot1,tfour,
    sinepw,capu,ayn,xlt,aynl,xll,axn,xn,beta,xl,e,a,
    tcube,delm,delomg,templ,tempe,tempa,xnode,tsq,xmp,
    omega,xnoddf,omgadf,xmdf,a1,a3ovk2,ao,betao,betao2,
    c1sq,c2,c3,coef,coef1,del1,delo,eeta,eosq,etasq,
    perige,pinvsq,psisq,qoms24,s4,temp,temp1,temp2,
    temp3,temp4,temp5,temp6,theta2,theta4,tsi;

  int i;  

  /* Initialization */
  if (isFlagClear(SGP4_INITIALIZED_FLAG))
    {
      SetFlag(SGP4_INITIALIZED_FLAG);

      /* Recover original mean motion (xnodp) and   */
      /* semimajor axis (aodp) from input elements. */
      a1 = pow(xke/tle->xno,tothrd);
      cosio = cos(tle->xincl);
      theta2 = cosio*cosio;
      x3thm1 = 3*theta2-1.0;
      eosq = tle->eo*tle->eo;
      betao2 = 1-eosq;
      betao = sqrt(betao2);
      del1 = 1.5*ck2*x3thm1/(a1*a1*betao*betao2);
      ao = a1*(1-del1*(0.5*tothrd+del1*(1+134/81*del1)));
      delo = 1.5*ck2*x3thm1/(ao*ao*betao*betao2);
      xnodp = tle->xno/(1+delo);
      aodp = ao/(1-delo);

      /* For perigee less than 220 kilometers, the "simple" flag is set */
      /* and the equations are truncated to linear variation in sqrt a  */
      /* and quadratic variation in mean anomaly.  Also, the c3 term,   */
      /* the delta omega term, and the delta m term are dropped.        */
      if((aodp*(1-tle->eo)/ae) < (220/xkmper+ae))
	SetFlag(SIMPLE_FLAG);
      else
	ClearFlag(SIMPLE_FLAG);

      /* For perigee below 156 km, the       */ 
      /* values of s and qoms2t are altered. */
      s4 = __s__;
      qoms24 = qoms2t;
      perige = (aodp*(1-tle->eo)-ae)*xkmper;
      if(perige < 156)
	{
       	  if(perige <= 98)
	    s4 = 20;
          else
	    s4 = perige-78;
	  qoms24 = pow((120-s4)*ae/xkmper,4);
	  s4 = s4/xkmper+ae;
	}; /* End of if(perige <= 98) */

      pinvsq = 1/(aodp*aodp*betao2*betao2);
      tsi = 1/(aodp-s4);
      eta = aodp*tle->eo*tsi;
      etasq = eta*eta;
      eeta = tle->eo*eta;
      psisq = fabs(1-etasq);
      coef = qoms24*pow(tsi,4);
      coef1 = coef/pow(psisq,3.5);
      c2 = coef1*xnodp*(aodp*(1+1.5*etasq+eeta*(4+etasq))+
	   0.75*ck2*tsi/psisq*x3thm1*(8+3*etasq*(8+etasq)));
      c1 = tle->bstar*c2;
      sinio = sin(tle->xincl);
      a3ovk2 = -xj3/ck2*pow(ae,3);
      c3 = coef*tsi*a3ovk2*xnodp*ae*sinio/tle->eo;
      x1mth2 = 1-theta2;
      c4 = 2*xnodp*coef1*aodp*betao2*(eta*(2+0.5*etasq)+
	   tle->eo*(0.5+2*etasq)-2*ck2*tsi/(aodp*psisq)*
	   (-3*x3thm1*(1-2*eeta+etasq*(1.5-0.5*eeta))+0.75*
	   x1mth2*(2*etasq-eeta*(1+etasq))*cos(2*tle->omegao)));
      c5 = 2*coef1*aodp*betao2*(1+2.75*(etasq+eeta)+eeta*etasq);
      theta4 = theta2*theta2;
      temp1 = 3*ck2*pinvsq*xnodp;
      temp2 = temp1*ck2*pinvsq;
      temp3 = 1.25*ck4*pinvsq*pinvsq*xnodp;
      xmdot = xnodp+0.5*temp1*betao*x3thm1+
	      0.0625*temp2*betao*(13-78*theta2+137*theta4);
      x1m5th = 1-5*theta2;
      omgdot = -0.5*temp1*x1m5th+0.0625*temp2*(7-114*theta2+
	       395*theta4)+temp3*(3-36*theta2+49*theta4);
      xhdot1 = -temp1*cosio;
      xnodot = xhdot1+(0.5*temp2*(4-19*theta2)+
	       2*temp3*(3-7*theta2))*cosio;
      omgcof = tle->bstar*c3*cos(tle->omegao);
      xmcof = -tothrd*coef*tle->bstar*ae/eeta;
      xnodcf = 3.5*betao2*xhdot1*c1;
      t2cof = 1.5*c1;
      xlcof = 0.125*a3ovk2*sinio*(3+5*cosio)/(1+cosio);
      aycof = 0.25*a3ovk2*sinio;
      delmo = pow(1+eta*cos(tle->xmo),3);
      sinmo = sin(tle->xmo);
      x7thm1 = 7*theta2-1;
      if (isFlagClear(SIMPLE_FLAG))
	{
	  c1sq = c1*c1;
	  d2 = 4*aodp*tsi*c1sq;
	  temp = d2*tsi*c1/3;
	  d3 = (17*aodp+s4)*temp;
	  d4 = 0.5*temp*aodp*tsi*(221*aodp+31*s4)*c1;
	  t3cof = d2+2*c1sq;
	  t4cof = 0.25*(3*d3+c1*(12*d2+10*c1sq));
	  t5cof = 0.2*(3*d4+12*c1*d3+6*d2*d2+15*c1sq*(2*d2+c1sq));
	}; /* End of if (isFlagClear(SIMPLE_FLAG)) */
    }; /* End of SGP4() initialization */

  /* Update for secular gravity and atmospheric drag. */
  xmdf = tle->xmo+xmdot*tsince;
  omgadf = tle->omegao+omgdot*tsince;
  xnoddf = tle->xnodeo+xnodot*tsince;
  omega = omgadf;
  xmp = xmdf;
  tsq = tsince*tsince;
  xnode = xnoddf+xnodcf*tsq;
  tempa = 1-c1*tsince;
  tempe = tle->bstar*c4*tsince;
  templ = t2cof*tsq;
  if (isFlagClear(SIMPLE_FLAG))
    {
      delomg = omgcof*tsince;
      delm = xmcof*(pow(1+eta*cos(xmdf),3)-delmo);
      temp = delomg+delm;
      xmp = xmdf+temp;
      omega = omgadf-temp;
      tcube = tsq*tsince;
      tfour = tsince*tcube;
      tempa = tempa-d2*tsq-d3*tcube-d4*tfour;
      tempe = tempe+tle->bstar*c5*(sin(xmp)-sinmo);
      templ = templ+t3cof*tcube+tfour*(t4cof+tsince*t5cof);
    }; /* End of if (isFlagClear(SIMPLE_FLAG)) */

  a = aodp*pow(tempa,2);
  e = tle->eo-tempe;
  xl = xmp+omega+xnode+xnodp*templ;
  beta = sqrt(1-e*e);
  xn = xke/pow(a,1.5);

  /* Long period periodics */
  axn = e*cos(omega);
  temp = 1/(a*beta*beta);
  xll = temp*xlcof*axn;
  aynl = temp*aycof;
  xlt = xl+xll;
  ayn = e*sin(omega)+aynl;

  /* Solve Kepler's' Equation */
  capu = FMod2p(xlt-xnode);
  temp2 = capu;

  i = 0;
  do
    {
      sinepw = sin(temp2);
      cosepw = cos(temp2);
      temp3 = axn*sinepw;
      temp4 = ayn*cosepw;
      temp5 = axn*cosepw;
      temp6 = ayn*sinepw;
      epw = (capu-temp4+temp3-temp2)/(1-temp5-temp6)+temp2;
      if(fabs(epw-temp2) <= e6a) break;
      temp2 = epw;
    }
  while( i++ < 10 );

  /* Short period preliminary quantities */
  ecose = temp5+temp6;
  esine = temp3-temp4;
  elsq = axn*axn+ayn*ayn;
  temp = 1-elsq;
  pl = a*temp;
  r = a*(1-ecose);
  temp1 = 1/r;
  rdot = xke*sqrt(a)*esine*temp1;
  rfdot = xke*sqrt(pl)*temp1;
  temp2 = a*temp1;
  betal = sqrt(temp);
  temp3 = 1/(1+betal);
  cosu = temp2*(cosepw-axn+ayn*esine*temp3);
  sinu = temp2*(sinepw-ayn-axn*esine*temp3);
  u = AcTan(sinu, cosu);
  sin2u = 2*sinu*cosu;
  cos2u = 2*cosu*cosu-1;
  temp = 1/pl;
  temp1 = ck2*temp;
  temp2 = temp1*temp;

  /* Update for short periodics */
  rk = r*(1-1.5*temp2*betal*x3thm1)+0.5*temp1*x1mth2*cos2u;
  uk = u-0.25*temp2*x7thm1*sin2u;
  xnodek = xnode+1.5*temp2*cosio*sin2u;
  xinck = tle->xincl+1.5*temp2*cosio*sinio*cos2u;
  rdotk = rdot-xn*temp1*x1mth2*sin2u;
  rfdotk = rfdot+xn*temp1*(x1mth2*cos2u+1.5*x3thm1);

  /* Orientation vectors */
  sinuk = sin(uk);
  cosuk = cos(uk);
  sinik = sin(xinck);
  cosik = cos(xinck);
  sinnok = sin(xnodek);
  cosnok = cos(xnodek);
  xmx = -sinnok*cosik;
  xmy = cosnok*cosik;
  ux = xmx*sinuk+cosnok*cosuk;
  uy = xmy*sinuk+sinnok*cosuk;
  uz = sinik*sinuk;
  vx = xmx*cosuk-cosnok*sinuk;
  vy = xmy*cosuk-sinnok*sinuk;
  vz = sinik*cosuk;

  /* Position and velocity */
  pos->x = rk*ux;
  pos->y = rk*uy;
  pos->z = rk*uz;
  vel->x = rdotk*ux+rfdotk*vx;
  vel->y = rdotk*uy+rfdotk*vy;
  vel->z = rdotk*uz+rfdotk*vz;

  *phase = xlt-xnode-omgadf+twopi;
  if(*phase < 0) *phase += twopi;
  *phase = FMod2p(*phase);

  tle->omegao1=omega;
  tle->xincl1=xinck;
  tle->xnodeo1=xnodek;

} /*SGP4*/

/*------------------------------------------------------------------*/

/* SDP4 */
/* This function is used to calculate the position and velocity */
/* of deep-space (period > 225 minutes) satellites. tsince is   */
/* time since epoch in minutes, tle is a pointer to a tle_t     */
/* structure with Keplerian orbital elements and pos and vel    */
/* are vector_t structures returning ECI satellite position and */
/* velocity. Use Convert_Sat_State() to convert to km and km/s. */
void 
SDP4(double tsince, tle_t *tle, vector_t *pos, vector_t *vel, double* phase)
{
  int i;

  static double
    x3thm1,c1,x1mth2,c4,xnodcf,t2cof,xlcof,aycof,x7thm1;

  double
    a,axn,ayn,aynl,beta,betal,capu,cos2u,cosepw,cosik,
    cosnok,cosu,cosuk,ecose,elsq,epw,esine,pl,theta4,
    rdot,rdotk,rfdot,rfdotk,rk,sin2u,sinepw,sinik,
    sinnok,sinu,sinuk,tempe,templ,tsq,u,uk,ux,uy,uz,
    vx,vy,vz,xinck,xl,xlt,xmam,xmdf,xmx,xmy,xnoddf,
    xnodek,xll,a1,a3ovk2,ao,c2,coef,coef1,x1m5th,
    xhdot1,del1,r,delo,eeta,eta,etasq,perige,
    psisq,tsi,qoms24,s4,pinvsq,temp,tempa,temp1,
    temp2,temp3,temp4,temp5,temp6;

  static deep_arg_t deep_arg;

  /* Initialization */
  if (isFlagClear(SDP4_INITIALIZED_FLAG))
    {
      SetFlag(SDP4_INITIALIZED_FLAG);

      /* Recover original mean motion (xnodp) and   */
      /* semimajor axis (aodp) from input elements. */
      a1 = pow(xke/tle->xno,tothrd);
      deep_arg.cosio = cos(tle->xincl);
      deep_arg.theta2 = deep_arg.cosio*deep_arg.cosio;
      x3thm1 = 3*deep_arg.theta2-1;
      deep_arg.eosq = tle->eo*tle->eo;
      deep_arg.betao2 = 1-deep_arg.eosq;
      deep_arg.betao = sqrt(deep_arg.betao2);
      del1 = 1.5*ck2*x3thm1/(a1*a1*deep_arg.betao*deep_arg.betao2);
      ao = a1*(1-del1*(0.5*tothrd+del1*(1+134/81*del1)));
      delo = 1.5*ck2*x3thm1/(ao*ao*deep_arg.betao*deep_arg.betao2);
      deep_arg.xnodp = tle->xno/(1+delo);
      deep_arg.aodp = ao/(1-delo);

      /* For perigee below 156 km, the values */
      /* of s and qoms2t are altered.         */
      s4 = __s__;
      qoms24 = qoms2t;
      perige = (deep_arg.aodp*(1-tle->eo)-ae)*xkmper;
      if(perige < 156)
	{
       	  if(perige <= 98)
	    s4 = 20;
          else
	    s4 = perige-78;
	  qoms24 = pow((120-s4)*ae/xkmper,4);
	  s4 = s4/xkmper+ae;
	}
      pinvsq = 1/(deep_arg.aodp*deep_arg.aodp*
               deep_arg.betao2*deep_arg.betao2);
      deep_arg.sing = sin(tle->omegao);
      deep_arg.cosg = cos(tle->omegao);
      tsi = 1/(deep_arg.aodp-s4);
      eta = deep_arg.aodp*tle->eo*tsi;
      etasq = eta*eta;
      eeta = tle->eo*eta;
      psisq = fabs(1-etasq);
      coef = qoms24*pow(tsi,4);
      coef1 = coef/pow(psisq,3.5);
      c2 = coef1*deep_arg.xnodp*(deep_arg.aodp*(1+1.5*etasq+eeta*
	   (4+etasq))+0.75*ck2*tsi/psisq*x3thm1*(8+3*etasq*(8+etasq)));
      c1 = tle->bstar*c2;
      deep_arg.sinio = sin(tle->xincl);
      a3ovk2 = -xj3/ck2*pow(ae,3);
      x1mth2 = 1-deep_arg.theta2;
      c4 = 2*deep_arg.xnodp*coef1*deep_arg.aodp*deep_arg.betao2*
           (eta*(2+0.5*etasq)+tle->eo*(0.5+2*etasq)-2*ck2*tsi/
           (deep_arg.aodp*psisq)*(-3*x3thm1*(1-2*eeta+etasq*
           (1.5-0.5*eeta))+0.75*x1mth2*(2*etasq-eeta*(1+etasq))*
           cos(2*tle->omegao)));
      theta4 = deep_arg.theta2*deep_arg.theta2;
      temp1 = 3*ck2*pinvsq*deep_arg.xnodp;
      temp2 = temp1*ck2*pinvsq;
      temp3 = 1.25*ck4*pinvsq*pinvsq*deep_arg.xnodp;
      deep_arg.xmdot = deep_arg.xnodp+0.5*temp1*deep_arg.betao*
	               x3thm1+0.0625*temp2*deep_arg.betao*
                       (13-78*deep_arg.theta2+137*theta4);
      x1m5th = 1-5*deep_arg.theta2;
      deep_arg.omgdot = -0.5*temp1*x1m5th+0.0625*temp2*
                        (7-114*deep_arg.theta2+395*theta4)+
	                temp3*(3-36*deep_arg.theta2+49*theta4);
      xhdot1 = -temp1*deep_arg.cosio;
      deep_arg.xnodot = xhdot1+(0.5*temp2*(4-19*deep_arg.theta2)+
		        2*temp3*(3-7*deep_arg.theta2))*deep_arg.cosio;
      xnodcf = 3.5*deep_arg.betao2*xhdot1*c1;
      t2cof = 1.5*c1;
      xlcof = 0.125*a3ovk2*deep_arg.sinio*(3+5*deep_arg.cosio)/
              (1+deep_arg.cosio);
      aycof = 0.25*a3ovk2*deep_arg.sinio;
      x7thm1 = 7*deep_arg.theta2-1;

      /* initialize Deep() */
      Deep(dpinit, tle, &deep_arg);
    }; /*End of SDP4() initialization */

  /* Update for secular gravity and atmospheric drag */
  xmdf = tle->xmo+deep_arg.xmdot*tsince;
  deep_arg.omgadf = tle->omegao+deep_arg.omgdot*tsince;
  xnoddf = tle->xnodeo+deep_arg.xnodot*tsince;
  tsq = tsince*tsince;
  deep_arg.xnode = xnoddf+xnodcf*tsq;
  tempa = 1-c1*tsince;
  tempe = tle->bstar*c4*tsince;
  templ = t2cof*tsq;
  deep_arg.xn = deep_arg.xnodp;

  /* Update for deep-space secular effects */
  deep_arg.xll = xmdf;
  deep_arg.t = tsince;

  Deep(dpsec, tle, &deep_arg);

  xmdf = deep_arg.xll;
  a = pow(xke/deep_arg.xn,tothrd)*tempa*tempa;
  deep_arg.em = deep_arg.em-tempe;
  xmam = xmdf+deep_arg.xnodp*templ;

  /* Update for deep-space periodic effects */
  deep_arg.xll = xmam;

  Deep(dpper, tle, &deep_arg);

  xmam = deep_arg.xll;
  xl = xmam+deep_arg.omgadf+deep_arg.xnode;
  beta = sqrt(1-deep_arg.em*deep_arg.em);
  deep_arg.xn = xke/pow(a,1.5);

  /* Long period periodics */
  axn = deep_arg.em*cos(deep_arg.omgadf);
  temp = 1/(a*beta*beta);
  xll = temp*xlcof*axn;
  aynl = temp*aycof;
  xlt = xl+xll;
  ayn = deep_arg.em*sin(deep_arg.omgadf)+aynl;

  /* Solve Kepler's Equation */
  capu = FMod2p(xlt-deep_arg.xnode);
  temp2 = capu;

  i = 0;
  do
    {
      sinepw = sin(temp2);
      cosepw = cos(temp2);
      temp3 = axn*sinepw;
      temp4 = ayn*cosepw;
      temp5 = axn*cosepw;
      temp6 = ayn*sinepw;
      epw = (capu-temp4+temp3-temp2)/(1-temp5-temp6)+temp2;
      if(fabs(epw-temp2) <= e6a) break;
      temp2 = epw;
    }
  while( i++ < 10 );

  /* Short period preliminary quantities */
  ecose = temp5+temp6;
  esine = temp3-temp4;
  elsq = axn*axn+ayn*ayn;
  temp = 1-elsq;
  pl = a*temp;
  r = a*(1-ecose);
  temp1 = 1/r;
  rdot = xke*sqrt(a)*esine*temp1;
  rfdot = xke*sqrt(pl)*temp1;
  temp2 = a*temp1;
  betal = sqrt(temp);
  temp3 = 1/(1+betal);
  cosu = temp2*(cosepw-axn+ayn*esine*temp3);
  sinu = temp2*(sinepw-ayn-axn*esine*temp3);
  u = AcTan(sinu,cosu);
  sin2u = 2*sinu*cosu;
  cos2u = 2*cosu*cosu-1;
  temp = 1/pl;
  temp1 = ck2*temp;
  temp2 = temp1*temp;

  /* Update for short periodics */
  rk = r*(1-1.5*temp2*betal*x3thm1)+0.5*temp1*x1mth2*cos2u;
  uk = u-0.25*temp2*x7thm1*sin2u;
  xnodek = deep_arg.xnode+1.5*temp2*deep_arg.cosio*sin2u;
  xinck = deep_arg.xinc+1.5*temp2*deep_arg.cosio*deep_arg.sinio*cos2u;
  rdotk = rdot-deep_arg.xn*temp1*x1mth2*sin2u;
  rfdotk = rfdot+deep_arg.xn*temp1*(x1mth2*cos2u+1.5*x3thm1);

  /* Orientation vectors */
  sinuk = sin(uk);
  cosuk = cos(uk);
  sinik = sin(xinck);
  cosik = cos(xinck);
  sinnok = sin(xnodek);
  cosnok = cos(xnodek);
  xmx = -sinnok*cosik;
  xmy = cosnok*cosik;
  ux = xmx*sinuk+cosnok*cosuk;
  uy = xmy*sinuk+sinnok*cosuk;
  uz = sinik*sinuk;
  vx = xmx*cosuk-cosnok*sinuk;
  vy = xmy*cosuk-sinnok*sinuk;
  vz = sinik*cosuk;

  /* Position and velocity */
  pos->x = rk*ux;
  pos->y = rk*uy;
  pos->z = rk*uz;
  vel->x = rdotk*ux+rfdotk*vx;
  vel->y = rdotk*uy+rfdotk*vy;
  vel->z = rdotk*uz+rfdotk*vz;

 /* Phase in rads */
  *phase = xlt-deep_arg.xnode-deep_arg.omgadf+twopi;
  if(*phase < 0) *phase += twopi;
  *phase = FMod2p(*phase);

  tle->omegao1=deep_arg.omgadf;
  tle->xincl1=deep_arg.xinc;
  tle->xnodeo1=deep_arg.xnode;
} /* SDP4 */

/*------------------------------------------------------------------*/

/* DEEP */
/* This function is used by SDP4 to add lunar and solar */
/* perturbation effects to deep-space orbit objects.    */
void
Deep(int ientry, tle_t *tle, deep_arg_t *deep_arg)
{
  static double
    thgr,xnq,xqncl,omegaq,zmol,zmos,savtsn,ee2,e3,xi2,
    xl2,xl3,xl4,xgh2,xgh3,xgh4,xh2,xh3,sse,ssi,ssg,xi3,
    se2,si2,sl2,sgh2,sh2,se3,si3,sl3,sgh3,sh3,sl4,sgh4,
    ssl,ssh,d3210,d3222,d4410,d4422,d5220,d5232,d5421,
    d5433,del1,del2,del3,fasx2,fasx4,fasx6,xlamo,xfact,
    xni,atime,stepp,stepn,step2,preep,pl,sghs,xli,
    d2201,d2211,sghl,sh1,pinc,pe,shs,zsingl,zcosgl,
    zsinhl,zcoshl,zsinil,zcosil;

  double
    a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,ainv2,alfdp,aqnv,
    sgh,sini2,sinis,sinok,sh,si,sil,day,betdp,dalf,
    bfact,c,cc,cosis,cosok,cosq,ctem,f322,zx,zy,
    dbet,dls,eoc,eq,f2,f220,f221,f3,f311,f321,xnoh,
    f330,f441,f442,f522,f523,f542,f543,g200,g201,
    g211,pgh,ph,s1,s2,s3,s4,s5,s6,s7,se,sel,ses,xls,
    g300,g310,g322,g410,g422,g520,g521,g532,g533,gam,
    sinq,sinzf,sis,sl,sll,sls,stem,temp,temp1,x1,x2,
    x2li,x2omi,x3,x4,x5,x6,x7,x8,xl,xldot,xmao,xnddt,
    xndot,xno2,xnodce,xnoi,xomi,xpidot,z1,z11,z12,z13,
    z2,z21,z22,z23,z3,z31,z32,z33,ze,zf,zm,zmo,zn,
    zsing,zsinh,zsini,zcosg,zcosh,zcosi,delt=0,ft=0;

  switch(ientry)
    {
    case dpinit : /* Entrance for deep space initialization */
      thgr = ThetaG(tle->epoch, deep_arg);
      eq = tle->eo;
      xnq = deep_arg->xnodp;
      aqnv = 1/deep_arg->aodp;
      xqncl = tle->xincl;
      xmao = tle->xmo;
      xpidot = deep_arg->omgdot+deep_arg->xnodot;
      sinq = sin(tle->xnodeo);
      cosq = cos(tle->xnodeo);
      omegaq = tle->omegao;

      /* Initialize lunar solar terms */
      day = deep_arg->ds50+18261.5;  /*Days since 1900 Jan 0.5*/
      if (day != preep)
	{
	  preep = day;
	  xnodce = 4.5236020-9.2422029E-4*day;
	  stem = sin(xnodce);
	  ctem = cos(xnodce);
	  zcosil = 0.91375164-0.03568096*ctem;
	  zsinil = sqrt(1-zcosil*zcosil);
	  zsinhl = 0.089683511*stem/zsinil;
	  zcoshl = sqrt(1-zsinhl*zsinhl);
	  c = 4.7199672+0.22997150*day;
	  gam = 5.8351514+0.0019443680*day;
	  zmol = FMod2p(c-gam);
	  zx = 0.39785416*stem/zsinil;
	  zy = zcoshl*ctem+0.91744867*zsinhl*stem;
	  zx = AcTan(zx,zy);
	  zx = gam+zx-xnodce;
	  zcosgl = cos(zx);
	  zsingl = sin(zx);
	  zmos = 6.2565837+0.017201977*day;
	  zmos = FMod2p(zmos);
	} /* End if(day != preep) */

      /* Do solar terms */
      savtsn = 1E20;
      zcosg = zcosgs;
      zsing = zsings;
      zcosi = zcosis;
      zsini = zsinis;
      zcosh = cosq;
      zsinh = sinq;
      cc = c1ss;
      zn = zns;
      ze = zes;
      zmo = zmos;
      xnoi = 1/xnq;

      /* Loop breaks when Solar terms are done a second */
      /* time, after Lunar terms are initialized        */
      for(;;) 
	{
	  /* Solar terms done again after Lunar terms are done */
	  a1 = zcosg*zcosh+zsing*zcosi*zsinh;
	  a3 = -zsing*zcosh+zcosg*zcosi*zsinh;
	  a7 = -zcosg*zsinh+zsing*zcosi*zcosh;
	  a8 = zsing*zsini;
	  a9 = zsing*zsinh+zcosg*zcosi*zcosh;
	  a10 = zcosg*zsini;
	  a2 = deep_arg->cosio*a7+ deep_arg->sinio*a8;
	  a4 = deep_arg->cosio*a9+ deep_arg->sinio*a10;
	  a5 = -deep_arg->sinio*a7+ deep_arg->cosio*a8;
	  a6 = -deep_arg->sinio*a9+ deep_arg->cosio*a10;
	  x1 = a1*deep_arg->cosg+a2*deep_arg->sing;
	  x2 = a3*deep_arg->cosg+a4*deep_arg->sing;
	  x3 = -a1*deep_arg->sing+a2*deep_arg->cosg;
	  x4 = -a3*deep_arg->sing+a4*deep_arg->cosg;
	  x5 = a5*deep_arg->sing;
	  x6 = a6*deep_arg->sing;
	  x7 = a5*deep_arg->cosg;
	  x8 = a6*deep_arg->cosg;
	  z31 = 12*x1*x1-3*x3*x3;
	  z32 = 24*x1*x2-6*x3*x4;
	  z33 = 12*x2*x2-3*x4*x4;
	  z1 = 3*(a1*a1+a2*a2)+z31*deep_arg->eosq;
	  z2 = 6*(a1*a3+a2*a4)+z32*deep_arg->eosq;
	  z3 = 3*(a3*a3+a4*a4)+z33*deep_arg->eosq;
	  z11 = -6*a1*a5+deep_arg->eosq*(-24*x1*x7-6*x3*x5);
	  z12 = -6*(a1*a6+a3*a5)+ deep_arg->eosq*
                (-24*(x2*x7+x1*x8)-6*(x3*x6+x4*x5));
	  z13 = -6*a3*a6+deep_arg->eosq*(-24*x2*x8-6*x4*x6);
	  z21 = 6*a2*a5+deep_arg->eosq*(24*x1*x5-6*x3*x7);
	  z22 = 6*(a4*a5+a2*a6)+ deep_arg->eosq*
                (24*(x2*x5+x1*x6)-6*(x4*x7+x3*x8));
	  z23 = 6*a4*a6+deep_arg->eosq*(24*x2*x6-6*x4*x8);
	  z1 = z1+z1+deep_arg->betao2*z31;
	  z2 = z2+z2+deep_arg->betao2*z32;
	  z3 = z3+z3+deep_arg->betao2*z33;
	  s3 = cc*xnoi;
	  s2 = -0.5*s3/deep_arg->betao;
	  s4 = s3*deep_arg->betao;
	  s1 = -15*eq*s4;
	  s5 = x1*x3+x2*x4;
	  s6 = x2*x3+x1*x4;
	  s7 = x2*x4-x1*x3;
	  se = s1*zn*s5;
	  si = s2*zn*(z11+z13);
	  sl = -zn*s3*(z1+z3-14-6*deep_arg->eosq);
	  sgh = s4*zn*(z31+z33-6);
	  sh = -zn*s2*(z21+z23);
	  if (xqncl < 5.2359877E-2) sh = 0;
	  ee2 = 2*s1*s6;
	  e3 = 2*s1*s7;
	  xi2 = 2*s2*z12;
	  xi3 = 2*s2*(z13-z11);
	  xl2 = -2*s3*z2;
	  xl3 = -2*s3*(z3-z1);
	  xl4 = -2*s3*(-21-9*deep_arg->eosq)*ze;
	  xgh2 = 2*s4*z32;
	  xgh3 = 2*s4*(z33-z31);
	  xgh4 = -18*s4*ze;
	  xh2 = -2*s2*z22;
	  xh3 = -2*s2*(z23-z21);

	  if(isFlagSet(LUNAR_TERMS_DONE_FLAG)) break;

	  /* Do lunar terms */
	  sse = se;
	  ssi = si;
	  ssl = sl;
	  ssh = sh/deep_arg->sinio;
	  ssg = sgh-deep_arg->cosio*ssh;
	  se2 = ee2;
	  si2 = xi2;
	  sl2 = xl2;
	  sgh2 = xgh2;
	  sh2 = xh2;
	  se3 = e3;
	  si3 = xi3;
	  sl3 = xl3;
	  sgh3 = xgh3;
	  sh3 = xh3;
	  sl4 = xl4;
	  sgh4 = xgh4;
	  zcosg = zcosgl;
	  zsing = zsingl;
	  zcosi = zcosil;
	  zsini = zsinil;
	  zcosh = zcoshl*cosq+zsinhl*sinq;
	  zsinh = sinq*zcoshl-cosq*zsinhl;
	  zn = znl;
	  cc = c1l;
	  ze = zel;
	  zmo = zmol;
	  SetFlag(LUNAR_TERMS_DONE_FLAG);
	} /* End of for(;;) */

      sse = sse+se;
      ssi = ssi+si;
      ssl = ssl+sl;
      ssg = ssg+sgh-deep_arg->cosio/deep_arg->sinio*sh;
      ssh = ssh+sh/deep_arg->sinio;

      /* Geopotential resonance initialization for 12 hour orbits */
      ClearFlag(RESONANCE_FLAG);
      ClearFlag(SYNCHRONOUS_FLAG);

      if( !((xnq < 0.0052359877) && (xnq > 0.0034906585)) )
	{
	  if( (xnq < 0.00826) || (xnq > 0.00924) ) return;
	  if (eq < 0.5) return;
	  SetFlag(RESONANCE_FLAG);
	  eoc = eq*deep_arg->eosq;
	  g201 = -0.306-(eq-0.64)*0.440;
	  if (eq <= 0.65)
	    {
	      g211 = 3.616-13.247*eq+16.290*deep_arg->eosq;
	      g310 = -19.302+117.390*eq-228.419*
                      deep_arg->eosq+156.591*eoc;
	      g322 = -18.9068+109.7927*eq-214.6334*
                     deep_arg->eosq+146.5816*eoc;
	      g410 = -41.122+242.694*eq-471.094*
                     deep_arg->eosq+313.953*eoc;
	      g422 = -146.407+841.880*eq-1629.014*
                     deep_arg->eosq+1083.435*eoc;
	      g520 = -532.114+3017.977*eq-5740*
                     deep_arg->eosq+3708.276*eoc;
	    }
	  else
	    {
	      g211 = -72.099+331.819*eq-508.738*
                     deep_arg->eosq+266.724*eoc;
	      g310 = -346.844+1582.851*eq-2415.925*
                     deep_arg->eosq+1246.113*eoc;
	      g322 = -342.585+1554.908*eq-2366.899*
                     deep_arg->eosq+1215.972*eoc;
	      g410 = -1052.797+4758.686*eq-7193.992*
                     deep_arg->eosq+3651.957*eoc;
	      g422 = -3581.69+16178.11*eq-24462.77*
                     deep_arg->eosq+ 12422.52*eoc;
	      if (eq <= 0.715)
		g520 = 1464.74-4664.75*eq+3763.64*deep_arg->eosq;
	      else
		g520 = -5149.66+29936.92*eq-54087.36*
                       deep_arg->eosq+31324.56*eoc;
	    } /* End if (eq <= 0.65) */

	  if (eq < 0.7)
	    {
	      g533 = -919.2277+4988.61*eq-9064.77*
                     deep_arg->eosq+5542.21*eoc;
	      g521 = -822.71072+4568.6173*eq-8491.4146*
                     deep_arg->eosq+5337.524*eoc;
	      g532 = -853.666+4690.25*eq-8624.77*
                     deep_arg->eosq+ 5341.4*eoc;
	    }
	  else
	    {
	      g533 = -37995.78+161616.52*eq-229838.2*
                     deep_arg->eosq+109377.94*eoc;
	      g521 = -51752.104+218913.95*eq-309468.16*
                     deep_arg->eosq+146349.42*eoc;
	     g532 = -40023.88+170470.89*eq-242699.48*
                    deep_arg->eosq+115605.82*eoc;
	    } /* End if (eq <= 0.7) */

	  sini2 = deep_arg->sinio*deep_arg->sinio;
	  f220 = 0.75*(1+2*deep_arg->cosio+deep_arg->theta2);
	  f221 = 1.5*sini2;
	  f321 = 1.875*deep_arg->sinio*(1-2*\
                 deep_arg->cosio-3*deep_arg->theta2);
	  f322 = -1.875*deep_arg->sinio*(1+2*
                 deep_arg->cosio-3*deep_arg->theta2);
	  f441 = 35*sini2*f220;
	  f442 = 39.3750*sini2*sini2;
	  f522 = 9.84375*deep_arg->sinio*(sini2*(1-2*deep_arg->cosio-5*
		 deep_arg->theta2)+0.33333333*(-2+4*deep_arg->cosio+
		 6*deep_arg->theta2));
	  f523 = deep_arg->sinio*(4.92187512*sini2*(-2-4*
		 deep_arg->cosio+10*deep_arg->theta2)+6.56250012
                 *(1+2*deep_arg->cosio-3*deep_arg->theta2));
	  f542 = 29.53125*deep_arg->sinio*(2-8*
                 deep_arg->cosio+deep_arg->theta2*
		 (-12+8*deep_arg->cosio+10*deep_arg->theta2));
	  f543 = 29.53125*deep_arg->sinio*(-2-8*deep_arg->cosio+
		 deep_arg->theta2*(12+8*deep_arg->cosio-10*
                 deep_arg->theta2));
	  xno2 = xnq*xnq;
	  ainv2 = aqnv*aqnv;
	  temp1 = 3*xno2*ainv2;
	  temp = temp1*root22;
	  d2201 = temp*f220*g201;
	  d2211 = temp*f221*g211;
	  temp1 = temp1*aqnv;
	  temp = temp1*root32;
	  d3210 = temp*f321*g310;
	  d3222 = temp*f322*g322;
	  temp1 = temp1*aqnv;
	  temp = 2*temp1*root44;
	  d4410 = temp*f441*g410;
	  d4422 = temp*f442*g422;
	  temp1 = temp1*aqnv;
	  temp = temp1*root52;
	  d5220 = temp*f522*g520;
	  d5232 = temp*f523*g532;
	  temp = 2*temp1*root54;
	  d5421 = temp*f542*g521;
	  d5433 = temp*f543*g533;
	  xlamo = xmao+tle->xnodeo+tle->xnodeo-thgr-thgr;
	  bfact = deep_arg->xmdot+deep_arg->xnodot+
                  deep_arg->xnodot-thdt-thdt;
	  bfact = bfact+ssl+ssh+ssh;
	} /* if( !(xnq < 0.0052359877) && (xnq > 0.0034906585) ) */
      else
	{
	  SetFlag(RESONANCE_FLAG);
	  SetFlag(SYNCHRONOUS_FLAG);
	  /* Synchronous resonance terms initialization */
	  g200 = 1+deep_arg->eosq*(-2.5+0.8125*deep_arg->eosq);
	  g310 = 1+2*deep_arg->eosq;
	  g300 = 1+deep_arg->eosq*(-6+6.60937*deep_arg->eosq);
	  f220 = 0.75*(1+deep_arg->cosio)*(1+deep_arg->cosio);
	  f311 = 0.9375*deep_arg->sinio*deep_arg->sinio*
	         (1+3*deep_arg->cosio)-0.75*(1+deep_arg->cosio);
	  f330 = 1+deep_arg->cosio;
	  f330 = 1.875*f330*f330*f330;
	  del1 = 3*xnq*xnq*aqnv*aqnv;
	  del2 = 2*del1*f220*g200*q22;
	  del3 = 3*del1*f330*g300*q33*aqnv;
	  del1 = del1*f311*g310*q31*aqnv;
	  fasx2 = 0.13130908;
	  fasx4 = 2.8843198;
	  fasx6 = 0.37448087;
	  xlamo = xmao+tle->xnodeo+tle->omegao-thgr;
	  bfact = deep_arg->xmdot+xpidot-thdt;
	  bfact = bfact+ssl+ssg+ssh;
	} /* End if( !(xnq < 0.0052359877) && (xnq > 0.0034906585) ) */

      xfact = bfact-xnq;

      /* Initialize integrator */
      xli = xlamo;
      xni = xnq;
      atime = 0;
      stepp = 720;
      stepn = -720;
      step2 = 259200;
      /* End case dpinit: */
      return;

    case dpsec: /* Entrance for deep space secular effects */
      deep_arg->xll = deep_arg->xll+ssl*deep_arg->t;
      deep_arg->omgadf = deep_arg->omgadf+ssg*deep_arg->t;
      deep_arg->xnode = deep_arg->xnode+ssh*deep_arg->t;
      deep_arg->em = tle->eo+sse*deep_arg->t;
      deep_arg->xinc = tle->xincl+ssi*deep_arg->t;
      if (deep_arg->xinc < 0)
	{
	  deep_arg->xinc = -deep_arg->xinc;
	  deep_arg->xnode = deep_arg->xnode + pi;
	  deep_arg->omgadf = deep_arg->omgadf-pi;
	}
      if( isFlagClear(RESONANCE_FLAG) ) return;

      do
	{
          if( (atime == 0) ||
	     ((deep_arg->t >= 0) && (atime < 0)) || 
	     ((deep_arg->t < 0) && (atime >= 0)) )
	    {
	      /* Epoch restart */
	      if( deep_arg->t >= 0 )
		delt = stepp;
	      else
		delt = stepn;

	      atime = 0;
	      xni = xnq;
	      xli = xlamo;
	    }
	  else
	    {	  
              if( fabs(deep_arg->t) >= fabs(atime) )
		{
		  if ( deep_arg->t > 0 )
		    delt = stepp;
		  else
		    delt = stepn;
		}
	    }

          do 
	    {
	      if ( fabs(deep_arg->t-atime) >= stepp )
		{
		  SetFlag(DO_LOOP_FLAG);
		  ClearFlag(EPOCH_RESTART_FLAG);
		}
	      else
		{
		  ft = deep_arg->t-atime;
		  ClearFlag(DO_LOOP_FLAG);
		}

	      if( fabs(deep_arg->t) < fabs(atime) )
		{
		  if (deep_arg->t >= 0)
		    delt = stepn;
		  else
		    delt = stepp;
		  SetFlag(DO_LOOP_FLAG | EPOCH_RESTART_FLAG);
		}

	      /* Dot terms calculated */
              if( isFlagSet(SYNCHRONOUS_FLAG) )
		{
		  xndot = del1*sin(xli-fasx2)+del2*sin(2*(xli-fasx4))
		          +del3*sin(3*(xli-fasx6));
		  xnddt = del1*cos(xli-fasx2)+2*del2*cos(2*(xli-fasx4))
		          +3*del3*cos(3*(xli-fasx6));
		}
	      else
		{
		  xomi = omegaq+deep_arg->omgdot*atime;
		  x2omi = xomi+xomi;
		  x2li = xli+xli;
		  xndot = d2201*sin(x2omi+xli-g22)
		          +d2211*sin(xli-g22)
		          +d3210*sin(xomi+xli-g32)
		          +d3222*sin(-xomi+xli-g32)
		          +d4410*sin(x2omi+x2li-g44)
		          +d4422*sin(x2li-g44)
		          +d5220*sin(xomi+xli-g52)
		          +d5232*sin(-xomi+xli-g52)
		          +d5421*sin(xomi+x2li-g54)
		          +d5433*sin(-xomi+x2li-g54);
		  xnddt = d2201*cos(x2omi+xli-g22)
		          +d2211*cos(xli-g22)
		          +d3210*cos(xomi+xli-g32)
		          +d3222*cos(-xomi+xli-g32)
		          +d5220*cos(xomi+xli-g52)
		          +d5232*cos(-xomi+xli-g52)
		          +2*(d4410*cos(x2omi+x2li-g44)
		          +d4422*cos(x2li-g44)
		          +d5421*cos(xomi+x2li-g54)
		          +d5433*cos(-xomi+x2li-g54));
		} /* End of if (isFlagSet(SYNCHRONOUS_FLAG)) */

	      xldot = xni+xfact;
	      xnddt = xnddt*xldot;

	      if(isFlagSet(DO_LOOP_FLAG))
		{
		  xli = xli+xldot*delt+xndot*step2;
		  xni = xni+xndot*delt+xnddt*step2;
		  atime = atime+delt;
		}
	    }
	  while(isFlagSet(DO_LOOP_FLAG) && isFlagClear(EPOCH_RESTART_FLAG));
	}
      while(isFlagSet(DO_LOOP_FLAG) && isFlagSet(EPOCH_RESTART_FLAG));

      deep_arg->xn = xni+xndot*ft+xnddt*ft*ft*0.5;
      xl = xli+xldot*ft+xndot*ft*ft*0.5;
      temp = -deep_arg->xnode+thgr+deep_arg->t*thdt;

      if (isFlagClear(SYNCHRONOUS_FLAG))
	deep_arg->xll = xl+temp+temp;
      else
	deep_arg->xll = xl-deep_arg->omgadf+temp;

      return;
      /*End case dpsec: */

    case dpper: /* Entrance for lunar-solar periodics */
      sinis = sin(deep_arg->xinc);
      cosis = cos(deep_arg->xinc);
      if (fabs(savtsn-deep_arg->t) >= 30)
	{
	  savtsn = deep_arg->t;
	  zm = zmos+zns*deep_arg->t;
	  zf = zm+2*zes*sin(zm);
	  sinzf = sin(zf);
	  f2 = 0.5*sinzf*sinzf-0.25;
	  f3 = -0.5*sinzf*cos(zf);
	  ses = se2*f2+se3*f3;
	  sis = si2*f2+si3*f3;
	  sls = sl2*f2+sl3*f3+sl4*sinzf;
	  sghs = sgh2*f2+sgh3*f3+sgh4*sinzf;
	  shs = sh2*f2+sh3*f3;
	  zm = zmol+znl*deep_arg->t;
	  zf = zm+2*zel*sin(zm);
	  sinzf = sin(zf);
	  f2 = 0.5*sinzf*sinzf-0.25;
	  f3 = -0.5*sinzf*cos(zf);
	  sel = ee2*f2+e3*f3;
	  sil = xi2*f2+xi3*f3;
	  sll = xl2*f2+xl3*f3+xl4*sinzf;
	  sghl = xgh2*f2+xgh3*f3+xgh4*sinzf;
	  sh1 = xh2*f2+xh3*f3;
	  pe = ses+sel;
	  pinc = sis+sil;
	  pl = sls+sll;
	}

      pgh = sghs+sghl;
      ph = shs+sh1;
      deep_arg->xinc = deep_arg->xinc+pinc;
      deep_arg->em = deep_arg->em+pe;

      if (xqncl >= 0.2)
	{
	  /* Apply periodics directly */
	  ph = ph/deep_arg->sinio;
	  pgh = pgh-deep_arg->cosio*ph;
	  deep_arg->omgadf = deep_arg->omgadf+pgh;
	  deep_arg->xnode = deep_arg->xnode+ph;
	  deep_arg->xll = deep_arg->xll+pl;
	}
      else
        {
	  /* Apply periodics with Lyddane modification */
	  sinok = sin(deep_arg->xnode);
	  cosok = cos(deep_arg->xnode);
	  alfdp = sinis*sinok;
	  betdp = sinis*cosok;
	  dalf = ph*cosok+pinc*cosis*sinok;
	  dbet = -ph*sinok+pinc*cosis*cosok;
	  alfdp = alfdp+dalf;
	  betdp = betdp+dbet;
	  deep_arg->xnode = FMod2p(deep_arg->xnode);
	  xls = deep_arg->xll+deep_arg->omgadf+cosis*deep_arg->xnode;
	  dls = pl+pgh-pinc*deep_arg->xnode*sinis;
	  xls = xls+dls;
	  xnoh = deep_arg->xnode;
	  deep_arg->xnode = AcTan(alfdp,betdp);

          /* This is a patch to Lyddane modification */
          /* suggested by Rob Matson. */
	  if(fabs(xnoh-deep_arg->xnode) > pi)
	    {
	      if(deep_arg->xnode < xnoh)
		deep_arg->xnode +=twopi;
	      else
		deep_arg->xnode -=twopi;
	    }

	  deep_arg->xll = deep_arg->xll+pl;
	  deep_arg->omgadf = xls-deep_arg->xll-cos(deep_arg->xinc)*
                             deep_arg->xnode;
	} /* End case dpper: */
      return;

    } /* End switch(ientry) */

} /* End of Deep() */

/*------------------------------------------------------------------*/

/* Functions for testing and setting/clearing flags */

/* An int variable holding the single-bit flags */
static int Flags = 0;

int
isFlagSet(int flag)
{
  return (Flags & flag);
}

int
isFlagClear(int flag)
{
  return (~Flags & flag);
}

void
SetFlag(int flag)
{
  Flags |= flag;
}

void
ClearFlag(int flag)
{
  Flags &= ~flag;
}

/*------------------------------------------------------------------*/
