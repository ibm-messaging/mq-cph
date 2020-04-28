#<copyright notice="lm-source" pids="" years="2007,2019">
#***************************************************************************
# Copyright (c) 2007,2019 IBM Corp.
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
#    Jerry Stevens - Initial implementation
#    Various members of the WebSphere MQ Performance Team at IBM Hursley UK
#***************************************************************************
#*</copyright>
############################################################################
#                                                                          #
# Performance Harness for IBM MQ C-MQI interface                           #
#                                                                          #
############################################################################

# Determine OS type
ifeq ($(OS),Windows_NT)
	OSTYPE = $(OS)
else
	OSTYPE = $(shell uname -s)
	UNIX = 1
endif

###########################################################################
# Include file to set OS-specific options
#
# There are several variables that these files should set,
# which specifiy compiler and linker programs, file extensions,
# command line flags to specify various option, amongst others.
#
# In addition the following vraibles can be appended to to add
# add-hoc options:
#
#   CPPDEFS - Preprocessor definitions (specified WITHOUT the appropriate
#             command-line option flag (e.g. -D))
#   CCFLAGS - Additional compiler flags for both C and C++ files
#   CFLAGS - Additional compiler flags for C files only
#   CXXFLAGS - Additional compiler flags for C++ files only
#   LDFLAGS - Additional linker flags
#
# Additionally, the variables release_<X> and debug_<X> can be appended to,
# where X is one of CCFLAGS, CFLAGS, or CXXFALGS, which will set
# those additional flags only for a release or debug build respectively.
###########################################################################
CPPDEFS = __STDC_FORMAT_MACROS

ifeq ($(OSTYPE),Windows_NT)
include cygwin.mk
endif

ifeq ($(OSTYPE),Linux)
include linux.mk
endif

ifeq ($(OSTYPE),AIX)
include aix.mk
endif

ifeq ($(OSTYPE),SunOS)
include solaris.mk
endif

# Program to build
CPH = cph$(EXE)
INSTALL = install

# Source directory
SRC = src

# Properties directory
PROPS = props

#C source file extension
CSRC = c

#C++ source file extension
CPPSRC = cpp

# Full list of object files to be created (one for each source file)
OBJS = $(patsubst $(SRC)/%.$(CSRC), %$(OBJ), $(wildcard $(SRC)/*.$(CSRC))) $(patsubst $(SRC)/%.$(CPPSRC), %$(OBJ), $(wildcard $(SRC)/*.$(CPPSRC)))

# List of include directories
INCLUDE += $(MQ_INSTALLATION_PATH)/inc

# OS-arch subolder name to put build outputs
BTYPE = $(OSTYPE)_$(ARCH)
$(info Platform sub-directory: $(BTYPE))

# Client applications to build

.DEFAULT_GOAL := Release

GOALS = \
Debug \
Release

# Make debug a trace build
Debug_CCFLAGS += $(SYM)CPH_DOTRACE -g
Release_CCFLAGS += $(SYM)NDEBUG

# Include dependency files
DEPS = $(patsubst $(SRC)/%.$(CSRC), %$(DEP), $(wildcard $(SRC)/*.$(CSRC))) $(patsubst $(SRC)/%.$(CPPSRC), %$(DEP), $(wildcard $(SRC)/*.$(CPPSRC)))
ifeq ($(MAKECMDGOALS),)
-include $(DEPS:%=$(.DEFAULT_GOAL)/$(BTYPE)/%)
else
-include $(DEPS:%=$(MAKECMDGOALS)/$(BTYPE)/%)
endif

# Don't delete binary or dependecy files
.PRECIOUS: %$(OBJ) %/$(CPH)

# Conditionally set additional compiler flags depending on the type of goal (release or debug)
define EXTRA

$(1)%$(OBJ): $(2)FLAGS += $$($(1)_$(2)FLAGS)

endef
$(foreach goal,$(GOALS),$(foreach ftype,CC C CXX,$(eval $(call EXTRA,$(goal),$(ftype)))))

# Rule to make 'Debug' or 'Release' version of cph
$(GOALS): %: %/$(BTYPE)/install

# Chaining rule to allow CPH to be 'installed' if installdir is specified
ifdef installdir
$(info Installation directory: $(installdir))
# Recipe to copy built executable and properties file to install directory,
# if installdir was specified on the command line
%/install: %/$(CPH)
	@echo "Copying files to install directory $(installdir)"
	$(MKDIR) $(installdir)
	$(CP) $(@D)/$(CPH) $(installdir)/$(CPH)
	$(CP) $(PROPS)/* $(installdir)
else
# There's a Tab at the beginning of the line 2 lines below:
#     This is important - DO NOT DELETE
%/install: %/$(CPH)
	
endif

# Rule to link executable from object files
%/$(CPH): $(OBJS:%=\%/%)
	@$(MKDIRPART)
	@echo "Linking executable: $@"
	$(LD) $(OUT) $(@D)/*$(OBJ) $(LIBS:%=$(LIB)%) $(LDFLAGS)
	@echo " "

.SECONDEXPANSION:

# Rule to build C objects
%$(OBJ):: $(SRC)/$$(notdir $$*).$(CSRC)
	@$(MKDIRPART)
	@echo "Building C object: $< -> $@"
	$(cc) $(OUT) $< $(CPPDEFS:%=$(SYM)%) $(INCLUDE:%=$(INC)%) $(CCFLAGS) $(CFLAGS)
	@echo " "

# Rule to build C++ objects
%$(OBJ):: $(SRC)/$$(notdir $$*).$(CPPSRC)
	@$(MKDIRPART)
	@echo "Building C++ object: $< -> $@"
	$(CC) $(OUT) $< $(CPPDEFS:%=$(SYM)%) $(INCLUDE:%=$(INC)%) $(CCFLAGS) $(CXXFLAGS)
	@echo " "

clean:
	$(RMDIR) $(GOALS:%=%/$(BTYPE))
