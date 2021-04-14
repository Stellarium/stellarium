#if 0
liblilxml
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

/** \file lilxml.h
    \brief A little DOM-style library to handle parsing and processing an XML file.

    It only handles elements, attributes and pcdata content. <! ... > and <? ... > are silently ignored. pcdata is collected into one string, sans leading whitespace first line. \n

    The following is an example of a cannonical usage for the lilxml library. Initialize a lil xml context and read an XML file in a root element.

    \code

    #include <lilxml.h>

    LilXML *lp = newLilXML();
	char errmsg[1024];
	XMLEle *root, *ep;
	int c;

	while ((c = fgetc(stdin)) != EOF) {
	    root = readXMLEle (lp, c, errmsg);
	    if (root)
		break;
	    if (errmsg[0])
		error ("Error: %s\n", errmsg);
	}

        // print the tag and pcdata content of each child element within the root

        for (ep = nextXMLEle (root, 1); ep != NULL; ep = nextXMLEle (root, 0))
	    printf ("%s: %s\n", tagXMLEle(ep), pcdataXMLEle(ep));


	// finished with root element and with lil xml context

	delXMLEle (root);
	delLilXML (lp);

     \endcode

 */

#pragma once

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* opaque handle types */
typedef struct xml_att_ XMLAtt;
typedef struct xml_ele_ XMLEle;
typedef struct LilXML_ LilXML;

/**
 * \defgroup lilxmlFunctions XML Functions: Functions to parse, process, and search XML.
 */
/*@{*/

/* creation and destruction functions */

/** \brief Create a new lilxml parser.
    \return a pointer to the lilxml parser on success. NULL on failure.
*/
extern LilXML *newLilXML();

/** \brief Delete a lilxml parser.
    \param lp a pointer to a lilxml parser to be deleted.
*/
extern void delLilXML(LilXML *lp);

/**
 * @brief delXMLEle Delete XML element.
 * @param e Pointer to XML element to delete. If nullptr, no action is taken.
 */
extern void delXMLEle(XMLEle *e);

/** \brief Process an XML chunk.
    \param lp a pointer to a lilxml parser.
    \param buf buffer to process.
    \param size size of buf
    \param errmsg a buffer to store error messages if an error in parsing is encountered.
    \return return a pointer to a NULL terminated array of parsed XML elements. An array of size 1 with on a NULL element means there is nothing to parse or a parsing is still in progress. A NULL pointer may be returned if a parsing error occurs. Check errmsg for errors if NULL is returned.
 */
extern XMLEle **parseXMLChunk(LilXML *lp, char *buf, int size, char errmsg[]);

/** \brief Process an XML one char at a time.
  \param lp a pointer to a lilxml parser.
  \param c one character to process.
  \param errmsg a buffer to store error messages if an error in parsing is encounterd.
  \return When the function parses a complete valid XML element, it will return a pointer to the XML element. A NULL is returned when parsing the element is still in progress, or if a parsing error occurs. Check errmsg for errors if NULL is returned.
*/
extern XMLEle *readXMLEle(LilXML *lp, int c, char errmsg[]);

/* search functions */
/** \brief Find an XML attribute within an XML element.
    \param e a pointer to the XML element to search.
    \param name the attribute name to search for.
    \return A pointer to the XML attribute if found or NULL on failure.
*/
extern XMLAtt *findXMLAtt(XMLEle *e, const char *name);

/** \brief Find an XML element within an XML element.
    \param e a pointer to the XML element to search.
    \param tag the element tag to search for.
    \return A pointer to the XML element if found or NULL on failure.
*/
extern XMLEle *findXMLEle(XMLEle *e, const char *tag);

/* iteration functions */
/** \brief Iterate an XML element for a list of nesetd XML elements.
    \param ep a pointer to the XML element to iterate.
    \param first the index of the starting XML element. Pass 1 to start iteration from the beginning of the XML element. Pass 0 to get the next element thereater.
    \return On success, a pointer to the next XML element is returned. NULL when there are no more elements.
*/
extern XMLEle *nextXMLEle(XMLEle *ep, int first);

/** \brief Iterate an XML element for a list of XML attributes.
    \param ep a pointer to the XML element to iterate.
    \param first the index of the starting XML attribute. Pass 1 to start iteration from the beginning of the XML element. Pass 0 to get the next attribute thereater.
    \return On success, a pointer to the next XML attribute is returned. NULL when there are no more attributes.
*/
extern XMLAtt *nextXMLAtt(XMLEle *ep, int first);

/* tree functions */
/** \brief Return the parent of an XML element.
    \return a pointer to the XML element parent.
*/
extern XMLEle *parentXMLEle(XMLEle *ep);

/** \brief Return the parent of an XML attribute.
    \return a pointer to the XML element parent.
*/
extern XMLEle *parentXMLAtt(XMLAtt *ap);

/* access functions */
/** \brief Return the tag of an XML element.
    \param ep a pointer to an XML element.
    \return the tag string.
*/
extern char *tagXMLEle(XMLEle *ep);

/** \brief Return the pcdata of an XML element.
    \param ep a pointer to an XML element.
    \return the pcdata string on success.
*/
extern char *pcdataXMLEle(XMLEle *ep);

/** \brief Return the name of an XML attribute.
    \param ap a pointer to an XML attribute.
    \return the name string of the attribute.
*/
extern char *nameXMLAtt(XMLAtt *ap);

/** \brief Return the value of an XML attribute.
    \param ap a pointer to an XML attribute.
    \return the value string of the attribute.
*/
extern char *valuXMLAtt(XMLAtt *ap);

/** \brief Return the number of characters in pcdata in an XML element.
    \param ep a pointer to an XML element.
    \return the length of the pcdata string.
*/
extern int pcdatalenXMLEle(XMLEle *ep);

/** \brief Return the number of nested XML elements in a parent XML element.
    \param ep a pointer to an XML element.
    \return the number of nested XML elements.
*/
extern int nXMLEle(XMLEle *ep);

/** \brief Return the number of XML attributes in a parent XML element.
    \param ep a pointer to an XML element.
    \return the number of XML attributes within the XML element.
*/
extern int nXMLAtt(XMLEle *ep);

/* editing functions */
/** \brief add an element with the given tag to the given element. parent can be NULL to make a new root.
    \return if parent is NULL, a new root is returned, otherwise, parent is returned.
*/
extern XMLEle *addXMLEle(XMLEle *parent, const char *tag);

/** \brief set the pcdata of the given element
    \param ep pointer to an XML element.
    \param pcdata pcdata to set.
*/
extern void editXMLEle(XMLEle *ep, const char *pcdata);

/** \brief Add an XML attribute to an existing XML element.
    \param ep pointer to an XML element
    \param name the name of the XML attribute to add.
    \param value the value of the XML attribute to add.
*/
extern XMLAtt *addXMLAtt(XMLEle *ep, const char *name, const char *value);

/** \brief Remove an XML attribute from an XML element.
    \param ep pointer to an XML element.
    \param name the name of the XML attribute to remove
*/
extern void rmXMLAtt(XMLEle *ep, const char *name);

/** \brief change the value of an attribute to str.
*   \param ap pointer to XML attribute
*   \param str new attribute value
*/
extern void editXMLAtt(XMLAtt *ap, const char *str);

/** \brief return a string with all xml-sensitive characters within the passed string replaced with their entity sequence equivalents.
*   N.B. caller must use the returned string before calling us again.
*/
extern char *entityXML(char *str);

/* convenience functions */
/** \brief Find an XML element's attribute value.
    \param ep a pointer to an XML element.
    \param name the name of the XML attribute to retrieve its value.
    \return the value string of an XML element on success. NULL on failure.
*/
extern const char *findXMLAttValu(XMLEle *ep, const char *name);

/** \brief Handy wrapper to read one xml file.
    \param fp pointer to FILE to read.
    \param lp pointer to lilxml parser.
    \param errmsg a buffer to store error messages on failure.
    \return root element else NULL with report in errmsg[].
*/
extern XMLEle *readXMLFile(FILE *fp, LilXML *lp, char errmsg[]);

/** \brief Print an XML element.
    \param fp a pointer to FILE where the print output is directed.
    \param e the XML element to print.
    \param level the printing level, set to 0 to print the whole element.
*/
extern void prXMLEle(FILE *fp, XMLEle *e, int level);

/** \brief sample print ep to string s.
*   N.B. s must be at least as large as that reported by sprlXMLEle()+1.
*   N.B. set level = 0 on first call.
*   \return return length of resulting string (sans trailing @\0@)
*/
extern int sprXMLEle(char *s, XMLEle *ep, int level);

/** \brief return number of bytes in a string guaranteed able to hold result of sprXLMEle(ep) (sans trailing @\0@).
*   N.B. set level = 0 on first call.
*/
extern int sprlXMLEle(XMLEle *ep, int level);

/* install alternatives to malloc/realloc/free */
extern void indi_xmlMalloc(void *(*newmalloc)(size_t size), void *(*newrealloc)(void *ptr, size_t size),
                           void (*newfree)(void *ptr));

/*@}*/

#ifdef __cplusplus
}
#endif

/* examples.

        initialize a lil xml context and read an XML file in a root element

	LilXML *lp = newLilXML();
	char errmsg[1024];
	XMLEle *root, *ep;
	int c;

	while ((c = fgetc(stdin)) != EOF) {
	    root = readXMLEle (lp, c, errmsg);
	    if (root)
		break;
	    if (errmsg[0])
		error ("Error: %s\n", errmsg);
	}

        print the tag and pcdata content of each child element within the root

        for (ep = nextXMLEle (root, 1); ep != NULL; ep = nextXMLEle (root, 0))
	    printf ("%s: %s\n", tagXMLEle(ep), pcdataXMLEle(ep));


	finished with root element and with lil xml context

	delXMLEle (root);
	delLilXML (lp);
 */

/* For RCS Only -- Do Not Edit
 * @(#) $RCSfile$ $Date: 2007-09-17 16:34:48 +0300 (Mon, 17 Sep 2007) $ $Revision: 713418 $ $Name:  $
 */
