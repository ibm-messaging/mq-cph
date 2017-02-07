#<copyright notice="lm-source" pids="" years="2014,2017">
#***************************************************************************
# Copyright (c) 2014,2017 IBM Corp.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# Contributors:
#    Various members of the WebSphere MQ Performance Team at IBM Hursley UK
#***************************************************************************
#*</copyright>
############################################################################
#                                                                          #
# Performance Harness for IBM MQ C-MQI interface                           #
#                                                                          #
############################################################################

# Include unix definitions
include unix.mk

CPPDEFS += CPH_SOLARIS

# C compiler command
#cc = /opt/gcc/bin/gcc -c # Newer GCC, likely to cause complications due to partial implementation of TR1 classes
cc = /usr/sfw/bin/gcc -c # Older GCC
#cc = cc -c # Sun's own compiler - very old by the looks of things; mqmake will likely use a newer version of this

# C++ compiler command
#CC = /opt/gcc/bin/g++ -c # Newer GCC, likely to cause complications due to partial implementation of TR1 classes
CC = /usr/sfw/bin/g++ -c # Older GCC
#CC = CC -c # Sun's own compiler - very old by the looks of things; mqmake will likely use a newer version of this

# Linker command
#LD = /opt/gcc/bin/g++ # Newer GCC, likely to cause complications due to partial implementation of TR1 classes
LD = /usr/sfw/bin/g++ # Older GCC
#LD = CC # Sun's own compiler - very old by the looks of things; mqmake will likely use a newer version of this

# Add flags to write dependency files
DEP = .d
CCFLAGS += -MMD -MP # GCC command
#CCFLAGS += -xMMD # Sun command - doesn't work with the current version of the compiler on mqperfs2

# Add debug flags for debug build
Debug_CCFLAGS += -O0 -g3 # GCC command
#Debug_CCFLAGS += -O1 -g # Sun's command
Debug_CFLAGS += $(SYM)_GLIBC_DEBUG # GCC only
Debug_CXXFLAGS += $(SYM)_GLIBCXX_DEBUG # GCC only

#Add optimization for release build
Release_CCFLAGS = -O3

# Set the rpath on the linked executable
LDFLAGS += $(LP)$(MQLIB)

# Add stdc++ libs
LIBS += stdc++ # GCC only
