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
/*</copyright">*/
/*******************************************************************************/
/*                                                                             */
/* Performance Harness for IBM MQ C-MQI interface                              */
/*                                                                             */
/*******************************************************************************/

/*
 * cphdefs.h
 * ---------
 *
 * Universal definitions which may be useful.
 */

#ifndef CPHDEFS_H_
#define CPHDEFS_H_

#ifdef __GNUC__
#define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#endif

#if defined(__SUNPRO_C) || defined(__SUNPRO_CC)
#define SUNPRO
#endif

// Define a macro to suppress "unused-variable" warnings in variable declarations.
#ifdef _MSC_VER
#pragma warning (disable: 6031)
#define UNUSED __pragma(warning(suppress: 4100 4101))
#elif defined(__GNUC__) || defined(__IBMCPP__)
#define UNUSED __attribute__((__unused__))
#else
#define UNUSED
#endif

#ifdef __cplusplus

/*
 * Sneaky helper function to determine equality of types at compile-time.
 * Particularly useful along with the "auto" keyword in C++11.
 */
extern "C++" {
namespace cph {
  template<typename T, typename U> struct is_same{ static const bool value = false; };
  template<typename T> struct is_same<T, T>{ static const bool value = true; };
  template<typename T, typename U> static inline bool eqTypes(T const &a, U const &b) { return cph::is_same<T, U>::value; }
}
}
/*
 * Try to indicate which levels of the C++ standard we support.
 * There are currently two standards we're interested in:
 *
 *   - TR1 (Technical Report 1) - Includes some additional standard libraries,
 *                                such as random, and unordered_map.
 *
 *   - c++11 - Major changes to the language standard, incorporates the libraries of TR1,
 *             and adds some additional ones (e.g. chrono).
 *
 * Depending on which of these are supported, and which compiler we are using, the header
 * files and namespaces of particular classes differ (particularly TR1 classes).
 */

#if __SUNPRO_C >= 0x5130 || defined(_MSC_VER) || defined(__APPLE__) || (defined(GCC_VERSION) && GCC_VERSION >= 40400 && (__cplusplus >= 201103L || defined(__GXX_EXPERIMENTAL_CXX0X__)))
  //#warning Enabling use of c++11 language features.
  #define ISUPPORT_CPP11
  #define tr1_inc(H) <H>
  #define tr1(F) std::F

#elif defined(GCC_VERSION) && GCC_VERSION >= 40000

  #define tr1_inc(H) <tr1/H>
  #define tr1(F) std::tr1::F

#elif defined(__IBMCPP__) && defined(__IBMCPP_TR1__)

  // Include altered xlC system header, containing compile bug fix
  extern "C++" {
    #include "fixes/xhashtbl.hpp"
  }
  #define tr1_inc(H) <H>
  #define tr1(F) std::tr1::F

#endif

#endif // __cplusplus

#endif /* CPHDEFS_H_ */
