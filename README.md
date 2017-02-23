# mq-cph
The IBM MQ C Performance Harness

# Overview

CPH (the IBM MQ C Performance Harness) is a performance test tool for [IBM® MQ](http://www-03.ibm.com/software/products/en/ibm-mq).  The source can be found on the [ibm-messaging GitHub](https://ibm-messaging.github.io/mq-cph).

# Build
After extracting the code from this repository, you can build an image with the latest version of MQ using the following command:

```
make
```
On Microsoft Windows, you can use the included VC++ 2005 project.

On Linux disributions, g++ is required. After running make, the cph executable will be built in the local directory. You may need to copy it to a folder in your executable path to be able to run cph from another path. The properties files in the 'props' directory also need to accessible in the same directory as the cph executable, or as defined by the CPH_INSTALLDIR environment variable.

Before running make, make sure the INCLUDE environment variable is configured with the path to the MQ include headers directory. On Windows this would be something like "C:\Program Files\IBM\WebSphere MQ\Tools\c\include", on Linux it would be something like "/opt/mqm/inc".

# Usage

Please see ([cph.pdf](cph.pdf))

# Troubleshooting

Please see ([cph.pdf](cph.pdf))

# Issues and contributions

For issues relating specifically to CPH, please use the [GitHub issue tracker](https://github.com/ibm-messaging/mq-cph/issues). For more general issues relating to IBM MQ, please use the [messaging community](https://developer.ibm.com/answers/?community=messaging). If you do submit a Pull Request related to this source code, please indicate in the Pull Request that you accept and agree to be bound by the terms of the [IBM Contributor License Agreement](CLA.md).

# License

The CPH source code and associated scripts are licensed under the [Apache License 2.0](./LICENSE).
