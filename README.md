# qtmgmt

## Preface

Note that qtmgmt is currently in the process of being developed for an initial release, the code is not considered stable, and must be built from source.

## About

qtmgmt is a cross-platform MCUmgr CLI utility, designed for issuing commands to devices, created in Qt 6 (and supporting Qt 5, though Qt 6 is recommended).

## Features

* Uses MCUmgr plugin from AuTerm https://github.com/thedjnK/AuTerm/
* Single command mode
* Interactive mode to allow for running commands as you go or via pipes
* Supports transports:
  - UART
  - Bluetooth
  - UDP
  - LoRaWAN
* Supports MCUmgr groups:
  - Image management
  - Filesystem management
  - OS management
  - Statistic management
  - Shell management
  - Settings management
  - Zephyr basic management
  - Enumeration management

Functionality can be disabled in custom builds by uncommenting the SKIP lines in ``qtmgmt-includes.pri``, which allows for lean and reduced size builds.

## Downloading

Test builds of unstable work-in-progress versions are provided by CI jobs currently:

 * [Linux (x86_64)](https://github.com/thedjnK/AuTerm-Build/actions/workflows/qtmgmt-linux.yml)
 * [Windows (x86_64)](https://github.com/thedjnK/AuTerm-Build/actions/workflows/qtmgmt-windows.yml)
 * [mac (x86_64 and arm64)](https://github.com/thedjnK/AuTerm-Build/actions/workflows/qtmgmt-mac.yml)

## Help and contributing

Users are welcome to open issues and submit pull requests to have features merged. PRs on github should target the `main` branch, PRs on the internal git server should target the `develop` branch.

## Compiling

For details on compiling, please refer to [the wiki](https://github.com/LairdCP/UwTerminalX/wiki/Compiling).

## License

qtmgmt is released under the [GPLv3 license](https://github.com/thedjnK/qtmgmt/blob/master/LICENSE). In future, the code may be available under LGPL (or other) licenses.
