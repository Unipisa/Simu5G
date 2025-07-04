#!/usr/bin/env python3
"""
Python rewrite of gen_runallexamples.pl
Generates fingerprint test CSV entries by scanning .ini files for configurations.
Only prints simulations that are NOT listed in the provided CSV files.
"""

import argparse
import csv
import os
import re
import sys
from pathlib import Path

def parse_existing_csv_files(csv_files):
    """
    Parse CSV files and extract (dir, args) pairs for comparison.
    Returns a set of (working_dir, args) tuples.
    """
    existing_simulations = set()

    for csv_file in csv_files:
        if not os.path.exists(csv_file):
            print(f"# Warning: CSV file {csv_file} not found", file=sys.stderr)
            continue

        try:
            with open(csv_file, 'r', encoding='utf-8', errors='ignore') as f:
                for line_num, line in enumerate(f, 1):
                    line = line.strip()
                    # Skip empty lines and comment lines
                    if not line or line.startswith('#'):
                        continue

                    # Split by comma and extract first two columns
                    parts = [part.strip() for part in line.split(',')]
                    if len(parts) >= 2:
                        working_dir = parts[0].strip()
                        args = parts[1].strip()
                        existing_simulations.add((working_dir, args))

        except Exception as e:
            print(f"# Error reading CSV file {csv_file}: {e}", file=sys.stderr)

    return existing_simulations

def main():
    # Parse command line arguments
    parser = argparse.ArgumentParser(
        description='Generate fingerprint test CSV entries, excluding simulations already in provided CSV files'
    )
    parser.add_argument('csv_files', nargs='*',
                       help='CSV files containing existing simulations to exclude')

    args = parser.parse_args()

    # Parse existing CSV files if provided
    existing_simulations = set()
    if args.csv_files:
        existing_simulations = parse_existing_csv_files(args.csv_files)
        print(f"# Loaded {len(existing_simulations)} existing simulations from {len(args.csv_files)} CSV file(s)", file=sys.stderr)
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

    # Collect all results for sorting and count filtered simulations
    all_results = []
    filtered_count = 0

    # Process each .ini file
    for fname in ini_files:
        try:
            with open(fname, 'r', encoding='utf-8', errors='ignore') as f:
                content = f.read()
        except Exception as e:
            print(f"# Error reading {fname}: {e}", file=sys.stderr)
            continue

        # Find all [Config ...] or [...] sections
        config_matches = re.findall(r'^\s*(\[(?:Config )?[a-zA-Z_0-9-]+\].*?)$', content, re.MULTILINE)

        # If no configs found, use General as default
        if not config_matches:
            config_matches = ["[Config General]"]

        # Find abstract configs to identify configs that should be skipped
        abstract_configs = set()

        # Find which config sections contain abstract markers
        config_sections = re.split(r'^\s*\[(?:Config )?[a-zA-Z_0-9-]+\]', content, flags=re.MULTILINE)
        config_names = re.findall(r'^\s*\[(?:Config )?([a-zA-Z_0-9-]+)\]', content, re.MULTILINE)

        # Check each config section for abstract markers
        for i, section in enumerate(config_sections[1:], 0):  # Skip first empty section
            if i < len(config_names):
                config_name = config_names[i]
                # Check for 'abstract = true' or '#abstract-config = true' patterns only
                if (re.search(r'^\s*abstract\s*=\s*true\s*(?:#.*)?$', section, re.MULTILINE | re.IGNORECASE) or
                    re.search(r'^\s*#\s*abstract-config\s*=\s*true\s*(?:#.*)?$', section, re.MULTILINE | re.IGNORECASE)):
                    abstract_configs.add(config_name)

        # Process each config
        for config_index, config_line in enumerate(config_matches):
            # Extract config name and comment
            config_match = re.match(r'^\[(?:Config )?([a-zA-Z_0-9-]+)\]\s*(#.*)?$', config_line)
            if not config_match:
                continue

            cfg = config_match.group(1)
            comment = config_match.group(2) or ""

            # Extract directory and filename
            dir_path, filename = os.path.split(fname)
            if dir_path.startswith('./'):
                dir_path = dir_path[2:]

            # Build the CSV line - keep the ./ prefix like the original Perl script
            working_dir = f"/{dir_path}/" if dir_path else "/"
            working_dir_padded = working_dir + ',' + ' ' * max(1, 36 - len(working_dir))

            command_args = f"-f {filename} -c {cfg} -r 0"
            command_padded = command_args + ',' + ' ' * max(0, 85 - len(working_dir_padded + command_args + ', '))

            time_limit = "---100s,"
            time_padded = time_limit + ' ' * max(0, 101 - len(working_dir_padded + command_padded + time_limit))

            fingerprint = "0000-0000/tplx, "
            result = "PASS,"

            run_line = working_dir_padded + command_padded + time_padded + fingerprint + result

            # Check if this simulation already exists in the provided CSV files
            simulation_key = (working_dir, command_args)
            if simulation_key in existing_simulations:
                # Skip this simulation as it already exists in the CSV files
                filtered_count += 1
                continue

            # Check if this should be skipped or commented
            skip_key = f"/{dir_path}/{filename}  {cfg}"

            # Determine the output line and whether it should be commented
            if "__interactive__" in comment.lower():
                output_line = f"# {run_line}   # {config_line}"
            elif cfg in abstract_configs:
                output_line = f"# {run_line}   # {cfg} is abstract"
            elif skip_key in skiplist:
                output_line = f"# {run_line}   {skiplist[skip_key]}"
            else:
                output_line = run_line

            # Add to results with sorting key (path, filename, original order in file)
            sort_key = (dir_path, filename, config_index)
            all_results.append((sort_key, output_line))

    # Sort results by path, filename, config name and print
    all_results.sort(key=lambda x: x[0])
    for _, output_line in all_results:
        print(output_line)

    # Report filtering statistics if CSV files were provided
    if args.csv_files and filtered_count > 0:
        print(f"# Filtered out {filtered_count} simulation(s) that were already present in the provided CSV file(s)", file=sys.stderr)

if __name__ == "__main__":
    main()
