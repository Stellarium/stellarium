#if 0
liblilxml
Copyright (C) 2003 Elwood C. Downey

This library is free software;
you can redistribute it and/or
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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

#endif

/* little DOM-style XML parser.
 * only handles elements, attributes and pcdata content.
 * <! ... > and <? ... > are silently ignored.
 * pcdata is collected into one string, sans leading whitespace first line.
 *
 * #define MAIN_TST to create standalone test program
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_MSC_VER)
#define snprintf _snprintf
#pragma warning(push)
///@todo Introduce plattform indipendent safe functions as macros to fix this
#pragma warning(disable : 4996)
#endif

#include "lilxml.h"

/* used to efficiently manage growing malloced string space */
typedef struct
{
    char *s; /* malloced memory for string */
    int sl;  /* string length, sans trailing \0 */
    int sm;  /* total malloced bytes */
} String;
#define MINMEM 64 /* starting string length */

static int oneXMLchar(LilXML *lp, int c, char ynot[]);
static void initParser(LilXML *lp);
static void pushXMLEle(LilXML *lp);
static void popXMLEle(LilXML *lp);
static void resetEndTag(LilXML *lp);
static XMLAtt *growAtt(XMLEle *e);
static XMLEle *growEle(XMLEle *pe);
static void freeAtt(XMLAtt *a);
static int isTokenChar(int start, int c);
static void growString(String *sp, int c);
static void appendString(String *sp, const char *str);
static void freeString(String *sp);
static void newString(String *sp);
static void *moremem(void *old, int n);

typedef enum {
    LOOK4START = 0, /* looking for first element start */
    LOOK4TAG,       /* looking for element tag */
    INTAG,          /* reading tag */
    LOOK4ATTRN,     /* looking for attr name, > or / */
    INATTRN,        /* reading attr name */
    LOOK4ATTRV,     /* looking for attr value */
    SAWSLASH,       /* saw / in element opening */
    INATTRV,        /* in attr value */
    ENTINATTRV,     /* in entity in attr value */
    LOOK4CON,       /* skipping leading content whitespc */
    INCON,          /* reading content */
    ENTINCON,       /* in entity in pcdata */
    SAWLTINCON,     /* saw < in content */
    LOOK4CLOSETAG,  /* looking for closing tag after < */
    INCLOSETAG      /* reading closing tag */
} State;            /* parsing states */

/* maintain state while parsing */
struct LilXML_
{
    State cs;      /* current state */
    int ln;        /* line number for diags */
    XMLEle *ce;    /* current element being built */
    String endtag; /* to check for match with opening tag*/
    String entity; /* collect entity seq */
    int delim;     /* attribute value delimiter */
    int lastc;     /* last char (just used wiht skipping)*/
    int skipping;  /* in comment or declaration */
    int inblob;    /* in oneBLOB element */
};

/* internal representation of a (possibly nested) XML element */
struct xml_ele_
{
    String tag;        /* element tag */
    XMLEle *pe;        /* parent element, or NULL if root */
    XMLAtt **at;       /* list of attributes */
    int nat;           /* number of attributes */
    int ait;           /* used to iterate over at[] */
    XMLEle **el;       /* list of child elements */
    int nel;           /* number of child elements */
    int eit;           /* used to iterate over el[] */
    String pcdata;     /* character data in this element */
    int pcdata_hasent; /* 1 if pcdata contains an entity char*/
};

/* internal representation of an attribute */
struct xml_att_
{
    String name; /* name */
    String valu; /* value */
    XMLEle *ce;  /* containing element */
};

/* characters that need escaping as "entities" in attr values and pcdata
 */
static char entities[] = "&<>'\"";

/* default memory managers, override with lilxmlMalloc() */
static void *(*mymalloc)(size_t size)             = malloc;
static void *(*myrealloc)(void *ptr, size_t size) = realloc;
static void (*myfree)(void *ptr)                  = free;

/* install new version of malloc/realloc/free.
 * N.B. don't call after first use of any other lilxml function
 */
void lilxmlMalloc(void *(*newmalloc)(size_t size), void *(*newrealloc)(void *ptr, size_t size),
                  void (*newfree)(void *ptr))
{
    mymalloc  = newmalloc;
    myrealloc = newrealloc;
    myfree    = newfree;
}

/* pass back a fresh handle for use with our other functions */
LilXML *newLilXML()
{
    LilXML *lp = (LilXML *)moremem(NULL, sizeof(LilXML));
    memset(lp, 0, sizeof(LilXML));
    initParser(lp);
    return (lp);
}

/* discard */
void delLilXML(LilXML *lp)
{
    delXMLEle(lp->ce);
    freeString(&lp->endtag);
    (*myfree)(lp);
}

/* delete ep and all its children and remove from parent's list if known */
void delXMLEle(XMLEle *ep)
{
    int i;

    /* benign if NULL */
    if (!ep)
        return;

    /* delete all parts of ep */
    freeString(&ep->tag);
    freeString(&ep->pcdata);
    if (ep->at)
    {
        for (i = 0; i < ep->nat; i++)
            freeAtt(ep->at[i]);
        (*myfree)(ep->at);
    }
    if (ep->el)
    {
        for (i = 0; i < ep->nel; i++)
        {
            /* forget parent so deleting doesn't modify _this_ el[] */
            ep->el[i]->pe = NULL;

            delXMLEle(ep->el[i]);
        }
        (*myfree)(ep->el);
    }

    /* remove from parent's list if known */
    if (ep->pe)
    {
        XMLEle *pe = ep->pe;
        for (i = 0; i < pe->nel; i++)
        {
            if (pe->el[i] == ep)
            {
                memmove(&pe->el[i], &pe->el[i + 1], (--pe->nel - i) * sizeof(XMLEle *));
                break;
            }
        }
    }

    /* delete ep itself */
    (*myfree)(ep);
}

//#define WITH_MEMCHR
XMLEle **parseXMLChunk(LilXML *lp, char *buf, int size, char ynot[])
{
    XMLEle **nodes = (XMLEle **)malloc(sizeof(XMLEle *));
    int nnodes     = 1;
    *nodes         = NULL;
    char *curr     = buf;
    int s;
    ynot[0] = '\0';

    if (lp->inblob)
    {
#ifdef WITH_ENCLEN
        if (size < lp->ce->pcdata.sm - lp->ce->pcdata.sl)
        {
            memcpy((void *)(lp->ce->pcdata.s + lp->ce->pcdata.sl), (const void *)buf, size);
            lp->ce->pcdata.sl += size;
            return nodes;
        }
        else
            lp->inblob = 0;
#endif
#ifdef WITH_MEMCHR
        char *ltpos = memchr(buf, '<', size);
        if (!ltpos)
        {
            lp->ce->pcdata.s = (char *)moremem(lp->ce->pcdata.s, lp->ce->pcdata.sm + size);
            lp->ce->pcdata.sm += size;
            memcpy((void *)(lp->ce->pcdata.s + lp->ce->pcdata.sl), (const void *)buf, size);
            lp->ce->pcdata.sl += size;
            return nodes;
        }
        else
            lp->inblob = 0;
#endif
    }
    else
    {
        if (lp->ce)
        {
            char *ctag = tagXMLEle(lp->ce);
            if (ctag && !(strcmp(ctag, "oneBLOB")) && (lp->cs == INCON))
            {
#ifdef WITH_ENCLEN
                XMLAtt *blenatt = findXMLAtt(lp->ce, "enclen");
                if (blenatt)
                {
                    int blen;
                    sscanf(valuXMLAtt(blenatt), "%d", &blen);
                    // if (lp->ce->pcdata.sm < blen) { // always realloc
                    if (blen % 72 != 0)
                        blen += (blen / 72) + 1; // add room for those '\n'
                    else
                        blen += (blen / 72);
                    lp->ce->pcdata.s  = (char *)moremem(lp->ce->pcdata.s, blen);
                    lp->ce->pcdata.sm = blen; // or always set sm
                    //}
                    if (size < blen - lp->ce->pcdata.sl)
                    {
                        memcpy((void *)(lp->ce->pcdata.s + lp->ce->pcdata.sl), (const void *)buf, size);
                        lp->ce->pcdata.sl += size;
                        lp->inblob = 1;
                        return nodes;
                    }
                }
#endif
#ifdef WITH_MEMCHR
                char *ltpos = memchr(buf, '<', size);
                if (!ltpos)
                {
                    lp->ce->pcdata.s = (char *)moremem(lp->ce->pcdata.s, lp->ce->pcdata.sm + size);
                    lp->ce->pcdata.sm += size;
                    memcpy((void *)(lp->ce->pcdata.s + lp->ce->pcdata.sl), (const void *)buf, size);
                    lp->ce->pcdata.sl += size;
                    lp->inblob = 1;
                    return nodes;
                }
                else
                    lp->inblob = 0;
#endif
            }
        }
    }
    while (curr - buf < size)
    {
        char newc = *curr;
        /* EOF? */
        if (newc == 0)
        {
            sprintf(ynot, "Line %d: early XML EOF", lp->ln);
            initParser(lp);
            curr++;
            continue;
        }

        /* new line? */
        if (newc == '\n')
            lp->ln++;

        /* skip comments and declarations. requires 1 char history */
        if (!lp->skipping && lp->lastc == '<' && (newc == '?' || newc == '!'))
        {
            lp->skipping = 1;
            lp->lastc    = newc;
            curr++;
            continue;
        }
        if (lp->skipping)
        {
            if (newc == '>')
                lp->skipping = 0;
            lp->lastc = newc;
            curr++;
            continue;
        }
        if (newc == '<')
        {
            lp->lastc = '<';
            curr++;
            continue;
        }

        /* do a pending '<' first then newc */
        if (lp->lastc == '<')
        {
            if (oneXMLchar(lp, '<', ynot) < 0)
            {
                initParser(lp);
                curr++;
                continue;
            }
            /* N.B. we assume '<' will never result in closure */
        }

        /* process newc (at last!) */
        s = oneXMLchar(lp, newc, ynot);
        if (s == 0)
        {
            lp->lastc = newc;
            curr++;
            continue;
        }
        if (s < 0)
        {
            initParser(lp);
            curr++;
            continue;
        }

        /* Ok! store ce in nodes and we start over.
         * N.B. up to caller to call delXMLEle with what we return.
         */
        nodes[nnodes - 1] = lp->ce;
        nodes             = (XMLEle **)realloc(nodes, (nnodes + 1) * sizeof(XMLEle *));
        nodes[nnodes]     = NULL;
        nnodes += 1;
        lp->ce = NULL;
        initParser(lp);
        curr++;
    }
    /*
     * N.B. up to caller to free nodes.
     */
    return nodes;
}

/* process one more character of an XML file.
 * when find closure with outter element return root of complete tree.
 * when find error return NULL with reason in ynot[].
 * when need more return NULL with ynot[0] = '\0'.
 * N.B. it is up to the caller to delete any tree returned with delXMLEle().
 */
XMLEle *readXMLEle(LilXML *lp, int newc, char ynot[])
{
    XMLEle *root;
    int s;

    /* start optimistic */
    ynot[0] = '\0';

    /* EOF? */
    if (newc == 0)
    {
        sprintf(ynot, "Line %d: early XML EOF", lp->ln);
        initParser(lp);
        return (NULL);
    }

    /* new line? */
    if (newc == '\n')
        lp->ln++;

    /* skip comments and declarations. requires 1 char history */
    if (!lp->skipping && lp->lastc == '<' && (newc == '?' || newc == '!'))
    {
        lp->skipping = 1;
        lp->lastc    = newc;
        return (NULL);
    }
    if (lp->skipping)
    {
        if (newc == '>')
            lp->skipping = 0;
        lp->lastc = newc;
        return (NULL);
    }
    if (newc == '<')
    {
        lp->lastc = '<';
        return (NULL);
    }

    /* do a pending '<' first then newc */
    if (lp->lastc == '<')
    {
        if (oneXMLchar(lp, '<', ynot) < 0)
        {
            initParser(lp);
            return (NULL);
        }
        /* N.B. we assume '<' will never result in closure */
    }

    /* process newc (at last!) */
    s = oneXMLchar(lp, newc, ynot);
    if (s == 0)
    {
        lp->lastc = newc;
        return (NULL);
    }
    if (s < 0)
    {
        initParser(lp);
        return (NULL);
    }

    /* Ok! return ce and we start over.
     * N.B. up to caller to call delXMLEle with what we return.
     */
    root   = lp->ce;
    lp->ce = NULL;
    initParser(lp);
    return (root);
}

/* parse the given XML string.
 * return XMLEle* else NULL with reason why in ynot[]
 */
XMLEle *parseXML(char buf[], char ynot[])
{
    LilXML *lp = newLilXML();
    XMLEle *root;

    do
    {
        root = readXMLEle(lp, *buf++, ynot);
    } while (!root && !ynot[0]);

    delLilXML(lp);

    return (root);
}

/* return a deep copy of the given XMLEle *
 */
XMLEle *cloneXMLEle(XMLEle *ep)
{
    char *buf;
    char ynot[1024];
    XMLEle *newep;

    buf = (*mymalloc)(sprlXMLEle(ep, 0) + 1);
    sprXMLEle(buf, ep, 0);
    newep = parseXML(buf, ynot);
    (*myfree)(buf);

    return (newep);
}

/* search ep for an attribute with given name.
 * return NULL if not found.
 */
XMLAtt *findXMLAtt(XMLEle *ep, const char *name)
{
    int i;

    for (i = 0; i < ep->nat; i++)
        if (!strcmp(ep->at[i]->name.s, name))
            return (ep->at[i]);
    return (NULL);
}

/* search ep for an element with given tag.
 * return NULL if not found.
 */
XMLEle *findXMLEle(XMLEle *ep, const char *tag)
{
    int tl = strlen(tag);
    int i;

    for (i = 0; i < ep->nel; i++)
    {
        String *sp = &ep->el[i]->tag;
        if (sp->sl == tl && !strcmp(sp->s, tag))
            return (ep->el[i]);
    }
    return (NULL);
}

/* iterate over each child element of ep.
 * call first time with first set to 1, then 0 from then on.
 * returns NULL when no more or err
 */
XMLEle *nextXMLEle(XMLEle *ep, int init)
{
    int eit;

    if (init)
        ep->eit = 0;

    eit = ep->eit++;
    if (eit < 0 || eit >= ep->nel)
        return (NULL);
    return (ep->el[eit]);
}

/* iterate over each attribute of ep.
 * call first time with first set to 1, then 0 from then on.
 * returns NULL when no more or err
 */
XMLAtt *nextXMLAtt(XMLEle *ep, int init)
{
    int ait;

    if (init)
        ep->ait = 0;

    ait = ep->ait++;
    if (ait < 0 || ait >= ep->nat)
        return (NULL);
    return (ep->at[ait]);
}

/* return parent of given XMLEle */
XMLEle *parentXMLEle(XMLEle *ep)
{
    return (ep->pe);
}

/* return parent element of given XMLAtt */
XMLEle *parentXMLAtt(XMLAtt *ap)
{
    return (ap->ce);
}

/* access functions */

/* return the tag name of the given element */
char *tagXMLEle(XMLEle *ep)
{
    return (ep->tag.s);
}

/* return the pcdata portion of the given element */
char *pcdataXMLEle(XMLEle *ep)
{
    return (ep->pcdata.s);
}

/* return the number of characters in the pcdata portion of the given element */
int pcdatalenXMLEle(XMLEle *ep)
{
    return (ep->pcdata.sl);
}

/* return the name of the given attribute */
char *nameXMLAtt(XMLAtt *ap)
{
    return (ap->name.s);
}

/* return the value of the given attribute */
char *valuXMLAtt(XMLAtt *ap)
{
    return (ap->valu.s);
}

/* return the number of child elements of the given element */
int nXMLEle(XMLEle *ep)
{
    return (ep->nel);
}

/* return the number of attributes in the given element */
int nXMLAtt(XMLEle *ep)
{
    return (ep->nat);
}

/* search ep for an attribute with the given name and return its value.
 * return "" if not found.
 */
const char *findXMLAttValu(XMLEle *ep, const char *name)
{
    XMLAtt *a = findXMLAtt(ep, name);
    return (a ? a->valu.s : "");
}

/* handy wrapper to read one xml file.
 * return root element else NULL with report in ynot[]
 */
XMLEle *readXMLFile(FILE *fp, LilXML *lp, char ynot[])
{
    int c;

    while ((c = fgetc(fp)) != EOF)
    {
        XMLEle *root = readXMLEle(lp, c, ynot);
        if (root || ynot[0])
            return (root);
    }

    return (NULL);
}

/* add an element with the given tag to the given element.
 * parent can be NULL to make a new root.
 */
XMLEle *addXMLEle(XMLEle *parent, const char *tag)
{
    XMLEle *ep = growEle(parent);
    appendString(&ep->tag, tag);
    return (ep);
}

/* append an existing element to the given element.
 * N.B. be mindful of when these are deleted, this is not a deep copy.
 */
void appXMLEle(XMLEle *ep, XMLEle *newep)
{
    ep->el            = (XMLEle **)moremem(ep->el, (ep->nel + 1) * sizeof(XMLEle *));
    ep->el[ep->nel++] = newep;
}

/* set the pcdata of the given element */
void editXMLEle(XMLEle *ep, const char *pcdata)
{
    freeString(&ep->pcdata);
    appendString(&ep->pcdata, pcdata);
    ep->pcdata_hasent = (strpbrk(pcdata, entities) != NULL);
}

/* add an attribute to the given XML element */
XMLAtt *addXMLAtt(XMLEle *ep, const char *name, const char *valu)
{
    XMLAtt *ap = growAtt(ep);
    appendString(&ap->name, name);
    appendString(&ap->valu, valu);
    return (ap);
}

/* remove the named attribute from ep, if any */
void rmXMLAtt(XMLEle *ep, const char *name)
{
    int i;

    for (i = 0; i < ep->nat; i++)
    {
        if (strcmp(ep->at[i]->name.s, name) == 0)
        {
            freeAtt(ep->at[i]);
            memmove(&ep->at[i], &ep->at[i + 1], (--ep->nat - i) * sizeof(XMLAtt *));
            return;
        }
    }
}

/* change the value of an attribute to str */
void editXMLAtt(XMLAtt *ap, const char *str)
{
    freeString(&ap->valu);
    appendString(&ap->valu, str);
}

/* sample print ep to fp
 * N.B. set level = 0 on first call
 */
#define PRINDENT 4 /* sample print indent each level */
void prXMLEle(FILE *fp, XMLEle *ep, int level)
{
    int indent = level * PRINDENT;
    int i;

    fprintf(fp, "%*s<%s", indent, "", ep->tag.s);
    for (i = 0; i < ep->nat; i++)
        fprintf(fp, " %s=\"%s\"", ep->at[i]->name.s, entityXML(ep->at[i]->valu.s));
    if (ep->nel > 0)
    {
        fprintf(fp, ">\n");
        for (i = 0; i < ep->nel; i++)
            prXMLEle(fp, ep->el[i], level + 1);
    }
    if (ep->pcdata.sl > 0)
    {
        if (ep->nel == 0)
            fprintf(fp, ">\n");
        if (ep->pcdata_hasent)
            fprintf(fp, "%s", entityXML(ep->pcdata.s));
        else
            fprintf(fp, "%s", ep->pcdata.s);
        if (ep->pcdata.s[ep->pcdata.sl - 1] != '\n')
            fprintf(fp, "\n");
    }
    if (ep->nel > 0 || ep->pcdata.sl > 0)
        fprintf(fp, "%*s</%s>\n", indent, "", ep->tag.s);
    else
        fprintf(fp, "/>\n");
}

/* sample print ep to string s.
 * N.B. s must be at least as large as that reported by sprlXMLEle()+1.
 * N.B. set level = 0 on first call
 * return length of resulting string (sans trailing \0)
 */
int sprXMLEle(char *s, XMLEle *ep, int level)
{
    int indent = level * PRINDENT;
    int sl     = 0;
    int i;

    sl += sprintf(s + sl, "%*s<%s", indent, "", ep->tag.s);
    for (i = 0; i < ep->nat; i++)
        sl += sprintf(s + sl, " %s=\"%s\"", ep->at[i]->name.s, entityXML(ep->at[i]->valu.s));
    if (ep->nel > 0)
    {
        sl += sprintf(s + sl, ">\n");
        for (i = 0; i < ep->nel; i++)
            sl += sprXMLEle(s + sl, ep->el[i], level + 1);
    }
    if (ep->pcdata.sl > 0)
    {
        if (ep->nel == 0)
            sl += sprintf(s + sl, ">\n");
        if (ep->pcdata_hasent)
            sl += sprintf(s + sl, "%s", entityXML(ep->pcdata.s));
        else
        {
            strcpy(s + sl, ep->pcdata.s);
            sl += ep->pcdata.sl;
        }
        if (ep->pcdata.s[ep->pcdata.sl - 1] != '\n')
            sl += sprintf(s + sl, "\n");
    }
    if (ep->nel > 0 || ep->pcdata.sl > 0)
        sl += sprintf(s + sl, "%*s</%s>\n", indent, "", ep->tag.s);
    else
        sl += sprintf(s + sl, "/>\n");

    return (sl);
}

/* return number of bytes in a string guaranteed able to hold result of
 * sprXLMEle(ep) (sans trailing \0).
 * N.B. set level = 0 on first call
 */
int sprlXMLEle(XMLEle *ep, int level)
{
    int indent = level * PRINDENT;
    int l      = 0;
    int i;

    l += indent + 1 + ep->tag.sl;
    for (i = 0; i < ep->nat; i++)
        l += ep->at[i]->name.sl + 4 + strlen(entityXML(ep->at[i]->valu.s));

    if (ep->nel > 0)
    {
        l += 2;
        for (i = 0; i < ep->nel; i++)
            l += sprlXMLEle(ep->el[i], level + 1);
    }
    if (ep->pcdata.sl > 0)
    {
        if (ep->nel == 0)
            l += 2;
        if (ep->pcdata_hasent)
            l += strlen(entityXML(ep->pcdata.s));
        else
            l += ep->pcdata.sl;
        if (ep->pcdata.s[ep->pcdata.sl - 1] != '\n')
            l += 1;
    }
    if (ep->nel > 0 || ep->pcdata.sl > 0)
        l += indent + 4 + ep->tag.sl;
    else
        l += 3;

    return (l);
}

/* return a string with all xml-sensitive characters within the passed string s
 * replaced with their entity sequence equivalents.
 * N.B. caller must use the returned string before calling us again.
 */
char *entityXML(char *s)
{
    static char *malbuf;
    int nmalbuf = 0;
    char *sret = NULL;
    char *ep = NULL;

    /* scan for each entity, if any */
    for (sret = s; (ep = strpbrk(s, entities)) != NULL; s = ep + 1)
    {
        /* found another entity, copy preceding to malloced buffer */
        int nnew = ep - s; /* all but entity itself */
        sret = malbuf = moremem(malbuf, nmalbuf + nnew + 10);
        memcpy(malbuf + nmalbuf, s, nnew);
        nmalbuf += nnew;

        /* replace with entity encoding */
        switch (*ep)
        {
            case '&':
                nmalbuf += sprintf(malbuf + nmalbuf, "&amp;");
                break;
            case '<':
                nmalbuf += sprintf(malbuf + nmalbuf, "&lt;");
                break;
            case '>':
                nmalbuf += sprintf(malbuf + nmalbuf, "&gt;");
                break;
            case '\'':
                nmalbuf += sprintf(malbuf + nmalbuf, "&apos;");
                break;
            case '"':
                nmalbuf += sprintf(malbuf + nmalbuf, "&quot;");
                break;
        }
    }

    /* return s if no entities, else malloc cleaned-up copy */
    if (sret == s)
    {
        /* using s, so free any alloced memory from last time */
        if (malbuf)
        {
            free(malbuf);
            malbuf = NULL;
        }
        return s;
    }
    else
    {
        /* put remaining part of s into malbuf */
        int nleft = strlen(s) + 1; /* include \0 */
        sret = malbuf = moremem(malbuf, nmalbuf + nleft);
        memcpy(malbuf + nmalbuf, s, nleft);
    }

    return (sret);
}

/* if ent is a recognized xml entity sequence, set *cp to char and return 1
 * else return 0
 */
static int decodeEntity(char *ent, int *cp)
{
    static struct
    {
        char *ent;
        char c;
    } enttable[] = {
        { "&amp;", '&' }, { "&apos;", '\'' }, { "&lt;", '<' }, { "&gt;", '>' }, { "&quot;", '"' },
    };
    for (int i = 0; i < (int)(sizeof(enttable) / sizeof(enttable[0])); i++)
    {
        if (strcmp(ent, enttable[i].ent) == 0)
        {
            *cp = enttable[i].c;
            return (1);
        }
    }

    return (0);
}

/* process one more char in XML file.
 * if find final closure, return 1 and tree is in ce.
 * if need more, return 0.
 * if real trouble, return -1 and put reason in ynot.
 */
static int oneXMLchar(LilXML *lp, int c, char ynot[])
{
    switch (lp->cs)
    {
        case LOOK4START: /* looking for first element start */
            if (c == '<')
            {
                pushXMLEle(lp);
                lp->cs = LOOK4TAG;
            }
            /* silently ignore until resync */
            break;

        case LOOK4TAG: /* looking for element tag */
            if (isTokenChar(1, c))
            {
                growString(&lp->ce->tag, c);
                lp->cs = INTAG;
            }
            else if (!isspace(c))
            {
                sprintf(ynot, "Line %d: Bogus tag char %c", lp->ln, c);
                return (-1);
            }
            break;

        case INTAG: /* reading tag */
            if (isTokenChar(0, c))
                growString(&lp->ce->tag, c);
            else if (c == '>')
                lp->cs = LOOK4CON;
            else if (c == '/')
                lp->cs = SAWSLASH;
            else
                lp->cs = LOOK4ATTRN;
            break;

        case LOOK4ATTRN: /* looking for attr name, > or / */
            if (c == '>')
                lp->cs = LOOK4CON;
            else if (c == '/')
                lp->cs = SAWSLASH;
            else if (isTokenChar(1, c))
            {
                XMLAtt *ap = growAtt(lp->ce);
                growString(&ap->name, c);
                lp->cs = INATTRN;
            }
            else if (!isspace(c))
            {
                sprintf(ynot, "Line %d: Bogus leading attr name char: %c", lp->ln, c);
                return (-1);
            }
            break;

        case SAWSLASH: /* saw / in element opening */
            if (c == '>')
            {
                if (!lp->ce->pe)
                    return (1); /* root has no content */
                popXMLEle(lp);
                lp->cs = LOOK4CON;
            }
            else
            {
                sprintf(ynot, "Line %d: Bogus char %c before >", lp->ln, c);
                return (-1);
            }
            break;

        case INATTRN: /* reading attr name */
            if (isTokenChar(0, c))
                growString(&lp->ce->at[lp->ce->nat - 1]->name, c);
            else if (isspace(c) || c == '=')
                lp->cs = LOOK4ATTRV;
            else
            {
                sprintf(ynot, "Line %d: Bogus attr name char: %c", lp->ln, c);
                return (-1);
            }
            break;

        case LOOK4ATTRV: /* looking for attr value */
            if (c == '\'' || c == '"')
            {
                lp->delim = c;
                lp->cs    = INATTRV;
            }
            else if (!(isspace(c) || c == '='))
            {
                sprintf(ynot, "Line %d: No value for attribute %s", lp->ln, lp->ce->at[lp->ce->nat - 1]->name.s);
                return (-1);
            }
            break;

        case INATTRV: /* in attr value */
            if (c == '&')
            {
                newString(&lp->entity);
                growString(&lp->entity, c);
                lp->cs = ENTINATTRV;
            }
            else if (c == lp->delim)
                lp->cs = LOOK4ATTRN;
            else if (!iscntrl(c))
                growString(&lp->ce->at[lp->ce->nat - 1]->valu, c);
            break;

        case ENTINATTRV: /* working on entity in attr valu */
            if (c == ';')
            {
                /* if find a recongized esp seq, add equiv char else raw seq */
                growString(&lp->entity, c);
                if (decodeEntity(lp->entity.s, &c))
                    growString(&lp->ce->at[lp->ce->nat - 1]->valu, c);
                else
                    appendString(&lp->ce->at[lp->ce->nat - 1]->valu, lp->entity.s);
                freeString(&lp->entity);
                lp->cs = INATTRV;
            }
            else
                growString(&lp->entity, c);
            break;

        case LOOK4CON: /* skipping leading content whitespace*/
            if (c == '<')
                lp->cs = SAWLTINCON;
            else if (!isspace(c))
            {
                growString(&lp->ce->pcdata, c);
                lp->cs = INCON;
            }
            break;

        case INCON: /* reading content */
            if (c == '&')
            {
                newString(&lp->entity);
                growString(&lp->entity, c);
                lp->cs = ENTINCON;
            }
            else if (c == '<')
            {
                /* chomp trailing whitespace */
                while (lp->ce->pcdata.sl > 0 && isspace(lp->ce->pcdata.s[lp->ce->pcdata.sl - 1]))
                    lp->ce->pcdata.s[--(lp->ce->pcdata.sl)] = '\0';
                lp->cs = SAWLTINCON;
            }
            else
            {
                growString(&lp->ce->pcdata, c);
            }
            break;

        case ENTINCON: /* working on entity in content */
            if (c == ';')
            {
                /* if find a recognized esc seq, add equiv char else raw seq */
                growString(&lp->entity, c);
                if (decodeEntity(lp->entity.s, &c))
                    growString(&lp->ce->pcdata, c);
                else
                {
                    appendString(&lp->ce->pcdata, lp->entity.s);
                    //lp->ce->pcdata_hasent = 1;
                }
                // JM 2018-09-26: Even if decoded, we always set
                // pcdata_hasent to 1 since we need to encode it again
                // before sending it over to clients and drivers.
                lp->ce->pcdata_hasent = 1;
                freeString(&lp->entity);
                lp->cs = INCON;
            }
            else
                growString(&lp->entity, c);
            break;

        case SAWLTINCON: /* saw < in content */
            if (c == '/')
            {
                resetEndTag(lp);
                lp->cs = LOOK4CLOSETAG;
            }
            else
            {
                pushXMLEle(lp);
                if (isTokenChar(1, c))
                {
                    growString(&lp->ce->tag, c);
                    lp->cs = INTAG;
                }
                else
                    lp->cs = LOOK4TAG;
            }
            break;

        case LOOK4CLOSETAG: /* looking for closing tag after < */
            if (isTokenChar(1, c))
            {
                growString(&lp->endtag, c);
                lp->cs = INCLOSETAG;
            }
            else if (!isspace(c))
            {
                sprintf(ynot, "Line %d: Bogus preend tag char %c", lp->ln, c);
                return (-1);
            }
            break;

        case INCLOSETAG: /* reading closing tag */
            if (isTokenChar(0, c))
                growString(&lp->endtag, c);
            else if (c == '>')
            {
                if (strcmp(lp->ce->tag.s, lp->endtag.s))
                {
                    sprintf(ynot, "Line %d: closing tag %s does not match %s", lp->ln, lp->endtag.s, lp->ce->tag.s);
                    return (-1);
                }
                else if (lp->ce->pe)
                {
                    popXMLEle(lp);
                    lp->cs = LOOK4CON; /* back to content after nested elem */
                }
                else
                    return (1); /* yes! */
            }
            else if (!isspace(c))
            {
                sprintf(ynot, "Line %d: Bogus end tag char %c", lp->ln, c);
                return (-1);
            }
            break;
    }

    return (0);
}

/* set up for a fresh start again */
static void initParser(LilXML *lp)
{
    delXMLEle(lp->ce);
    freeString(&lp->endtag);
    memset(lp, 0, sizeof(*lp));
    newString(&lp->endtag);
    lp->cs = LOOK4START;
    lp->ln = 1;
}

/* start a new XMLEle.
 * point ce to a new XMLEle.
 * if ce already set up, add to its list of child elements too.
 * endtag no longer valid.
 */
static void pushXMLEle(LilXML *lp)
{
    lp->ce = growEle(lp->ce);
    resetEndTag(lp);
}

/* point ce to parent of current ce.
 * endtag no longer valid.
 */
static void popXMLEle(LilXML *lp)
{
    lp->ce = lp->ce->pe;
    resetEndTag(lp);
}

/* return one new XMLEle, added to the given element if given */
static XMLEle *growEle(XMLEle *pe)
{
    XMLEle *newe = (XMLEle *)moremem(NULL, sizeof(XMLEle));

    memset(newe, 0, sizeof(XMLEle));
    newString(&newe->tag);
    newString(&newe->pcdata);
    newe->pe = pe;

    if (pe)
    {
        pe->el            = (XMLEle **)moremem(pe->el, (pe->nel + 1) * sizeof(XMLEle *));
        pe->el[pe->nel++] = newe;
    }

    return (newe);
}

/* add room for and return one new XMLAtt to the given element */
static XMLAtt *growAtt(XMLEle *ep)
{
    XMLAtt *newa = (XMLAtt *)moremem(NULL, sizeof(XMLAtt));

    memset(newa, 0, sizeof(*newa));
    newString(&newa->name);
    newString(&newa->valu);
    newa->ce = ep;

    ep->at            = (XMLAtt **)moremem(ep->at, (ep->nat + 1) * sizeof(XMLAtt *));
    ep->at[ep->nat++] = newa;

    return (newa);
}

/* free a and all it holds */
static void freeAtt(XMLAtt *a)
{
    if (!a)
        return;
    freeString(&a->name);
    freeString(&a->valu);
    (*myfree)(a);
}

/* reset endtag */
static void resetEndTag(LilXML *lp)
{
    freeString(&lp->endtag);
    newString(&lp->endtag);
}

/* 1 if c is a valid token character, else 0.
 * it can be alpha or '_' or numeric unless start.
 */
static int isTokenChar(int start, int c)
{
    return (isalpha(c) || c == '_' || (!start && isdigit(c)));
}

/* grow the String storage at *sp to append c */
static void growString(String *sp, int c)
{
    int l = sp->sl + 2; /* need room for '\0' plus c */

    if (l > sp->sm)
    {
        if (!sp->s)
            newString(sp);
        else
            sp->s = (char *)moremem(sp->s, sp->sm *= 2);
    }
    sp->s[--l] = '\0';
    sp->s[--l] = (char)c;
    sp->sl++;
}

/* append str to the String storage at *sp */
static void appendString(String *sp, const char *str)
{
    if (!sp || !str)
        return;

    int strl = strlen(str);
    int l    = sp->sl + strl + 1; /* need room for '\0' */

    if (l > sp->sm)
    {
        if (!sp->s)
            newString(sp);
        if (l > sp->sm)
            sp->s = (char *)moremem(sp->s, (sp->sm = l));
    }
    if (sp->s)
    {
        strcpy(&sp->s[sp->sl], str);
        sp->sl += strl;
    }
}

/* init a String with a malloced string containing just \0 */
static void newString(String *sp)
{
    if (!sp)
        return;

    sp->s  = (char *)moremem(NULL, MINMEM);
    sp->sm = MINMEM;
    *sp->s = '\0';
    sp->sl = 0;
}

/* free memory used by the given String */
static void freeString(String *sp)
{
    if (sp->s)
        (*myfree)(sp->s);
    sp->s  = NULL;
    sp->sl = 0;
    sp->sm = 0;
}

/* like malloc but knows to use realloc if already started */
static void *moremem(void *old, int n)
{
    return (old ? (*myrealloc)(old, n) : (*mymalloc)(n));
}

#if defined(MAIN_TST)
int main(int ac, char *av[])
{
    LilXML *lp = newLilXML();
    char ynot[1024];
    XMLEle *root;

    root = readXMLFile(stdin, lp, ynot);
    if (root)
    {
        char *str;
        int l;

        if (ac > 1)
        {
            XMLEle *theend = addXMLEle(root, "theend");
            editXMLEle(theend, "Added to test editing");
            addXMLAtt(theend, "hello", "world");
        }

        fprintf(stderr, "::::::::::::: %s\n", tagXMLEle(root));
        prXMLEle(stdout, root, 0);

        l   = sprlXMLEle(root, 0);
        str = malloc(l + 1);
        fprintf(stderr, "::::::::::::: %s : %d : %d", tagXMLEle(root), l, sprXMLEle(str, root, 0));
        fprintf(stderr, ": %d\n", printf("%s", str));

        delXMLEle(root);
    }
    else if (ynot[0])
    {
        fprintf(stderr, "Error: %s\n", ynot);
    }

    delLilXML(lp);

    return (0);
}
#endif

#if defined(_MSC_VER)
#undef snprintf
#pragma warning(pop)
#endif
