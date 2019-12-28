
/*
 * $Revision: 1.1 $
 * $Id: queue.h,v 1.1 2000/02/23 00:51:25 bobby Exp bobby $
 */
#include <sys/mman.h>

#ifndef _QUEUE_H
#define _QUEUE_H

#define getmem malloc

/* Generic doubly linked, circular queue --------------------------------*/
typedef struct GenericDStruct {
  struct GenericDStruct * next ;
  struct GenericDStruct * prev ;
} GenericDList ; /* Generic List
		  * used for list manipulation functions. 
		  * All lists should
		  * have 'next' as their first field 
		  */
		
/**** Some list manipulation macros. Next, Insert, Delete ****/
#define InitDQ(head,type) { \
    head =(type *) getmem (sizeof (type)) ;\
    head->next = head->prev = head ;\
}

#define NextDQ(ptr)(ptr->next)

#define InsertDQ(pos, element){ \
    element->next = pos->next; \
    pos->next->prev = element; \
    pos->next = element ; \
    element->prev = pos ;\
}

/* DOESN'T FREE */
#define DelNextDQ(pos){\
    pos->next->next->prev = pos;\
    pos->next = pos->next->next;\
}


#define EmptyDQ(h)(h->next==h)
#define DelDQ(pos){ \
    pos->prev->next = pos->next;\
    pos->next->prev = pos->prev;\
}

/* Generic singly linked (non circular) queue ----------------------------*/
typedef struct GenericStruct {
  struct GenericStruct * next ;
} GenericList ; /* Generic List
		 * used for list manipulation functions. 
		 * All lists should
		 * have 'next' as their first field 
		 */
		


/**** Some list manipulation macros. Next, Insert, Delete ****/
#define InitQ(head,type) { \
    head =(type *) getmem (sizeof (type)) ;\
    head->next = NULL ;\
}

#define NextQ(ptr)(ptr->next)

#define InsertQ(pos, element){ \
    element->next = pos->next; \
    pos->next = element ; \
}

/* DOESN'T FREE */
#define DelNextQ(pos){\
    pos->next = pos->next->next;\
}


#define EmptyQ(h)(h->next==NULL)

#endif

