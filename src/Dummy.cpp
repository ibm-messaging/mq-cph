/*<copyright notice="lm-source" pids="" years="2014,2018">*/
/*******************************************************************************
 * Copyright (c) 2014,2018 IBM Corp.
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
 *    Rowan Lonsdale - Initial implementation
 *    Various members of the WebSphere MQ Performance Team at IBM Hursley UK
 *******************************************************************************/
/*</copyright>*/
/*******************************************************************************/
/*                                                                             */
/* Performance Harness for IBM MQ C-MQI interface                              */
/*                                                                             */
/*******************************************************************************/

#include "Dummy.hpp"
#include <math.h>
#include <sstream>

#include "cphTrace.h"
#include "cphLog.h"

namespace cph {

Dummy::Dummy(ControlThread * pControlThread) : WorkerThread(pControlThread, "Dummy"),
    primes(), largestGap(0), largestGapIndex(1) {
  if(messages>0)
    primes.reserve(messages);
}
Dummy::~Dummy() {}
void Dummy::openSession(){}

/*
 * Method: closeSession
 *
 * Print a summary of the progress made so far.
 */
void Dummy::closeSession(){
  std::stringstream ss;
  ss << name << " found " << getIterations() << " primes.\n";
  ss << "Largest prime: " << primes[getIterations()-1] << "\n";
  ss << "Largest gap between consecutive primes was " << largestGap << ", between " <<
      primes[largestGapIndex-1] << "[" << largestGapIndex << "] and " << primes[largestGapIndex] << "[" << largestGapIndex+1 << "]\n";
  cphLogPrintLn(pConfig->pLog, LOG_INFO, ss.str().data());
}


/**
 * Method: oneIteration
 *
 * Finds prime numbers to simulate the cpu load of a real messaging operation.
 **/
void Dummy::oneIteration(){
  CPHTRACEENTRY(pConfig->pTrc)

  unsigned int numToFind = getIterations()+1;
  CPHTRACEMSG(pConfig->pTrc, "Looking for prime [%u]", numToFind)

  if(numToFind==1)
    primes.push_back(2);
  else {
    unsigned long candidate = primes.back()+1;
    bool isPrime;
    do {
      unsigned long lim = (unsigned long) floor(sqrt((double) candidate));
      isPrime = true;
      for(long i=0; primes[i]<=lim; i++){
        if(candidate%primes[i]==0){
           isPrime = false;
           ++candidate;
           break;
        }
      }
    } while (!isPrime);
    primes.push_back(candidate);
    unsigned int gap = candidate - primes[numToFind-2];
    if(gap>largestGap){
      largestGap = gap;
      largestGapIndex = numToFind-1;
    }
  }
  CPHTRACEMSG(pConfig->pTrc, "Found prime number [%u]: %lu", numToFind, primes[numToFind-1])
    CPHTRACEEXIT(pConfig->pTrc)
}

}
