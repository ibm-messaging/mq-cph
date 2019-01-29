/*<copyright notice="lm-source" pids="" years="2007,2018">*/
/*******************************************************************************
 * Copyright (c) 2007,2018 IBM Corp.
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

/* This module provides a set of functions that are used to read in the contents of a
   property file and store them in a CPH_BUNDLE structure. This contains the contents of the
   property file as a linked list of name/value pairs
*/

#include <string.h>

#include "cphUtil.h"
#include "cphBundle.h"
#include "cphArrayList.h"
#include "cphListIterator.h"

/*
** Method: cphBundleLoad
**
** This function reads a property file and stores the contents in a CPH_BUNDLE structure. This contains
** each line of the property file as a name/value list.
**
** Input Parameters: pConfig - a pointer to the configuration control block
**                   moduleName - the name of the module for which we wish to read the associated property file
**
** Returns: a pointer to the CPH_BUNDLE on successful exection and NULL otherwise
**
*/
void *cphBundleLoad(CPH_CONFIG *pConfig, char *moduleName) {
    FILE *fp;
    int loadedOK = CPHTRUE;
    char propFileName[512];
    size_t dirLen;
    char value[1024];
    char errorString[1024];

    CPH_BUNDLE *pBundle = NULL;

    CPHTRACEENTRY(pConfig->pTrc)

    /* If we have a CPH_INSTALLDIR env var prefix prepend that to the property file name */
    dirLen = strlen(pConfig->cphInstallDir);
    if (0 < dirLen) {
        strcpy(propFileName, pConfig->cphInstallDir);
        if (pConfig->cphInstallDir[ dirLen - 1 ] != CPH_DIRSEP) {
            propFileName[dirLen] = CPH_DIRSEP;
            propFileName[dirLen+1] = '\0';
        }
        strcat(propFileName, moduleName);
    }
    else
        strcpy(propFileName, moduleName);

    strcat(propFileName, ".properties");

    fp = fopen(propFileName,
#ifdef AMQ_AS400
               "r");
#else
               "rt");
#endif
    if (NULL != fp) {
		char aLine[2048];
        /* Initiliase a new CPH_BUNDLE structure into which we read all the lines in the property file
        as name/value pairs */
        cphBundleIni(&pBundle, pConfig, moduleName);
		
        while ( NULL != fgets(aLine, 1023, fp)) {
            char name[80];
            char *token;

            /* If the line was just white space or empty ignore it */
            if (0 == strlen(aLine)) continue;

            /* If it was a comment ignore it */
            if ('#' == aLine[0]) continue;

            /* Remove any CRLF */
            if ((aLine[strlen(aLine) - 1] == '\n') || (aLine[strlen(aLine) - 1] == '\r')) {
                size_t len = strlen(aLine);
                do {
                    len--;
                    aLine[len] = '\0';
                } while ((len > 0) && ((aLine[len - 1] == '\r') || (aLine[len - 1] == '\n')) );
            }

            /* Take off any terminating and leading white space */
            cphUtilTrim(aLine);

            /* If the line is now just white space or empty ignore it */
            if (0 == strlen(aLine)) continue;

            /* Append any continuation lines   */
            while (aLine[strlen(aLine) - 1] == '\\') {
                if (NULL == fgets(aLine + strlen(aLine) - 1, 255, fp)) {
          sprintf(errorString, "OOPS! Expected line continuation but found EOF (or IO error) in %s\n", propFileName);
                    cphLogPrintLn(pConfig->pLog, LOG_ERROR, errorString);
                    break;
                }

                /* Remove any CRLF */
                if ((aLine[strlen(aLine) - 1] == '\n') || (aLine[strlen(aLine) - 1] == '\r')) {
                    size_t len = strlen(aLine);
                    do {
                       len--;
                       aLine[len] = '\0';
                    } while ((len > 0) && ((aLine[len - 1] == '\r') || (aLine[len - 1] == '\n')) );
                }
            }

            /* Parse out the line into its name and value components */
            strcpy(name, strtok(aLine, "="));

            /* The value may or may not be present */
            token = strtok(NULL, "=");
            if (NULL != token)
                strcpy(value, token);
            else
                *value = '\0';

            /* Don't be sensitive to white space around the "=" character */
            cphUtilRTrim(name);
            cphUtilLTrim(value);

            /* Store the name value pair into the given CPH_NAMEVAL structure */
            if (CPHTRUE != cphNameValAdd(&(pBundle->pNameValList), name, value)) {
                cphLogPrintLn( pConfig->pLog, LOG_ERROR, "ERROR storing name/value pair." );
                loadedOK = CPHFALSE;
                break;
            }
        }
        fclose(fp);
	    /* Add the pointer to this bundle into the array of resource bundle pointers */
	    cphArrayListAdd(pConfig->bundles, pBundle);
    }
    else {
        sprintf(errorString, "ERROR couldn't open properties file: %s.", propFileName);
        cphLogPrintLn( pConfig->pLog, LOG_ERROR, errorString );
        loadedOK = CPHFALSE;
    }
	
    CPHTRACEEXIT(pConfig->pTrc)

    if (CPHTRUE == loadedOK) return(pBundle);
    return(NULL);
}


/*
** Method: cphBundleIni
**
** Initialise a CPH_BUNDLE
**
** Input Parameters: pConfig - pointer to the configuration control block
**                   moduleName - the name of the module which this CPH_BUNDLE relates to
**
** Output Parameters: ppBundle - double pointer to the CPH_BUNDLE to be allocated
**
**
** Returns: CPHTRUE on successful exection, CPHFALSE otherwise
**
*/
void cphBundleIni(CPH_BUNDLE **ppBundle, CPH_CONFIG *pConfig, char *moduleName) {
   CPH_BUNDLE *pBundle;
   CPHTRACEENTRY(pConfig->pTrc)

   pBundle = malloc(sizeof(CPH_BUNDLE));
   if (NULL != pBundle) {
       strcpy(pBundle->clazzName, moduleName);
       pBundle->pConfig = pConfig;
       pBundle->pNameValList = NULL;
   }

   *ppBundle = pBundle;

   CPHTRACEEXIT(pConfig->pTrc)
}

/*
** Method: cphBundleFree
**
** This function disposes of a CPH_BUNDLE
**
** Input/Output Parameters: ppBundle - double pointer to the CPH_BUNDLE. The pointer
**                          is set to NULL when it is successfully disposed of.
**
** Returns: CPHTRUE on successful exection, CPHFALSE otherwise
**
*/
int  cphBundleFree(CPH_BUNDLE **ppBundle) {
    int status = CPHFALSE;
    CPHTRACEREF(pTrc, (*ppBundle)->pConfig->pTrc)
    CPHTRACEENTRY(pTrc)

  if (NULL != *ppBundle) {

        /* Free the CPH_NAMEVAL list of name/value pairs that were read in from the property file */
        cphNameValFree(&(*ppBundle)->pNameValList);

    free(*ppBundle);
    *ppBundle = NULL;
    status = CPHTRUE;
  }

    CPHTRACEEXIT(pTrc)

    return(status);
}

/*
** Method: cphBundleGetBundle
**
** This function sets a pointer to a CPH_BUNDLE representing a registered module (if it exists)
**
** Input Paramters:         module - The name of the module for which a CPH_BUNDLE is being searched for. 
**
** Input/Output Parameters: ppBundle - double pointer to the CPH_BUNDLE. A pointer to a registered  
**                          module's bundle is stored here, if it is found
**
** Returns: CPHTRUE on successful exection (CPH_BUNDLE for module found), CPHFALSE otherwise
**
*/
int cphBundleGetBundle(CPH_BUNDLE **pBundle, CPH_CONFIG *pConfig, char *module) {
    CPH_LISTITERATOR *pIterator;
    int moduleFound = CPHFALSE;
    pIterator = cphArrayListListIterator(pConfig->bundles);
    do {
       CPH_ARRAYLISTITEM *pItem = cphListIteratorNext(pIterator);
       *pBundle = (CPH_BUNDLE*) pItem->item;

       if (0 == strcmp(module, (*pBundle)->clazzName))
       {
          return CPHTRUE;
       }

    } while (cphListIteratorHasNext(pIterator));
	return CPHFALSE;
}




