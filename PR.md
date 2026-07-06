### Description

Add a Gaia DR3-based star catalog generation pipeline and the engine changes needed to render the resulting high-level catalogs smoothly.

**Catalog generation (`util/gaiacat/`)**

- `skychart2cat` converts Gaia DR3 `.bin` extracts into Stellarium `.cat` catalogs. SkyChart `.dat` files are also supported as a fallback, but their parallax and proper-motion values have a known precision issue (~10^4–10^5× magnification for near-zero measurements) and should be avoided for science-quality catalogs.
- `extract_columns.py` compresses the full Gaia DR3 CSV release into compact `.bin` files (60 B/star) containing only the fields needed for the Star2 format.
- `dedupcat` removes duplicate stars between adjacent magnitude levels.
- `cmpcat` compares two catalogs zone-by-zone for validation.

The pipeline preserves stars missing BP/RP photometry, includes proper motion, parallax and B-V, and supports level 8/9/10 catalogs in the Star2 format.

**Engine changes**

- `SpecialZoneArray::draw()` now returns early when the level's brightest star (`mag_min`) is fainter than the current visibility limit. This avoids paging in huge mmap'd catalogs every frame just to discover nothing is visible, which removes the stuttering seen with the new level 8+ catalogs.
- Optional compact storage for high-level catalogs: when enabled, only the zone-count table and star data are kept, omitting the per-zone `SpecialZoneData` array. Restricted to `level >= 9`. Controlled by `stars/flag_compact_star_storage` (default `false`).
- `StarMgr::searchGaia()` searches all loaded levels using `maxGeodesicGridLevel` instead of the draw-limited `getMaxSearchLevel()`, and Phase 2 uses a 60" search radius around the HEALPix pixel center to catch stars near pixel edges.
- Fixed an integer overflow in `StarMgr::checkAndLoadCatalog()` when computing MD5 checksums for `.cat` files larger than 2 GB.

### Screenshots (if appropriate):
<img width="1920" height="1080" alt="图片" src="https://github.com/user-attachments/assets/972b519f-e551-420f-a339-d6eeb7d730a3" />

Galactic center — dense star field loaded smoothly, no stuttering

<img width="1920" height="1080" alt="图片" src="https://github.com/user-attachments/assets/68842e11-79ea-4b3b-bfed-65b565cc524c" />

Star at ~22 mag, found via the search window

### Type of change
<!--- What types of changes does your code introduce? Put an `x` in all the boxes that apply: -->
- [x] Bug fix (non-breaking change which fixes an issue)
- [x] New feature (non-breaking change which adds functionality)
- [ ] Breaking change (fix or feature that would cause existing functionality to change)
- [x] This change requires a documentation update
- [ ] Housekeeping

### How Has This Been Tested?

Generated `stars_8` (6.5 GB), `stars_9` (19 GB) and `stars_10` (26.7 GB) Star2 catalogs from Cartes du Ciel `.dat` files and from Gaia DR3 CSV extracts on Windows 10. All catalogs were placed on an HDD. Browsing the galactic center at FOVs from 60° down to 0.01° shows no stuttering; at FOV 0.1° performance is limited by GPU draw-call throughput, not disk I/O.

`testGaiaSearch` was run on levels 8, 9 and 10 with up to 100,000 samples per level. Phase 1 finds the correct zone for ~73–95% of stars; Phase 2 finds the remainder with zero misses.

**Test Configuration**:
* Operating system: Windows 10 64-bit
* Graphics Card: NVIDIA RTX 2070 laptop, driver version 551.86

## Checklist:
<!--- Go over all the following points, and put an `x` in all the boxes that apply. -->
<!--- If you're unsure about any of these, don't hesitate to ask. We're here to help! -->
- [x] My code follows the [code style](http://stellarium.org/doc/head/codingStyle.html) of this project.
- [ ] I have performed a self-review of my own code
- [ ] I have commented my code, particularly in hard-to-understand areas
- [ ] I have made corresponding changes to the documentation (header file)
- [ ] I have updated the respective chapter in the Stellarium User Guide
- [ ] My changes generate no new warnings
- [ ] I have added tests that prove my fix is effective or that my feature works
- [x] New and existing unit tests pass locally with my changes
- [x] Any dependent changes have been merged and published in downstream modules
