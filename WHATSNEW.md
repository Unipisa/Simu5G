# What's New in Simu5G

## v1.3.0 (2025-01-16)

This version introduces a range of updates focused on streamlining module access, improving parameter handling, optimizing code structure, and enhancing documentation. This release eliminates deprecated elements, refactors constructors, and adopts modern C++ practices for better efficiency and maintainability. The update also addresses multiple bug fixes, including initialization, memory leaks, and signal registration issues.

**Key Enhancements:**

- Improved organization and clarity of include directives.
- Transitioned to the use of network nodes instead of deprecated functions like `getAncestorPar()`.
- Refactor with range-based for loops, structured bindings, and auto type iterators.
- Significant code cleaning, including the removal of unused parameters, variables, and members.
- Adopted modern C++ practices, such as using `nullptr` and `static_cast`, and optimizing loop and destructor usage.
- Enhanced documentation through comprehensive reviews.

**Bug Fixes:**

- Fixed initialization issues leading to potential segfaults.
- Addressed memory leaks and corrected signal registration issues.
- Corrected conditions causing reassociation failure and errors in data packet handling.

**Module and Parameter Refactoring:**

- Simplified parameter access by using `networkNode->par()`.
- Introduced NED parameters for dynamic module access, replacing hardcoded paths.
- Consolidated redundant module initialization and parameter processing methods.

**Optimizations and Quality Improvements:**

- Consolidated module access and parameter passing for efficiency.
- Implemented static signal registrations for cleaner code.
- Removed unwanted clear() calls and refined vector initializations.

**Additional Changes:**

- Added D2D specific `rcvdSinr` statistics and improved logging formats.
- Removed obsolete code, statistics, and unused functions.
- Enhanced modularity and readability of simulation examples.
- Implemented detailed module documentation for better user guidance.

Developers and users are encouraged to upgrade to this latest version to benefit from these cumulative improvements, ensuring a more optimized and error-free simulation environment.

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
