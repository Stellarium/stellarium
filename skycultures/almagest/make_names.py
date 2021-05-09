import sys
STARS_FILE = 'star_names.fab'
DSO_FILE = 'dso_names.fab'
# HIP           SAO     HD      HR
CROSS_ID = '../../stars/default/cross-id.dat'
HIP = 0
HR = 4
DSO = {
    'NGC 869/884': ['NGC 869', 'NGC 884'],
    'NGC 2632 M44': ['M 44'],
    'NGC 5139': ['NGC 5139'],
}

def parse_cross_id(path):
    hr_to_hip = {}
    with open(path) as f:
        next(f, None)   # skip header line
        for line in f:
            parts = line.split('\t')
            hip = int(parts[HIP])
            hr = parts[HR].strip()
            if hr:
                hr_to_hip[int(hr)] = hip
    return hr_to_hip

CAT_FILE = 'almstars/cat1.dat'
CAT_HR = slice(30, 34)
CAT_ID = slice(35, 48)
CAT_DESC = slice(48, None)
def parse_cat(path):
    with open(path) as f:
        next(f, None)   # skip header line
        for line in f:
            hr = int(line[CAT_HR].strip())
            modern_id = line[CAT_ID].strip()
            desc = line[CAT_DESC].strip()
            yield hr, modern_id, desc

hr_to_hip = parse_cross_id(CROSS_ID)
with open(STARS_FILE, 'w') as stars, open(DSO_FILE, 'w') as dso:
    for hr, modern_id, desc in parse_cat(CAT_FILE):
        tag = hr_to_hip.get(hr)
        if tag is not None:
            print(f'{tag}|_("{desc}")', file=stars)
            continue
        tags = DSO.get(modern_id)
        if tags is not None:
            for tag in tags:
                print(f'{tag}|_("{desc}")', file=dso)
        else:
            print(f'No HIP number found for HR {hr} ({modern_id})', file=sys.stderr)