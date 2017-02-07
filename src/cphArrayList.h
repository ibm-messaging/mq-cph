/*<copyright notice="lm-source" pids="" years="2007,2017">*/
/*******************************************************************************
 * Copyright (c) 2007,2017 IBM Corp.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/*
 * Contributors:
 *    Jerry Stevens - Initial implementation
 *    Various members of the WebSphere MQ Performance Team at IBM Hursley UK
 *******************************************************************************/
/*</copyright>*/
/*******************************************************************************/
/*                                                                             */
/* Performance Harness for IBM MQ C-MQI interface                              */
/*                                                                             */
/*******************************************************************************/

#ifndef _CPHARRAYLIST
#define _CPHARRAYLIST

/*typedef struct _cphlistiterator CPH_LISTITERATOR; */

/* Following syntax doesn't work on Linux so have to declare as void */
/* typedef struct _cpharraylistitem CPH_ARRAYLISTITEM; */

/* CPH_ARRAYLISTITEM is a struct to define an individual item in the list and to point to the
   next entry */
typedef struct _cpharraylistitem {
    void *item;               /* The address of the item */

    /* CPH_ARRAYLISTITEM *next; */
  void *next;               /* The address of the next item in the list */
} CPH_ARRAYLISTITEM;



/* The CPH_ARRAYLIST structure - this contains a linked list of CPH_ARRAYLISTITEM structures */
typedef struct _cpharraylist {
   int nEntries;              /* The number of entries in the list */
   CPH_ARRAYLISTITEM *items;  /* The address of the first entry in the list */

} CPH_ARRAYLIST;

/* Function prototypes */
void cphArrayListIni(CPH_ARRAYLIST **ppArrayList);
int  cphArrayListFree(CPH_ARRAYLIST **ppArrayList);
int  cphArrayListAdd(CPH_ARRAYLIST *pArrayList, void *anItem);
int  cphArrayListSize(CPH_ARRAYLIST *pArrayList);
int  cphArrayListIndexOf(CPH_ARRAYLIST *pArrayList, void *anItem);
void *cphArrayListGet(CPH_ARRAYLIST *pArrayList, int index);
int  cphArrayListIsEmpty(CPH_ARRAYLIST *pArrayList);

#endif
