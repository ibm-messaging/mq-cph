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
 * Define some C99 standard i/o functions
 * that Microsoft still doesn't support.
 */

#ifndef MSIO_H_
#define MSIO_H_

#include <stdio.h>

#if defined(_MSC_VER) && (_MSC_VER < 1900) //support for C99 headers was added in VS2014
#include <stdarg.h>

#if defined(__cplusplus)
  extern "C" {
#endif

#define vsnprintf vsnprintf_c99

static int vsnprintf_c99(char* str, size_t size, const char* format, va_list args){
  int count = _vscprintf(format, args);
  _vsnprintf_s(str, size, _TRUNCATE, format, args);
  return count;
}

static int snprintf(char* str, size_t size, const char* format, ...){
  int count;
  va_list args;
  va_start(args, format);
  count = vsnprintf(str, size, format, args);
  va_end(args);
  return count;
}

#if defined(__cplusplus)
  }
#endif
#endif

#endif /* MSIO_H_ */
