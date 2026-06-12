/* ========================================================================
 * scriptReference.js - Stellarium Scripting API Reference Loader
 * ========================================================================
 * 
 * This module loads Stellarium scripting API definitions from JSON files
 * and provides them for autocomplete, quick reference, syntax validation,
 * and code generation across the Remote Control interface.
 * 
 * Features:
 * - Loads module index from modules-index.json
 * - Dynamically loads function definitions from module JSON files
 * - Supports both old and new JSON formats (backward compatible)
 * - Provides unified autocomplete items for all modules
 * - Case-sensitive function validation (isValidFunction)
 * - Case error detection (isCaseError)
 * - Categorized quick reference data
 * - Snippet loading and management
 * - Search functions by name, description, or tags
 * - Module-specific function queries
 * - Built-in fallback functions when JSON unavailable
 * 
 * Technical Implementation:
 * - Promise-based module loading with parallel fetching
 * - Caching system for performance (autocompleteCache, quickReferenceCache)
 * - Dynamic module name extraction from index
 * - Parameter and return type transformation
 * - Support for example extraction from multiple formats
 * - Strict case-sensitive function matching for validation
 * - Error handling with graceful fallback to built-in functions
 * 
 * Data Sources:
 * - modules-index.json: Index of all available modules
 * - {moduleId}.json: Function definitions for each module
 * - snippets.json: Code snippet definitions
 * 
 * @module scriptReference
 * @requires jquery
 * 
 * @author kutaibaa akraa (GitHub: @kutaibaa-akraa)
 * @date 2026-06-06
 * @license GPLv2+
 * @version 1.0.0
 * 
 * ======================================================================== */

define(["jquery"], function($) {
    "use strict";

    // =====================================================================
    // PRIVATE VARIABLES
    // =====================================================================

    var moduleIndex = null;
    var loadedModules = {};
    var allFunctions = [];
    var allSnippets = [];
    var isLoaded = false;
    var isLoading = false;
    var loadPromise = null;
    var basePath = "js/scripteditor/data/";
    
    // Cache for transformed autocomplete items
    var autocompleteCache = null;
    var quickReferenceCache = null;
    var autocompleteItemsCache = null;
    
    // Built-in fallback functions (loaded only if JSON fails)
    var builtInFunctions = [];

    // =====================================================================
    // BUILT-IN FALLBACK FUNCTIONS
    // =====================================================================

    /**
     * Get built-in fallback functions if JSON files cannot be loaded
     * These mirror the essential functions from the autocompleteItems array
     * 
     * @returns {Array} Basic function definitions
     */
    function getBuiltInFunctions() {
        return [
            // Core functions
            { name: "setDate", namespace: "core", moduleId: "core", moduleName: "Core API", category: "time", signature: "setDate(date, timezone)", fullSignature: "core.setDate(date, timezone)", description: "Set simulation date and time", parameters: [{ name: "date", type: "string", description: "Date string or 'now'", required: true }, { name: "timezone", type: "string", description: "'utc' or 'local'", required: false, defaultValue: "utc" }], returnType: { type: "void" }, examples: ["core.setDate(\"2025-01-01T00:00:00\", \"utc\");"], tags: ["time", "date"], since: "0.10.0", deprecated: false },
            { name: "setJD", namespace: "core", moduleId: "core", moduleName: "Core API", category: "time", signature: "setJD(jday)", fullSignature: "core.setJD(jday)", description: "Set time using Julian Day", parameters: [{ name: "jday", type: "number", description: "Julian Day", required: true }], returnType: { type: "void" }, examples: ["core.setJD(2460050.5);"], tags: ["time", "julian"], since: "0.10.0", deprecated: false },
            { name: "getJD", namespace: "core", moduleId: "core", moduleName: "Core API", category: "time", signature: "getJD()", fullSignature: "core.getJD()", description: "Get current Julian Day", parameters: [], returnType: { type: "number", description: "Julian Day" }, examples: ["var jd = core.getJD();"], tags: ["time", "julian"], since: "0.10.0", deprecated: false },
            { name: "setTimeRate", namespace: "core", moduleId: "core", moduleName: "Core API", category: "time", signature: "setTimeRate(rate)", fullSignature: "core.setTimeRate(rate)", description: "Set time flow rate (1 = normal)", parameters: [{ name: "rate", type: "number", description: "Time rate multiplier", required: true }], returnType: { type: "void" }, examples: ["core.setTimeRate(1);", "core.setTimeRate(0);", "core.setTimeRate(3600);"], tags: ["time", "rate"], since: "0.10.0", deprecated: false },
            { name: "getTimeRate", namespace: "core", moduleId: "core", moduleName: "Core API", category: "time", signature: "getTimeRate()", fullSignature: "core.getTimeRate()", description: "Get current time rate", parameters: [], returnType: { type: "number" }, examples: ["var rate = core.getTimeRate();"], tags: ["time", "rate"], since: "0.10.0", deprecated: false },
            { name: "wait", namespace: "core", moduleId: "core", moduleName: "Core API", category: "control", signature: "wait(seconds)", fullSignature: "core.wait(seconds)", description: "Pause script execution", parameters: [{ name: "seconds", type: "number", description: "Seconds to wait", required: true }], returnType: { type: "void" }, examples: ["core.wait(2);"], tags: ["wait", "delay"], since: "0.10.0", deprecated: false },
            { name: "debug", namespace: "core", moduleId: "core", moduleName: "Core API", category: "debug", signature: "debug(message)", fullSignature: "core.debug(message)", description: "Print debug message", parameters: [{ name: "message", type: "string", description: "Debug message", required: true }], returnType: { type: "void" }, examples: ["core.debug(\"Hello\");"], tags: ["debug", "log"], since: "0.10.0", deprecated: false },
            { name: "moveToObject", namespace: "core", moduleId: "core", moduleName: "Core API", category: "movement", signature: "moveToObject(name, duration)", fullSignature: "core.moveToObject(name, duration)", description: "Move view to celestial object", parameters: [{ name: "name", type: "string", description: "Object name", required: true }, { name: "duration", type: "number", description: "Duration in seconds", required: false, defaultValue: "1" }], returnType: { type: "void" }, examples: ["core.moveToObject(\"Mars\", 2);"], tags: ["movement", "object"], since: "0.10.0", deprecated: false },
            { name: "moveToAltAzi", namespace: "core", moduleId: "core", moduleName: "Core API", category: "movement", signature: "moveToAltAzi(az, alt, duration)", fullSignature: "core.moveToAltAzi(az, alt, duration)", description: "Move to horizontal coordinates (degrees)", parameters: [{ name: "az", type: "number", description: "Azimuth in degrees", required: true }, { name: "alt", type: "number", description: "Altitude in degrees", required: true }, { name: "duration", type: "number", description: "Duration", required: false, defaultValue: "1" }], returnType: { type: "void" }, examples: ["core.moveToAltAzi(180, 45, 2);"], tags: ["movement", "altaz"], since: "0.10.0", deprecated: false },
            { name: "setObserverLocation", namespace: "core", moduleId: "core", moduleName: "Core API", category: "location", signature: "setObserverLocation(location, planet)", fullSignature: "core.setObserverLocation(location, planet)", description: "Set observer location", parameters: [{ name: "location", type: "string", description: "Location name or coordinates", required: true }, { name: "planet", type: "string", description: "Planet name", required: false, defaultValue: "Earth" }], returnType: { type: "void" }, examples: ["core.setObserverLocation(\"Cairo, Egypt\", \"Earth\");"], tags: ["location", "observer"], since: "0.10.0", deprecated: false },
            { name: "vec3d", namespace: "core", moduleId: "core", moduleName: "Core API", category: "vector", signature: "vec3d(x, y, z)", fullSignature: "core.vec3d(x, y, z)", description: "Create a 3D vector (double precision)", parameters: [{ name: "x", type: "number", description: "X component", required: true }, { name: "y", type: "number", description: "Y component", required: true }, { name: "z", type: "number", description: "Z component", required: true }], returnType: { type: "Vec3d", description: "3D vector object" }, examples: ["var vec = core.vec3d(0.5, 0.5, 0.707);"], tags: ["vector", "3d"], since: "0.14.0", deprecated: false }
        ];
    }

    // =====================================================================
    // GET ALL AUTOCOMPLETE ITEMS FROM ALL MODULES
    // =====================================================================

    /**
     * Get all autocomplete items from all loaded modules
     * FIXED: Now includes ConstellationMgr and all other modules
     * FIXED: Returns cached version for performance
     * 
     * @param {boolean} forceRefresh - Force refresh cache
     * @returns {Array} All autocomplete items
     */
    function getAllAutocompleteItems(forceRefresh) {
        if (autocompleteItemsCache && !forceRefresh) {
            return autocompleteItemsCache;
        }
        
        var allItems = [];
        
        // Collect from all loaded modules
        for (var moduleId in loadedModules) {
            if (loadedModules.hasOwnProperty(moduleId)) {
                var functions = loadedModules[moduleId];
                var moduleName = moduleIndex?.modules?.find(function(m) { 
                    return m.id === moduleId; 
                })?.name || moduleId;
                
                for (var i = 0; i < functions.length; i++) {
                    var fn = functions[i];
                    var namespace = fn.namespace || moduleId;
                    
                    allItems.push({
                        fullName: namespace + '.' + fn.name,
                        name: fn.name,
                        prefix: namespace + '.' + fn.name,
                        signature: fn.signature || fn.name + "()",
                        fullSignature: fn.fullSignature || fn.signature || namespace + '.' + fn.name + "()",
                        category: fn.category || moduleId,
                        desc: fn.description || fn.desc || '',
                        moduleId: moduleId,
                        moduleName: moduleName,
                        parameters: fn.parameters || [],
                        returnType: fn.returnType,
                        firstExample: getFirstExample(fn.examples || fn.example),
                        examples: fn.examples || (fn.example ? [fn.example] : []),
                        type: fn.type || 'method',
                        tags: fn.tags || []
                    });
                }
            }
        }
        
        autocompleteItemsCache = allItems;
        console.log('[ScriptReference] Built autocomplete cache with ' + allItems.length + ' items');
        return allItems;
    }

    // =====================================================================
    // TRANSFORMATION FUNCTIONS
    // =====================================================================

    /**
     * Transform parameter to standard format
     * 
     * @param {Object} param - Raw parameter object
     * @returns {Object} Standardized parameter
     */
    function transformParameter(param) {
        return {
            name: param.name,
            type: param.type,
            description: param.description || param.desc || "",
            required: param.required === true,
            defaultValue: param.defaultValue || param.default || null,
            example: param.example || null
        };
    }

    /**
     * Transform return type to standard format
     * 
     * @param {Object|string} ret - Raw return type
     * @returns {Object} Standardized return type
     */
    function transformReturnType(ret) {
        if (!ret) return { type: "void", description: null };
        if (typeof ret === "string") return { type: ret, description: null };
        return {
            type: ret.type || "void",
            description: ret.description || null
        };
    }

    /**
     * Get the first example from examples array
     * 
     * @param {Array} examples - Array of examples
     * @returns {string} First example or empty string
     */
    function getFirstExample(examples) {
        if (!examples) return "";
        if (typeof examples === "string") return examples;
        if (Array.isArray(examples) && examples.length > 0) return examples[0];
        return "";
    }

    /**
     * Transform function to autocomplete item format
     * 
     * @param {Object} fn - Raw function definition
     * @returns {Object} Formatted autocomplete item
     */
    function functionToAutocompleteItem(fn) {
        var namespace = fn.namespace || fn.moduleId || "core";
        var name = fn.name;
        var fullPrefix = namespace + "." + name;
        
        // Build signature string from parameters if not provided
        var signature = fn.signature;
        if (!signature) {
            var paramNames = (fn.parameters || []).map(function(p) { return p.name; });
            signature = name + "(" + paramNames.join(", ") + ")";
        }
        
        var fullSignature = fn.fullSignature || fullPrefix + "(" + (fn.parameters || []).map(function(p) { 
            return p.type + " " + p.name;
        }).join(", ") + ")";
        
        // Determine category for quick reference grouping
        var category = fn.category || fn.moduleId || "other";
        if (namespace === "core") {
            if (name.match(/move|zoom|fov|track|mount|look/i)) category = "movement";
            else if (name.match(/date|time|jd|jday|rate|wait/i)) category = "core";
            else if (name.match(/object|select|label|info/i)) category = "objects";
            else if (name.match(/location|observer|planet|setObserver/i)) category = "location";
            else if (name.match(/sky|culture/i)) category = "skyculture";
            else if (name.match(/vec3|vector/i)) category = "vector";
        }
        
        return {
            prefix: fullPrefix,
            name: name,
            fullName: fullPrefix,
            signature: signature,
            fullSignature: fullSignature,
            desc: fn.description || "",
            category: category,
            moduleId: fn.moduleId || namespace,
            moduleName: fn.moduleName || (fn.moduleId === "core" ? "Core API" : fn.moduleId),
            parameters: (fn.parameters || []).map(transformParameter),
            returnType: transformReturnType(fn.returnType),
            examples: fn.examples || (fn.example ? [fn.example] : []),
            firstExample: getFirstExample(fn.examples || fn.example),
            notes: fn.notes || "",
            tags: fn.tags || [],
            since: fn.since,
            deprecated: fn.deprecated === true,
            type: fn.type || "method"
        };
    }

    /**
     * Transform snippet to quick reference format
     * 
     * @param {Object} snippet - Raw snippet definition
     * @returns {Object} Formatted snippet item
     */
    function snippetToQuickReferenceItem(snippet) {
        return {
            id: snippet.id,
            name: snippet.name,
            code: snippet.code,
            description: snippet.description || snippet.desc,
            category: snippet.category,
            categoryName: snippet.categoryName,
            type: "snippet"
        };
    }

    // =====================================================================
    // MODULE LOADING
    // =====================================================================

    /**
     * Load the module index file
     * 
     * @returns {Promise} Resolves with module index data
     */
    function loadModuleIndex() {
        return $.ajax({
            url: basePath + "modules-index.json",
            dataType: "json",
            cache: false,
            timeout: 5000
        }).then(function(data) {
            moduleIndex = data;
            console.log("[ScriptReference] Module index loaded: " + (data.modules?.length || 0) + " modules");
            return data;
        }).fail(function(jqXHR, textStatus, errorThrown) {
            console.warn("[ScriptReference] Could not load module index:", textStatus);
            return null;
        });
    }

    /**
     * Load a specific module's function definitions
     * Supports both old and new JSON structures
     * FIXED: Better handling of ConstellationMgr.json format
     * 
     * @param {string} moduleId - Module identifier
     * @param {string} fileName - JSON filename
     * @returns {Promise} Resolves with module function data
     */
    function loadModule(moduleId, fileName) {
        if (loadedModules[moduleId]) {
            return Promise.resolve(loadedModules[moduleId]);
        }

        return $.ajax({
            url: basePath + fileName,
            dataType: "json",
            cache: false,
            timeout: 5000
        }).then(function(data) {
            var functions = [];
            var moduleName = moduleIndex?.modules?.find(function(m) { return m.id === moduleId; })?.name || moduleId;
            
            if (Array.isArray(data)) {
                // Old structure: direct array of functions (like ConstellationMgr.json)
                functions = data.map(function(fn) {
                    fn.moduleId = moduleId;
                    fn.moduleName = moduleName;
                    fn.namespace = fn.namespace || moduleId;
                    // Ensure required fields exist
                    if (!fn.description && fn.desc) fn.description = fn.desc;
                    if (!fn.examples && fn.example) fn.examples = [fn.example];
                    if (!fn.returnType) fn.returnType = { type: "void" };
                    if (typeof fn.returnType === "string") fn.returnType = { type: fn.returnType };
                    return fn;
                });
            } else if (data.functions && Array.isArray(data.functions)) {
                // New structure: { moduleId, moduleName, functions: [...] }
                functions = data.functions.map(function(fn) {
                    fn.moduleId = data.moduleId || moduleId;
                    fn.moduleName = data.moduleName || moduleName;
                    fn.namespace = fn.namespace || data.moduleId || moduleId;
                    if (!fn.description && fn.desc) fn.description = fn.desc;
                    if (!fn.examples && fn.example) fn.examples = [fn.example];
                    if (!fn.returnType) fn.returnType = { type: "void" };
                    if (typeof fn.returnType === "string") fn.returnType = { type: fn.returnType };
                    return fn;
                });
            } else if (typeof data === "object" && data !== null) {
                // Unknown structure - try to extract functions
                for (var key in data) {
                    if (data.hasOwnProperty(key) && Array.isArray(data[key])) {
                        functions = functions.concat(data[key].map(function(fn) {
                            fn.moduleId = moduleId;
                            fn.moduleName = moduleName;
                            fn.namespace = fn.namespace || moduleId;
                            return fn;
                        }));
                    }
                }
            }
            
            loadedModules[moduleId] = functions;
            console.log("[ScriptReference] Loaded module: " + moduleId + " (" + functions.length + " functions)");
            
            // Clear caches after loading new module
            autocompleteCache = null;
            quickReferenceCache = null;
            autocompleteItemsCache = null;
            
            return functions;
        }).fail(function(jqXHR, textStatus, errorThrown) {
            console.warn("[ScriptReference] Could not load module: " + moduleId, textStatus, errorThrown);
            return [];
        });
    }

    /**
     * Load snippets from snippets.json file
     * 
     * @returns {Promise} Resolves with snippets array
     */
    function loadSnippets() {
        return $.ajax({
            url: basePath + "snippets.json",
            dataType: "json",
            cache: false,
            timeout: 5000
        }).then(function(data) {
            var snippets = [];
            
            if (data.categories) {
                for (var cat in data.categories) {
                    if (data.categories.hasOwnProperty(cat)) {
                        var category = data.categories[cat];
                        if (category.items && Array.isArray(category.items)) {
                            category.items.forEach(function(item) {
                                snippets.push({
                                    id: item.id,
                                    name: item.name,
                                    code: item.code,
                                    description: item.desc || item.description,
                                    category: cat,
                                    categoryName: category.name
                                });
                            });
                        }
                    }
                }
            } else if (Array.isArray(data)) {
                snippets = data;
            }
            
            allSnippets = snippets;
            console.log("[ScriptReference] Loaded snippets: " + snippets.length);
            return snippets;
        }).fail(function() {
            console.warn("[ScriptReference] Could not load snippets.json, using empty snippets");
            allSnippets = [];
            return [];
        });
    }

    /**
     * Load all modules from the index
     * 
     * @returns {Promise} Resolves when all modules are loaded
     */
    function loadAllModules() {
        if (isLoaded) {
            return Promise.resolve(allFunctions);
        }
        
        if (isLoading) {
            return loadPromise;
        }
        
        isLoading = true;
        
        loadPromise = loadModuleIndex().then(function(index) {
            if (!index || !index.modules || index.modules.length === 0) {
                console.warn("[ScriptReference] No module index or empty, using built-in functions");
               // builtInFunctions = getBuiltInFunctions();
                allFunctions = builtInFunctions.slice();
                isLoading = false;
                isLoaded = true;
                return allFunctions;
            }

            var loadPromises = index.modules.map(function(mod) {
                return loadModule(mod.id, mod.file);
            });

            return Promise.all(loadPromises).then(function(results) {
                allFunctions = [];
                results.forEach(function(funcs) {
                    allFunctions = allFunctions.concat(funcs);
                });
                
                // If no functions were loaded from JSON, use built-in
                if (allFunctions.length === 0) {
                    console.warn("[ScriptReference] No functions loaded from JSON, using built-in fallback");
                   // builtInFunctions = getBuiltInFunctions();
                    allFunctions = builtInFunctions.slice();
                }
                
                // Load snippets separately (don't fail if missing)
                return loadSnippets();
            }).then(function() {
                isLoading = false;
                isLoaded = true;
                autocompleteCache = null;
                quickReferenceCache = null;
                autocompleteItemsCache = null;
                console.log("[ScriptReference] Total functions loaded: " + allFunctions.length);
                console.log("[ScriptReference] Total snippets loaded: " + allSnippets.length);
                return allFunctions;
            }).catch(function(err) {
                isLoading = false;
                console.error("[ScriptReference] Error loading modules:", err);
                // Fallback to built-in functions
               // builtInFunctions = getBuiltInFunctions();
                allFunctions = builtInFunctions.slice();
                isLoaded = true;
                return allFunctions;
            });
        });
        
        return loadPromise;
    }

    // =====================================================================
    // SEARCH AND FILTERING
    // =====================================================================

    /**
     * Search functions by name, description, or tags
     * FIXED: Uses getAllAutocompleteItems for complete search
     * 
     * @param {string} query - Search query
     * @returns {Array} Matching functions
     */
    function searchFunctions(query) {
        if (!query || query.trim() === "") {
            return getAllAutocompleteItems();
        }
        
        var q = query.toLowerCase().trim();
        var allItems = getAllAutocompleteItems();
        
        return allItems.filter(function(fn) {
            // Search in full name
            if (fn.fullName && fn.fullName.toLowerCase().indexOf(q) >= 0) return true;
            // Search in name
            if (fn.name && fn.name.toLowerCase().indexOf(q) >= 0) return true;
            // Search in description
            if (fn.desc && fn.desc.toLowerCase().indexOf(q) >= 0) return true;
            // Search in signature
            if (fn.signature && fn.signature.toLowerCase().indexOf(q) >= 0) return true;
            // Search in tags
            if (fn.tags && fn.tags.some(function(tag) { 
                return tag.toLowerCase().indexOf(q) >= 0; 
            })) return true;
            // Search in module name
            if (fn.moduleName && fn.moduleName.toLowerCase().indexOf(q) >= 0) return true;
            return false;
        });
    }

		function getAutocompleteItems(forceRefresh) {
				if (autocompleteCache && !forceRefresh) {
						return autocompleteCache;
				}
				
				var allItems = getAllAutocompleteItems(forceRefresh);
				
				// Ensure core functions are always included (even if JSON fails)
				var coreFunctionsExist = false;
				for (var i = 0; i < allItems.length; i++) {
						if (allItems[i].fullName === 'core.debug') {
								coreFunctionsExist = true;
								break;
						}
				}
				
				if (!coreFunctionsExist) {
						console.warn('[ScriptReference] Core functions missing, adding built-ins');
						var builtIns = getBuiltInFunctions();
						for (var b = 0; b < builtIns.length; b++) {
								var fn = builtIns[b];
								allItems.push({
										fullName: fn.namespace + '.' + fn.name,
										name: fn.name,
										prefix: fn.namespace + '.' + fn.name,
										signature: fn.signature,
										fullSignature: fn.fullSignature,
										desc: fn.description,
										category: fn.category,
										moduleId: fn.moduleId,
										moduleName: fn.moduleName,
										parameters: fn.parameters,
										returnType: fn.returnType,
										examples: fn.examples,
										firstExample: getFirstExample(fn.examples),
										type: fn.type || 'method'
								});
						}
				}
				
				autocompleteCache = allItems.map(function(fn) {
						return {
								prefix: fn.prefix,
								name: fn.name,
								fullName: fn.fullName,
								signature: fn.signature,
								fullSignature: fn.fullSignature,
								desc: fn.desc,
								category: fn.category,
								moduleId: fn.moduleId,
								moduleName: fn.moduleName,
								parameters: fn.parameters,
								returnType: fn.returnType,
								examples: fn.examples,
								firstExample: fn.firstExample,
								type: fn.type
						};
				});
				
				return autocompleteCache;
		}

    /**
     * Get categorized quick reference data
     * 
     * @param {boolean} forceRefresh - Force refresh cache
     * @returns {Object} Categorized functions
     */
    function getCategorizedReference(forceRefresh) {
        if (quickReferenceCache && !forceRefresh) {
            return quickReferenceCache;
        }
        
        var categories = {};
        
        allFunctions.forEach(function(fn) {
            var cat = fn.moduleId || fn.category || "other";
            var catDisplayName = fn.moduleName || cat;
            
            if (!categories[cat]) {
                categories[cat] = {
                    id: cat,
                    name: catDisplayName,
                    items: []
                };
            }
            
            categories[cat].items.push(functionToAutocompleteItem(fn));
        });
        
        // Sort categories by name
        var sortedCategories = {};
        Object.keys(categories).sort().forEach(function(key) {
            sortedCategories[key] = categories[key];
        });
        
        quickReferenceCache = sortedCategories;
        return sortedCategories;
    }

    /**
     * Get all snippets
     * 
     * @returns {Array} All snippet items
     */
    function getSnippets() {
        return allSnippets.map(snippetToQuickReferenceItem);
    }

    /**
     * Get snippets by category
     * 
     * @param {string} category - Category name
     * @returns {Array} Snippets in category
     */
    function getSnippetsByCategory(category) {
        return allSnippets.filter(function(s) { return s.category === category; }).map(snippetToQuickReferenceItem);
    }

    /**
     * Get functions for a specific module
     * 
     * @param {string} moduleId - Module identifier
     * @returns {Array} Module functions
     */
    function getModuleFunctions(moduleId) {
        return allFunctions.filter(function(fn) {
            return fn.moduleId === moduleId;
        }).map(functionToAutocompleteItem);
    }

    // =====================================================================
    // FUNCTION VALIDATION FOR SYNTAX HIGHLIGHTING
    // =====================================================================

		/**
		 * Check if a function exists in the loaded API data
		 * STRICT case-sensitive validation - NO case-insensitive fallback
		 * 
		 * @param {string} funcName - Function name (e.g., "setDate" or "debug")
		 * @param {string|null} namespace - Optional namespace (e.g., "core")
		 * @returns {boolean} - True only for exact case match
		 */
		function isValidFunction(funcName, namespace) {
				// If not loaded, return false (will revalidate after loading)
				if (!isLoaded || !allFunctions || allFunctions.length === 0) {
						return false;
				}
				
				// Build the full name with exact case
				var fullName = namespace ? namespace + '.' + funcName : funcName;
				
				// Strict case-sensitive search through allFunctions (already loaded)
				for (var i = 0; i < allFunctions.length; i++) {
						var fn = allFunctions[i];
						var fnNamespace = fn.namespace || fn.moduleId;
						var fnFullName = fnNamespace + '.' + fn.name;
						
						// Exact case-sensitive match
						if (fnFullName === fullName) {
								return true;
						}
						
						// For standalone functions (no namespace) - like global functions
						if (!namespace && fn.name === funcName) {
								return true;
						}
				}
				
				return false;
		}

		/**
		 * Check if a function has the correct case (case error detection)
		 * Returns true if function exists but with different case
		 * 
		 * @param {string} funcName - Function name
		 * @param {string|null} namespace - Optional namespace
		 * @returns {boolean} - True if case error detected
		 */
		function isCaseError(funcName, namespace) {
				// If not loaded, return false
				if (!isLoaded || !allFunctions || allFunctions.length === 0) {
						return false;
				}
				
				var fullName = namespace ? namespace + '.' + funcName : funcName;
				var fullNameLower = fullName.toLowerCase();
				
				for (var i = 0; i < allFunctions.length; i++) {
						var fn = allFunctions[i];
						var fnNamespace = fn.namespace || fn.moduleId;
						var fnFullName = fnNamespace + '.' + fn.name;
						
						// Case-insensitive match but not exact match = case error
						if (fnFullName.toLowerCase() === fullNameLower && 
								fnFullName !== fullName) {
								return true;
						}
						
						// Check standalone function name
						if (!namespace) {
								if (fn.name.toLowerCase() === funcName.toLowerCase() &&
										fn.name !== funcName) {
										return true;
								}
						}
				}
				
				return false;
		}

		// In the return statement, add isCaseError:
		return {
				init: function() {
						return loadAllModules();
				},
				getAllFunctions: function() {
						return allFunctions.slice();
				},
				getAllSnippets: function() {
						return allSnippets.slice();
				},
				getSnippets: function() {
						return allSnippets.map(function(snippet) {
								return {
										id: snippet.id,
										name: snippet.name,
										code: snippet.code,
										description: snippet.description || snippet.desc,
										category: snippet.category,
										categoryName: snippet.categoryName,
										type: 'snippet'
								};
						});
				},
				getSnippetsByCategory: function(category) {
						return allSnippets.filter(function(s) { 
								return s.category === category; 
						}).map(function(snippet) {
								return {
										id: snippet.id,
										name: snippet.name,
										code: snippet.code,
										description: snippet.description || snippet.desc,
										category: snippet.category,
										categoryName: snippet.categoryName,
										type: 'snippet'
								};
						});
				},
				getAutocompleteItems: getAutocompleteItems,
				getCategorizedReference: getCategorizedReference,
				searchFunctions: searchFunctions,
				getModuleFunctions: getModuleFunctions,
				isLoaded: function() {
						return isLoaded;
				},
				getModuleIndex: function() {
						return moduleIndex;
				},
				refresh: function() {
						autocompleteCache = null;
						quickReferenceCache = null;
						autocompleteItemsCache = null;
				},
				isValidFunction: isValidFunction,
				isCaseError: isCaseError  // safe because it checks isLoaded first
		};

});