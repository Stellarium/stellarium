/* ========================================================================
 * skyculture-api.js - Unified API Layer
 * ========================================================================
 * 
 * This module provides a unified API abstraction for the Sky Culture
 * subsystem. All server communication is centralized here, providing:
 * 
 * - A single point for adding request headers, error handling, etc.
 * - Consistent promise-based interface
 * - Built-in caching for expensive queries
 * - Concurrency control for batch operations
 * - Standardized response parsing
 * 
 * KEY INSIGHT: The /api/objects/info endpoint returns the same structure
 * for ALL object types (constellations, asterisms, stars, DSOs). This
 * allows us to use a SINGLE fetch function for all types, extracting
 * only the fields we need:
 * 
 * - above-horizon:      boolean  (visibility)
 * - localized-name:     string   (translated name, available for all types)
 * - iauConstellation:   string   (parent constellation, for non-constellations)
 * - vmag:               number   (magnitude, meaningful only for stars/DSOs)
 * - object-type:        string   (detailed type, e.g. "eruptive variable star")
 * - star-type:          string   (for stars: "star" | "double-star" | ...)
 * - variable-star:      string   (for stars: "no" | "eruptive" | ...)
 * 
 * @module skyculture/api
 * @requires jquery
 * @requires ./skyculture-config
 * @requires ./skyculture-state
 * @author kutaibaa akraa (GitHub: @kutaibaa-akraa)
 * @license GPLv2+
 * @version 1.0.0
 * ======================================================================== */

define(["jquery", "./skyculture-config", "./skyculture-state"],
    function($, Config, State) {
    "use strict";

    var ENDPOINTS = Config.API_ENDPOINTS;

    // =====================================================================
    // LOW-LEVEL AJAX HELPERS
    // =====================================================================

    /**
     * Promise-based AJAX wrapper with standardized error handling.
     * 
     * @param {Object} options - jQuery.ajax options
     * @returns {Promise<Object>} Resolves with data, rejects with error
     */
    function ajax(options) {
        return new Promise(function(resolve, reject) {
            $.ajax(options)
                .done(function(data) { resolve(data); })
                .fail(function(xhr, status, error) {
                    if (status === 'abort') return; // silently ignore aborts
                    reject({
                        xhr:    xhr,
                        status: status,
                        error:  error
                    });
                });
        });
    }

    // =====================================================================
    // CULTURE LIST
    // =====================================================================

    /**
     * Fetch the list of available sky cultures.
     * Returns an array of {id, name} objects, sorted by name.
     * 
     * @returns {Promise<Array<{id: string, name: string}>>}
     */
    function fetchCultures() {
        return ajax({
            url:      ENDPOINTS.listSkyCultures,
            type:     'GET',
            dataType: 'json'
        }).then(function(data) {
            var culturesArray = [];
            for (var id in data) {
                if (data.hasOwnProperty(id)) {
                    culturesArray.push({ id: id, name: data[id] });
                }
            }
            culturesArray.sort(function(a, b) {
                return a.name.localeCompare(b.name);
            });
            return culturesArray;
        });
    }

    // =====================================================================
    // INDEX.JSON (SINGLE SOURCE OF TRUTH)
    // =====================================================================

    /**
     * Fetch the complete culture data from index.json.
     * This is the SINGLE SOURCE OF TRUTH for all culture-specific data.
     * 
     * @returns {Promise<Object>} Raw index.json data
     */
    function fetchIndexJson() {
        return ajax({
            url:      ENDPOINTS.skyCultureIndex,
            type:     'GET',
            dataType: 'json'
        });
    }

    // =====================================================================
    // OBJECT INFO (UNIFIED)
    // =====================================================================

    /**
     * Extract the fields we care about from a raw /api/objects/info response.
     * 
     * @param {Object} raw - Raw response from /api/objects/info
     * @returns {Object} Normalized info object
     */
    function normalizeObjectInfo(raw) {
        if (!raw || !raw.found) return null;

        return {
            found:             true,
            name:              raw.name || null,
            aboveHorizon:      raw['above-horizon'] === true,
            localizedName:     raw['localized-name'] || null,
						culturalNames:     raw['cultural-names'] || null,
            iauConstellation:  raw.iauConstellation || null,
            objectType:        raw['object-type'] || null,
            starType:          raw['star-type'] || null,
            variableStar:      raw['variable-star'] || null,
            vmag:              (typeof raw.vmag === 'number') ? raw.vmag : null,
            spectralClass:     raw['spectral-class'] || null,
            type:              raw.type || null
        };
    }

    /**
     * Fetch info for a single object by identifier.
     * 
     * The identifier may be a name, a HIP ID, a NAME prefix, etc.
     * The API handles normalization on the server side.
     * 
     * @param {string} identifier - Object identifier (name or catalog ID)
     * @returns {Promise<Object|null>} Normalized info or null
     */
    function fetchObjectInfo(identifier) {
        return ajax({
            url:      ENDPOINTS.objectInfo,
            type:     'GET',
            data:     { name: identifier, format: 'json' },
            dataType: 'json',
            timeout:  Config.OBJECT_INFO_TIMEOUT
        }).then(function(data) {
            return normalizeObjectInfo(data);
        }).catch(function() {
            return null;
        });
    }

    /**
     * Fetch info for a batch of objects with concurrency control.
     * 
     * @param {Array<{item: Object, apiId: string}>} items - Items to fetch
     * @param {Function} [progressCallback] - Optional (loaded, total) callback
     * @returns {Promise<Array<{item: Object, info: Object|null}>>}
     */
    function fetchObjectsBatch(items, progressCallback) {
        var concurrency = Config.FETCH_CONCURRENCY;
        var results     = [];
        var total       = items.length;
        var completed   = 0;

        return new Promise(function(resolve) {
            if (total === 0) {
                resolve([]);
                return;
            }

            function processBatch(startIndex) {
                var endIndex = Math.min(startIndex + concurrency, total);
                var batch    = items.slice(startIndex, endIndex);

                var batchPromises = batch.map(function(item) {
                    return fetchObjectInfo(item.apiId);
                });

                Promise.all(batchPromises).then(function(batchResults) {
                    for (var i = 0; i < batch.length; i++) {
                        results.push({
                            item: batch[i].item,
                            info: batchResults[i] || null
                        });
                    }
                    completed += batch.length;
                    if (progressCallback) progressCallback(completed, total);

                    if (endIndex < total) {
                        processBatch(endIndex);
                    } else {
                        resolve(results);
                    }
                }).catch(function(error) {
                    console.error('[SkyCulture:Api] Batch error:', error);
                    resolve(results);
                });
            }

            processBatch(0);
        });
    }

    /**
     * Fetch visibility data for a list of patterns.
     * Convenience wrapper around fetchObjectsBatch that uses each pattern's
     * searchName or name as the API identifier.
     * 
     * @param {Array} patterns - Pattern objects with {id, name, searchName}
     * @param {Function} [progressCallback] - Optional progress callback
     * @returns {Promise<Array<{pattern: Object, info: Object|null}>>}
     */
    function fetchVisibilityBatch(patterns, progressCallback) {
        var items = patterns.map(function(pattern) {
            return {
                item:  pattern,
                apiId: pattern.searchName || pattern.name || pattern.id
            };
        });

        return fetchObjectsBatch(items, progressCallback).then(function(results) {
            // Rename 'item' → 'pattern' for clarity
            return results.map(function(r) {
                return { pattern: r.item, info: r.info };
            });
        });
    }

    // =====================================================================
    // FOCUS / NAVIGATION
    // =====================================================================

    /**
     * Focus Stellarium on a target object.
     * 
     * @param {string} target - Object name or ID
     * @param {string} mode - Focus mode ('center' | 'mark' | 'zoom')
     * @returns {Promise}
     */
    function focusObject(target, mode) {
        return ajax({
            url:  ENDPOINTS.focus,
            type: 'POST',
            data: { target: target, mode: mode || 'center' }
        });
    }

    /**
     * Execute a Stellarium script directly.
     * 
     * @param {string} code - Script code to execute
     * @returns {Promise}
     */
    function runScriptDirect(code) {
        return ajax({
            url:  ENDPOINTS.scriptDirect,
            type: 'POST',
            data: { code: code, useIncludes: false }
        });
    }

    // =====================================================================
    // CACHED VARIANTS
    // =====================================================================

    /**
     * Get cached constellation info or fetch it if not cached/expired.
     * 
     * @param {string} cultureId - Current culture ID
     * @param {Object} pattern - Constellation pattern {id, searchName, name}
     * @param {boolean} forceRefresh - If true, ignore cache
     * @returns {Promise<Object|null>}
     */
    function getConstellationInfo(cultureId, pattern, forceRefresh) {
        var cacheKey = cultureId + '|' + pattern.id;
        var cached   = State.raw.cache.constellation[cacheKey];

        // Check cache validity
        if (!forceRefresh && cached) {
            var age = Date.now() - cached.timestamp;
            if (age < Config.CONSTELLATION_CACHE_TTL) {
                return Promise.resolve(cached.info);
            }
        }

        // Fetch fresh
        var queryName = pattern.searchName || pattern.name;
        return fetchObjectInfo(queryName).then(function(info) {
            State.raw.cache.constellation[cacheKey] = {
                info:      info,
                timestamp: Date.now()
            };
            return info;
        });
    }
		
    /**
     * Fetch the current server status (location + time + view).
     * 
     * Returns:
     *   {
     *     location: { latitude, longitude, altitude, name, planet, ... },
     *     time:     { jday, utc, local, isTimeNow, timeZone, ... },
     *     view:     { fov }
     *   }
     * 
     * On failure, resolves with `null` — the caller can proceed without
     * the metadata.
     * 
     * @returns {Promise<Object|null>}
     */
    function fetchStatus() {
        return ajax({
            url:      ENDPOINTS.status,
            type:     'GET',
            dataType: 'json',
            timeout:  3000
        }).catch(function(err) {
            console.warn('[SkyCulture:Api] fetchStatus failed:', err);
            return null;
        });
    }
		
    // =====================================================================
    // PUBLIC API
    // =====================================================================

    return {
        // Low-level
        ajax: ajax,

        // Culture list
        fetchCultures: fetchCultures,

        // Index.json
        fetchIndexJson: fetchIndexJson,

        // Object info
        fetchObjectInfo: fetchObjectInfo,
        fetchObjectsBatch: fetchObjectsBatch,
        fetchVisibilityBatch: fetchVisibilityBatch,
        getConstellationInfo: getConstellationInfo,
        normalizeObjectInfo: normalizeObjectInfo,
        fetchStatus: fetchStatus,
        // Focus / scripts
        focusObject: focusObject,
        runScriptDirect: runScriptDirect
    };
});