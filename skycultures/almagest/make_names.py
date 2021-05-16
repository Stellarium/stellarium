import sys
PROPER_NAMES = {              #  HIP
    110: 'Arcturus',          # 69673
    149: 'Lyra',              # 91262
    222: 'Capella',           # 24608
    288: 'Aquila',            # 97649
    449: 'Praesepe',          # -----   (NGC 2632, M 44)
    452: 'Aselli 1',          # 42806   (Translated as 'asses' in a footnote.)
    453: 'Aselli 2',          # 42911
    469: 'Regulus',           # 49669
    509: 'Vindemiatrix',      # 63608   (In fn.: προτρυγητήρ 'Harbinger of Vintage'.)
    510: 'Spica',             # 65474   (In fn.: στάχυς, an ear of wheat or other cereal.)
    553: 'Antares',           # 80763
    818: 'the Dog',           # 32349
    848: 'Procyon',           # 37279   (before the dog)
    892: 'Canopus',           # 30438
}
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
CAT_CON = slice(5, 8)     # constellation abbreviation
CAT_IDX = slice(9, 11)    # index within constellation
CAT_HR = slice(30, 34)    # HR number
CAT_ID = slice(35, 48)    # modern Bayer/Flamsteed ID
CAT_DESC = slice(48, None)
def parse_cat(path):
    with open(path) as f:
        print(next(f, None).strip())   # skip header line
        for line in f:
            con = line[CAT_CON]
            idx = int(line[CAT_IDX])
            hr = int(line[CAT_HR].strip())
            modern_id = line[CAT_ID].strip()
            desc = line[CAT_DESC].strip()
            yield con, idx, hr, modern_id, desc

hr_to_hip = parse_cross_id(CROSS_ID)
hr_to_mid = {}
hr_to_con = {}
with open(STARS_FILE, 'w') as stars, open(DSO_FILE, 'w') as dso:
    for i, (con, idx, hr, modern_id, desc) in enumerate(parse_cat(CAT_FILE), 1):
        if hr:
            if hr in hr_to_mid and modern_id != hr_to_mid[hr]:
                print(f'discrepancy in modern id for HR {hr}: '
                      f'{modern_id} != {hr_to_mid[hr]}')
            else:
                hr_to_mid[hr] = modern_id
            hr_to_con.setdefault(hr, []).append(con)
        tag = hr_to_hip.get(hr)
        if tag is not None:
            file = stars
            tags = [tag]
        elif tags is not None:
            file = dso
            tags = DSO.get(modern_id)
        else:
            file, tags = None, []
        if file is not None:
            for tag in tags:
                name = PROPER_NAMES.get(i)
                if name is not None:
                    print(f'{tag}|_("{name}")', file=file)
                print(f'{tag}|("{con} {idx}")', file=file)
                print(f'{tag}|_("{desc}")', file=file)
        else:
            print(f'No HIP number found for HR {hr} ({modern_id})', file=sys.stderr)
print(f'  found {len(hr_to_mid)} unique stars and {len(PROPER_NAMES)} proper names')
for hr, cons in hr_to_con.items():
    if len(cons) > 1:
        mid = hr_to_mid[hr]
        print(f'{mid} (HR {hr}) is in {", ".join(cons)}.')
