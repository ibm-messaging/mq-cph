# mq-cph
The IBM MQ C Performance Harness

# Overview

MQ-CPH (the IBM MQ C Performance Harness) is a performance test tool for [IBM® MQ](http://www-03.ibm.com/software/products/en/ibm-mq).  The source can be found on the [ibm-messaging GitHub](https://ibm-messaging.github.io/mq-cph).

# Build
After extracting the code from this repository, you can build an image with the latest version of MQ (and copy the executable and all required property files to an installation directory, e.g. ~/cph) using the following commands:

```
export installdir ~/cph
make
```
On Microsoft Windows, you can use the included VC++ 2005 project.

On Linux disributions, g++ is required. After running make, the cph executable will be built in the local directory (./Release/...). If you specified an 'installdir' then the cph executable and the required property files (in the 'props' directory) will have been copied to that directory, otherwise you need to copy them yourself. 
The cph executable needs to be run from the directory in which it is 'installed' to access the property files, unless the CPH_INSTALLDIR environment variable is set to indicate their location.

Before running make, make sure the INCLUDE environment variable is configured with the path to the MQ include headers directory. On Windows this would be something like "C:\Program Files\IBM\WebSphere MQ\Tools\c\include", on Linux it would be something like "/opt/mqm/inc".

# Usage

Please see ([cph.pdf](cph.pdf))

# Troubleshooting

Please see ([cph.pdf](cph.pdf))

# Issues and contributions

For issues relating specifically to CPH, please use the [GitHub issue tracker](https://github.com/ibm-messaging/mq-cph/issues). For more general issues relating to IBM MQ, please use the [messaging community](https://developer.ibm.com/answers/?community=messaging). If you do submit a Pull Request related to this source code, please indicate in the Pull Request that you accept and agree to be bound by the terms of the [IBM Contributor License Agreement](CLA.md).

# License

The CPH source code and associated scripts are licensed under the [Apache License 2.0](./LICENSE).
