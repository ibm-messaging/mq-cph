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

/* These methods mimick the Java ListIterator class and are used to traverse a CPH_ARRAYLIST
   array. The cphListIteratorNext method is used to access this array in order.
*/

#include "cphUtil.h"
#include "cphArrayList.h"
#include "cphListIterator.h"
#include <stdio.h>
#include <stdlib.h>
/*
** Method: cphListIteratorIni
**
** Initialise a new CPH_LISTITERATOR from a CPH_ARRAYLIST structure
**
** Input Parameters: arrayList - the CPH_ARRAYLIST array which is to be traversed
**
** Output Parameters: ppListIterator - double pointer to the new CPH_LISTITERATOR structure
**
*/
void cphListIteratorIni(CPH_LISTITERATOR **ppListIterator, CPH_ARRAYLIST *arrayList) {
   CPH_LISTITERATOR *pListIterator;

   pListIterator = malloc(sizeof(CPH_LISTITERATOR));
   if (NULL != pListIterator) {
      pListIterator->listStart = arrayList->items;
    pListIterator->currentItem  = NULL;
   }

   *ppListIterator = pListIterator;
}

/*
** Method: cphListIteratorFree
**
** Dispose of a CPH_LISTERATOR structure. On successful deletion, the pointer to the
** structure is set to NULL.
**
** Input Parameters: ppListIterator - double pointer to the CPH_LISTITERATOR
**
** Returns: CPHTRUE on successful exection, CPHFALSE otherwise
**
*/
int cphListIteratorFree(CPH_LISTITERATOR **ppListIterator) {
  if (NULL != *ppListIterator) {
    free(*ppListIterator);
    *ppListIterator = NULL;
    return(CPHTRUE);
  }
  return(CPHFALSE);
}

/*
** Method: cphListIteratorHasNext
**
** This function is used to determine whether there are any more entries in the CPH_ARRAYLIST that
** have not yet been read with cphListIteratorNext.
**
** Input Parameters: pListIterator - pointer to the CPH_LISTITERATOR structure
**
** Returns: CPHTRUE if there are further entries to be traversed and CPHFALSE if not.
**
*/
int cphListIteratorHasNext(CPH_LISTITERATOR *pListIterator) {
  int status = CPHFALSE;

  if (NULL == pListIterator->currentItem) {
        if (pListIterator->listStart != NULL)
            status = CPHTRUE;
  } else {
     if (NULL != pListIterator->currentItem->next) status = CPHTRUE;
  }

  return(status);
}

/*
** Method: cphListIteratorNext
**
** Retrive the next item from the CPH_ARRAYLIST. The address of the array item is returned.
**
** Input Parameters: pListIterator - pointer to the CPH_LISTITERATOR structure
**
** Returns: The address of the retrived item, or NULL if there were no more items to retrieve.
**
*/
CPH_ARRAYLISTITEM* cphListIteratorNext(CPH_LISTITERATOR *pListIterator) {
  CPH_ARRAYLISTITEM* pArrayListItem = NULL;

  if (NULL == pListIterator->currentItem) {
        if (NULL != pListIterator->listStart) {
       pListIterator->currentItem = pListIterator->listStart;
       pArrayListItem = pListIterator->currentItem;
        }
  } else {
    if (NULL != pListIterator->currentItem->next) {
       pListIterator->currentItem = pListIterator->currentItem->next;
         pArrayListItem = pListIterator->currentItem;
    }
  }
  return pArrayListItem;
}

/*
** Method: cphArrayListListIterator
**
** This method is used to create a new CPH_LISTITERATOR from a CPH_ARRAYLIST.
**
** Input Parameters: pLArrayList - pointer to a CPH_ARRAYLIST structure
**
** Returns: The address of the new CPH_LISTITERATOR or NULL if it could not
**          be created.
**
*/
CPH_LISTITERATOR* cphArrayListListIterator(CPH_ARRAYLIST *pArrayList) {
  CPH_LISTITERATOR *pListIterator;

  cphListIteratorIni(&pListIterator, pArrayList);

  return pListIterator;

}
