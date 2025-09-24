#!/usr/bin/env python3

import csv
import subprocess
import os
import re
import sys

def get_num_runs(working_dir, ini_file, config):
    """Get the number of runs for a given simulation configuration."""
    try:
        # Convert absolute path to relative path
        if working_dir.startswith('/'):
            working_dir = working_dir[1:]  # Remove leading slash

        # Change to working directory and run opp_run command
        cmd = f"cd {working_dir} && opp_run -S -q numruns -f {ini_file} -c {config}"
        result = subprocess.run(cmd, shell=True, capture_output=True, text=True, cwd=".")

        if result.returncode == 0:
            return int(result.stdout.strip())
        else:
            print(f"Warning: Could not get numruns for {working_dir} -c {config}: {result.stderr}")
            return 1  # Default to 1 run if we can't determine
    except Exception as e:
        print(f"Error getting numruns for {working_dir} -c {config}: {e}")
        return 1

def parse_args(args_str):
    """Parse the args string to extract ini file and config."""
    # Look for -f <inifile> and -c <config> patterns
    ini_match = re.search(r'-f\s+(\S+)', args_str)
    config_match = re.search(r'-c\s+(\S+)', args_str)

    ini_file = ini_match.group(1) if ini_match else "omnetpp.ini"
    config = config_match.group(1) if config_match else "General"

    return ini_file, config

def create_run_specific_args(original_args, run_number):
    """Create new args string with specific run number."""
    # Remove existing -r argument if present
    args_without_r = re.sub(r'-r\s+\d+', '', original_args)
    # Add the new run number
    return f"{args_without_r.strip()} -r {run_number}"

def extend_simulations_csv(input_file, output_file):
    """Extend the simulations.csv file to include all runs."""
    extended_lines = []

    with open(input_file, 'r') as file:
        lines = file.readlines()

    for line_num, line in enumerate(lines, start=1):
        original_line = line.rstrip('\n\r')

        # Keep header and comment lines as-is
        if original_line.startswith('#'):
            extended_lines.append(original_line)
            continue

        if not original_line.strip():  # Skip empty lines
            continue

        # Parse the line manually to preserve formatting
        parts = original_line.split(',')
        if len(parts) < 5:
            print(f"Warning: Skipping line {line_num} - insufficient columns")
            extended_lines.append(original_line)
            continue

        working_dir = parts[0].strip()
        original_args = parts[1].strip()
        simtime_limit = parts[2].strip()
        original_fingerprint = parts[3].strip()
        expected_result = parts[4].strip()
        tags = parts[5].strip() if len(parts) > 5 else ""

        # Parse the arguments to get ini file and config
        ini_file, config = parse_args(original_args)

        if not config:
            print(f"Warning: Could not extract config from args: {original_args}")
            extended_lines.append(original_line)
            continue

        print(f"Processing {working_dir} -c {config}")

        # Get number of runs for this configuration
        num_runs = get_num_runs(working_dir, ini_file, config)

        # Generate entries for all runs
        for run in range(num_runs):
            new_args = create_run_specific_args(original_args, run)

            # For run 0, use the original line if it already has "-r 0"
            if run == 0 and "-r 0" in original_args:
                # This is the original run 0, keep the original line exactly
                extended_lines.append(original_line)
            else:
                # New run, use the original fingerprint (preserve the valuable suffix parts)
                fingerprint = original_fingerprint
                # Create new line based on original formatting pattern
                formatted_line = format_csv_line_like_original(original_line, working_dir, new_args, simtime_limit, fingerprint, expected_result, tags)
                extended_lines.append(formatted_line)

        print(f"  Generated {num_runs} runs")

    # Write the extended CSV
    with open(output_file, 'w') as file:
        for line in extended_lines:
            file.write(line + '\n')

    print(f"Extended CSV written to {output_file}")
    print(f"Generated {len(extended_lines) - 1} total simulation entries")


def format_csv_line_like_original(original_line, working_dir, args, simtime_limit, fingerprint, expected_result, tags):
    """Format a CSV line to match the spacing of the original line."""
    # Find the positions of commas in the original line to understand spacing
    comma_positions = [i for i, char in enumerate(original_line) if char == ',']

    if len(comma_positions) < 4:
        # Fallback to simple formatting if we can't parse the original
        return f"{working_dir}, {args}, {simtime_limit}, {fingerprint}, {expected_result}," + (f" {tags}" if tags else "")

    # Extract the spacing patterns from the original line
    # Position after first comma (workingdir,)
    pos_after_workingdir = comma_positions[0] + 1

    # Position after second comma (args,)
    pos_after_args = comma_positions[1] + 1

    # Position after third comma (simtime_limit,)
    pos_after_simtime = comma_positions[2] + 1

    # Position after fourth comma (fingerprint,)
    pos_after_fingerprint = comma_positions[3] + 1

    # Build the new line with the same spacing pattern
    result = working_dir + ","

    # Calculate spaces needed to match original args column position
    spaces_before_args = pos_after_workingdir - len(result)
    result += " " * spaces_before_args + args + ","

    # Calculate spaces needed to match original simtime_limit column position
    spaces_before_simtime = pos_after_args - len(result)
    result += " " * spaces_before_simtime + simtime_limit + ","

    # Calculate spaces needed to match original fingerprint column position
    spaces_before_fingerprint = pos_after_simtime - len(result)
    result += " " * spaces_before_fingerprint + fingerprint + ","

    # Calculate spaces needed to match original expected_result column position
    spaces_before_result = pos_after_fingerprint - len(result)
    result += " " * spaces_before_result + expected_result + ","

    # Add tags if present
    if tags:
        result += f" {tags}"

    return result

def main():
    input_file = "tests/fingerprint/simulations.csv"
    output_file = "tests/fingerprint/simulations_extended.csv"

    if not os.path.exists(input_file):
        print(f"Error: Input file {input_file} not found")
        sys.exit(1)

    print(f"Extending {input_file} to include all runs...")
    extend_simulations_csv(input_file, output_file)
    print("Done!")

if __name__ == "__main__":
    main()
