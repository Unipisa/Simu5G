# What's New in Simu5G

## v1.3.0 (2025-02-06)

- Compatible with **OMNeT++ 6.1.0** and **INET 4.5.4**
- New modules: MultiUEMECApp, MecRnisTestApp, UeRnisTestApp
- Added NED documentation for modules
- Increased reusability of modules via changes such as replacing hardcoded module
  paths in the C++ code with NED parameters (binderModule, macModule, etc.), and
  elimination of ancestorPar() calls by introducing local parameters instead.
- Other NED adjustments, such as removal of unused NED parameters and splitting
  NED files to have one module per file. See doc/NED-changes.txt for details.
- Extensive C++ modernization, and adaption of more OMNeT++ best practices.
- Various bug fixes.

## v1.2.3 (2025-01-10)

- Added support for **OMNeT++ 6.1.0** and **INET 4.5.4**.

## v1.2.2 (2023-04-19)

- Compatible with **OMNeT++ 6.0.1** and **INET 4.5**.
- Tested on Ubuntu 22.04 and macOS Ventura.

## v1.2.1 (2022-07-19)

- Compatible with **OMNeT++ 6.0** and **INET 4.4.0**.
- Tested on Ubuntu 20.04.
- Modifications to support the latest versions of OMNeT++ 6.0 and INET v4.4.0.
- Refactoring of simulation and emulation folders.
- Various bug fixes.

## v1.2.0 (2021-08-30)

- Compatible with **OMNeT++ 6.0 (pre10 and pre11)** and **INET 4.3.2**.
- Tested on Ubuntu 16.04, 18.04, 20.04, macOS Catalina, and Windows 7.
- Added modelling of **ETSI MEC** entities.
- Support for **real-time emulation capabilities** (Linux OS only).
- Modelling of background cells and background UEs for **larger scale** simulations and emulations.
- Several bug fixes.

## v1.1.0 (2021-04-16)

- Compatible with **OMNeT++ 5.6.2** and **INET 4.2.2**.
- Tested on Ubuntu 16.04, 18.04, 20.04, macOS Catalina, and Windows 7.
