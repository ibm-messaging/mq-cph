/*<copyright notice="lm-source" pids="" years="2007,2022">*/
/*******************************************************************************
 * Copyright (c) 2007,2021 IBM Corp.
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

#include "cphUtil.h"

#if defined(AMQ_NT)
  #include <winbase.h>
  #include <stdint.h>
#elif defined(CPH_UNIX)
#if defined(AMQ_AS400)
     #include <mih/retthid.h>
   #endif
   #include <unistd.h>
   #include <pthread.h>
#endif

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <assert.h>

#if defined(AMQ_AS400) || defined(AMQ_MACOS)
#include <sys/select.h>
#define SMALLPART tv_usec
#elif defined(CPH_UNIX)
#define SMALLPART tv_nsec
#endif

/* Static variable that the controlC handlers sets to tell the rest of the program to close down */
extern volatile sig_atomic_t cphControlCInvoked;

#if defined(AMQ_NT)
// Performance counter frequency (used for timing).
long CountsPerMillisecond;
long CountsPerMicrosecond;
uint64_t performanceFrequency;
#endif

/*
** Method: cphUtilSleep
**
** sleep for a specified number of milliseconds
**
** Input Parameters: msecs - the number of milliseconds to sleep
**
**
*/
void cphUtilSleep( int mSecs ) {
#if defined(WIN32)
   Sleep( mSecs );
#elif defined(AMQ_AS400)
   struct timeval tval;
   tval.tv_sec  = (mSecs) / 1000;
   tval.tv_usec = ((mSecs) % 1000) * 1000;
   select(0, NULL, NULL, NULL, &tval);
#else
   struct timespec rqtp;
   rqtp.tv_sec  = (mSecs) / 1000;
   rqtp.tv_nsec = ((mSecs) % 1000) * 1000000;
   nanosleep(&rqtp, NULL);
#endif
}

/*
** Method: cphUtilTimeIni
**
** Initialise a CPH_TIME structure to zero. This structure is implemented differently on Linux and
** Windows. On Windows it is a ULARGE_INTEGER and on Linux a timeval structure.
**
** Input Parameters: pTime - pointer to the CPH_TIME value to be initialised
**
** Returns: CPHTRUE on successful exection, CPHFALSE otherwise
**
*/
int cphUtilTimeIni(CPH_TIME *pTime) {
#if defined(WIN32)
    pTime->LowPart = 0;
    pTime->HighPart = 0;
#elif defined(CPH_UNIX)
    pTime->tv_sec = 0;
    pTime->SMALLPART = 0;
#endif
    return(CPHTRUE);
}

/*
** Method: cphUtilGetNow
**
** Get the current time in milliseconds as a CPH_TIME value. CPH_TIME hides the machine dependencies between
** Windows and Linux.
**
** Returns: The CPH_TIME value corresponding to the current time. This is only guaranteed to be useful for comparing times
** (establishing durations). It is not intended to be used for time-stamping.
**
*/
CPH_TIME cphUtilGetNow() {
   CPH_TIME ret;
#if defined(AMQ_NT)

   QueryPerformanceCounter(&ret);

#else //#elif defined(CPH_UNIX)

   ret.tv_sec = 0;
   ret.SMALLPART = 0;

#if defined(AMQ_AS400) || defined(AMQ_MACOS)
  /* On iSeries we get the number of seconds and micro seconds since   */
  /* the epoch using the gettimeofday function, which is now obsolete  */
  /* on other Unix platforms.                                          */
   gettimeofday(&ret, NULL);
#else
   clock_gettime(CLOCK_REALTIME, &ret);
#endif
#endif
   return ret;
}

/*
** Method: cphUtilGetTimeDifference
**
** Calculate the difference between two CPH_TIME values
**
** Input Parameters: time1 - the later CPH_TIME to be compared
**                   time2 - the earlier CPH_TIME to be compared
**
** Returns: a long value representing the number of milliseconds after time2 that time1 occurs.
**
*/
long cphUtilGetTimeDifference(CPH_TIME time1, CPH_TIME time2) {

#if defined(AMQ_NT)
   if(CountsPerMillisecond==0){
      LARGE_INTEGER freq;
      QueryPerformanceFrequency(&freq);
      CountsPerMillisecond = (long) (freq.QuadPart/1000);
   }
   return (long) ((time1.QuadPart-time2.QuadPart)/CountsPerMillisecond);
#elif defined(AMQ_AS400) || defined(AMQ_MACOS)
   return (long) ((time1.tv_sec-time2.tv_sec)*1000 + (time1.tv_usec-time2.tv_usec)/1000);
#elif defined(CPH_UNIX)
   return (long) ((time1.tv_sec-time2.tv_sec)*1000 + (time1.tv_nsec-time2.tv_nsec)/1000000);
#endif

}
long cphUtilGetUsTimeDifference(CPH_TIME time1, CPH_TIME time2) {

#if defined(AMQ_NT)
   if(CountsPerMicrosecond==0){
      LARGE_INTEGER freq;
      QueryPerformanceFrequency(&freq);
      CountsPerMicrosecond = (long) (freq.QuadPart/1000000);
   }
   return (long) ((time1.QuadPart-time2.QuadPart)/CountsPerMicrosecond);
#elif defined(AMQ_AS400) || defined(AMQ_MACOS)
   return (long) ((time1.tv_sec-time2.tv_sec)*1000000 + (time1.tv_usec-time2.tv_usec));
#elif defined(CPH_UNIX)
   if(time1.tv_nsec > time2.tv_nsec){
     return (long) ((time1.tv_sec - time2.tv_sec) * 1000000 - (time2.tv_nsec - time1.tv_nsec)/1000);
} else {
  return (long) ((time1.tv_sec - time2.tv_sec) * 1000000 + (time1.tv_nsec - time2.tv_nsec)/1000);
}
#endif

}

double cphUtilGetDoubleDuration(CPH_TIME start, CPH_TIME end){
#if defined(AMQ_NT)
   if(performanceFrequency==0){
      LARGE_INTEGER freq;
      QueryPerformanceFrequency(&freq);
      performanceFrequency = freq.QuadPart;
   }
   return (double)(end.QuadPart-start.QuadPart)/performanceFrequency;
#elif defined(AMQ_AS400) || defined(AMQ_MACOS)
   return (double)((end.tv_sec-start.tv_sec)*1000000 + end.tv_usec - start.tv_usec)/1000000;
#elif defined(CPH_UNIX)
   return (double)((end.tv_sec-start.tv_sec)*1000000000 + end.tv_nsec - start.tv_nsec)/1000000000;
#endif
}

/*
** Method: cphUtilTimeCompare
**
** Compare two CPH_TIMES and return an integer value set to -1, 0, or +1 depending on
** whether the first time is less than, equal two, or greater than the second time.
**
** Input Parameters: time1 - the first CPH_TIME to be compared
**                   time2 - the second CPH_TIME to be compared
**
** Returns:
**      -1 if pTime1 < pTime2
**       0  if pTime1 = pTime2
**       1  if pTime1 > pTime2
*/
int cphUtilTimeCompare(CPH_TIME time1, CPH_TIME time2) {

#if defined(WIN32)
  if(time1.QuadPart<time2.QuadPart)
    return -1;
  else if(time1.QuadPart>time2.QuadPart)
    return 1;
  else
    return 0;
#elif defined(CPH_UNIX)
  if(time1.tv_sec<time2.tv_sec)
    return -1;
  else if(time1.tv_sec>time2.tv_sec)
    return 1;
  else {
    if(time1.SMALLPART<time2.SMALLPART)
      return -1;
    else if(time1.SMALLPART>time2.SMALLPART)
      return 1;
    else
      return 0;
  }
#endif
}

/*
** Method: cphCopyTime
**
** Copy one CPH_TIME to another
**
** Input Parameters: pTimeDst - destination CPH_TIME
**                   pTimeSrc - source CPH_TIME
**
** Returns:
**          None
*/
void cphCopyTime(CPH_TIME *pTimeDst, CPH_TIME *pTimeSrc) {

   #if defined(WIN32)
      pTimeDst->LowPart = pTimeSrc->LowPart;
      pTimeDst->HighPart = pTimeSrc->HighPart;
      pTimeDst->QuadPart = pTimeSrc->QuadPart;
   #elif defined(CPH_UNIX)
      pTimeDst->tv_sec = pTimeSrc->tv_sec;
      pTimeDst->SMALLPART = pTimeSrc->SMALLPART;
   #endif
}

/*
** Method: cphUtilTimeIsZero
**
** Determine whether a CPH_TIME is at its zero initial state or not
**
** Input Parameters: pTime - the CPH_TIME to be checked
**
** Returns: CPHTRUE if the CPH_TIME is zero and CPHFALSE otherwise
**
*/
int cphUtilTimeIsZero(CPH_TIME pTime) {
#if defined(WIN32)
    if (pTime.QuadPart==0) return(CPHTRUE);
#elif defined(CPH_UNIX)
    if (pTime.tv_sec == 0 && pTime.SMALLPART == 0) return(CPHTRUE);
#endif
    return(CPHFALSE);
}

/*
** Method: cphUtilGetThreadId
**
** Get the thread id of the calling thread
**
** Returns: the thread number of the calling thread (cph_pthread_t)
**
*/
cph_pthread_t cphUtilGetThreadId() {
    cph_pthread_t threadId;
#if defined(WIN32)
    threadId = GetCurrentThreadId();
#elif defined(AMQ_AS400)
    _RETTHID_Thread_ID_T mytid = _RETTHID();
    threadId = mytid.Thread_ID_Low;
#elif defined(AMQ_MACOS)
    threadId = (uint64_t)pthread_mach_thread_np(pthread_self());
#elif defined(CPH_UNIX)
    threadId = pthread_self();
#else
#error "Don't know how to find threadId on this platform"
#endif
    return(threadId);
}

/*
** Method: cphUtilGetProcessId
**
** Get the process id of the calling process
**
** Returns: the process id of the calling process (int)
**
*/
int cphUtilGetProcessId() {
    int processId;
#if defined(WIN32)
    processId = GetCurrentProcessId();
#else //#elif defined(CPH_UNIX)
    processId = (int) getpid();
#endif
    return(processId);
}

/*
** Method: cphUtilSigInt
**
** We register this method to be called (via a call to signal) in the event Control-C is pressed.
** The method sets the static variable cphControlCInvoked to tell the rest of cph to close down
** and re-registers the method to be called again in the event of further control-C requests.
**
** Input Parameters: dummysignum - dummy argument, required
**
*/
/* NB: This method gets invoked by Windows on a different thread */
void cphUtilSigInt(int dummysignum) {
    cphControlCInvoked = dummysignum;
    signal(SIGINT, cphUtilSigInt);
    signal(SIGTERM, cphUtilSigInt);
}

/*
** Method: cphUtilGetTraceTime
**
** Return a character string representation of the time in the following format:
   dd_mm_yyyy hh:mm:ss:mmm
**
** This method is used by the trace mechanism.
**
** Output Parameters: chTime - the character string in which the time is to be written
**
** Returns: CPHTRUE on successful exection, CPHFALSE otherwise
**
*/
int cphUtilGetTraceTime(char *chTime) {
#if defined(WIN32)
  SYSTEMTIME stime;

  GetSystemTime (&stime);

  sprintf(chTime, "%02u_%02u_%04u %02u:%02u:%02u.%03u",
      stime.wDay,
      stime.wMonth,
      stime.wYear,
      stime.wHour,
      stime.wMinute,
      stime.wSecond,
      stime.wMilliseconds);

  return(CPHTRUE);
#elif defined(CPH_UNIX)

  /* This is used to store the "broken down" time */
  struct tm  brokenDownTime;

#if defined(AMQ_AS400) || defined(AMQ_MACOS)
  /* This variable will get the number of seconds and micro seconds since the epoch */
  struct timeval atimeval;
  if (0 == gettimeofday(&atimeval, NULL)) {
    unsigned int milliSecs = atimeval.tv_usec / 1000;
#else
  struct timespec atimeval;
  if (0 == clock_gettime(CLOCK_REALTIME, &atimeval)) {
    unsigned int milliSecs = atimeval.tv_nsec / 1000000;
#endif

    localtime_r(&(atimeval.tv_sec), &brokenDownTime);

    sprintf(chTime, "%02d_%02d_%04d %02d:%02d:%02d.%03u",
        brokenDownTime.tm_mday,
        1 + brokenDownTime.tm_mon,
        1900 + brokenDownTime.tm_year,
        brokenDownTime.tm_hour,
        brokenDownTime.tm_min,
        brokenDownTime.tm_sec,
        milliSecs);

    return(CPHTRUE);
  }
  return(CPHFALSE);
#endif
}

/*
** Method: cphUtilRTrim
**
** Trim a string from the right
**
** Input Parameters: aLine - the character string to be trimmed
**
** Returns: a pointer to the character string to enable trim calls to be nested
**
*/
char *cphUtilRTrim(char *aLine) {
    if (NULL == aLine) return(NULL);
    if (0 == strlen(aLine)) return(aLine);

    while (aLine[strlen(aLine) - 1] == ' ') {
       aLine[strlen(aLine) - 1] = '\0';
    }

    return(aLine);
}

/*
** Method: cphUtilLTrim
**
** Trim a string from the left
**
** Input Parameters: aLine - the character string to be trimmed
**
** Returns: a pointer to the character string to enable trim calls to be nested
**
*/
char *cphUtilLTrim(char *aLine) {

    if (NULL != aLine) {
       size_t len;
       size_t i=0;

       if (0 == (len = strlen(aLine))) return(aLine);

       /* find the first non white space character */
       while ( (aLine[i] == ' ') && (i < len) ) i++;

       /* If the whole line was blanks return a NULL string */
       if (i == len)
           *aLine = '\0';
       /* Otherwise move the non black section of the line up to the beginning and terminate with a null */
       else if (0 < i) {
          memmove(aLine, &aLine[i], len -i);
          aLine[len -i] = '\0';
       }
    }
    return(aLine);
}

/*
** Method: cphUtilTrim
**
** Trim a character string from the right and left by calling cphUtilRTrim and cphUtilLTrim
**
** Input Parameters: aLine - the character string to be trimmed
**
** Returns: a pointer to the character string to enable trim calls to be nested
**
*/
/* Trim a string from the right and the left */
char *cphUtilTrim(char *aLine) {
    return(cphUtilLTrim(cphUtilRTrim(aLine)));
}

/*
** Method: cphUtilStringEndsWith
**
** Scan a given string to see if it ends with another given string
**
** Input Parameters: aString - the string to be checked
**                   ending - the string we are checking for at the end of aString
**
** Returns: CPHTRUE if the first string ends with the second string, CPHFALSE otherwise
**
*/
int cphUtilStringEndsWith(char *aString, char *ending) {
    int result = CPHFALSE;
    char *ptr;

  assert(aString != NULL);

    /* Search for the sub-string */
    ptr = strstr(aString, ending);

    /* If strstr returned aString then the seach string was empty
       If strstr returned NULL then the seach string was not found
    */
    if ( (aString != ptr) && (NULL != ptr) ) {
        /* Was the seach string at the end of the string ? */
        if (strlen(aString) == (ptr - aString) + strlen(ending))
            result = CPHTRUE;

    }
    return(result);
}

/*
** Method: cphUtilstrcrlf
**
** Replace all literal "\n" characters in a string with a binary \n
**
** Input Parameters: aString - a pointer to the string to be edited
**
** Returns: the pointer to the character string for calling convenience
**
*/
char *cphUtilstrcrlf(char *aString) {
    char *res;

  assert(aString != NULL);

    res = strstr(aString, "\\n");

    while ( (NULL != res) && (res != aString) ) {
       *res = ' ';
       *(res+1) = '\n';
       res = strstr(aString, "\\n");
    }
    return(aString);

}

/*
** Method: cphUtilstrcrlfTotabcrlf
**
** Replace all literal "\n" characters in a string with a binary CRLF and tab
**
** Input Parameters: aString - a pointer to the string to be edited
**
** Returns: the pointer to the character string for calling convenience
**
*/
char *cphUtilstrcrlfTotabcrlf(char *aString) {
    char *res;

  assert(aString != NULL);

    res = strstr(aString, "\\n");

    while ( (NULL != res) && (res != aString) ) {
       *res = '\n';
       *(res+1) = '\t';
       res = strstr(aString, "\\n");
    }
    return(aString);

}

/*
** Method: cphUtilReadMsgFile
**
** Function to return a char array from the contents of a file. This is used to build the message contents to be published. The
** character string is allocated with realloc and needs to be disposed of by the caller.
**
** Input Parameters: pSize - pointer to MQLONG where the length of the char* read can pe written to.
**                   fileSpec - Fully qualified filename.
**
** Returns: a pointer to the built character string or NULL, if the file indicated by fileSpec does not exist.
**
*/
char *cphUtilReadMsgFile(MQLONG *pSize, const char *fileSpec) { 
   MQLONG readSize=4096;
   *pSize = 0;
   char *msg=NULL;
   MQLONG elementsRead = 0;

   FILE *msgFile = fopen(fileSpec, "rb");

   if (msgFile == NULL) return NULL;

   do {
      *pSize = *pSize + readSize; 
      msg=realloc(msg, *pSize);
      elementsRead = (MQLONG) fread(msg + *pSize - readSize, 1, readSize, msgFile);
   } while(elementsRead == readSize);

   *pSize = *pSize - (readSize - elementsRead);
   msg=realloc(msg, *pSize);

   return msg;
}

/*
** Method: cphUtilMakeBigString
**
** Function to build a string of a given size. This is used to build the message contents to be published. The
** string is allocated with malloc and needs to be disposed of by the caller.
**
** Input Parameters: size - the length of the string to build
**
** Returns: a pointer to the built character string
**
*/
char *cphUtilMakeBigString(int size, int randomise) {

    char *str = NULL;

    /* Actually grab one byte more than requested so we can terminate the string */
    /* NB: This string is deallocated by cphPublisherFree */
  size++;
    if (NULL != (str = (char*) malloc(size))) {
        int i;
        char c = 65;
        if(!randomise){
          for (i=0; i < size; i++) {
            str[i] = c++;
            if (c > 122) c = 65;
          }
        } else {
          for (i=0; i < size; i++) {
            c = rand() % 58 + 65;   /*pseudo-random number in range 65-122. rand is left unseeded so tests are repeatable*/
            str[i] = c;
          }
        }

    /* Null terminate the string for trace etc */
    str[size - 1] = '\0';
    }
    return(str);
}

/*
** Method: cphUtilMakeBigStringWithRFH2
**
** Function to build a string of a given size, plus an additional RFH2.
** This is used to build the message contents to be published. The
** string is allocated with malloc and needs to be disposed of by the caller.
**
** Input Parameters: size - the length of the string to build
**
** Returns: a pointer to the built character string
**
*/
char *cphUtilMakeBigStringWithRFH2(int size, size_t *pRfh2Len) {

    char *str = NULL;
    char *ptr = NULL;
    MQRFH2 rfh2 = {MQRFH2_DEFAULT};
    MQLONG rfh2Len;

  assert(size >= 0);

#define RFH2_NAME_VALUE_DATA_LENGTH_1 32
#define RFH2_NAME_VALUE_DATA_LENGTH_2 144

    rfh2Len = sizeof(MQRFH2) + sizeof(MQLONG) + RFH2_NAME_VALUE_DATA_LENGTH_1 + sizeof(MQLONG) + RFH2_NAME_VALUE_DATA_LENGTH_2;
    rfh2.StrucLength = rfh2Len;
    rfh2.CodedCharSetId = 1208;
    memcpy(rfh2.Format, "MQSTR    ", MQ_FORMAT_LENGTH);
    rfh2.NameValueCCSID = 1208;

    /* Actually grab one byte more than requested so we can terminate the string */
    /* NB: This string is deallocated by cphPublisherFree */
    if (NULL != (ptr = (char*) malloc(size + 1 + rfh2Len))) {
     int i;
       char c = 65;
       *pRfh2Len = rfh2Len;
       str = ptr;
       memcpy(str, &rfh2, sizeof(MQRFH2));
       str += sizeof(MQRFH2);
       *((MQLONG *)str) = RFH2_NAME_VALUE_DATA_LENGTH_1;
       str += sizeof(MQLONG);
/*           memcpy(str, "<psc><Command>Publish</Command><Topic>MQJMS/PSIVT/Information</Topic></psc> ", RFH2_NAME_VALUE_DATA_LENGTH);*/
       memcpy(str, "<mcd><Msd>jms_text</Msd></mcd>  ", RFH2_NAME_VALUE_DATA_LENGTH_1);
       str += RFH2_NAME_VALUE_DATA_LENGTH_1;
       *((MQLONG *)str) = RFH2_NAME_VALUE_DATA_LENGTH_2;
       str += sizeof(MQLONG);
       memcpy(str, "<jms><Dst>topic://TOPIC1</Dst><Tms>1207047258454</Tms><Dlv>1</Dlv><Uci dt='bin.hex'>414D51435A4D53504552463420202020CDF7F147011B0020</Uci></jms>", RFH2_NAME_VALUE_DATA_LENGTH_2);
       str += RFH2_NAME_VALUE_DATA_LENGTH_2;
       for (i=0; i < size; i++) {
           str[i] = c++;
           if (c > 122) c = 65;
       }

       /* Null terminate the string for trace etc */
       str[size] = '\0';
    }

    return(ptr);
}

/*
** Method: cphBuildRFH2
**
** Function to build an RFH2 header
** This is used to build an RFH2 header containing a message property which will be used when selecting the message
** string is allocated with malloc and needs to be disposed of by the caller.
**
** Ouput Parameters: pSize - the length of the returned RFH2 header
**
** Returns: a pointer to the built character string
**
*/
char *cphBuildRFH2(MQLONG *pSize, MQBYTE24 correlationID) {

    char *str = NULL;
    char *ptr = NULL;
    MQRFH2 rfh2 = {MQRFH2_DEFAULT};
    MQLONG rfh2Len;

    // Length of user properties, padded to make header divisible by 4
    #define RFH2_USR_CORRELID_OPEN_LEN 15
    #define RFH2_USR_CORRELID_TOTAL 60
    #define RFH2_USR_CORRELID_CLOSE_LEN 21

    rfh2Len = sizeof(MQRFH2) + sizeof(MQLONG) + RFH2_USR_CORRELID_TOTAL;
    rfh2.StrucLength = rfh2Len;
    rfh2.CodedCharSetId = 1208;
    memcpy(rfh2.Format, "MQSTR    ", MQ_FORMAT_LENGTH);
    rfh2.NameValueCCSID = 1208;

    if (NULL != (ptr = (char*) malloc(rfh2Len))) {
      str = ptr;
      memcpy(str, &rfh2, sizeof(MQRFH2));
      str += sizeof(MQRFH2);
      *((MQLONG *)str) = RFH2_USR_CORRELID_TOTAL;
      str += sizeof(MQLONG);
      memcpy(str, "<usr><correlid>", RFH2_USR_CORRELID_OPEN_LEN);
      str += RFH2_USR_CORRELID_OPEN_LEN;
      memcpy(str, correlationID, sizeof(MQBYTE24));
      str += sizeof(MQBYTE24);
      memcpy(str, "</correlid></usr>    ", RFH2_USR_CORRELID_CLOSE_LEN);
    }

    *pSize = rfh2Len;
    return(ptr);
}

/*
** Method: cphGetEnv
**
** Returns the value of a specified environment variable
**
** Input Parameters: varName - the name of the environment variable it is required to retrieve
**
** Returns: a pointer to the value of the environment variable or NULL if it is not defined
**
*/
int cphGetEnv(char *varName, char *varValue, size_t buffSize)
{
    int status = CPHTRUE;
#if defined(WIN32)
    size_t requiredSize;

    /* Get the size of the array we need to retrieve the value */
    if (0 != getenv_s( &requiredSize, NULL, 0, varName))
        return(CPHFALSE);

    /* If this size is less than the memory we have, return an error */
    if (buffSize < requiredSize) return(CPHFALSE);

    /* Now get the value of the environment variable into the supplied buffer */
    if (0 != getenv_s(&requiredSize, varValue, buffSize, varName))
        status = CPHFALSE;
#elif defined(CPH_UNIX)
    char *buff;
    if (NULL != (buff = getenv(varName))) {
        if (buffSize > strlen(buff))
           strcpy(varValue, buff);
        else
           status = CPHFALSE;
    } else {
        status = CPHFALSE;
    }
#endif
    return(status);
}

#ifdef SMALLPART
#undef SMALLPART
#endif
