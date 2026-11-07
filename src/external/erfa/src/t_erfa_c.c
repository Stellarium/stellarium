#include <erfa.h>
#include "erfam.h"
#include <stdio.h>

static int verbose = 0;

/*
**  - - - - - - - - -
**   t _ e r f a _ c
**  - - - - - - - - -
**
**  Validate the ERFA C functions.
**
**  Each ERFA function is at least called and a usually quite basic test
**  is performed.  Successful completion is signalled by a confirming
**  message.  Failure of a given function or group of functions results
**  in error messages.
**
**  All messages go to stdout.
**
**  This revision:  2021 July 29
**
*/

static void viv(int ival, int ivalok,
                const char *func, const char *test, int *status)
/*
**  - - - -
**   v i v
**  - - - -
**
**  Validate an integer result.
**
**  Internal function used by t_erfa_c program.
**
**  Given:
**     ival     int          value computed by function under test
**     ivalok   int          correct value
**     func     char[]       name of function under test
**     test     char[]       name of individual test
**
**  Given and returned:
**     status   int          set to TRUE if test fails
**
**  This revision:  2013 August 7
*/
{
   if (ival != ivalok) {
      *status = 1;
      printf("%s failed: %s want %d got %d\n",
             func, test, ivalok, ival);
   } else if (verbose) {
      printf("%s passed: %s want %d got %d\n",
                    func, test, ivalok, ival);
   }

}

static void vvd(double val, double valok, double dval,
                const char *func, const char *test, int *status)
/*
**  - - - -
**   v v d
**  - - - -
**
**  Validate a double result.
**
**  Internal function used by t_erfa_c program.
**
**  Given:
**     val      double       value computed by function under test
**     valok    double       expected value
**     dval     double       maximum allowable error
**     func     char[]       name of function under test
**     test     char[]       name of individual test
**
**  Given and returned:
**     status   int          set to TRUE if test fails
**
**  This revision:  2016 April 21
*/
{
   double a, f;   /* absolute and fractional error */


   a = val - valok;
   if (a != 0.0 && fabs(a) > fabs(dval)) {
      f = fabs(valok / a);
      *status = 1;
      printf("%s failed: %s want %.20g got %.20g (1/%.3g)\n",
             func, test, valok, val, f);
   } else if (verbose) {
      printf("%s passed: %s want %.20g got %.20g\n",
             func, test, valok, val);
   }

}

static void t_a2af(int *status)
/*
**  - - - - - - -
**   t _ a 2 a f
**  - - - - - - -
**
**  Test eraA2af function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraA2af, viv
**
**  This revision:  2013 August 7
*/
{
   int idmsf[4];
   char s;


   eraA2af(4, 2.345, &s, idmsf);

   viv(s, '+', "eraA2af", "s", status);

   viv(idmsf[0],  134, "eraA2af", "0", status);
   viv(idmsf[1],   21, "eraA2af", "1", status);
   viv(idmsf[2],   30, "eraA2af", "2", status);
   viv(idmsf[3], 9706, "eraA2af", "3", status);

}

static void t_a2tf(int *status)
/*
**  - - - - - - -
**   t _ a 2 t f
**  - - - - - - -
**
**  Test eraA2tf function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraA2tf, viv
**
**  This revision:  2013 August 7
*/
{
   int ihmsf[4];
   char s;


   eraA2tf(4, -3.01234, &s, ihmsf);

   viv((int)s, '-', "eraA2tf", "s", status);

   viv(ihmsf[0],   11, "eraA2tf", "0", status);
   viv(ihmsf[1],   30, "eraA2tf", "1", status);
   viv(ihmsf[2],   22, "eraA2tf", "2", status);
   viv(ihmsf[3], 6484, "eraA2tf", "3", status);

}

static void t_ab(int *status)
/*
**  - - - - -
**   t _ a b
**  - - - - -
**
**  Test eraAb function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraAb, vvd
**
**  This revision:  2013 October 1
*/
{
   double pnat[3], v[3], s, bm1, ppr[3];


   pnat[0] = -0.76321968546737951;
   pnat[1] = -0.60869453983060384;
   pnat[2] = -0.21676408580639883;
   v[0] =  2.1044018893653786e-5;
   v[1] = -8.9108923304429319e-5;
   v[2] = -3.8633714797716569e-5;
   s = 0.99980921395708788;
   bm1 = 0.99999999506209258;

   eraAb(pnat, v, s, bm1, ppr);

   vvd(ppr[0], -0.7631631094219556269, 1e-12, "eraAb", "1", status);
   vvd(ppr[1], -0.6087553082505590832, 1e-12, "eraAb", "2", status);
   vvd(ppr[2], -0.2167926269368471279, 1e-12, "eraAb", "3", status);

}

static void t_ae2hd(int *status)
/*
**  - - - - - - - -
**   t _ a e 2 h d
**  - - - - - - - -
**
**  Test eraAe2hd function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraAe2hd and vvd
**
**  This revision:  2017 October 21
*/
{
   double a, e, p, h, d;


   a = 5.5;
   e = 1.1;
   p = 0.7;

   eraAe2hd(a, e, p, &h, &d);

   vvd(h, 0.5933291115507309663, 1e-14, "eraAe2hd", "h", status);
   vvd(d, 0.9613934761647817620, 1e-14, "eraAe2hd", "d", status);

}

static void t_af2a(int *status)
/*
**  - - - - - - -
**   t _ a f 2 a
**  - - - - - - -
**
**  Test eraAf2a function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraAf2a, viv
**
**  This revision:  2013 August 7
*/
{
   double a;
   int j;


   j = eraAf2a('-', 45, 13, 27.2, &a);

   vvd(a, -0.7893115794313644842, 1e-12, "eraAf2a", "a", status);
   viv(j, 0, "eraAf2a", "j", status);

}

static void t_anp(int *status)
/*
**  - - - - - -
**   t _ a n p
**  - - - - - -
**
**  Test eraAnp function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraAnp, vvd
**
**  This revision:  2013 August 7
*/
{
   vvd(eraAnp(-0.1), 6.183185307179586477, 1e-12, "eraAnp", "", status);
}

static void t_anpm(int *status)
/*
**  - - - - - - -
**   t _ a n p m
**  - - - - - - -
**
**  Test eraAnpm function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraAnpm, vvd
**
**  This revision:  2013 August 7
*/
{
   vvd(eraAnpm(-4.0), 2.283185307179586477, 1e-12, "eraAnpm", "", status);
}

static void t_apcg(int *status)
/*
**  - - - - - - -
**   t _ a p c g
**  - - - - - - -
**
**  Test eraApcg function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraApcg, vvd
**
**  This revision:  2017 March 15
*/
{
   double date1, date2, ebpv[2][3], ehp[3];
   eraASTROM astrom;


   date1 = 2456165.5;
   date2 = 0.401182685;
   ebpv[0][0] =  0.901310875;
   ebpv[0][1] = -0.417402664;
   ebpv[0][2] = -0.180982288;
   ebpv[1][0] =  0.00742727954;
   ebpv[1][1] =  0.0140507459;
   ebpv[1][2] =  0.00609045792;
   ehp[0] =  0.903358544;
   ehp[1] = -0.415395237;
   ehp[2] = -0.180084014;

   eraApcg(date1, date2, ebpv, ehp, &astrom);

   vvd(astrom.pmt, 12.65133794027378508, 1e-11,
                   "eraApcg", "pmt", status);
   vvd(astrom.eb[0], 0.901310875, 1e-12,
                     "eraApcg", "eb(1)", status);
   vvd(astrom.eb[1], -0.417402664, 1e-12,
                     "eraApcg", "eb(2)", status);
   vvd(astrom.eb[2], -0.180982288, 1e-12,
                     "eraApcg", "eb(3)", status);
   vvd(astrom.eh[0], 0.8940025429324143045, 1e-12,
                     "eraApcg", "eh(1)", status);
   vvd(astrom.eh[1], -0.4110930268679817955, 1e-12,
                     "eraApcg", "eh(2)", status);
   vvd(astrom.eh[2], -0.1782189004872870264, 1e-12,
                     "eraApcg", "eh(3)", status);
   vvd(astrom.em, 1.010465295811013146, 1e-12,
                  "eraApcg", "em", status);
   vvd(astrom.v[0], 0.4289638913597693554e-4, 1e-16,
                    "eraApcg", "v(1)", status);
   vvd(astrom.v[1], 0.8115034051581320575e-4, 1e-16,
                    "eraApcg", "v(2)", status);
   vvd(astrom.v[2], 0.3517555136380563427e-4, 1e-16,
                    "eraApcg", "v(3)", status);
   vvd(astrom.bm1, 0.9999999951686012981, 1e-12,
                   "eraApcg", "bm1", status);
   vvd(astrom.bpn[0][0], 1.0, 0.0,
                         "eraApcg", "bpn(1,1)", status);
   vvd(astrom.bpn[1][0], 0.0, 0.0,
                         "eraApcg", "bpn(2,1)", status);
   vvd(astrom.bpn[2][0], 0.0, 0.0,
                         "eraApcg", "bpn(3,1)", status);
   vvd(astrom.bpn[0][1], 0.0, 0.0,
                         "eraApcg", "bpn(1,2)", status);
   vvd(astrom.bpn[1][1], 1.0, 0.0,
                         "eraApcg", "bpn(2,2)", status);
   vvd(astrom.bpn[2][1], 0.0, 0.0,
                         "eraApcg", "bpn(3,2)", status);
   vvd(astrom.bpn[0][2], 0.0, 0.0,
                         "eraApcg", "bpn(1,3)", status);
   vvd(astrom.bpn[1][2], 0.0, 0.0,
                         "eraApcg", "bpn(2,3)", status);
   vvd(astrom.bpn[2][2], 1.0, 0.0,
                         "eraApcg", "bpn(3,3)", status);

}

static void t_apcg13(int *status)
/*
**  - - - - - - - - -
**   t _ a p c g 1 3
**  - - - - - - - - -
**
**  Test eraApcg13 function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraApcg13, vvd
**
**  This revision:  2017 March 15
*/
{
   double date1, date2;
   eraASTROM astrom;


   date1 = 2456165.5;
   date2 = 0.401182685;

   eraApcg13(date1, date2, &astrom);

   vvd(astrom.pmt, 12.65133794027378508, 1e-11,
                   "eraApcg13", "pmt", status);
   vvd(astrom.eb[0], 0.9013108747340644755, 1e-12,
                   "eraApcg13", "eb(1)", status);
   vvd(astrom.eb[1], -0.4174026640406119957, 1e-12,
                   "eraApcg13", "eb(2)", status);
   vvd(astrom.eb[2], -0.1809822877867817771, 1e-12,
                   "eraApcg13", "eb(3)", status);
   vvd(astrom.eh[0], 0.8940025429255499549, 1e-12,
                   "eraApcg13", "eh(1)", status);
   vvd(astrom.eh[1], -0.4110930268331896318, 1e-12,
                   "eraApcg13", "eh(2)", status);
   vvd(astrom.eh[2], -0.1782189006019749850, 1e-12,
                   "eraApcg13", "eh(3)", status);
   vvd(astrom.em, 1.010465295964664178, 1e-12,
                   "eraApcg13", "em", status);
   vvd(astrom.v[0], 0.4289638912941341125e-4, 1e-16,
                   "eraApcg13", "v(1)", status);
   vvd(astrom.v[1], 0.8115034032405042132e-4, 1e-16,
                   "eraApcg13", "v(2)", status);
   vvd(astrom.v[2], 0.3517555135536470279e-4, 1e-16,
                   "eraApcg13", "v(3)", status);
   vvd(astrom.bm1, 0.9999999951686013142, 1e-12,
                   "eraApcg13", "bm1", status);
   vvd(astrom.bpn[0][0], 1.0, 0.0,
                         "eraApcg13", "bpn(1,1)", status);
   vvd(astrom.bpn[1][0], 0.0, 0.0,
                         "eraApcg13", "bpn(2,1)", status);
   vvd(astrom.bpn[2][0], 0.0, 0.0,
                         "eraApcg13", "bpn(3,1)", status);
   vvd(astrom.bpn[0][1], 0.0, 0.0,
                         "eraApcg13", "bpn(1,2)", status);
   vvd(astrom.bpn[1][1], 1.0, 0.0,
                         "eraApcg13", "bpn(2,2)", status);
   vvd(astrom.bpn[2][1], 0.0, 0.0,
                         "eraApcg13", "bpn(3,2)", status);
   vvd(astrom.bpn[0][2], 0.0, 0.0,
                         "eraApcg13", "bpn(1,3)", status);
   vvd(astrom.bpn[1][2], 0.0, 0.0,
                         "eraApcg13", "bpn(2,3)", status);
   vvd(astrom.bpn[2][2], 1.0, 0.0,
                         "eraApcg13", "bpn(3,3)", status);

}

static void t_apci(int *status)
/*
**  - - - - - - -
**   t _ a p c i
**  - - - - - - -
**
**  Test eraApci function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraApci, vvd
**
**  This revision:  2017 March 15
*/
{
   double date1, date2, ebpv[2][3], ehp[3], x, y, s;
   eraASTROM astrom;


   date1 = 2456165.5;
   date2 = 0.401182685;
   ebpv[0][0] =  0.901310875;
   ebpv[0][1] = -0.417402664;
   ebpv[0][2] = -0.180982288;
   ebpv[1][0] =  0.00742727954;
   ebpv[1][1] =  0.0140507459;
   ebpv[1][2] =  0.00609045792;
   ehp[0] =  0.903358544;
   ehp[1] = -0.415395237;
   ehp[2] = -0.180084014;
   x =  0.0013122272;
   y = -2.92808623e-5;
   s =  3.05749468e-8;

   eraApci(date1, date2, ebpv, ehp, x, y, s, &astrom);

   vvd(astrom.pmt, 12.65133794027378508, 1e-11,
                   "eraApci", "pmt", status);
   vvd(astrom.eb[0], 0.901310875, 1e-12,
                     "eraApci", "eb(1)", status);
   vvd(astrom.eb[1], -0.417402664, 1e-12,
                     "eraApci", "eb(2)", status);
   vvd(astrom.eb[2], -0.180982288, 1e-12,
                     "eraApci", "eb(3)", status);
   vvd(astrom.eh[0], 0.8940025429324143045, 1e-12,
                     "eraApci", "eh(1)", status);
   vvd(astrom.eh[1], -0.4110930268679817955, 1e-12,
                     "eraApci", "eh(2)", status);
   vvd(astrom.eh[2], -0.1782189004872870264, 1e-12,
                     "eraApci", "eh(3)", status);
   vvd(astrom.em, 1.010465295811013146, 1e-12,
                  "eraApci", "em", status);
   vvd(astrom.v[0], 0.4289638913597693554e-4, 1e-16,
                    "eraApci", "v(1)", status);
   vvd(astrom.v[1], 0.8115034051581320575e-4, 1e-16,
                    "eraApci", "v(2)", status);
   vvd(astrom.v[2], 0.3517555136380563427e-4, 1e-16,
                    "eraApci", "v(3)", status);
   vvd(astrom.bm1, 0.9999999951686012981, 1e-12,
                   "eraApci", "bm1", status);
   vvd(astrom.bpn[0][0], 0.9999991390295159156, 1e-12,
                         "eraApci", "bpn(1,1)", status);
   vvd(astrom.bpn[1][0], 0.4978650072505016932e-7, 1e-12,
                         "eraApci", "bpn(2,1)", status);
   vvd(astrom.bpn[2][0], 0.1312227200000000000e-2, 1e-12,
                         "eraApci", "bpn(3,1)", status);
   vvd(astrom.bpn[0][1], -0.1136336653771609630e-7, 1e-12,
                         "eraApci", "bpn(1,2)", status);
   vvd(astrom.bpn[1][1], 0.9999999995713154868, 1e-12,
                         "eraApci", "bpn(2,2)", status);
   vvd(astrom.bpn[2][1], -0.2928086230000000000e-4, 1e-12,
                         "eraApci", "bpn(3,2)", status);
   vvd(astrom.bpn[0][2], -0.1312227200895260194e-2, 1e-12,
                         "eraApci", "bpn(1,3)", status);
   vvd(astrom.bpn[1][2], 0.2928082217872315680e-4, 1e-12,
                         "eraApci", "bpn(2,3)", status);
   vvd(astrom.bpn[2][2], 0.9999991386008323373, 1e-12,
                         "eraApci", "bpn(3,3)", status);

}

static void t_apci13(int *status)
/*
**  - - - - - - - - -
**   t _ a p c i 1 3
**  - - - - - - - - -
**
**  Test eraApci13 function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraApci13, vvd
**
**  This revision:  2017 March 15
*/
{
   double date1, date2, eo;
   eraASTROM astrom;


   date1 = 2456165.5;
   date2 = 0.401182685;

   eraApci13(date1, date2, &astrom, &eo);

   vvd(astrom.pmt, 12.65133794027378508, 1e-11,
                   "eraApci13", "pmt", status);
   vvd(astrom.eb[0], 0.9013108747340644755, 1e-12,
                     "eraApci13", "eb(1)", status);
   vvd(astrom.eb[1], -0.4174026640406119957, 1e-12,
                     "eraApci13", "eb(2)", status);
   vvd(astrom.eb[2], -0.1809822877867817771, 1e-12,
                     "eraApci13", "eb(3)", status);
   vvd(astrom.eh[0], 0.8940025429255499549, 1e-12,
                     "eraApci13", "eh(1)", status);
   vvd(astrom.eh[1], -0.4110930268331896318, 1e-12,
                     "eraApci13", "eh(2)", status);
   vvd(astrom.eh[2], -0.1782189006019749850, 1e-12,
                     "eraApci13", "eh(3)", status);
   vvd(astrom.em, 1.010465295964664178, 1e-12,
                  "eraApci13", "em", status);
   vvd(astrom.v[0], 0.4289638912941341125e-4, 1e-16,
                    "eraApci13", "v(1)", status);
   vvd(astrom.v[1], 0.8115034032405042132e-4, 1e-16,
                    "eraApci13", "v(2)", status);
   vvd(astrom.v[2], 0.3517555135536470279e-4, 1e-16,
                    "eraApci13", "v(3)", status);
   vvd(astrom.bm1, 0.9999999951686013142, 1e-12,
                   "eraApci13", "bm1", status);
   vvd(astrom.bpn[0][0], 0.9999992060376761710, 1e-12,
                         "eraApci13", "bpn(1,1)", status);
   vvd(astrom.bpn[1][0], 0.4124244860106037157e-7, 1e-12,
                         "eraApci13", "bpn(2,1)", status);
   vvd(astrom.bpn[2][0], 0.1260128571051709670e-2, 1e-12,
                         "eraApci13", "bpn(3,1)", status);
   vvd(astrom.bpn[0][1], -0.1282291987222130690e-7, 1e-12,
                         "eraApci13", "bpn(1,2)", status);
   vvd(astrom.bpn[1][1], 0.9999999997456835325, 1e-12,
                         "eraApci13", "bpn(2,2)", status);
   vvd(astrom.bpn[2][1], -0.2255288829420524935e-4, 1e-12,
                         "eraApci13", "bpn(3,2)", status);
   vvd(astrom.bpn[0][2], -0.1260128571661374559e-2, 1e-12,
                         "eraApci13", "bpn(1,3)", status);
   vvd(astrom.bpn[1][2], 0.2255285422953395494e-4, 1e-12,
                         "eraApci13", "bpn(2,3)", status);
   vvd(astrom.bpn[2][2], 0.9999992057833604343, 1e-12,
                         "eraApci13", "bpn(3,3)", status);
   vvd(eo, -0.2900618712657375647e-2, 1e-12,
           "eraApci13", "eo", status);

}

static void t_apco(int *status)
/*
**  - - - - - - -
**   t _ a p c o
**  - - - - - - -
**
**  Test eraApco function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraApco, vvd
**
**  This revision:  2021 January 5
*/
{
   double date1, date2, ebpv[2][3], ehp[3], x, y, s,
          theta, elong, phi, hm, xp, yp, sp, refa, refb;
   eraASTROM astrom;


   date1 = 2456384.5;
   date2 = 0.970031644;
   ebpv[0][0] = -0.974170438;
   ebpv[0][1] = -0.211520082;
   ebpv[0][2] = -0.0917583024;
   ebpv[1][0] = 0.00364365824;
   ebpv[1][1] = -0.0154287319;
   ebpv[1][2] = -0.00668922024;
   ehp[0] = -0.973458265;
   ehp[1] = -0.209215307;
   ehp[2] = -0.0906996477;
   x = 0.0013122272;
   y = -2.92808623e-5;
   s = 3.05749468e-8;
   theta = 3.14540971;
   elong = -0.527800806;
   phi = -1.2345856;
   hm = 2738.0;
   xp = 2.47230737e-7;
   yp = 1.82640464e-6;
   sp = -3.01974337e-11;
   refa = 0.000201418779;
   refb = -2.36140831e-7;

   eraApco(date1, date2, ebpv, ehp, x, y, s,
           theta, elong, phi, hm, xp, yp, sp,
           refa, refb, &astrom);

   vvd(astrom.pmt, 13.25248468622587269, 1e-11,
                   "eraApco", "pmt", status);
   vvd(astrom.eb[0], -0.9741827110630322720, 1e-12,
                     "eraApco", "eb(1)", status);
   vvd(astrom.eb[1], -0.2115130190135344832, 1e-12,
                     "eraApco", "eb(2)", status);
   vvd(astrom.eb[2], -0.09179840186949532298, 1e-12,
                     "eraApco", "eb(3)", status);
   vvd(astrom.eh[0], -0.9736425571689739035, 1e-12,
                     "eraApco", "eh(1)", status);
   vvd(astrom.eh[1], -0.2092452125849330936, 1e-12,
                     "eraApco", "eh(2)", status);
   vvd(astrom.eh[2], -0.09075578152243272599, 1e-12,
                     "eraApco", "eh(3)", status);
   vvd(astrom.em, 0.9998233241709957653, 1e-12,
                  "eraApco", "em", status);
   vvd(astrom.v[0], 0.2078704992916728762e-4, 1e-16,
                    "eraApco", "v(1)", status);
   vvd(astrom.v[1], -0.8955360107151952319e-4, 1e-16,
                    "eraApco", "v(2)", status);
   vvd(astrom.v[2], -0.3863338994288951082e-4, 1e-16,
                    "eraApco", "v(3)", status);
   vvd(astrom.bm1, 0.9999999950277561236, 1e-12,
                   "eraApco", "bm1", status);
   vvd(astrom.bpn[0][0], 0.9999991390295159156, 1e-12,
                         "eraApco", "bpn(1,1)", status);
   vvd(astrom.bpn[1][0], 0.4978650072505016932e-7, 1e-12,
                         "eraApco", "bpn(2,1)", status);
   vvd(astrom.bpn[2][0], 0.1312227200000000000e-2, 1e-12,
                         "eraApco", "bpn(3,1)", status);
   vvd(astrom.bpn[0][1], -0.1136336653771609630e-7, 1e-12,
                         "eraApco", "bpn(1,2)", status);
   vvd(astrom.bpn[1][1], 0.9999999995713154868, 1e-12,
                         "eraApco", "bpn(2,2)", status);
   vvd(astrom.bpn[2][1], -0.2928086230000000000e-4, 1e-12,
                         "eraApco", "bpn(3,2)", status);
   vvd(astrom.bpn[0][2], -0.1312227200895260194e-2, 1e-12,
                         "eraApco", "bpn(1,3)", status);
   vvd(astrom.bpn[1][2], 0.2928082217872315680e-4, 1e-12,
                         "eraApco", "bpn(2,3)", status);
   vvd(astrom.bpn[2][2], 0.9999991386008323373, 1e-12,
                         "eraApco", "bpn(3,3)", status);
   vvd(astrom.along, -0.5278008060295995734, 1e-12,
                     "eraApco", "along", status);
   vvd(astrom.xpl, 0.1133427418130752958e-5, 1e-17,
                   "eraApco", "xpl", status);
   vvd(astrom.ypl, 0.1453347595780646207e-5, 1e-17,
                   "eraApco", "ypl", status);
   vvd(astrom.sphi, -0.9440115679003211329, 1e-12,
                    "eraApco", "sphi", status);
   vvd(astrom.cphi, 0.3299123514971474711, 1e-12,
                    "eraApco", "cphi", status);
   vvd(astrom.diurab, 0, 0,
                      "eraApco", "diurab", status);
   vvd(astrom.eral, 2.617608903970400427, 1e-12,
                    "eraApco", "eral", status);
   vvd(astrom.refa, 0.2014187790000000000e-3, 1e-15,
                    "eraApco", "refa", status);
   vvd(astrom.refb, -0.2361408310000000000e-6, 1e-18,
                    "eraApco", "refb", status);

}

static void t_apco13(int *status)
/*
**  - - - - - - - - -
**   t _ a p c o 1 3
**  - - - - - - - - -
**
**  Test eraApco13 function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraApco13, vvd, viv
**
**  This revision:  2021 January 5
*/
{
   double utc1, utc2, dut1, elong, phi, hm, xp, yp,
          phpa, tc, rh, wl, eo;
   eraASTROM astrom;
   int j;


   utc1 = 2456384.5;
   utc2 = 0.969254051;
   dut1 = 0.1550675;
   elong = -0.527800806;
   phi = -1.2345856;
   hm = 2738.0;
   xp = 2.47230737e-7;
   yp = 1.82640464e-6;
   phpa = 731.0;
   tc = 12.8;
   rh = 0.59;
   wl = 0.55;

   j = eraApco13(utc1, utc2, dut1, elong, phi, hm, xp, yp,
                 phpa, tc, rh, wl, &astrom, &eo);

   vvd(astrom.pmt, 13.25248468622475727, 1e-11,
                   "eraApco13", "pmt", status);
   vvd(astrom.eb[0], -0.9741827107320875162, 1e-12,
                   "eraApco13", "eb(1)", status);
   vvd(astrom.eb[1], -0.2115130190489716682, 1e-12,
                     "eraApco13", "eb(2)", status);
   vvd(astrom.eb[2], -0.09179840189496755339, 1e-12,
                     "eraApco13", "eb(3)", status);
   vvd(astrom.eh[0], -0.9736425572586935247, 1e-12,
                     "eraApco13", "eh(1)", status);
   vvd(astrom.eh[1], -0.2092452121603336166, 1e-12,
                     "eraApco13", "eh(2)", status);
   vvd(astrom.eh[2], -0.09075578153885665295, 1e-12,
                     "eraApco13", "eh(3)", status);
   vvd(astrom.em, 0.9998233240913898141, 1e-12,
                  "eraApco13", "em", status);
   vvd(astrom.v[0], 0.2078704994520489246e-4, 1e-16,
                    "eraApco13", "v(1)", status);
   vvd(astrom.v[1], -0.8955360133238868938e-4, 1e-16,
                    "eraApco13", "v(2)", status);
   vvd(astrom.v[2], -0.3863338993055887398e-4, 1e-16,
                    "eraApco13", "v(3)", status);
   vvd(astrom.bm1, 0.9999999950277561004, 1e-12,
                   "eraApco13", "bm1", status);
   vvd(astrom.bpn[0][0], 0.9999991390295147999, 1e-12,
                         "eraApco13", "bpn(1,1)", status);
   vvd(astrom.bpn[1][0], 0.4978650075315529277e-7, 1e-12,
                         "eraApco13", "bpn(2,1)", status);
   vvd(astrom.bpn[2][0], 0.001312227200850293372, 1e-12,
                         "eraApco13", "bpn(3,1)", status);
   vvd(astrom.bpn[0][1], -0.1136336652812486604e-7, 1e-12,
                         "eraApco13", "bpn(1,2)", status);
   vvd(astrom.bpn[1][1], 0.9999999995713154865, 1e-12,
                         "eraApco13", "bpn(2,2)", status);
   vvd(astrom.bpn[2][1], -0.2928086230975367296e-4, 1e-12,
                         "eraApco13", "bpn(3,2)", status);
   vvd(astrom.bpn[0][2], -0.001312227201745553566, 1e-12,
                         "eraApco13", "bpn(1,3)", status);
   vvd(astrom.bpn[1][2], 0.2928082218847679162e-4, 1e-12,
                         "eraApco13", "bpn(2,3)", status);
   vvd(astrom.bpn[2][2], 0.9999991386008312212, 1e-12,
                         "eraApco13", "bpn(3,3)", status);
   vvd(astrom.along, -0.5278008060295995733, 1e-12,
                     "eraApco13", "along", status);
   vvd(astrom.xpl, 0.1133427418130752958e-5, 1e-17,
                   "eraApco13", "xpl", status);
   vvd(astrom.ypl, 0.1453347595780646207e-5, 1e-17,
                   "eraApco13", "ypl", status);
   vvd(astrom.sphi, -0.9440115679003211329, 1e-12,
                    "eraApco13", "sphi", status);
   vvd(astrom.cphi, 0.3299123514971474711, 1e-12,
                    "eraApco13", "cphi", status);
   vvd(astrom.diurab, 0, 0,
                      "eraApco13", "diurab", status);
   vvd(astrom.eral, 2.617608909189664000, 1e-12,
                    "eraApco13", "eral", status);
   vvd(astrom.refa, 0.2014187785940396921e-3, 1e-15,
                    "eraApco13", "refa", status);
   vvd(astrom.refb, -0.2361408314943696227e-6, 1e-18,
                    "eraApco13", "refb", status);
   vvd(eo, -0.003020548354802412839, 1e-14,
           "eraApco13", "eo", status);
   viv(j, 0, "eraApco13", "j", status);

}

static void t_apcs(int *status)
/*
**  - - - - - - -
**   t _ a p c s
**  - - - - - - -
**
**  Test eraApcs function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraApcs, vvd
**
**  This revision:  2017 March 15
*/
{
   double date1, date2, pv[2][3], ebpv[2][3], ehp[3];
   eraASTROM astrom;


   date1 = 2456384.5;
   date2 = 0.970031644;
   pv[0][0] = -1836024.09;
   pv[0][1] = 1056607.72;
   pv[0][2] = -5998795.26;
   pv[1][0] = -77.0361767;
   pv[1][1] = -133.310856;
   pv[1][2] = 0.0971855934;
   ebpv[0][0] = -0.974170438;
   ebpv[0][1] = -0.211520082;
   ebpv[0][2] = -0.0917583024;
   ebpv[1][0] = 0.00364365824;
   ebpv[1][1] = -0.0154287319;
   ebpv[1][2] = -0.00668922024;
   ehp[0] = -0.973458265;
   ehp[1] = -0.209215307;
   ehp[2] = -0.0906996477;

   eraApcs(date1, date2, pv, ebpv, ehp, &astrom);

   vvd(astrom.pmt, 13.25248468622587269, 1e-11,
                   "eraApcs", "pmt", status);
   vvd(astrom.eb[0], -0.9741827110629881886, 1e-12,
                     "eraApcs", "eb(1)", status);
   vvd(astrom.eb[1], -0.2115130190136415986, 1e-12,
                     "eraApcs", "eb(2)", status);
   vvd(astrom.eb[2], -0.09179840186954412099, 1e-12,
                     "eraApcs", "eb(3)", status);
   vvd(astrom.eh[0], -0.9736425571689454706, 1e-12,
                     "eraApcs", "eh(1)", status);
   vvd(astrom.eh[1], -0.2092452125850435930, 1e-12,
                     "eraApcs", "eh(2)", status);
   vvd(astrom.eh[2], -0.09075578152248299218, 1e-12,
                     "eraApcs", "eh(3)", status);
   vvd(astrom.em, 0.9998233241709796859, 1e-12,
                  "eraApcs", "em", status);
   vvd(astrom.v[0], 0.2078704993282685510e-4, 1e-16,
                    "eraApcs", "v(1)", status);
   vvd(astrom.v[1], -0.8955360106989405683e-4, 1e-16,
                    "eraApcs", "v(2)", status);
   vvd(astrom.v[2], -0.3863338994289409097e-4, 1e-16,
                    "eraApcs", "v(3)", status);
   vvd(astrom.bm1, 0.9999999950277561237, 1e-12,
                   "eraApcs", "bm1", status);
   vvd(astrom.bpn[0][0], 1, 0,
                         "eraApcs", "bpn(1,1)", status);
   vvd(astrom.bpn[1][0], 0, 0,
                         "eraApcs", "bpn(2,1)", status);
   vvd(astrom.bpn[2][0], 0, 0,
                         "eraApcs", "bpn(3,1)", status);
   vvd(astrom.bpn[0][1], 0, 0,
                         "eraApcs", "bpn(1,2)", status);
   vvd(astrom.bpn[1][1], 1, 0,
                         "eraApcs", "bpn(2,2)", status);
   vvd(astrom.bpn[2][1], 0, 0,
                         "eraApcs", "bpn(3,2)", status);
   vvd(astrom.bpn[0][2], 0, 0,
                         "eraApcs", "bpn(1,3)", status);
   vvd(astrom.bpn[1][2], 0, 0,
                         "eraApcs", "bpn(2,3)", status);
   vvd(astrom.bpn[2][2], 1, 0,
                         "eraApcs", "bpn(3,3)", status);

}

static void t_apcs13(int *status)
/*
**  - - - - - - - - -
**   t _ a p c s 1 3
**  - - - - - - - - -
**
**  Test eraApcs13 function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraApcs13, vvd
**
**  This revision:  2017 March 15
*/
{
   double date1, date2, pv[2][3];
   eraASTROM astrom;


   date1 = 2456165.5;
   date2 = 0.401182685;
   pv[0][0] = -6241497.16;
   pv[0][1] = 401346.896;
   pv[0][2] = -1251136.04;
   pv[1][0] = -29.264597;
   pv[1][1] = -455.021831;
   pv[1][2] = 0.0266151194;

   eraApcs13(date1, date2, pv, &astrom);

   vvd(astrom.pmt, 12.65133794027378508, 1e-11,
                   "eraApcs13", "pmt", status);
   vvd(astrom.eb[0], 0.9012691529025250644, 1e-12,
                     "eraApcs13", "eb(1)", status);
   vvd(astrom.eb[1], -0.4173999812023194317, 1e-12,
                     "eraApcs13", "eb(2)", status);
   vvd(astrom.eb[2], -0.1809906511146429670, 1e-12,
                     "eraApcs13", "eb(3)", status);
   vvd(astrom.eh[0], 0.8939939101760130792, 1e-12,
                     "eraApcs13", "eh(1)", status);
   vvd(astrom.eh[1], -0.4111053891734021478, 1e-12,
                     "eraApcs13", "eh(2)", status);
   vvd(astrom.eh[2], -0.1782336880636997374, 1e-12,
                     "eraApcs13", "eh(3)", status);
   vvd(astrom.em, 1.010428384373491095, 1e-12,
                  "eraApcs13", "em", status);
   vvd(astrom.v[0], 0.4279877294121697570e-4, 1e-16,
                    "eraApcs13", "v(1)", status);
   vvd(astrom.v[1], 0.7963255087052120678e-4, 1e-16,
                    "eraApcs13", "v(2)", status);
   vvd(astrom.v[2], 0.3517564013384691531e-4, 1e-16,
                    "eraApcs13", "v(3)", status);
   vvd(astrom.bm1, 0.9999999952947980978, 1e-12,
                   "eraApcs13", "bm1", status);
   vvd(astrom.bpn[0][0], 1, 0,
                         "eraApcs13", "bpn(1,1)", status);
   vvd(astrom.bpn[1][0], 0, 0,
                         "eraApcs13", "bpn(2,1)", status);
   vvd(astrom.bpn[2][0], 0, 0,
                         "eraApcs13", "bpn(3,1)", status);
   vvd(astrom.bpn[0][1], 0, 0,
                         "eraApcs13", "bpn(1,2)", status);
   vvd(astrom.bpn[1][1], 1, 0,
                         "eraApcs13", "bpn(2,2)", status);
   vvd(astrom.bpn[2][1], 0, 0,
                         "eraApcs13", "bpn(3,2)", status);
   vvd(astrom.bpn[0][2], 0, 0,
                         "eraApcs13", "bpn(1,3)", status);
   vvd(astrom.bpn[1][2], 0, 0,
                         "eraApcs13", "bpn(2,3)", status);
   vvd(astrom.bpn[2][2], 1, 0,
                         "eraApcs13", "bpn(3,3)", status);

}

static void t_aper(int *status)
/*
**  - - - - - - -
**   t _ a p e r
**  - - - - - - -
*
**  Test eraAper function.
*
**  Returned:
**     status    int         FALSE = success, TRUE = fail
*
**  Called:  eraAper, vvd
*
**  This revision:  2013 October 3
*/
{
   double theta;
   eraASTROM astrom;


   astrom.along = 1.234;
   theta = 5.678;

   eraAper(theta, &astrom);

   vvd(astrom.eral, 6.912000000000000000, 1e-12,
                    "eraAper", "pmt", status);

}

static void t_aper13(int *status)
/*
**  - - - - - - - - -
**   t _ a p e r 1 3
**  - - - - - - - - -
**
**  Test eraAper13 function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraAper13, vvd
**
**  This revision:  2013 October 3
*/
{
   double ut11, ut12;
   eraASTROM astrom;


   astrom.along = 1.234;
   ut11 = 2456165.5;
   ut12 = 0.401182685;

   eraAper13(ut11, ut12, &astrom);

   vvd(astrom.eral, 3.316236661789694933, 1e-12,
                    "eraAper13", "pmt", status);

}

static void t_apio(int *status)
/*
**  - - - - - - -
**   t _ a p i o
**  - - - - - - -
**
**  Test eraApio function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraApio, vvd
**
**  This revision:  2021 January 5
*/
{
   double sp, theta, elong, phi, hm, xp, yp, refa, refb;
   eraASTROM astrom;


   sp = -3.01974337e-11;
   theta = 3.14540971;
   elong = -0.527800806;
   phi = -1.2345856;
   hm = 2738.0;
   xp = 2.47230737e-7;
   yp = 1.82640464e-6;
   refa = 0.000201418779;
   refb = -2.36140831e-7;

   eraApio(sp, theta, elong, phi, hm, xp, yp, refa, refb, &astrom);

   vvd(astrom.along, -0.5278008060295995734, 1e-12,
                     "eraApio", "along", status);
   vvd(astrom.xpl, 0.1133427418130752958e-5, 1e-17,
                   "eraApio", "xpl", status);
   vvd(astrom.ypl, 0.1453347595780646207e-5, 1e-17,
                   "eraApio", "ypl", status);
   vvd(astrom.sphi, -0.9440115679003211329, 1e-12,
                    "eraApio", "sphi", status);
   vvd(astrom.cphi, 0.3299123514971474711, 1e-12,
                    "eraApio", "cphi", status);
   vvd(astrom.diurab, 0.5135843661699913529e-6, 1e-12,
                      "eraApio", "diurab", status);
   vvd(astrom.eral, 2.617608903970400427, 1e-12,
                    "eraApio", "eral", status);
   vvd(astrom.refa, 0.2014187790000000000e-3, 1e-15,
                    "eraApio", "refa", status);
   vvd(astrom.refb, -0.2361408310000000000e-6, 1e-18,
                    "eraApio", "refb", status);

}

static void t_apio13(int *status)
/*
**  - - - - - - - - -
**   t _ a p i o 1 3
**  - - - - - - - - -
**
**  Test eraApio13 function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraApio13, vvd, viv
**
**  This revision:  2021 January 5
*/
{
   double utc1, utc2, dut1, elong, phi, hm, xp, yp, phpa, tc, rh, wl;
   int j;
   eraASTROM astrom;


   utc1 = 2456384.5;
   utc2 = 0.969254051;
   dut1 = 0.1550675;
   elong = -0.527800806;
   phi = -1.2345856;
   hm = 2738.0;
   xp = 2.47230737e-7;
   yp = 1.82640464e-6;
   phpa = 731.0;
   tc = 12.8;
   rh = 0.59;
   wl = 0.55;

   j = eraApio13(utc1, utc2, dut1, elong, phi, hm, xp, yp,
                 phpa, tc, rh, wl, &astrom);

   vvd(astrom.along, -0.5278008060295995733, 1e-12,
                     "eraApio13", "along", status);
   vvd(astrom.xpl, 0.1133427418130752958e-5, 1e-17,
                   "eraApio13", "xpl", status);
   vvd(astrom.ypl, 0.1453347595780646207e-5, 1e-17,
                   "eraApio13", "ypl", status);
   vvd(astrom.sphi, -0.9440115679003211329, 1e-12,
                    "eraApio13", "sphi", status);
   vvd(astrom.cphi, 0.3299123514971474711, 1e-12,
                    "eraApio13", "cphi", status);
   vvd(astrom.diurab, 0.5135843661699913529e-6, 1e-12,
                      "eraApio13", "diurab", status);
   vvd(astrom.eral, 2.617608909189664000, 1e-12,
                    "eraApio13", "eral", status);
   vvd(astrom.refa, 0.2014187785940396921e-3, 1e-15,
                    "eraApio13", "refa", status);
   vvd(astrom.refb, -0.2361408314943696227e-6, 1e-18,
                    "eraApio13", "refb", status);
   viv(j, 0, "eraApio13", "j", status);

}

static void t_atcc13(int *status)
/*
**  - - - - - - - - -
**   t _ a t c c 1 3
**  - - - - - - - - -
**
**  Test eraAtcc13 function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraAtcc13, vvd
**
**  This revision:  2021 April 18
*/
{
   double rc, dc, pr, pd, px, rv, date1, date2, ra, da;


   rc = 2.71;
   dc = 0.174;
   pr = 1e-5;
   pd = 5e-6;
   px = 0.1;
   rv = 55.0;
   date1 = 2456165.5;
   date2 = 0.401182685;

   eraAtcc13(rc, dc, pr, pd, px, rv, date1, date2, &ra, &da);

   vvd(ra,  2.710126504531372384, 1e-12,
           "eraAtcc13", "ra", status);
   vvd(da, 0.1740632537628350152, 1e-12,
           "eraAtcc13", "da", status);

}

static void t_atccq(int *status)
/*
**  - - - - - - - -
**   t _ a t c c q
**  - - - - - - - -
**
**  Test eraAtccq function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraApci13, eraAtccq, vvd
**
**  This revision:  2021 July 29
*/
{
   double date1, date2, eo, rc, dc, pr, pd, px, rv, ra, da;
   eraASTROM astrom;

   date1 = 2456165.5;
   date2 = 0.401182685;
   eraApci13(date1, date2, &astrom, &eo);
   rc = 2.71;
   dc = 0.174;
   pr = 1e-5;
   pd = 5e-6;
   px = 0.1;
   rv = 55.0;

   eraAtccq(rc, dc, pr, pd, px, rv, &astrom, &ra, &da);

   vvd(ra, 2.710126504531372384, 1e-12, "eraAtccq", "ra", status);
   vvd(da, 0.1740632537628350152, 1e-12, "eraAtccq", "da", status);

}

static void t_atci13(int *status)
/*
**  - - - - - - - - -
**   t _ a t c i 1 3
**  - - - - - - - - -
**
**  Test eraAtci13 function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraAtci13, vvd
**
**  This revision:  2017 March 15
*/
{
   double rc, dc, pr, pd, px, rv, date1, date2, ri, di, eo;


   rc = 2.71;
   dc = 0.174;
   pr = 1e-5;
   pd = 5e-6;
   px = 0.1;
   rv = 55.0;
   date1 = 2456165.5;
   date2 = 0.401182685;

   eraAtci13(rc, dc, pr, pd, px, rv, date1, date2, &ri, &di, &eo);

   vvd(ri, 2.710121572968696744, 1e-12,
           "eraAtci13", "ri", status);
   vvd(di, 0.1729371367219539137, 1e-12,
           "eraAtci13", "di", status);
   vvd(eo, -0.002900618712657375647, 1e-14,
           "eraAtci13", "eo", status);

}

static void t_atciq(int *status)
/*
**  - - - - - - - -
**   t _ a t c i q
**  - - - - - - - -
**
**  Test eraAtciq function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraApci13, eraAtciq, vvd
**
**  This revision:  2017 March 15
*/
{
   double date1, date2, eo, rc, dc, pr, pd, px, rv, ri, di;
   eraASTROM astrom;

   date1 = 2456165.5;
   date2 = 0.401182685;
   eraApci13(date1, date2, &astrom, &eo);
   rc = 2.71;
   dc = 0.174;
   pr = 1e-5;
   pd = 5e-6;
   px = 0.1;
   rv = 55.0;

   eraAtciq(rc, dc, pr, pd, px, rv, &astrom, &ri, &di);

   vvd(ri, 2.710121572968696744, 1e-12, "eraAtciq", "ri", status);
   vvd(di, 0.1729371367219539137, 1e-12, "eraAtciq", "di", status);

}

static void t_atciqn(int *status)
/*
**  - - - - - - - - -
**   t _ a t c i q n
**  - - - - - - - - -
**
**  Test eraAtciqn function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraApci13, eraAtciqn, vvd
**
**  This revision:  2017 March 15
*/
{
   eraLDBODY b[3];
   double date1, date2, eo, rc, dc, pr, pd, px, rv, ri, di;
   eraASTROM astrom;

   date1 = 2456165.5;
   date2 = 0.401182685;
   eraApci13(date1, date2, &astrom, &eo);
   rc = 2.71;
   dc = 0.174;
   pr = 1e-5;
   pd = 5e-6;
   px = 0.1;
   rv = 55.0;
   b[0].bm = 0.00028574;
   b[0].dl = 3e-10;
   b[0].pv[0][0] = -7.81014427;
   b[0].pv[0][1] = -5.60956681;
   b[0].pv[0][2] = -1.98079819;
   b[0].pv[1][0] =  0.0030723249;
   b[0].pv[1][1] = -0.00406995477;
   b[0].pv[1][2] = -0.00181335842;
   b[1].bm = 0.00095435;
   b[1].dl = 3e-9;
   b[1].pv[0][0] =  0.738098796;
   b[1].pv[0][1] =  4.63658692;
   b[1].pv[0][2] =  1.9693136;
   b[1].pv[1][0] = -0.00755816922;
   b[1].pv[1][1] =  0.00126913722;
   b[1].pv[1][2] =  0.000727999001;
   b[2].bm = 1.0;
   b[2].dl = 6e-6;
   b[2].pv[0][0] = -0.000712174377;
   b[2].pv[0][1] = -0.00230478303;
   b[2].pv[0][2] = -0.00105865966;
   b[2].pv[1][0] =  6.29235213e-6;
   b[2].pv[1][1] = -3.30888387e-7;
   b[2].pv[1][2] = -2.96486623e-7;

   eraAtciqn ( rc, dc, pr, pd, px, rv, &astrom, 3, b, &ri, &di);

   vvd(ri, 2.710122008104983335, 1e-12, "eraAtciqn", "ri", status);
   vvd(di, 0.1729371916492767821, 1e-12, "eraAtciqn", "di", status);

}

static void t_atciqz(int *status)
/*
**  - - - - - - - - -
**   t _ a t c i q z
**  - - - - - - - - -
**
**  Test eraAtciqz function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraApci13, eraAtciqz, vvd
**
**  This revision:  2017 March 15
*/
{
   double date1, date2, eo, rc, dc, ri, di;
   eraASTROM astrom;


   date1 = 2456165.5;
   date2 = 0.401182685;
   eraApci13(date1, date2, &astrom, &eo);
   rc = 2.71;
   dc = 0.174;

   eraAtciqz(rc, dc, &astrom, &ri, &di);

   vvd(ri, 2.709994899247256984, 1e-12, "eraAtciqz", "ri", status);
   vvd(di, 0.1728740720984931891, 1e-12, "eraAtciqz", "di", status);

}

static void t_atco13(int *status)
/*
**  - - - - - - - - -
**   t _ a t c o 1 3
**  - - - - - - - - -
**
**  Test eraAtco13 function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraAtco13, vvd, viv
**
**  This revision:  2021 January 5
*/
{
   double rc, dc, pr, pd, px, rv, utc1, utc2, dut1,
          elong, phi, hm, xp, yp, phpa, tc, rh, wl,
          aob, zob, hob, dob, rob, eo;
   int j;


   rc = 2.71;
   dc = 0.174;
   pr = 1e-5;
   pd = 5e-6;
   px = 0.1;
   rv = 55.0;
   utc1 = 2456384.5;
   utc2 = 0.969254051;
   dut1 = 0.1550675;
   elong = -0.527800806;
   phi = -1.2345856;
   hm = 2738.0;
   xp = 2.47230737e-7;
   yp = 1.82640464e-6;
   phpa = 731.0;
   tc = 12.8;
   rh = 0.59;
   wl = 0.55;

   j = eraAtco13(rc, dc, pr, pd, px, rv,
                 utc1, utc2, dut1, elong, phi, hm, xp, yp,
                 phpa, tc, rh, wl,
                 &aob, &zob, &hob, &dob, &rob, &eo);

   vvd(aob, 0.9251774485485515207e-1, 1e-12, "eraAtco13", "aob", status);
   vvd(zob, 1.407661405256499357, 1e-12, "eraAtco13", "zob", status);
   vvd(hob, -0.9265154431529724692e-1, 1e-12, "eraAtco13", "hob", status);
   vvd(dob, 0.1716626560072526200, 1e-12, "eraAtco13", "dob", status);
   vvd(rob, 2.710260453504961012, 1e-12, "eraAtco13", "rob", status);
   vvd(eo, -0.003020548354802412839, 1e-14, "eraAtco13", "eo", status);
   viv(j, 0, "eraAtco13", "j", status);

}

static void t_atic13(int *status)
/*
**  - - - - - - - - -
**   t _ a t i c 1 3
**  - - - - - - - - -
**
**  Test eraAtic13 function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraAtic13, vvd
**
**  This revision:  2017 March 15
*/
{
   double ri, di, date1, date2, rc, dc, eo;


   ri = 2.710121572969038991;
   di = 0.1729371367218230438;
   date1 = 2456165.5;
   date2 = 0.401182685;

   eraAtic13(ri, di, date1, date2, &rc, &dc, &eo);

   vvd(rc, 2.710126504531716819, 1e-12, "eraAtic13", "rc", status);
   vvd(dc, 0.1740632537627034482, 1e-12, "eraAtic13", "dc", status);
   vvd(eo, -0.002900618712657375647, 1e-14, "eraAtic13", "eo", status);

}

static void t_aticq(int *status)
/*
**  - - - - - - - -
**   t _ a t i c q
**  - - - - - - - -
**
**  Test eraAticq function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraApci13, eraAticq, vvd
**
**  This revision:  2017 March 15
*/
{
   double date1, date2, eo, ri, di, rc, dc;
   eraASTROM astrom;


   date1 = 2456165.5;
   date2 = 0.401182685;
   eraApci13(date1, date2, &astrom, &eo);
   ri = 2.710121572969038991;
   di = 0.1729371367218230438;

   eraAticq(ri, di, &astrom, &rc, &dc);

   vvd(rc, 2.710126504531716819, 1e-12, "eraAticq", "rc", status);
   vvd(dc, 0.1740632537627034482, 1e-12, "eraAticq", "dc", status);

}

static void t_aticqn(int *status)
/*
**  - - - - - - - - -
**   t _ a t i c q n
**  - - - - - - - - -
**
**  Test eraAticqn function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraApci13, eraAticqn, vvd
**
**  This revision:  2017 March 15
*/
{
   double date1, date2, eo, ri, di, rc, dc;
   eraLDBODY b[3];
   eraASTROM astrom;


   date1 = 2456165.5;
   date2 = 0.401182685;
   eraApci13(date1, date2, &astrom, &eo);
   ri = 2.709994899247599271;
   di = 0.1728740720983623469;
   b[0].bm = 0.00028574;
   b[0].dl = 3e-10;
   b[0].pv[0][0] = -7.81014427;
   b[0].pv[0][1] = -5.60956681;
   b[0].pv[0][2] = -1.98079819;
   b[0].pv[1][0] =  0.0030723249;
   b[0].pv[1][1] = -0.00406995477;
   b[0].pv[1][2] = -0.00181335842;
   b[1].bm = 0.00095435;
   b[1].dl = 3e-9;
   b[1].pv[0][0] =  0.738098796;
   b[1].pv[0][1] =  4.63658692;
   b[1].pv[0][2] =  1.9693136;
   b[1].pv[1][0] = -0.00755816922;
   b[1].pv[1][1] =  0.00126913722;
   b[1].pv[1][2] =  0.000727999001;
   b[2].bm = 1.0;
   b[2].dl = 6e-6;
   b[2].pv[0][0] = -0.000712174377;
   b[2].pv[0][1] = -0.00230478303;
   b[2].pv[0][2] = -0.00105865966;
   b[2].pv[1][0] =  6.29235213e-6;
   b[2].pv[1][1] = -3.30888387e-7;
   b[2].pv[1][2] = -2.96486623e-7;

   eraAticqn(ri, di, &astrom, 3, b, &rc, &dc);

   vvd(rc, 2.709999575033027333, 1e-12, "eraAtciqn", "rc", status);
   vvd(dc, 0.1739999656316469990, 1e-12, "eraAtciqn", "dc", status);

}

static void t_atio13(int *status)
/*
**  - - - - - - - - -
**   t _ a t i o 1 3
**  - - - - - - - - -
**
**  Test eraAtio13 function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraAtio13, vvd, viv
**
**  This revision:  2021 January 5
*/
{
   double ri, di, utc1, utc2, dut1, elong, phi, hm, xp, yp,
          phpa, tc, rh, wl, aob, zob, hob, dob, rob;
   int j;


   ri = 2.710121572969038991;
   di = 0.1729371367218230438;
   utc1 = 2456384.5;
   utc2 = 0.969254051;
   dut1 = 0.1550675;
   elong = -0.527800806;
   phi = -1.2345856;
   hm = 2738.0;
   xp = 2.47230737e-7;
   yp = 1.82640464e-6;
   phpa = 731.0;
   tc = 12.8;
   rh = 0.59;
   wl = 0.55;

   j = eraAtio13(ri, di, utc1, utc2, dut1, elong, phi, hm,
                 xp, yp, phpa, tc, rh, wl,
                 &aob, &zob, &hob, &dob, &rob);

   vvd(aob, 0.9233952224895122499e-1, 1e-12, "eraAtio13", "aob", status);
   vvd(zob, 1.407758704513549991, 1e-12, "eraAtio13", "zob", status);
   vvd(hob, -0.9247619879881698140e-1, 1e-12, "eraAtio13", "hob", status);
   vvd(dob, 0.1717653435756234676, 1e-12, "eraAtio13", "dob", status);
   vvd(rob, 2.710085107988480746, 1e-12, "eraAtio13", "rob", status);
   viv(j, 0, "eraAtio13", "j", status);

}

static void t_atioq(int *status)
/*
**  - - - - - - - -
**   t _ a t i o q
**  - - - - - - - -
**
**  Test eraAtioq function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraApio13, eraAtioq, vvd, viv
**
**  This revision:  2021 January 5
*/
{
   double utc1, utc2, dut1, elong, phi, hm, xp, yp,
          phpa, tc, rh, wl, ri, di, aob, zob, hob, dob, rob;
   eraASTROM astrom;


   utc1 = 2456384.5;
   utc2 = 0.969254051;
   dut1 = 0.1550675;
   elong = -0.527800806;
   phi = -1.2345856;
   hm = 2738.0;
   xp = 2.47230737e-7;
   yp = 1.82640464e-6;
   phpa = 731.0;
   tc = 12.8;
   rh = 0.59;
   wl = 0.55;
   (void) eraApio13(utc1, utc2, dut1, elong, phi, hm, xp, yp,
                    phpa, tc, rh, wl, &astrom);
   ri = 2.710121572969038991;
   di = 0.1729371367218230438;

   eraAtioq(ri, di, &astrom, &aob, &zob, &hob, &dob, &rob);

   vvd(aob, 0.9233952224895122499e-1, 1e-12, "eraAtioq", "aob", status);
   vvd(zob, 1.407758704513549991, 1e-12, "eraAtioq", "zob", status);
   vvd(hob, -0.9247619879881698140e-1, 1e-12, "eraAtioq", "hob", status);
   vvd(dob, 0.1717653435756234676, 1e-12, "eraAtioq", "dob", status);
   vvd(rob, 2.710085107988480746, 1e-12, "eraAtioq", "rob", status);

}

static void t_atoc13(int *status)
/*
**  - - - - - - - - -
**   t _ a t o c 1 3
**  - - - - - - - - -
**
**  Test eraAtoc13 function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraAtoc13, vvd, viv
**
**  This revision:  2021 January 5
*/
{
   double utc1, utc2, dut1,
          elong, phi, hm, xp, yp, phpa, tc, rh, wl,
          ob1, ob2, rc, dc;
   int j;


   utc1 = 2456384.5;
   utc2 = 0.969254051;
   dut1 = 0.1550675;
   elong = -0.527800806;
   phi = -1.2345856;
   hm = 2738.0;
   xp = 2.47230737e-7;
   yp = 1.82640464e-6;
   phpa = 731.0;
   tc = 12.8;
   rh = 0.59;
   wl = 0.55;

   ob1 = 2.710085107986886201;
   ob2 = 0.1717653435758265198;
   j = eraAtoc13 ( "R", ob1, ob2, utc1, utc2, dut1,
                   elong, phi, hm, xp, yp, phpa, tc, rh, wl,
                   &rc, &dc);
   vvd(rc, 2.709956744659136129, 1e-12, "eraAtoc13", "R/rc", status);
   vvd(dc, 0.1741696500898471362, 1e-12, "eraAtoc13", "R/dc", status);
   viv(j, 0, "eraAtoc13", "R/j", status);

   ob1 = -0.09247619879782006106;
   ob2 = 0.1717653435758265198;
   j = eraAtoc13 ( "H", ob1, ob2, utc1, utc2, dut1,
                   elong, phi, hm, xp, yp, phpa, tc, rh, wl,
                   &rc, &dc);
   vvd(rc, 2.709956744659734086, 1e-12, "eraAtoc13", "H/rc", status);
   vvd(dc, 0.1741696500898471362, 1e-12, "eraAtoc13", "H/dc", status);
   viv(j, 0, "eraAtoc13", "H/j", status);

   ob1 = 0.09233952224794989993;
   ob2 = 1.407758704513722461;
   j = eraAtoc13 ( "A", ob1, ob2, utc1, utc2, dut1,
                   elong, phi, hm, xp, yp, phpa, tc, rh, wl,
                   &rc, &dc);
   vvd(rc, 2.709956744659734086, 1e-12, "eraAtoc13", "A/rc", status);
   vvd(dc, 0.1741696500898471366, 1e-12, "eraAtoc13", "A/dc", status);
   viv(j, 0, "eraAtoc13", "A/j", status);

}

static void t_atoi13(int *status)
/*
**  - - - - - - - - -
**   t _ a t o i 1 3
**  - - - - - - - - -
**
**  Test eraAtoi13 function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraAtoi13, vvd, viv
**
**  This revision:  2021 January 5
*/
{
   double utc1, utc2, dut1, elong, phi, hm, xp, yp, phpa, tc, rh, wl,
          ob1, ob2, ri, di;
   int j;


   utc1 = 2456384.5;
   utc2 = 0.969254051;
   dut1 = 0.1550675;
   elong = -0.527800806;
   phi = -1.2345856;
   hm = 2738.0;
   xp = 2.47230737e-7;
   yp = 1.82640464e-6;
   phpa = 731.0;
   tc = 12.8;
   rh = 0.59;
   wl = 0.55;

   ob1 = 2.710085107986886201;
   ob2 = 0.1717653435758265198;
   j = eraAtoi13 ( "R", ob1, ob2, utc1, utc2, dut1,
                   elong, phi, hm, xp, yp, phpa, tc, rh, wl,
                   &ri, &di);
   vvd(ri, 2.710121574447540810, 1e-12, "eraAtoi13", "R/ri", status);
   vvd(di, 0.1729371839116608778, 1e-12, "eraAtoi13", "R/di", status);
   viv(j, 0, "eraAtoi13", "R/J", status);

   ob1 = -0.09247619879782006106;
   ob2 = 0.1717653435758265198;
   j = eraAtoi13 ( "H", ob1, ob2, utc1, utc2, dut1,
                   elong, phi, hm, xp, yp, phpa, tc, rh, wl,
                   &ri, &di);
   vvd(ri, 2.710121574448138676, 1e-12, "eraAtoi13", "H/ri", status);
   vvd(di, 0.1729371839116608778, 1e-12, "eraAtoi13", "H/di", status);
   viv(j, 0, "eraAtoi13", "H/J", status);

   ob1 = 0.09233952224794989993;
   ob2 = 1.407758704513722461;
   j = eraAtoi13 ( "A", ob1, ob2, utc1, utc2, dut1,
                   elong, phi, hm, xp, yp, phpa, tc, rh, wl,
                   &ri, &di);
   vvd(ri, 2.710121574448138676, 1e-12, "eraAtoi13", "A/ri", status);
   vvd(di, 0.1729371839116608781, 1e-12, "eraAtoi13", "A/di", status);
   viv(j, 0, "eraAtoi13", "A/J", status);

}

static void t_atoiq(int *status)
/*
**  - - - - - - - -
**   t _ a t o i q
**  - - - - - - - -
*
**  Test eraAtoiq function.
*
**  Returned:
**     status    int         FALSE = success, TRUE = fail
*
**  Called:  eraApio13, eraAtoiq, vvd
*
**  This revision:  2021 January 5
*/
{
   double utc1, utc2, dut1, elong, phi, hm, xp, yp, phpa, tc, rh, wl,
          ob1, ob2, ri, di;
   eraASTROM astrom;


   utc1 = 2456384.5;
   utc2 = 0.969254051;
   dut1 = 0.1550675;
   elong = -0.527800806;
   phi = -1.2345856;
   hm = 2738.0;
   xp = 2.47230737e-7;
   yp = 1.82640464e-6;
   phpa = 731.0;
   tc = 12.8;
   rh = 0.59;
   wl = 0.55;
   (void) eraApio13(utc1, utc2, dut1, elong, phi, hm, xp, yp,
                    phpa, tc, rh, wl, &astrom);

   ob1 = 2.710085107986886201;
   ob2 = 0.1717653435758265198;
   eraAtoiq("R", ob1, ob2, &astrom, &ri, &di);
   vvd(ri, 2.710121574447540810, 1e-12,
           "eraAtoiq", "R/ri", status);
   vvd(di, 0.17293718391166087785, 1e-12,
           "eraAtoiq", "R/di", status);

   ob1 = -0.09247619879782006106;
   ob2 = 0.1717653435758265198;
   eraAtoiq("H", ob1, ob2, &astrom, &ri, &di);
   vvd(ri, 2.710121574448138676, 1e-12,
           "eraAtoiq", "H/ri", status);
   vvd(di, 0.1729371839116608778, 1e-12,
           "eraAtoiq", "H/di", status);

   ob1 = 0.09233952224794989993;
   ob2 = 1.407758704513722461;
   eraAtoiq("A", ob1, ob2, &astrom, &ri, &di);
   vvd(ri, 2.710121574448138676, 1e-12,
           "eraAtoiq", "A/ri", status);
   vvd(di, 0.1729371839116608781, 1e-12,
           "eraAtoiq", "A/di", status);

}

static void t_bi00(int *status)
/*
**  - - - - - - -
**   t _ b i 0 0
**  - - - - - - -
**
**  Test eraBi00 function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraBi00, vvd
**
**  This revision:  2013 August 7
*/
{
   double dpsibi, depsbi, dra;

   eraBi00(&dpsibi, &depsbi, &dra);

   vvd(dpsibi, -0.2025309152835086613e-6, 1e-12,
      "eraBi00", "dpsibi", status);
   vvd(depsbi, -0.3306041454222147847e-7, 1e-12,
      "eraBi00", "depsbi", status);
   vvd(dra, -0.7078279744199225506e-7, 1e-12,
      "eraBi00", "dra", status);
}

static void t_bp00(int *status)
/*
**  - - - - - - -
**   t _ b p 0 0
**  - - - - - - -
**
**  Test eraBp00 function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraBp00, vvd
**
**  This revision:  2013 August 7
*/
{
   double rb[3][3], rp[3][3], rbp[3][3];


   eraBp00(2400000.5, 50123.9999, rb, rp, rbp);

   vvd(rb[0][0], 0.9999999999999942498, 1e-12,
       "eraBp00", "rb11", status);
   vvd(rb[0][1], -0.7078279744199196626e-7, 1e-16,
       "eraBp00", "rb12", status);
   vvd(rb[0][2], 0.8056217146976134152e-7, 1e-16,
       "eraBp00", "rb13", status);
   vvd(rb[1][0], 0.7078279477857337206e-7, 1e-16,
       "eraBp00", "rb21", status);
   vvd(rb[1][1], 0.9999999999999969484, 1e-12,
       "eraBp00", "rb22", status);
   vvd(rb[1][2], 0.3306041454222136517e-7, 1e-16,
       "eraBp00", "rb23", status);
   vvd(rb[2][0], -0.8056217380986972157e-7, 1e-16,
       "eraBp00", "rb31", status);
   vvd(rb[2][1], -0.3306040883980552500e-7, 1e-16,
       "eraBp00", "rb32", status);
   vvd(rb[2][2], 0.9999999999999962084, 1e-12,
       "eraBp00", "rb33", status);

   vvd(rp[0][0], 0.9999995504864048241, 1e-12,
       "eraBp00", "rp11", status);
   vvd(rp[0][1], 0.8696113836207084411e-3, 1e-14,
       "eraBp00", "rp12", status);
   vvd(rp[0][2], 0.3778928813389333402e-3, 1e-14,
       "eraBp00", "rp13", status);
   vvd(rp[1][0], -0.8696113818227265968e-3, 1e-14,
       "eraBp00", "rp21", status);
   vvd(rp[1][1], 0.9999996218879365258, 1e-12,
       "eraBp00", "rp22", status);
   vvd(rp[1][2], -0.1690679263009242066e-6, 1e-14,
       "eraBp00", "rp23", status);
   vvd(rp[2][0], -0.3778928854764695214e-3, 1e-14,
       "eraBp00", "rp31", status);
   vvd(rp[2][1], -0.1595521004195286491e-6, 1e-14,
       "eraBp00", "rp32", status);
   vvd(rp[2][2], 0.9999999285984682756, 1e-12,
       "eraBp00", "rp33", status);

   vvd(rbp[0][0], 0.9999995505175087260, 1e-12,
       "eraBp00", "rbp11", status);
   vvd(rbp[0][1], 0.8695405883617884705e-3, 1e-14,
       "eraBp00", "rbp12", status);
   vvd(rbp[0][2], 0.3779734722239007105e-3, 1e-14,
       "eraBp00", "rbp13", status);
   vvd(rbp[1][0], -0.8695405990410863719e-3, 1e-14,
       "eraBp00", "rbp21", status);
   vvd(rbp[1][1], 0.9999996219494925900, 1e-12,
       "eraBp00", "rbp22", status);
   vvd(rbp[1][2], -0.1360775820404982209e-6, 1e-14,
       "eraBp00", "rbp23", status);
   vvd(rbp[2][0], -0.3779734476558184991e-3, 1e-14,
       "eraBp00", "rbp31", status);
   vvd(rbp[2][1], -0.1925857585832024058e-6, 1e-14,
       "eraBp00", "rbp32", status);
   vvd(rbp[2][2], 0.9999999285680153377, 1e-12,
       "eraBp00", "rbp33", status);
}

static void t_bp06(int *status)
/*
**  - - - - - - -
**   t _ b p 0 6
**  - - - - - - -
**
**  Test eraBp06 function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraBp06, vvd
**
**  This revision:  2013 August 7
*/
{
   double rb[3][3], rp[3][3], rbp[3][3];


   eraBp06(2400000.5, 50123.9999, rb, rp, rbp);

   vvd(rb[0][0], 0.9999999999999942497, 1e-12,
       "eraBp06", "rb11", status);
   vvd(rb[0][1], -0.7078368960971557145e-7, 1e-14,
       "eraBp06", "rb12", status);
   vvd(rb[0][2], 0.8056213977613185606e-7, 1e-14,
       "eraBp06", "rb13", status);
   vvd(rb[1][0], 0.7078368694637674333e-7, 1e-14,
       "eraBp06", "rb21", status);
   vvd(rb[1][1], 0.9999999999999969484, 1e-12,
       "eraBp06", "rb22", status);
   vvd(rb[1][2], 0.3305943742989134124e-7, 1e-14,
       "eraBp06", "rb23", status);
   vvd(rb[2][0], -0.8056214211620056792e-7, 1e-14,
       "eraBp06", "rb31", status);
   vvd(rb[2][1], -0.3305943172740586950e-7, 1e-14,
       "eraBp06", "rb32", status);
   vvd(rb[2][2], 0.9999999999999962084, 1e-12,
       "eraBp06", "rb33", status);

   vvd(rp[0][0], 0.9999995504864960278, 1e-12,
       "eraBp06", "rp11", status);
   vvd(rp[0][1], 0.8696112578855404832e-3, 1e-14,
       "eraBp06", "rp12", status);
   vvd(rp[0][2], 0.3778929293341390127e-3, 1e-14,
       "eraBp06", "rp13", status);
   vvd(rp[1][0], -0.8696112560510186244e-3, 1e-14,
       "eraBp06", "rp21", status);
   vvd(rp[1][1], 0.9999996218880458820, 1e-12,
       "eraBp06", "rp22", status);
   vvd(rp[1][2], -0.1691646168941896285e-6, 1e-14,
       "eraBp06", "rp23", status);
   vvd(rp[2][0], -0.3778929335557603418e-3, 1e-14,
       "eraBp06", "rp31", status);
   vvd(rp[2][1], -0.1594554040786495076e-6, 1e-14,
       "eraBp06", "rp32", status);
   vvd(rp[2][2], 0.9999999285984501222, 1e-12,
       "eraBp06", "rp33", status);

   vvd(rbp[0][0], 0.9999995505176007047, 1e-12,
       "eraBp06", "rbp11", status);
   vvd(rbp[0][1], 0.8695404617348208406e-3, 1e-14,
       "eraBp06", "rbp12", status);
   vvd(rbp[0][2], 0.3779735201865589104e-3, 1e-14,
       "eraBp06", "rbp13", status);
   vvd(rbp[1][0], -0.8695404723772031414e-3, 1e-14,
       "eraBp06", "rbp21", status);
   vvd(rbp[1][1], 0.9999996219496027161, 1e-12,
       "eraBp06", "rbp22", status);
   vvd(rbp[1][2], -0.1361752497080270143e-6, 1e-14,
       "eraBp06", "rbp23", status);
   vvd(rbp[2][0], -0.3779734957034089490e-3, 1e-14,
       "eraBp06", "rbp31", status);
   vvd(rbp[2][1], -0.1924880847894457113e-6, 1e-14,
       "eraBp06", "rbp32", status);
   vvd(rbp[2][2], 0.9999999285679971958, 1e-12,
       "eraBp06", "rbp33", status);
}

static void t_bpn2xy(int *status)
/*
**  - - - - - - - - -
**   t _ b p n 2 x y
**  - - - - - - - - -
**
**  Test eraBpn2xy function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraBpn2xy, vvd
**
**  This revision:  2013 August 7
*/
{
   double rbpn[3][3], x, y;


   rbpn[0][0] =  9.999962358680738e-1;
   rbpn[0][1] = -2.516417057665452e-3;
   rbpn[0][2] = -1.093569785342370e-3;

   rbpn[1][0] =  2.516462370370876e-3;
   rbpn[1][1] =  9.999968329010883e-1;
   rbpn[1][2] =  4.006159587358310e-5;

   rbpn[2][0] =  1.093465510215479e-3;
   rbpn[2][1] = -4.281337229063151e-5;
   rbpn[2][2] =  9.999994012499173e-1;

   eraBpn2xy(rbpn, &x, &y);

   vvd(x,  1.093465510215479e-3, 1e-12, "eraBpn2xy", "x", status);
   vvd(y, -4.281337229063151e-5, 1e-12, "eraBpn2xy", "y", status);

}

static void t_c2i00a(int *status)
/*
**  - - - - - - - - -
**   t _ c 2 i 0 0 a
**  - - - - - - - - -
**
**  Test eraC2i00a function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraC2i00a, vvd
**
**  This revision:  2013 August 7
*/
{
   double rc2i[3][3];


   eraC2i00a(2400000.5, 53736.0, rc2i);

   vvd(rc2i[0][0], 0.9999998323037165557, 1e-12,
       "eraC2i00a", "11", status);
   vvd(rc2i[0][1], 0.5581526348992140183e-9, 1e-12,
       "eraC2i00a", "12", status);
   vvd(rc2i[0][2], -0.5791308477073443415e-3, 1e-12,
       "eraC2i00a", "13", status);

   vvd(rc2i[1][0], -0.2384266227870752452e-7, 1e-12,
       "eraC2i00a", "21", status);
   vvd(rc2i[1][1], 0.9999999991917405258, 1e-12,
       "eraC2i00a", "22", status);
   vvd(rc2i[1][2], -0.4020594955028209745e-4, 1e-12,
       "eraC2i00a", "23", status);

   vvd(rc2i[2][0], 0.5791308472168152904e-3, 1e-12,
       "eraC2i00a", "31", status);
   vvd(rc2i[2][1], 0.4020595661591500259e-4, 1e-12,
       "eraC2i00a", "32", status);
   vvd(rc2i[2][2], 0.9999998314954572304, 1e-12,
       "eraC2i00a", "33", status);

}

static void t_c2i00b(int *status)
/*
**  - - - - - - - - -
**   t _ c 2 i 0 0 b
**  - - - - - - - - -
**
**  Test eraC2i00b function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraC2i00b, vvd
**
**  This revision:  2013 August 7
*/
{
   double rc2i[3][3];


   eraC2i00b(2400000.5, 53736.0, rc2i);

   vvd(rc2i[0][0], 0.9999998323040954356, 1e-12,
       "eraC2i00b", "11", status);
   vvd(rc2i[0][1], 0.5581526349131823372e-9, 1e-12,
       "eraC2i00b", "12", status);
   vvd(rc2i[0][2], -0.5791301934855394005e-3, 1e-12,
       "eraC2i00b", "13", status);

   vvd(rc2i[1][0], -0.2384239285499175543e-7, 1e-12,
       "eraC2i00b", "21", status);
   vvd(rc2i[1][1], 0.9999999991917574043, 1e-12,
       "eraC2i00b", "22", status);
   vvd(rc2i[1][2], -0.4020552974819030066e-4, 1e-12,
       "eraC2i00b", "23", status);

   vvd(rc2i[2][0], 0.5791301929950208873e-3, 1e-12,
       "eraC2i00b", "31", status);
   vvd(rc2i[2][1], 0.4020553681373720832e-4, 1e-12,
       "eraC2i00b", "32", status);
   vvd(rc2i[2][2], 0.9999998314958529887, 1e-12,
       "eraC2i00b", "33", status);

}

static void t_c2i06a(int *status)
/*
**  - - - - - - - - -
**   t _ c 2 i 0 6 a
**  - - - - - - - - -
**
**  Test eraC2i06a function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraC2i06a, vvd
**
**  This revision:  2013 August 7
*/
{
   double rc2i[3][3];


   eraC2i06a(2400000.5, 53736.0, rc2i);

   vvd(rc2i[0][0], 0.9999998323037159379, 1e-12,
       "eraC2i06a", "11", status);
   vvd(rc2i[0][1], 0.5581121329587613787e-9, 1e-12,
       "eraC2i06a", "12", status);
   vvd(rc2i[0][2], -0.5791308487740529749e-3, 1e-12,
       "eraC2i06a", "13", status);

   vvd(rc2i[1][0], -0.2384253169452306581e-7, 1e-12,
       "eraC2i06a", "21", status);
   vvd(rc2i[1][1], 0.9999999991917467827, 1e-12,
       "eraC2i06a", "22", status);
   vvd(rc2i[1][2], -0.4020579392895682558e-4, 1e-12,
       "eraC2i06a", "23", status);

   vvd(rc2i[2][0], 0.5791308482835292617e-3, 1e-12,
       "eraC2i06a", "31", status);
   vvd(rc2i[2][1], 0.4020580099454020310e-4, 1e-12,
       "eraC2i06a", "32", status);
   vvd(rc2i[2][2], 0.9999998314954628695, 1e-12,
       "eraC2i06a", "33", status);

}

static void t_c2ibpn(int *status)
/*
**  - - - - - - - - -
**   t _ c 2 i b p n
**  - - - - - - - - -
**
**  Test eraC2ibpn function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraC2ibpn, vvd
**
**  This revision:  2013 August 7
*/
{
   double rbpn[3][3], rc2i[3][3];


   rbpn[0][0] =  9.999962358680738e-1;
   rbpn[0][1] = -2.516417057665452e-3;
   rbpn[0][2] = -1.093569785342370e-3;

   rbpn[1][0] =  2.516462370370876e-3;
   rbpn[1][1] =  9.999968329010883e-1;
   rbpn[1][2] =  4.006159587358310e-5;

   rbpn[2][0] =  1.093465510215479e-3;
   rbpn[2][1] = -4.281337229063151e-5;
   rbpn[2][2] =  9.999994012499173e-1;

   eraC2ibpn(2400000.5, 50123.9999, rbpn, rc2i);

   vvd(rc2i[0][0], 0.9999994021664089977, 1e-12,
       "eraC2ibpn", "11", status);
   vvd(rc2i[0][1], -0.3869195948017503664e-8, 1e-12,
       "eraC2ibpn", "12", status);
   vvd(rc2i[0][2], -0.1093465511383285076e-2, 1e-12,
       "eraC2ibpn", "13", status);

   vvd(rc2i[1][0], 0.5068413965715446111e-7, 1e-12,
       "eraC2ibpn", "21", status);
   vvd(rc2i[1][1], 0.9999999990835075686, 1e-12,
       "eraC2ibpn", "22", status);
   vvd(rc2i[1][2], 0.4281334246452708915e-4, 1e-12,
       "eraC2ibpn", "23", status);

   vvd(rc2i[2][0], 0.1093465510215479000e-2, 1e-12,
       "eraC2ibpn", "31", status);
   vvd(rc2i[2][1], -0.4281337229063151000e-4, 1e-12,
       "eraC2ibpn", "32", status);
   vvd(rc2i[2][2], 0.9999994012499173103, 1e-12,
       "eraC2ibpn", "33", status);

}

static void t_c2ixy(int *status)
/*
**  - - - - - - - -
**   t _ c 2 i x y
**  - - - - - - - -
**
**  Test eraC2ixy function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraC2ixy, vvd
**
**  This revision:  2013 August 7
*/
{
   double x, y, rc2i[3][3];


   x = 0.5791308486706011000e-3;
   y = 0.4020579816732961219e-4;

   eraC2ixy(2400000.5, 53736, x, y, rc2i);

   vvd(rc2i[0][0], 0.9999998323037157138, 1e-12,
       "eraC2ixy", "11", status);
   vvd(rc2i[0][1], 0.5581526349032241205e-9, 1e-12,
       "eraC2ixy", "12", status);
   vvd(rc2i[0][2], -0.5791308491611263745e-3, 1e-12,
       "eraC2ixy", "13", status);

   vvd(rc2i[1][0], -0.2384257057469842953e-7, 1e-12,
       "eraC2ixy", "21", status);
   vvd(rc2i[1][1], 0.9999999991917468964, 1e-12,
       "eraC2ixy", "22", status);
   vvd(rc2i[1][2], -0.4020579110172324363e-4, 1e-12,
       "eraC2ixy", "23", status);

   vvd(rc2i[2][0], 0.5791308486706011000e-3, 1e-12,
       "eraC2ixy", "31", status);
   vvd(rc2i[2][1], 0.4020579816732961219e-4, 1e-12,
       "eraC2ixy", "32", status);
   vvd(rc2i[2][2], 0.9999998314954627590, 1e-12,
       "eraC2ixy", "33", status);

}

static void t_c2ixys(int *status)
/*
**  - - - - - - - - -
**   t _ c 2 i x y s
**  - - - - - - - - -
**
**  Test eraC2ixys function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraC2ixys, vvd
**
**  This revision:  2013 August 7
*/
{
   double x, y, s, rc2i[3][3];


   x =  0.5791308486706011000e-3;
   y =  0.4020579816732961219e-4;
   s = -0.1220040848472271978e-7;

   eraC2ixys(x, y, s, rc2i);

   vvd(rc2i[0][0], 0.9999998323037157138, 1e-12,
       "eraC2ixys", "11", status);
   vvd(rc2i[0][1], 0.5581984869168499149e-9, 1e-12,
       "eraC2ixys", "12", status);
   vvd(rc2i[0][2], -0.5791308491611282180e-3, 1e-12,
       "eraC2ixys", "13", status);

   vvd(rc2i[1][0], -0.2384261642670440317e-7, 1e-12,
       "eraC2ixys", "21", status);
   vvd(rc2i[1][1], 0.9999999991917468964, 1e-12,
       "eraC2ixys", "22", status);
   vvd(rc2i[1][2], -0.4020579110169668931e-4, 1e-12,
       "eraC2ixys", "23", status);

   vvd(rc2i[2][0], 0.5791308486706011000e-3, 1e-12,
       "eraC2ixys", "31", status);
   vvd(rc2i[2][1], 0.4020579816732961219e-4, 1e-12,
       "eraC2ixys", "32", status);
   vvd(rc2i[2][2], 0.9999998314954627590, 1e-12,
       "eraC2ixys", "33", status);

}

static void t_c2s(int *status)
/*
**  - - - - - -
**   t _ c 2 s
**  - - - - - -
**
**  Test eraC2s function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraC2s, vvd
**
**  This revision:  2013 August 7
*/
{
   double p[3], theta, phi;


   p[0] = 100.0;
   p[1] = -50.0;
   p[2] =  25.0;

   eraC2s(p, &theta, &phi);

   vvd(theta, -0.4636476090008061162, 1e-14, "eraC2s", "theta", status);
   vvd(phi, 0.2199879773954594463, 1e-14, "eraC2s", "phi", status);

}

static void t_c2t00a(int *status)
/*
**  - - - - - - - - -
**   t _ c 2 t 0 0 a
**  - - - - - - - - -
**
**  Test eraC2t00a function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraC2t00a, vvd
**
**  This revision:  2013 August 7
*/
{
   double tta, ttb, uta, utb, xp, yp, rc2t[3][3];


   tta = 2400000.5;
   uta = 2400000.5;
   ttb = 53736.0;
   utb = 53736.0;
   xp = 2.55060238e-7;
   yp = 1.860359247e-6;

   eraC2t00a(tta, ttb, uta, utb, xp, yp, rc2t);

   vvd(rc2t[0][0], -0.1810332128307182668, 1e-12,
       "eraC2t00a", "11", status);
   vvd(rc2t[0][1], 0.9834769806938457836, 1e-12,
       "eraC2t00a", "12", status);
   vvd(rc2t[0][2], 0.6555535638688341725e-4, 1e-12,
       "eraC2t00a", "13", status);

   vvd(rc2t[1][0], -0.9834768134135984552, 1e-12,
       "eraC2t00a", "21", status);
   vvd(rc2t[1][1], -0.1810332203649520727, 1e-12,
       "eraC2t00a", "22", status);
   vvd(rc2t[1][2], 0.5749801116141056317e-3, 1e-12,
       "eraC2t00a", "23", status);

   vvd(rc2t[2][0], 0.5773474014081406921e-3, 1e-12,
       "eraC2t00a", "31", status);
   vvd(rc2t[2][1], 0.3961832391770163647e-4, 1e-12,
       "eraC2t00a", "32", status);
   vvd(rc2t[2][2], 0.9999998325501692289, 1e-12,
       "eraC2t00a", "33", status);

}

static void t_c2t00b(int *status)
/*
**  - - - - - - - - -
**   t _ c 2 t 0 0 b
**  - - - - - - - - -
**
**  Test eraC2t00b function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraC2t00b, vvd
**
**  This revision:  2013 August 7
*/
{
   double tta, ttb, uta, utb, xp, yp, rc2t[3][3];


   tta = 2400000.5;
   uta = 2400000.5;
   ttb = 53736.0;
   utb = 53736.0;
   xp = 2.55060238e-7;
   yp = 1.860359247e-6;

   eraC2t00b(tta, ttb, uta, utb, xp, yp, rc2t);

   vvd(rc2t[0][0], -0.1810332128439678965, 1e-12,
       "eraC2t00b", "11", status);
   vvd(rc2t[0][1], 0.9834769806913872359, 1e-12,
       "eraC2t00b", "12", status);
   vvd(rc2t[0][2], 0.6555565082458415611e-4, 1e-12,
       "eraC2t00b", "13", status);

   vvd(rc2t[1][0], -0.9834768134115435923, 1e-12,
       "eraC2t00b", "21", status);
   vvd(rc2t[1][1], -0.1810332203784001946, 1e-12,
       "eraC2t00b", "22", status);
   vvd(rc2t[1][2], 0.5749793922030017230e-3, 1e-12,
       "eraC2t00b", "23", status);

   vvd(rc2t[2][0], 0.5773467471863534901e-3, 1e-12,
       "eraC2t00b", "31", status);
   vvd(rc2t[2][1], 0.3961790411549945020e-4, 1e-12,
       "eraC2t00b", "32", status);
   vvd(rc2t[2][2], 0.9999998325505635738, 1e-12,
       "eraC2t00b", "33", status);

}

static void t_c2t06a(int *status)
/*
**  - - - - - - - - -
**   t _ c 2 t 0 6 a
**  - - - - - - - - -
**
**  Test eraC2t06a function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraC2t06a, vvd
**
**  This revision:  2013 August 7
*/
{
   double tta, ttb, uta, utb, xp, yp, rc2t[3][3];


   tta = 2400000.5;
   uta = 2400000.5;
   ttb = 53736.0;
   utb = 53736.0;
   xp = 2.55060238e-7;
   yp = 1.860359247e-6;

   eraC2t06a(tta, ttb, uta, utb, xp, yp, rc2t);

   vvd(rc2t[0][0], -0.1810332128305897282, 1e-12,
       "eraC2t06a", "11", status);
   vvd(rc2t[0][1], 0.9834769806938592296, 1e-12,
       "eraC2t06a", "12", status);
   vvd(rc2t[0][2], 0.6555550962998436505e-4, 1e-12,
       "eraC2t06a", "13", status);

   vvd(rc2t[1][0], -0.9834768134136214897, 1e-12,
       "eraC2t06a", "21", status);
   vvd(rc2t[1][1], -0.1810332203649130832, 1e-12,
       "eraC2t06a", "22", status);
   vvd(rc2t[1][2], 0.5749800844905594110e-3, 1e-12,
       "eraC2t06a", "23", status);

   vvd(rc2t[2][0], 0.5773474024748545878e-3, 1e-12,
       "eraC2t06a", "31", status);
   vvd(rc2t[2][1], 0.3961816829632690581e-4, 1e-12,
       "eraC2t06a", "32", status);
   vvd(rc2t[2][2], 0.9999998325501747785, 1e-12,
       "eraC2t06a", "33", status);

}

static void t_c2tcio(int *status)
/*
**  - - - - - - - - -
**   t _ c 2 t c i o
**  - - - - - - - - -
**
**  Test eraC2tcio function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraC2tcio, vvd
**
**  This revision:  2013 August 7
*/
{
   double rc2i[3][3], era, rpom[3][3], rc2t[3][3];


   rc2i[0][0] =  0.9999998323037164738;
   rc2i[0][1] =  0.5581526271714303683e-9;
   rc2i[0][2] = -0.5791308477073443903e-3;

   rc2i[1][0] = -0.2384266227524722273e-7;
   rc2i[1][1] =  0.9999999991917404296;
   rc2i[1][2] = -0.4020594955030704125e-4;

   rc2i[2][0] =  0.5791308472168153320e-3;
   rc2i[2][1] =  0.4020595661593994396e-4;
   rc2i[2][2] =  0.9999998314954572365;

   era = 1.75283325530307;

   rpom[0][0] =  0.9999999999999674705;
   rpom[0][1] = -0.1367174580728847031e-10;
   rpom[0][2] =  0.2550602379999972723e-6;

   rpom[1][0] =  0.1414624947957029721e-10;
   rpom[1][1] =  0.9999999999982694954;
   rpom[1][2] = -0.1860359246998866338e-5;

   rpom[2][0] = -0.2550602379741215275e-6;
   rpom[2][1] =  0.1860359247002413923e-5;
   rpom[2][2] =  0.9999999999982369658;


   eraC2tcio(rc2i, era, rpom, rc2t);

   vvd(rc2t[0][0], -0.1810332128307110439, 1e-12,
       "eraC2tcio", "11", status);
   vvd(rc2t[0][1], 0.9834769806938470149, 1e-12,
       "eraC2tcio", "12", status);
   vvd(rc2t[0][2], 0.6555535638685466874e-4, 1e-12,
       "eraC2tcio", "13", status);

   vvd(rc2t[1][0], -0.9834768134135996657, 1e-12,
       "eraC2tcio", "21", status);
   vvd(rc2t[1][1], -0.1810332203649448367, 1e-12,
       "eraC2tcio", "22", status);
   vvd(rc2t[1][2], 0.5749801116141106528e-3, 1e-12,
       "eraC2tcio", "23", status);

   vvd(rc2t[2][0], 0.5773474014081407076e-3, 1e-12,
       "eraC2tcio", "31", status);
   vvd(rc2t[2][1], 0.3961832391772658944e-4, 1e-12,
       "eraC2tcio", "32", status);
   vvd(rc2t[2][2], 0.9999998325501691969, 1e-12,
       "eraC2tcio", "33", status);

}

static void t_c2teqx(int *status)
/*
**  - - - - - - - - -
**   t _ c 2 t e q x
**  - - - - - - - - -
**
**  Test eraC2teqx function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraC2teqx, vvd
**
**  This revision:  2013 August 7
*/
{
   double rbpn[3][3], gst, rpom[3][3], rc2t[3][3];


   rbpn[0][0] =  0.9999989440476103608;
   rbpn[0][1] = -0.1332881761240011518e-2;
   rbpn[0][2] = -0.5790767434730085097e-3;

   rbpn[1][0] =  0.1332858254308954453e-2;
   rbpn[1][1] =  0.9999991109044505944;
   rbpn[1][2] = -0.4097782710401555759e-4;

   rbpn[2][0] =  0.5791308472168153320e-3;
   rbpn[2][1] =  0.4020595661593994396e-4;
   rbpn[2][2] =  0.9999998314954572365;

   gst = 1.754166138040730516;

   rpom[0][0] =  0.9999999999999674705;
   rpom[0][1] = -0.1367174580728847031e-10;
   rpom[0][2] =  0.2550602379999972723e-6;

   rpom[1][0] =  0.1414624947957029721e-10;
   rpom[1][1] =  0.9999999999982694954;
   rpom[1][2] = -0.1860359246998866338e-5;

   rpom[2][0] = -0.2550602379741215275e-6;
   rpom[2][1] =  0.1860359247002413923e-5;
   rpom[2][2] =  0.9999999999982369658;

   eraC2teqx(rbpn, gst, rpom, rc2t);

   vvd(rc2t[0][0], -0.1810332128528685730, 1e-12,
       "eraC2teqx", "11", status);
   vvd(rc2t[0][1], 0.9834769806897685071, 1e-12,
       "eraC2teqx", "12", status);
   vvd(rc2t[0][2], 0.6555535639982634449e-4, 1e-12,
       "eraC2teqx", "13", status);

   vvd(rc2t[1][0], -0.9834768134095211257, 1e-12,
       "eraC2teqx", "21", status);
   vvd(rc2t[1][1], -0.1810332203871023800, 1e-12,
       "eraC2teqx", "22", status);
   vvd(rc2t[1][2], 0.5749801116126438962e-3, 1e-12,
       "eraC2teqx", "23", status);

   vvd(rc2t[2][0], 0.5773474014081539467e-3, 1e-12,
       "eraC2teqx", "31", status);
   vvd(rc2t[2][1], 0.3961832391768640871e-4, 1e-12,
       "eraC2teqx", "32", status);
   vvd(rc2t[2][2], 0.9999998325501691969, 1e-12,
       "eraC2teqx", "33", status);

}

static void t_c2tpe(int *status)
/*
**  - - - - - - - -
**   t _ c 2 t p e
**  - - - - - - - -
**
**  Test eraC2tpe function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraC2tpe, vvd
**
**  This revision:  2013 August 7
*/
{
   double tta, ttb, uta, utb, dpsi, deps, xp, yp, rc2t[3][3];


   tta = 2400000.5;
   uta = 2400000.5;
   ttb = 53736.0;
   utb = 53736.0;
   deps =  0.4090789763356509900;
   dpsi = -0.9630909107115582393e-5;
   xp = 2.55060238e-7;
   yp = 1.860359247e-6;

   eraC2tpe(tta, ttb, uta, utb, dpsi, deps, xp, yp, rc2t);

   vvd(rc2t[0][0], -0.1813677995763029394, 1e-12,
       "eraC2tpe", "11", status);
   vvd(rc2t[0][1], 0.9023482206891683275, 1e-12,
       "eraC2tpe", "12", status);
   vvd(rc2t[0][2], -0.3909902938641085751, 1e-12,
       "eraC2tpe", "13", status);

   vvd(rc2t[1][0], -0.9834147641476804807, 1e-12,
       "eraC2tpe", "21", status);
   vvd(rc2t[1][1], -0.1659883635434995121, 1e-12,
       "eraC2tpe", "22", status);
   vvd(rc2t[1][2], 0.7309763898042819705e-1, 1e-12,
       "eraC2tpe", "23", status);

   vvd(rc2t[2][0], 0.1059685430673215247e-2, 1e-12,
       "eraC2tpe", "31", status);
   vvd(rc2t[2][1], 0.3977631855605078674, 1e-12,
       "eraC2tpe", "32", status);
   vvd(rc2t[2][2], 0.9174875068792735362, 1e-12,
       "eraC2tpe", "33", status);

}

static void t_c2txy(int *status)
/*
**  - - - - - - - -
**   t _ c 2 t x y
**  - - - - - - - -
**
**  Test eraC2txy function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraC2txy, vvd
**
**  This revision:  2013 August 7
*/
{
   double tta, ttb, uta, utb, x, y, xp, yp, rc2t[3][3];


   tta = 2400000.5;
   uta = 2400000.5;
   ttb = 53736.0;
   utb = 53736.0;
   x = 0.5791308486706011000e-3;
   y = 0.4020579816732961219e-4;
   xp = 2.55060238e-7;
   yp = 1.860359247e-6;

   eraC2txy(tta, ttb, uta, utb, x, y, xp, yp, rc2t);

   vvd(rc2t[0][0], -0.1810332128306279253, 1e-12,
       "eraC2txy", "11", status);
   vvd(rc2t[0][1], 0.9834769806938520084, 1e-12,
       "eraC2txy", "12", status);
   vvd(rc2t[0][2], 0.6555551248057665829e-4, 1e-12,
       "eraC2txy", "13", status);

   vvd(rc2t[1][0], -0.9834768134136142314, 1e-12,
       "eraC2txy", "21", status);
   vvd(rc2t[1][1], -0.1810332203649529312, 1e-12,
       "eraC2txy", "22", status);
   vvd(rc2t[1][2], 0.5749800843594139912e-3, 1e-12,
       "eraC2txy", "23", status);

   vvd(rc2t[2][0], 0.5773474028619264494e-3, 1e-12,
       "eraC2txy", "31", status);
   vvd(rc2t[2][1], 0.3961816546911624260e-4, 1e-12,
       "eraC2txy", "32", status);
   vvd(rc2t[2][2], 0.9999998325501746670, 1e-12,
       "eraC2txy", "33", status);

}

static void t_cal2jd(int *status)
/*
**  - - - - - - - - -
**   t _ c a l 2 j d
**  - - - - - - - - -
**
**  Test eraCal2jd function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraCal2jd, vvd, viv
**
**  This revision:  2013 August 7
*/
{
   int j;
   double djm0, djm;


   j = eraCal2jd(2003, 06, 01, &djm0, &djm);

   vvd(djm0, 2400000.5, 0.0, "eraCal2jd", "djm0", status);
   vvd(djm,    52791.0, 0.0, "eraCal2jd", "djm", status);

   viv(j, 0, "eraCal2jd", "j", status);

}

static void t_cp(int *status)
/*
**  - - - - -
**   t _ c p
**  - - - - -
**
**  Test eraCp function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraCp, vvd
**
**  This revision:  2013 August 7
*/
{
   double p[3], c[3];


   p[0] =  0.3;
   p[1] =  1.2;
   p[2] = -2.5;

   eraCp(p, c);

   vvd(c[0],  0.3, 0.0, "eraCp", "1", status);
   vvd(c[1],  1.2, 0.0, "eraCp", "2", status);
   vvd(c[2], -2.5, 0.0, "eraCp", "3", status);
}

static void t_cpv(int *status)
/*
**  - - - - - -
**   t _ c p v
**  - - - - - -
**
**  Test eraCpv function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraCpv, vvd
**
**  This revision:  2013 August 7
*/
{
   double pv[2][3], c[2][3];


   pv[0][0] =  0.3;
   pv[0][1] =  1.2;
   pv[0][2] = -2.5;

   pv[1][0] = -0.5;
   pv[1][1] =  3.1;
   pv[1][2] =  0.9;

   eraCpv(pv, c);

   vvd(c[0][0],  0.3, 0.0, "eraCpv", "p1", status);
   vvd(c[0][1],  1.2, 0.0, "eraCpv", "p2", status);
   vvd(c[0][2], -2.5, 0.0, "eraCpv", "p3", status);

   vvd(c[1][0], -0.5, 0.0, "eraCpv", "v1", status);
   vvd(c[1][1],  3.1, 0.0, "eraCpv", "v2", status);
   vvd(c[1][2],  0.9, 0.0, "eraCpv", "v3", status);

}

static void t_cr(int *status)
/*
**  - - - - -
**   t _ c r
**  - - - - -
**
**  Test eraCr function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraCr, vvd
**
**  This revision:  2013 August 7
*/
{
   double r[3][3], c[3][3];


   r[0][0] = 2.0;
   r[0][1] = 3.0;
   r[0][2] = 2.0;

   r[1][0] = 3.0;
   r[1][1] = 2.0;
   r[1][2] = 3.0;

   r[2][0] = 3.0;
   r[2][1] = 4.0;
   r[2][2] = 5.0;

   eraCr(r, c);

   vvd(c[0][0], 2.0, 0.0, "eraCr", "11", status);
   vvd(c[0][1], 3.0, 0.0, "eraCr", "12", status);
   vvd(c[0][2], 2.0, 0.0, "eraCr", "13", status);

   vvd(c[1][0], 3.0, 0.0, "eraCr", "21", status);
   vvd(c[1][1], 2.0, 0.0, "eraCr", "22", status);
   vvd(c[1][2], 3.0, 0.0, "eraCr", "23", status);

   vvd(c[2][0], 3.0, 0.0, "eraCr", "31", status);
   vvd(c[2][1], 4.0, 0.0, "eraCr", "32", status);
   vvd(c[2][2], 5.0, 0.0, "eraCr", "33", status);
}

static void t_d2dtf(int *status )
/*
**  - - - - - - - -
**   t _ d 2 d t f
**  - - - - - - - -
**
**  Test eraD2dtf function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraD2dtf, viv
**
**  This revision:  2013 August 7
*/
{
   int j, iy, im, id, ihmsf[4];


   j = eraD2dtf("UTC", 5, 2400000.5, 49533.99999, &iy, &im, &id, ihmsf);

   viv(iy, 1994, "eraD2dtf", "y", status);
   viv(im, 6, "eraD2dtf", "mo", status);
   viv(id, 30, "eraD2dtf", "d", status);
   viv(ihmsf[0], 23, "eraD2dtf", "h", status);
   viv(ihmsf[1], 59, "eraD2dtf", "m", status);
   viv(ihmsf[2], 60, "eraD2dtf", "s", status);
   viv(ihmsf[3], 13599, "eraD2dtf", "f", status);
   viv(j, 0, "eraD2dtf", "j", status);

}

static void t_d2tf(int *status)
/*
**  - - - - - - -
**   t _ d 2 t f
**  - - - - - - -
**
**  Test eraD2tf function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraD2tf, viv, vvd
**
**  This revision:  2013 August 7
*/
{
   int ihmsf[4];
   char s;


   eraD2tf(4, -0.987654321, &s, ihmsf);

   viv((int)s, '-', "eraD2tf", "s", status);

   viv(ihmsf[0], 23, "eraD2tf", "0", status);
   viv(ihmsf[1], 42, "eraD2tf", "1", status);
   viv(ihmsf[2], 13, "eraD2tf", "2", status);
   viv(ihmsf[3], 3333, "eraD2tf", "3", status);

}

static void t_dat(int *status)
/*
**  - - - - - -
**   t _ d a t
**  - - - - - -
**
**  Test eraDat function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraDat, vvd, viv
**
**  This revision:  2016 July 11
*/
{
   int j;
   double deltat;


   j = eraDat(2003, 6, 1, 0.0, &deltat);

   vvd(deltat, 32.0, 0.0, "eraDat", "d1", status);
   viv(j, 0, "eraDat", "j1", status);

   j = eraDat(2008, 1, 17, 0.0, &deltat);

   vvd(deltat, 33.0, 0.0, "eraDat", "d2", status);
   viv(j, 0, "eraDat", "j2", status);

   j = eraDat(2017, 9, 1, 0.0, &deltat);

   vvd(deltat, 37.0, 0.0, "eraDat", "d3", status);
   viv(j, 0, "eraDat", "j3", status);

}

static void t_dtdb(int *status)
/*
**  - - - - - - -
**   t _ d t d b
**  - - - - - - -
**
**  Test eraDtdb function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraDtdb, vvd
**
**  This revision:  2013 August 7
*/
{
   double dtdb;


   dtdb = eraDtdb(2448939.5, 0.123, 0.76543, 5.0123, 5525.242, 3190.0);

   vvd(dtdb, -0.1280368005936998991e-2, 1e-15, "eraDtdb", "", status);

}

static void t_dtf2d(int *status)
/*
**  - - - - - - - -
**   t _ d t f 2 d
**  - - - - - - - -
**
**  Test eraDtf2d function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraDtf2d, vvd, viv
**
**  This revision:  2013 August 7
*/
{
   double u1, u2;
   int j;


   j = eraDtf2d("UTC", 1994, 6, 30, 23, 59, 60.13599, &u1, &u2);

   vvd(u1+u2, 2449534.49999, 1e-6, "eraDtf2d", "u", status);
   viv(j, 0, "eraDtf2d", "j", status);

}

static void t_eceq06(int *status)
/*
**  - - - - -
**   t _ e c e q 0 6
**  - - - - -
**
**  Test eraEceq06 function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraEceq06, vvd
**
**  This revision:  2016 March 12
*/
{
   double date1, date2, dl, db, dr, dd;


   date1 = 2456165.5;
   date2 = 0.401182685;
   dl = 5.1;
   db = -0.9;

   eraEceq06(date1, date2, dl, db, &dr, &dd);

   vvd(dr, 5.533459733613627767, 1e-14, "eraEceq06", "dr", status);
   vvd(dd, -1.246542932554480576, 1e-14, "eraEceq06", "dd", status);

}

static void t_ecm06(int *status)
/*
**  - - - - - - - -
**   t _ e c m 0 6
**  - - - - - - - -
**
**  Test eraEcm06 function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraEcm06, vvd
**
**  This revision:  2016 March 12
*/
{
   double date1, date2, rm[3][3];


   date1 = 2456165.5;
   date2 = 0.401182685;

   eraEcm06(date1, date2, rm);

   vvd(rm[0][0], 0.9999952427708701137, 1e-14,
       "eraEcm06", "rm11", status);
   vvd(rm[0][1], -0.2829062057663042347e-2, 1e-14,
       "eraEcm06", "rm12", status);
   vvd(rm[0][2], -0.1229163741100017629e-2, 1e-14,
       "eraEcm06", "rm13", status);
   vvd(rm[1][0], 0.3084546876908653562e-2, 1e-14,
       "eraEcm06", "rm21", status);
   vvd(rm[1][1], 0.9174891871550392514, 1e-14,
       "eraEcm06", "rm22", status);
   vvd(rm[1][2], 0.3977487611849338124, 1e-14,
       "eraEcm06", "rm23", status);
   vvd(rm[2][0], 0.2488512951527405928e-5, 1e-14,
       "eraEcm06", "rm31", status);
   vvd(rm[2][1], -0.3977506604161195467, 1e-14,
       "eraEcm06", "rm32", status);
   vvd(rm[2][2], 0.9174935488232863071, 1e-14,
       "eraEcm06", "rm33", status);

}

static void t_ee00(int *status)
/*
**  - - - - - - -
**   t _ e e 0 0
**  - - - - - - -
**
**  Test eraEe00 function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraEe00, vvd
**
**  This revision:  2013 August 7
*/
{
   double epsa, dpsi, ee;


   epsa =  0.4090789763356509900;
   dpsi = -0.9630909107115582393e-5;

   ee = eraEe00(2400000.5, 53736.0, epsa, dpsi);

   vvd(ee, -0.8834193235367965479e-5, 1e-18, "eraEe00", "", status);

}

static void t_ee00a(int *status)
/*
**  - - - - - - - -
**   t _ e e 0 0 a
**  - - - - - - - -
**
**  Test eraEe00a function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraEe00a, vvd
**
**  This revision:  2013 August 7
*/
{
   double ee;


   ee = eraEe00a(2400000.5, 53736.0);

   vvd(ee, -0.8834192459222588227e-5, 1e-18, "eraEe00a", "", status);

}

static void t_ee00b(int *status)
/*
**  - - - - - - - -
**   t _ e e 0 0 b
**  - - - - - - - -
**
**  Test eraEe00b function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraEe00b, vvd
**
**  This revision:  2013 August 7
*/
{
   double ee;


   ee = eraEe00b(2400000.5, 53736.0);

   vvd(ee, -0.8835700060003032831e-5, 1e-18, "eraEe00b", "", status);

}

static void t_ee06a(int *status)
/*
**  - - - - - - - -
**   t _ e e 0 6 a
**  - - - - - - - -
**
**  Test eraEe06a function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraEe06a, vvd
**
**  This revision:  2013 August 7
*/
{
   double ee;


   ee = eraEe06a(2400000.5, 53736.0);

   vvd(ee, -0.8834195072043790156e-5, 1e-15, "eraEe06a", "", status);
}

static void t_eect00(int *status)
/*
**  - - - - - - - - -
**   t _ e e c t 0 0
**  - - - - - - - - -
**
**  Test eraEect00 function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraEect00, vvd
**
**  This revision:  2013 August 7
*/
{
   double eect;


   eect = eraEect00(2400000.5, 53736.0);

   vvd(eect, 0.2046085004885125264e-8, 1e-20, "eraEect00", "", status);

}

static void t_eform(int *status)
/*
**  - - - - - - - -
**   t _ e f o r m
**  - - - - - - - -
**
**  Test eraEform function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraEform, viv, vvd
**
**  This revision:  2016 March 12
*/
{
   int j;
   double a, f;

   j = eraEform(0, &a, &f);

   viv(j, -1, "eraEform", "j0", status);

   j = eraEform(ERFA_WGS84, &a, &f);

   viv(j, 0, "eraEform", "j1", status);
   vvd(a, 6378137.0, 1e-10, "eraEform", "a1", status);
   vvd(f, 0.3352810664747480720e-2, 1e-18, "eraEform", "f1", status);

   j = eraEform(ERFA_GRS80, &a, &f);

   viv(j, 0, "eraEform", "j2", status);
   vvd(a, 6378137.0, 1e-10, "eraEform", "a2", status);
   vvd(f, 0.3352810681182318935e-2, 1e-18, "eraEform", "f2", status);

   j = eraEform(ERFA_WGS72, &a, &f);

   viv(j, 0, "eraEform", "j2", status);
   vvd(a, 6378135.0, 1e-10, "eraEform", "a3", status);
   vvd(f, 0.3352779454167504862e-2, 1e-18, "eraEform", "f3", status);

   j = eraEform(4, &a, &f);
   viv(j, -1, "eraEform", "j3", status);
}

static void t_eo06a(int *status)
/*
**  - - - - - - - -
**   t _ e o 0 6 a
**  - - - - - - - -
**
**  Test eraEo06a function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraEo06a, vvd
**
**  This revision:  2013 August 7
*/
{
   double eo;


   eo = eraEo06a(2400000.5, 53736.0);

   vvd(eo, -0.1332882371941833644e-2, 1e-15, "eraEo06a", "", status);

}

static void t_eors(int *status)
/*
**  - - - - - - -
**   t _ e o r s
**  - - - - - - -
**
**  Test eraEors function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraEors, vvd
**
**  This revision:  2013 August 7
*/
{
   double rnpb[3][3], s, eo;


   rnpb[0][0] =  0.9999989440476103608;
   rnpb[0][1] = -0.1332881761240011518e-2;
   rnpb[0][2] = -0.5790767434730085097e-3;

   rnpb[1][0] =  0.1332858254308954453e-2;
   rnpb[1][1] =  0.9999991109044505944;
   rnpb[1][2] = -0.4097782710401555759e-4;

   rnpb[2][0] =  0.5791308472168153320e-3;
   rnpb[2][1] =  0.4020595661593994396e-4;
   rnpb[2][2] =  0.9999998314954572365;

   s = -0.1220040848472271978e-7;

   eo = eraEors(rnpb, s);

   vvd(eo, -0.1332882715130744606e-2, 1e-14, "eraEors", "", status);

}

static void t_epb(int *status)
/*
**  - - - - - -
**   t _ e p b
**  - - - - - -
**
**  Test eraEpb function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraEpb, vvd
**
**  This revision:  2013 August 7
*/
{
   double epb;


   epb = eraEpb(2415019.8135, 30103.18648);

   vvd(epb, 1982.418424159278580, 1e-12, "eraEpb", "", status);

}

static void t_epb2jd(int *status)
/*
**  - - - - - - - - -
**   t _ e p b 2 j d
**  - - - - - - - - -
**
**  Test eraEpb2jd function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraEpb2jd, vvd
**
**  This revision:  2013 August 7
*/
{
   double epb, djm0, djm;


   epb = 1957.3;

   eraEpb2jd(epb, &djm0, &djm);

   vvd(djm0, 2400000.5, 1e-9, "eraEpb2jd", "djm0", status);
   vvd(djm, 35948.1915101513, 1e-9, "eraEpb2jd", "mjd", status);

}

static void t_epj(int *status)
/*
**  - - - - - -
**   t _ e p j
**  - - - - - -
**
**  Test eraEpj function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraEpj, vvd
**
**  This revision:  2013 August 7
*/
{
   double epj;


   epj = eraEpj(2451545, -7392.5);

   vvd(epj, 1979.760438056125941, 1e-12, "eraEpj", "", status);

}

static void t_epj2jd(int *status)
/*
**  - - - - - - - - -
**   t _ e p j 2 j d
**  - - - - - - - - -
**
**  Test eraEpj2jd function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraEpj2jd, vvd
**
**  This revision:  2013 August 7
*/
{
   double epj, djm0, djm;


   epj = 1996.8;

   eraEpj2jd(epj, &djm0, &djm);

   vvd(djm0, 2400000.5, 1e-9, "eraEpj2jd", "djm0", status);
   vvd(djm,    50375.7, 1e-9, "eraEpj2jd", "mjd",  status);

}

static void t_epv00(int *status)
/*
**  - - - - - - - -
**   t _ e p v 0 0
**  - - - - - - - -
**
**  Test eraEpv00 function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called: eraEpv00, vvd, viv
**
**  This revision:  2013 August 7
*/
{
   double pvh[2][3], pvb[2][3];
   int j;


   j = eraEpv00(2400000.5, 53411.52501161, pvh, pvb);

   vvd(pvh[0][0], -0.7757238809297706813, 1e-14,
       "eraEpv00", "ph(x)", status);
   vvd(pvh[0][1], 0.5598052241363340596, 1e-14,
       "eraEpv00", "ph(y)", status);
   vvd(pvh[0][2], 0.2426998466481686993, 1e-14,
       "eraEpv00", "ph(z)", status);

   vvd(pvh[1][0], -0.1091891824147313846e-1, 1e-15,
       "eraEpv00", "vh(x)", status);
   vvd(pvh[1][1], -0.1247187268440845008e-1, 1e-15,
       "eraEpv00", "vh(y)", status);
   vvd(pvh[1][2], -0.5407569418065039061e-2, 1e-15,
       "eraEpv00", "vh(z)", status);

   vvd(pvb[0][0], -0.7714104440491111971, 1e-14,
       "eraEpv00", "pb(x)", status);
   vvd(pvb[0][1], 0.5598412061824171323, 1e-14,
       "eraEpv00", "pb(y)", status);
   vvd(pvb[0][2], 0.2425996277722452400, 1e-14,
       "eraEpv00", "pb(z)", status);

   vvd(pvb[1][0], -0.1091874268116823295e-1, 1e-15,
       "eraEpv00", "vb(x)", status);
   vvd(pvb[1][1], -0.1246525461732861538e-1, 1e-15,
       "eraEpv00", "vb(y)", status);
   vvd(pvb[1][2], -0.5404773180966231279e-2, 1e-15,
       "eraEpv00", "vb(z)", status);

   viv(j, 0, "eraEpv00", "j", status);

}

static void t_eqec06(int *status)
/*
**  - - - - - - - - -
**   t _ e q e c 0 6
**  - - - - - - - - -
**
**  Test eraEqec06 function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraEqec06, vvd
**
**  This revision:  2016 March 12
*/
{
   double date1, date2, dr, dd, dl, db;


   date1 = 1234.5;
   date2 = 2440000.5;
   dr = 1.234;
   dd = 0.987;

   eraEqec06(date1, date2, dr, dd, &dl, &db);

   vvd(dl, 1.342509918994654619, 1e-14, "eraEqec06", "dl", status);
   vvd(db, 0.5926215259704608132, 1e-14, "eraEqec06", "db", status);

}

static void t_eqeq94(int *status)
/*
**  - - - - - - - - -
**   t _ e q e q 9 4
**  - - - - - - - - -
**
**  Test eraEqeq94 function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraEqeq94, vvd
**
**  This revision:  2013 August 7
*/
{
   double eqeq;


   eqeq = eraEqeq94(2400000.5, 41234.0);

   vvd(eqeq, 0.5357758254609256894e-4, 1e-17, "eraEqeq94", "", status);

}

static void t_era00(int *status)
/*
**  - - - - - - - -
**   t _ e r a 0 0
**  - - - - - - - -
**
**  Test eraEra00 function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraEra00, vvd
**
**  This revision:  2013 August 7
*/
{
   double era00;


   era00 = eraEra00(2400000.5, 54388.0);

   vvd(era00, 0.4022837240028158102, 1e-12, "eraEra00", "", status);

}

static void t_fad03(int *status)
/*
**  - - - - - - - -
**   t _ f a d 0 3
**  - - - - - - - -
**
**  Test eraFad03 function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraFad03, vvd
**
**  This revision:  2013 August 7
*/
{
   vvd(eraFad03(0.80), 1.946709205396925672, 1e-12,
       "eraFad03", "", status);
}

static void t_fae03(int *status)
/*
**  - - - - - - - -
**   t _ f a e 0 3
**  - - - - - - - -
**
**  Test eraFae03 function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraFae03, vvd
**
**  This revision:  2013 August 7
*/
{
   vvd(eraFae03(0.80), 1.744713738913081846, 1e-12,
       "eraFae03", "", status);
}

static void t_faf03(int *status)
/*
**  - - - - - - - -
**   t _ f a f 0 3
**  - - - - - - - -
**
**  Test eraFaf03 function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraFaf03, vvd
**
**  This revision:  2013 August 7
*/
{
   vvd(eraFaf03(0.80), 0.2597711366745499518, 1e-12,
       "eraFaf03", "", status);
}

static void t_faju03(int *status)
/*
**  - - - - - - - - -
**   t _ f a j u 0 3
**  - - - - - - - - -
**
**  Test eraFaju03 function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraFaju03, vvd
**
**  This revision:  2013 August 7
*/
{
   vvd(eraFaju03(0.80), 5.275711665202481138, 1e-12,
       "eraFaju03", "", status);
}

static void t_fal03(int *status)
/*
**  - - - - - - - -
**   t _ f a l 0 3
**  - - - - - - - -
**
**  Test eraFal03 function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraFal03, vvd
**
**  This revision:  2013 August 7
*/
{
   vvd(eraFal03(0.80), 5.132369751108684150, 1e-12,
       "eraFal03", "", status);
}

static void t_falp03(int *status)
/*
**  - - - - - - - - -
**   t _ f a l p 0 3
**  - - - - - - - - -
**
**  Test eraFalp03 function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraFalp03, vvd
**
**  This revision:  2013 August 7
*/
{
   vvd(eraFalp03(0.80), 6.226797973505507345, 1e-12,
      "eraFalp03", "", status);
}

static void t_fama03(int *status)
/*
**  - - - - - - - - -
**   t _ f a m a 0 3
**  - - - - - - - - -
**
**  Test eraFama03 function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraFama03, vvd
**
**  This revision:  2013 August 7
*/
{
   vvd(eraFama03(0.80), 3.275506840277781492, 1e-12,
       "eraFama03", "", status);
}

static void t_fame03(int *status)
/*
**  - - - - - - - - -
**   t _ f a m e 0 3
**  - - - - - - - - -
**
**  Test eraFame03 function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraFame03, vvd
**
**  This revision:  2013 August 7
*/
{
   vvd(eraFame03(0.80), 5.417338184297289661, 1e-12,
       "eraFame03", "", status);
}

static void t_fane03(int *status)
/*
**  - - - - - - - - -
**   t _ f a n e 0 3
**  - - - - - - - - -
**
**  Test eraFane03 function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraFane03, vvd
**
**  This revision:  2013 August 7
*/
{
   vvd(eraFane03(0.80), 2.079343830860413523, 1e-12,
       "eraFane03", "", status);
}

static void t_faom03(int *status)
/*
**  - - - - - - - - -
**   t _ f a o m 0 3
**  - - - - - - - - -
**
**  Test eraFaom03 function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraFaom03, vvd
**
**  This revision:  2013 August 7
*/
{
   vvd(eraFaom03(0.80), -5.973618440951302183, 1e-12,
       "eraFaom03", "", status);
}

static void t_fapa03(int *status)
/*
**  - - - - - - - - -
**   t _ f a p a 0 3
**  - - - - - - - - -
**
**  Test eraFapa03 function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraFapa03, vvd
**
**  This revision:  2013 August 7
*/
{
   vvd(eraFapa03(0.80), 0.1950884762240000000e-1, 1e-12,
       "eraFapa03", "", status);
}

static void t_fasa03(int *status)
/*
**  - - - - - - - - -
**   t _ f a s a 0 3
**  - - - - - - - - -
**
**  Test eraFasa03 function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraFasa03, vvd
**
**  This revision:  2013 August 7
*/
{
   vvd(eraFasa03(0.80), 5.371574539440827046, 1e-12,
       "eraFasa03", "", status);
}

static void t_faur03(int *status)
/*
**  - - - - - - - - -
**   t _ f a u r 0 3
**  - - - - - - - - -
**
**  Test eraFaur03 function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraFaur03, vvd
**
**  This revision:  2013 August 7
*/
{
   vvd(eraFaur03(0.80), 5.180636450180413523, 1e-12,
       "eraFaur03", "", status);
}

static void t_fave03(int *status)
/*
**  - - - - - - - - -
**   t _ f a v e 0 3
**  - - - - - - - - -
**
**  Test eraFave03 function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraFave03, vvd
**
**  This revision:  2013 August 7
*/
{
   vvd(eraFave03(0.80), 3.424900460533758000, 1e-12,
       "eraFave03", "", status);
}

static void t_fk425(int *status)
/*
**  - - - - - - - -
**   t _ f k 4 2 5
**  - - - - - - - -
**
**  Test eraFk425 function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraFk425, vvd
**
**  This revision:  2018 December 6
*/
{
   double r1950, d1950, dr1950, dd1950, p1950, v1950,
          r2000, d2000, dr2000, dd2000, p2000, v2000;


   r1950 = 0.07626899753879587532;
   d1950 = -1.137405378399605780;
   dr1950 = 0.1973749217849087460e-4;
   dd1950 = 0.5659714913272723189e-5;
   p1950 = 0.134;
   v1950 = 8.7;

   eraFk425(r1950, d1950, dr1950, dd1950, p1950, v1950,
            &r2000, &d2000, &dr2000, &dd2000, &p2000, &v2000);

   vvd(r2000, 0.08757989933556446040, 1e-14,
       "eraFk425", "r2000", status);
   vvd(d2000, -1.132279113042091895, 1e-12,
       "eraFk425", "d2000", status);
   vvd(dr2000, 0.1953670614474396139e-4, 1e-17,
       "eraFk425", "dr2000", status);
   vvd(dd2000, 0.5637686678659640164e-5, 1e-18,
       "eraFk425", "dd2000", status);
   vvd(p2000, 0.1339919950582767871, 1e-13, "eraFk425", "p2000", status);
   vvd(v2000, 8.736999669183529069, 1e-12, "eraFk425", "v2000", status);

}

static void t_fk45z(int *status)
/*
**  - - - - - - - -
**   t _ f k 4 5 z
**  - - - - - - - -
**
**  Test eraFk45z function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraFk45z, vvd
**
**  This revision:  2018 December 6
*/
{
   double r1950, d1950, bepoch, r2000, d2000;


   r1950 = 0.01602284975382960982;
   d1950 = -0.1164347929099906024;
   bepoch = 1954.677617625256806;

   eraFk45z(r1950, d1950, bepoch, &r2000, &d2000);

   vvd(r2000, 0.02719295911606862303, 1e-15,
       "eraFk45z", "r2000", status);
   vvd(d2000, -0.1115766001565926892, 1e-13,
       "eraFk45z", "d2000", status);

}

static void t_fk524(int *status)
/*
**  - - - - - - - -
**   t _ f k 5 2 4
**  - - - - - - - -
**
**  Test eraFk524 function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraFk524, vvd
**
**  This revision:  2018 December 6
*/
{
   double r2000, d2000, dr2000, dd2000, p2000, v2000,
          r1950, d1950, dr1950, dd1950, p1950, v1950;


   r2000 = 0.8723503576487275595;
   d2000 = -0.7517076365138887672;
   dr2000 = 0.2019447755430472323e-4;
   dd2000 = 0.3541563940505160433e-5;
   p2000 = 0.1559;
   v2000 = 86.87;

   eraFk524(r2000, d2000, dr2000, dd2000, p2000, v2000,
            &r1950, &d1950, &dr1950,&dd1950, &p1950, &v1950);

   vvd(r1950, 0.8636359659799603487, 1e-13,
       "eraFk524", "r1950", status);
   vvd(d1950, -0.7550281733160843059, 1e-13,
       "eraFk524", "d1950", status);
   vvd(dr1950, 0.2023628192747172486e-4, 1e-17,
       "eraFk524", "dr1950", status);
   vvd(dd1950, 0.3624459754935334718e-5, 1e-18,
       "eraFk524", "dd1950", status);
   vvd(p1950, 0.1560079963299390241, 1e-13,
       "eraFk524", "p1950", status);
   vvd(v1950, 86.79606353469163751, 1e-11, "eraFk524", "v1950", status);

}

static void t_fk52h(int *status)
/*
**  - - - - - - - -
**   t _ f k 5 2 h
**  - - - - - - - -
**
**  Test eraFk52h function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraFk52h, vvd
**
**  This revision:  2021 January 5
*/
{
   double r5, d5, dr5, dd5, px5, rv5, rh, dh, drh, ddh, pxh, rvh;


   r5  =  1.76779433;
   d5  = -0.2917517103;
   dr5 = -1.91851572e-7;
   dd5 = -5.8468475e-6;
   px5 =  0.379210;
   rv5 = -7.6;

   eraFk52h(r5, d5, dr5, dd5, px5, rv5,
            &rh, &dh, &drh, &ddh, &pxh, &rvh);

   vvd(rh, 1.767794226299947632, 1e-14,
       "eraFk52h", "ra", status);
   vvd(dh,  -0.2917516070530391757, 1e-14,
       "eraFk52h", "dec", status);
   vvd(drh, -0.1961874125605721270e-6,1e-19,
       "eraFk52h", "dr5", status);
   vvd(ddh, -0.58459905176693911e-5, 1e-19,
       "eraFk52h", "dd5", status);
   vvd(pxh,  0.37921, 1e-14,
       "eraFk52h", "px", status);
   vvd(rvh, -7.6000000940000254, 1e-11,
       "eraFk52h", "rv", status);

}

static void t_fk54z(int *status)
/*
**  - - - - - - - -
**   t _ f k 5 4 z
**  - - - - - - - -
**
**  Test eraFk54z function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraFk54z, vvd
**
**  This revision:  2018 December 6
*/
{
   double r2000, d2000, bepoch, r1950, d1950, dr1950, dd1950;


   r2000 = 0.02719026625066316119;
   d2000 = -0.1115815170738754813;
   bepoch = 1954.677308160316374;

   eraFk54z(r2000, d2000, bepoch, &r1950, &d1950, &dr1950, &dd1950);

   vvd(r1950, 0.01602015588390065476, 1e-14,
       "eraFk54z", "r1950", status);
   vvd(d1950, -0.1164397101110765346, 1e-13,
       "eraFk54z", "d1950", status);
   vvd(dr1950, -0.1175712648471090704e-7, 1e-20,
       "eraFk54z", "dr1950", status);
   vvd(dd1950, 0.2108109051316431056e-7, 1e-20,
       "eraFk54z", "dd1950", status);

}

static void t_fk5hip(int *status)
/*
**  - - - - - - - - -
**   t _ f k 5 h i p
**  - - - - - - - - -
**
**  Test eraFk5hip function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraFk5hip, vvd
**
**  This revision:  2013 August 7
*/
{
   double r5h[3][3], s5h[3];


   eraFk5hip(r5h, s5h);

   vvd(r5h[0][0], 0.9999999999999928638, 1e-14,
       "eraFk5hip", "11", status);
   vvd(r5h[0][1], 0.1110223351022919694e-6, 1e-17,
       "eraFk5hip", "12", status);
   vvd(r5h[0][2], 0.4411803962536558154e-7, 1e-17,
       "eraFk5hip", "13", status);
   vvd(r5h[1][0], -0.1110223308458746430e-6, 1e-17,
       "eraFk5hip", "21", status);
   vvd(r5h[1][1], 0.9999999999999891830, 1e-14,
       "eraFk5hip", "22", status);
   vvd(r5h[1][2], -0.9647792498984142358e-7, 1e-17,
       "eraFk5hip", "23", status);
   vvd(r5h[2][0], -0.4411805033656962252e-7, 1e-17,
       "eraFk5hip", "31", status);
   vvd(r5h[2][1], 0.9647792009175314354e-7, 1e-17,
       "eraFk5hip", "32", status);
   vvd(r5h[2][2], 0.9999999999999943728, 1e-14,
       "eraFk5hip", "33", status);
   vvd(s5h[0], -0.1454441043328607981e-8, 1e-17,
       "eraFk5hip", "s1", status);
   vvd(s5h[1], 0.2908882086657215962e-8, 1e-17,
       "eraFk5hip", "s2", status);
   vvd(s5h[2], 0.3393695767766751955e-8, 1e-17,
       "eraFk5hip", "s3", status);

}

static void t_fk5hz(int *status)
/*
**  - - - - - - - -
**   t _ f k 5 h z
**  - - - - - - - -
**
**  Test eraFk5hz function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraFk5hz, vvd
**
**  This revision:  2013 August 7
*/
{
   double r5, d5, rh, dh;


   r5 =  1.76779433;
   d5 = -0.2917517103;

   eraFk5hz(r5, d5, 2400000.5, 54479.0, &rh, &dh);

   vvd(rh,  1.767794191464423978, 1e-12, "eraFk5hz", "ra", status);
   vvd(dh, -0.2917516001679884419, 1e-12, "eraFk5hz", "dec", status);

}

static void t_fw2m(int *status)
/*
**  - - - - - - -
**   t _ f w 2 m
**  - - - - - - -
**
**  Test eraFw2m function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraFw2m, vvd
**
**  This revision:  2013 August 7
*/
{
   double gamb, phib, psi, eps, r[3][3];


   gamb = -0.2243387670997992368e-5;
   phib =  0.4091014602391312982;
   psi  = -0.9501954178013015092e-3;
   eps  =  0.4091014316587367472;

   eraFw2m(gamb, phib, psi, eps, r);

   vvd(r[0][0], 0.9999995505176007047, 1e-12,
       "eraFw2m", "11", status);
   vvd(r[0][1], 0.8695404617348192957e-3, 1e-12,
       "eraFw2m", "12", status);
   vvd(r[0][2], 0.3779735201865582571e-3, 1e-12,
       "eraFw2m", "13", status);

   vvd(r[1][0], -0.8695404723772016038e-3, 1e-12,
       "eraFw2m", "21", status);
   vvd(r[1][1], 0.9999996219496027161, 1e-12,
       "eraFw2m", "22", status);
   vvd(r[1][2], -0.1361752496887100026e-6, 1e-12,
       "eraFw2m", "23", status);

   vvd(r[2][0], -0.3779734957034082790e-3, 1e-12,
       "eraFw2m", "31", status);
   vvd(r[2][1], -0.1924880848087615651e-6, 1e-12,
       "eraFw2m", "32", status);
   vvd(r[2][2], 0.9999999285679971958, 1e-12,
       "eraFw2m", "33", status);

}

static void t_fw2xy(int *status)
/*
**  - - - - - - - -
**   t _ f w 2 x y
**  - - - - - - - -
**
**  Test eraFw2xy function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraFw2xy, vvd
**
**  This revision:  2013 August 7
*/
{
   double gamb, phib, psi, eps, x, y;


   gamb = -0.2243387670997992368e-5;
   phib =  0.4091014602391312982;
   psi  = -0.9501954178013015092e-3;
   eps  =  0.4091014316587367472;

   eraFw2xy(gamb, phib, psi, eps, &x, &y);

   vvd(x, -0.3779734957034082790e-3, 1e-14, "eraFw2xy", "x", status);
   vvd(y, -0.1924880848087615651e-6, 1e-14, "eraFw2xy", "y", status);

}

static void t_g2icrs(int *status)
/*
**  - - - - - - - - -
**   t _ g 2 i c r s
**  - - - - - - - - -
**
**  Test eraG2icrs function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraG2icrs, vvd
**
**  This revision:  2015 January 30
*/
{
   double dl, db, dr, dd;


   dl =  5.5850536063818546461558105;
   db = -0.7853981633974483096156608;
   eraG2icrs (dl, db, &dr, &dd);
   vvd(dr,  5.9338074302227188048671, 1e-14, "eraG2icrs", "R", status);
   vvd(dd, -1.1784870613579944551541, 1e-14, "eraG2icrs", "D", status);
 }

static void t_gc2gd(int *status)
/*
**  - - - - - - - -
**   t _ g c 2 g d
**  - - - - - - - -
**
**  Test eraGc2gd function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraGc2gd, viv, vvd
**
**  This revision:  2016 March 12
*/
{
   int j;
   double xyz[] = {2e6, 3e6, 5.244e6};
   double e, p, h;

   j = eraGc2gd(0, xyz, &e, &p, &h);

   viv(j, -1, "eraGc2gd", "j0", status);

   j = eraGc2gd(ERFA_WGS84, xyz, &e, &p, &h);

   viv(j, 0, "eraGc2gd", "j1", status);
   vvd(e, 0.9827937232473290680, 1e-14, "eraGc2gd", "e1", status);
   vvd(p, 0.97160184819075459, 1e-14, "eraGc2gd", "p1", status);
   vvd(h, 331.4172461426059892, 1e-8, "eraGc2gd", "h1", status);

   j = eraGc2gd(ERFA_GRS80, xyz, &e, &p, &h);

   viv(j, 0, "eraGc2gd", "j2", status);
   vvd(e, 0.9827937232473290680, 1e-14, "eraGc2gd", "e2", status);
   vvd(p, 0.97160184820607853, 1e-14, "eraGc2gd", "p2", status);
   vvd(h, 331.41731754844348, 1e-8, "eraGc2gd", "h2", status);

   j = eraGc2gd(ERFA_WGS72, xyz, &e, &p, &h);

   viv(j, 0, "eraGc2gd", "j3", status);
   vvd(e, 0.9827937232473290680, 1e-14, "eraGc2gd", "e3", status);
   vvd(p, 0.9716018181101511937, 1e-14, "eraGc2gd", "p3", status);
   vvd(h, 333.2770726130318123, 1e-8, "eraGc2gd", "h3", status);

   j = eraGc2gd(4, xyz, &e, &p, &h);

   viv(j, -1, "eraGc2gd", "j4", status);
}

static void t_gc2gde(int *status)
/*
**  - - - - - - - - -
**   t _ g c 2 g d e
**  - - - - - - - - -
**
**  Test eraGc2gde function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraGc2gde, viv, vvd
**
**  This revision:  2016 March 12
*/
{
   int j;
   double a = 6378136.0, f = 0.0033528;
   double xyz[] = {2e6, 3e6, 5.244e6};
   double e, p, h;

   j = eraGc2gde(a, f, xyz, &e, &p, &h);

   viv(j, 0, "eraGc2gde", "j", status);
   vvd(e, 0.9827937232473290680, 1e-14, "eraGc2gde", "e", status);
   vvd(p, 0.9716018377570411532, 1e-14, "eraGc2gde", "p", status);
   vvd(h, 332.36862495764397, 1e-8, "eraGc2gde", "h", status);
}

static void t_gd2gc(int *status)
/*
**  - - - - - - - -
**   t _ g d 2 g c
**  - - - - - - - -
**
**  Test eraGd2gc function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraGd2gc, viv, vvd
**
**  This revision:  2016 March 12
*/
{
   int j;
   double e = 3.1, p = -0.5, h = 2500.0;
   double xyz[3];

   j = eraGd2gc(0, e, p, h, xyz);

   viv(j, -1, "eraGd2gc", "j0", status);

   j = eraGd2gc(ERFA_WGS84, e, p, h, xyz);

   viv(j, 0, "eraGd2gc", "j1", status);
   vvd(xyz[0], -5599000.5577049947, 1e-7, "eraGd2gc", "1/1", status);
   vvd(xyz[1], 233011.67223479203, 1e-7, "eraGd2gc", "2/1", status);
   vvd(xyz[2], -3040909.4706983363, 1e-7, "eraGd2gc", "3/1", status);

   j = eraGd2gc(ERFA_GRS80, e, p, h, xyz);

   viv(j, 0, "eraGd2gc", "j2", status);
   vvd(xyz[0], -5599000.5577260984, 1e-7, "eraGd2gc", "1/2", status);
   vvd(xyz[1], 233011.6722356702949, 1e-7, "eraGd2gc", "2/2", status);
   vvd(xyz[2], -3040909.4706095476, 1e-7, "eraGd2gc", "3/2", status);

   j = eraGd2gc(ERFA_WGS72, e, p, h, xyz);

   viv(j, 0, "eraGd2gc", "j3", status);
   vvd(xyz[0], -5598998.7626301490, 1e-7, "eraGd2gc", "1/3", status);
   vvd(xyz[1], 233011.5975297822211, 1e-7, "eraGd2gc", "2/3", status);
   vvd(xyz[2], -3040908.6861467111, 1e-7, "eraGd2gc", "3/3", status);

   j = eraGd2gc(4, e, p, h, xyz);

   viv(j, -1, "eraGd2gc", "j4", status);
}

static void t_gd2gce(int *status)
/*
**  - - - - - - - - -
**   t _ g d 2 g c e
**  - - - - - - - - -
**
**  Test eraGd2gce function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraGd2gce, viv, vvd
**
**  This revision:  2016 March 12
*/
{
   int j;
   double a = 6378136.0, f = 0.0033528;
   double e = 3.1, p = -0.5, h = 2500.0;
   double xyz[3];

   j = eraGd2gce(a, f, e, p, h, xyz);

   viv(j, 0, "eraGd2gce", "j", status);
   vvd(xyz[0], -5598999.6665116328, 1e-7, "eraGd2gce", "1", status);
   vvd(xyz[1], 233011.6351463057189, 1e-7, "eraGd2gce", "2", status);
   vvd(xyz[2], -3040909.0517314132, 1e-7, "eraGd2gce", "3", status);
}

static void t_gmst00(int *status)
/*
**  - - - - - - - - -
**   t _ g m s t 0 0
**  - - - - - - - - -
**
**  Test eraGmst00 function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraGmst00, vvd
**
**  This revision:  2013 August 7
*/
{
   double theta;


   theta = eraGmst00(2400000.5, 53736.0, 2400000.5, 53736.0);

   vvd(theta, 1.754174972210740592, 1e-12, "eraGmst00", "", status);

}

static void t_gmst06(int *status)
/*
**  - - - - - - - - -
**   t _ g m s t 0 6
**  - - - - - - - - -
**
**  Test eraGmst06 function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraGmst06, vvd
**
**  This revision:  2013 August 7
*/
{
   double theta;


   theta = eraGmst06(2400000.5, 53736.0, 2400000.5, 53736.0);

   vvd(theta, 1.754174971870091203, 1e-12, "eraGmst06", "", status);

}

static void t_gmst82(int *status)
/*
**  - - - - - - - - -
**   t _ g m s t 8 2
**  - - - - - - - - -
**
**  Test eraGmst82 function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraGmst82, vvd
**
**  This revision:  2013 August 7
*/
{
   double theta;


   theta = eraGmst82(2400000.5, 53736.0);

   vvd(theta, 1.754174981860675096, 1e-12, "eraGmst82", "", status);

}

static void t_gst00a(int *status)
/*
**  - - - - - - - - -
**   t _ g s t 0 0 a
**  - - - - - - - - -
**
**  Test eraGst00a function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraGst00a, vvd
**
**  This revision:  2013 August 7
*/
{
   double theta;


   theta = eraGst00a(2400000.5, 53736.0, 2400000.5, 53736.0);

   vvd(theta, 1.754166138018281369, 1e-12, "eraGst00a", "", status);

}

static void t_gst00b(int *status)
/*
**  - - - - - - - - -
**   t _ g s t 0 0 b
**  - - - - - - - - -
**
**  Test eraGst00b function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraGst00b, vvd
**
**  This revision:  2013 August 7
*/
{
   double theta;


   theta = eraGst00b(2400000.5, 53736.0);

   vvd(theta, 1.754166136510680589, 1e-12, "eraGst00b", "", status);

}

static void t_gst06(int *status)
/*
**  - - - - - - - -
**   t _ g s t 0 6
**  - - - - - - - -
**
**  Test eraGst06 function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraGst06, vvd
**
**  This revision:  2013 August 7
*/
{
   double rnpb[3][3], theta;


   rnpb[0][0] =  0.9999989440476103608;
   rnpb[0][1] = -0.1332881761240011518e-2;
   rnpb[0][2] = -0.5790767434730085097e-3;

   rnpb[1][0] =  0.1332858254308954453e-2;
   rnpb[1][1] =  0.9999991109044505944;
   rnpb[1][2] = -0.4097782710401555759e-4;

   rnpb[2][0] =  0.5791308472168153320e-3;
   rnpb[2][1] =  0.4020595661593994396e-4;
   rnpb[2][2] =  0.9999998314954572365;

   theta = eraGst06(2400000.5, 53736.0, 2400000.5, 53736.0, rnpb);

   vvd(theta, 1.754166138018167568, 1e-12, "eraGst06", "", status);

}

static void t_gst06a(int *status)
/*
**  - - - - - - - - -
**   t _ g s t 0 6 a
**  - - - - - - - - -
**
**  Test eraGst06a function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraGst06a, vvd
**
**  This revision:  2013 August 7
*/
{
   double theta;


   theta = eraGst06a(2400000.5, 53736.0, 2400000.5, 53736.0);

   vvd(theta, 1.754166137675019159, 1e-12, "eraGst06a", "", status);

}

static void t_gst94(int *status)
/*
**  - - - - - - - -
**   t _ g s t 9 4
**  - - - - - - - -
**
**  Test eraGst94 function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraGst94, vvd
**
**  This revision:  2013 August 7
*/
{
   double theta;


   theta = eraGst94(2400000.5, 53736.0);

   vvd(theta, 1.754166136020645203, 1e-12, "eraGst94", "", status);

}

static void t_icrs2g(int *status)
/*
**  - - - - - - - - -
**   t _ i c r s 2 g
**  - - - - - - - - -
**
**  Test eraIcrs2g function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraIcrs2g, vvd
**
**  This revision:  2015 January 30
*/
{
   double dr, dd, dl, db;

   dr =  5.9338074302227188048671087;
   dd = -1.1784870613579944551540570;
   eraIcrs2g (dr, dd, &dl, &db);
   vvd(dl,  5.5850536063818546461558, 1e-14, "eraIcrs2g", "L", status);
   vvd(db, -0.7853981633974483096157, 1e-14, "eraIcrs2g", "B", status);
 }

static void t_h2fk5(int *status)
/*
**  - - - - - - - -
**   t _ h 2 f k 5
**  - - - - - - - -
**
**  Test eraH2fk5 function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraH2fk5, vvd
**
**  This revision:  2017 January 3
*/
{
   double rh, dh, drh, ddh, pxh, rvh, r5, d5, dr5, dd5, px5, rv5;


   rh  =  1.767794352;
   dh  = -0.2917512594;
   drh = -2.76413026e-6;
   ddh = -5.92994449e-6;
   pxh =  0.379210;
   rvh = -7.6;

   eraH2fk5(rh, dh, drh, ddh, pxh, rvh,
            &r5, &d5, &dr5, &dd5, &px5, &rv5);

   vvd(r5, 1.767794455700065506, 1e-13,
       "eraH2fk5", "ra", status);
   vvd(d5, -0.2917513626469638890, 1e-13,
       "eraH2fk5", "dec", status);
   vvd(dr5, -0.27597945024511204e-5, 1e-18,
       "eraH2fk5", "dr5", status);
   vvd(dd5, -0.59308014093262838e-5, 1e-18,
       "eraH2fk5", "dd5", status);
   vvd(px5, 0.37921, 1e-13,
       "eraH2fk5", "px", status);
   vvd(rv5, -7.6000001309071126, 1e-11,
       "eraH2fk5", "rv", status);

}

static void t_hd2ae(int *status)
/*
**  - - - - - - - -
**   t _ h d 2 a e
**  - - - - - - - -
**
**  Test eraHd2ae function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraHd2ae and vvd
**
**  This revision:  2017 October 21
*/
{
   double h, d, p, a, e;


   h = 1.1;
   d = 1.2;
   p = 0.3;

   eraHd2ae(h, d, p, &a, &e);

   vvd(a, 5.916889243730066194, 1e-13, "eraHd2ae", "a", status);
   vvd(e, 0.4472186304990486228, 1e-14, "eraHd2ae", "e", status);

}

static void t_hd2pa(int *status)
/*
**  - - - - - - - -
**   t _ h d 2 p a
**  - - - - - - - -
**
**  Test eraHd2pa function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraHd2pa and vvd
**
**  This revision:  2017 October 21
*/
{
   double h, d, p, q;


   h = 1.1;
   d = 1.2;
   p = 0.3;

   q = eraHd2pa(h, d, p);

   vvd(q, 1.906227428001995580, 1e-13, "eraHd2pa", "q", status);

}

static void t_hfk5z(int *status)
/*
**  - - - - - - - -
**   t _ h f k 5 z
**  - - - - - - - -
**
**  Test eraHfk5z function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraHfk5z, vvd
**
**  This revision:  2013 August 7
*/
{
   double rh, dh, r5, d5, dr5, dd5;



   rh =  1.767794352;
   dh = -0.2917512594;

   eraHfk5z(rh, dh, 2400000.5, 54479.0, &r5, &d5, &dr5, &dd5);

   vvd(r5, 1.767794490535581026, 1e-13,
       "eraHfk5z", "ra", status);
   vvd(d5, -0.2917513695320114258, 1e-14,
       "eraHfk5z", "dec", status);
   vvd(dr5, 0.4335890983539243029e-8, 1e-22,
       "eraHfk5z", "dr5", status);
   vvd(dd5, -0.8569648841237745902e-9, 1e-23,
       "eraHfk5z", "dd5", status);

}

static void t_ir(int *status)
/*
**  - - - - -
**   t _ i r
**  - - - - -
**
**  Test eraIr function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraIr, vvd
**
**  This revision:  2013 August 7
*/
{
   double r[3][3];


   r[0][0] = 2.0;
   r[0][1] = 3.0;
   r[0][2] = 2.0;

   r[1][0] = 3.0;
   r[1][1] = 2.0;
   r[1][2] = 3.0;

   r[2][0] = 3.0;
   r[2][1] = 4.0;
   r[2][2] = 5.0;

   eraIr(r);

   vvd(r[0][0], 1.0, 0.0, "eraIr", "11", status);
   vvd(r[0][1], 0.0, 0.0, "eraIr", "12", status);
   vvd(r[0][2], 0.0, 0.0, "eraIr", "13", status);

   vvd(r[1][0], 0.0, 0.0, "eraIr", "21", status);
   vvd(r[1][1], 1.0, 0.0, "eraIr", "22", status);
   vvd(r[1][2], 0.0, 0.0, "eraIr", "23", status);

   vvd(r[2][0], 0.0, 0.0, "eraIr", "31", status);
   vvd(r[2][1], 0.0, 0.0, "eraIr", "32", status);
   vvd(r[2][2], 1.0, 0.0, "eraIr", "33", status);

}

static void t_jd2cal(int *status)
/*
**  - - - - - - - - -
**   t _ j d 2 c a l
**  - - - - - - - - -
**
**  Test eraJd2cal function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraJd2cal, viv, vvd
**
**  This revision:  2013 August 7
*/
{
   double dj1, dj2, fd;
   int iy, im, id, j;


   dj1 = 2400000.5;
   dj2 = 50123.9999;

   j = eraJd2cal(dj1, dj2, &iy, &im, &id, &fd);

   viv(iy, 1996, "eraJd2cal", "y", status);
   viv(im, 2, "eraJd2cal", "m", status);
   viv(id, 10, "eraJd2cal", "d", status);
   vvd(fd, 0.9999, 1e-7, "eraJd2cal", "fd", status);
   viv(j, 0, "eraJd2cal", "j", status);

}

static void t_jdcalf(int *status)
/*
**  - - - - - - - - -
**   t _ j d c a l f
**  - - - - - - - - -
**
**  Test eraJdcalf function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraJdcalf, viv
**
**  This revision:  2013 August 7
*/
{
   double dj1, dj2;
   int iydmf[4], j;


   dj1 = 2400000.5;
   dj2 = 50123.9999;

   j = eraJdcalf(4, dj1, dj2, iydmf);

   viv(iydmf[0], 1996, "eraJdcalf", "y", status);
   viv(iydmf[1], 2, "eraJdcalf", "m", status);
   viv(iydmf[2], 10, "eraJdcalf", "d", status);
   viv(iydmf[3], 9999, "eraJdcalf", "f", status);

   viv(j, 0, "eraJdcalf", "j", status);

}

static void t_ld(int *status)
/*
**  - - - - -
**   t _ l d
**  - - - - -
**
**  Test eraLd function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraLd, vvd
*
**  This revision:  2013 October 2
*/
{
   double bm, p[3], q[3], e[3], em, dlim, p1[3];


   bm = 0.00028574;
   p[0] = -0.763276255;
   p[1] = -0.608633767;
   p[2] = -0.216735543;
   q[0] = -0.763276255;
   q[1] = -0.608633767;
   q[2] = -0.216735543;
   e[0] = 0.76700421;
   e[1] = 0.605629598;
   e[2] = 0.211937094;
   em = 8.91276983;
   dlim = 3e-10;

   eraLd(bm, p, q, e, em, dlim, p1);

   vvd(p1[0], -0.7632762548968159627, 1e-12,
               "eraLd", "1", status);
   vvd(p1[1], -0.6086337670823762701, 1e-12,
               "eraLd", "2", status);
   vvd(p1[2], -0.2167355431320546947, 1e-12,
               "eraLd", "3", status);

}

static void t_ldn(int *status)
/*
**  - - - - - -
**   t _ l d n
**  - - - - - -
**
**  Test eraLdn function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraLdn, vvd
**
**  This revision:  2013 October 2
*/
{
   int n;
   eraLDBODY b[3];
   double ob[3], sc[3], sn[3];


   n = 3;
   b[0].bm = 0.00028574;
   b[0].dl = 3e-10;
   b[0].pv[0][0] = -7.81014427;
   b[0].pv[0][1] = -5.60956681;
   b[0].pv[0][2] = -1.98079819;
   b[0].pv[1][0] =  0.0030723249;
   b[0].pv[1][1] = -0.00406995477;
   b[0].pv[1][2] = -0.00181335842;
   b[1].bm = 0.00095435;
   b[1].dl = 3e-9;
   b[1].pv[0][0] =  0.738098796;
   b[1].pv[0][1] =  4.63658692;
   b[1].pv[0][2] =  1.9693136;
   b[1].pv[1][0] = -0.00755816922;
   b[1].pv[1][1] =  0.00126913722;
   b[1].pv[1][2] =  0.000727999001;
   b[2].bm = 1.0;
   b[2].dl = 6e-6;
   b[2].pv[0][0] = -0.000712174377;
   b[2].pv[0][1] = -0.00230478303;
   b[2].pv[0][2] = -0.00105865966;
   b[2].pv[1][0] =  6.29235213e-6;
   b[2].pv[1][1] = -3.30888387e-7;
   b[2].pv[1][2] = -2.96486623e-7;
   ob[0] =  -0.974170437;
   ob[1] =  -0.2115201;
   ob[2] =  -0.0917583114;
   sc[0] =  -0.763276255;
   sc[1] =  -0.608633767;
   sc[2] =  -0.216735543;

   eraLdn(n, b, ob, sc, sn);

   vvd(sn[0], -0.7632762579693333866, 1e-12,
               "eraLdn", "1", status);
   vvd(sn[1], -0.6086337636093002660, 1e-12,
               "eraLdn", "2", status);
   vvd(sn[2], -0.2167355420646328159, 1e-12,
               "eraLdn", "3", status);

}

static void t_ldsun(int *status)
/*
**  - - - - - - - -
**   t _ l d s u n
**  - - - - - - - -
**
**  Test eraLdsun function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraLdsun, vvd
**
**  This revision:  2013 October 2
*/
{
   double p[3], e[3], em, p1[3];


   p[0] = -0.763276255;
   p[1] = -0.608633767;
   p[2] = -0.216735543;
   e[0] = -0.973644023;
   e[1] = -0.20925523;
   e[2] = -0.0907169552;
   em = 0.999809214;

   eraLdsun(p, e, em, p1);

   vvd(p1[0], -0.7632762580731413169, 1e-12,
               "eraLdsun", "1", status);
   vvd(p1[1], -0.6086337635262647900, 1e-12,
               "eraLdsun", "2", status);
   vvd(p1[2], -0.2167355419322321302, 1e-12,
               "eraLdsun", "3", status);

}

static void t_lteceq(int *status)
/*
**  - - - - - - - - -
**   t _ l t e c e q
**  - - - - - - - - -
**
**  Test eraLteceq function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraLteceq, vvd
**
**  This revision:  2016 March 12
*/
{
   double epj, dl, db, dr, dd;


   epj = 2500.0;
   dl = 1.5;
   db = 0.6;

   eraLteceq(epj, dl, db, &dr, &dd);

   vvd(dr, 1.275156021861921167, 1e-14, "eraLteceq", "dr", status);
   vvd(dd, 0.9966573543519204791, 1e-14, "eraLteceq", "dd", status);

}

static void t_ltecm(int *status)
/*
**  - - - - - - - -
**   t _ l t e c m
**  - - - - - - - -
**
**  Test eraLtecm function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraLtecm, vvd
**
**  This revision:  2016 March 12
*/
{
   double epj, rm[3][3];


   epj = -3000.0;

   eraLtecm(epj, rm);

   vvd(rm[0][0], 0.3564105644859788825, 1e-14,
       "eraLtecm", "rm11", status);
   vvd(rm[0][1], 0.8530575738617682284, 1e-14,
       "eraLtecm", "rm12", status);
   vvd(rm[0][2], 0.3811355207795060435, 1e-14,
       "eraLtecm", "rm13", status);
   vvd(rm[1][0], -0.9343283469640709942, 1e-14,
       "eraLtecm", "rm21", status);
   vvd(rm[1][1], 0.3247830597681745976, 1e-14,
       "eraLtecm", "rm22", status);
   vvd(rm[1][2], 0.1467872751535940865, 1e-14,
       "eraLtecm", "rm23", status);
   vvd(rm[2][0], 0.1431636191201167793e-2, 1e-14,
       "eraLtecm", "rm31", status);
   vvd(rm[2][1], -0.4084222566960599342, 1e-14,
       "eraLtecm", "rm32", status);
   vvd(rm[2][2], 0.9127919865189030899, 1e-14,
       "eraLtecm", "rm33", status);

}

static void t_lteqec(int *status)
/*
**  - - - - - - - - -
**   t _ l t e q e c
**  - - - - - - - - -
**
**  Test eraLteqec function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraLteqec, vvd
**
**  This revision:  2016 March 12
*/
{
   double epj, dr, dd, dl, db;


   epj = -1500.0;
   dr = 1.234;
   dd = 0.987;

   eraLteqec(epj, dr, dd, &dl, &db);

   vvd(dl, 0.5039483649047114859, 1e-14, "eraLteqec", "dl", status);
   vvd(db, 0.5848534459726224882, 1e-14, "eraLteqec", "db", status);

}

static void t_ltp(int *status)
/*
**  - - - - - -
**   t _ l t p
**  - - - - - -
**
**  Test eraLtp function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraLtp, vvd
**
**  This revision:  2016 March 12
*/
{
   double epj, rp[3][3];


   epj = 1666.666;

   eraLtp(epj, rp);

   vvd(rp[0][0], 0.9967044141159213819, 1e-14,
       "eraLtp", "rp11", status);
   vvd(rp[0][1], 0.7437801893193210840e-1, 1e-14,
       "eraLtp", "rp12", status);
   vvd(rp[0][2], 0.3237624409345603401e-1, 1e-14,
       "eraLtp", "rp13", status);
   vvd(rp[1][0], -0.7437802731819618167e-1, 1e-14,
       "eraLtp", "rp21", status);
   vvd(rp[1][1], 0.9972293894454533070, 1e-14,
       "eraLtp", "rp22", status);
   vvd(rp[1][2], -0.1205768842723593346e-2, 1e-14,
       "eraLtp", "rp23", status);
   vvd(rp[2][0], -0.3237622482766575399e-1, 1e-14,
       "eraLtp", "rp31", status);
   vvd(rp[2][1], -0.1206286039697609008e-2, 1e-14,
       "eraLtp", "rp32", status);
   vvd(rp[2][2], 0.9994750246704010914, 1e-14,
       "eraLtp", "rp33", status);

}

static void t_ltpb(int *status)
/*
**  - - - - - - -
**   t _ l t p b
**  - - - - - - -
**
**  Test eraLtpb function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraLtpb, vvd
**
**  This revision:  2016 March 12
*/
{
   double epj, rpb[3][3];


   epj = 1666.666;

   eraLtpb(epj, rpb);

   vvd(rpb[0][0], 0.9967044167723271851, 1e-14,
       "eraLtpb", "rpb11", status);
   vvd(rpb[0][1], 0.7437794731203340345e-1, 1e-14,
       "eraLtpb", "rpb12", status);
   vvd(rpb[0][2], 0.3237632684841625547e-1, 1e-14,
       "eraLtpb", "rpb13", status);
   vvd(rpb[1][0], -0.7437795663437177152e-1, 1e-14,
       "eraLtpb", "rpb21", status);
   vvd(rpb[1][1], 0.9972293947500013666, 1e-14,
       "eraLtpb", "rpb22", status);
   vvd(rpb[1][2], -0.1205741865911243235e-2, 1e-14,
       "eraLtpb", "rpb23", status);
   vvd(rpb[2][0], -0.3237630543224664992e-1, 1e-14,
       "eraLtpb", "rpb31", status);
   vvd(rpb[2][1], -0.1206316791076485295e-2, 1e-14,
       "eraLtpb", "rpb32", status);
   vvd(rpb[2][2], 0.9994750220222438819, 1e-14,
       "eraLtpb", "rpb33", status);

}

static void t_ltpecl(int *status)
/*
**  - - - - - - - - -
**   t _ l t p e c l
**  - - - - - - - - -
**
**  Test eraLtpecl function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraLtpecl, vvd
**
**  This revision:  2016 March 12
*/
{
   double epj, vec[3];


   epj = -1500.0;

   eraLtpecl(epj, vec);

   vvd(vec[0], 0.4768625676477096525e-3, 1e-14,
       "eraLtpecl", "vec1", status);
   vvd(vec[1], -0.4052259533091875112, 1e-14,
       "eraLtpecl", "vec2", status);
   vvd(vec[2], 0.9142164401096448012, 1e-14,
       "eraLtpecl", "vec3", status);

}

static void t_ltpequ(int *status)
/*
**  - - - - - - - - -
**   t _ l t p e q u
**  - - - - - - - - -
**
**  Test eraLtpequ function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraLtpequ, vvd
**
**  This revision:  2016 March 12
*/
{
   double epj, veq[3];


   epj = -2500.0;

   eraLtpequ(epj, veq);

   vvd(veq[0], -0.3586652560237326659, 1e-14,
       "eraLtpequ", "veq1", status);
   vvd(veq[1], -0.1996978910771128475, 1e-14,
       "eraLtpequ", "veq2", status);
   vvd(veq[2], 0.9118552442250819624, 1e-14,
       "eraLtpequ", "veq3", status);

}

static void t_moon98(int *status)
/*
**  - - - - - - - - -
**   t _ m o o n 9 8
**  - - - - - - - - -
**
**  Test eraMoon98 function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraMoon98, vvd, viv
**
**  This revision:  2021 April 12
*/
{
   double pv[2][3];


   eraMoon98(2400000.5, 43999.9, pv);

   vvd(pv[0][0], -0.2601295959971044180e-2, 1e-11,
       "eraMoon98", "x 4", status);
   vvd(pv[0][1], 0.6139750944302742189e-3, 1e-11,
       "eraMoon98", "y 4", status);
   vvd(pv[0][2], 0.2640794528229828909e-3, 1e-11,
       "eraMoon98", "z 4", status);

   vvd(pv[1][0], -0.1244321506649895021e-3, 1e-11,
       "eraMoon98", "xd 4", status);
   vvd(pv[1][1], -0.5219076942678119398e-3, 1e-11,
       "eraMoon98", "yd 4", status);
   vvd(pv[1][2], -0.1716132214378462047e-3, 1e-11,
       "eraMoon98", "zd 4", status);

}

static void t_num00a(int *status)
/*
**  - - - - - - - - -
**   t _ n u m 0 0 a
**  - - - - - - - - -
**
**  Test eraNum00a function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraNum00a, vvd
**
**  This revision:  2013 August 7
*/
{
   double rmatn[3][3];


   eraNum00a(2400000.5, 53736.0, rmatn);

   vvd(rmatn[0][0], 0.9999999999536227949, 1e-12,
       "eraNum00a", "11", status);
   vvd(rmatn[0][1], 0.8836238544090873336e-5, 1e-12,
       "eraNum00a", "12", status);
   vvd(rmatn[0][2], 0.3830835237722400669e-5, 1e-12,
       "eraNum00a", "13", status);

   vvd(rmatn[1][0], -0.8836082880798569274e-5, 1e-12,
       "eraNum00a", "21", status);
   vvd(rmatn[1][1], 0.9999999991354655028, 1e-12,
       "eraNum00a", "22", status);
   vvd(rmatn[1][2], -0.4063240865362499850e-4, 1e-12,
       "eraNum00a", "23", status);

   vvd(rmatn[2][0], -0.3831194272065995866e-5, 1e-12,
       "eraNum00a", "31", status);
   vvd(rmatn[2][1], 0.4063237480216291775e-4, 1e-12,
       "eraNum00a", "32", status);
   vvd(rmatn[2][2], 0.9999999991671660338, 1e-12,
       "eraNum00a", "33", status);

}

static void t_num00b(int *status)
/*
**  - - - - - - - - -
**   t _ n u m 0 0 b
**  - - - - - - - - -
**
**  Test eraNum00b function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraNum00b, vvd
**
**  This revision:  2013 August 7
*/
{
    double rmatn[3][3];

    eraNum00b(2400000.5, 53736, rmatn);

   vvd(rmatn[0][0], 0.9999999999536069682, 1e-12,
       "eraNum00b", "11", status);
   vvd(rmatn[0][1], 0.8837746144871248011e-5, 1e-12,
       "eraNum00b", "12", status);
   vvd(rmatn[0][2], 0.3831488838252202945e-5, 1e-12,
       "eraNum00b", "13", status);

   vvd(rmatn[1][0], -0.8837590456632304720e-5, 1e-12,
       "eraNum00b", "21", status);
   vvd(rmatn[1][1], 0.9999999991354692733, 1e-12,
       "eraNum00b", "22", status);
   vvd(rmatn[1][2], -0.4063198798559591654e-4, 1e-12,
       "eraNum00b", "23", status);

   vvd(rmatn[2][0], -0.3831847930134941271e-5, 1e-12,
       "eraNum00b", "31", status);
   vvd(rmatn[2][1], 0.4063195412258168380e-4, 1e-12,
       "eraNum00b", "32", status);
   vvd(rmatn[2][2], 0.9999999991671806225, 1e-12,
       "eraNum00b", "33", status);

}

static void t_num06a(int *status)
/*
**  - - - - - - - - -
**   t _ n u m 0 6 a
**  - - - - - - - - -
**
**  Test eraNum06a function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraNum06a, vvd
**
**  This revision:  2013 August 7
*/
{
    double rmatn[3][3];

    eraNum06a(2400000.5, 53736, rmatn);

   vvd(rmatn[0][0], 0.9999999999536227668, 1e-12,
       "eraNum06a", "11", status);
   vvd(rmatn[0][1], 0.8836241998111535233e-5, 1e-12,
       "eraNum06a", "12", status);
   vvd(rmatn[0][2], 0.3830834608415287707e-5, 1e-12,
       "eraNum06a", "13", status);

   vvd(rmatn[1][0], -0.8836086334870740138e-5, 1e-12,
       "eraNum06a", "21", status);
   vvd(rmatn[1][1], 0.9999999991354657474, 1e-12,
       "eraNum06a", "22", status);
   vvd(rmatn[1][2], -0.4063240188248455065e-4, 1e-12,
       "eraNum06a", "23", status);

   vvd(rmatn[2][0], -0.3831193642839398128e-5, 1e-12,
       "eraNum06a", "31", status);
   vvd(rmatn[2][1], 0.4063236803101479770e-4, 1e-12,
       "eraNum06a", "32", status);
   vvd(rmatn[2][2], 0.9999999991671663114, 1e-12,
       "eraNum06a", "33", status);

}

static void t_numat(int *status)
/*
**  - - - - - - - -
**   t _ n u m a t
**  - - - - - - - -
**
**  Test eraNumat function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraNumat, vvd
**
**  This revision:  2013 August 7
*/
{
   double epsa, dpsi, deps, rmatn[3][3];


   epsa =  0.4090789763356509900;
   dpsi = -0.9630909107115582393e-5;
   deps =  0.4063239174001678826e-4;

   eraNumat(epsa, dpsi, deps, rmatn);

   vvd(rmatn[0][0], 0.9999999999536227949, 1e-12,
       "eraNumat", "11", status);
   vvd(rmatn[0][1], 0.8836239320236250577e-5, 1e-12,
       "eraNumat", "12", status);
   vvd(rmatn[0][2], 0.3830833447458251908e-5, 1e-12,
       "eraNumat", "13", status);

   vvd(rmatn[1][0], -0.8836083657016688588e-5, 1e-12,
       "eraNumat", "21", status);
   vvd(rmatn[1][1], 0.9999999991354654959, 1e-12,
       "eraNumat", "22", status);
   vvd(rmatn[1][2], -0.4063240865361857698e-4, 1e-12,
       "eraNumat", "23", status);

   vvd(rmatn[2][0], -0.3831192481833385226e-5, 1e-12,
       "eraNumat", "31", status);
   vvd(rmatn[2][1], 0.4063237480216934159e-4, 1e-12,
       "eraNumat", "32", status);
   vvd(rmatn[2][2], 0.9999999991671660407, 1e-12,
       "eraNumat", "33", status);

}

static void t_nut00a(int *status)
/*
**  - - - - - - - - -
**   t _ n u t 0 0 a
**  - - - - - - - - -
**
**  Test eraNut00a function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraNut00a, vvd
**
**  This revision:  2013 August 7
*/
{
   double dpsi, deps;


   eraNut00a(2400000.5, 53736.0, &dpsi, &deps);

   vvd(dpsi, -0.9630909107115518431e-5, 1e-13,
       "eraNut00a", "dpsi", status);
   vvd(deps,  0.4063239174001678710e-4, 1e-13,
       "eraNut00a", "deps", status);

}

static void t_nut00b(int *status)
/*
**  - - - - - - - - -
**   t _ n u t 0 0 b
**  - - - - - - - - -
**
**  Test eraNut00b function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraNut00b, vvd
**
**  This revision:  2013 August 7
*/
{
   double dpsi, deps;


   eraNut00b(2400000.5, 53736.0, &dpsi, &deps);

   vvd(dpsi, -0.9632552291148362783e-5, 1e-13,
       "eraNut00b", "dpsi", status);
   vvd(deps,  0.4063197106621159367e-4, 1e-13,
       "eraNut00b", "deps", status);

}

static void t_nut06a(int *status)
/*
**  - - - - - - - - -
**   t _ n u t 0 6 a
**  - - - - - - - - -
**
**  Test eraNut06a function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraNut06a, vvd
**
**  This revision:  2013 August 7
*/
{
   double dpsi, deps;


   eraNut06a(2400000.5, 53736.0, &dpsi, &deps);

   vvd(dpsi, -0.9630912025820308797e-5, 1e-13,
       "eraNut06a", "dpsi", status);
   vvd(deps,  0.4063238496887249798e-4, 1e-13,
       "eraNut06a", "deps", status);

}

static void t_nut80(int *status)
/*
**  - - - - - - - -
**   t _ n u t 8 0
**  - - - - - - - -
**
**  Test eraNut80 function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraNut80, vvd
**
**  This revision:  2013 August 7
*/
{
   double dpsi, deps;


   eraNut80(2400000.5, 53736.0, &dpsi, &deps);

   vvd(dpsi, -0.9643658353226563966e-5, 1e-13,
       "eraNut80", "dpsi", status);
   vvd(deps,  0.4060051006879713322e-4, 1e-13,
       "eraNut80", "deps", status);

}

static void t_nutm80(int *status)
/*
**  - - - - - - - - -
**   t _ n u t m 8 0
**  - - - - - - - - -
**
**  Test eraNutm80 function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraNutm80, vvd
**
**  This revision:  2013 August 7
*/
{
   double rmatn[3][3];


   eraNutm80(2400000.5, 53736.0, rmatn);

   vvd(rmatn[0][0], 0.9999999999534999268, 1e-12,
      "eraNutm80", "11", status);
   vvd(rmatn[0][1], 0.8847935789636432161e-5, 1e-12,
      "eraNutm80", "12", status);
   vvd(rmatn[0][2], 0.3835906502164019142e-5, 1e-12,
      "eraNutm80", "13", status);

   vvd(rmatn[1][0], -0.8847780042583435924e-5, 1e-12,
      "eraNutm80", "21", status);
   vvd(rmatn[1][1], 0.9999999991366569963, 1e-12,
      "eraNutm80", "22", status);
   vvd(rmatn[1][2], -0.4060052702727130809e-4, 1e-12,
      "eraNutm80", "23", status);

   vvd(rmatn[2][0], -0.3836265729708478796e-5, 1e-12,
      "eraNutm80", "31", status);
   vvd(rmatn[2][1], 0.4060049308612638555e-4, 1e-12,
      "eraNutm80", "32", status);
   vvd(rmatn[2][2], 0.9999999991684415129, 1e-12,
      "eraNutm80", "33", status);

}

static void t_obl06(int *status)
/*
**  - - - - - - - -
**   t _ o b l 0 6
**  - - - - - - - -
**
**  Test eraObl06 function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraObl06, vvd
**
**  This revision:  2013 August 7
*/
{
   vvd(eraObl06(2400000.5, 54388.0), 0.4090749229387258204, 1e-14,
       "eraObl06", "", status);
}

static void t_obl80(int *status)
/*
**  - - - - - - - -
**   t _ o b l 8 0
**  - - - - - - - -
**
**  Test eraObl80 function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraObl80, vvd
**
**  This revision:  2013 August 7
*/
{
   double eps0;


   eps0 = eraObl80(2400000.5, 54388.0);

   vvd(eps0, 0.4090751347643816218, 1e-14, "eraObl80", "", status);

}

static void t_p06e(int *status)
/*
**  - - - - - - -
**   t _ p 0 6 e
**  - - - - - - -
**
**  Test eraP06e function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraP06e, vvd
**
**  This revision:  2020 May 30
*/
{
    double eps0, psia, oma, bpa, bqa, pia, bpia,
           epsa, chia, za, zetaa, thetaa, pa, gam, phi, psi;


   eraP06e(2400000.5, 52541.0, &eps0, &psia, &oma, &bpa,
           &bqa, &pia, &bpia, &epsa, &chia, &za,
           &zetaa, &thetaa, &pa, &gam, &phi, &psi);

   vvd(eps0, 0.4090926006005828715, 1e-14,
       "eraP06e", "eps0", status);
   vvd(psia, 0.6664369630191613431e-3, 1e-14,
       "eraP06e", "psia", status);
   vvd(oma , 0.4090925973783255982, 1e-14,
       "eraP06e", "oma", status);
   vvd(bpa, 0.5561149371265209445e-6, 1e-14,
       "eraP06e", "bpa", status);
   vvd(bqa, -0.6191517193290621270e-5, 1e-14,
       "eraP06e", "bqa", status);
   vvd(pia, 0.6216441751884382923e-5, 1e-14,
       "eraP06e", "pia", status);
   vvd(bpia, 3.052014180023779882, 1e-14,
       "eraP06e", "bpia", status);
   vvd(epsa, 0.4090864054922431688, 1e-14,
       "eraP06e", "epsa", status);
   vvd(chia, 0.1387703379530915364e-5, 1e-14,
       "eraP06e", "chia", status);
   vvd(za, 0.2921789846651790546e-3, 1e-14,
       "eraP06e", "za", status);
   vvd(zetaa, 0.3178773290332009310e-3, 1e-14,
       "eraP06e", "zetaa", status);
   vvd(thetaa, 0.2650932701657497181e-3, 1e-14,
       "eraP06e", "thetaa", status);
   vvd(pa, 0.6651637681381016288e-3, 1e-14,
       "eraP06e", "pa", status);
   vvd(gam, 0.1398077115963754987e-5, 1e-14,
       "eraP06e", "gam", status);
   vvd(phi, 0.4090864090837462602, 1e-14,
       "eraP06e", "phi", status);
   vvd(psi, 0.6664464807480920325e-3, 1e-14,
       "eraP06e", "psi", status);

}

static void t_p2pv(int *status)
/*
**  - - - - - - -
**   t _ p 2 p v
**  - - - - - - -
**
**  Test eraP2pv function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraP2pv, vvd
**
**  This revision:  2013 August 7
*/
{
   double p[3], pv[2][3];


   p[0] = 0.25;
   p[1] = 1.2;
   p[2] = 3.0;

   pv[0][0] =  0.3;
   pv[0][1] =  1.2;
   pv[0][2] = -2.5;

   pv[1][0] = -0.5;
   pv[1][1] =  3.1;
   pv[1][2] =  0.9;

   eraP2pv(p, pv);

   vvd(pv[0][0], 0.25, 0.0, "eraP2pv", "p1", status);
   vvd(pv[0][1], 1.2,  0.0, "eraP2pv", "p2", status);
   vvd(pv[0][2], 3.0,  0.0, "eraP2pv", "p3", status);

   vvd(pv[1][0], 0.0,  0.0, "eraP2pv", "v1", status);
   vvd(pv[1][1], 0.0,  0.0, "eraP2pv", "v2", status);
   vvd(pv[1][2], 0.0,  0.0, "eraP2pv", "v3", status);

}

static void t_p2s(int *status)
/*
**  - - - - - -
**   t _ p 2 s
**  - - - - - -
**
**  Test eraP2s function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraP2s, vvd
**
**  This revision:  2013 August 7
*/
{
   double p[3], theta, phi, r;


   p[0] = 100.0;
   p[1] = -50.0;
   p[2] =  25.0;

   eraP2s(p, &theta, &phi, &r);

   vvd(theta, -0.4636476090008061162, 1e-12, "eraP2s", "theta", status);
   vvd(phi, 0.2199879773954594463, 1e-12, "eraP2s", "phi", status);
   vvd(r, 114.5643923738960002, 1e-9, "eraP2s", "r", status);

}

static void t_pap(int *status)
/*
**  - - - - - -
**   t _ p a p
**  - - - - - -
**
**  Test eraPap function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraPap, vvd
**
**  This revision:  2013 August 7
*/
{
   double a[3], b[3], theta;


   a[0] =  1.0;
   a[1] =  0.1;
   a[2] =  0.2;

   b[0] = -3.0;
   b[1] = 1e-3;
   b[2] =  0.2;

   theta = eraPap(a, b);

   vvd(theta, 0.3671514267841113674, 1e-12, "eraPap", "", status);

}

static void t_pas(int *status)
/*
**  - - - - - -
**   t _ p a s
**  - - - - - -
**
**  Test eraPas function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraPas, vvd
**
**  This revision:  2013 August 7
*/
{
   double al, ap, bl, bp, theta;


   al =  1.0;
   ap =  0.1;
   bl =  0.2;
   bp = -1.0;

   theta = eraPas(al, ap, bl, bp);

   vvd(theta, -2.724544922932270424, 1e-12, "eraPas", "", status);

}

static void t_pb06(int *status)
/*
**  - - - - - - -
**   t _ p b 0 6
**  - - - - - - -
**
**  Test eraPb06 function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraPb06, vvd
**
**  This revision:  2013 August 7
*/
{
   double bzeta, bz, btheta;


   eraPb06(2400000.5, 50123.9999, &bzeta, &bz, &btheta);

   vvd(bzeta, -0.5092634016326478238e-3, 1e-12,
       "eraPb06", "bzeta", status);
   vvd(bz, -0.3602772060566044413e-3, 1e-12,
       "eraPb06", "bz", status);
   vvd(btheta, -0.3779735537167811177e-3, 1e-12,
       "eraPb06", "btheta", status);

}

static void t_pdp(int *status)
/*
**  - - - - - -
**   t _ p d p
**  - - - - - -
**
**  Test eraPdp function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraPdp, vvd
**
**  This revision:  2013 August 7
*/
{
   double a[3], b[3], adb;


   a[0] = 2.0;
   a[1] = 2.0;
   a[2] = 3.0;

   b[0] = 1.0;
   b[1] = 3.0;
   b[2] = 4.0;

   adb = eraPdp(a, b);

   vvd(adb, 20, 1e-12, "eraPdp", "", status);

}

static void t_pfw06(int *status)
/*
**  - - - - - - - -
**   t _ p f w 0 6
**  - - - - - - - -
**
**  Test eraPfw06 function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraPfw06, vvd
**
**  This revision:  2013 August 7
*/
{
   double gamb, phib, psib, epsa;


   eraPfw06(2400000.5, 50123.9999, &gamb, &phib, &psib, &epsa);

   vvd(gamb, -0.2243387670997995690e-5, 1e-16,
       "eraPfw06", "gamb", status);
   vvd(phib,  0.4091014602391312808, 1e-12,
       "eraPfw06", "phib", status);
   vvd(psib, -0.9501954178013031895e-3, 1e-14,
       "eraPfw06", "psib", status);
   vvd(epsa,  0.4091014316587367491, 1e-12,
       "eraPfw06", "epsa", status);

}

static void t_plan94(int *status)
/*
**  - - - - - - - - -
**   t _ p l a n 9 4
**  - - - - - - - - -
**
**  Test eraPlan94 function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraPlan94, vvd, viv
**
**  This revision:  2013 October 2
*/
{
   double pv[2][3];
   int j;


   j = eraPlan94(2400000.5, 1e6, 0, pv);

   vvd(pv[0][0], 0.0, 0.0, "eraPlan94", "x 1", status);
   vvd(pv[0][1], 0.0, 0.0, "eraPlan94", "y 1", status);
   vvd(pv[0][2], 0.0, 0.0, "eraPlan94", "z 1", status);

   vvd(pv[1][0], 0.0, 0.0, "eraPlan94", "xd 1", status);
   vvd(pv[1][1], 0.0, 0.0, "eraPlan94", "yd 1", status);
   vvd(pv[1][2], 0.0, 0.0, "eraPlan94", "zd 1", status);

   viv(j, -1, "eraPlan94", "j 1", status);

   j = eraPlan94(2400000.5, 1e6, 10, pv);

   viv(j, -1, "eraPlan94", "j 2", status);

   j = eraPlan94(2400000.5, -320000, 3, pv);

   vvd(pv[0][0], 0.9308038666832975759, 1e-11,
       "eraPlan94", "x 3", status);
   vvd(pv[0][1], 0.3258319040261346000, 1e-11,
       "eraPlan94", "y 3", status);
   vvd(pv[0][2], 0.1422794544481140560, 1e-11,
       "eraPlan94", "z 3", status);

   vvd(pv[1][0], -0.6429458958255170006e-2, 1e-11,
       "eraPlan94", "xd 3", status);
   vvd(pv[1][1], 0.1468570657704237764e-1, 1e-11,
       "eraPlan94", "yd 3", status);
   vvd(pv[1][2], 0.6406996426270981189e-2, 1e-11,
       "eraPlan94", "zd 3", status);

   viv(j, 1, "eraPlan94", "j 3", status);

   j = eraPlan94(2400000.5, 43999.9, 1, pv);

   vvd(pv[0][0], 0.2945293959257430832, 1e-11,
       "eraPlan94", "x 4", status);
   vvd(pv[0][1], -0.2452204176601049596, 1e-11,
       "eraPlan94", "y 4", status);
   vvd(pv[0][2], -0.1615427700571978153, 1e-11,
       "eraPlan94", "z 4", status);

   vvd(pv[1][0], 0.1413867871404614441e-1, 1e-11,
       "eraPlan94", "xd 4", status);
   vvd(pv[1][1], 0.1946548301104706582e-1, 1e-11,
       "eraPlan94", "yd 4", status);
   vvd(pv[1][2], 0.8929809783898904786e-2, 1e-11,
       "eraPlan94", "zd 4", status);

   viv(j, 0, "eraPlan94", "j 4", status);

}

static void t_pmat00(int *status)
/*
**  - - - - - - - - -
**   t _ p m a t 0 0
**  - - - - - - - - -
**
**  Test eraPmat00 function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraPmat00, vvd
**
**  This revision:  2013 August 7
*/
{
   double rbp[3][3];


   eraPmat00(2400000.5, 50123.9999, rbp);

   vvd(rbp[0][0], 0.9999995505175087260, 1e-12,
       "eraPmat00", "11", status);
   vvd(rbp[0][1], 0.8695405883617884705e-3, 1e-14,
       "eraPmat00", "12", status);
   vvd(rbp[0][2], 0.3779734722239007105e-3, 1e-14,
       "eraPmat00", "13", status);

   vvd(rbp[1][0], -0.8695405990410863719e-3, 1e-14,
       "eraPmat00", "21", status);
   vvd(rbp[1][1], 0.9999996219494925900, 1e-12,
       "eraPmat00", "22", status);
   vvd(rbp[1][2], -0.1360775820404982209e-6, 1e-14,
       "eraPmat00", "23", status);

   vvd(rbp[2][0], -0.3779734476558184991e-3, 1e-14,
       "eraPmat00", "31", status);
   vvd(rbp[2][1], -0.1925857585832024058e-6, 1e-14,
       "eraPmat00", "32", status);
   vvd(rbp[2][2], 0.9999999285680153377, 1e-12,
       "eraPmat00", "33", status);

}

static void t_pmat06(int *status)
/*
**  - - - - - - - - -
**   t _ p m a t 0 6
**  - - - - - - - - -
**
**  Test eraPmat06 function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraPmat06, vvd
**
**  This revision:  2013 August 7
*/
{
   double rbp[3][3];


   eraPmat06(2400000.5, 50123.9999, rbp);

   vvd(rbp[0][0], 0.9999995505176007047, 1e-12,
       "eraPmat06", "11", status);
   vvd(rbp[0][1], 0.8695404617348208406e-3, 1e-14,
       "eraPmat06", "12", status);
   vvd(rbp[0][2], 0.3779735201865589104e-3, 1e-14,
       "eraPmat06", "13", status);

   vvd(rbp[1][0], -0.8695404723772031414e-3, 1e-14,
       "eraPmat06", "21", status);
   vvd(rbp[1][1], 0.9999996219496027161, 1e-12,
       "eraPmat06", "22", status);
   vvd(rbp[1][2], -0.1361752497080270143e-6, 1e-14,
       "eraPmat06", "23", status);

   vvd(rbp[2][0], -0.3779734957034089490e-3, 1e-14,
       "eraPmat06", "31", status);
   vvd(rbp[2][1], -0.1924880847894457113e-6, 1e-14,
       "eraPmat06", "32", status);
   vvd(rbp[2][2], 0.9999999285679971958, 1e-12,
       "eraPmat06", "33", status);

}

static void t_pmat76(int *status)
/*
**  - - - - - - - - -
**   t _ p m a t 7 6
**  - - - - - - - - -
**
**  Test eraPmat76 function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraPmat76, vvd
**
**  This revision:  2013 August 7
*/
{
   double rmatp[3][3];


   eraPmat76(2400000.5, 50123.9999, rmatp);

   vvd(rmatp[0][0], 0.9999995504328350733, 1e-12,
       "eraPmat76", "11", status);
   vvd(rmatp[0][1], 0.8696632209480960785e-3, 1e-14,
       "eraPmat76", "12", status);
   vvd(rmatp[0][2], 0.3779153474959888345e-3, 1e-14,
       "eraPmat76", "13", status);

   vvd(rmatp[1][0], -0.8696632209485112192e-3, 1e-14,
       "eraPmat76", "21", status);
   vvd(rmatp[1][1], 0.9999996218428560614, 1e-12,
       "eraPmat76", "22", status);
   vvd(rmatp[1][2], -0.1643284776111886407e-6, 1e-14,
       "eraPmat76", "23", status);

   vvd(rmatp[2][0], -0.3779153474950335077e-3, 1e-14,
       "eraPmat76", "31", status);
   vvd(rmatp[2][1], -0.1643306746147366896e-6, 1e-14,
       "eraPmat76", "32", status);
   vvd(rmatp[2][2], 0.9999999285899790119, 1e-12,
       "eraPmat76", "33", status);

}

static void t_pm(int *status)
/*
**  - - - - -
**   t _ p m
**  - - - - -
**
**  Test eraPm function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraPm, vvd
**
**  This revision:  2013 August 7
*/
{
   double p[3], r;


   p[0] =  0.3;
   p[1] =  1.2;
   p[2] = -2.5;

   r = eraPm(p);

   vvd(r, 2.789265136196270604, 1e-12, "eraPm", "", status);

}

static void t_pmp(int *status)
/*
**  - - - - - -
**   t _ p m p
**  - - - - - -
**
**  Test eraPmp function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraPmp, vvd
**
**  This revision:  2013 August 7
*/
{
   double a[3], b[3], amb[3];


   a[0] = 2.0;
   a[1] = 2.0;
   a[2] = 3.0;

   b[0] = 1.0;
   b[1] = 3.0;
   b[2] = 4.0;

   eraPmp(a, b, amb);

   vvd(amb[0],  1.0, 1e-12, "eraPmp", "0", status);
   vvd(amb[1], -1.0, 1e-12, "eraPmp", "1", status);
   vvd(amb[2], -1.0, 1e-12, "eraPmp", "2", status);

}

static void t_pmpx(int *status)
/*
**  - - - - - - -
**   t _ p m p x
**  - - - - - - -
**
**  Test eraPmpx function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraPmpx, vvd
**
**  This revision:  2017 March 15
*/
{
   double rc, dc, pr, pd, px, rv, pmt, pob[3], pco[3];


   rc = 1.234;
   dc = 0.789;
   pr = 1e-5;
   pd = -2e-5;
   px = 1e-2;
   rv = 10.0;
   pmt = 8.75;
   pob[0] = 0.9;
   pob[1] = 0.4;
   pob[2] = 0.1;

   eraPmpx(rc, dc, pr, pd, px, rv, pmt, pob, pco);

   vvd(pco[0], 0.2328137623960308438, 1e-12,
               "eraPmpx", "1", status);
   vvd(pco[1], 0.6651097085397855328, 1e-12,
               "eraPmpx", "2", status);
   vvd(pco[2], 0.7095257765896359837, 1e-12,
               "eraPmpx", "3", status);

}

static void t_pmsafe(int *status)
/*
**  - - - - - - - - -
**   t _ p m s a f e
**  - - - - - - - - -
**
**  Test eraPmsafe function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraPmsafe, vvd, viv
**
**  This revision:  2017 March 15
*/
{
   int j;
   double ra1, dec1, pmr1, pmd1, px1, rv1, ep1a, ep1b, ep2a, ep2b,
          ra2, dec2, pmr2, pmd2, px2, rv2;


   ra1 = 1.234;
   dec1 = 0.789;
   pmr1 = 1e-5;
   pmd1 = -2e-5;
   px1 = 1e-2;
   rv1 = 10.0;
   ep1a = 2400000.5;
   ep1b = 48348.5625;
   ep2a = 2400000.5;
   ep2b = 51544.5;

   j = eraPmsafe(ra1, dec1, pmr1, pmd1, px1, rv1,
                 ep1a, ep1b, ep2a, ep2b,
                 &ra2, &dec2, &pmr2, &pmd2, &px2, &rv2);

   vvd(ra2, 1.234087484501017061, 1e-12,
            "eraPmsafe", "ra2", status);
   vvd(dec2, 0.7888249982450468567, 1e-12,
            "eraPmsafe", "dec2", status);
   vvd(pmr2, 0.9996457663586073988e-5, 1e-12,
             "eraPmsafe", "pmr2", status);
   vvd(pmd2, -0.2000040085106754565e-4, 1e-16,
             "eraPmsafe", "pmd2", status);
   vvd(px2, 0.9999997295356830666e-2, 1e-12,
            "eraPmsafe", "px2", status);
   vvd(rv2, 10.38468380293920069, 1e-10,
            "eraPmsafe", "rv2", status);
   viv ( j, 0, "eraPmsafe", "j", status);

}

static void t_pn(int *status)
/*
**  - - - - -
**   t _ p n
**  - - - - -
**
**  Test eraPn function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraPn, vvd
**
**  This revision:  2013 August 7
*/
{
   double p[3], r, u[3];


   p[0] =  0.3;
   p[1] =  1.2;
   p[2] = -2.5;

   eraPn(p, &r, u);

   vvd(r, 2.789265136196270604, 1e-12, "eraPn", "r", status);

   vvd(u[0], 0.1075552109073112058, 1e-12, "eraPn", "u1", status);
   vvd(u[1], 0.4302208436292448232, 1e-12, "eraPn", "u2", status);
   vvd(u[2], -0.8962934242275933816, 1e-12, "eraPn", "u3", status);

}

static void t_pn00(int *status)
/*
**  - - - - - - -
**   t _ p n 0 0
**  - - - - - - -
**
**  Test eraPn00 function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraPn00, vvd
**
**  This revision:  2013 August 7
*/
{
   double dpsi, deps, epsa,
          rb[3][3], rp[3][3], rbp[3][3], rn[3][3], rbpn[3][3];


   dpsi = -0.9632552291149335877e-5;
   deps =  0.4063197106621141414e-4;

   eraPn00(2400000.5, 53736.0, dpsi, deps,
           &epsa, rb, rp, rbp, rn, rbpn);

   vvd(epsa, 0.4090791789404229916, 1e-12, "eraPn00", "epsa", status);

   vvd(rb[0][0], 0.9999999999999942498, 1e-12,
       "eraPn00", "rb11", status);
   vvd(rb[0][1], -0.7078279744199196626e-7, 1e-18,
       "eraPn00", "rb12", status);
   vvd(rb[0][2], 0.8056217146976134152e-7, 1e-18,
       "eraPn00", "rb13", status);

   vvd(rb[1][0], 0.7078279477857337206e-7, 1e-18,
       "eraPn00", "rb21", status);
   vvd(rb[1][1], 0.9999999999999969484, 1e-12,
       "eraPn00", "rb22", status);
   vvd(rb[1][2], 0.3306041454222136517e-7, 1e-18,
       "eraPn00", "rb23", status);

   vvd(rb[2][0], -0.8056217380986972157e-7, 1e-18,
       "eraPn00", "rb31", status);
   vvd(rb[2][1], -0.3306040883980552500e-7, 1e-18,
       "eraPn00", "rb32", status);
   vvd(rb[2][2], 0.9999999999999962084, 1e-12,
       "eraPn00", "rb33", status);

   vvd(rp[0][0], 0.9999989300532289018, 1e-12,
       "eraPn00", "rp11", status);
   vvd(rp[0][1], -0.1341647226791824349e-2, 1e-14,
       "eraPn00", "rp12", status);
   vvd(rp[0][2], -0.5829880927190296547e-3, 1e-14,
       "eraPn00", "rp13", status);

   vvd(rp[1][0], 0.1341647231069759008e-2, 1e-14,
       "eraPn00", "rp21", status);
   vvd(rp[1][1], 0.9999990999908750433, 1e-12,
       "eraPn00", "rp22", status);
   vvd(rp[1][2], -0.3837444441583715468e-6, 1e-14,
       "eraPn00", "rp23", status);

   vvd(rp[2][0], 0.5829880828740957684e-3, 1e-14,
       "eraPn00", "rp31", status);
   vvd(rp[2][1], -0.3984203267708834759e-6, 1e-14,
       "eraPn00", "rp32", status);
   vvd(rp[2][2], 0.9999998300623538046, 1e-12,
       "eraPn00", "rp33", status);

   vvd(rbp[0][0], 0.9999989300052243993, 1e-12,
       "eraPn00", "rbp11", status);
   vvd(rbp[0][1], -0.1341717990239703727e-2, 1e-14,
       "eraPn00", "rbp12", status);
   vvd(rbp[0][2], -0.5829075749891684053e-3, 1e-14,
       "eraPn00", "rbp13", status);

   vvd(rbp[1][0], 0.1341718013831739992e-2, 1e-14,
       "eraPn00", "rbp21", status);
   vvd(rbp[1][1], 0.9999990998959191343, 1e-12,
       "eraPn00", "rbp22", status);
   vvd(rbp[1][2], -0.3505759733565421170e-6, 1e-14,
       "eraPn00", "rbp23", status);

   vvd(rbp[2][0], 0.5829075206857717883e-3, 1e-14,
       "eraPn00", "rbp31", status);
   vvd(rbp[2][1], -0.4315219955198608970e-6, 1e-14,
       "eraPn00", "rbp32", status);
   vvd(rbp[2][2], 0.9999998301093036269, 1e-12,
       "eraPn00", "rbp33", status);

   vvd(rn[0][0], 0.9999999999536069682, 1e-12,
       "eraPn00", "rn11", status);
   vvd(rn[0][1], 0.8837746144872140812e-5, 1e-16,
       "eraPn00", "rn12", status);
   vvd(rn[0][2], 0.3831488838252590008e-5, 1e-16,
       "eraPn00", "rn13", status);

   vvd(rn[1][0], -0.8837590456633197506e-5, 1e-16,
       "eraPn00", "rn21", status);
   vvd(rn[1][1], 0.9999999991354692733, 1e-12,
       "eraPn00", "rn22", status);
   vvd(rn[1][2], -0.4063198798559573702e-4, 1e-16,
       "eraPn00", "rn23", status);

   vvd(rn[2][0], -0.3831847930135328368e-5, 1e-16,
       "eraPn00", "rn31", status);
   vvd(rn[2][1], 0.4063195412258150427e-4, 1e-16,
       "eraPn00", "rn32", status);
   vvd(rn[2][2], 0.9999999991671806225, 1e-12,
       "eraPn00", "rn33", status);

   vvd(rbpn[0][0], 0.9999989440499982806, 1e-12,
       "eraPn00", "rbpn11", status);
   vvd(rbpn[0][1], -0.1332880253640848301e-2, 1e-14,
       "eraPn00", "rbpn12", status);
   vvd(rbpn[0][2], -0.5790760898731087295e-3, 1e-14,
       "eraPn00", "rbpn13", status);

   vvd(rbpn[1][0], 0.1332856746979948745e-2, 1e-14,
       "eraPn00", "rbpn21", status);
   vvd(rbpn[1][1], 0.9999991109064768883, 1e-12,
       "eraPn00", "rbpn22", status);
   vvd(rbpn[1][2], -0.4097740555723063806e-4, 1e-14,
       "eraPn00", "rbpn23", status);

   vvd(rbpn[2][0], 0.5791301929950205000e-3, 1e-14,
       "eraPn00", "rbpn31", status);
   vvd(rbpn[2][1], 0.4020553681373702931e-4, 1e-14,
       "eraPn00", "rbpn32", status);
   vvd(rbpn[2][2], 0.9999998314958529887, 1e-12,
       "eraPn00", "rbpn33", status);

}

static void t_pn00a(int *status)
/*
**  - - - - - - - -
**   t _ p n 0 0 a
**  - - - - - - - -
**
**  Test eraPn00a function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraPn00a, vvd
**
**  This revision:  2013 August 7
*/
{
   double dpsi, deps, epsa,
          rb[3][3], rp[3][3], rbp[3][3], rn[3][3], rbpn[3][3];


   eraPn00a(2400000.5, 53736.0,
            &dpsi, &deps, &epsa, rb, rp, rbp, rn, rbpn);

   vvd(dpsi, -0.9630909107115518431e-5, 1e-12,
       "eraPn00a", "dpsi", status);
   vvd(deps,  0.4063239174001678710e-4, 1e-12,
       "eraPn00a", "deps", status);
   vvd(epsa,  0.4090791789404229916, 1e-12, "eraPn00a", "epsa", status);

   vvd(rb[0][0], 0.9999999999999942498, 1e-12,
       "eraPn00a", "rb11", status);
   vvd(rb[0][1], -0.7078279744199196626e-7, 1e-16,
       "eraPn00a", "rb12", status);
   vvd(rb[0][2], 0.8056217146976134152e-7, 1e-16,
       "eraPn00a", "rb13", status);

   vvd(rb[1][0], 0.7078279477857337206e-7, 1e-16,
       "eraPn00a", "rb21", status);
   vvd(rb[1][1], 0.9999999999999969484, 1e-12,
       "eraPn00a", "rb22", status);
   vvd(rb[1][2], 0.3306041454222136517e-7, 1e-16,
       "eraPn00a", "rb23", status);

   vvd(rb[2][0], -0.8056217380986972157e-7, 1e-16,
       "eraPn00a", "rb31", status);
   vvd(rb[2][1], -0.3306040883980552500e-7, 1e-16,
       "eraPn00a", "rb32", status);
   vvd(rb[2][2], 0.9999999999999962084, 1e-12,
       "eraPn00a", "rb33", status);

   vvd(rp[0][0], 0.9999989300532289018, 1e-12,
       "eraPn00a", "rp11", status);
   vvd(rp[0][1], -0.1341647226791824349e-2, 1e-14,
       "eraPn00a", "rp12", status);
   vvd(rp[0][2], -0.5829880927190296547e-3, 1e-14,
       "eraPn00a", "rp13", status);

   vvd(rp[1][0], 0.1341647231069759008e-2, 1e-14,
       "eraPn00a", "rp21", status);
   vvd(rp[1][1], 0.9999990999908750433, 1e-12,
       "eraPn00a", "rp22", status);
   vvd(rp[1][2], -0.3837444441583715468e-6, 1e-14,
       "eraPn00a", "rp23", status);

   vvd(rp[2][0], 0.5829880828740957684e-3, 1e-14,
       "eraPn00a", "rp31", status);
   vvd(rp[2][1], -0.3984203267708834759e-6, 1e-14,
       "eraPn00a", "rp32", status);
   vvd(rp[2][2], 0.9999998300623538046, 1e-12,
       "eraPn00a", "rp33", status);

   vvd(rbp[0][0], 0.9999989300052243993, 1e-12,
       "eraPn00a", "rbp11", status);
   vvd(rbp[0][1], -0.1341717990239703727e-2, 1e-14,
       "eraPn00a", "rbp12", status);
   vvd(rbp[0][2], -0.5829075749891684053e-3, 1e-14,
       "eraPn00a", "rbp13", status);

   vvd(rbp[1][0], 0.1341718013831739992e-2, 1e-14,
       "eraPn00a", "rbp21", status);
   vvd(rbp[1][1], 0.9999990998959191343, 1e-12,
       "eraPn00a", "rbp22", status);
   vvd(rbp[1][2], -0.3505759733565421170e-6, 1e-14,
       "eraPn00a", "rbp23", status);

   vvd(rbp[2][0], 0.5829075206857717883e-3, 1e-14,
       "eraPn00a", "rbp31", status);
   vvd(rbp[2][1], -0.4315219955198608970e-6, 1e-14,
       "eraPn00a", "rbp32", status);
   vvd(rbp[2][2], 0.9999998301093036269, 1e-12,
       "eraPn00a", "rbp33", status);

   vvd(rn[0][0], 0.9999999999536227949, 1e-12,
       "eraPn00a", "rn11", status);
   vvd(rn[0][1], 0.8836238544090873336e-5, 1e-14,
       "eraPn00a", "rn12", status);
   vvd(rn[0][2], 0.3830835237722400669e-5, 1e-14,
       "eraPn00a", "rn13", status);

   vvd(rn[1][0], -0.8836082880798569274e-5, 1e-14,
       "eraPn00a", "rn21", status);
   vvd(rn[1][1], 0.9999999991354655028, 1e-12,
       "eraPn00a", "rn22", status);
   vvd(rn[1][2], -0.4063240865362499850e-4, 1e-14,
       "eraPn00a", "rn23", status);

   vvd(rn[2][0], -0.3831194272065995866e-5, 1e-14,
       "eraPn00a", "rn31", status);
   vvd(rn[2][1], 0.4063237480216291775e-4, 1e-14,
       "eraPn00a", "rn32", status);
   vvd(rn[2][2], 0.9999999991671660338, 1e-12,
       "eraPn00a", "rn33", status);

   vvd(rbpn[0][0], 0.9999989440476103435, 1e-12,
       "eraPn00a", "rbpn11", status);
   vvd(rbpn[0][1], -0.1332881761240011763e-2, 1e-14,
       "eraPn00a", "rbpn12", status);
   vvd(rbpn[0][2], -0.5790767434730085751e-3, 1e-14,
       "eraPn00a", "rbpn13", status);

   vvd(rbpn[1][0], 0.1332858254308954658e-2, 1e-14,
       "eraPn00a", "rbpn21", status);
   vvd(rbpn[1][1], 0.9999991109044505577, 1e-12,
       "eraPn00a", "rbpn22", status);
   vvd(rbpn[1][2], -0.4097782710396580452e-4, 1e-14,
       "eraPn00a", "rbpn23", status);

   vvd(rbpn[2][0], 0.5791308472168152904e-3, 1e-14,
       "eraPn00a", "rbpn31", status);
   vvd(rbpn[2][1], 0.4020595661591500259e-4, 1e-14,
       "eraPn00a", "rbpn32", status);
   vvd(rbpn[2][2], 0.9999998314954572304, 1e-12,
       "eraPn00a", "rbpn33", status);

}

static void t_pn00b(int *status)
/*
**  - - - - - - - -
**   t _ p n 0 0 b
**  - - - - - - - -
**
**  Test eraPn00b function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraPn00b, vvd
**
**  This revision:  2013 August 7
*/
{
   double dpsi, deps, epsa,
          rb[3][3], rp[3][3], rbp[3][3], rn[3][3], rbpn[3][3];


   eraPn00b(2400000.5, 53736.0, &dpsi, &deps, &epsa,
            rb, rp, rbp, rn, rbpn);

   vvd(dpsi, -0.9632552291148362783e-5, 1e-12,
       "eraPn00b", "dpsi", status);
   vvd(deps,  0.4063197106621159367e-4, 1e-12,
       "eraPn00b", "deps", status);
   vvd(epsa,  0.4090791789404229916, 1e-12, "eraPn00b", "epsa", status);

   vvd(rb[0][0], 0.9999999999999942498, 1e-12,
      "eraPn00b", "rb11", status);
   vvd(rb[0][1], -0.7078279744199196626e-7, 1e-16,
      "eraPn00b", "rb12", status);
   vvd(rb[0][2], 0.8056217146976134152e-7, 1e-16,
      "eraPn00b", "rb13", status);

   vvd(rb[1][0], 0.7078279477857337206e-7, 1e-16,
      "eraPn00b", "rb21", status);
   vvd(rb[1][1], 0.9999999999999969484, 1e-12,
      "eraPn00b", "rb22", status);
   vvd(rb[1][2], 0.3306041454222136517e-7, 1e-16,
      "eraPn00b", "rb23", status);

   vvd(rb[2][0], -0.8056217380986972157e-7, 1e-16,
      "eraPn00b", "rb31", status);
   vvd(rb[2][1], -0.3306040883980552500e-7, 1e-16,
      "eraPn00b", "rb32", status);
   vvd(rb[2][2], 0.9999999999999962084, 1e-12,
      "eraPn00b", "rb33", status);

   vvd(rp[0][0], 0.9999989300532289018, 1e-12,
      "eraPn00b", "rp11", status);
   vvd(rp[0][1], -0.1341647226791824349e-2, 1e-14,
      "eraPn00b", "rp12", status);
   vvd(rp[0][2], -0.5829880927190296547e-3, 1e-14,
      "eraPn00b", "rp13", status);

   vvd(rp[1][0], 0.1341647231069759008e-2, 1e-14,
      "eraPn00b", "rp21", status);
   vvd(rp[1][1], 0.9999990999908750433, 1e-12,
      "eraPn00b", "rp22", status);
   vvd(rp[1][2], -0.3837444441583715468e-6, 1e-14,
      "eraPn00b", "rp23", status);

   vvd(rp[2][0], 0.5829880828740957684e-3, 1e-14,
      "eraPn00b", "rp31", status);
   vvd(rp[2][1], -0.3984203267708834759e-6, 1e-14,
      "eraPn00b", "rp32", status);
   vvd(rp[2][2], 0.9999998300623538046, 1e-12,
      "eraPn00b", "rp33", status);

   vvd(rbp[0][0], 0.9999989300052243993, 1e-12,
      "eraPn00b", "rbp11", status);
   vvd(rbp[0][1], -0.1341717990239703727e-2, 1e-14,
      "eraPn00b", "rbp12", status);
   vvd(rbp[0][2], -0.5829075749891684053e-3, 1e-14,
      "eraPn00b", "rbp13", status);

   vvd(rbp[1][0], 0.1341718013831739992e-2, 1e-14,
      "eraPn00b", "rbp21", status);
   vvd(rbp[1][1], 0.9999990998959191343, 1e-12,
      "eraPn00b", "rbp22", status);
   vvd(rbp[1][2], -0.3505759733565421170e-6, 1e-14,
      "eraPn00b", "rbp23", status);

   vvd(rbp[2][0], 0.5829075206857717883e-3, 1e-14,
      "eraPn00b", "rbp31", status);
   vvd(rbp[2][1], -0.4315219955198608970e-6, 1e-14,
      "eraPn00b", "rbp32", status);
   vvd(rbp[2][2], 0.9999998301093036269, 1e-12,
      "eraPn00b", "rbp33", status);

   vvd(rn[0][0], 0.9999999999536069682, 1e-12,
      "eraPn00b", "rn11", status);
   vvd(rn[0][1], 0.8837746144871248011e-5, 1e-14,
      "eraPn00b", "rn12", status);
   vvd(rn[0][2], 0.3831488838252202945e-5, 1e-14,
      "eraPn00b", "rn13", status);

   vvd(rn[1][0], -0.8837590456632304720e-5, 1e-14,
      "eraPn00b", "rn21", status);
   vvd(rn[1][1], 0.9999999991354692733, 1e-12,
      "eraPn00b", "rn22", status);
   vvd(rn[1][2], -0.4063198798559591654e-4, 1e-14,
      "eraPn00b", "rn23", status);

   vvd(rn[2][0], -0.3831847930134941271e-5, 1e-14,
      "eraPn00b", "rn31", status);
   vvd(rn[2][1], 0.4063195412258168380e-4, 1e-14,
      "eraPn00b", "rn32", status);
   vvd(rn[2][2], 0.9999999991671806225, 1e-12,
      "eraPn00b", "rn33", status);

   vvd(rbpn[0][0], 0.9999989440499982806, 1e-12,
      "eraPn00b", "rbpn11", status);
   vvd(rbpn[0][1], -0.1332880253640849194e-2, 1e-14,
      "eraPn00b", "rbpn12", status);
   vvd(rbpn[0][2], -0.5790760898731091166e-3, 1e-14,
      "eraPn00b", "rbpn13", status);

   vvd(rbpn[1][0], 0.1332856746979949638e-2, 1e-14,
      "eraPn00b", "rbpn21", status);
   vvd(rbpn[1][1], 0.9999991109064768883, 1e-12,
      "eraPn00b", "rbpn22", status);
   vvd(rbpn[1][2], -0.4097740555723081811e-4, 1e-14,
      "eraPn00b", "rbpn23", status);

   vvd(rbpn[2][0], 0.5791301929950208873e-3, 1e-14,
      "eraPn00b", "rbpn31", status);
   vvd(rbpn[2][1], 0.4020553681373720832e-4, 1e-14,
      "eraPn00b", "rbpn32", status);
   vvd(rbpn[2][2], 0.9999998314958529887, 1e-12,
      "eraPn00b", "rbpn33", status);

}

static void t_pn06a(int *status)
/*
**  - - - - - - - -
**   t _ p n 0 6 a
**  - - - - - - - -
**
**  Test eraPn06a function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraPn06a, vvd
**
**  This revision:  2013 August 7
*/
{
   double dpsi, deps, epsa;
   double rb[3][3], rp[3][3], rbp[3][3], rn[3][3], rbpn[3][3];


   eraPn06a(2400000.5, 53736.0, &dpsi, &deps, &epsa,
            rb, rp, rbp, rn, rbpn);

   vvd(dpsi, -0.9630912025820308797e-5, 1e-12,
       "eraPn06a", "dpsi", status);
   vvd(deps,  0.4063238496887249798e-4, 1e-12,
       "eraPn06a", "deps", status);
   vvd(epsa,  0.4090789763356509926, 1e-12, "eraPn06a", "epsa", status);

   vvd(rb[0][0], 0.9999999999999942497, 1e-12,
       "eraPn06a", "rb11", status);
   vvd(rb[0][1], -0.7078368960971557145e-7, 1e-14,
       "eraPn06a", "rb12", status);
   vvd(rb[0][2], 0.8056213977613185606e-7, 1e-14,
       "eraPn06a", "rb13", status);

   vvd(rb[1][0], 0.7078368694637674333e-7, 1e-14,
       "eraPn06a", "rb21", status);
   vvd(rb[1][1], 0.9999999999999969484, 1e-12,
       "eraPn06a", "rb22", status);
   vvd(rb[1][2], 0.3305943742989134124e-7, 1e-14,
       "eraPn06a", "rb23", status);

   vvd(rb[2][0], -0.8056214211620056792e-7, 1e-14,
       "eraPn06a", "rb31", status);
   vvd(rb[2][1], -0.3305943172740586950e-7, 1e-14,
       "eraPn06a", "rb32", status);
   vvd(rb[2][2], 0.9999999999999962084, 1e-12,
       "eraPn06a", "rb33", status);

   vvd(rp[0][0], 0.9999989300536854831, 1e-12,
       "eraPn06a", "rp11", status);
   vvd(rp[0][1], -0.1341646886204443795e-2, 1e-14,
       "eraPn06a", "rp12", status);
   vvd(rp[0][2], -0.5829880933488627759e-3, 1e-14,
       "eraPn06a", "rp13", status);

   vvd(rp[1][0], 0.1341646890569782183e-2, 1e-14,
       "eraPn06a", "rp21", status);
   vvd(rp[1][1], 0.9999990999913319321, 1e-12,
       "eraPn06a", "rp22", status);
   vvd(rp[1][2], -0.3835944216374477457e-6, 1e-14,
       "eraPn06a", "rp23", status);

   vvd(rp[2][0], 0.5829880833027867368e-3, 1e-14,
       "eraPn06a", "rp31", status);
   vvd(rp[2][1], -0.3985701514686976112e-6, 1e-14,
       "eraPn06a", "rp32", status);
   vvd(rp[2][2], 0.9999998300623534950, 1e-12,
       "eraPn06a", "rp33", status);

   vvd(rbp[0][0], 0.9999989300056797893, 1e-12,
       "eraPn06a", "rbp11", status);
   vvd(rbp[0][1], -0.1341717650545059598e-2, 1e-14,
       "eraPn06a", "rbp12", status);
   vvd(rbp[0][2], -0.5829075756493728856e-3, 1e-14,
       "eraPn06a", "rbp13", status);

   vvd(rbp[1][0], 0.1341717674223918101e-2, 1e-14,
       "eraPn06a", "rbp21", status);
   vvd(rbp[1][1], 0.9999990998963748448, 1e-12,
       "eraPn06a", "rbp22", status);
   vvd(rbp[1][2], -0.3504269280170069029e-6, 1e-14,
       "eraPn06a", "rbp23", status);

   vvd(rbp[2][0], 0.5829075211461454599e-3, 1e-14,
       "eraPn06a", "rbp31", status);
   vvd(rbp[2][1], -0.4316708436255949093e-6, 1e-14,
       "eraPn06a", "rbp32", status);
   vvd(rbp[2][2], 0.9999998301093032943, 1e-12,
       "eraPn06a", "rbp33", status);

   vvd(rn[0][0], 0.9999999999536227668, 1e-12,
       "eraPn06a", "rn11", status);
   vvd(rn[0][1], 0.8836241998111535233e-5, 1e-14,
       "eraPn06a", "rn12", status);
   vvd(rn[0][2], 0.3830834608415287707e-5, 1e-14,
       "eraPn06a", "rn13", status);

   vvd(rn[1][0], -0.8836086334870740138e-5, 1e-14,
       "eraPn06a", "rn21", status);
   vvd(rn[1][1], 0.9999999991354657474, 1e-12,
       "eraPn06a", "rn22", status);
   vvd(rn[1][2], -0.4063240188248455065e-4, 1e-14,
       "eraPn06a", "rn23", status);

   vvd(rn[2][0], -0.3831193642839398128e-5, 1e-14,
       "eraPn06a", "rn31", status);
   vvd(rn[2][1], 0.4063236803101479770e-4, 1e-14,
       "eraPn06a", "rn32", status);
   vvd(rn[2][2], 0.9999999991671663114, 1e-12,
       "eraPn06a", "rn33", status);

   vvd(rbpn[0][0], 0.9999989440480669738, 1e-12,
       "eraPn06a", "rbpn11", status);
   vvd(rbpn[0][1], -0.1332881418091915973e-2, 1e-14,
       "eraPn06a", "rbpn12", status);
   vvd(rbpn[0][2], -0.5790767447612042565e-3, 1e-14,
       "eraPn06a", "rbpn13", status);

   vvd(rbpn[1][0], 0.1332857911250989133e-2, 1e-14,
       "eraPn06a", "rbpn21", status);
   vvd(rbpn[1][1], 0.9999991109049141908, 1e-12,
       "eraPn06a", "rbpn22", status);
   vvd(rbpn[1][2], -0.4097767128546784878e-4, 1e-14,
       "eraPn06a", "rbpn23", status);

   vvd(rbpn[2][0], 0.5791308482835292617e-3, 1e-14,
       "eraPn06a", "rbpn31", status);
   vvd(rbpn[2][1], 0.4020580099454020310e-4, 1e-14,
       "eraPn06a", "rbpn32", status);
   vvd(rbpn[2][2], 0.9999998314954628695, 1e-12,
       "eraPn06a", "rbpn33", status);

}

static void t_pn06(int *status)
/*
**  - - - - - - -
**   t _ p n 0 6
**  - - - - - - -
**
**  Test eraPn06 function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraPn06, vvd
**
**  This revision:  2013 August 7
*/
{
   double dpsi, deps, epsa,
          rb[3][3], rp[3][3], rbp[3][3], rn[3][3], rbpn[3][3];


   dpsi = -0.9632552291149335877e-5;
   deps =  0.4063197106621141414e-4;

   eraPn06(2400000.5, 53736.0, dpsi, deps,
           &epsa, rb, rp, rbp, rn, rbpn);

   vvd(epsa, 0.4090789763356509926, 1e-12, "eraPn06", "epsa", status);

   vvd(rb[0][0], 0.9999999999999942497, 1e-12,
       "eraPn06", "rb11", status);
   vvd(rb[0][1], -0.7078368960971557145e-7, 1e-14,
       "eraPn06", "rb12", status);
   vvd(rb[0][2], 0.8056213977613185606e-7, 1e-14,
       "eraPn06", "rb13", status);

   vvd(rb[1][0], 0.7078368694637674333e-7, 1e-14,
       "eraPn06", "rb21", status);
   vvd(rb[1][1], 0.9999999999999969484, 1e-12,
       "eraPn06", "rb22", status);
   vvd(rb[1][2], 0.3305943742989134124e-7, 1e-14,
       "eraPn06", "rb23", status);

   vvd(rb[2][0], -0.8056214211620056792e-7, 1e-14,
       "eraPn06", "rb31", status);
   vvd(rb[2][1], -0.3305943172740586950e-7, 1e-14,
       "eraPn06", "rb32", status);
   vvd(rb[2][2], 0.9999999999999962084, 1e-12,
       "eraPn06", "rb33", status);

   vvd(rp[0][0], 0.9999989300536854831, 1e-12,
       "eraPn06", "rp11", status);
   vvd(rp[0][1], -0.1341646886204443795e-2, 1e-14,
       "eraPn06", "rp12", status);
   vvd(rp[0][2], -0.5829880933488627759e-3, 1e-14,
       "eraPn06", "rp13", status);

   vvd(rp[1][0], 0.1341646890569782183e-2, 1e-14,
       "eraPn06", "rp21", status);
   vvd(rp[1][1], 0.9999990999913319321, 1e-12,
       "eraPn06", "rp22", status);
   vvd(rp[1][2], -0.3835944216374477457e-6, 1e-14,
       "eraPn06", "rp23", status);

   vvd(rp[2][0], 0.5829880833027867368e-3, 1e-14,
       "eraPn06", "rp31", status);
   vvd(rp[2][1], -0.3985701514686976112e-6, 1e-14,
       "eraPn06", "rp32", status);
   vvd(rp[2][2], 0.9999998300623534950, 1e-12,
       "eraPn06", "rp33", status);

   vvd(rbp[0][0], 0.9999989300056797893, 1e-12,
       "eraPn06", "rbp11", status);
   vvd(rbp[0][1], -0.1341717650545059598e-2, 1e-14,
       "eraPn06", "rbp12", status);
   vvd(rbp[0][2], -0.5829075756493728856e-3, 1e-14,
       "eraPn06", "rbp13", status);

   vvd(rbp[1][0], 0.1341717674223918101e-2, 1e-14,
       "eraPn06", "rbp21", status);
   vvd(rbp[1][1], 0.9999990998963748448, 1e-12,
       "eraPn06", "rbp22", status);
   vvd(rbp[1][2], -0.3504269280170069029e-6, 1e-14,
       "eraPn06", "rbp23", status);

   vvd(rbp[2][0], 0.5829075211461454599e-3, 1e-14,
       "eraPn06", "rbp31", status);
   vvd(rbp[2][1], -0.4316708436255949093e-6, 1e-14,
       "eraPn06", "rbp32", status);
   vvd(rbp[2][2], 0.9999998301093032943, 1e-12,
       "eraPn06", "rbp33", status);

   vvd(rn[0][0], 0.9999999999536069682, 1e-12,
       "eraPn06", "rn11", status);
   vvd(rn[0][1], 0.8837746921149881914e-5, 1e-14,
       "eraPn06", "rn12", status);
   vvd(rn[0][2], 0.3831487047682968703e-5, 1e-14,
       "eraPn06", "rn13", status);

   vvd(rn[1][0], -0.8837591232983692340e-5, 1e-14,
       "eraPn06", "rn21", status);
   vvd(rn[1][1], 0.9999999991354692664, 1e-12,
       "eraPn06", "rn22", status);
   vvd(rn[1][2], -0.4063198798558931215e-4, 1e-14,
       "eraPn06", "rn23", status);

   vvd(rn[2][0], -0.3831846139597250235e-5, 1e-14,
       "eraPn06", "rn31", status);
   vvd(rn[2][1], 0.4063195412258792914e-4, 1e-14,
       "eraPn06", "rn32", status);
   vvd(rn[2][2], 0.9999999991671806293, 1e-12,
       "eraPn06", "rn33", status);

   vvd(rbpn[0][0], 0.9999989440504506688, 1e-12,
       "eraPn06", "rbpn11", status);
   vvd(rbpn[0][1], -0.1332879913170492655e-2, 1e-14,
       "eraPn06", "rbpn12", status);
   vvd(rbpn[0][2], -0.5790760923225655753e-3, 1e-14,
       "eraPn06", "rbpn13", status);

   vvd(rbpn[1][0], 0.1332856406595754748e-2, 1e-14,
       "eraPn06", "rbpn21", status);
   vvd(rbpn[1][1], 0.9999991109069366795, 1e-12,
       "eraPn06", "rbpn22", status);
   vvd(rbpn[1][2], -0.4097725651142641812e-4, 1e-14,
       "eraPn06", "rbpn23", status);

   vvd(rbpn[2][0], 0.5791301952321296716e-3, 1e-14,
       "eraPn06", "rbpn31", status);
   vvd(rbpn[2][1], 0.4020538796195230577e-4, 1e-14,
       "eraPn06", "rbpn32", status);
   vvd(rbpn[2][2], 0.9999998314958576778, 1e-12,
       "eraPn06", "rbpn33", status);

}

static void t_pnm00a(int *status)
/*
**  - - - - - - - - -
**   t _ p n m 0 0 a
**  - - - - - - - - -
**
**  Test eraPnm00a function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraPnm00a, vvd
**
**  This revision:  2013 August 7
*/
{
   double rbpn[3][3];


   eraPnm00a(2400000.5, 50123.9999, rbpn);

   vvd(rbpn[0][0], 0.9999995832793134257, 1e-12,
       "eraPnm00a", "11", status);
   vvd(rbpn[0][1], 0.8372384254137809439e-3, 1e-14,
       "eraPnm00a", "12", status);
   vvd(rbpn[0][2], 0.3639684306407150645e-3, 1e-14,
       "eraPnm00a", "13", status);

   vvd(rbpn[1][0], -0.8372535226570394543e-3, 1e-14,
       "eraPnm00a", "21", status);
   vvd(rbpn[1][1], 0.9999996486491582471, 1e-12,
       "eraPnm00a", "22", status);
   vvd(rbpn[1][2], 0.4132915262664072381e-4, 1e-14,
       "eraPnm00a", "23", status);

   vvd(rbpn[2][0], -0.3639337004054317729e-3, 1e-14,
       "eraPnm00a", "31", status);
   vvd(rbpn[2][1], -0.4163386925461775873e-4, 1e-14,
       "eraPnm00a", "32", status);
   vvd(rbpn[2][2], 0.9999999329094390695, 1e-12,
       "eraPnm00a", "33", status);

}

static void t_pnm00b(int *status)
/*
**  - - - - - - - - -
**   t _ p n m 0 0 b
**  - - - - - - - - -
**
**  Test eraPnm00b function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraPnm00b, vvd
**
**  This revision:  2013 August 7
*/
{
   double rbpn[3][3];


   eraPnm00b(2400000.5, 50123.9999, rbpn);

   vvd(rbpn[0][0], 0.9999995832776208280, 1e-12,
       "eraPnm00b", "11", status);
   vvd(rbpn[0][1], 0.8372401264429654837e-3, 1e-14,
       "eraPnm00b", "12", status);
   vvd(rbpn[0][2], 0.3639691681450271771e-3, 1e-14,
       "eraPnm00b", "13", status);

   vvd(rbpn[1][0], -0.8372552234147137424e-3, 1e-14,
       "eraPnm00b", "21", status);
   vvd(rbpn[1][1], 0.9999996486477686123, 1e-12,
       "eraPnm00b", "22", status);
   vvd(rbpn[1][2], 0.4132832190946052890e-4, 1e-14,
       "eraPnm00b", "23", status);

   vvd(rbpn[2][0], -0.3639344385341866407e-3, 1e-14,
       "eraPnm00b", "31", status);
   vvd(rbpn[2][1], -0.4163303977421522785e-4, 1e-14,
       "eraPnm00b", "32", status);
   vvd(rbpn[2][2], 0.9999999329092049734, 1e-12,
       "eraPnm00b", "33", status);

}

static void t_pnm06a(int *status)
/*
**  - - - - - - - - -
**   t _ p n m 0 6 a
**  - - - - - - - - -
**
**  Test eraPnm06a function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraPnm06a, vvd
**
**  This revision:  2013 August 7
*/
{
   double rbpn[3][3];


   eraPnm06a(2400000.5, 50123.9999, rbpn);

   vvd(rbpn[0][0], 0.9999995832794205484, 1e-12,
       "eraPnm06a", "11", status);
   vvd(rbpn[0][1], 0.8372382772630962111e-3, 1e-14,
       "eraPnm06a", "12", status);
   vvd(rbpn[0][2], 0.3639684771140623099e-3, 1e-14,
       "eraPnm06a", "13", status);

   vvd(rbpn[1][0], -0.8372533744743683605e-3, 1e-14,
       "eraPnm06a", "21", status);
   vvd(rbpn[1][1], 0.9999996486492861646, 1e-12,
       "eraPnm06a", "22", status);
   vvd(rbpn[1][2], 0.4132905944611019498e-4, 1e-14,
       "eraPnm06a", "23", status);

   vvd(rbpn[2][0], -0.3639337469629464969e-3, 1e-14,
       "eraPnm06a", "31", status);
   vvd(rbpn[2][1], -0.4163377605910663999e-4, 1e-14,
       "eraPnm06a", "32", status);
   vvd(rbpn[2][2], 0.9999999329094260057, 1e-12,
       "eraPnm06a", "33", status);

}

static void t_pnm80(int *status)
/*
**  - - - - - - - -
**   t _ p n m 8 0
**  - - - - - - - -
**
**  Test eraPnm80 function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraPnm80, vvd
**
**  This revision:  2013 August 7
*/
{
   double rmatpn[3][3];


   eraPnm80(2400000.5, 50123.9999, rmatpn);

   vvd(rmatpn[0][0], 0.9999995831934611169, 1e-12,
       "eraPnm80", "11", status);
   vvd(rmatpn[0][1], 0.8373654045728124011e-3, 1e-14,
       "eraPnm80", "12", status);
   vvd(rmatpn[0][2], 0.3639121916933106191e-3, 1e-14,
       "eraPnm80", "13", status);

   vvd(rmatpn[1][0], -0.8373804896118301316e-3, 1e-14,
       "eraPnm80", "21", status);
   vvd(rmatpn[1][1], 0.9999996485439674092, 1e-12,
       "eraPnm80", "22", status);
   vvd(rmatpn[1][2], 0.4130202510421549752e-4, 1e-14,
       "eraPnm80", "23", status);

   vvd(rmatpn[2][0], -0.3638774789072144473e-3, 1e-14,
       "eraPnm80", "31", status);
   vvd(rmatpn[2][1], -0.4160674085851722359e-4, 1e-14,
       "eraPnm80", "32", status);
   vvd(rmatpn[2][2], 0.9999999329310274805, 1e-12,
       "eraPnm80", "33", status);

}

static void t_pom00(int *status)
/*
**  - - - - - - - -
**   t _ p o m 0 0
**  - - - - - - - -
**
**  Test eraPom00 function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraPom00, vvd
**
**  This revision:  2013 August 7
*/
{
   double xp, yp, sp, rpom[3][3];


   xp =  2.55060238e-7;
   yp =  1.860359247e-6;
   sp = -0.1367174580728891460e-10;

   eraPom00(xp, yp, sp, rpom);

   vvd(rpom[0][0], 0.9999999999999674721, 1e-12,
       "eraPom00", "11", status);
   vvd(rpom[0][1], -0.1367174580728846989e-10, 1e-16,
       "eraPom00", "12", status);
   vvd(rpom[0][2], 0.2550602379999972345e-6, 1e-16,
       "eraPom00", "13", status);

   vvd(rpom[1][0], 0.1414624947957029801e-10, 1e-16,
       "eraPom00", "21", status);
   vvd(rpom[1][1], 0.9999999999982695317, 1e-12,
       "eraPom00", "22", status);
   vvd(rpom[1][2], -0.1860359246998866389e-5, 1e-16,
       "eraPom00", "23", status);

   vvd(rpom[2][0], -0.2550602379741215021e-6, 1e-16,
       "eraPom00", "31", status);
   vvd(rpom[2][1], 0.1860359247002414021e-5, 1e-16,
       "eraPom00", "32", status);
   vvd(rpom[2][2], 0.9999999999982370039, 1e-12,
       "eraPom00", "33", status);

}

static void t_ppp(int *status)
/*
**  - - - - - -
**   t _ p p p
**  - - - - - -
**
**  Test eraPpp function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraPpp, vvd
**
**  This revision:  2013 August 7
*/
{
   double a[3], b[3], apb[3];


   a[0] = 2.0;
   a[1] = 2.0;
   a[2] = 3.0;

   b[0] = 1.0;
   b[1] = 3.0;
   b[2] = 4.0;

   eraPpp(a, b, apb);

   vvd(apb[0], 3.0, 1e-12, "eraPpp", "0", status);
   vvd(apb[1], 5.0, 1e-12, "eraPpp", "1", status);
   vvd(apb[2], 7.0, 1e-12, "eraPpp", "2", status);

}

static void t_ppsp(int *status)
/*
**  - - - - - - -
**   t _ p p s p
**  - - - - - - -
**
**  Test eraPpsp function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraPpsp, vvd
**
**  This revision:  2013 August 7
*/
{
   double a[3], s, b[3], apsb[3];


   a[0] = 2.0;
   a[1] = 2.0;
   a[2] = 3.0;

   s = 5.0;

   b[0] = 1.0;
   b[1] = 3.0;
   b[2] = 4.0;

   eraPpsp(a, s, b, apsb);

   vvd(apsb[0], 7.0, 1e-12, "eraPpsp", "0", status);
   vvd(apsb[1], 17.0, 1e-12, "eraPpsp", "1", status);
   vvd(apsb[2], 23.0, 1e-12, "eraPpsp", "2", status);

}

static void t_pr00(int *status)
/*
**  - - - - - - -
**   t _ p r 0 0
**  - - - - - - -
**
**  Test eraPr00 function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraPr00, vvd
**
**  This revision:  2013 August 7
*/
{
   double dpsipr, depspr;

   eraPr00(2400000.5, 53736, &dpsipr, &depspr);

   vvd(dpsipr, -0.8716465172668347629e-7, 1e-22,
      "eraPr00", "dpsipr", status);
   vvd(depspr, -0.7342018386722813087e-8, 1e-22,
      "eraPr00", "depspr", status);

}

static void t_prec76(int *status)
/*
**  - - - - - - - - -
**   t _ p r e c 7 6
**  - - - - - - - - -
**
**  Test eraPrec76 function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraPrec76, vvd
**
**  This revision:  2013 August 7
*/
{
   double ep01, ep02, ep11, ep12, zeta, z, theta;


   ep01 = 2400000.5;
   ep02 = 33282.0;
   ep11 = 2400000.5;
   ep12 = 51544.0;

   eraPrec76(ep01, ep02, ep11, ep12, &zeta, &z, &theta);

   vvd(zeta,  0.5588961642000161243e-2, 1e-12,
       "eraPrec76", "zeta",  status);
   vvd(z,     0.5589922365870680624e-2, 1e-12,
       "eraPrec76", "z",     status);
   vvd(theta, 0.4858945471687296760e-2, 1e-12,
       "eraPrec76", "theta", status);

}

static void t_pv2p(int *status)
/*
**  - - - - - - -
**   t _ p v 2 p
**  - - - - - - -
**
**  Test eraPv2p function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraPv2p, vvd
**
**  This revision:  2013 August 7
*/
{
   double pv[2][3], p[3];


   pv[0][0] =  0.3;
   pv[0][1] =  1.2;
   pv[0][2] = -2.5;

   pv[1][0] = -0.5;
   pv[1][1] =  3.1;
   pv[1][2] =  0.9;

   eraPv2p(pv, p);

   vvd(p[0],  0.3, 0.0, "eraPv2p", "1", status);
   vvd(p[1],  1.2, 0.0, "eraPv2p", "2", status);
   vvd(p[2], -2.5, 0.0, "eraPv2p", "3", status);

}

static void t_pv2s(int *status)
/*
**  - - - - - - -
**   t _ p v 2 s
**  - - - - - - -
**
**  Test eraPv2s function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraPv2s, vvd
**
**  This revision:  2013 August 7
*/
{
   double pv[2][3], theta, phi, r, td, pd, rd;


   pv[0][0] = -0.4514964673880165;
   pv[0][1] =  0.03093394277342585;
   pv[0][2] =  0.05594668105108779;

   pv[1][0] =  1.292270850663260e-5;
   pv[1][1] =  2.652814182060692e-6;
   pv[1][2] =  2.568431853930293e-6;

   eraPv2s(pv, &theta, &phi, &r, &td, &pd, &rd);

   vvd(theta, 3.073185307179586515, 1e-12, "eraPv2s", "theta", status);
   vvd(phi, 0.1229999999999999992, 1e-12, "eraPv2s", "phi", status);
   vvd(r, 0.4559999999999999757, 1e-12, "eraPv2s", "r", status);
   vvd(td, -0.7800000000000000364e-5, 1e-16, "eraPv2s", "td", status);
   vvd(pd, 0.9010000000000001639e-5, 1e-16, "eraPv2s", "pd", status);
   vvd(rd, -0.1229999999999999832e-4, 1e-16, "eraPv2s", "rd", status);

}

static void t_pvdpv(int *status)
/*
**  - - - - - - - -
**   t _ p v d p v
**  - - - - - - - -
**
**  Test eraPvdpv function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraPvdpv, vvd
**
**  This revision:  2013 August 7
*/
{
   double a[2][3], b[2][3], adb[2];


   a[0][0] = 2.0;
   a[0][1] = 2.0;
   a[0][2] = 3.0;

   a[1][0] = 6.0;
   a[1][1] = 0.0;
   a[1][2] = 4.0;

   b[0][0] = 1.0;
   b[0][1] = 3.0;
   b[0][2] = 4.0;

   b[1][0] = 0.0;
   b[1][1] = 2.0;
   b[1][2] = 8.0;

   eraPvdpv(a, b, adb);

   vvd(adb[0], 20.0, 1e-12, "eraPvdpv", "1", status);
   vvd(adb[1], 50.0, 1e-12, "eraPvdpv", "2", status);

}

static void t_pvm(int *status)
/*
**  - - - - - -
**   t _ p v m
**  - - - - - -
**
**  Test eraPvm function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraPvm, vvd
**
**  This revision:  2013 August 7
*/
{
   double pv[2][3], r, s;


   pv[0][0] =  0.3;
   pv[0][1] =  1.2;
   pv[0][2] = -2.5;

   pv[1][0] =  0.45;
   pv[1][1] = -0.25;
   pv[1][2] =  1.1;

   eraPvm(pv, &r, &s);

   vvd(r, 2.789265136196270604, 1e-12, "eraPvm", "r", status);
   vvd(s, 1.214495780149111922, 1e-12, "eraPvm", "s", status);

}

static void t_pvmpv(int *status)
/*
**  - - - - - - - -
**   t _ p v m p v
**  - - - - - - - -
**
**  Test eraPvmpv function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraPvmpv, vvd
**
**  This revision:  2013 August 7
*/
{
   double a[2][3], b[2][3], amb[2][3];


   a[0][0] = 2.0;
   a[0][1] = 2.0;
   a[0][2] = 3.0;

   a[1][0] = 5.0;
   a[1][1] = 6.0;
   a[1][2] = 3.0;

   b[0][0] = 1.0;
   b[0][1] = 3.0;
   b[0][2] = 4.0;

   b[1][0] = 3.0;
   b[1][1] = 2.0;
   b[1][2] = 1.0;

   eraPvmpv(a, b, amb);

   vvd(amb[0][0],  1.0, 1e-12, "eraPvmpv", "11", status);
   vvd(amb[0][1], -1.0, 1e-12, "eraPvmpv", "21", status);
   vvd(amb[0][2], -1.0, 1e-12, "eraPvmpv", "31", status);

   vvd(amb[1][0],  2.0, 1e-12, "eraPvmpv", "12", status);
   vvd(amb[1][1],  4.0, 1e-12, "eraPvmpv", "22", status);
   vvd(amb[1][2],  2.0, 1e-12, "eraPvmpv", "32", status);

}

static void t_pvppv(int *status)
/*
**  - - - - - - - -
**   t _ p v p p v
**  - - - - - - - -
**
**  Test eraPvppv function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraPvppv, vvd
**
**  This revision:  2013 August 7
*/
{
   double a[2][3], b[2][3], apb[2][3];


   a[0][0] = 2.0;
   a[0][1] = 2.0;
   a[0][2] = 3.0;

   a[1][0] = 5.0;
   a[1][1] = 6.0;
   a[1][2] = 3.0;

   b[0][0] = 1.0;
   b[0][1] = 3.0;
   b[0][2] = 4.0;

   b[1][0] = 3.0;
   b[1][1] = 2.0;
   b[1][2] = 1.0;

   eraPvppv(a, b, apb);

   vvd(apb[0][0], 3.0, 1e-12, "eraPvppv", "p1", status);
   vvd(apb[0][1], 5.0, 1e-12, "eraPvppv", "p2", status);
   vvd(apb[0][2], 7.0, 1e-12, "eraPvppv", "p3", status);

   vvd(apb[1][0], 8.0, 1e-12, "eraPvppv", "v1", status);
   vvd(apb[1][1], 8.0, 1e-12, "eraPvppv", "v2", status);
   vvd(apb[1][2], 4.0, 1e-12, "eraPvppv", "v3", status);

}

static void t_pvstar(int *status)
/*
**  - - - - - - - - -
**   t _ p v s t a r
**  - - - - - - - - -
**
**  Test eraPvstar function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraPvstar, vvd, viv
**
**  This revision:  2017 March 15
*/
{
   double pv[2][3], ra, dec, pmr, pmd, px, rv;
   int j;


   pv[0][0] =  126668.5912743160601;
   pv[0][1] =  2136.792716839935195;
   pv[0][2] = -245251.2339876830091;

   pv[1][0] = -0.4051854035740712739e-2;
   pv[1][1] = -0.6253919754866173866e-2;
   pv[1][2] =  0.1189353719774107189e-1;

   j = eraPvstar(pv, &ra, &dec, &pmr, &pmd, &px, &rv);

   vvd(ra, 0.1686756e-1, 1e-12, "eraPvstar", "ra", status);
   vvd(dec, -1.093989828, 1e-12, "eraPvstar", "dec", status);
   vvd(pmr, -0.1783235160000472788e-4, 1e-16, "eraPvstar", "pmr", status);
   vvd(pmd, 0.2336024047000619347e-5, 1e-16, "eraPvstar", "pmd", status);
   vvd(px, 0.74723, 1e-12, "eraPvstar", "px", status);
   vvd(rv, -21.60000010107306010, 1e-11, "eraPvstar", "rv", status);

   viv(j, 0, "eraPvstar", "j", status);

}

static void t_pvtob(int *status)
/*
**  - - - - - - - -
**   t _ p v t o b
**  - - - - - - - -
**
**  Test eraPvtob function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraPvtob, vvd
**
**  This revision:  2013 October 2
*/
{
   double elong, phi, hm, xp, yp, sp, theta, pv[2][3];


   elong = 2.0;
   phi = 0.5;
   hm = 3000.0;
   xp = 1e-6;
   yp = -0.5e-6;
   sp = 1e-8;
   theta = 5.0;

   eraPvtob(elong, phi, hm, xp, yp, sp, theta, pv);

   vvd(pv[0][0], 4225081.367071159207, 1e-5,
                 "eraPvtob", "p(1)", status);
   vvd(pv[0][1], 3681943.215856198144, 1e-5,
                 "eraPvtob", "p(2)", status);
   vvd(pv[0][2], 3041149.399241260785, 1e-5,
                 "eraPvtob", "p(3)", status);
   vvd(pv[1][0], -268.4915389365998787, 1e-9,
                 "eraPvtob", "v(1)", status);
   vvd(pv[1][1], 308.0977983288903123, 1e-9,
                 "eraPvtob", "v(2)", status);
   vvd(pv[1][2], 0, 0,
                 "eraPvtob", "v(3)", status);

}

static void t_pvu(int *status)
/*
**  - - - - - -
**   t _ p v u
**  - - - - - -
**
**  Test eraPvu function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraPvu, vvd
**
**  This revision:  2021 January 5
*/
{
   double pv[2][3], upv[2][3];


   pv[0][0] =  126668.5912743160734;
   pv[0][1] =  2136.792716839935565;
   pv[0][2] = -245251.2339876830229;

   pv[1][0] = -0.4051854035740713039e-2;
   pv[1][1] = -0.6253919754866175788e-2;
   pv[1][2] =  0.1189353719774107615e-1;

   eraPvu(2920.0, pv, upv);

   vvd(upv[0][0], 126656.7598605317105, 1e-6,
       "eraPvu", "p1", status);
   vvd(upv[0][1], 2118.531271155726332, 1e-8,
       "eraPvu", "p2", status);
   vvd(upv[0][2], -245216.5048590656190, 1e-6,
       "eraPvu", "p3", status);

   vvd(upv[1][0], -0.4051854035740713039e-2, 1e-12,
       "eraPvu", "v1", status);
   vvd(upv[1][1], -0.6253919754866175788e-2, 1e-12,
       "eraPvu", "v2", status);
   vvd(upv[1][2], 0.1189353719774107615e-1, 1e-12,
       "eraPvu", "v3", status);

}

static void t_pvup(int *status)
/*
**  - - - - - - -
**   t _ p v u p
**  - - - - - - -
**
**  Test eraPvup function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraPvup, vvd
**
**  This revision:  2021 January 5
*/
{
   double pv[2][3], p[3];


   pv[0][0] =  126668.5912743160734;
   pv[0][1] =  2136.792716839935565;
   pv[0][2] = -245251.2339876830229;

   pv[1][0] = -0.4051854035740713039e-2;
   pv[1][1] = -0.6253919754866175788e-2;
   pv[1][2] =  0.1189353719774107615e-1;

   eraPvup(2920.0, pv, p);

   vvd(p[0],  126656.7598605317105,   1e-6, "eraPvup", "1", status);
   vvd(p[1],    2118.531271155726332, 1e-8, "eraPvup", "2", status);
   vvd(p[2], -245216.5048590656190,   1e-6, "eraPvup", "3", status);

}

static void t_pvxpv(int *status)
/*
**  - - - - - - - -
**   t _ p v x p v
**  - - - - - - - -
**
**  Test eraPvxpv function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraPvxpv, vvd
**
**  This revision:  2013 August 7
*/
{
   double a[2][3], b[2][3], axb[2][3];


   a[0][0] = 2.0;
   a[0][1] = 2.0;
   a[0][2] = 3.0;

   a[1][0] = 6.0;
   a[1][1] = 0.0;
   a[1][2] = 4.0;

   b[0][0] = 1.0;
   b[0][1] = 3.0;
   b[0][2] = 4.0;

   b[1][0] = 0.0;
   b[1][1] = 2.0;
   b[1][2] = 8.0;

   eraPvxpv(a, b, axb);

   vvd(axb[0][0],  -1.0, 1e-12, "eraPvxpv", "p1", status);
   vvd(axb[0][1],  -5.0, 1e-12, "eraPvxpv", "p2", status);
   vvd(axb[0][2],   4.0, 1e-12, "eraPvxpv", "p3", status);

   vvd(axb[1][0],  -2.0, 1e-12, "eraPvxpv", "v1", status);
   vvd(axb[1][1], -36.0, 1e-12, "eraPvxpv", "v2", status);
   vvd(axb[1][2],  22.0, 1e-12, "eraPvxpv", "v3", status);

}

static void t_pxp(int *status)
/*
**  - - - - - -
**   t _ p x p
**  - - - - - -
**
**  Test eraPxp function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraPxp, vvd
**
**  This revision:  2013 August 7
*/
{
   double a[3], b[3], axb[3];


   a[0] = 2.0;
   a[1] = 2.0;
   a[2] = 3.0;

   b[0] = 1.0;
   b[1] = 3.0;
   b[2] = 4.0;

   eraPxp(a, b, axb);

   vvd(axb[0], -1.0, 1e-12, "eraPxp", "1", status);
   vvd(axb[1], -5.0, 1e-12, "eraPxp", "2", status);
   vvd(axb[2],  4.0, 1e-12, "eraPxp", "3", status);

}

static void t_refco(int *status)
/*
**  - - - - - - - -
**   t _ r e f c o
**  - - - - - - - -
**
**  Test eraRefco function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraRefco, vvd
**
**  This revision:  2013 October 2
*/
{
   double phpa, tc, rh, wl, refa, refb;


   phpa = 800.0;
   tc = 10.0;
   rh = 0.9;
   wl = 0.4;

   eraRefco(phpa, tc, rh, wl, &refa, &refb);

   vvd(refa, 0.2264949956241415009e-3, 1e-15,
             "eraRefco", "refa", status);
   vvd(refb, -0.2598658261729343970e-6, 1e-18,
             "eraRefco", "refb", status);

}

static void t_rm2v(int *status)
/*
**  - - - - - - -
**   t _ r m 2 v
**  - - - - - - -
**
**  Test eraRm2v function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraRm2v, vvd
**
**  This revision:  2013 August 7
*/
{
   double r[3][3], w[3];


   r[0][0] =  0.00;
   r[0][1] = -0.80;
   r[0][2] = -0.60;

   r[1][0] =  0.80;
   r[1][1] = -0.36;
   r[1][2] =  0.48;

   r[2][0] =  0.60;
   r[2][1] =  0.48;
   r[2][2] = -0.64;

   eraRm2v(r, w);

   vvd(w[0],  0.0,                  1e-12, "eraRm2v", "1", status);
   vvd(w[1],  1.413716694115406957, 1e-12, "eraRm2v", "2", status);
   vvd(w[2], -1.884955592153875943, 1e-12, "eraRm2v", "3", status);

}

static void t_rv2m(int *status)
/*
**  - - - - - - -
**   t _ r v 2 m
**  - - - - - - -
**
**  Test eraRv2m function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraRv2m, vvd
**
**  This revision:  2013 August 7
*/
{
   double w[3], r[3][3];


   w[0] =  0.0;
   w[1] =  1.41371669;
   w[2] = -1.88495559;

   eraRv2m(w, r);

   vvd(r[0][0], -0.7071067782221119905, 1e-14, "eraRv2m", "11", status);
   vvd(r[0][1], -0.5656854276809129651, 1e-14, "eraRv2m", "12", status);
   vvd(r[0][2], -0.4242640700104211225, 1e-14, "eraRv2m", "13", status);

   vvd(r[1][0],  0.5656854276809129651, 1e-14, "eraRv2m", "21", status);
   vvd(r[1][1], -0.0925483394532274246, 1e-14, "eraRv2m", "22", status);
   vvd(r[1][2], -0.8194112531408833269, 1e-14, "eraRv2m", "23", status);

   vvd(r[2][0],  0.4242640700104211225, 1e-14, "eraRv2m", "31", status);
   vvd(r[2][1], -0.8194112531408833269, 1e-14, "eraRv2m", "32", status);
   vvd(r[2][2],  0.3854415612311154341, 1e-14, "eraRv2m", "33", status);

}

static void t_rx(int *status)
/*
**  - - - - -
**   t _ r x
**  - - - - -
**
**  Test eraRx function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraRx, vvd
**
**  This revision:  2013 August 7
*/
{
   double phi, r[3][3];


   phi = 0.3456789;

   r[0][0] = 2.0;
   r[0][1] = 3.0;
   r[0][2] = 2.0;

   r[1][0] = 3.0;
   r[1][1] = 2.0;
   r[1][2] = 3.0;

   r[2][0] = 3.0;
   r[2][1] = 4.0;
   r[2][2] = 5.0;

   eraRx(phi, r);

   vvd(r[0][0], 2.0, 0.0, "eraRx", "11", status);
   vvd(r[0][1], 3.0, 0.0, "eraRx", "12", status);
   vvd(r[0][2], 2.0, 0.0, "eraRx", "13", status);

   vvd(r[1][0], 3.839043388235612460, 1e-12, "eraRx", "21", status);
   vvd(r[1][1], 3.237033249594111899, 1e-12, "eraRx", "22", status);
   vvd(r[1][2], 4.516714379005982719, 1e-12, "eraRx", "23", status);

   vvd(r[2][0], 1.806030415924501684, 1e-12, "eraRx", "31", status);
   vvd(r[2][1], 3.085711545336372503, 1e-12, "eraRx", "32", status);
   vvd(r[2][2], 3.687721683977873065, 1e-12, "eraRx", "33", status);

}

static void t_rxp(int *status)
/*
**  - - - - - -
**   t _ r x p
**  - - - - - -
**
**  Test eraRxp function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraRxp, vvd
**
**  This revision:  2013 August 7
*/
{
   double r[3][3], p[3], rp[3];


   r[0][0] = 2.0;
   r[0][1] = 3.0;
   r[0][2] = 2.0;

   r[1][0] = 3.0;
   r[1][1] = 2.0;
   r[1][2] = 3.0;

   r[2][0] = 3.0;
   r[2][1] = 4.0;
   r[2][2] = 5.0;

   p[0] = 0.2;
   p[1] = 1.5;
   p[2] = 0.1;

   eraRxp(r, p, rp);

   vvd(rp[0], 5.1, 1e-12, "eraRxp", "1", status);
   vvd(rp[1], 3.9, 1e-12, "eraRxp", "2", status);
   vvd(rp[2], 7.1, 1e-12, "eraRxp", "3", status);

}

static void t_rxpv(int *status)
/*
**  - - - - - - -
**   t _ r x p v
**  - - - - - - -
**
**  Test eraRxpv function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraRxpv, vvd
**
**  This revision:  2013 August 7
*/
{
   double r[3][3], pv[2][3], rpv[2][3];


   r[0][0] = 2.0;
   r[0][1] = 3.0;
   r[0][2] = 2.0;

   r[1][0] = 3.0;
   r[1][1] = 2.0;
   r[1][2] = 3.0;

   r[2][0] = 3.0;
   r[2][1] = 4.0;
   r[2][2] = 5.0;

   pv[0][0] = 0.2;
   pv[0][1] = 1.5;
   pv[0][2] = 0.1;

   pv[1][0] = 1.5;
   pv[1][1] = 0.2;
   pv[1][2] = 0.1;

   eraRxpv(r, pv, rpv);

   vvd(rpv[0][0], 5.1, 1e-12, "eraRxpv", "11", status);
   vvd(rpv[1][0], 3.8, 1e-12, "eraRxpv", "12", status);

   vvd(rpv[0][1], 3.9, 1e-12, "eraRxpv", "21", status);
   vvd(rpv[1][1], 5.2, 1e-12, "eraRxpv", "22", status);

   vvd(rpv[0][2], 7.1, 1e-12, "eraRxpv", "31", status);
   vvd(rpv[1][2], 5.8, 1e-12, "eraRxpv", "32", status);

}

static void t_rxr(int *status)
/*
**  - - - - - -
**   t _ r x r
**  - - - - - -
**
**  Test eraRxr function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraRxr, vvd
**
**  This revision:  2013 August 7
*/
{
   double a[3][3], b[3][3], atb[3][3];


   a[0][0] = 2.0;
   a[0][1] = 3.0;
   a[0][2] = 2.0;

   a[1][0] = 3.0;
   a[1][1] = 2.0;
   a[1][2] = 3.0;

   a[2][0] = 3.0;
   a[2][1] = 4.0;
   a[2][2] = 5.0;

   b[0][0] = 1.0;
   b[0][1] = 2.0;
   b[0][2] = 2.0;

   b[1][0] = 4.0;
   b[1][1] = 1.0;
   b[1][2] = 1.0;

   b[2][0] = 3.0;
   b[2][1] = 0.0;
   b[2][2] = 1.0;

   eraRxr(a, b, atb);

   vvd(atb[0][0], 20.0, 1e-12, "eraRxr", "11", status);
   vvd(atb[0][1],  7.0, 1e-12, "eraRxr", "12", status);
   vvd(atb[0][2],  9.0, 1e-12, "eraRxr", "13", status);

   vvd(atb[1][0], 20.0, 1e-12, "eraRxr", "21", status);
   vvd(atb[1][1],  8.0, 1e-12, "eraRxr", "22", status);
   vvd(atb[1][2], 11.0, 1e-12, "eraRxr", "23", status);

   vvd(atb[2][0], 34.0, 1e-12, "eraRxr", "31", status);
   vvd(atb[2][1], 10.0, 1e-12, "eraRxr", "32", status);
   vvd(atb[2][2], 15.0, 1e-12, "eraRxr", "33", status);

}

static void t_ry(int *status)
/*
**  - - - - -
**   t _ r y
**  - - - - -
**
**  Test eraRy function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraRy, vvd
**
**  This revision:  2013 August 7
*/
{
   double theta, r[3][3];


   theta = 0.3456789;

   r[0][0] = 2.0;
   r[0][1] = 3.0;
   r[0][2] = 2.0;

   r[1][0] = 3.0;
   r[1][1] = 2.0;
   r[1][2] = 3.0;

   r[2][0] = 3.0;
   r[2][1] = 4.0;
   r[2][2] = 5.0;

   eraRy(theta, r);

   vvd(r[0][0], 0.8651847818978159930, 1e-12, "eraRy", "11", status);
   vvd(r[0][1], 1.467194920539316554, 1e-12, "eraRy", "12", status);
   vvd(r[0][2], 0.1875137911274457342, 1e-12, "eraRy", "13", status);

   vvd(r[1][0], 3, 1e-12, "eraRy", "21", status);
   vvd(r[1][1], 2, 1e-12, "eraRy", "22", status);
   vvd(r[1][2], 3, 1e-12, "eraRy", "23", status);

   vvd(r[2][0], 3.500207892850427330, 1e-12, "eraRy", "31", status);
   vvd(r[2][1], 4.779889022262298150, 1e-12, "eraRy", "32", status);
   vvd(r[2][2], 5.381899160903798712, 1e-12, "eraRy", "33", status);

}

static void t_rz(int *status)
/*
**  - - - - -
**   t _ r z
**  - - - - -
**
**  Test eraRz function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraRz, vvd
**
**  This revision:  2013 August 7
*/
{
   double psi, r[3][3];


   psi = 0.3456789;

   r[0][0] = 2.0;
   r[0][1] = 3.0;
   r[0][2] = 2.0;

   r[1][0] = 3.0;
   r[1][1] = 2.0;
   r[1][2] = 3.0;

   r[2][0] = 3.0;
   r[2][1] = 4.0;
   r[2][2] = 5.0;

   eraRz(psi, r);

   vvd(r[0][0], 2.898197754208926769, 1e-12, "eraRz", "11", status);
   vvd(r[0][1], 3.500207892850427330, 1e-12, "eraRz", "12", status);
   vvd(r[0][2], 2.898197754208926769, 1e-12, "eraRz", "13", status);

   vvd(r[1][0], 2.144865911309686813, 1e-12, "eraRz", "21", status);
   vvd(r[1][1], 0.865184781897815993, 1e-12, "eraRz", "22", status);
   vvd(r[1][2], 2.144865911309686813, 1e-12, "eraRz", "23", status);

   vvd(r[2][0], 3.0, 1e-12, "eraRz", "31", status);
   vvd(r[2][1], 4.0, 1e-12, "eraRz", "32", status);
   vvd(r[2][2], 5.0, 1e-12, "eraRz", "33", status);

}

static void t_s00a(int *status)
/*
**  - - - - - - -
**   t _ s 0 0 a
**  - - - - - - -
**
**  Test eraS00a function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraS00a, vvd
**
**  This revision:  2013 August 7
*/
{
   double s;


   s = eraS00a(2400000.5, 52541.0);

   vvd(s, -0.1340684448919163584e-7, 1e-18, "eraS00a", "", status);

}

static void t_s00b(int *status)
/*
**  - - - - - - -
**   t _ s 0 0 b
**  - - - - - - -
**
**  Test eraS00b function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraS00b, vvd
**
**  This revision:  2013 August 7
*/
{
   double s;


   s = eraS00b(2400000.5, 52541.0);

   vvd(s, -0.1340695782951026584e-7, 1e-18, "eraS00b", "", status);

}

static void t_s00(int *status)
/*
**  - - - - - -
**   t _ s 0 0
**  - - - - - -
**
**  Test eraS00 function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraS00, vvd
**
**  This revision:  2013 August 7
*/
{
   double x, y, s;


   x = 0.5791308486706011000e-3;
   y = 0.4020579816732961219e-4;

   s = eraS00(2400000.5, 53736.0, x, y);

   vvd(s, -0.1220036263270905693e-7, 1e-18, "eraS00", "", status);

}

static void t_s06a(int *status)
/*
**  - - - - - - -
**   t _ s 0 6 a
**  - - - - - - -
**
**  Test eraS06a function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraS06a, vvd
**
**  This revision:  2013 August 7
*/
{
   double s;


   s = eraS06a(2400000.5, 52541.0);

   vvd(s, -0.1340680437291812383e-7, 1e-18, "eraS06a", "", status);

}

static void t_s06(int *status)
/*
**  - - - - - -
**   t _ s 0 6
**  - - - - - -
**
**  Test eraS06 function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraS06, vvd
**
**  This revision:  2013 August 7
*/
{
   double x, y, s;


   x = 0.5791308486706011000e-3;
   y = 0.4020579816732961219e-4;

   s = eraS06(2400000.5, 53736.0, x, y);

   vvd(s, -0.1220032213076463117e-7, 1e-18, "eraS06", "", status);

}

static void t_s2c(int *status)
/*
**  - - - - - -
**   t _ s 2 c
**  - - - - - -
**
**  Test eraS2c function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraS2c, vvd
**
**  This revision:  2013 August 7
*/
{
   double c[3];


   eraS2c(3.0123, -0.999, c);

   vvd(c[0], -0.5366267667260523906, 1e-12, "eraS2c", "1", status);
   vvd(c[1],  0.0697711109765145365, 1e-12, "eraS2c", "2", status);
   vvd(c[2], -0.8409302618566214041, 1e-12, "eraS2c", "3", status);

}

static void t_s2p(int *status)
/*
**  - - - - - -
**   t _ s 2 p
**  - - - - - -
**
**  Test eraS2p function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraS2p, vvd
**
**  This revision:  2013 August 7
*/
{
   double p[3];


   eraS2p(-3.21, 0.123, 0.456, p);

   vvd(p[0], -0.4514964673880165228, 1e-12, "eraS2p", "x", status);
   vvd(p[1],  0.0309339427734258688, 1e-12, "eraS2p", "y", status);
   vvd(p[2],  0.0559466810510877933, 1e-12, "eraS2p", "z", status);

}

static void t_s2pv(int *status)
/*
**  - - - - - - -
**   t _ s 2 p v
**  - - - - - - -
**
**  Test eraS2pv function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraS2pv, vvd
**
**  This revision:  2013 August 7
*/
{
   double pv[2][3];


   eraS2pv(-3.21, 0.123, 0.456, -7.8e-6, 9.01e-6, -1.23e-5, pv);

   vvd(pv[0][0], -0.4514964673880165228, 1e-12, "eraS2pv", "x", status);
   vvd(pv[0][1],  0.0309339427734258688, 1e-12, "eraS2pv", "y", status);
   vvd(pv[0][2],  0.0559466810510877933, 1e-12, "eraS2pv", "z", status);

   vvd(pv[1][0],  0.1292270850663260170e-4, 1e-16,
       "eraS2pv", "vx", status);
   vvd(pv[1][1],  0.2652814182060691422e-5, 1e-16,
       "eraS2pv", "vy", status);
   vvd(pv[1][2],  0.2568431853930292259e-5, 1e-16,
       "eraS2pv", "vz", status);

}

static void t_s2xpv(int *status)
/*
**  - - - - - - - -
**   t _ s 2 x p v
**  - - - - - - - -
**
**  Test eraS2xpv function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraS2xpv, vvd
**
**  This revision:  2013 August 7
*/
{
   double s1, s2, pv[2][3], spv[2][3];


   s1 = 2.0;
   s2 = 3.0;

   pv[0][0] =  0.3;
   pv[0][1] =  1.2;
   pv[0][2] = -2.5;

   pv[1][0] =  0.5;
   pv[1][1] =  2.3;
   pv[1][2] = -0.4;

   eraS2xpv(s1, s2, pv, spv);

   vvd(spv[0][0],  0.6, 1e-12, "eraS2xpv", "p1", status);
   vvd(spv[0][1],  2.4, 1e-12, "eraS2xpv", "p2", status);
   vvd(spv[0][2], -5.0, 1e-12, "eraS2xpv", "p3", status);

   vvd(spv[1][0],  1.5, 1e-12, "eraS2xpv", "v1", status);
   vvd(spv[1][1],  6.9, 1e-12, "eraS2xpv", "v2", status);
   vvd(spv[1][2], -1.2, 1e-12, "eraS2xpv", "v3", status);

}

static void t_sepp(int *status)
/*
**  - - - - - - -
**   t _ s e p p
**  - - - - - - -
**
**  Test eraSepp function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraSepp, vvd
**
**  This revision:  2013 August 7
*/
{
   double a[3], b[3], s;


   a[0] =  1.0;
   a[1] =  0.1;
   a[2] =  0.2;

   b[0] = -3.0;
   b[1] =  1e-3;
   b[2] =  0.2;

   s = eraSepp(a, b);

   vvd(s, 2.860391919024660768, 1e-12, "eraSepp", "", status);

}

static void t_seps(int *status)
/*
**  - - - - - - -
**   t _ s e p s
**  - - - - - - -
**
**  Test eraSeps function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraSeps, vvd
**
**  This revision:  2013 August 7
*/
{
   double al, ap, bl, bp, s;


   al =  1.0;
   ap =  0.1;

   bl =  0.2;
   bp = -3.0;

   s = eraSeps(al, ap, bl, bp);

   vvd(s, 2.346722016996998842, 1e-14, "eraSeps", "", status);

}

static void t_sp00(int *status)
/*
**  - - - - - - -
**   t _ s p 0 0
**  - - - - - - -
**
**  Test eraSp00 function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraSp00, vvd
**
**  This revision:  2013 August 7
*/
{
   vvd(eraSp00(2400000.5, 52541.0),
       -0.6216698469981019309e-11, 1e-12, "eraSp00", "", status);

}

static void t_starpm(int *status)
/*
**  - - - - - - - - -
**   t _ s t a r p m
**  - - - - - - - - -
**
**  Test eraStarpm function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraStarpm, vvd, viv
**
**  This revision:  2017 March 15
*/
{
   double ra1, dec1, pmr1, pmd1, px1, rv1;
   double ra2, dec2, pmr2, pmd2, px2, rv2;
   int j;


   ra1 =   0.01686756;
   dec1 = -1.093989828;
   pmr1 = -1.78323516e-5;
   pmd1 =  2.336024047e-6;
   px1 =   0.74723;
   rv1 = -21.6;

   j = eraStarpm(ra1, dec1, pmr1, pmd1, px1, rv1,
                 2400000.5, 50083.0, 2400000.5, 53736.0,
                 &ra2, &dec2, &pmr2, &pmd2, &px2, &rv2);

   vvd(ra2, 0.01668919069414256149, 1e-13,
       "eraStarpm", "ra", status);
   vvd(dec2, -1.093966454217127897, 1e-13,
       "eraStarpm", "dec", status);
   vvd(pmr2, -0.1783662682153176524e-4, 1e-17,
       "eraStarpm", "pmr", status);
   vvd(pmd2, 0.2338092915983989595e-5, 1e-17,
       "eraStarpm", "pmd", status);
   vvd(px2, 0.7473533835317719243, 1e-13,
       "eraStarpm", "px", status);
   vvd(rv2, -21.59905170476417175, 1e-11,
       "eraStarpm", "rv", status);

   viv(j, 0, "eraStarpm", "j", status);

}

static void t_starpv(int *status)
/*
**  - - - - - - - - -
**   t _ s t a r p v
**  - - - - - - - - -
**
**  Test eraStarpv function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraStarpv, vvd, viv
**
**  This revision:  2017 March 15
*/
{
   double ra, dec, pmr, pmd, px, rv, pv[2][3];
   int j;


   ra =   0.01686756;
   dec = -1.093989828;
   pmr = -1.78323516e-5;
   pmd =  2.336024047e-6;
   px =   0.74723;
   rv = -21.6;

   j = eraStarpv(ra, dec, pmr, pmd, px, rv, pv);

   vvd(pv[0][0], 126668.5912743160601, 1e-10,
       "eraStarpv", "11", status);
   vvd(pv[0][1], 2136.792716839935195, 1e-12,
       "eraStarpv", "12", status);
   vvd(pv[0][2], -245251.2339876830091, 1e-10,
       "eraStarpv", "13", status);

   vvd(pv[1][0], -0.4051854008955659551e-2, 1e-13,
       "eraStarpv", "21", status);
   vvd(pv[1][1], -0.6253919754414777970e-2, 1e-15,
       "eraStarpv", "22", status);
   vvd(pv[1][2], 0.1189353714588109341e-1, 1e-13,
       "eraStarpv", "23", status);

   viv(j, 0, "eraStarpv", "j", status);

}

static void t_sxp(int *status)
/*
**  - - - - - -
**   t _ s x p
**  - - - - - -
**
**  Test eraSxp function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraSxp, vvd
**
**  This revision:  2013 August 7
*/
{
   double s, p[3], sp[3];


   s = 2.0;

   p[0] =  0.3;
   p[1] =  1.2;
   p[2] = -2.5;

   eraSxp(s, p, sp);

   vvd(sp[0],  0.6, 0.0, "eraSxp", "1", status);
   vvd(sp[1],  2.4, 0.0, "eraSxp", "2", status);
   vvd(sp[2], -5.0, 0.0, "eraSxp", "3", status);

}


static void t_sxpv(int *status)
/*
**  - - - - - - -
**   t _ s x p v
**  - - - - - - -
**
**  Test eraSxpv function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraSxpv, vvd
**
**  This revision:  2013 August 7
*/
{
   double s, pv[2][3], spv[2][3];


   s = 2.0;

   pv[0][0] =  0.3;
   pv[0][1] =  1.2;
   pv[0][2] = -2.5;

   pv[1][0] =  0.5;
   pv[1][1] =  3.2;
   pv[1][2] = -0.7;

   eraSxpv(s, pv, spv);

   vvd(spv[0][0],  0.6, 0.0, "eraSxpv", "p1", status);
   vvd(spv[0][1],  2.4, 0.0, "eraSxpv", "p2", status);
   vvd(spv[0][2], -5.0, 0.0, "eraSxpv", "p3", status);

   vvd(spv[1][0],  1.0, 0.0, "eraSxpv", "v1", status);
   vvd(spv[1][1],  6.4, 0.0, "eraSxpv", "v2", status);
   vvd(spv[1][2], -1.4, 0.0, "eraSxpv", "v3", status);

}

static void t_taitt(int *status)
/*
**  - - - - - - - -
**   t _ t a i t t
**  - - - - - - - -
**
**  Test eraTaitt function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraTaitt, vvd, viv
**
**  This revision:  2013 August 7
*/
{
   double t1, t2;
   int j;


   j = eraTaitt(2453750.5, 0.892482639, &t1, &t2);

   vvd(t1, 2453750.5, 1e-6, "eraTaitt", "t1", status);
   vvd(t2, 0.892855139, 1e-12, "eraTaitt", "t2", status);
   viv(j, 0, "eraTaitt", "j", status);

}

static void t_taiut1(int *status)
/*
**  - - - - - - - - -
**   t _ t a i u t 1
**  - - - - - - - - -
**
**  Test eraTaiut1 function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraTaiut1, vvd, viv
**
**  This revision:  2013 August 7
*/
{
   double u1, u2;
   int j;


   j = eraTaiut1(2453750.5, 0.892482639, -32.6659, &u1, &u2);

   vvd(u1, 2453750.5, 1e-6, "eraTaiut1", "u1", status);
   vvd(u2, 0.8921045614537037037, 1e-12, "eraTaiut1", "u2", status);
   viv(j, 0, "eraTaiut1", "j", status);

}

static void t_taiutc(int *status)
/*
**  - - - - - - - - -
**   t _ t a i u t c
**  - - - - - - - - -
**
**  Test eraTaiutc function.
**
**  Returned:
**     status    LOGICAL     TRUE = success, FALSE = fail
**
**  Called:  eraTaiutc, vvd, viv
**
**  This revision:  2013 October 3
*/
{
   double u1, u2;
   int j;


   j = eraTaiutc(2453750.5, 0.892482639, &u1, &u2);

   vvd(u1, 2453750.5, 1e-6, "eraTaiutc", "u1", status);
   vvd(u2, 0.8921006945555555556, 1e-12, "eraTaiutc", "u2", status);
   viv(j, 0, "eraTaiutc", "j", status);

}

static void t_tcbtdb(int *status)
/*
**  - - - - - - - - -
**   t _ t c b t d b
**  - - - - - - - - -
**
**  Test eraTcbtdb function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraTcbtdb, vvd, viv
**
**  This revision:  2013 August 7
*/
{
   double b1, b2;
   int j;


   j = eraTcbtdb(2453750.5, 0.893019599, &b1, &b2);

   vvd(b1, 2453750.5, 1e-6, "eraTcbtdb", "b1", status);
   vvd(b2, 0.8928551362746343397, 1e-12, "eraTcbtdb", "b2", status);
   viv(j, 0, "eraTcbtdb", "j", status);

}

static void t_tcgtt(int *status)
/*
**  - - - - - - - -
**   t _ t c g t t
**  - - - - - - - -
**
**  Test eraTcgtt function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraTcgtt, vvd, viv
**
**  This revision:  2013 August 7
*/
{
   double t1, t2;
   int j;


   j = eraTcgtt(2453750.5, 0.892862531, &t1, &t2);

   vvd(t1, 2453750.5, 1e-6, "eraTcgtt", "t1", status);
   vvd(t2, 0.8928551387488816828, 1e-12, "eraTcgtt", "t2", status);
   viv(j, 0, "eraTcgtt", "j", status);

}

static void t_tdbtcb(int *status)
/*
**  - - - - - - - - -
**   t _ t d b t c b
**  - - - - - - - - -
**
**  Test eraTdbtcb function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraTdbtcb, vvd, viv
**
**  This revision:  2013 August 7
*/
{
   double b1, b2;
   int j;


   j = eraTdbtcb(2453750.5, 0.892855137, &b1, &b2);

   vvd( b1, 2453750.5, 1e-6, "eraTdbtcb", "b1", status);
   vvd( b2, 0.8930195997253656716, 1e-12, "eraTdbtcb", "b2", status);
   viv(j, 0, "eraTdbtcb", "j", status);

}

static void t_tdbtt(int *status)
/*
**  - - - - - - - -
**   t _ t d b t t
**  - - - - - - - -
**
**  Test eraTdbtt function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraTdbtt, vvd, viv
**
**  This revision:  2013 August 7
*/
{
   double t1, t2;
   int j;


   j = eraTdbtt(2453750.5, 0.892855137, -0.000201, &t1, &t2);

   vvd(t1, 2453750.5, 1e-6, "eraTdbtt", "t1", status);
   vvd(t2, 0.8928551393263888889, 1e-12, "eraTdbtt", "t2", status);
   viv(j, 0, "eraTdbtt", "j", status);

}

static void t_tf2a(int *status)
/*
**  - - - - - - -
**   t _ t f 2 a
**  - - - - - - -
**
**  Test eraTf2a function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraTf2a, vvd, viv
**
**  This revision:  2013 August 7
*/
{
   double a;
   int j;


   j = eraTf2a('+', 4, 58, 20.2, &a);

   vvd(a, 1.301739278189537429, 1e-12, "eraTf2a", "a", status);
   viv(j, 0, "eraTf2a", "j", status);

}

static void t_tf2d(int *status)
/*
**  - - - - - - -
**   t _ t f 2 d
**  - - - - - - -
**
**  Test eraTf2d function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraTf2d, vvd, viv
**
**  This revision:  2013 August 7
*/
{
   double d;
   int j;


   j = eraTf2d(' ', 23, 55, 10.9, &d);

   vvd(d, 0.9966539351851851852, 1e-12, "eraTf2d", "d", status);
   viv(j, 0, "eraTf2d", "j", status);

}

static void t_tpors(int *status)
/*
**  - - - - - - - -
**   t _ t p o r s
**  - - - - - - - -
**
**  Test eraTpors function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraTpors, vvd, viv
**
**  This revision:  2017 October 21
*/
{
   double xi, eta, ra, dec, az1, bz1, az2, bz2;
   int n;


   xi = -0.03;
   eta = 0.07;
   ra = 1.3;
   dec = 1.5;

   n = eraTpors(xi, eta, ra, dec, &az1, &bz1, &az2, &bz2);

   vvd(az1, 1.736621577783208748, 1e-13, "eraTpors", "az1", status);
   vvd(bz1, 1.436736561844090323, 1e-13, "eraTpors", "bz1", status);

   vvd(az2, 4.004971075806584490, 1e-13, "eraTpors", "az2", status);
   vvd(bz2, 1.565084088476417917, 1e-13, "eraTpors", "bz2", status);

   viv(n, 2, "eraTpors", "n", status);

}

static void t_tporv(int *status)
/*
**  - - - - - - - -
**   t _ t p o r v
**  - - - - - - - -
**
**  Test eraTporv function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraTporv, eraS2c, vvd, viv
**
**  This revision:  2017 October 21
*/
{
   double xi, eta, ra, dec, v[3], vz1[3], vz2[3];
   int n;


   xi = -0.03;
   eta = 0.07;
   ra = 1.3;
   dec = 1.5;
   eraS2c(ra, dec, v);

   n = eraTporv(xi, eta, v, vz1, vz2);

   vvd(vz1[0], -0.02206252822366888610, 1e-15,
       "eraTporv", "x1", status);
   vvd(vz1[1], 0.1318251060359645016, 1e-14,
       "eraTporv", "y1", status);
   vvd(vz1[2], 0.9910274397144543895, 1e-14,
       "eraTporv", "z1", status);

   vvd(vz2[0], -0.003712211763801968173, 1e-16,
       "eraTporv", "x2", status);
   vvd(vz2[1], -0.004341519956299836813, 1e-16,
       "eraTporv", "y2", status);
   vvd(vz2[2], 0.9999836852110587012, 1e-14,
       "eraTporv", "z2", status);

   viv(n, 2, "eraTporv", "n", status);

}

static void t_tpsts(int *status)
/*
**  - - - - - - - -
**   t _ t p s t s
**  - - - - - - - -
**
**  Test eraTpsts function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraTpsts, vvd
**
**  This revision:  2017 October 21
*/
{
   double xi, eta, raz, decz, ra, dec;


   xi = -0.03;
   eta = 0.07;
   raz = 2.3;
   decz = 1.5;

   eraTpsts(xi, eta, raz, decz, &ra, &dec);

   vvd(ra, 0.7596127167359629775, 1e-14, "eraTpsts", "ra", status);
   vvd(dec, 1.540864645109263028, 1e-13, "eraTpsts", "dec", status);

}

static void t_tpstv(int *status)
/*
**  - - - - - - - -
**   t _ t p s t v
**  - - - - - - - -
**
**  Test eraTpstv function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraTpstv, eraS2c, vvd
**
**  This revision:  2017 October 21
*/
{
   double xi, eta, raz, decz, vz[3], v[3];


   xi = -0.03;
   eta = 0.07;
   raz = 2.3;
   decz = 1.5;
   eraS2c(raz, decz, vz);

   eraTpstv(xi, eta, vz, v);

   vvd(v[0], 0.02170030454907376677, 1e-15, "eraTpstv", "x", status);
   vvd(v[1], 0.02060909590535367447, 1e-15, "eraTpstv", "y", status);
   vvd(v[2], 0.9995520806583523804, 1e-14, "eraTpstv", "z", status);

}

static void t_tpxes(int *status)
/*
**  - - - - - - - -
**   t _ t p x e s
**  - - - - - - - -
**
**  Test eraTpxes function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraTpxes, vvd, viv
**
**  This revision:  2017 October 21
*/
{
   double ra, dec, raz, decz, xi, eta;
   int j;


   ra = 1.3;
   dec = 1.55;
   raz = 2.3;
   decz = 1.5;

   j = eraTpxes(ra, dec, raz, decz, &xi, &eta);

   vvd(xi, -0.01753200983236980595, 1e-15, "eraTpxes", "xi", status);
   vvd(eta, 0.05962940005778712891, 1e-15, "eraTpxes", "eta", status);

   viv(j, 0, "eraTpxes", "j", status);

}

static void t_tpxev(int *status)
/*
**  - - - - - - - -
**   t _ t p x e v
**  - - - - - - - -
**
**  Test eraTpxev function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraTpxev, eraS2c, vvd
**
**  This revision:  2017 October 21
*/
{
   double ra, dec, raz, decz, v[3], vz[3], xi, eta;
   int j;


   ra = 1.3;
   dec = 1.55;
   raz = 2.3;
   decz = 1.5;
   eraS2c(ra, dec, v);
   eraS2c(raz, decz, vz);

   j = eraTpxev(v, vz, &xi, &eta);

   vvd(xi, -0.01753200983236980595, 1e-15, "eraTpxev", "xi", status);
   vvd(eta, 0.05962940005778712891, 1e-15, "eraTpxev", "eta", status);

   viv(j, 0, "eraTpxev", "j", status);

}

static void t_tr(int *status)
/*
**  - - - - -
**   t _ t r
**  - - - - -
**
**  Test eraTr function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraTr, vvd
**
**  This revision:  2013 August 7
*/
{
   double r[3][3], rt[3][3];


   r[0][0] = 2.0;
   r[0][1] = 3.0;
   r[0][2] = 2.0;

   r[1][0] = 3.0;
   r[1][1] = 2.0;
   r[1][2] = 3.0;

   r[2][0] = 3.0;
   r[2][1] = 4.0;
   r[2][2] = 5.0;

   eraTr(r, rt);

   vvd(rt[0][0], 2.0, 0.0, "eraTr", "11", status);
   vvd(rt[0][1], 3.0, 0.0, "eraTr", "12", status);
   vvd(rt[0][2], 3.0, 0.0, "eraTr", "13", status);

   vvd(rt[1][0], 3.0, 0.0, "eraTr", "21", status);
   vvd(rt[1][1], 2.0, 0.0, "eraTr", "22", status);
   vvd(rt[1][2], 4.0, 0.0, "eraTr", "23", status);

   vvd(rt[2][0], 2.0, 0.0, "eraTr", "31", status);
   vvd(rt[2][1], 3.0, 0.0, "eraTr", "32", status);
   vvd(rt[2][2], 5.0, 0.0, "eraTr", "33", status);

}

static void t_trxp(int *status)
/*
**  - - - - - - -
**   t _ t r x p
**  - - - - - - -
**
**  Test eraTrxp function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraTrxp, vvd
**
**  This revision:  2013 August 7
*/
{
   double r[3][3], p[3], trp[3];


   r[0][0] = 2.0;
   r[0][1] = 3.0;
   r[0][2] = 2.0;

   r[1][0] = 3.0;
   r[1][1] = 2.0;
   r[1][2] = 3.0;

   r[2][0] = 3.0;
   r[2][1] = 4.0;
   r[2][2] = 5.0;

   p[0] = 0.2;
   p[1] = 1.5;
   p[2] = 0.1;

   eraTrxp(r, p, trp);

   vvd(trp[0], 5.2, 1e-12, "eraTrxp", "1", status);
   vvd(trp[1], 4.0, 1e-12, "eraTrxp", "2", status);
   vvd(trp[2], 5.4, 1e-12, "eraTrxp", "3", status);

}

static void t_trxpv(int *status)
/*
**  - - - - - - - -
**   t _ t r x p v
**  - - - - - - - -
**
**  Test eraTrxpv function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraTrxpv, vvd
**
**  This revision:  2013 August 7
*/
{
   double r[3][3], pv[2][3], trpv[2][3];


   r[0][0] = 2.0;
   r[0][1] = 3.0;
   r[0][2] = 2.0;

   r[1][0] = 3.0;
   r[1][1] = 2.0;
   r[1][2] = 3.0;

   r[2][0] = 3.0;
   r[2][1] = 4.0;
   r[2][2] = 5.0;

   pv[0][0] = 0.2;
   pv[0][1] = 1.5;
   pv[0][2] = 0.1;

   pv[1][0] = 1.5;
   pv[1][1] = 0.2;
   pv[1][2] = 0.1;

   eraTrxpv(r, pv, trpv);

   vvd(trpv[0][0], 5.2, 1e-12, "eraTrxpv", "p1", status);
   vvd(trpv[0][1], 4.0, 1e-12, "eraTrxpv", "p1", status);
   vvd(trpv[0][2], 5.4, 1e-12, "eraTrxpv", "p1", status);

   vvd(trpv[1][0], 3.9, 1e-12, "eraTrxpv", "v1", status);
   vvd(trpv[1][1], 5.3, 1e-12, "eraTrxpv", "v2", status);
   vvd(trpv[1][2], 4.1, 1e-12, "eraTrxpv", "v3", status);

}

static void t_tttai(int *status)
/*
**  - - - - - - - -
**   t _ t t t a i
**  - - - - - - - -
**
**  Test eraTttai function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraTttai, vvd, viv
**
**  This revision:  2013 August 7
*/
{
   double a1, a2;
   int j;


   j = eraTttai(2453750.5, 0.892482639, &a1, &a2);

   vvd(a1, 2453750.5, 1e-6, "eraTttai", "a1", status);
   vvd(a2, 0.892110139, 1e-12, "eraTttai", "a2", status);
   viv(j, 0, "eraTttai", "j", status);

}

static void t_tttcg(int *status)
/*
**  - - - - - - - -
**   t _ t t t c g
**  - - - - - - - -
**
**  Test eraTttcg function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraTttcg, vvd, viv
**
**  This revision:  2013 August 7
*/
{
   double g1, g2;
   int j;


   j = eraTttcg(2453750.5, 0.892482639, &g1, &g2);

   vvd( g1, 2453750.5, 1e-6, "eraTttcg", "g1", status);
   vvd( g2, 0.8924900312508587113, 1e-12, "eraTttcg", "g2", status);
   viv(j, 0, "eraTttcg", "j", status);

}

static void t_tttdb(int *status)
/*
**  - - - - - - - -
**   t _ t t t d b
**  - - - - - - - -
**
**  Test eraTttdb function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraTttdb, vvd, viv
**
**  This revision:  2013 August 7
*/
{
   double b1, b2;
   int j;


   j = eraTttdb(2453750.5, 0.892855139, -0.000201, &b1, &b2);

   vvd(b1, 2453750.5, 1e-6, "eraTttdb", "b1", status);
   vvd(b2, 0.8928551366736111111, 1e-12, "eraTttdb", "b2", status);
   viv(j, 0, "eraTttdb", "j", status);

}

static void t_ttut1(int *status)
/*
**  - - - - - - - -
**   t _ t t u t 1
**  - - - - - - - -
**
**  Test eraTtut1 function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraTtut1, vvd, viv
**
**  This revision:  2013 August 7
*/
{
   double u1, u2;
   int j;


   j = eraTtut1(2453750.5, 0.892855139, 64.8499, &u1, &u2);

   vvd(u1, 2453750.5, 1e-6, "eraTtut1", "u1", status);
   vvd(u2, 0.8921045614537037037, 1e-12, "eraTtut1", "u2", status);
   viv(j, 0, "eraTtut1", "j", status);

}

static void t_ut1tai(int *status)
/*
**  - - - - - - - - -
**   t _ u t 1 t a i
**  - - - - - - - - -
**
**  Test eraUt1tai function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraUt1tai, vvd, viv
**
**  This revision:  2013 August 7
*/
{
   double a1, a2;
   int j;


   j = eraUt1tai(2453750.5, 0.892104561, -32.6659, &a1, &a2);

   vvd(a1, 2453750.5, 1e-6, "eraUt1tai", "a1", status);
   vvd(a2, 0.8924826385462962963, 1e-12, "eraUt1tai", "a2", status);
   viv(j, 0, "eraUt1tai", "j", status);

}

static void t_ut1tt(int *status)
/*
**  - - - - - - - -
**   t _ u t 1 t t
**  - - - - - - - -
**
**  Test eraUt1tt function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraUt1tt, vvd, viv
**
**  This revision:  2013 October 3
*/
{
   double t1, t2;
   int j;


   j = eraUt1tt(2453750.5, 0.892104561, 64.8499, &t1, &t2);

   vvd(t1, 2453750.5, 1e-6, "eraUt1tt", "t1", status);
   vvd(t2, 0.8928551385462962963, 1e-12, "eraUt1tt", "t2", status);
   viv(j, 0, "eraUt1tt", "j", status);

}

static void t_ut1utc(int *status)
/*
**  - - - - - - - - -
**   t _ u t 1 u t c
**  - - - - - - - - -
**
**  Test eraUt1utc function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraUt1utc, vvd, viv
**
**  This revision:  2013 August 7
*/
{
   double u1, u2;
   int j;


   j = eraUt1utc(2453750.5, 0.892104561, 0.3341, &u1, &u2);

   vvd(u1, 2453750.5, 1e-6, "eraUt1utc", "u1", status);
   vvd(u2, 0.8921006941018518519, 1e-12, "eraUt1utc", "u2", status);
   viv(j, 0, "eraUt1utc", "j", status);

}

static void t_utctai(int *status)
/*
**  - - - - - - - - -
**   t _ u t c t a i
**  - - - - - - - - -
**
**  Test eraUtctai function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraUtctai, vvd, viv
**
**  This revision:  2013 August 7
*/
{
   double u1, u2;
   int j;


   j = eraUtctai(2453750.5, 0.892100694, &u1, &u2);

   vvd(u1, 2453750.5, 1e-6, "eraUtctai", "u1", status);
   vvd(u2, 0.8924826384444444444, 1e-12, "eraUtctai", "u2", status);
   viv(j, 0, "eraUtctai", "j", status);

}

static void t_utcut1(int *status)
/*
**  - - - - - - - - -
**   t _ u t c u t 1
**  - - - - - - - - -
**
**  Test eraUtcut1 function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraUtcut1, vvd, viv
**
**  This revision:  2013 August 7
*/
{
   double u1, u2;
   int j;


   j = eraUtcut1(2453750.5, 0.892100694, 0.3341, &u1, &u2);

   vvd(u1, 2453750.5, 1e-6, "eraUtcut1", "u1", status);
   vvd(u2, 0.8921045608981481481, 1e-12, "eraUtcut1", "u2", status);
   viv(j, 0, "eraUtcut1", "j", status);

}

static void t_xy06(int *status)
/*
**  - - - - - - -
**   t _ x y 0 6
**  - - - - - - -
**
**  Test eraXy06 function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraXy06, vvd
**
**  This revision:  2013 August 7
*/
{
   double x, y;


   eraXy06(2400000.5, 53736.0, &x, &y);

   vvd(x, 0.5791308486706010975e-3, 1e-15, "eraXy06", "x", status);
   vvd(y, 0.4020579816732958141e-4, 1e-16, "eraXy06", "y", status);

}

static void t_xys00a(int *status)
/*
**  - - - - - - - - -
**   t _ x y s 0 0 a
**  - - - - - - - - -
**
**  Test eraXys00a function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraXys00a, vvd
**
**  This revision:  2013 August 7
*/
{
   double x, y, s;


   eraXys00a(2400000.5, 53736.0, &x, &y, &s);

   vvd(x,  0.5791308472168152904e-3, 1e-14, "eraXys00a", "x", status);
   vvd(y,  0.4020595661591500259e-4, 1e-15, "eraXys00a", "y", status);
   vvd(s, -0.1220040848471549623e-7, 1e-18, "eraXys00a", "s", status);

}

static void t_xys00b(int *status)
/*
**  - - - - - - - - -
**   t _ x y s 0 0 b
**  - - - - - - - - -
**
**  Test eraXys00b function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraXys00b, vvd
**
**  This revision:  2013 August 7
*/
{
   double x, y, s;


   eraXys00b(2400000.5, 53736.0, &x, &y, &s);

   vvd(x,  0.5791301929950208873e-3, 1e-14, "eraXys00b", "x", status);
   vvd(y,  0.4020553681373720832e-4, 1e-15, "eraXys00b", "y", status);
   vvd(s, -0.1220027377285083189e-7, 1e-18, "eraXys00b", "s", status);

}

static void t_xys06a(int *status)
/*
**  - - - - - - - - -
**   t _ x y s 0 6 a
**  - - - - - - - - -
**
**  Test eraXys06a function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraXys06a, vvd
**
**  This revision:  2013 August 7
*/
{
   double x, y, s;


   eraXys06a(2400000.5, 53736.0, &x, &y, &s);

   vvd(x,  0.5791308482835292617e-3, 1e-14, "eraXys06a", "x", status);
   vvd(y,  0.4020580099454020310e-4, 1e-15, "eraXys06a", "y", status);
   vvd(s, -0.1220032294164579896e-7, 1e-18, "eraXys06a", "s", status);

}

static void t_zp(int *status)
/*
**  - - - - -
**   t _ z p
**  - - - - -
**
**  Test eraZp function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraZp, vvd
**
**  This revision:  2013 August 7
*/
{
   double p[3];


   p[0] =  0.3;
   p[1] =  1.2;
   p[2] = -2.5;

   eraZp(p);

   vvd(p[0], 0.0, 0.0, "eraZp", "1", status);
   vvd(p[1], 0.0, 0.0, "eraZp", "2", status);
   vvd(p[2], 0.0, 0.0, "eraZp", "3", status);

}

static void t_zpv(int *status)
/*
**  - - - - - -
**   t _ z p v
**  - - - - - -
**
**  Test eraZpv function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraZpv, vvd
**
**  This revision:  2013 August 7
*/
{
   double pv[2][3];


   pv[0][0] =  0.3;
   pv[0][1] =  1.2;
   pv[0][2] = -2.5;

   pv[1][0] = -0.5;
   pv[1][1] =  3.1;
   pv[1][2] =  0.9;

   eraZpv(pv);

   vvd(pv[0][0], 0.0, 0.0, "eraZpv", "p1", status);
   vvd(pv[0][1], 0.0, 0.0, "eraZpv", "p2", status);
   vvd(pv[0][2], 0.0, 0.0, "eraZpv", "p3", status);

   vvd(pv[1][0], 0.0, 0.0, "eraZpv", "v1", status);
   vvd(pv[1][1], 0.0, 0.0, "eraZpv", "v2", status);
   vvd(pv[1][2], 0.0, 0.0, "eraZpv", "v3", status);

}

static void t_zr(int *status)
/*
**  - - - - -
**   t _ z r
**  - - - - -
**
**  Test eraZr function.
**
**  Returned:
**     status    int         FALSE = success, TRUE = fail
**
**  Called:  eraZr, vvd
**
**  This revision:  2013 August 7
*/
{
   double r[3][3];


   r[0][0] = 2.0;
   r[1][0] = 3.0;
   r[2][0] = 2.0;

   r[0][1] = 3.0;
   r[1][1] = 2.0;
   r[2][1] = 3.0;

   r[0][2] = 3.0;
   r[1][2] = 4.0;
   r[2][2] = 5.0;

   eraZr(r);

   vvd(r[0][0], 0.0, 0.0, "eraZr", "00", status);
   vvd(r[1][0], 0.0, 0.0, "eraZr", "01", status);
   vvd(r[2][0], 0.0, 0.0, "eraZr", "02", status);

   vvd(r[0][1], 0.0, 0.0, "eraZr", "10", status);
   vvd(r[1][1], 0.0, 0.0, "eraZr", "11", status);
   vvd(r[2][1], 0.0, 0.0, "eraZr", "12", status);

   vvd(r[0][2], 0.0, 0.0, "eraZr", "20", status);
   vvd(r[1][2], 0.0, 0.0, "eraZr", "21", status);
   vvd(r[2][2], 0.0, 0.0, "eraZr", "22", status);

}

int main(int argc, char *argv[])
/*
**  - - - - -
**   m a i n
**  - - - - -
**
**  This revision:  2021 April 18
*/
{
   int status;


/* If any command-line argument, switch to verbose reporting. */
   if (argc > 1) {
      verbose = 1;
      argv[0][0] += 0;    /* to avoid compiler warnings */
   }

/* Preset the &status to FALSE = success. */
   status = 0;

/* Test all of the ERFA functions. */
   t_a2af(&status);
   t_a2tf(&status);
   t_ab(&status);
   t_ae2hd(&status);
   t_af2a(&status);
   t_anp(&status);
   t_anpm(&status);
   t_apcg(&status);
   t_apcg13(&status);
   t_apci(&status);
   t_apci13(&status);
   t_apco(&status);
   t_apco13(&status);
   t_apcs(&status);
   t_apcs13(&status);
   t_aper(&status);
   t_aper13(&status);
   t_apio(&status);
   t_apio13(&status);
   t_atcc13(&status);
   t_atccq(&status);
   t_atci13(&status);
   t_atciq(&status);
   t_atciqn(&status);
   t_atciqz(&status);
   t_atco13(&status);
   t_atic13(&status);
   t_aticq(&status);
   t_aticqn(&status);
   t_atio13(&status);
   t_atioq(&status);
   t_atoc13(&status);
   t_atoi13(&status);
   t_atoiq(&status);
   t_bi00(&status);
   t_bp00(&status);
   t_bp06(&status);
   t_bpn2xy(&status);
   t_c2i00a(&status);
   t_c2i00b(&status);
   t_c2i06a(&status);
   t_c2ibpn(&status);
   t_c2ixy(&status);
   t_c2ixys(&status);
   t_c2s(&status);
   t_c2t00a(&status);
   t_c2t00b(&status);
   t_c2t06a(&status);
   t_c2tcio(&status);
   t_c2teqx(&status);
   t_c2tpe(&status);
   t_c2txy(&status);
   t_cal2jd(&status);
   t_cp(&status);
   t_cpv(&status);
   t_cr(&status);
   t_d2dtf(&status);
   t_d2tf(&status);
   t_dat(&status);
   t_dtdb(&status);
   t_dtf2d(&status);
   t_eceq06(&status);
   t_ecm06(&status);
   t_ee00(&status);
   t_ee00a(&status);
   t_ee00b(&status);
   t_ee06a(&status);
   t_eect00(&status);
   t_eform(&status);
   t_eo06a(&status);
   t_eors(&status);
   t_epb(&status);
   t_epb2jd(&status);
   t_epj(&status);
   t_epj2jd(&status);
   t_epv00(&status);
   t_eqec06(&status);
   t_eqeq94(&status);
   t_era00(&status);
   t_fad03(&status);
   t_fae03(&status);
   t_faf03(&status);
   t_faju03(&status);
   t_fal03(&status);
   t_falp03(&status);
   t_fama03(&status);
   t_fame03(&status);
   t_fane03(&status);
   t_faom03(&status);
   t_fapa03(&status);
   t_fasa03(&status);
   t_faur03(&status);
   t_fave03(&status);
   t_fk425(&status);
   t_fk45z(&status);
   t_fk524(&status);
   t_fk52h(&status);
   t_fk54z(&status);
   t_fk5hip(&status);
   t_fk5hz(&status);
   t_fw2m(&status);
   t_fw2xy(&status);
   t_g2icrs(&status);
   t_gc2gd(&status);
   t_gc2gde(&status);
   t_gd2gc(&status);
   t_gd2gce(&status);
   t_gmst00(&status);
   t_gmst06(&status);
   t_gmst82(&status);
   t_gst00a(&status);
   t_gst00b(&status);
   t_gst06(&status);
   t_gst06a(&status);
   t_gst94(&status);
   t_h2fk5(&status);
   t_hd2ae(&status);
   t_hd2pa(&status);
   t_hfk5z(&status);
   t_icrs2g(&status);
   t_ir(&status);
   t_jd2cal(&status);
   t_jdcalf(&status);
   t_ld(&status);
   t_ldn(&status);
   t_ldsun(&status);
   t_lteceq(&status);
   t_ltecm(&status);
   t_lteqec(&status);
   t_ltp(&status);
   t_ltpb(&status);
   t_ltpecl(&status);
   t_ltpequ(&status);
   t_moon98(&status);
   t_num00a(&status);
   t_num00b(&status);
   t_num06a(&status);
   t_numat(&status);
   t_nut00a(&status);
   t_nut00b(&status);
   t_nut06a(&status);
   t_nut80(&status);
   t_nutm80(&status);
   t_obl06(&status);
   t_obl80(&status);
   t_p06e(&status);
   t_p2pv(&status);
   t_p2s(&status);
   t_pap(&status);
   t_pas(&status);
   t_pb06(&status);
   t_pdp(&status);
   t_pfw06(&status);
   t_plan94(&status);
   t_pmat00(&status);
   t_pmat06(&status);
   t_pmat76(&status);
   t_pm(&status);
   t_pmp(&status);
   t_pmpx(&status);
   t_pmsafe(&status);
   t_pn(&status);
   t_pn00(&status);
   t_pn00a(&status);
   t_pn00b(&status);
   t_pn06a(&status);
   t_pn06(&status);
   t_pnm00a(&status);
   t_pnm00b(&status);
   t_pnm06a(&status);
   t_pnm80(&status);
   t_pom00(&status);
   t_ppp(&status);
   t_ppsp(&status);
   t_pr00(&status);
   t_prec76(&status);
   t_pv2p(&status);
   t_pv2s(&status);
   t_pvdpv(&status);
   t_pvm(&status);
   t_pvmpv(&status);
   t_pvppv(&status);
   t_pvstar(&status);
   t_pvtob(&status);
   t_pvu(&status);
   t_pvup(&status);
   t_pvxpv(&status);
   t_pxp(&status);
   t_refco(&status);
   t_rm2v(&status);
   t_rv2m(&status);
   t_rx(&status);
   t_rxp(&status);
   t_rxpv(&status);
   t_rxr(&status);
   t_ry(&status);
   t_rz(&status);
   t_s00a(&status);
   t_s00b(&status);
   t_s00(&status);
   t_s06a(&status);
   t_s06(&status);
   t_s2c(&status);
   t_s2p(&status);
   t_s2pv(&status);
   t_s2xpv(&status);
   t_sepp(&status);
   t_seps(&status);
   t_sp00(&status);
   t_starpm(&status);
   t_starpv(&status);
   t_sxp(&status);
   t_sxpv(&status);
   t_taitt(&status);
   t_taiut1(&status);
   t_taiutc(&status);
   t_tcbtdb(&status);
   t_tcgtt(&status);
   t_tdbtcb(&status);
   t_tdbtt(&status);
   t_tf2a(&status);
   t_tf2d(&status);
   t_tpors(&status);
   t_tporv(&status);
   t_tpsts(&status);
   t_tpstv(&status);
   t_tpxes(&status);
   t_tpxev(&status);
   t_tr(&status);
   t_trxp(&status);
   t_trxpv(&status);
   t_tttai(&status);
   t_tttcg(&status);
   t_tttdb(&status);
   t_ttut1(&status);
   t_ut1tai(&status);
   t_ut1tt(&status) ;
   t_ut1utc(&status);
   t_utctai(&status);
   t_utcut1(&status);
   t_xy06(&status);
   t_xys00a(&status);
   t_xys00b(&status);
   t_xys06a(&status);
   t_zp(&status);
   t_zpv(&status);
   t_zr(&status);

/* Report, set up an appropriate exit status, and finish. */
   if (status) {
      printf("t_erfa_c validation failed!\n");
   } else {
      printf("t_erfa_c validation successful\n");
   }
   return status;
}

/*----------------------------------------------------------------------
**  
**  
**  Copyright (C) 2013-2023, NumFOCUS Foundation.
**  All rights reserved.
**  
**  This library is derived, with permission, from the International
**  Astronomical Union's "Standards of Fundamental Astronomy" library,
**  available from http://www.iausofa.org.
**  
**  The ERFA version is intended to retain identical functionality to
**  the SOFA library, but made distinct through different function and
**  file names, as set out in the SOFA license conditions.  The SOFA
**  original has a role as a reference standard for the IAU and IERS,
**  and consequently redistribution is permitted only in its unaltered
**  state.  The ERFA version is not subject to this restriction and
**  therefore can be included in distributions which do not support the
**  concept of "read only" software.
**  
**  Although the intent is to replicate the SOFA API (other than
**  replacement of prefix names) and results (with the exception of
**  bugs;  any that are discovered will be fixed), SOFA is not
**  responsible for any errors found in this version of the library.
**  
**  If you wish to acknowledge the SOFA heritage, please acknowledge
**  that you are using a library derived from SOFA, rather than SOFA
**  itself.
**  
**  
**  TERMS AND CONDITIONS
**  
**  Redistribution and use in source and binary forms, with or without
**  modification, are permitted provided that the following conditions
**  are met:
**  
**  1 Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
**  
**  2 Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in
**    the documentation and/or other materials provided with the
**    distribution.
**  
**  3 Neither the name of the Standards Of Fundamental Astronomy Board,
**    the International Astronomical Union nor the names of its
**    contributors may be used to endorse or promote products derived
**    from this software without specific prior written permission.
**  
**  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
**  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
**  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
**  FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
**  COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
**  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
**  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
**  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
**  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
**  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
**  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
**  POSSIBILITY OF SUCH DAMAGE.
**  
*/
