#if 0
INDI
Copyright (C) 2003 Elwood C. Downey

This library is free software;
you can redistribute it and / or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation;
either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY;
without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library;
if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110 - 1301  USA

#endif

#pragma once

/** \mainpage Instrument Neutral Distributed Interface INDI
 *
\section Introduction

<p>INDI is a simple XML-like communications protocol described for interactive and automated remote control of diverse instrumentation. INDI is small, easy to parse, and stateless.</p>
<p>In the INDI paradigm each Device poses all command and status functions in terms of settings and getting Properties.
Each Property is a vector of one or more names members. Each property has a current value vector; a target value vector; provides information about how it should be sequenced with
respect to other Properties to accomplish one coordinated unit of observation; and provides hints as to how it might be displayed for interactive manipulation in a GUI.</p>

<p>Clients learn the Properties of a particular Device at runtime using introspection. This decouples Client and Device implementation histories. Devices have a complete authority over whether to accept commands from Clients. INDI accommadates intermediate servers, broadcasting, and connection topologies ranging from one-to-one on a single system to many-to-many between systems of different genre.</p>

<p>The INDI protocol can be nested within other XML elements such as constraints for automatic scheduling and execution.
For a complete review on the INDI protocol, please refer to the INDI <a href="http://www.clearskyinstitute.com/INDI/INDI.pdf">white paper</a>.</p>

\section Audience Intended Audience

INDI is intended for developers who seek a scalable API for device control and automation. Hardware drivers written under INDI can be used under any INDI-compatible client. INDI serves as a backend only, you need frontend clients to control devices. Current clients include <a href="http://edu.kde.org/kstars">KStars</a>, <a href="http://www.clearyskyinstitute.com/xephem">Xephem</a>, <a href="http://pygtkindiclient.sourceforge.net/">DCD</a>, and <a href="http://www.stargazing.net/astropc">Cartes du Ciel</a>.

\section Development Developing under INDI

<p>Please refere to the <a href="http://www.indilib.org/develop/developer-manual">INDI Developers Manual</a> for a complete guide on INDI's driver developemnt framework.</p>

<p>The INDI Library API is divided into the following main sections:</p>
<ul>
<li><a href="indidevapi_8h.html">INDI Device API</a></li>
<li><a href="classINDI_1_1BaseClient.html">INDI Client API</a></li>
<li><a href="namespaceINDI.html">INDI Base Drivers</a>: Base classes for all INDI drivers. Current base drivers include:
 <ul>
<li><a href="classINDI_1_1Telescope.html">Telescope</a></li>
<li><a href="classINDI_1_1CCD.html">CCD</a></li>
<li><a href="classINDI_1_1GuiderInterface.html">Guider</a></li>
<li><a href="classINDI_1_1FilterWheel.html">Filter Wheel</a></li>
<li><a href="classINDI_1_1Focuser.html">Focuser</a></li>
<li><a href="classINDI_1_1Dome.html">Dome</a></li>
<li><a href="classINDI_1_1Weather.html">Weather</a></li>
<li><a href="classINDI_1_1GPS.html">GPS</a></li>
<li><a href="classINDI_1_1USBDevice.html">USB</a></li>
</ul>
<li>@ref INDI::SP "INDI Standard Properties"</li>
<li><a href="md_libs_indibase_alignment_alignment_white_paper.html">INDI Alignment Subsystem</a></li>
<li><a href="structINDI_1_1Logger.html">INDI Debugging & Logging API</a></li>
<li><a href="indicom_8h.html">INDI Common Routine Library</a></li>
<li><a href="lilxml_8h.html">INDI LilXML Library</a></li>
<li><a href="group__configFunctions.html">Configuration</a></li>
</ul>

\section Tutorials

INDI Library includes a number of tutorials to illustrate development of INDI drivers. Check out the <a href="examples.html">examples</a> provided with INDI library.

\section Simulators

Simulators provide a great framework to test drivers and equipment alike. INDI Library provides the following simulators:
<ul>
<li><b>Telescope Simulator</b>: Offers GOTO capability, motion control, guiding, and ability to set Periodic Error (PE) which is read by the CCD simulator when generating images.</li>
<li><b>CCD Simulator</b>: Offers a very flexible CCD simulator with a primary CCD chip and a guide chip. The simulator generate images based on the RA & DEC coordinates it
 snoops from the telescope driver using General Star Catalog (GSC). Please note that you must install GSC for the CCD simulator to work properly. Furthermore,
 The simulator snoops FWHM from the focuser simulator which affects the generated images focus. All images are generated in standard FITS format.</li>
<li><b>Filter Wheel Simulator</b>: Offers a simple simulator to change filter wheels and their corresponding designations.</li>
<li><b>Focuser Simulator</b>: Offers a simple simualtor for an absolute position focuser. It generates a simulated FWHM value that may be used by other simulator such as the CCD simulator.</li>
<li><b>Dome Simulator</b>: Offers a simple simulator for an absolute position dome with shutter.
<li><b>GPS Simulator</b>: Offers a simple simulator for GPS devices that send time and location data to the client and other drivers.
</ul>

\section Help

You can find information on INDI development in the <a href="http://www.indilib.org">INDI Library</a> site. Furthermore, you can discuss INDI related issues on the <a href="http://sourceforge.net/mail/?group_id=90275">INDI development mailing list</a>.

\author Jasem Mutlaq
\author Elwood Downey
*/

/** \file indiapi.h
    \brief Constants and Data structure definitions for the interface to the reference INDI C API implementation.
    \author Elwood C. Downey
*/

/*******************************************************************************
 * INDI wire protocol version implemented by this API.
 * N.B. this is indepedent of the API itself.
 */

#define INDIV 1.7

/* INDI Library version */
#define INDI_VERSION_MAJOR   1
#define INDI_VERSION_MINOR   5
#define INDI_VERSION_RELEASE 0

/*******************************************************************************
 * Manifest constants
 */

/**
 * @typedef ISState
 * @brief Switch state.
 */
typedef enum {
    ISS_OFF = 0, /*!< Switch is OFF */
    ISS_ON       /*!< Switch is ON */
} ISState;

/**
 * @typedef IPState
 * @brief Property state.
 */
typedef enum {
    IPS_IDLE = 0, /*!< State is idle */
    IPS_OK,       /*!< State is ok */
    IPS_BUSY,     /*!< State is busy */
    IPS_ALERT     /*!< State is alert */
} IPState;

/**
 * @typedef ISRule
 * @brief Switch vector rule hint.
 */
typedef enum {
    ISR_1OFMANY, /*!< Only 1 switch of many can be ON (e.g. radio buttons) */
    ISR_ATMOST1, /*!< At most one switch can be ON, but all switches can be off. It is similar to ISR_1OFMANY with the exception that all switches can be off. */
    ISR_NOFMANY  /*!< Any number of switches can be ON (e.g. check boxes) */
} ISRule;

/**
 * @typedef IPerm
 * @brief Permission hint, with respect to client.
 */
typedef enum {
    IP_RO, /*!< Read Only */
    IP_WO, /*!< Write Only */
    IP_RW  /*!< Read & Write */
} IPerm;

// The XML strings for these attributes may be any length but implementations
// are only obligued to support these lengths for the various string attributes.
#define MAXINDINAME    64
#define MAXINDILABEL   64
#define MAXINDIDEVICE  64
#define MAXINDIGROUP   64
#define MAXINDIFORMAT  64
#define MAXINDIBLOBFMT 64
#define MAXINDITSTAMP  64
#define MAXINDIMESSAGE 255

/*******************************************************************************
 * Typedefs for each INDI Property type.
 *
 * INumber.format may be any printf-style appropriate for double
 * or style "m" to create sexigesimal using the form "%<w>.<f>m" where
 *   <w> is the total field width.
 *   <f> is the width of the fraction. valid values are:
 *      9  ->  :mm:ss.ss
 *      8  ->  :mm:ss.s
 *      6  ->  :mm:ss
 *      5  ->  :mm.m
 *      3  ->  :mm
 *
 * examples:
 *
 *   to produce     use
 *
 *    "-123:45"    %7.3m
 *  "  0:01:02"    %9.6m
 */

/**
 * @struct IText
 * @brief One text descriptor.
 */
typedef struct
{
    /** Index name */
    char name[MAXINDINAME];
    /** Short description */
    char label[MAXINDILABEL];
    /** Allocated text string */
    char *text;
    /** Pointer to parent */
    struct _ITextVectorProperty *tvp;
    /** Helper info */
    void *aux0;
    /** Helper info */
    void *aux1;
} IText;

/**
 * @struct _ITextVectorProperty
 * @brief Text vector property descriptor.
 */
typedef struct _ITextVectorProperty
{
    /** Device name */
    char device[MAXINDIDEVICE];
    /** Property name */
    char name[MAXINDINAME];
    /** Short description */
    char label[MAXINDILABEL];
    /** GUI grouping hint */
    char group[MAXINDIGROUP];
    /** Client accessibility hint */
    IPerm p;
    /** Current max time to change, secs */
    double timeout;
    /** Current property state */
    IPState s;
    /** Texts comprising this vector */
    IText *tp;
    /** Dimension of tp[] */
    int ntp;
    /** ISO 8601 timestamp of this event */
    char timestamp[MAXINDITSTAMP];
    /** Helper info */
    void *aux;
} ITextVectorProperty;

/**
 * @struct INumber
 * @brief One number descriptor.
 */
typedef struct
{
    /** Index name */
    char name[MAXINDINAME];
    /** Short description */
    char label[MAXINDILABEL];
    /** GUI display format, see above */
    char format[MAXINDIFORMAT];
    /** Range min, ignored if min == max */
    double min;
    /** Range max, ignored if min == max */
    double max;
    /** Step size, ignored if step == 0 */
    double step;
    /** Current value */
    double value;
    /** Pointer to parent */
    struct _INumberVectorProperty *nvp;
    /** Helper info */
    void *aux0;
    /** Helper info */
    void *aux1;
} INumber;

/**
 * @struct _INumberVectorProperty
 * @brief Number vector property descriptor.
 *
 * INumber.format may be any printf-style appropriate for double or style
 * "m" to create sexigesimal using the form "%\<w\>.\<f\>m" where:\n
 * \<w\> is the total field width.\n
 * \<f\> is the width of the fraction. valid values are:\n
 *       9  ->  \<w\>:mm:ss.ss \n
 *       8  ->  \<w\>:mm:ss.s \n
 *       6  ->  \<w\>:mm:ss \n
 *       5  ->  \<w\>:mm.m \n
 *       3  ->  \<w\>:mm \n
 *
 * examples:\n
 *
 * To produce "-123:45", use \%7.3m \n
 * To produce "  0:01:02", use \%9.6m
 */
typedef struct _INumberVectorProperty
{
    /** Device name */
    char device[MAXINDIDEVICE];
    /** Property name */
    char name[MAXINDINAME];
    /** Short description */
    char label[MAXINDILABEL];
    /** GUI grouping hint */
    char group[MAXINDIGROUP];
    /** Client accessibility hint */
    IPerm p;
    /** Current max time to change, secs */
    double timeout;
    /** current property state */
    IPState s;
    /** Numbers comprising this vector */
    INumber *np;
    /** Dimension of np[] */
    int nnp;
    /** ISO 8601 timestamp of this event */
    char timestamp[MAXINDITSTAMP];
    /** Helper info */
    void *aux;
} INumberVectorProperty;

/**
 * @struct ISwitch
 * @brief One switch descriptor.
 */
typedef struct
{
    /** Index name */
    char name[MAXINDINAME];
    /** Switch label */
    char label[MAXINDILABEL];
    /** Switch state */
    ISState s;
    /** Pointer to parent */
    struct _ISwitchVectorProperty *svp;
    /** Helper info */
    void *aux;
} ISwitch;

/**
 * @struct _ISwitchVectorProperty
 * @brief Switch vector property descriptor.
 */
typedef struct _ISwitchVectorProperty
{
    /** Device name */
    char device[MAXINDIDEVICE];
    /** Property name */
    char name[MAXINDINAME];
    /** Short description */
    char label[MAXINDILABEL];
    /** GUI grouping hint */
    char group[MAXINDIGROUP];
    /** Client accessibility hint */
    IPerm p;
    /** Switch behavior hint */
    ISRule r;
    /** Current max time to change, secs */
    double timeout;
    /** Current property state */
    IPState s;
    /** Switches comprising this vector */
    ISwitch *sp;
    /** Dimension of sp[] */
    int nsp;
    /** ISO 8601 timestamp of this event */
    char timestamp[MAXINDITSTAMP];
    /** Helper info */
    void *aux;
} ISwitchVectorProperty;

/**
 * @struct ILight
 * @brief One light descriptor.
 */
typedef struct
{
    /** Index name */
    char name[MAXINDINAME];
    /** Light labels */
    char label[MAXINDILABEL];
    /** Light state */
    IPState s;
    /** Pointer to parent */
    struct _ILightVectorProperty *lvp;
    /** Helper info */
    void *aux;
} ILight;

/**
 * @struct _ILightVectorProperty
 * @brief Light vector property descriptor.
 */
typedef struct _ILightVectorProperty
{
    /** Device name */
    char device[MAXINDIDEVICE];
    /** Property name */
    char name[MAXINDINAME];
    /** Short description */
    char label[MAXINDILABEL];
    /** GUI grouping hint */
    char group[MAXINDIGROUP];
    /** Current property state */
    IPState s;
    /** Lights comprising this vector */
    ILight *lp;
    /** Dimension of lp[] */
    int nlp;
    /** ISO 8601 timestamp of this event */
    char timestamp[MAXINDITSTAMP];
    /** Helper info */
    void *aux;
} ILightVectorProperty;

/**
 * @struct IBLOB
 * @brief One Blob (Binary Large Object) descriptor.
 */
typedef struct /* one BLOB descriptor */
{
    /** Index name */
    char name[MAXINDINAME];
    /** Blob label */
    char label[MAXINDILABEL];
    /** Format attr */
    char format[MAXINDIBLOBFMT];
    /** Allocated binary large object bytes */
    void *blob;
    /** Blob size in bytes */
    int bloblen;
    /** N uncompressed bytes */
    int size;
    /** Pointer to parent */
    struct _IBLOBVectorProperty *bvp;
    /** Helper info */
    void *aux0;
    /** Helper info */
    void *aux1;
    /** Helper info */
    void *aux2;
} IBLOB;

/**
 * @struct _IBLOBVectorProperty
 * @brief BLOB (Binary Large Object) vector property descriptor.
 */
typedef struct _IBLOBVectorProperty /* BLOB vector property descriptor */
{
    /** Device name */
    char device[MAXINDIDEVICE];
    /** Property name */
    char name[MAXINDINAME];
    /** Short description */
    char label[MAXINDILABEL];
    /** GUI grouping hint */
    char group[MAXINDIGROUP];
    /** Client accessibility hint */
    IPerm p;
    /** Current max time to change, secs */
    double timeout;
    /** Current property state */
    IPState s;
    /** BLOBs comprising this vector */
    IBLOB *bp;
    /** Dimension of bp[] */
    int nbp;
    /** ISO 8601 timestamp of this event */
    char timestamp[MAXINDITSTAMP];
    /** Helper info */
    void *aux;
} IBLOBVectorProperty;

/**
 * @brief Handy macro to find the number of elements in array a[]. Must be used
 * with actual array, not pointer.
 */
#define NARRAY(a) (sizeof(a) / sizeof(a[0]))
