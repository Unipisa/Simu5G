# Simu5G Development Rules

## Coding Guidelines

- EXPLICIT ERRORS PREFERRED: When the code meets an unexpected condition,
  report it as an error (assertion or throw) rather than silently ignore or
  sidestep it with making the affected code block conditional on the inputs
  being OK, or using early returns. For example, if a pointer argument is
  nullptr, throw an exception or assert, do not just skip dereferencing it by
  surrounding the code with `if (p != nullptr)` statements.

- ASSERT VS EXCEPTION: Assertions are for ensuring assumptions are valid,
  exceptions are for reporting invalid input or state.

- EXCEPTION TYPE: Prefer cRuntimeError which has a sprintf-like argument list.
  Include relevant values in the error message. (Usually no need to include
  the enclosing class name, as the path and type of the OMNeT++ module "in context"
  will be part of the error message).

- Use `ASSERT()` (of OMNeT++) instead of `assert()`.

- Use `check_and_cast()` instead of `dynamic_cast()`.

- Do not **EVER** add static variables, unless explicitly requested by the user.

## Common Pitfalls:

- Msg files: In msg file generated code, the namespace begins where the
  namespace directive occurs in the MSG file. It does NOT apply to the whole file.

- If a simple module (which does not extend another) has no `@class`, that does
  NOT mean that the module will be a dummy that does nothing. The default is to
  look for a C++ class with the same name as the NED type (moduloo the
  namespace) -- if it does not exist, that will be an error.

## Nits:

- Do not use "—" (m-dash?) in code comments or code, use plain hyphen (-)
  instead. Similarly, avoid using fancy Unicode characters if plain ASCII can
  also do the job. For example, write "->" and ":)" instead of Unicode arrow and
  smiley chars.

## Compiling

- Use debug builds with `-j`: `make MODE=debug -j12`.

## Be Critical

- Bring up criticism without explicit request from the user! The user is INTERESTED
  in what is wrong in the code!

- BE CRITICAL. Do not assume the code in the original project is always correct!
  Report misnamed functions and variables (where the name does not much the
   functionality) and suspicious (potentially buggy) code fragments.

- INCORRECT MODEL. In case of a simulation model of a network protocol, report if
  the code obviously does NOT confirm to the standard (RFC, IEEE standard, etc).

- Report if you find code or functionality in a class that obviously does NOT
  belong there.

## OMNeT++ INI Syntax

- **Multi-line values** use indentation-based line continuation. Any line that
  begins with whitespace is automatically treated as a continuation of the
  previous line's value — no backslash or other special syntax is needed.
  The closing bracket/parenthesis must also be indented, otherwise the parser
  treats it as a new (malformed) `key=value` line.

  ```ini
  *.gnb.cellularNic.qfiContextManager.drbConfig = [
      {"drb": 0, "ue": 2049, "qfiList": [1, 2]},
      {"drb": 1, "ue": 2049, "qfiList": [3, 4]}
      ]
  ```

- **`object` NED parameters**: Use bare `[...]` and `{...}` syntax directly
  in INI — do NOT wrap with `json()`. The `json()` function does not exist
  in OMNeT++ 6.x.

## Running Simulations

- Use `Cwd` parameter of `run_command` to set the directory — never use `cd`
  in the command itself.
- The INI file name is a positional argument (no `-f` flag).
- Use `-c <ConfigName>` to select the configuration section, `-u Cmdenv` for
  command-line execution.
- The `simu5g_dbg` binary is in `$PATH` — do not search for it or specify
  absolute paths.

  Example:
  ```
  simu5g_dbg -u Cmdenv -c VoIP-DL-MultiQfi omnetpp_drb.ini
  ```
