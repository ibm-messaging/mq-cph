/*<copyright notice="lm-source" pids="" years="2014,2017">*/
/*******************************************************************************
 * Copyright (c) 2014,2017 IBM Corp.
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

/*
 * Define some C99 standard integral-types and macros,
 * that Microsoft still doesn't support.
 */

#ifndef MSINT_H_
#define MSINT_H_

#if defined(_MSC_VER) && (_MSC_VER < 1900) //support for C99 headers was added in VS2014

#if defined(__cplusplus)
  extern "C" {
#endif

typedef unsigned __int32 uint32_t;
typedef unsigned __int64 uint64_t;

#define PRIX32 "I32X"
#define PRIX64 "I64X"
#define PRIu32 "I32u"
#define PRIu64 "I64u"

#if defined(__cplusplus)
  }
#endif

#else
#include <inttypes.h>
#endif

#endif /* MSINT_H_ */
