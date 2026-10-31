/* jshint expr: true */
/**
 * planet_data.js
 *
 * Astronomical data for the location panel, adjusted for planetary flattening.
 *
 * SOURCES:
 *   - Earth: WGS84 Geodetic System (NIMA TR8350.2).
 *   - Sun, Planets, Major Moons: IAU WGCCRE 2015 report, as encoded in
 *     ssystem_major.ini. For oblate bodies, a is the equatorial radius
 *     and b is derived from oblateness: b = a * (1 - oblateness).
 *   - Small Moons & Asteroids: mean radius from ssystem_major.ini and
 *     ssystem_minor.ini. These bodies are irregular, so a single mean
 *     radius is used (no flattening model).
 *
 * MODEL:
 *   - oblate_spheroid: has a (equatorial) and b (polar).
 *   - sphere:          has a single radius r.
 *   - triaxial:        has a single mean radius r_mean.
 */
define([], function() {
    "use strict";

    var DEG2RAD = Math.PI / 180;

    // ---------------------------------------------------------------------
    // Body database
    // ---------------------------------------------------------------------
    // type: "oblate_spheroid" | "sphere" | "triaxial"
    // For oblate_spheroid: a (eq), b (pol)
    // For sphere/triaxial: r (or r_mean)
    // ---------------------------------------------------------------------
    var bodies = {
        // --- Sun ---
        "Sun":              { type: "sphere", r: 696000 },

        // --- Oblate planets (WGCCRE 2015) ---
        "Mercury":          { type: "oblate_spheroid", a: 2440.53,  b: 2438.26 },
        "Venus":            { type: "sphere",          r: 6051.8 },
        "Earth":            { type: "oblate_spheroid", a: 6378.1366, b: 6356.752314245 }, // WGS84
        "Mars":             { type: "oblate_spheroid", a: 3396.19,  b: 3376.20 },
        "Jupiter":          { type: "oblate_spheroid", a: 71492,    b: 66854 },
        "Saturn":           { type: "oblate_spheroid", a: 60268,    b: 54364 },
        "Uranus":           { type: "oblate_spheroid", a: 25559,    b: 24973 },
        "Neptune":          { type: "oblate_spheroid", a: 24764,    b: 24341 },
        "Pluto":            { type: "sphere",          r: 1188.3 },

        // --- Major moons (spheres, WGCCRE 2015) ---
        "Moon":             { type: "sphere", r: 1737.4 },
        "Io":               { type: "sphere", r: 1821.49 },
        "Europa":           { type: "sphere", r: 1560.8 },
        "Ganymede":         { type: "sphere", r: 2631.2 },
        "Callisto":         { type: "sphere", r: 2410.3 },
        "Mimas":            { type: "sphere", r: 198.2 },
        "Enceladus":        { type: "sphere", r: 252.1 },
        "Tethys":           { type: "sphere", r: 531.0 },
        "Dione":            { type: "sphere", r: 561.4 },
        "Rhea":             { type: "sphere", r: 763.5 },
        "Titan":            { type: "sphere", r: 2575.0 },
        "Iapetus":          { type: "sphere", r: 734.3 },
        "Miranda":          { type: "sphere", r: 235.8 },
        "Ariel":            { type: "sphere", r: 578.9 },
        "Umbriel":          { type: "sphere", r: 584.7 },
        "Titania":          { type: "sphere", r: 788.9 },
        "Oberon":           { type: "sphere", r: 761.4 },
        "Triton":           { type: "sphere", r: 1352.6 },
        "Charon":           { type: "sphere", r: 606.0 },

        // --- Small moons (mean radius, ssystem_major.ini) ---
        "Phobos":           { type: "triaxial", r_mean: 11.08 },
        "Deimos":           { type: "triaxial", r_mean: 6.2 },
        "Amalthea":         { type: "triaxial", r_mean: 83.5 },
        "Thebe":            { type: "triaxial", r_mean: 49.3 },
        "Metis":            { type: "triaxial", r_mean: 21.5 },
        "Adrastea":         { type: "triaxial", r_mean: 8.2 },
        "Himalia":          { type: "triaxial", r_mean: 85 },
        "Elara":            { type: "triaxial", r_mean: 40 },
        "Pasiphae":         { type: "triaxial", r_mean: 18 },
        "Sinope":           { type: "triaxial", r_mean: 14 },
        "Carme":            { type: "triaxial", r_mean: 15 },
        "Ananke":           { type: "triaxial", r_mean: 10 },
        "Leda":             { type: "triaxial", r_mean: 5 },
        "Lysithea":         { type: "triaxial", r_mean: 12 },
        "Pan":              { type: "triaxial", r_mean: 14.0 },
        "Atlas":            { type: "triaxial", r_mean: 15.1 },
        "Prometheus":       { type: "triaxial", r_mean: 43.1 },
        "Pandora":          { type: "triaxial", r_mean: 40.6 },
        "Epimetheus":       { type: "triaxial", r_mean: 58.2 },
        "Janus":            { type: "triaxial", r_mean: 89.2 },
        "Hyperion":         { type: "triaxial", r_mean: 135 },
        "Phoebe":           { type: "triaxial", r_mean: 106.5 },
        "Helene":           { type: "triaxial", r_mean: 18 },
        "Telesto":          { type: "triaxial", r_mean: 12.4 },
        "Calypso":          { type: "triaxial", r_mean: 9.6 },
        "Puck":             { type: "triaxial", r_mean: 77 },
        "Cordelia":         { type: "triaxial", r_mean: 13 },
        "Ophelia":          { type: "triaxial", r_mean: 15 },
        "Bianca":           { type: "triaxial", r_mean: 25.7 },
        "Cressida":         { type: "triaxial", r_mean: 31 },
        "Desdemona":        { type: "triaxial", r_mean: 27 },
        "Juliet":           { type: "triaxial", r_mean: 42 },
        "Naiad":            { type: "triaxial", r_mean: 29 },
        "Thalassa":         { type: "triaxial", r_mean: 40 },
        "Despina":          { type: "triaxial", r_mean: 74 },
        "Galatea":          { type: "triaxial", r_mean: 79 },
        "Larissa":          { type: "triaxial", r_mean: 96 },
        "Proteus":          { type: "triaxial", r_mean: 208 },
        "Nereid":           { type: "triaxial", r_mean: 170 },
        "Halimede":         { type: "triaxial", r_mean: 62 },
        "Sao":              { type: "triaxial", r_mean: 44 },
        "Laomedeia":        { type: "triaxial", r_mean: 42 },
        "Psamathe":         { type: "triaxial", r_mean: 40 },
        "Neso":             { type: "triaxial", r_mean: 60 },
        "Styx":             { type: "triaxial", r_mean: 16 },
        "Nix":              { type: "triaxial", r_mean: 92 },
        "Kerberos":         { type: "triaxial", r_mean: 19 },
        "Hydra":            { type: "triaxial", r_mean: 114 },

        // --- Dwarf planets & TNOs (mean radius, ssystem_minor.ini) ---
        "(1) Ceres":        { type: "triaxial", r_mean: 469.7 },
        "(2) Pallas":       { type: "triaxial", r_mean: 256 },
        "(3) Juno":         { type: "triaxial", r_mean: 123.298 },
        "(4) Vesta":        { type: "triaxial", r_mean: 262.7 },
        "(136108) Haumea":  { type: "triaxial", r_mean: 780 },   // mean of 1050×840×537
        "(136199) Eris":    { type: "triaxial", r_mean: 1163 },
        "(136472) Makemake":{ type: "triaxial", r_mean: 715 },
        "(90377) Sedna":    { type: "triaxial", r_mean: 497.5 },
        "(90482) Orcus":    { type: "triaxial", r_mean: 458.5 },
        "(50000) Quaoar":   { type: "triaxial", r_mean: 545 },
        "(20000) Varuna":   { type: "triaxial", r_mean: 334 },
        "(120347) Salacia": { type: "triaxial", r_mean: 423 },
        "(174567) Varda":   { type: "triaxial", r_mean: 370 },
        "(225088) Gonggong":{ type: "triaxial", r_mean: 615 },

        // --- Asteroids (mean radius, ssystem_minor.ini) ---
        "(21) Lutetia":     { type: "triaxial", r_mean: 48 },
        "(243) Ida":        { type: "triaxial", r_mean: 16 },
        "(253) Mathilde":   { type: "triaxial", r_mean: 30 },
        "(433) Eros":       { type: "triaxial", r_mean: 8.42 },
        "(951) Gaspra":     { type: "triaxial", r_mean: 18 },
        "(2867) Steins":    { type: "triaxial", r_mean: 11 },
        "(25143) Itokawa":  { type: "triaxial", r_mean: 1 },
        "(101955) Bennu":   { type: "triaxial", r_mean: 0.246 },
        "(162173) Ryugu":   { type: "triaxial", r_mean: 0.435 }
    };

    // ---------------------------------------------------------------------
    // Radius lookup
    // ---------------------------------------------------------------------

    /**
     * Return the radius of the given body in kilometers.
     *
     * For oblate spheroids, the geodetic radius at the given latitude is
     * computed. If no latitude is given, the equatorial radius is returned.
     *
     * For spheres and triaxial bodies, a single radius is returned
     * regardless of latitude.
     *
     * @param {string} name        - Body name.
     * @param {number} [latDeg]    - Latitude in degrees.
     * @returns {number} Radius in km.
     */
    function getRadiusKm(name, latDeg) {
        var body = bodies[name];
        if (!body) {
            body = bodies["Earth"]; // fallback
        }

        // Spheres and irregular bodies: one radius.
        if (body.type === "sphere") {
            return body.r;
        }
        if (body.type === "triaxial") {
            return body.r_mean;
        }

        // Oblate spheroid.
        if (latDeg === undefined || latDeg === null) {
            return body.a;
        }

        var phi = latDeg * DEG2RAD;
        var cosPhi = Math.cos(phi);
        var sinPhi = Math.sin(phi);
        var a = body.a;
        var b = body.b;

        var num = Math.pow(a * a * cosPhi, 2) + Math.pow(b * b * sinPhi, 2);
        var den = Math.pow(a * cosPhi, 2) + Math.pow(b * sinPhi, 2);
        return Math.sqrt(num / den);
    }

    function degreesToKm(degrees, name, latDeg) {
        return degrees * DEG2RAD * getRadiusKm(name, latDeg);
    }

    function kmToDegrees(km, name, latDeg) {
        return km / (getRadiusKm(name, latDeg) * DEG2RAD);
    }

    function getAllRadii() {
        var copy = {};
        for (var k in bodies) {
            if (bodies.hasOwnProperty(k)) {
                var b = bodies[k];
                copy[k] = (b.type === "oblate_spheroid") ? b.a : (b.r || b.r_mean);
            }
        }
        return copy;
    }

    return {
        getRadiusKm:  getRadiusKm,
        degreesToKm:  degreesToKm,
        kmToDegrees:  kmToDegrees,
        getAllRadii:  getAllRadii
    };
});