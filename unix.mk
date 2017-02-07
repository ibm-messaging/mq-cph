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

# Get processor architecture
ARCH ?= $(shell uname -p)

# Add AMQ_UNIX preprocessor definition
CPPDEFS += CPH_UNIX

# Executable file extension
EXE =

# Object file extension
OBJ = .o

#Command to delete files
RM = rm -f

#Command to delete directories
RMDIR = rm -rf

#Command to make a directory
MKDIR = mkdir -p

#Command to create a parent directory if it's missing
MKDIRPART = if [ ! -d $(@D) ]; then echo "Creating output directory: $(@D)"; echo ""; $(MKDIR) $(@D); fi

#Command to link file
LN = ln -fs

#Command to copy a file or directory
CP = cp -vr

# Option to specify target as compiler output file
OUT = -o $@

#Option to add include directory (headers)
INC = -I

#Option to add lib directory to libpath
LP = -L

#Option to add preprocessor symbol
SYM = -D

#Option to link library
LIB = -l

# Set default MQ installation path
MQ_INSTALLATION_PATH ?= /opt/mqm

#Set addressing mode
BITNESS ?= 64

# Set MQ library path
ifeq ($(BITNESS),64)
	MQLIB = $(MQ_INSTALLATION_PATH)/lib64
else
	MQLIB = $(MQ_INSTALLATION_PATH)/lib
endif

# Libraries to link
LIBS = pthread c dl rt m