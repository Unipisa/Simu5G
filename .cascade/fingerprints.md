# Fingerprint tests

## Using the fingerprint tests

- Use the provided "fingerprint test" suite for testing. The command is this:
  `(cd tests/fingerprint/ && ./fingerprints -d)`. It runs all simulations in the
  project (in `examples/`, `showcases/`, etc) for a configured time limit, and
  verifies that they produce the same simulation fingerprints as the recorded
  ones. Runtime errors and fingerprint validation failures are reported.
  Simulations and fingerprints are stored in csv files (`simulations.csv`). See
  OMNeT++ manual for what simulation fingerprints are. Running the full fingerprint
  test takes about a few minutes on a contemporary laptop.

- Filtering: Add `-m <substring>` args to filter for a subset of simulations that
  include any of the substrings (in the config name, directory name, or elsewhere).
  Such partial fingerprint tests are useful for saving time between intermediate
  refactoring steps, for example. (After completing the refactoring, running the
  full fingerprint test suite is required.)

- Errors and fingerprint failures are NOT ACCEPTABLE (except when expected).

## Checking fingerprint test results

Run the fingerprint tests and redirect output to a log file:
```
(cd tests/fingerprint/ && ./fingerprints -d) 2>&1 | tee fp_stdout.log
```

The log file `fp_stdout.log` is the ONLY output you need to look at.

### 1. Check the summary line at the end

```
tail -10 fp_stdout.log
```

It prints one of:
```
Ran 127 tests in 167.938s

OK
```
or:
```
Ran 127 tests in 167.938s

FAILED (errors=7)
```
or:
```
Ran 127 tests in 167.938s

FAILED (failures=3, errors=2)
```

- **errors** = simulations that crashed or produced runtime errors. NEVER acceptable.
- **failures** = fingerprint mismatches. Only acceptable if the mismatch is
  an expected consequence of the change (e.g. `sz` changing after removing
  a module).
- The result must be `OK` or `FAILED (failures=N)` with zero errors.
  Any errors mean there is a bug to fix before proceeding.

### 2. If there are errors

Search the log for the error lines:
```
grep 'ERROR (should be PASS)' fp_stdout.log
```

Each matching line contains the simulation directory, command-line args,
and the full error message. No need to look at surrounding context.
If you need a stack trace (e.g. for crashes), re-run the simulation manually.
Fix all errors before proceeding.

### 3. If there are only failures (fingerprint mismatches)

The log shows which fingerprint ingredients differ for each failing
simulation. Verify the mismatches are expected consequences of the change.
Then update fingerprints:
```
cp tests/fingerprint/simulations.csv.UPDATED tests/fingerprint/simulations.csv
```

### 4. DO NOT update fingerprints if there are any errors. Fix the errors first.

### 5. DO NOT look at .ERROR or .FAILED files. Use fp_stdout.log solely.

## What are fingerprints?

- FINGERPRINTS: 32-bit integer, computed by taking the specified "ingredients"
  into account. Ingredients are the parts after the slash. E.g.:
  `ce26-3758/tplx;5d56-5fa2/~tNl;2251-2948/sz`  contains 3 fingerprints
  (separated by semicolons), and the letters in `tplx`, `~tNl`, `sz` are all
  "ingredients". `t` is simulation time, `p` is module path, `l` is packet
  length, `s` is scalar results, etc. For more info on fingerprints mechanism
  (multiple fingerprints, fingerprint ingredients, etc), see the OMNeT++ manual.

## Fingerprint ingredients recognized by OMNeT++

See Testing chapter in the OMNeT++ manual.

  - "e": event number
  - "t": simulation time
  - "n": message (event) full name
  - "c": message (event) class name
  - "k": message kind
  - "l": message (packet) bit length
  - "o": message control info class name
  - "g": message control info (uses parsimPack())
  - "d": message data (legacy, DO NOT USE)* (uses parsimPack())
  - "b": message contents (uses parsimPack())
  - "i": module id
  - "m": module full name (name with index)
  - "p": module full path (hierarchical name)
  - "a": module class name
  - "r": random numbers drawn
  - "s": scalar results (just the data, without name or module path)
  - "z": statistic results: histogram, statistical summary (just the data, without name or module path)
  - "v": vector results (just the data, without name or module path); has no effect if vector recording is disabled
  - "y": display strings of all modules (on refreshDisplay())
  - "f": essential properties (geometry) of canvas figures (on refreshDisplay())
  - "x": extra data provided by modules
  - "0": clean hasher

## Fingerprint ingredients added by INET:

These are implemented in the inet::FingerprintCalculator class.

  - "~"	NETWORK_COMMUNICATION_FILTER: Excludes intra-node communication; only
    includes packet arrivals crossing different network nodes.

  - "U"	PACKET_UPDATE_FILTER: Excludes packet update arrival events (e.g.,
    transmission truncations) from fingerprint calculation.

  - "N"	NETWORK_NODE_PATH: Adds full paths of sender and arrival network nodes for
    packet arrival events.

  - "I"	NETWORK_INTERFACE_PATH: Adds full interface paths of sender and arrival
    network interfaces for packet arrival events.

  - "D"	PACKET_DATA:	Includes raw packet data bytes/bits in the fingerprint for
    non-zero-length packets. Requires packet serialization, and forces checksum
    computation. It cannot be used if the packet contains chunks that do not
    have serializers.

## Typical fingerprint ingredient combinations:

  - "tplx": Simulation time, module path, and packet length. Includes module
    path, so it is sensitive to module renames. But, since it does NOT include
    module IDs, it is NOT sensitive to the addition or deletion of modules that
    have no events in them, and to module relocations, submodule order changes.
    To validate module renames, use "tilx".

  - "tilx": Simulation time, module ID, and packet length. Uses module ID
    instead of module path, so it is insensitive to module renames, but it is
    sensitive to changes in model structure: adding/deleting or relocating
    modules, and also to adding/deleting connections with channel objects
    (channel objects also occupy IDs). Note: module IDs are actually
    *component* IDs (modules and channels). Adding or removing parameters,
    gates, and connections without channel objects do NOT affect component IDs.
    (One subtleity: connection display strings are stored in channel objects,
    so adding `@display` to an otherwise plain connection DOES shift
    module IDs.). Note that tplx and tilx are somewhat complementary.

  - "sz": Scalar and histogram/statistical summary results (ignores vector
    results). The hash itself does not explicitly include the module path or
    statistic name, just the numerical value. It is sensitive the
    addition/deletion of statistics (so it may also easily change when
    switching to a different INET version). It is also sensitive to changes in
    the order results are recorded (e.g. to changes in module order).

  - "tyf": Graphical fingerprint: display strings and figure properties. It is
    useful for testing model visualization. Since display strings and other
    visualizations (e.g. canvas figures) are usually updated from
    updateDisplay() methods in the model, the fingerprint runner test will turn
    on Cmdenv's "fake GUI" feature for these fingerprints, that generates
    updateDisplay() calls in the model.

  - "~tNl": Network-level traffic without packet data (packet lengths only). It
    is insensitive to events inside network nodes: as long as the network-level
    traffic stays the same, the fingerprint will not change. It is useful for
    validating restructuring changes inside network nodes.

  - "~tND": Network-level traffic including packet data. This one is like
    "~tNl", but stricter, and also has a larger runtime cost. It works via
    packet serialization. The fingerprint runner script automatically enables
    computed checksums/CRCs/FCS for this fingerprint, as it is needed for
    serialization. It does not work if some packet chunk does not have a
    serializer.

General advice:

  - When refactoring, it is advisable to break down the changes to steps that
    are independently testable, and break only one type of fingerprints. For
    example, if possible at all, one change should change either "tplx" or
    "tilx", but not both at the same time.

  - After accepting the new fingerprint results by overwriting the
    simulations.csv file with the new ones, it is not necessary to run the full
    fingerprint tests again, just to be safe (it takes too much time.) You can
    run it for a few selected simulations to be sure (use the `-m` switch to
    filter).
