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

# Set default MQ installation path
MQ_INSTALLATION_PATH ?= /usr/mqm

# Include unix definitions
include unix.mk

CPPDEFS += CPH_AIX

XLC_INSTALLDIR=/opt/IBM/xlC/*/bin

# C compiler command
cc = $(XLC_INSTALLDIR)/xlc_r -c -q64 -qtune=pwr7 -qlibansi -qnolm -qhalt=s

###################################################
# C++ compiler command
#
# Options:
#   -qlanglvl=extended0x : Use c++11x standard (which is still "experimental" for IBM)
#   -D__IBMCPP_TR1__=1 : Enable some of the "experimental" parts of the above, namely
#                        unordered_map and unordered_set
###################################################
CC = $(XLC_INSTALLDIR)/xlC_r -+ -c -q64 -qtune=pwr7 -qlibansi -qnolm -qhalt=s -qlanglvl=extended0x -D__IBMCPP_TR1__=1

# Linker command
LD = $(XLC_INSTALLDIR)/xlC_r -q64 -bmaxdata:0x2000000000

# Add flags to write dependency files
DEP = .u
CCFLAGS += -qmakedep=gcc

# Add debug flags for debug build
Debug_CCFLAGS += -O0 -g

#Add optimization for release build
Release_CCFLAGS = -O3 -Q

# Set the rpath on the linked executable
LDFLAGS += $(LP)$(MQLIB)
