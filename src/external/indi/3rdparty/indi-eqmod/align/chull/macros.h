/*====================================================================
    macros.h
 
 	macros used to access data structures and perform quick tests.

  ====================================================================*/

#pragma once

#include <stdlib.h>

/* general-purpose macros */
#define SWAP(t, x, y) \
    {                 \
        t = x;        \
        x = y;        \
        y = t;        \
    }

/*char *malloc();*/

#define NEW(p, type)                                \
    if ((p = (type *)malloc(sizeof(type))) == NULL) \
    {                                               \
        printf("Out of Memory!\n");                 \
        exit(0);                                    \
    }

#define FREE(p)          \
    if (p)               \
    {                    \
        free((char *)p); \
        p = NULL;        \
    }

#define ADD(head, p)                 \
    if (head)                        \
    {                                \
        p->next       = head;        \
        p->prev       = head->prev;  \
        head->prev    = p;           \
        p->prev->next = p;           \
    }                                \
    else                             \
    {                                \
        head       = p;              \
        head->next = head->prev = p; \
    }

#define DELETE(head, p)          \
    if (head)                    \
    {                            \
        if (head == head->next)  \
            head = NULL;         \
        else if (p == head)      \
            head = head->next;   \
        p->next->prev = p->prev; \
        p->prev->next = p->next; \
        FREE(p);                 \
    }
