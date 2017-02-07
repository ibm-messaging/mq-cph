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

/* These functions implement a StringBuffer which mimicks the Java equivalent. This is used
   for assembling cph stats lines and error messages
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cphStringBuffer.h"
#include "cphUtil.h"

/*
** Method: cphStringBufferIni
**
** This method initialises a new CPH_STRINGBUFFER structure. It does not add any data
** to it however, that must be done with one of the append methods.
**
** Output Parameters: ppStringBuffer - double pointer to a new CPH_STRINGBUFFER
**
*/
void cphStringBufferIni(CPH_STRINGBUFFER **ppStringBuffer) {
    CPH_STRINGBUFFER *pStringBuffer;

    pStringBuffer = malloc(sizeof(CPH_STRINGBUFFER));
    if (NULL != pStringBuffer) {
        pStringBuffer->sb = NULL;
    }

    *ppStringBuffer = pStringBuffer;
}

/*
** Method: cphStringBufferFree
**
** Disposes of a CPH_STRINGBUFFER structure. On successful deletion the pointer to the
** structure is set to NULL.
**
** Input/Output Parameters: ppStringBuffer - double pointer to the CPH_STRINGBUFFER
**
** Returns: CPHTRUE on successful exection, CPHFALSE otherwise
**
*/
int cphStringBufferFree(CPH_STRINGBUFFER **ppStringBuffer) {
    if (NULL != *ppStringBuffer) {
        if (NULL != (*ppStringBuffer)->sb) free((*ppStringBuffer)->sb);
        free(*ppStringBuffer);
        *ppStringBuffer = NULL;
        return(CPHTRUE);
    }
    return(CPHFALSE);
}

/*
** Method: cphStringBufferAppend
**
** Append a character string to a CPH_STRINGBUFFER. The other append routines for integer and
** double data types convert their data to string and then also call this function for adding
** data to the structure.
**
** Input Parameters: pStringBuffer - pointer to a CPH_STRINGBUFFER structure
**                   aString - pointer to the character string to be added
**
** Returns: CPHTRUE on successful exection, CPHFALSE otherwise
**
*/
int cphStringBufferAppend(CPH_STRINGBUFFER *pStringBuffer, char const * const aString) {

    if (NULL == pStringBuffer->sb) {
        pStringBuffer->sb = malloc(1 + strlen(aString));
        if (pStringBuffer->sb == NULL) {
            printf("MEMORY ALLOCATION FAILED.\n");
            return(CPHFALSE);
        }
        strcpy(pStringBuffer->sb, aString);
    } else {
        size_t len = strlen(pStringBuffer->sb);
        void* pBuffer = realloc(pStringBuffer->sb, 1 + len + strlen(aString));
        if (NULL == pBuffer) {
            //In the event of failure, the original block is left untouched by realloc, so free it...
            free(pStringBuffer->sb);
            pStringBuffer->sb = NULL;
            printf("MEMORY ALLOCATION FAILED.\n");
            return(CPHFALSE);
        }
        pStringBuffer->sb = pBuffer;
        strcat(pStringBuffer->sb, aString);
    }

    return(CPHTRUE);
}

/*
** Method: cphStringBufferAppendInt
**
** Append an integer to a CPH_STRINBUFFER. The integer is converted to a string
** and then added to the structure with a call to cphStringBufferAppend.
**
** Input Parameters: pStringBuffer - pointer to a CPH_STRINGBUFFER structure
**                   anInt - integer value to be added
**
** Returns: CPHTRUE on successful exection, CPHFALSE otherwise
**
*/
int cphStringBufferAppendInt(CPH_STRINGBUFFER *pStringBuffer, int anInt) {
    char buff[80];

    sprintf(buff, "%d", anInt);
    return(cphStringBufferAppend(pStringBuffer, buff));
}

/*
** Method: cphStringBufferAppendDouble
**
** Append a double to a CPH_STRINBUFFER. The double is converted to a string
** and then added to the structure with a call to cphStringBufferAppend.
**
** Input Parameters: pStringBuffer - pointer to a CPH_STRINGBUFFER structure
**                   aDouble - double value to be added
**
** Returns: CPHTRUE on successful exection, CPHFALSE otherwise
**
*/
int cphStringBufferAppendDouble(CPH_STRINGBUFFER *pStringBuffer, double aDouble) {
    char buff[80];

    sprintf(buff, "%.2f", aDouble);
    return(cphStringBufferAppend(pStringBuffer, buff));
}

/*
** Method: cphStringBufferSetLength
**
** Alter the length of the allocated string in the CPH_STRINGBUFFER. This is typically
** called to set the length to zero before a CPH_STRINGBUFFER is to be reused. If the new
** length is less than the existing length, the data is truncated with a NULL. If the new
** length is greater than the existing length, the string is reallocated, copied to the
** the new buffer and padded with NULLs up to the new length.
**
** Input Parameters: pStringBuffer - pointer to a CPH_STRINGBUFFER structure
**                   newLength - the new length for the CPH_STRINGBUFFER
**
** Returns: CPHTRUE on successful exection, CPHFALSE otherwise
**
*/
int cphStringBufferSetLength(CPH_STRINGBUFFER *pStringBuffer, unsigned int newLength) {
    char *newBuff;
    unsigned int i;

    /* As the HP compiler says, this is a "pointless comparison of unsigned   */
    /* integer with zero" so I have commented it out.                         */
    /* if (newLength < 0)         */
    /*   return(CPHFALSE);        */
    /* else if (newLength == 0) { */
    if (newLength == 0) {
        if (NULL != pStringBuffer) {
            if (NULL != pStringBuffer->sb) free(pStringBuffer->sb);
            pStringBuffer->sb = malloc(1);
            if (pStringBuffer->sb == NULL) {
                printf("MEMORY ALLOCATION FAILED in cphStringBufferSetLength\n");
                return(CPHFALSE);
            }
            *(pStringBuffer->sb) = '\0';
        }
    }
    else if (newLength < strlen(pStringBuffer->sb)) {
        for (i = newLength; i < strlen(pStringBuffer->sb); i++)
            pStringBuffer->sb[i] = '\0';
    }
    else if (newLength > strlen(pStringBuffer->sb)) {
        /* Could use realloc here but doing a malloc and strncpy ensures the additional part of the
           buffer is initialised with NULL characters */
        newBuff = malloc(1 + newLength);
        if (newBuff == NULL) {
            printf("MEMORY ALLOCATION FAILED in cphStringBufferSetLength\n");
            return(CPHFALSE);
        }
        strncpy(newBuff, pStringBuffer->sb, newLength);
        newBuff[newLength]='\0';
        free(pStringBuffer->sb);
        pStringBuffer->sb = newBuff;
    }
    return(CPHTRUE);
}

/*
** Method: cphStringBufferGetLength
**
** Returns the current number of characters in the buffer
**
** Input Parameters: pStringBuffer - pointer to a CPH_STRINGBUFFER structure
**
** Returns: The number of characters in the buffer (int)
**
*/
int cphStringBufferGetLength(CPH_STRINGBUFFER *pStringBuffer) {
    if ( (pStringBuffer != NULL) && (pStringBuffer->sb != NULL) )
        return ((int) strlen(pStringBuffer->sb));

    return(0);
}

/*
** Method: cphStringBufferToString
**
** Return a CPH_STRINGBUFFER as a string value. This is typically used after
** the required data items have been added to the structure and before the
** string representation of the structure is displayed or written to file.
**
** Input Parameters: pStringBuffer - pointer to a CPH_STRINGBUFFER structure
**
** Returns: a character string representation of the data in the buffer
**
*/
char *cphStringBufferToString(CPH_STRINGBUFFER *pStringBuffer) {
    return(pStringBuffer->sb);
}
