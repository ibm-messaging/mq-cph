# mq-cph
The IBM MQ C Performance Harness

# Overview

MQ-CPH (the IBM MQ C Performance Harness) is a performance test tool for [IBM® MQ](http://www-03.ibm.com/software/products/en/ibm-mq).

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

# Sample File

samples/rr_1 contains files that can be used to setup a queue manager for use with requester/responder tests.

Note that if you are using these scripts to setup a queue manager for use with the MQ-CPH blog article
["MQ-CPH Performance Harness Released on GitHub"](https://www.ibm.com/developerworks/community/blogs/messaging/entry/MQ_C_Performance_Harness_Released_on_GitHub?lang=en)
then you should use the cph command lines in samples/rr_1/ReadMe, instead of the ones in the blog article. The queue manager setup has been changed to require a userid/password on the client connected requesters.  

# Troubleshooting

Please see ([cph.pdf](cph.pdf))

# Issues and contributions

For issues relating specifically to CPH, please use the [GitHub issue tracker](https://github.com/ibm-messaging/mq-cph/issues). For more general issues relating to IBM MQ, please use the [messaging community](https://developer.ibm.com/answers/?community=messaging). If you do submit a Pull Request related to this source code, please indicate in the Pull Request that you accept and agree to be bound by the terms of the [IBM Contributor License Agreement](CLA.md).

# License

The CPH source code and associated scripts are licensed under the [Apache License 2.0](./LICENSE).
