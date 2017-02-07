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

#ifndef _CPHSTRINGBUFFER
#define _CPHSTRINGBUFFER

/* The CPH_STRINGBUFFER structure */
typedef struct _cphstringbuffer {
    char *sb;         /* Pointer to a character string containing the data in the buffer */
} CPH_STRINGBUFFER;

/* Function prototypes */
void cphStringBufferIni(CPH_STRINGBUFFER **ppStringBuffer);
int cphStringBufferFree(CPH_STRINGBUFFER **ppStringBuffer);
int cphStringBufferAppend(CPH_STRINGBUFFER *pStringBuffer, char const * const aString);
int cphStringBufferAppendInt(CPH_STRINGBUFFER *pStringBuffer, int anInt);
int cphStringBufferAppendDouble(CPH_STRINGBUFFER *pStringBuffer, double aDouble);
int cphStringBufferSetLength(CPH_STRINGBUFFER *pStringBuffer, unsigned int newLength);
int cphStringBufferGetLength(CPH_STRINGBUFFER *pStringBuffer);
char *cphStringBufferToString(CPH_STRINGBUFFER *pStringBuffer);

#endif
