#if 0
INDI
Copyright (C) 2003 - 2006 Elwood C. Downey

Modified by Jasem Mutlaq (2003 - 2015)

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

/** \file indidevapi.h
    \brief Interface to the reference INDI C API device implementation on the Device Driver side.
*
\author Elwood C. Downey
\author Jasem Mutlaq

This file is divided into two main sections:\n
<ol><li> Functions the INDI device driver framework defines which the Driver may
call:</li>

<ul><li>IDxxx functions to send messages to an INDI client. Please note that it is recommended to use the
 INDI Library wrapper functions instead of calling IDxxx directly.
 <ul>
  <li>IDMessage: Use @ref INDI::Logger "INDI Logging Framework" functions (e.g. LOG_DEBUG..etc) instead.</li>
  <li>IDDefXXX: use @ref INDI::DefaultDevice "INDI Default Device" defXXX functions intead.</li>
 </ul>
 </li>
<li>IExxx functions to implement the event driven model.</li>
<li>IUxxx functions to perform handy utility functions.</li></ul>

<li>Functions the INDI device driver framework calls which the Driver must
define:</li>

<ul><li>ISxxx to respond to messages from a Client.</li></ul></ol>

<p>These functions are the interface to the INDI C-language Device Driver
reference implementation library. Any driver that uses this interface is
expected to \#include "indidevapi.h" and to link with indidrivermain.o and
eventloop.o. Indidevapi.h further includes indiapi.h. The former contains
the prototypes for the functions documented here, although many functions
take arguments defined in the latter.</p>

<p>These functions make it much easier to write a compliant INDI driver than
starting from scratch, and also serve as a concrete
example of the interactions an INDI driver, in any language, is expected to
accommodate.</p>

<p>The reference driver framework and the optimizations made within the reference
indiserver both assume and require that one driver program implements exactly
one logical INDI device.</p>

<p>The functions in this framework fall into two broad categories. Some are
functions that a driver must define because they are called by the reference
framework; these functions begin with IS. The remaining functions are library
utilities a driver may use to do important operations.</p>

<p>A major point to realize is that an INDI driver built with this framework
does not contain the C main() function. As soon as a driver begins executing,
it listens on stdin for INDI messages. Only when a valid and appropriate
message is received will it then call the driver via one of the IS functions.
The driver is then expected to respond promptly by calling one of the ID
library functions. It may also use any of the IU utility functions as desired
to make processing a message easier.</p>

<p>Rather separate from these IS, ID and IU functions are a collection of
functions that utilize the notion of a callback. In a callback design,
the driver registers a function with the framework to be called under
certain circumstances. When said circumstances occur, the framework will
call the callback function. The driver never calls these callbacks directly.
These callback functions begin with IE. They can arrange for a callback
function to be called under three kinds of circumstances: when a given file
descriptor may be read without blocking (because either data is available
or EOF has been encountered), when a given time interval has elapsed, or
when the framework has nothing urgent to do. The callback functions for each
circumstance must be written according to a well defined prototype since,
after all, the framework must know how to call the callback correctlty.</p>

*/

/*******************************************************************************
 * get the data structures
 */

#include "indiapi.h"
#include "lilxml.h"

/*******************************************************************************
 *******************************************************************************
 *
 *  Functions the INDI device driver framework defines which the Driver may call
 *
 *******************************************************************************
 *******************************************************************************
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \defgroup d2cFunctions IDDef Functions: Functions drivers call to define their properties to clients.

<p>Each of the following functions creates the appropriate XML formatted
INDI message from its arguments and writes it to stdout. From there, is it
typically read by indiserver which then sends it to the clients that have
expressed interest in messages from the Device indicated in the message.</p>

<p>In addition to type-specific arguments, all end with a printf-style format
string, and appropriate subsequent arguments, that form the \param msg
attribute within the INDI message. If the format argument is NULL, no message
attribute is included with the message. Note that a \e timestamp
attribute is also always added automatically based on the clock on the
computer on which this driver is running.</p>

*/

/*@{*/

/** \brief Tell client to create a text vector property.
    \param t pointer to the vector text property to be defined.
    \param msg message in printf style to send to the client. May be NULL.
*/
extern void IDDefText(const ITextVectorProperty *t, const char *msg, ...)
#ifdef __GNUC__
__attribute__((format(printf, 2, 3)))
#endif
;

/** \brief Tell client to create a number number property.
    \param n pointer to the vector number property to be defined.
    \param msg message in printf style to send to the client. May be NULL.
*/
extern void IDDefNumber(const INumberVectorProperty *n, const char *msg, ...)
#ifdef __GNUC__
__attribute__((format(printf, 2, 3)))
#endif
;

/** \brief Tell client to create a switch vector property.
    \param s pointer to the vector switch property to be defined.
    \param msg message in printf style to send to the client. May be NULL.
*/
extern void IDDefSwitch(const ISwitchVectorProperty *s, const char *msg, ...)
#ifdef __GNUC__
__attribute__((format(printf, 2, 3)))
#endif
;

/** \brief Tell client to create a light vector property.
    \param l pointer to the vector light property to be defined.
    \param msg message in printf style to send to the client. May be NULL.
*/
extern void IDDefLight(const ILightVectorProperty *l, const char *msg, ...)
#ifdef __GNUC__
__attribute__((format(printf, 2, 3)))
#endif
;

/** \brief Tell client to create a BLOB vector property.
    \param b pointer to the vector BLOB property to be defined.
    \param msg message in printf style to send to the client. May be NULL.
 */
extern void IDDefBLOB(const IBLOBVectorProperty *b, const char *msg, ...)
#ifdef __GNUC__
__attribute__((format(printf, 2, 3)))
#endif
;

/*@}*/

/**
 * \defgroup d2cuFunctions IDSet Functions: Functions drivers call to tell clients of new values for existing properties.
 */
/*@{*/

/** \brief Tell client to update an existing text vector property.
    \param t pointer to the vector text property.
    \param msg message in printf style to send to the client. May be NULL.
*/
extern void IDSetText(const ITextVectorProperty *t, const char *msg, ...)
#ifdef __GNUC__
__attribute__((format(printf, 2, 3)))
#endif
;

/** \brief Tell client to update an existing number vector property.
    \param n pointer to the vector number property.
    \param msg message in printf style to send to the client. May be NULL.
*/
extern void IDSetNumber(const INumberVectorProperty *n, const char *msg, ...)
#ifdef __GNUC__
__attribute__((format(printf, 2, 3)))
#endif
;

/** \brief Tell client to update an existing switch vector property.
    \param s pointer to the vector switch property.
    \param msg message in printf style to send to the client. May be NULL.
*/
extern void IDSetSwitch(const ISwitchVectorProperty *s, const char *msg, ...)
#ifdef __GNUC__
__attribute__((format(printf, 2, 3)))
#endif
;

/** \brief Tell client to update an existing light vector property.
    \param l pointer to the vector light property.
    \param msg message in printf style to send to the client. May be NULL.
*/
extern void IDSetLight(const ILightVectorProperty *l, const char *msg, ...)
#ifdef __GNUC__
__attribute__((format(printf, 2, 3)))
#endif
;

/** \brief Tell client to update an existing BLOB vector property.
    \param b pointer to the vector BLOB property.
    \param msg message in printf style to send to the client. May be NULL.
 */
extern void IDSetBLOB(const IBLOBVectorProperty *b, const char *msg, ...)
#ifdef __GNUC__
__attribute__((format(printf, 2, 3)))
#endif
;

/*@}*/

/**
 * \defgroup d2duFunctions ID Functions: Functions to delete properties, and log messages locally or remotely.
 */
/*@{*/

/** \brief Function Drivers call to send log messages to Clients.

    If dev is specified the Client shall associate the message with that device; if dev is NULL the Client shall treat the message as generic from no specific Device.

    \param dev device name
    \param msg message in printf style to send to the client.
*/
extern void IDMessage(const char *dev, const char *msg, ...)
#ifdef __GNUC__
__attribute__((format(printf, 2, 3)))
#endif
;

/** \brief Function Drivers call to inform Clients a Property is no longer available, or the entire device is gone if name is NULL.

    \param dev device name. If device name is NULL, the entire device will be deleted.
    \param name property name to be deleted.
    \param msg message in printf style to send to the client.
*/
extern void IDDelete(const char *dev, const char *name, const char *msg, ...)
#ifdef __GNUC__
__attribute__((format(printf, 3, 4)))
#endif
;

/** \brief Function Drivers call to log a message locally.

    The message is not sent to any Clients.

    \param msg message in printf style to send to the client.
*/
extern void IDLog(const char *msg, ...)
#ifdef __GNUC__
__attribute__((format(printf, 1, 2)))
#endif
;

/*@}*/

/**
 * \defgroup snoopFunctions ISnoop Functions: Functions drivers call to snoop on other drivers.

 */
/*@{*/

/** \typedef BLOBHandling
    \brief How drivers handle BLOBs incoming from snooping drivers */
typedef enum
{
    B_NEVER = 0, /*!< Never receive BLOBs */
    B_ALSO,      /*!< Receive BLOBs along with normal messages */
    B_ONLY       /*!< ONLY receive BLOBs from drivers, ignore all other traffic */
} BLOBHandling;

/** \brief Function a Driver calls to snoop on another Device. Snooped messages will then arrive via ISSnoopDevice.
    \param snooped_device name of the device to snoop.
    \param snooped_property name of the snooped property in the device.
*/
extern void IDSnoopDevice(const char *snooped_device, const char *snooped_property);

/** \brief Function a Driver calls to control whether they will receive BLOBs from snooped devices.
    \param snooped_device name of the device to snoop.
    \param snooped_property name of property to snoop. If NULL, then all BLOBs from the given device are snooped.
    \param bh How drivers handle BLOBs incoming from snooping drivers.
*/
extern void IDSnoopBLOBs(const char *snooped_device, const char *snooped_property, BLOBHandling bh);

/*@}*/

/**
 * \defgroup deventFunctions IE Functions: Functions drivers call to register with the INDI event utilities.

     Callbacks are called when a read on a file descriptor will not block. Timers are called once after a specified interval. Workprocs are called when there is nothing else to do. The "Add" functions return a unique id for use with their corresponding "Rm" removal function. An arbitrary pointer may be specified when a function is registered which will be stored and forwarded unchanged when the function is later invoked.
 */
/*@{*/

/* signature of a callback, timout caller and work procedure function */

/** \typedef IE_CBF
    \brief Signature of a callback. */
typedef void(IE_CBF)(int readfiledes, void *userpointer);

/** \typedef IE_TCF
    \brief Signature of a timeout caller. */
typedef void(IE_TCF)(void *userpointer);

/** \typedef IE_WPF
    \brief Signature of a work procedure function. */
typedef void(IE_WPF)(void *userpointer);

/* functions to add and remove callbacks, timers and work procedures */

/** \brief Register a new callback, \e fp, to be called with \e userpointer as argument when \e readfiledes is ready.
*
* \param readfiledes file descriptor.
* \param fp a pointer to the callback function.
* \param userpointer a pointer to be passed to the callback function when called.
* \return a unique callback id for use with IERmCallback().
*/
extern int IEAddCallback(int readfiledes, IE_CBF *fp, void *userpointer);

/** \brief Remove a callback function.
*
* \param callbackid the callback ID returned from IEAddCallback()
*/
extern void IERmCallback(int callbackid);

/** \brief Register a new timer function, \e fp, to be called with \e ud as argument after \e ms.

 Add to list in order of decreasing time from epoch, ie, last entry runs soonest. The timer will only invoke the callback function \b once. You need to call addTimer again if you want to repeat the process.
*
* \param millisecs timer period in milliseconds.
* \param fp a pointer to the callback function.
* \param userpointer a pointer to be passed to the callback function when called.
* \return a unique id for use with IERmTimer().
*/
extern int IEAddTimer(int millisecs, IE_TCF *fp, void *userpointer);

/** \brief Remove the timer with the given \e timerid, as returned from IEAddTimer.
*
* \param timerid the timer callback ID returned from IEAddTimer().
*/
extern void IERmTimer(int timerid);

/** \brief Add a new work procedure, fp, to be called with ud when nothing else to do.
*
* \param fp a pointer to the work procedure callback function.
* \param userpointer a pointer to be passed to the callback function when called.
* \return a unique id for use with IERmWorkProc().
*/
extern int IEAddWorkProc(IE_WPF *fp, void *userpointer);

/** \brief Remove a work procedure.
*
* \param workprocid The unique ID for the work procedure to be removed.
*/
extern void IERmWorkProc(int workprocid);

/* wait in-line for a flag to set, presumably by another event function */

extern int IEDeferLoop(int maxms, int *flagp);
extern int IEDeferLoop0(int maxms, int *flagp);

/*@}*/

/**
 * \defgroup dutilFunctions IU Functions: Functions drivers call to perform handy utility routines.

<p>This section describes handy utility functions that are provided by the
framework for tasks commonly required in the processing of client
messages. It is not strictly necessary to use these functions, but it
both prudent and efficient to do so.</P>

<p>These do not communicate with the Client in any way.</p>
 */
/*@{*/

/** \brief Find an IText member in a vector text property.
*
* \param tvp a pointer to a text vector property.
* \param name the name of the member to search for.
* \return a pointer to an IText member on match, or NULL if nothing is found.
*/
extern IText *IUFindText(const ITextVectorProperty *tvp, const char *name);

/** \brief Find an INumber member in a number text property.
*
* \param nvp a pointer to a number vector property.
* \param name the name of the member to search for.
* \return a pointer to an INumber member on match, or NULL if nothing is found.
*/
extern INumber *IUFindNumber(const INumberVectorProperty *nvp, const char *name);

/** \brief Find an ISwitch member in a vector switch property.
*
* \param svp a pointer to a switch vector property.
* \param name the name of the member to search for.
* \return a pointer to an ISwitch member on match, or NULL if nothing is found.
*/
extern ISwitch *IUFindSwitch(const ISwitchVectorProperty *svp, const char *name);

/** \brief Find an ILight member in a vector Light property.
*
* \param lvp a pointer to a Light vector property.
* \param name the name of the member to search for.
* \return a pointer to an ILight member on match, or NULL if nothing is found.
*/
extern ILight *IUFindLight(const ILightVectorProperty *lvp, const char *name);

/** \brief Find an IBLOB member in a vector BLOB property.
*
* \param bvp a pointer to a BLOB vector property.
* \param name the name of the member to search for.
* \return a pointer to an IBLOB member on match, or NULL if nothing is found.
*/
extern IBLOB *IUFindBLOB(const IBLOBVectorProperty *bvp, const char *name);

/** \brief Returns the first ON switch it finds in the vector switch property.

*   \note This is only valid for ISR_1OFMANY mode. That is, when only one switch out of many is allowed to be ON. Do not use this function if you can have multiple ON switches in the same vector property.
*
* \param sp a pointer to a switch vector property.
* \return a pointer to the \e first ON ISwitch member if found. If all switches are off, NULL is returned.
*/
extern ISwitch *IUFindOnSwitch(const ISwitchVectorProperty *sp);

/** \brief Returns the index of the string in a string array
*
* \param needle the string to match against each element in the hay
* \param hay a pointer to a string array to search in
* \param n the size of hay
* \return index of needle if found in the hay. Otherwise -1 if not found.
*/
extern int IUFindIndex(const char *needle, char **hay, unsigned int n);

/** \brief Returns the index of first ON switch it finds in the vector switch property.

*   \note This is only valid for ISR_1OFMANY mode. That is, when only one switch out of many is allowed to be ON. Do not use this function if you can have multiple ON switches in the same vector property.
*
* \param sp a pointer to a switch vector property.
* \return index to the \e first ON ISwitch member if found. If all switches are off, -1 is returned.
*/

extern int IUFindOnSwitchIndex(const ISwitchVectorProperty *sp);

/** \brief Returns the name of the first ON switch it finds in the supplied arguments.

*   \note This is only valid for ISR_1OFMANY mode. That is, when only one switch out of many is allowed to be ON. Do not use this function if you can have multiple ON switches in the same vector property.
*   \note This is a convience function intended to be used in ISNewSwitch(...) function to find out ON switch name without having to change actual switch state via IUUpdateSwitch(..)
*
* \param states list of switch states passed by ISNewSwitch()
* \param names list of switch names passed by ISNewSwitch()
* \param n number of switches passed by ISNewSwitch()
* \return name of the \e first ON ISwitch member if found. If all switches are off, NULL is returned.
*/
extern const char *IUFindOnSwitchName(ISState *states, char *names[], int n);

/** \brief Reset all switches in a switch vector property to OFF.
*
* \param svp a pointer to a switch vector property.
*/
extern void IUResetSwitch(ISwitchVectorProperty *svp);

/** \brief Update all switches in a switch vector property.
*
* \param svp a pointer to a switch vector property.
* \param states the states of the new ISwitch members.
* \param names the names of the ISwtich members to update.
* \param n the number of ISwitch members to update.
* \return 0 if update successful, -1 otherwise.
*/
extern int IUUpdateSwitch(ISwitchVectorProperty *svp, ISState *states, char *names[], int n);

/** \brief Update all numbers in a number vector property.
*
* \param nvp a pointer to a number vector property.
* \param values the states of the new INumber members.
* \param names the names of the INumber members to update.
* \param n the number of INumber members to update.
* \return 0 if update successful, -1 otherwise. Update will fail if values are out of scope, or in case of property name mismatch.
*/
extern int IUUpdateNumber(INumberVectorProperty *nvp, double values[], char *names[], int n);

/** \brief Update all text members in a text vector property.
*
* \param tvp a pointer to a text vector property.
* \param texts a pointer to the text members
* \param names the names of the IText members to update.
* \param n the number of IText members to update.
* \return 0 if update successful, -1 otherwise. Update will fail in case of property name mismatch.
*/
extern int IUUpdateText(ITextVectorProperty *tvp, char *texts[], char *names[], int n);

/** \brief Update all BLOB members in a BLOB vector property.
*
* \param bvp a pointer to a BLOB vector property.
* \param sizes sizes of the blobs.
* \param blobsizes size of the blobs, raw without compression.
* \param blobs a pointer to the BLOB members
* \param names the names of the IBLOB members to update.
* \param formats The blob format or extension.
* \param n the number of IBLOB members to update.
* \return 0 if update successful, -1 otherwise. Update will fail in case of property name mismatch.
*/
extern int IUUpdateBLOB(IBLOBVectorProperty *bvp, int sizes[], int blobsizes[], char *blobs[], char *formats[],
                        char *names[], int n);

/** \brief Function to save blob metadata in the corresponding blob.
    \param bp pointer to an IBLOB member.
    \param size size of the blob buffer encoded in base64
    \param blobsize actual size of the buffer after base64 decoding. This is the actual byte count used in drivers.
    \param blob pointer to the blob buffer
    \param format format of the blob buffer
    \note Do not call this function directly, it is called internally by IUUpdateBLOB.
    */
extern int IUSaveBLOB(IBLOB *bp, int size, int blobsize, char *blob, char *format);

/** \brief Function to update the min and max elements of a number in the client
    \param nvp pointer to an INumberVectorProperty.
    \warning This call is not INDI protocol compliant. It sends setNumberVector along with updated Min/Max/Step values so that the client
    updates the range accordingly for this property. In the INDI-compliant paradigm, it is NOT possible to update min/max/step step of an existing number
    property and the only way is to do so is to delete and redefine the number property again. However, due to the many problems with approach in device drivers,
    INDI Library defines this function to simplify the update process without requiring a complete delete/define cycle.
 */
extern void IUUpdateMinMax(const INumberVectorProperty *nvp);

/** \brief Function to reliably save new text in a IText.
    \param tp pointer to an IText member.
    \param newtext the new text to be saved
*/
extern void IUSaveText(IText *tp, const char *newtext);

/** \brief Assign attributes for a switch property. The switch's auxiliary elements will be set to NULL.
    \param sp pointer a switch property to fill
    \param name the switch name
    \param label the switch label
    \param s the switch state (ISS_ON or ISS_OFF)
*/
extern void IUFillSwitch(ISwitch *sp, const char *name, const char *label, ISState s);

/** \brief Assign attributes for a light property. The light's auxiliary elements will be set to NULL.
    \param lp pointer a light property to fill
    \param name the light name
    \param label the light label
    \param s the light state (IDLE, WARNING, OK, ALERT)
*/
extern void IUFillLight(ILight *lp, const char *name, const char *label, IPState s);

/** \brief Assign attributes for a number property. The number's auxiliary elements will be set to NULL.
    \param np pointer a number property to fill
    \param name the number name
    \param label the number label
    \param format the number format in printf style (e.g. "%02d")
    \param min  the minimum possible value
    \param max the maximum possible value
    \param step the step used to climb from minimum value to maximum value
    \param value the number's current value
*/
extern void IUFillNumber(INumber *np, const char *name, const char *label, const char *format, double min, double max,
                         double step, double value);

/** \brief Assign attributes for a text property. The text's auxiliary elements will be set to NULL.
    \param tp pointer a text property to fill
    \param name the text name
    \param label the text label
    \param initialText the initial text
*/
extern void IUFillText(IText *tp, const char *name, const char *label, const char *initialText);

/** \brief Assign attributes for a BLOB property. The BLOB's data and auxiliary elements will be set to NULL.
    \param bp pointer a BLOB property to fill
    \param name the BLOB name
    \param label the BLOB label
    \param format the BLOB format.
*/
extern void IUFillBLOB(IBLOB *bp, const char *name, const char *label, const char *format);

/** \brief Assign attributes for a switch vector property. The vector's auxiliary elements will be set to NULL.
    \param svp pointer a switch vector property to fill
    \param sp pointer to an array of switches
    \param nsp the dimension of sp
    \param dev the device name this vector property belongs to
    \param name the vector property name
    \param label the vector property label
    \param group the vector property group
    \param p the vector property permission
    \param r the switches behavior
    \param timeout vector property timeout in seconds
    \param s the vector property initial state.
*/
extern void IUFillSwitchVector(ISwitchVectorProperty *svp, ISwitch *sp, int nsp, const char *dev, const char *name,
                               const char *label, const char *group, IPerm p, ISRule r, double timeout, IPState s);

/** \brief Assign attributes for a light vector property. The vector's auxiliary elements will be set to NULL.
    \param lvp pointer a light vector property to fill
    \param lp pointer to an array of lights
    \param nlp the dimension of lp
    \param dev the device name this vector property belongs to
    \param name the vector property name
    \param label the vector property label
    \param group the vector property group
    \param s the vector property initial state.
*/
extern void IUFillLightVector(ILightVectorProperty *lvp, ILight *lp, int nlp, const char *dev, const char *name,
                              const char *label, const char *group, IPState s);

/** \brief Assign attributes for a number vector property. The vector's auxiliary elements will be set to NULL.
    \param nvp pointer a number vector property to fill
    \param np pointer to an array of numbers
    \param nnp the dimension of np
    \param dev the device name this vector property belongs to
    \param name the vector property name
    \param label the vector property label
    \param group the vector property group
    \param p the vector property permission
    \param timeout vector property timeout in seconds
    \param s the vector property initial state.
*/
extern void IUFillNumberVector(INumberVectorProperty *nvp, INumber *np, int nnp, const char *dev, const char *name,
                               const char *label, const char *group, IPerm p, double timeout, IPState s);

/** \brief Assign attributes for a text vector property. The vector's auxiliary elements will be set to NULL.
    \param tvp pointer a text vector property to fill
    \param tp pointer to an array of texts
    \param ntp the dimension of tp
    \param dev the device name this vector property belongs to
    \param name the vector property name
    \param label the vector property label
    \param group the vector property group
    \param p the vector property permission
    \param timeout vector property timeout in seconds
    \param s the vector property initial state.
*/
extern void IUFillTextVector(ITextVectorProperty *tvp, IText *tp, int ntp, const char *dev, const char *name,
                             const char *label, const char *group, IPerm p, double timeout, IPState s);

/** \brief Assign attributes for a BLOB vector property. The vector's auxiliary elements will be set to NULL.
    \param bvp pointer a BLOB vector property to fill
    \param bp pointer to an array of BLOBs
    \param nbp the dimension of bp
    \param dev the device name this vector property belongs to
    \param name the vector property name
    \param label the vector property label
    \param group the vector property group
    \param p the vector property permission
    \param timeout vector property timeout in seconds
    \param s the vector property initial state.
*/
extern void IUFillBLOBVector(IBLOBVectorProperty *bvp, IBLOB *bp, int nbp, const char *dev, const char *name,
                             const char *label, const char *group, IPerm p, double timeout, IPState s);

/** \brief Update a snooped number vector property from the given XML root element.
    \param root XML root elememnt containing the snopped property content
    \param nvp a pointer to the number vector property to be updated.
    \return 0 if cracking the XML element and updating the property proceeded without errors, -1 if trouble.
*/
extern int IUSnoopNumber(XMLEle *root, INumberVectorProperty *nvp);

/** \brief Update a snooped text vector property from the given XML root element.
    \param root XML root elememnt containing the snopped property content
    \param tvp a pointer to the text vector property to be updated.
    \return 0 if cracking the XML element and updating the property proceeded without errors, -1 if trouble.
*/
extern int IUSnoopText(XMLEle *root, ITextVectorProperty *tvp);

/** \brief Update a snooped light vector property from the given XML root element.
    \param root XML root elememnt containing the snopped property content
    \param lvp a pointer to the light vector property to be updated.
    \return 0 if cracking the XML element and updating the property proceeded without errors, -1 if trouble.
*/
extern int IUSnoopLight(XMLEle *root, ILightVectorProperty *lvp);

/** \brief Update a snooped switch vector property from the given XML root element.
    \param root XML root elememnt containing the snopped property content
    \param svp a pointer to the switch vector property to be updated.
    \return 0 if cracking the XML element and updating the property proceeded without errors, -1 if trouble.
*/
extern int IUSnoopSwitch(XMLEle *root, ISwitchVectorProperty *svp);

/** \brief Update a snooped BLOB vector property from the given XML root element.
    \param root XML root elememnt containing the snopped property content
    \param bvp a pointer to the BLOB vector property to be updated.
    \return 0 if cracking the XML element and updating the property proceeded without errors, -1 if trouble.
*/
extern int IUSnoopBLOB(XMLEle *root, IBLOBVectorProperty *bvp);

/*@}*/

/*******************************************************************************
 *******************************************************************************
 *
 *   Functions the INDI Device Driver framework calls which the Driver must
 *   define.
 *
 *******************************************************************************
 *******************************************************************************
 */

/**
 * \defgroup dcuFunctions IS Functions: Functions all drivers must define.

This section defines functions that must be defined in each driver. These
functions are never called by the driver, but are called by the driver
framework. These must always be defined even if they do nothing.
*/
/*@{*/

/** \brief Get Device Properties
    \param dev the name of the device.

This function is called by the framework whenever the driver has received a
getProperties message from an INDI client. The argument \param dev is either a
string containing the name of the device specified within the message, or NULL
if no device was specified. If the driver does not recognize the device, it
should ignore the message and do nothing. If dev matches the device the driver
is implementing, or dev is NULL, the driver must respond by sending one defXXX
message to describe each property defined by this device, including its
current (or initial) value. The recommended way to send these messages is to
call the appropriate IDDef functions.
*/
extern void ISGetProperties(const char *dev);

/** \brief Update the value of an existing text vector property.
    \param dev the name of the device.
    \param name the name of the text vector property to update.
    \param texts an array of text values.
    \param names parallel names to the array of text values.
    \param n the dimension of texts[].
    \note You do not need to call this function, it is called by INDI when new text values arrive from the client.
*/
extern void ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n);

/** \brief Update the value of an existing number vector property.
    \param dev the name of the device.
    \param name the name of the number vector property to update.
    \param values an array of number values.
    \param names parallel names to the array of number values.
    \param n the dimension of doubles[].
    \note You do not need to call this function, it is called by INDI when new number values arrive from the client.
*/
extern void ISNewNumber(const char *dev, const char *name, double *values, char *names[], int n);

/** \brief Update the value of an existing switch vector property.
    \param dev the name of the device.
    \param name the name of the switch vector property to update.
    \param states an array of switch states.
    \param names parallel names to the array of switch states.
    \param n the dimension of states[].
    \note You do not need to call this function, it is called by INDI when new switch values arrive from the client.
*/
extern void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n);

/** \brief Update data of an existing blob vector property.
    \param dev the name of the device.
    \param name the name of the blob vector property to update.
    \param sizes an array of base64 blob sizes in bytes \e before decoding.
    \param blobsizes an array of the sizes of blobs \e after decoding from base64.
    \param blobs an array of decoded data. Each blob size is found in \e blobsizes array.
    \param formats Blob data format (e.g. fits.z).
    \param names names of blob members to update.
    \param n the number of blobs to update.
    \note You do not need to call this function, it is called by INDI when new blob values arrive from the client.
          e.g. BLOB element with name names[0] has data located in blobs[0] with size sizes[0] and format formats[0].
*/

extern void ISNewBLOB(const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[], char *formats[],
                      char *names[], int n);

/** \brief Function defined by Drivers that is called when another Driver it is snooping (by having previously called IDSnoopDevice()) sent any INDI message.
    \param root The argument contains the full message exactly as it was sent by the driver.
    \e Hint: use the IUSnoopXXX utility functions to help crack the message if it was one of setXXX or defXXX.
*/
extern void ISSnoopDevice(XMLEle *root);

/*@}*/

/* Handy readability macro to avoid unused variables warnings */
#define INDI_UNUSED(x) (void)x

/** \brief Extract dev and name attributes from an XML element.
    \param root The XML element to be parsed.
    \param dev pointer to an allocated buffer to save the extracted element device name attribute.
           The buffer size must be at least MAXINDIDEVICE bytes.
    \param name pointer to an allocated buffer to save the extracted elemented name attribute.
           The buffer size must be at least MAXINDINAME bytes.
    \param msg pointer to an allocated char buffer to store error messages. The minimum buffer size is MAXRBUF.
    \return 0 if successful, -1 if error is encountered and msg is set.
*/
extern int crackDN(XMLEle *root, char **dev, char **name, char msg[]);

/** \brief Extract property state (Idle, OK, Busy, Alert) from the supplied string.
    \param str A string representation of the state.
    \param ip Pointer to IPState structure to store the extracted property state.
    \return 0 if successful, -1 if error is encountered.
*/
extern int crackIPState(const char *str, IPState *ip);

/** \brief Extract switch state (On or Off) from the supplied string.
    \param str A string representation of the switch state.
    \param ip Pointer to ISState structure to store the extracted switch state.
    \return 0 if successful, -1 if error is encountered.
*/
extern int crackISState(const char *str, ISState *ip);

/** \brief Extract property permission state (RW, RO, WO) from the supplied string.
    \param str A string representation of the permission state.
    \param ip Pointer to IPerm structure to store the extracted permission state.
    \return 0 if successful, -1 if error is encountered.
*/
extern int crackIPerm(const char *str, IPerm *ip);

/** \brief Extract switch rule (OneOfMany, OnlyOne..etc) from the supplied string.
    \param str A string representation of the switch rule.
    \param ip Pointer to ISRule structure to store the extracted switch rule.
    \return 0 if successful, -1 if error is encountered.
*/
extern int crackISRule(const char *str, ISRule *ip);

/** \return Returns a string representation of the supplied property state. */
extern const char *pstateStr(IPState s);
/** \return Returns a string representation of the supplied switch status. */
extern const char *sstateStr(ISState s);
/** \return Returns a string representation of the supplied switch rule. */
extern const char *ruleStr(ISRule r);
/** \return Returns a string representation of the supplied permission value. */
extern const char *permStr(IPerm p);

extern void xmlv1();

#ifdef __cplusplus
}
#endif
