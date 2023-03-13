# mq-cph
The IBM MQ C Performance Harness

# Overview

MQ-CPH (the IBM MQ C Performance Harness) is a performance test tool for [IBM® MQ](http://www-03.ibm.com/software/products/en/ibm-mq).  The source can be found on the [ibm-messaging GitHub](https://ibm-messaging.github.io/mq-cph).

# Build
After extracting the code from this repository, you can build an image with the latest version of MQ (and copy the executable and all required property files to an installation directory, e.g. ~/cph)

### Make Pre-requisites
Before running make, make sure the MQ header files are available. For Unix platforms, the makefile will assume they are in $MQ_INSTALLATION_DIR/inc or /opt/mqm/inc (if MQ_INSTALLATION_DIR is not defined). Alternatively you can set the 'INCLUDE' environment variable to a specific directory.

On Windows, the include directory wil be something like "C:\Program Files\IBM\WebSphere MQ\Tools\c\include"

Build, using the following commands, depending on platform:

#### Linux
```
export installdir=~/cph
make
```
On Linux disributions, g++ is required. 

#### AIX
```
export installdir ~/cph
gmake
```
AIX requires the GNU make tool, available here:
http://www-03.ibm.com/systems/power/software/aix/linux/toolbox/alpha.html

#### Windows
On Microsoft Windows, you can use the included VC++ 2005 project.

### Post Make Tasks
After running make, the cph executable will be built in the local directory (./Release/...). If you specified an 'installdir' then the cph executable and the required property files (in the 'props' directory) will have been copied to that directory, otherwise you need to copy them yourself. 
The cph executable needs to be run from the directory in which it is 'installed' to access the property files, unless the CPH_INSTALLDIR environment variable is set to indicate their location.


# Usage

Please see ([cph.pdf](cph.pdf))

# Sample Files

See ([MQ-CPH_Introduction.pdf](samples/MQ-CPH_Introduction.pdf)) in the samples directory, for an introduction to the tool, along with an example of running it.

### Running multiple mq-cph processes in parallel.  
A Python script [runcph_agg.py](samples/utilities/runcph_agg.py) can be used to spread threads across multiple mq-cph processes. The script will monitor and aggregate the output from multiple processes, reporting the overall message rate at runtime and on shutdown.

You can use it in two ways. E.g. to run 10 threads across 3 processes, either call it directly from the command line (note that you can leave the -nt parm on the mq-cph command, but it will be ignored. The number of threads is controlled by the -t parm of runcph_agg.py instead).
```
runcph_agg.py -c "./cph -vo 3 -nt 1 -ss 2 -ms 2048 -wt 10 -wi 0 -rl 0 -tx -pp -tc Requester -to 30 -iq REQUEST -oq REPLY -db 1 -dx 10 -dn 1 -jp 1414 -jc SYSTEM.DEF.SVRCONN -jb qm_name -jt mqc -jh qm_host" -t 10 -p 3
```

Or you can call it from another Python script:
```
#!/usr/bin/env python
import runcph_agg

runcph_agg.launch("./cph -vo 3 -ss 2 -ms 2048 -wt 10 -wi 0 -rl 0 -tx -pp -tc Requester -to 30 -iq REQUEST -oq REPLY -db 1 -dx 10 -dn 1 -jp 1414 -jc SYSTEM.DEF.SVRCONN -jb qm_name -jt mqc -jh qm_host", 10, 3, 0)
```

For command line usage:
```
runcph_agg.py -h
```

Similarly to executing mq-cph directly, the runcph_agg.py script needs to be run from the directory in which mq-cph is installed, to access the property files, unless the CPH_INSTALLDIR environment variable is set to indicate their location.

# Troubleshooting

Please see ([cph.pdf](cph.pdf))

# Docker

There is a git repo that can help develop a dockerized version of cph called [cphtestp](https://github.com/ibm-messaging/cphtestp)

# Issues and contributions

For issues relating specifically to CPH, please use the [GitHub issue tracker](https://github.com/ibm-messaging/mq-cph/issues). For more general issues relating to IBM MQ, please use the [messaging community](https://developer.ibm.com/answers/?community=messaging). If you do submit a Pull Request related to this source code, please indicate in the Pull Request that you accept and agree to be bound by the terms of the [IBM Contributor License Agreement](CLA.md).

# License

The CPH source code and associated scripts are licensed under the [Apache License 2.0](./LICENSE).
