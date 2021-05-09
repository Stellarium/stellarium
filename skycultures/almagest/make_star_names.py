import sys
# HIP           SAO     HD      HR
CROSS_ID = '../../stars/default/cross-id.dat'
HIP = 0
HR = 4

def parse_cross_id(path):
    hr_to_hip = {}
    with open(path) as f:
        next(f)   # skip header line
        for line in f:
            parts = line.split('\t')
            hip = int(parts[HIP])
            hr = parts[HR].strip()
            if hr:
                hr_to_hip[int(hr)] = hip
            #breakpoint()
    return hr_to_hip

CAT_FILE = 'almstars/cat1.dat'
CAT_HR = slice(30, 34)
CAT_ID = slice(36, 48)
CAT_DESC = slice(48, None)
def parse_cat(path):
    hr_desc = []
    with open(path) as f:
        next(f)   # skip header line
        for line in f:
            hr = int(line[CAT_HR].strip())
            modern_id = line[CAT_ID].strip()
            desc = line[CAT_DESC].strip()
            yield hr, modern_id, desc

hr_to_hip = parse_cross_id(CROSS_ID)

for hr, modern_id, desc in parse_cat(CAT_FILE):

    try:
        hip = hr_to_hip[hr]
        print(f'{hip}|_("{desc}")')
    except KeyError:
        print(f'No HIP number found for HR {hr} ({modern_id})', file=sys.stderr)