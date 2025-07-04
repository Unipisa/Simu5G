#!/usr/bin/env python3
"""
Python rewrite of gen_runallexamples.pl
Generates fingerprint test CSV entries by scanning .ini files for configurations.
"""

import os
import re
import sys
from pathlib import Path

def main():
    # Define the skip list - tests that should be commented out
    skiplist_text = """
/examples/emulation/extclient/omnetpp.ini  General                       # ext interface tests are not supported as they require pcap drivers and external events
/examples/emulation/extserver/omnetpp.ini  Uplink_Traffic                # ext interface tests are not supported as they require pcap drivers and external events
/examples/emulation/extserver/omnetpp.ini  Downlink_Traffic              # ext interface tests are not supported as they require pcap drivers and external events
/examples/emulation/extserver/omnetpp.ini  Uplink_and_Downlink_Traffic   # ext interface tests are not supported as they require pcap drivers and external events
/examples/emulation/traceroute/omnetpp.ini  General                      # ext interface tests are not supported as they require pcap drivers and external events
/examples/ethernet/lans/defaults.ini  General                            # <!> Error: Network `' or `inet.examples.ethernet.lans.' not found, check .ini and .ned files. # The defaults.ini file included from other ini files
"""

    # Parse skip list
    skiplist = {}
    for line in skiplist_text.strip().split('\n'):
        line = line.strip()
        if not line:
            continue

        # Extract comment part
        comment_match = re.search(r'#\s*(.*)$', line)
        comment = '# ' + comment_match.group(1) if comment_match else ''

        # Remove comment from line
        line = re.sub(r'\s*#.*', '', line)
        if line:
            skiplist[line] = comment

    # Get script directory and change to project root
    script_dir = Path(__file__).parent
    project_root = script_dir / "../.."
    os.chdir(project_root.resolve())

    # Find all .ini files in specific directories only
    target_dirs = ['simulations', 'showcases', 'tutorials']
    ini_files = []

    for target_dir in target_dirs:
        if os.path.exists(target_dir):
            for root, dirs, files in os.walk(target_dir):
                for file in files:
                    if file.endswith('.ini'):
                        ini_files.append(os.path.join(root, file))

    ini_files.sort()

    if not ini_files:
        print("Not found ini files", file=sys.stderr)
        sys.exit(1)

    print("# working directory, command line arguments, simulation time limit, fingerprint, expected result, tags")

    # Process each .ini file
    for fname in ini_files:
        try:
            with open(fname, 'r', encoding='utf-8', errors='ignore') as f:
                content = f.read()
        except Exception as e:
            print(f"# Error reading {fname}: {e}", file=sys.stderr)
            continue

        # Find all [Config ...] sections
        config_matches = re.findall(r'^\s*(\[Config [a-zA-Z_0-9-]+\].*?)$', content, re.MULTILINE)

        # If no configs found, use General as default
        if not config_matches:
            config_matches = ["[Config General]"]

        # Find extends clauses to identify extended configs
        extends = {}
        extends_matches = re.findall(r'^\s*extends\s*=\s*([^#\n]+)\s*(?:#.*)?$', content, re.MULTILINE)
        for extends_line in extends_matches:
            extends_line = extends_line.strip()
            # Split by comma and process each extended config
            for item in re.split(r'\s*,\s*', extends_line):
                item = item.strip()
                if item:
                    extends[item] = f"# {item} extended"

        # Process each config
        for config_line in config_matches:
            # Extract config name and comment
            config_match = re.match(r'^\[Config ([a-zA-Z_0-9-]+)\]\s*(#.*)?$', config_line)
            if not config_match:
                continue

            cfg = config_match.group(1)
            comment = config_match.group(2) or ""

            # Extract directory and filename
            dir_path, filename = os.path.split(fname)
            if dir_path.startswith('./'):
                dir_path = dir_path[2:]

            # Build the CSV line - keep the ./ prefix like the original Perl script
            working_dir = f"/./{dir_path}/" if dir_path else "/./"
            working_dir_padded = working_dir + ' ' * max(0, 36 - len(working_dir))

            command_args = f"-f {filename} -c {cfg} -r 0"
            command_padded = command_args + ',' + ' ' * max(0, 83 - len(working_dir_padded + command_args + ','))

            time_limit = "---100s,"
            time_padded = time_limit + ' ' * max(0, 100 - len(working_dir_padded + command_padded + time_limit))

            fingerprint = "0000-0000/tplx, "
            result = "PASS,"

            run_line = working_dir_padded + command_padded + time_padded + fingerprint + result

            # Check if this should be skipped or commented
            skip_key = f"/{dir_path}/{filename}  {cfg}"

            if "__interactive__" in comment.lower():
                print(f"# {run_line}   # {config_line}")
            elif cfg in extends:
                print(f"# {run_line}   {extends[cfg]}")
            elif skip_key in skiplist:
                print(f"# {run_line}   {skiplist[skip_key]}")
            else:
                print(run_line)

if __name__ == "__main__":
    main()
