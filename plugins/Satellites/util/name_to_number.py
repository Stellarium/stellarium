#!/usr/bin/env python3
# This Python script reads a satellites.json file in the old format
# (using names for satellite IDs) and ouputs a file that uses catalog numbers
# as satellite IDs.
# --Bogdan Marinov

import argparse
import json
import sys
from pathlib import Path


def main():
    parser = argparse.ArgumentParser(
        description='This Python script reads a satellites.json file in the old format (using names for satellite IDs)'
                    ' and ouputs a file that uses catalog numbers as satellite IDs.')
    parser.add_argument(dest='input', type=Path, metavar='input', help='satellites.json file (default: %(default)s)')
    parser.add_argument(dest='output', type=Path, metavar='output',
                        help='satellites_with_numbers.json file (default: %(default)s)')

    args = parser.parse_args()

    input_file = Path(args.input)
    output_file = Path(args.output)
    if not input_file.exists():
        print(f"'{input_file}' do not exist.")
        sys.exit(1)

    # Open the satellites.json file and read it as a hash
    with open(input_file, mode='r') as reader:
        old_json = json.load(reader)

    # For each key/value pair:
    new_json = dict()
    for name, satellite in old_json['satellites'].items():
        if 'tle2' not in satellite:
            continue

        # The catalog number is the second number on the second line of the TLE
        catalog_num = satellite['tle2'].split(' ')[1]
        satellite['name'] = name
        if 'lastUpdated' in satellite:
            del satellite['lastUpdated']
        new_json[catalog_num] = satellite.copy()

    # Save the result hash as satellites_with_numbers.json
    old_json['satellites'] = new_json
    with open(output_file, 'w') as writer:
        json.dump(old_json, writer, indent=4)


if __name__ == '__main__':
    main()
