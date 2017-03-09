#!/usr/bin/perl
#
#*******************************************************************************
#  Copyright (c) 2007,2017 IBM Corp.
# 
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
# 
#     http://www.apache.org/licenses/LICENSE-2.0
# 
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
# 
#
#  Contributors:
#     Various members of the WebSphere MQ Performance Team at IBM Hursley UK.
# *******************************************************************************/

# Merge 2 qm.ini files (2nd file overrides first)

use strict;
use warnings;

#use lib '.';

#use Carp;
#use File::Spec::Functions;
#use File::Temp;
use Getopt::Long;
use MQConfigIni;
#use Perf::Env;
#use Perf::MQEnv;

my ($qm_ini_file1,$qm_ini_file2) = @ARGV;
if( !defined($qm_ini_file1) || !defined($qm_ini_file1) ){die "You must supply two qm.ini files to be merged\n";}


my @fns = ($qm_ini_file1, $qm_ini_file2);

my $qm = readMQConfigFile(@fns);
writeMQConfigFile($qm_ini_file1, $qm);

print ("QM .ini file $qm_ini_file1 updated with contents of $qm_ini_file2\n");


