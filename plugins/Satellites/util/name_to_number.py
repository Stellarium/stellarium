# This Python script reads a satellites.json file in the old format
# (using names for satellite IDs) and ouputs a file that uses catalog numbers
# as satellite IDs.
# --Bogdan Marinov

import json;

# Open the satellites.json file and read it as a hash
old_f = open('satellites.json')
old_json = json.load(old_f)
old_f.close()

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
new_f = open('satellites_with_numbers.json', 'w')
json.dump(old_json, new_f, indent = 4)
new_f.close()
