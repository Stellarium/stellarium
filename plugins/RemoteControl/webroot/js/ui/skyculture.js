/* ========================================================================
 * skyculture.js - Sky Culture Manager with Direct Index.json Data Loading
 * ========================================================================
 * 
 * This module provides a complete UI for managing sky cultures and their
 * associated astronomical data in the Stellarium Remote Control plugin.
 * 
 * =========================================================================
 * TABLE OF CONTENTS
 * =========================================================================
 * 1. Overview and Architecture
 * 2. Key Features
 * 3. Data Flow Diagram
 * 4. API Endpoints Used
 * 5. Module Dependencies
 * 6. Constants and Configuration
 * 7. Private Variables
 * 8. Helper Functions
 * 9. Data Extraction Functions (Direct from index.json)
 * 10. Panel Rendering Functions
 * 11. Culture Management Functions
 * 12. Synchronization Listeners
 * 13. Initialization
 * 14. Public API
 * 
 * =========================================================================
 * 1. OVERVIEW AND ARCHITECTURE
 * =========================================================================
 * 
 * The Sky Culture Manager provides a multi-panel tabbed interface for
 * exploring astronomical data across different cultural traditions. It
 * replaces the traditional dropdown selector with an interactive button
 * grid and organizes cultural patterns into categorized panels.
 * 
 * DATA SOURCE PHILOSOPHY:
 * -----------------------
 * This module uses index.json as the SINGLE SOURCE OF TRUTH for all
 * culture-specific data. This architectural decision was made because:
 * 
 * 1. The Stellarium API's listobjectsbytype endpoint does not properly
 *    filter constellation and asterism names by culture/language.
 * 2. The API may return inconsistent results (mixing English and native
 *    names, missing entries, or incorrect counts).
 * 3. index.json contains the complete, authoritative data for each culture.
 * 
 * By loading directly from index.json, we guarantee:
 * - Correct constellation and asterism counts
 * - Proper display name selection (English preferred, native as fallback)
 * - Functional search using the English name
 * - Consistent behavior across all 54+ sky cultures
 * 
 * ARCHITECTURE FLOW:
 * ------------------
 * 
 *   ┌─────────────────┐
 *   │ listskyculture  │  (GET /api/view/listskyculture)
 *   │ (API)           │  → Returns translated culture names (needs new Api) right now English names only
 *   └────────┬────────┘
 *            │
 *            ▼
 *   ┌─────────────────┐
 *   │ Culture Buttons │  (User selects a culture)
 *   │ (UI Grid)       │
 *   └────────┬────────┘
 *            │
 *            ▼
 *   ┌─────────────────┐     ┌─────────────────┐
 *   │ setSkyCulture   │───▶│ StelProperty    │  (POST /api/stelproperty/set)
 *   │                 │     │ Update          │
 *   └────────┬────────┘     └─────────────────┘
 *            │
 *            ▼
 *   ┌─────────────────┐
 *   │ index.json      │  (GET /api/view/skyculturedescription/index.json)
 *   │ (Single Source) │  → Complete culture data
 *   └────────┬────────┘
 *            │
 *            ├──────────────────────────────────────┐
 *            │                                      │
 *            ▼                                      ▼
 *   ┌─────────────────┐                    ┌─────────────────┐
 *   │ Constellations  │                    │ Asterisms       │
 *   │ (extract from   │                    │ (extract from   │
 *   │  index.json)    │                    │  index.json)    │
 *   └─────────────────┘                    └─────────────────┘
 *            │                                      │
 *            ▼                                      ▼
 *   ┌─────────────────┐                    ┌─────────────────┐
 *   │ Zodiac Signs    │                    │ Lunar Mansions  │
 *   │ (extract from   │                    │ (extract from   │
 *   │  index.json)    │                    │  index.json)    │
 *   └─────────────────┘                    └─────────────────┘
 *            │                                      │
 *            ▼                                      ▼
 *   ┌─────────────────┐                    ┌─────────────────┐
 *   │ Notable Stars   │                    │ Description     │
 *   │ (extract from   │                    │ (iframe)        │
 *   │  index.json)    │                    │                 │
 *   └─────────────────┘                    └─────────────────┘
 *            │                                      │
 *            └──────────────┬───────────────────────┘
 *                           ▼
 *                  ┌─────────────────┐
 *                  │ RENDER PANELS   │
 *                  │ (Button Grids)  │
 *                  └─────────────────┘
 * 
 * =========================================================================
 * 2. KEY FEATURES
 * =========================================================================
 * 
 * - Sky culture selection via responsive, scrollable button grid
 * - Six-panel tabbed interface (Constellations, Asterisms, Zodiac Signs,
 *   Lunar Mansions, Notable Stars, Description)
 * - Direct data extraction from index.json (no API filtering issues)
 * - Constellation isolation with automatic state preservation
 * - Type-specific FOV zoom levels (constellations: 60°, asterisms: 30°, etc.)
 * - Real-time synchronization with server-side culture changes
 * - Automatic selection clearing to prevent view conflicts
 * - Translation support for culture names (via listskyculture API)
 * - Lazy loading with visual loading placeholders
 * - Responsive design with mobile/touch support
 * 
 * =========================================================================
 * 3. DATA FLOW DIAGRAM
 * =========================================================================
 * 
 * User Action                Module Response              Server Interaction
 * ─────────────────────────────────────────────────────────────────────────
 * 
 * Page Load
 *     │
 *     ▼
 * init() ─────────────────────────────────────────────▶ GET /listskyculture
 *     │                                                      │
 *     │                                                      ▼
 *     │                                            Translated culture names
 *     │                                                      │
 *     ▼                                                      ▼
 * renderCultureButtons() ◀─────────────────────────── availableCultures[]
 *     │
 *     ▼
 * loadAllCultureData() ──────────────────────────────▶ GET /index.json
 *     │                                                      │
 *     │                                                      ▼
 *     │                                            Complete culture data
 *     │                                                      │
 *     ├── loadConstellationsFromIndexJson() ◀────────────────┤
 *     ├── loadAsterismsFromIndexJson()    ◀──────────────────┤
 *     ├── extractZodiac()                 ◀──────────────────┤
 *     ├── extractLunarMansions()          ◀──────────────────┤
 *     └── extractStarNames()              ◀──────────────────┤
 *     │                                                      │
 *     ▼                                                      │
 * renderConstellationsPanel()                                │
 * renderAsterismsPanel()                                     │
 * renderZodiacPanel()                                        │
 * renderLunarPanel()                                         │
 * renderStarsPanel()                                         │
 *     │                                                      │
 *     ▼                                                      │
 * User clicks constellation ──────────────────────────────────┤
 *     │                                                      │
 *     ▼                                                      │
 * stelUtils.toggleConstellationHighlight() ──────────────────┼──▶ POST /main/focus
 *                                                            │    POST /stelproperty/set
 *                                                            │
 * =========================================================================
 * 4. API ENDPOINTS USED
 * =========================================================================
 * 
 * | Endpoint                                      | Method | Purpose                    |
 * |-----------------------------------------------|--------|----------------------------|
 * | /api/view/listskyculture                      | GET    | List all cultures (translated) |
 * | /api/view/skyculturedescription/index.json    | GET    | Complete culture data (source of truth) |
 * | /api/stelproperty/set                         | POST   | Change current culture     |
 * | /api/view/skyculturedescription/              | GET    | Culture HTML description (iframe) |
 * 
 * Note: /api/objects/listobjectsbytype is NOT used for constellations
 * and asterisms due to inconsistent filtering behavior across cultures.
 * 
 * =========================================================================
 * 5. MODULE DEPENDENCIES
 * =========================================================================
 * 
 * @requires jquery               - DOM manipulation and AJAX
 * @requires api/properties       - StelProperty access
 * @requires api/remotecontrol    - Server communication and translation
 * @requires ui/stellarium-utils  - Shared navigation utilities
 * 
 * =========================================================================
 * 6. AUTHOR AND VERSION
 * =========================================================================
 * 
 * @author kutaibaa akraa (GitHub: @kutaibaa-akraa)
 * @date 2026-04-18
 * @license GPLv2+
 * @version 4.0.0 - Direct index.json loading (eliminates API filtering issues)
 * 
 * ======================================================================== */

define(["jquery", "api/properties", "api/remotecontrol", "ui/stellarium-utils"], 
    function($, propApi, rc, stelUtils) {
    "use strict";

    // =====================================================================
    // 6. CONSTANTS AND CONFIGURATION
    // =====================================================================

    /**
     * Default FOV (Field of View) values for different object types.
     * These values provide optimal viewing angles for each category.
     * @constant {Object.<string, number>}
     */
    var DEFAULT_FOV = {
        constellation: 60,
        asterism: 30,
        zodiac: 40,
        lunar: 30,
        star: 15
    };

    // =====================================================================
    // 7. PRIVATE VARIABLES
    // =====================================================================

    // ---------------------------------------------------------------------
    // DOM Elements
    // ---------------------------------------------------------------------

    /** @type {jQuery} Container for culture selection buttons */
    var $cultureContainer = null;
    
    /** @type {jQuery} Container for constellation buttons */
    var $constellationsContainer = null;
    
    /** @type {jQuery} Container for asterism buttons */
    var $asterismsContainer = null;
    
    /** @type {jQuery} Container for zodiac buttons */
    var $zodiacContainer = null;
    
    /** @type {jQuery} Container for lunar mansion buttons */
    var $lunarContainer = null;
    
    /** @type {jQuery} Container for star buttons */
    var $starsContainer = null;
    
    /** @type {jQuery} iframe for culture description */
    var $infoFrame = null;
    
    /** @type {jQuery} Element displaying culture count badge */
    var $cultureCount = null;

    // ---------------------------------------------------------------------
    // State Variables
    // ---------------------------------------------------------------------

    /** @type {string} Currently active culture ID (e.g., "korean", "arabic_al-sufi") */
    var currentCultureId = null;
    
    /** @type {string} Current Stellarium language code (e.g., 'en', 'ar', 'fr') */
    var currentLanguage = 'en';
    
    /** @type {string} Name of the currently active culture (translated) */
    var currentCultureName = null;
    
    /** @type {Array<{id: string, name: string}>} List of all available cultures */
    var availableCultures = [];
    
    /** @type {string} Name of the currently selected pattern (for UI highlighting) */
    var selectedPattern = null;
    
    /** @type {string} ID of the currently selected pattern */
    var selectedPatternId = null;
		
		/** @type {jQuery} Container for artwork images */
		var $artworkContainer = null;

		/** @type {Array} Current artwork data extracted from index.json */
		var currentArtworkData = [];

		/** @type {boolean} Flag to prevent multiple artwork click events */
		var isArtworkClickLocked = false;
		
		/** @type {string|null} Currently selected artwork ID */
		var selectedArtworkId = null;
    
    /** 
     * Current patterns organized by type.
     * @type {Object.<string, Array>}
     */
    var currentPatternsData = { 
        constellations: [], 
        asterisms: [], 
        zodiac: [], 
        lunar: [], 
        stars: [] 
    };
    
    /** @type {Object} Raw culture data from index.json (cached for star navigation) */
    var currentCultureRawData = null;
    
    /** @type {boolean} Module initialization state */
    var isInitialized = false;
    
    /** @type {boolean} Synchronization enabled flag (can be toggled for debugging) */
    var syncEnabled = true;

    // ---------------------------------------------------------------------
    // Constellations Tour Toolbar Variables
    // ---------------------------------------------------------------------

    /** @type {jQuery} Generate dynamic script button */
    var $constTourGenerateDynamicBtn = null;
    
    /** @type {jQuery} Generate static array script button */
    var $constTourGenerateStaticBtn = null;
    
    /** @type {jQuery} Duration per constellation (seconds) */
    var $constTourDuration = null;
    
    /** @type {jQuery} Isolate selected constellation checkbox */
    var $constTourIsolate = null;
    
    /** @type {jQuery} Zoom FOV when zoomed in (degrees) */
    var $constTourZoomFov = null;
    
    /** @type {jQuery} Overview FOV between constellations (degrees) */
    var $constTourOverviewFov = null;
    
    /** @type {jQuery} Constellation labels font size (pixels) */
    var $constTourFontSize = null;
    
    /** @type {jQuery} Constellation line thickness (pixels) */
    var $constTourLineThickness = null;
    
    /** @type {jQuery} Constellation boundaries thickness (pixels) */
    var $constTourBoundariesThickness = null;
    
    /** @type {jQuery} Constellation hulls thickness (pixels) */
    var $constTourHullsThickness = null;
    
    /** @type {jQuery} Show constellation artwork checkbox */
    var $constTourShowArt = null;
    
    /** @type {jQuery} Show constellation boundaries checkbox */
    var $constTourShowBoundaries = null;
    
    /** @type {jQuery} Show constellation hulls checkbox */
    var $constTourShowHulls = null;
    
    /** @type {jQuery} Artwork intensity slider (0-100) */
    var $constTourArtIntensity = null;
    
    /** @type {jQuery} Artwork fade duration (seconds) */
    var $constTourArtFade = null;
    
    /** @type {jQuery} Constellation lines fade duration (seconds) */
    var $constTourLinesFade = null;
    
    /** @type {jQuery} Constellation labels fade duration (seconds) */
    var $constTourLabelsFade = null;
    
    /** @type {jQuery} Constellation boundaries fade duration (seconds) */
    var $constTourBoundariesFade = null;
    
    /** @type {jQuery} Constellation hulls fade duration (seconds) */
    var $constTourHullsFade = null;
    
    /** @type {jQuery} Select last only (single selection mode) checkbox */
    var $constTourPickLastOnly = null;
    
    /** @type {jQuery} Show constellation lines checkbox */
    var $constTourShowLines = null;
		
		    // Multi-Select Mode Variables for Constellations
    /** @type {jQuery} Multi-select mode checkbox */
    var $constMultiSelectMode = null;
    
    /** @type {Set} Set of selected constellation IDs */
    var selectedConstellationIds = new Set();
    
    /** @type {jQuery} Display for selection count */
    var $constSelectionCount = null;
    
    /** @type {boolean} Flag to prevent navigation during multi-select */
    var isMultiSelectModeActive = false;

    // ---------------------------------------------------------------------
    // Asterisms Tour Toolbar Variables
    // ---------------------------------------------------------------------

    /** @type {jQuery} Generate static array script button for asterisms */
    var $asterTourGenerateStaticBtn = null;
    
    /** @type {jQuery} Duration per asterism (seconds) */
    var $asterTourDuration = null;
    
    /** @type {jQuery} Isolate selected asterism checkbox */
    var $asterTourIsolate = null;
    
    // Note: The following asterisms UI elements are accessed via $('#id') directly:
    // - $('#aster-zoom-fov') - Zoom FOV input
    // - $('#aster-overview-fov') - Overview FOV input
    // - $('#aster-show-rayhelpers') - Show ray helpers checkbox
    // - $('#aster-font-size') - Font size input
    // - $('#aster-line-thickness') - Line thickness input
    // - $('#aster-rayhelper-thickness') - Ray helper thickness input
    // - $('#aster-lines-fade') - Lines fade duration
    // - $('#aster-labels-fade') - Labels fade duration
    // - $('#aster-rayhelpers-fade') - Ray helpers fade duration
    // - Color sliders: $('#aster-lines-color-r/g/b'), etc.
		
		// ============================================================
		// MULTI-SELECT MODE VARIABLES FOR ASTERISMS
		// ============================================================

		/**
		 * Multi-select mode checkbox for asterisms.
		 * When checked, clicking asterism buttons adds/removes them from a selection set
		 * instead of navigating directly.
		 * @type {jQuery}
		 */
		var $asterMultiSelectMode = null;

		/**
		 * Set of selected asterism IDs.
		 * Uses JavaScript Set for O(1) lookup performance.
		 * @type {Set<string>}
		 */
		var selectedAsterismIds = new Set();

		/**
		 * Display element for selection count.
		 * Shows how many asterisms are currently selected.
		 * @type {jQuery}
		 */
		var $asterSelectionCount = null;

		/**
		 * Flag indicating whether multi-select mode is active.
		 * Prevents navigation during multi-select mode.
		 * @type {boolean}
		 */
		var isAsterMultiSelectModeActive = false;
		
		// ---------------------------------------------------------------------
    // Lunar Mansions Tour Toolbar Variables
    // ---------------------------------------------------------------------

    /** @type {jQuery} Generate static array script button for lunar mansions */
    var $lunarTourGenerateStaticBtn = null;
    
    /** @type {jQuery} Duration per lunar mansion (seconds) */
    var $lunarTourDuration = null;
    
    /** @type {jQuery} Zoom FOV when zoomed in (degrees) */
    var $lunarTourZoomFov = null;
    
    /** @type {jQuery} Overview FOV between lunar mansions (degrees) */
    var $lunarTourOverviewFov = null;
    
    /** @type {jQuery} Font size for labels (pixels) */
    var $lunarTourFontSize = null;
    
    /** @type {jQuery} Line thickness for lunar system lines (pixels) */
    var $lunarTourLineThickness = null;
    
    /** @type {jQuery} Show labels on screen checkbox */
    var $lunarTourShowLabels = null;
    
    /** @type {jQuery} Show lunar system lines checkbox */
    var $lunarTourShowLines = null;
    
    // Color controls - will be accessed via $('#lunar-lines-color-r') etc.
    
    // ============================================================
    // MULTI-SELECT MODE VARIABLES FOR LUNAR MANSIONS
    // ============================================================
    
    /**
     * Multi-select mode checkbox for lunar mansions.
     * When checked, clicking lunar mansion buttons adds/removes them from a selection set
     * instead of navigating directly.
     * @type {jQuery}
     */
    var $lunarMultiSelectMode = null;
    
    /**
     * Set of selected lunar mansion IDs.
     * Uses JavaScript Set for O(1) lookup performance.
     * @type {Set<string>}
     */
    var selectedLunarMansionIds = new Set();
    
    /**
     * Display element for selection count.
     * Shows how many lunar mansions are currently selected.
     * @type {jQuery}
     */
    var $lunarSelectionCount = null;
    
    /**
     * Flag indicating whether multi-select mode is active.
     * Prevents navigation during multi-select mode.
     * @type {boolean}
     */
    var isLunarMultiSelectModeActive = false;
		
		// ---------------------------------------------------------------------
    // Stars Tour Toolbar Variables
    // ---------------------------------------------------------------------

    /** @type {jQuery} Generate static array script button for stars */
    var $starsTourGenerateStaticBtn = null;
    
    /** @type {jQuery} Duration per star (seconds) */
    var $starsTourDuration = null;
    
    /** @type {jQuery} Zoom FOV when zoomed in (degrees) */
    var $starsTourZoomFov = null;
    
    /** @type {jQuery} Overview FOV between stars (degrees) */
    var $starsTourOverviewFov = null;
    
    /** @type {jQuery} Font size for labels (pixels) */
    var $starsTourFontSize = null;
    
    /** @type {jQuery} Show labels on screen checkbox */
    var $starsTourShowLabels = null;
    
    /** @type {jQuery} Labels amount slider (0-10) */
    var $starsTourLabelsAmount = null;
    
    /** @type {jQuery} Show additional names checkbox */
    var $starsTourShowAdditionalNames = null;
    
    /** @type {jQuery} Show double stars designation checkbox */
    var $starsTourShowDblStars = null;
    
    /** @type {jQuery} Show variable stars designation checkbox */
    var $starsTourShowVarStars = null;
    
    /** @type {jQuery} Show HIP numbers checkbox */
    var $starsTourShowHIPNumbers = null;
    
    /** @type {jQuery} Absolute star scale slider */
    var $starsTourAbsoluteScale = null;
    
    /** @type {jQuery} Relative star scale slider */
    var $starsTourRelativeScale = null;
    
    /** @type {jQuery} Show twinkle checkbox */
    var $starsTourShowTwinkle = null;
    
    /** @type {jQuery} Twinkle amount slider */
    var $starsTourTwinkleAmount = null;
    
    /** @type {jQuery} Show spiky stars checkbox */
    var $starsTourShowSpiky = null;
    
    /** @type {jQuery} Show star halo checkbox */
    var $starsTourShowHalo = null;
    
    /** @type {jQuery} Use magnitude limit checkbox */
    var $starsTourUseMagLimit = null;
    
    /** @type {jQuery} Custom magnitude limit slider */
    var $starsTourMagLimit = null;
    
    // Color controls for star labels - will be accessed via $('#star-labels-color-r') etc.
    
    // ============================================================
    // MULTI-SELECT MODE VARIABLES FOR STARS
    // ============================================================
    
    /**
     * Multi-select mode checkbox for stars.
     * When checked, clicking star buttons adds/removes them from a selection set
     * instead of navigating directly.
     * @type {jQuery}
     */
    var $starsMultiSelectMode = null;
    
    /**
     * Set of selected star IDs.
     * Uses JavaScript Set for O(1) lookup performance.
     * @type {Set<string>}
     */
    var selectedStarIds = new Set();
    
    /**
     * Display element for selection count.
     * Shows how many stars are currently selected.
     * @type {jQuery}
     */
    var $starsSelectionCount = null;
    
    /**
     * Flag indicating whether multi-select mode is active.
     * Prevents navigation during multi-select mode.
     * @type {boolean}
     */
    var isStarsMultiSelectModeActive = false;

    // ---------------------------------------------------------------------
    // Translation Function
    // ---------------------------------------------------------------------
    
    /** @type {Function} Translation function from remotecontrol module */
    var _tr = rc.tr;
		
    // =====================================================================
    // 8. HELPER FUNCTIONS
    // =====================================================================

    /**
     * Escape HTML special characters to prevent XSS attacks.
     * Converts the characters & < > " ' to their HTML entity equivalents.
     * 
     * @param {string} str - The string to escape
     * @returns {string} The escaped string, safe for HTML insertion
     */
    function escapeHtml(str) {
        if (!str) return '';
        return String(str)
            .replace(/&/g, '&amp;')
            .replace(/</g, '&lt;')
            .replace(/>/g, '&gt;')
            .replace(/"/g, '&quot;')
            .replace(/'/g, '&#39;');
    }

    /**
     * Update the pattern count badge displayed in tab headers.
     * 
     * @param {string} type - Pattern type (constellations, asterisms, zodiac, lunar, stars)
     * @param {number} count - Number of items to display
     */
    function updatePatternCount(type, count) {
        var $countElement = $("#" + type + "-count");
        if ($countElement && $countElement.length) {
            $countElement.text(count > 0 ? "(" + count + ")" : "");
        }
    }

		/**
		 * Clear active state from all pattern buttons across all containers.
		 * Called when selection is cleared or culture is changed.
		 * Also clears selected artwork state.
		 */
		function clearAllActiveButtons() {
				var containers = [
						$constellationsContainer, $asterismsContainer, 
						$zodiacContainer, $lunarContainer, $starsContainer
				];
				for (var i = 0; i < containers.length; i++) {
						if (containers[i] && containers[i].length) {
								containers[i].find(".pattern-btn").removeClass("active");
						}
				}
				selectedPattern = null;
				selectedPatternId = null;
				
				// ============================================================
				// NEW: Clear artwork selection state
				// ============================================================
				if ($artworkContainer && $artworkContainer.length) {
						$artworkContainer.find('.artwork-item').removeClass('selected-artwork');
						selectedArtworkId = null;
				}
		}

		/**
		 * Update button active states based on current selection.
		 * Ensures only the button matching selectedPatternId is highlighted.
		 * 
		 * Enhanced matching logic:
		 * - Exact match (most reliable)
		 * - Composite ID number extraction for zodiac and lunar mansions
		 * - Name-based match as final fallback
		 * 
		 * @function updateAllButtonStates
		 */
		function updateAllButtonStates() {
				var containers = [
						$constellationsContainer, $asterismsContainer, 
						$zodiacContainer, $lunarContainer, $starsContainer
				];
				
				for (var c = 0; c < containers.length; c++) {
						var container = containers[c];
						if (container && container.length) {
								container.find(".pattern-btn").removeClass("active");
								
								if (selectedPatternId) {
										container.find(".pattern-btn").each(function() {
												var $btn = $(this);
												var btnPatternId = $btn.data("pattern-id");
												
												var match = false;
												
												// ============================================================
												// METHOD 1: EXACT MATCH (MOST RELIABLE)
												// ============================================================
												if (btnPatternId === selectedPatternId) {
														match = true;
												}
												
												// ============================================================
												// METHOD 2: ZODIAC COMPOSITE ID MATCHING
												// Prevents "zodiac_modern_0" from matching "zodiac_modern_1"
												// Extracts and compares the numeric index
												// ============================================================
												if (!match && btnPatternId && selectedPatternId) {
														// Check if both are zodiac composite IDs
														if (btnPatternId.indexOf('zodiac_') === 0 && 
																selectedPatternId.indexOf('zodiac_') === 0) {
																var btnParts = btnPatternId.split('_');
																var selectedParts = selectedPatternId.split('_');
																var btnNum = parseInt(btnParts[btnParts.length - 1], 10);
																var selectedNum = parseInt(selectedParts[selectedParts.length - 1], 10);
																if (btnNum === selectedNum) {
																		match = true;
																}
														}
														// ========================================================
														// METHOD 3: LUNAR MANSION COMPOSITE ID MATCHING
														// Prevents "lunar_1" from matching "lunar_16"
														// ========================================================
														else if (btnPatternId.indexOf('lunar_') === 0 && 
																		 selectedPatternId.indexOf('lunar_') === 0) {
																var btnNum = parseInt(btnPatternId.split('_')[1], 10);
																var selectedNum = parseInt(selectedPatternId.split('_')[1], 10);
																if (btnNum === selectedNum) {
																		match = true;
																}
														}
														// ========================================================
														// METHOD 4: CONSTELLATION ABBREVIATION MATCHING
														// For CON prefix IDs (e.g., "CON chinese_manchu Borboho")
														// ========================================================
														else if (btnPatternId.indexOf('CON ') === 0 && 
																		 selectedPatternId.indexOf('CON ') === 0) {
																var btnParts = btnPatternId.split(' ');
																var selectedParts = selectedPatternId.split(' ');
																if (btnParts.length >= 3 && selectedParts.length >= 3) {
																		if (btnParts[2] === selectedParts[2]) {
																				match = true;
																		}
																}
														}
														// ========================================================
														// METHOD 5: ASTERISM ABBREVIATION MATCHING
														// For AST prefix IDs (e.g., "AST chinese_manchu 001")
														// ========================================================
														else if (btnPatternId.indexOf('AST ') === 0 && 
																		 selectedPatternId.indexOf('AST ') === 0) {
																var btnParts = btnPatternId.split(' ');
																var selectedParts = selectedPatternId.split(' ');
																if (btnParts.length >= 3 && selectedParts.length >= 3) {
																		if (btnParts[2] === selectedParts[2]) {
																				match = true;
																		}
																}
														}
														// ========================================================
														// METHOD 6: SIMPLE ID MATCHING (for backward compatibility)
														// ========================================================
														else if (btnPatternId === selectedPatternId) {
																match = true;
														}
												}
												
												// ============================================================
												// METHOD 7: MATCH BY NAME (FINAL FALLBACK)
												// Used when ID matching fails
												// ============================================================
												if (!match && selectedPattern) {
														var btnName = $btn.data("pattern-name");
														var btnSearchName = $btn.data("search-name");
														if (btnName === selectedPattern || btnSearchName === selectedPattern) {
																match = true;
																console.log("[SkyCulture] Matched by name fallback:", btnName);
														}
												}
												
												if (match) {
														$btn.addClass("active");
														console.log("[SkyCulture] Activated button:", $btn.data("pattern-name"), "ID:", btnPatternId);
												}
										});
								}
						}
				}
		}

    /**
     * Get the current Stellarium language from StelProperty.
     * 
     * @returns {string} Language code (e.g., 'en', 'ar', 'fr')
     */
    function getCurrentLanguage() {
        if (propApi) {
            var lang = propApi.getStelProp("StelLocaleMgr.appLanguage");
            if (lang) {
                currentLanguage = lang;
                return lang;
            }
        }
        return currentLanguage;
    }

    /**
     * Render buttons in a container with click handlers.
     * This is the core UI rendering function used by all pattern panels.
     * 
     * @param {jQuery} $container - Container element to populate
     * @param {Array} patterns - Array of pattern objects with properties:
     *                           {id, name, searchName, type}
     * @param {string} containerName - Name of container type (for placeholder text)
     * @param {Function} onClickHandler - Click handler function(patternName, patternType, searchName, patternId)
     * @param {boolean} disableOriginalHandler - If true, skip attaching the original handler (for multi-select mode)
     */
    function renderButtons($container, patterns, containerName, onClickHandler, disableOriginalHandler) {
        if (!$container || !$container.length) return;
        $container.empty();
        
        // Handle empty state
        if (!patterns || patterns.length === 0) {
            var placeholderText = _tr("No data available");
            if (containerName === "constellations") {
                placeholderText = _tr("No constellations available for this culture");
            } else if (containerName === "asterisms") {
                placeholderText = _tr("No asterisms available for this culture");
            } else if (containerName === "zodiac") {
                placeholderText = _tr("No zodiac data available for this culture");
            } else if (containerName === "lunar") {
                placeholderText = _tr("No lunar mansions data available for this culture");
            } else if (containerName === "stars") {
                placeholderText = _tr("No star name data available for this culture");
            }
            
            $container.html('<div class="loading-placeholder">' + placeholderText + '</div>');
            return;
        }
        
        // Build button grid
        var buttonsHtml = '<div class="patterns-buttons-grid">';
        for (var i = 0; i < patterns.length; i++) {
            var pattern = patterns[i];
            var isActive = (selectedPatternId === pattern.id);
            var tooltip = pattern.type === "star" ? ' title="' + escapeHtml(pattern.id) + '"' : '';
            
            buttonsHtml += '<button type="button" class="pattern-btn' + (isActive ? ' active' : '') + 
                '" data-pattern-id="' + escapeHtml(pattern.id) + 
                '" data-pattern-name="' + escapeHtml(pattern.name) + 
                '" data-pattern-type="' + escapeHtml(pattern.type) + 
                '" data-search-name="' + escapeHtml(pattern.searchName || pattern.name) + '"' + 
                tooltip + '>' + escapeHtml(pattern.name) + '</button>';
        }
        buttonsHtml += '</div>';
        $container.html(buttonsHtml);
        
        // Attach click handlers - ONLY if not disabled
        if (!disableOriginalHandler) {
            $container.find(".pattern-btn").on("click", function() {
                var $btn = $(this);
                var patternId = $btn.data("pattern-id");
                var patternName = $btn.data("pattern-name");
                var patternType = $btn.data("pattern-type");
                var searchName = $btn.data("search-name");
                
                if ($btn.hasClass('active')) {
                    // Toggle OFF: Deselect and clear highlight
                    $container.find(".pattern-btn").removeClass("active");
                    selectedPattern = null;
                    selectedPatternId = null;
                    if (patternType === "constellation") {
                        stelUtils.clearConstellationHighlight();
                    }
                } else {
                    // Toggle ON: Select and execute action
                    $container.find(".pattern-btn").removeClass("active");
                    $btn.addClass("active");
                    selectedPattern = patternName;
                    selectedPatternId = patternId;
                    if (patternName && onClickHandler) {
                        onClickHandler(patternName, patternType, searchName, patternId);
                    }
                }
            });
        }
    }

    // =====================================================================
    // 9. DATA EXTRACTION FUNCTIONS (Direct from index.json)
    // =====================================================================
    // 
    // These functions extract data directly from the index.json structure.
    // This bypasses the Stellarium API's inconsistent filtering behavior
    // and guarantees correct data for all 56+ sky cultures.
    // =====================================================================

    /**
     * Load constellations directly from index.json data.
     * 
     * Display Name Priority:
     * 1. English name (if available) - preferred for consistency
     * 2. Native name (if available) - fallback when English missing
     * 3. Constellation ID - absolute fallback
     * 
     * Search Name: Always uses English name (or fallback) for reliable
     * Stellarium object lookup.
     * 
     * @param {Object} cultureData - The culture data from index.json
     * @returns {Array} Constellation pattern objects with properties:
     *                  {id, name, searchName, type, englishName, nativeName}
     */
    function loadConstellationsFromIndexJson(cultureData) {
        var patterns = [];
        
        if (!cultureData || !cultureData.constellations || !Array.isArray(cultureData.constellations)) {
            console.warn("[SkyCulture] No constellations array in culture data");
            return patterns;
        }
        
        var totalConstellations = cultureData.constellations.length;
        
        for (var i = 0; i < totalConstellations; i++) {
            var con = cultureData.constellations[i];
            var id = con.id;
            
            if (!id) {
                console.warn("[SkyCulture] Constellation missing ID at index", i);
                continue;
            }
            
            // Extract name components
            var englishName = (con.common_name && con.common_name.english) 
                ? con.common_name.english.trim() 
                : null;
            var nativeName = (con.common_name && con.common_name.native) 
                ? con.common_name.native.trim() 
                : null;
            
            // Determine display name: English preferred, then native, then ID
            var displayName = englishName || nativeName || id;
            
            // Search name: English preferred for reliable Stellarium lookup
            var searchName = englishName || displayName;
            
            patterns.push({
                id: id,
                name: displayName,
                searchName: searchName,
                type: "constellation",
                englishName: englishName,
                nativeName: nativeName
            });
        }
        
        // Sort alphabetically by display name
        patterns.sort(function(a, b) {
            return a.name.localeCompare(b.name);
        });
        
        console.log("[SkyCulture] Loaded " + patterns.length + " constellations from index.json " +
            "(total in file: " + totalConstellations + ")");
        
        return patterns;
    }

    /**
     * Load asterisms directly from index.json data.
     * 
     * This function filters out "ray helpers" (is_ray_helper === true) which
     * are auxiliary lines rather than proper asterisms.
     * 
     * Display Name Priority: Same as constellations (English → Native → ID)
     * 
     * @param {Object} cultureData - The culture data from index.json
     * @returns {Array} Asterism pattern objects
     */
    function loadAsterismsFromIndexJson(cultureData) {
        var patterns = [];
        
        if (!cultureData || !cultureData.asterisms || !Array.isArray(cultureData.asterisms)) {
            console.warn("[SkyCulture] No asterisms array in culture data");
            return patterns;
        }
        
        var totalAsterisms = cultureData.asterisms.length;
        var skippedRayHelpers = 0;
        
        for (var i = 0; i < totalAsterisms; i++) {
            var ast = cultureData.asterisms[i];
            
            // Skip ray helpers - they are auxiliary lines, not proper asterisms
            if (ast.is_ray_helper === true) {
                skippedRayHelpers++;
                continue;
            }
            
            var id = ast.id;
            if (!id) {
                console.warn("[SkyCulture] Asterism missing ID at index", i);
                continue;
            }
            
            // Extract name components
            var englishName = (ast.common_name && ast.common_name.english) 
                ? ast.common_name.english.trim() 
                : null;
            var nativeName = (ast.common_name && ast.common_name.native) 
                ? ast.common_name.native.trim() 
                : null;
            
            // Determine display name: English preferred, then native, then ID
            var displayName = englishName || nativeName || id;
            
            // Search name: English preferred for reliable Stellarium lookup
            var searchName = englishName || displayName;
            
            patterns.push({
                id: id,
                name: displayName,
                searchName: searchName,
                type: "asterism",
                englishName: englishName,
                nativeName: nativeName
            });
        }
        
        // Sort alphabetically by display name
        patterns.sort(function(a, b) {
            return a.name.localeCompare(b.name);
        });
        
        console.log("[SkyCulture] Loaded " + patterns.length + " asterisms from index.json " +
            "(total: " + totalAsterisms + ", skipped " + skippedRayHelpers + " ray helpers)");
        
        return patterns;
    }

		/**
		 * Extract zodiac signs from culture data.
		 * 
		 * Note: Zodiac signs are always extracted from index.json as there is
		 * no API endpoint for this data type.
		 * 
		 * IMPORTANT: This function now generates composite IDs in the format
		 * "zodiac_cultureId_index" to prevent partial matching issues.
		 * For backward compatibility, the simple ID is also stored.
		 * 
		 * Example IDs:
		 * - compositeId: "zodiac_modern_0" (for Aries)
		 * - simpleId: "aries" (for backward compatibility)
		 * 
		 * @param {Object} cultureData - The culture data from index.json
		 * @returns {Array|null} Zodiac pattern objects with properties:
		 *                       {id, compositeId, name, searchName, type, englishName, nativeName, symbol}
		 */
		function extractZodiac(cultureData) {
				if (!cultureData || !cultureData.zodiac || !cultureData.zodiac.names || 
						!cultureData.zodiac.names.length) {
						return null;
				}
				
				var patterns = [];
				var zodiacNames = cultureData.zodiac.names;
				var cultureId = currentCultureId || 'unknown';
				
				for (var i = 0; i < zodiacNames.length; i++) {
						var sign = zodiacNames[i];
						var signName = sign.english || sign.native || ("Sign " + (i + 1));
						
						// Create simple ID for backward compatibility (e.g., "aries")
						var simpleId = signName.toLowerCase().replace(/\s+/g, '_');
						
						// Create composite ID to prevent partial matching issues
						// Format: "zodiac_cultureId_index" (e.g., "zodiac_modern_0")
						var compositeId = "zodiac_" + cultureId + "_" + i;
						
						patterns.push({ 
								id: compositeId,           // Primary ID for matching (prevents partial matches)
								simpleId: simpleId,        // Legacy ID for backward compatibility
								name: signName,            // Display name
								searchName: signName,      // Name used for API lookup
								type: "zodiac",
								englishName: sign.english || null,
								nativeName: sign.native || null,
								symbol: sign.symbol || null
						});
				}
				
				console.log("[SkyCulture] Extracted " + patterns.length + " zodiac signs with composite IDs");
				return patterns;
		}

    /**
     * Extract lunar mansions from culture data.
     * 
     * Note: Lunar mansions are always extracted from index.json as there is
     * no API endpoint for this data type.
     * 
     * @param {Object} cultureData - The culture data from index.json
     * @returns {Array|null} Lunar mansion pattern objects or null if not available
     */
    function extractLunarMansions(cultureData) {
        if (!cultureData || !cultureData.lunar_system || !cultureData.lunar_system.names || 
            !cultureData.lunar_system.names.length) {
            return null;
        }
        
        var patterns = [];
        var lunarNames = cultureData.lunar_system.names;
        
        for (var i = 0; i < lunarNames.length; i++) {
            var mansion = lunarNames[i];
            var mansionName = mansion.english || mansion.native || ("Mansion " + (i + 1));
            
            patterns.push({ 
                id: "lunar_" + (i + 1), 
                name: mansionName, 
                searchName: mansionName, 
                type: "lunar" 
            });
        }
        
        console.log("[SkyCulture] Extracted " + patterns.length + " lunar mansions");
        return patterns;
    }

		/**
		 * Extract notable star names from culture data.
		 * 
		 * Handles multiple name variants for the same star (HIP ID) and combines
		 * them for display. Limits results to 3200 entries to prevent UI
		 * performance degradation with extremely large datasets.
		 * 
		 * IMPORTANT: This function also handles special prefixes like "NAME"
		 * which are used for planets and solar system objects. For these,
		 * we extract the actual object name (e.g., "NAME Venus" -> "Venus")
		 * to ensure proper navigation in Stellarium.
		 * 
		 * @param {Object} cultureData - The culture data from index.json
		 * @returns {Array|null} Star pattern objects or null if not available
		 */
		function extractStarNames(cultureData) {
				if (!cultureData || !cultureData.common_names || 
						typeof cultureData.common_names !== 'object' || 
						Object.keys(cultureData.common_names).length === 0) {
						return null;
				}
				
				var patterns = [];
				var commonNames = cultureData.common_names;
				
				for (var hipId in commonNames) {
						if (!commonNames.hasOwnProperty(hipId)) continue;
						
						var entries = commonNames[hipId];
						if (!entries || !Array.isArray(entries) || entries.length === 0) continue;
						
						var displayNames = [];
						var primaryName = null;
						var primaryNativeName = null;
						
						// Process the ID - handle "NAME" prefix for solar system objects
						var searchId = hipId;
						var originalId = hipId;
						var isSolarSystemObject = false;
						
						// Check if this is a solar system object (NAME prefix)
						if (hipId.indexOf('NAME ') === 0) {
								isSolarSystemObject = true;
								// Extract the actual object name (e.g., "NAME Mars" -> "Mars")
								searchId = hipId.substring(5).trim();
								console.log("[SkyCulture] Solar system object detected:", hipId, "-> search ID:", searchId);
						}
						
						// Collect all valid names for this star/object
						for (var i = 0; i < entries.length; i++) {
								var entry = entries[i];
								
								// Collect English names (skip codes like ARA_01)
								if (entry.english && entry.english.trim()) {
										var name = entry.english.trim();
										if (!name.match(/^[A-Z]+_[0-9]+$/)) {
												displayNames.push({ type: 'english', value: name });
												if (primaryName === null) primaryName = name;
										}
								}
								
								// Collect native names as fallback
								if (entry.native && entry.native.trim()) {
										var nativeName = entry.native.trim();
										if (primaryNativeName === null) primaryNativeName = nativeName;
										var exists = displayNames.some(function(dn) { 
												return dn.value === nativeName; 
										});
										if (!exists) {
												displayNames.push({ type: 'native', value: nativeName });
										}
								}
						}
						
						// Determine display and search names
						var displayName = '';
						var finalSearchName = '';
						
						if (displayNames.length === 0) {
								displayName = searchId;
								finalSearchName = searchId;
						} else {
								displayName = displayNames[0].value;
								finalSearchName = primaryName || primaryNativeName || displayNames[0].value;
								
								// Combine multiple names for compact display
								if (displayNames.length === 2) {
										displayName = displayNames[0].value + " - " + displayNames[1].value;
								} else if (displayNames.length >= 3) {
										displayName = displayNames[0].value + " - " + displayNames[1].value;
										var additional = displayNames.slice(2).map(function(dn) { 
												return dn.value; 
										}).join(" / ");
										displayName += " (" + additional + ")";
								}
								
								// Truncate extremely long display names
								if (displayName.length > 140) {
										displayName = displayName.substring(0, 137) + "...";
								}
						}
						
						// For solar system objects, ensure search name is the actual object name
						if (isSolarSystemObject) {
								finalSearchName = searchId;
						}
						
						patterns.push({
								id: searchId,                    // Use cleaned ID for navigation (e.g., "Mars" not "NAME Mars")
								originalId: originalId,          // Keep original ID for reference (e.g., "NAME Mars")
								name: displayName,
								searchName: finalSearchName,
								type: "star",
								isSolarSystem: isSolarSystemObject,
								englishName: primaryName,
								nativeName: primaryNativeName
						});
				}
				
				if (patterns.length === 0) return null;
				
				// Sort alphabetically and limit to prevent UI lag
				patterns.sort(function(a, b) { return a.name.localeCompare(b.name); });
				var limitedPatterns = patterns.slice(0, 3200);
				
				console.log("[SkyCulture] Extracted " + limitedPatterns.length + " star names " +
						(patterns.length > 3200 ? "(limited from " + patterns.length + ")" : ""));
				
				return limitedPatterns;
		}

		/**
		 * Extract artwork data from culture data.
		 * 
		 * Extracts constellation illustrations from the index.json file.
		 * Each artwork contains:
		 * - id: Constellation ID
		 * - name: Display name (English or native)
		 * - searchName: Name used for Stellarium navigation
		 * - imageFile: Path to image file (e.g., "illustrations/UMi.png")
		 * - imagePath: Full API path to the image
		 * - nativeName: Native name of the constellation (if available)
		 * - englishName: English name of the constellation (if available)
		 * 
		 * Returns empty array if no illustrations are available.
		 * 
		 * @param {Object} cultureData - The culture data from index.json
		 * @param {Array} constellations - Pre-loaded constellation patterns
		 * @returns {Array} Array of artwork objects
		 */
		function extractArtworkFromIndexJson(cultureData, constellations) {
				var artworks = [];
				
				// Early exit if no culture data or no constellations
				if (!cultureData || !constellations || !constellations.length) {
						console.log("[SkyCulture] No constellations data available for artwork extraction");
						return artworks;
				}
				
				// Check if constellations have image data
				var hasImages = false;
				if (cultureData.constellations && Array.isArray(cultureData.constellations)) {
						for (var i = 0; i < cultureData.constellations.length; i++) {
								if (cultureData.constellations[i].image && cultureData.constellations[i].image.file) {
										hasImages = true;
										break;
								}
						}
				}
				
				if (!hasImages) {
						console.log("[SkyCulture] No artwork images found in culture data");
						return artworks;
				}
				
				// Build a map of constellation IDs to patterns for quick lookup
				var patternMap = {};
				for (var i = 0; i < constellations.length; i++) {
						patternMap[constellations[i].id] = constellations[i];
				}
				
				// Extract artwork from constellations
				for (var i = 0; i < cultureData.constellations.length; i++) {
						var origCon = cultureData.constellations[i];
						if (!origCon.image || !origCon.image.file) continue;
						
						// Find matching pattern in constellations array to get display names
						var matchingPattern = patternMap[origCon.id] || null;
						
						// Get display name - prefer English from common_name, then native, then id
						var displayName = origCon.id;
						if (origCon.common_name) {
								displayName = origCon.common_name.english || 
															origCon.common_name.native || 
															origCon.id;
						}
						
						// Get search name (for Stellarium navigation) - prefer English
						var searchName = origCon.id;
						if (origCon.common_name) {
								searchName = origCon.common_name.english || 
														 origCon.common_name.native || 
														 origCon.id;
						}
						
						// Build artwork object
						artworks.push({
								id: origCon.id,
								name: displayName,
								searchName: searchName,
								imageFile: origCon.image.file,
								imagePath: '/api/view/skyculturedescription/' + origCon.image.file,
								nativeName: origCon.common_name ? origCon.common_name.native || null : null,
								englishName: origCon.common_name ? origCon.common_name.english || null : null,
								// Store the original constellation data for reference
								originalData: origCon
						});
				}
				
				// Sort alphabetically by display name
				artworks.sort(function(a, b) {
						return a.name.localeCompare(b.name);
				});
				
				console.log("[SkyCulture] Extracted " + artworks.length + " artwork images from index.json");
				return artworks;
		}

    // =====================================================================
    // 10. PANEL RENDERING FUNCTIONS
    // =====================================================================

    /**
     * Render the Constellations panel.
     * 
     * @param {Array} patterns - Constellation pattern objects
     * @param {boolean} disableHandler - If true, skip attaching the original click handler
     */
    function renderConstellationsPanel(patterns, disableHandler) {
        if (!patterns || !patterns.length) {
            if ($constellationsContainer) {
                $constellationsContainer.html('<div class="loading-placeholder">' + 
                    _tr("No constellations available for this culture") + '</div>');
            }
            updatePatternCount("constellations", 0);
            currentPatternsData.constellations = [];
            return;
        }
        
        updatePatternCount("constellations", patterns.length);
        currentPatternsData.constellations = patterns;
        
        renderButtons($constellationsContainer, patterns, "constellations", 
            function(patternName, patternType, searchName, patternId) {
                // IMPORTANT: Pass BOTH name and original ID
                stelUtils.toggleConstellationHighlight(searchName || patternName, patternId);
                updateAllButtonStates();
            },
            disableHandler === true
        );
    }

    /**
     * Render the Asterisms panel.
     * 
     * @param {Array} patterns - Asterism pattern objects
     */
    function renderAsterismsPanel(patterns) {
        if (!patterns || !patterns.length) {
            if ($asterismsContainer) {
                $asterismsContainer.html('<div class="loading-placeholder">' + 
                    _tr("No asterisms available for this culture") + '</div>');
            }
            updatePatternCount("asterisms", 0);
            currentPatternsData.asterisms = [];
            return;
        }
        
        updatePatternCount("asterisms", patterns.length);
        currentPatternsData.asterisms = patterns;
        
        renderButtons($asterismsContainer, patterns, "asterisms", 
            function(patternName, patternType, searchName, patternId) {
                stelUtils.goToAsterism(searchName || patternName, patternId);
                updateAllButtonStates();
            });
    }

		/**
		 * Render the Zodiac Signs panel.
		 * 
		 * IMPORTANT: This function passes the patternId (composite ID) to the
		 * navigation handler to ensure proper bidirectional synchronization.
		 * 
		 * @param {Array} patterns - Zodiac pattern objects with composite IDs
		 */
		function renderZodiacPanel(patterns) {
				if (!patterns || !patterns.length) {
						if ($zodiacContainer) {
								$zodiacContainer.html('<div class="loading-placeholder">' + 
										_tr("No zodiac signs available for this culture") + '</div>');
						}
						updatePatternCount("zodiac", 0);
						currentPatternsData.zodiac = [];
						return;
				}
				
				updatePatternCount("zodiac", patterns.length);
				currentPatternsData.zodiac = patterns;
				
				renderButtons($zodiacContainer, patterns, "zodiac", 
						function(patternName, patternType, searchName, patternId) {
								console.log("[SkyCulture] Zodiac button clicked:", patternName, "ID:", patternId);
								stelUtils.goToZodiacSign(searchName || patternName, patternId);
								updateAllButtonStates();
						});
		}

    /**
     * Render the Lunar Mansions panel.
     * 
     * @param {Array} patterns - Lunar mansion pattern objects
     */
    function renderLunarPanel(patterns) {
        if (!patterns || !patterns.length) {
            if ($lunarContainer) {
                $lunarContainer.html('<div class="loading-placeholder">' + 
                    _tr("No lunar mansions available for this culture") + '</div>');
            }
            updatePatternCount("lunar", 0);
            currentPatternsData.lunar = [];
            return;
        }
        
        updatePatternCount("lunar", patterns.length);
        currentPatternsData.lunar = patterns;
        
        renderButtons($lunarContainer, patterns, "lunar", 
            function(patternName, patternType, searchName, patternId) {
                stelUtils.goToLunarMansion(searchName || patternName, patternId);
                updateAllButtonStates();
            });
    }

    /**
     * Render the Notable Stars panel.
     * 
     * @param {Array} patterns - Star pattern objects
     */
    function renderStarsPanel(patterns) {
        if (!patterns || !patterns.length) {
            if ($starsContainer) {
                $starsContainer.html('<div class="loading-placeholder">' + 
                    _tr("No star names available for this culture") + '</div>');
            }
            updatePatternCount("stars", 0);
            currentPatternsData.stars = [];
            return;
        }
        
        updatePatternCount("stars", patterns.length);
        currentPatternsData.stars = patterns;
        
        renderButtons($starsContainer, patterns, "stars", 
            function(patternName, patternType, searchName, patternId) {
                stelUtils.goToObject(patternId, DEFAULT_FOV.star, 'star', currentCultureRawData);
                updateAllButtonStates();
            });
    }

		/**
		 * Render the Artwork panel with constellation images.
		 * 
		 * @param {Array} artworks - Array of artwork objects
		 */
		function renderArtworkPanel(artworks) {
				if (!$artworkContainer || !$artworkContainer.length) {
						return;
				}
				
				// ============================================================
				// FIX: Force clear the container completely before rendering
				// ============================================================
				$artworkContainer.empty();
				updatePatternCount("artwork", artworks ? artworks.length : 0);
				currentArtworkData = artworks || [];
				
				if (!artworks || !artworks.length) {
						$artworkContainer.html('<div class="loading-placeholder">' + 
								_tr("No artwork available for this culture") + '</div>');
						selectedArtworkId = null;
						return;
				}
				
				// ============================================================
				// FIX: Add cache-busting parameter to prevent browser caching
				// when switching between cultures with same image filenames
				// ============================================================
				var cacheBuster = '?t=' + new Date().getTime();
				var cultureId = currentCultureId || 'unknown';
				
				// Build artwork grid
				var gridHtml = '<div class="artwork-grid" data-culture="' + escapeHtml(cultureId) + '">';
				
				for (var i = 0; i < artworks.length; i++) {
						var art = artworks[i];
						var isSelected = (selectedArtworkId === art.id);
						// Add cache-busting parameter to image URL
						var imageUrl = art.imagePath + cacheBuster;
						
						gridHtml += '<div class="artwork-item' + (isSelected ? ' selected-artwork' : '') + 
								'" data-artwork-id="' + escapeHtml(art.id) + 
								'" data-search-name="' + escapeHtml(art.searchName) + 
								'" data-artwork-name="' + escapeHtml(art.name) + '">';
						
						// Image
						gridHtml += '<div class="artwork-image-wrapper">';
						gridHtml += '<img src="' + escapeHtml(imageUrl) + 
								'" alt="' + escapeHtml(art.name) + 
								'" class="artwork-image" loading="lazy" ' +
								'onerror="this.style.display=\'none\'; this.parentNode.querySelector(\'.artwork-error\').style.display=\'flex\';" />';
						gridHtml += '<div class="artwork-error" style="display:none;position:absolute;top:0;left:0;width:100%;height:100%;align-items:center;justify-content:center;background:rgba(0,0,0,0.1);color:#8A8C8E;font-size:11px;">' + _tr("Image not found") + '</div>';
						// Zoom button
						gridHtml += '<button class="artwork-zoom-btn" title="' + _tr("Zoom in") + '">' +
								'<span class="artwork-zoom-icon">⊕</span>' +
								'</button>';
						gridHtml += '</div>';
						
						// Name label
						gridHtml += '<div class="artwork-name">' + escapeHtml(art.name) + '</div>';
						
						// Native name (if available and different from display name)
						if (art.nativeName && art.nativeName !== art.name) {
								gridHtml += '<div class="artwork-native-name">' + escapeHtml(art.nativeName) + '</div>';
						}
						
						gridHtml += '</div>';
				}
				
				gridHtml += '</div>';
				$artworkContainer.html(gridHtml);
				
				// ============================================================
				// Attach click handlers for artwork items
				// ============================================================
				$artworkContainer.find('.artwork-item').on('click', function(e) {
						// Check if click was on zoom button
						if ($(e.target).closest('.artwork-zoom-btn').length) {
								return;
						}
						
						if (isArtworkClickLocked) return;
						isArtworkClickLocked = true;
						
						var $item = $(this);
						var searchName = $item.data('search-name');
						var artworkId = $item.data('artwork-id');
						var artworkName = $item.data('artwork-name');
						
						if (searchName) {
								// Toggle constellation highlight - returns true if activated, false if deactivated
								var isNowActive = stelUtils.toggleConstellationHighlight(searchName, artworkId);
								
								// Update artwork selection state based on toggle result
								if (isNowActive) {
										// Activated: mark as selected
										selectedArtworkId = artworkId;
										$artworkContainer.find('.artwork-item').removeClass('selected-artwork');
										$item.addClass('selected-artwork');
										
										// ============================================================
										// CRITICAL: Update button states when activated
										// ============================================================
										updateAllButtonStates();
										updateConstellationButtonState(artworkId);
										
								} else {
										// Deactivated: clear selection
										selectedArtworkId = null;
										$artworkContainer.find('.artwork-item').removeClass('selected-artwork');
										
										// ============================================================
										// CRITICAL: Clear pattern selection and button states when deactivated
										// ============================================================
										selectedPattern = null;
										selectedPatternId = null;
										clearAllActiveButtons();
										updateAllButtonStates();
								}
								
								console.log("[SkyCulture] Artwork clicked:", artworkName, "ID:", artworkId, "Active:", isNowActive);
						}
						
						setTimeout(function() {
								isArtworkClickLocked = false;
						}, 300);
				});
				
				// ============================================================
				// Zoom button handler
				// ============================================================
				$artworkContainer.find('.artwork-zoom-btn').on('click', function(e) {
						e.preventDefault();
						e.stopPropagation();
						
						var $item = $(this).closest('.artwork-item');
						var imgSrc = $item.find('.artwork-image').attr('src');
						var imgAlt = $item.find('.artwork-image').attr('alt');
						
						showArtworkModal(imgSrc, imgAlt);
				});
		}

		/**
		 * Update the active state of the corresponding constellation button
		 * when an artwork is clicked.
		 * 
		 * @param {string} constellationId - The ID of the constellation
		 */
		function updateConstellationButtonState(constellationId) {
				if (!$constellationsContainer) return;
				
				// Remove active class from all constellation buttons
				$constellationsContainer.find('.pattern-btn').removeClass('active');
				
				// Find and activate the matching button
				$constellationsContainer.find('.pattern-btn').each(function() {
						var $btn = $(this);
						var btnId = $btn.data('pattern-id');
						if (btnId === constellationId) {
								$btn.addClass('active');
								selectedPattern = $btn.data('pattern-name');
								selectedPatternId = constellationId;
						}
				});
		}

		/**
		 * Show artwork in a modal dialog.
		 * 
		 * @param {string} imageSrc - The image source URL
		 * @param {string} imageName - The name of the artwork
		 */
		function showArtworkModal(imageSrc, imageName) {
				// Check if modal already exists
				var $existingModal = $('#artwork-modal-overlay');
				if ($existingModal.length) {
						$existingModal.remove();
				}
				
				var modalHtml = 
						'<div id="artwork-modal-overlay" class="artwork-modal-overlay">' +
								'<div class="artwork-modal-content">' +
										'<button class="artwork-modal-close" title="' + _tr("Close") + '">✕</button>' +
										'<div class="artwork-modal-image-wrapper">' +
												'<img src="' + escapeHtml(imageSrc) + '" alt="' + escapeHtml(imageName) + '" class="artwork-modal-image" />' +
										'</div>' +
										'<div class="artwork-modal-name">' + escapeHtml(imageName) + '</div>' +
								'</div>' +
						'</div>';
				
				$('body').append(modalHtml);
				
				// Close modal on click outside or on close button
				$('#artwork-modal-overlay').on('click', function(e) {
						if (e.target === this || $(e.target).closest('.artwork-modal-close').length) {
								$(this).fadeOut(200, function() {
										$(this).remove();
								});
						}
				});
				
				// Close on Escape key
				$(document).on('keydown.artworkModal', function(e) {
						if (e.key === 'Escape') {
								$('#artwork-modal-overlay').fadeOut(200, function() {
										$(this).remove();
										$(document).off('keydown.artworkModal');
								});
						}
				});
		}

    // =====================================================================
    // 11. MAIN DATA LOADING FUNCTION
    // =====================================================================

    /**
     * Load all culture data and render all panels.
     * 
     * This is the main orchestration function that:
     * 1. Updates the current language
     * 2. Shows loading placeholders in all panels
     * 3. Fetches index.json (single source of truth)
     * 4. Extracts and renders all data types
     * 
     * @param {Function} callback - Optional callback when loading completes
     */
    function loadAllCultureData(callback) {
        if (!currentCultureId) { 
            if (callback) callback(false); 
            return; 
        }
        
        // Update current language
        getCurrentLanguage();
				   
        // Show loading states in all panels
        if ($constellationsContainer) {
            $constellationsContainer.html('<div class="loading-placeholder">' + 
                _tr("Loading constellations...") + '</div>');
        }
        if ($asterismsContainer) {
            $asterismsContainer.html('<div class="loading-placeholder">' + 
                _tr("Loading asterisms...") + '</div>');
        }
        if ($zodiacContainer) {
            $zodiacContainer.html('<div class="loading-placeholder">' + 
                _tr("Loading zodiac signs...") + '</div>');
        }
        if ($lunarContainer) {
            $lunarContainer.html('<div class="loading-placeholder">' + 
                _tr("Loading lunar mansions...") + '</div>');
        }
        if ($starsContainer) {
            $starsContainer.html('<div class="loading-placeholder">' + 
                _tr("Loading star names...") + '</div>');
        }
				
				    // Artwork loading placeholder
				if ($artworkContainer) {
						$artworkContainer.html('<div class="loading-placeholder">' + 
								_tr("Loading artwork...") + '</div>');
				}
        
        // Reset counters and clear active selections
        updatePatternCount("constellations", 0);
        updatePatternCount("asterisms", 0);
        updatePatternCount("zodiac", 0);
        updatePatternCount("lunar", 0);
        updatePatternCount("stars", 0);
				updatePatternCount("artwork", 0);        
				clearAllActiveButtons();
				
		    selectedArtworkId = null;
				if ($artworkContainer && $artworkContainer.length) {
						$artworkContainer.find('.artwork-item').removeClass('selected-artwork');
				}
        
        // Fetch index.json - the SINGLE SOURCE OF TRUTH
        $.ajax({
            url: "/api/view/skyculturedescription/index.json",
            type: 'GET',
            dataType: 'json',
            success: function(data) {
                currentCultureRawData = data;
                
                console.log("[SkyCulture] Successfully loaded index.json for culture:", currentCultureId);
                
                // Extract all data types directly from index.json
                var constellations = loadConstellationsFromIndexJson(data);
                var asterisms = loadAsterismsFromIndexJson(data);
                var zodiac = extractZodiac(data);
                var lunar = extractLunarMansions(data);
                var stars = extractStarNames(data);
								var artworks = extractArtworkFromIndexJson(data, constellations);
                
                // Render all panels
                renderConstellationsPanel(constellations);
                renderAsterismsPanel(asterisms);
                renderZodiacPanel(zodiac);
                renderLunarPanel(lunar);
                renderStarsPanel(stars);
								renderArtworkPanel(artworks);								
													
								// Render all panels
                renderConstellationsPanel(constellations, isMultiSelectModeActive);
                
                console.log("[SkyCulture] All panels rendered for culture:", currentCultureId);
                if (callback) callback(true);
            },
            error: function(xhr, status, errorThrown) {
                console.error("[SkyCulture] Failed to load index.json:", errorThrown);
                
                // Show error states in all panels
                renderConstellationsPanel([]);
                renderAsterismsPanel([]);
                renderZodiacPanel(null);
                renderLunarPanel(null);
                renderStarsPanel(null);
								renderArtworkPanel([]); // Clear artwork on error										
                
                if (callback) callback(false);
            }
        });
    }

    // =====================================================================
    // 12. SKY CULTURE MANAGEMENT FUNCTIONS
    // =====================================================================

    /**
     * Update the info iframe with the current culture description.
     */
    function updateInfoFrame() {
        if ($infoFrame && $infoFrame.length) {
            $infoFrame.attr("src", "/api/view/skyculturedescription/");
        }
    }

    /**
     * Update culture button active states based on currentCultureId.
     */
    function updateCultureButtonStates() {
        if (!$cultureContainer) return;
        $cultureContainer.find(".skyculture-btn").removeClass("active");
        $cultureContainer.find(".skyculture-btn").each(function() {
            if ($(this).data("culture-id") === currentCultureId) {
                $(this).addClass("active");
            }
        });
    }

    /**
     * Render culture selection buttons.
     * Names are automatically translated by the server.
     * 
     * @param {Array} cultures - Available cultures with translated names
     * @param {string} activeCulture - Currently active culture ID
     */
    function renderCultureButtons(cultures, activeCulture) {
        if (!$cultureContainer) return;
        $cultureContainer.empty();
        
        if (!cultures || !cultures.length) {
            $cultureContainer.html('<div class="loading-placeholder">' + 
                _tr("No sky cultures available") + '</div>');
            return;
        }
        
        if ($cultureCount && $cultureCount.length) {
            $cultureCount.text('(' + cultures.length + ')');
        }
        
        var buttonsHtml = '<div class="skyculture-buttons-grid">';
        for (var i = 0; i < cultures.length; i++) {
            var culture = cultures[i];
            var isActive = (culture.id === activeCulture);
            // culture.name is already translated by the server
            buttonsHtml += '<button type="button" class="skyculture-btn' + 
                (isActive ? ' active' : '') + 
                '" data-culture-id="' + escapeHtml(culture.id) + '">' + 
                escapeHtml(culture.name) + '</button>';
        }
        buttonsHtml += '</div>';
        $cultureContainer.html(buttonsHtml);
        
        $cultureContainer.find(".skyculture-btn").on("click", function() {
            var cultureId = $(this).data("culture-id");
            if (cultureId && cultureId !== currentCultureId) {
                setSkyCulture(cultureId);
            }
        });
    }

    /**
     * Load available sky cultures from server.
     * Names are automatically translated based on Stellarium's current language.
     * 
     * @param {Function} callback - Optional callback when loading completes
     */
    function loadSkyCultures(callback) {
        $.ajax({
            url: '/api/view/listskyculture',
            type: 'GET',
            dataType: 'json',
            success: function(data) {
                var culturesArray = [];
                for (var id in data) {
                    if (data.hasOwnProperty(id)) {
                        // data[id] contains the translated name
                        culturesArray.push({ id: id, name: data[id] });
                    }
                }
                culturesArray.sort(function(a, b) { 
                    return a.name.localeCompare(b.name); 
                });
                availableCultures = culturesArray;
                
                console.log("[SkyCulture] Loaded " + availableCultures.length + 
                    " sky cultures with translated names");
                
                if (propApi) {
                    currentCultureId = propApi.getStelProp("StelSkyCultureMgr.currentSkyCultureID") || 
                                      (availableCultures.length > 0 ? availableCultures[0].id : null);
                								
								    var culture = availableCultures.find(function(c) { return c.id === currentCultureId; });
                    currentCultureName = culture ? culture.name : currentCultureId;
                }
        
                
                renderCultureButtons(availableCultures, currentCultureId);
                updateInfoFrame();
                loadAllCultureData();
                
                if (callback) callback(true);
            },
            error: function(xhr, status, errorThrown) {
                console.error("[SkyCulture] Failed to load sky cultures:", errorThrown);
                if ($cultureContainer) {
                    $cultureContainer.html('<div class="loading-placeholder">' + 
                        _tr("Error loading sky cultures") + '</div>');
                }
                if (callback) callback(false);
            }
        });
    }

    /**
     * Set the current sky culture.
     * 
     * @param {string} cultureId - Culture ID to set (e.g., "korean", "arabic_al-sufi")
     */
    function setSkyCulture(cultureId) {
        if (!cultureId || cultureId === currentCultureId) return;
        
        console.log("[SkyCulture] Setting sky culture to: " + cultureId);
        
        if (propApi && typeof propApi.setStelProp === 'function') {
            propApi.setStelProp("StelSkyCultureMgr.currentSkyCultureID", cultureId);
        }
        
        currentCultureId = cultureId;
				
				var culture = availableCultures.find(function(c) { return c.id === cultureId; });
        currentCultureName = culture ? culture.name : cultureId;
        
				updateCultureButtonStates();
        updateInfoFrame();
        
        selectedPattern = null;
        selectedPatternId = null;
        
        loadAllCultureData();
    }

    /**
     * Refresh all data by reloading cultures.
     */
    function refresh() {
        loadSkyCultures();
    }

    // =====================================================================
    // 13. SYNCHRONIZATION LISTENERS
    // =====================================================================

		/**
		 * Setup synchronization listeners for culture and selection changes.
		 * Ensures the UI stays in sync with Stellarium's state.
		 * 
		 * Enhanced matching logic for zodiac signs:
		 * - Exact match by composite ID
		 * - Numeric index extraction from composite IDs
		 * - Name-based matching as fallback
		 * - Support for both new composite IDs and legacy simple IDs
		 */
		function setupSyncListeners() {
				// Listen to objectSelected events from tables and other components
				$(document).on("objectSelected", function(evt, data) {
						if (!syncEnabled) return;
						
						/*if (!data || !data.type || data.type === 'none' || !data.name) {
								clearAllActiveButtons();
								return;
						}*/
						
						if (!data || !data.type || data.type === 'none' || !data.name) {
								clearAllActiveButtons();
								selectedPattern = null;
								selectedPatternId = null;
								
								// Cleaar selection in Arwork tab
								if ($artworkContainer && $artworkContainer.length) {
										$artworkContainer.find('.artwork-item').removeClass('selected-artwork');
										selectedArtworkId = null;
								}
								updateAllButtonStates();
								return;
						}
						
						var found = false;
						var patternGroups = [
								{ container: $constellationsContainer, patterns: currentPatternsData.constellations, type: "constellation" },
								{ container: $asterismsContainer, patterns: currentPatternsData.asterisms, type: "asterism" },
								{ container: $zodiacContainer, patterns: currentPatternsData.zodiac, type: "zodiac" },
								{ container: $lunarContainer, patterns: currentPatternsData.lunar, type: "lunar" },
								{ container: $starsContainer, patterns: currentPatternsData.stars, type: "star" }
						];
						
						for (var g = 0; g < patternGroups.length; g++) {
								var group = patternGroups[g];
								var $container = group.container;
								var patterns = group.patterns;
								if (!$container || !$container.length || !patterns) continue;
								
								for (var i = 0; i < patterns.length; i++) {
										var pattern = patterns[i];
										var match = false;
										
										// ============================================================
										// ZODIAC MATCHING LOGIC
										// ============================================================
										if (group.type === "zodiac") {
												// METHOD 1: Match by composite ID (most reliable)
												if (data.id && pattern.id && data.id === pattern.id) {
														match = true;
												}
												// METHOD 2: Match by numeric index from composite ID
												else if (data.id && pattern.id && 
																 data.id.indexOf('zodiac_') === 0 && 
																 pattern.id.indexOf('zodiac_') === 0) {
														var dataParts = data.id.split('_');
														var patternParts = pattern.id.split('_');
														var dataIndex = parseInt(dataParts[dataParts.length - 1], 10);
														var patternIndex = parseInt(patternParts[patternParts.length - 1], 10);
														if (dataIndex === patternIndex) {
																match = true;
														}
												}
												// METHOD 3: Match by simple ID (legacy support)
												else if (data.id && pattern.simpleId && data.id === pattern.simpleId) {
														match = true;
												}
												// METHOD 4: Match by name (fallback)
												else if (data.name) {
														match = (pattern.name === data.name || 
																		 pattern.searchName === data.name ||
																		 (pattern.englishName && pattern.englishName === data.name));
												}
										}
										// ============================================================
										// LUNAR MANSION MATCHING LOGIC
										// ============================================================
										else if (group.type === "lunar") {
												// Exact match
												if (data.id && pattern.id && data.id === pattern.id) {
														match = true;
												}
												// Match by numeric index
												else if (data.id && pattern.id && 
																 data.id.indexOf('lunar_') === 0 && 
																 pattern.id.indexOf('lunar_') === 0) {
														var dataNum = parseInt(data.id.split('_')[1], 10);
														var patternNum = parseInt(pattern.id.split('_')[1], 10);
														if (dataNum === patternNum) {
																match = true;
														}
												}
												// Match by name
												else if (data.name) {
														match = (pattern.name === data.name || 
																		 pattern.searchName === data.name);
												}
										}
										// ============================================================
										// CONSTELLATION MATCHING LOGIC
										// ============================================================
										else if (group.type === "constellation") {
												// Match by original ID
												if (data.id && pattern.id) {
														if (pattern.id === data.id) {
																match = true;
														}
														// Match by abbreviation (last part of CON ID)
														else if (pattern.id.indexOf('CON ') === 0 && 
																		 data.id.indexOf('CON ') === 0) {
																var patternParts = pattern.id.split(' ');
																var dataParts = data.id.split(' ');
																if (patternParts.length >= 3 && dataParts.length >= 3) {
																		if (patternParts[2] === dataParts[2]) {
																				match = true;
																		}
																}
														}
												}
												// Match by name
												if (!match && data.name) {
														match = (pattern.name === data.name || 
																		 pattern.searchName === data.name ||
																		 (pattern.englishName && pattern.englishName === data.name));
												}
										}
										// ============================================================
										// ASTERISM MATCHING LOGIC
										// ============================================================
										else if (group.type === "asterism") {
												if (data.id && pattern.id) {
														if (pattern.id === data.id) {
																match = true;
														}
														// Match by abbreviation (last part of AST ID)
														else if (pattern.id.indexOf('AST ') === 0 && 
																		 data.id.indexOf('AST ') === 0) {
																var patternParts = pattern.id.split(' ');
																var dataParts = data.id.split(' ');
																if (patternParts.length >= 3 && dataParts.length >= 3) {
																		if (patternParts[2] === dataParts[2]) {
																				match = true;
																		}
																}
														}
												}
												if (!match && data.name) {
														match = (pattern.name === data.name || 
																		 pattern.searchName === data.name);
												}
										}
										// ============================================================
										// STAR MATCHING LOGIC
										// ============================================================
										else if (group.type === "star") {
												if (data.id && pattern.id && pattern.id === data.id) {
														match = true;
												}
												else if (data.name && pattern.name === data.name) {
														match = true;
												}
												else if (data.name && pattern.searchName === data.name) {
														match = true;
												}
										}
										
										if (match) {
												$container.find(".pattern-btn").removeClass("active");
												$container.find(".pattern-btn").each(function() {
														var $btn = $(this);
														var btnPatternId = $btn.data("pattern-id");
														// Exact match only for button activation
														if (btnPatternId === pattern.id || 
																(pattern.simpleId && btnPatternId === pattern.simpleId)) {
																$btn.addClass("active");
																selectedPattern = pattern.name;
																selectedPatternId = pattern.id;
																found = true;
																console.log("[SkyCulture] ObjectSelected matched:", pattern.name, "ID:", pattern.id);
														}
												});
												break;
										}
								}
								if (found) break;
						}
						
						if (!found) clearAllActiveButtons();
						
						if (data.type === 'constellation' && data.id && $artworkContainer && $artworkContainer.length) {
								var constellationId = data.id;
								var found = false;
								
								$artworkContainer.find('.artwork-item').each(function() {
										var $item = $(this);
										var itemId = $item.data('artwork-id');
										
										if (itemId === constellationId) {
												$artworkContainer.find('.artwork-item').removeClass('selected-artwork');
												$item.addClass('selected-artwork');
												selectedArtworkId = constellationId;
												found = true;
												console.log("[SkyCulture] Artwork synchronized from external event:", data.name);
										}
								});
								
								if (!found) {
										// Clear selection , image not found
										$artworkContainer.find('.artwork-item').removeClass('selected-artwork');
										selectedArtworkId = null;
								}
						} else if (data.type !== 'constellation') {
								// Unkown event, clear selection
								if ($artworkContainer && $artworkContainer.length) {
										$artworkContainer.find('.artwork-item').removeClass('selected-artwork');
										selectedArtworkId = null;
								}
						}
				});
				
				// ============================================================
				// LISTEN TO CULTURE CHANGES FROM SERVER (NEW)
				// ============================================================
				if (propApi) {
						$(propApi).on("stelPropertyChanged:StelSkyCultureMgr.currentSkyCultureID", 
								function(evt, data) {
										var newCultureId = data.value;
										if (newCultureId && newCultureId !== currentCultureId) {
												console.log("[SkyCulture] Culture changed from server to: " + newCultureId);
												currentCultureId = newCultureId;
												// Find the culture name from availableCultures
												var culture = availableCultures.find(function(c) { return c.id === newCultureId; });
												if (culture) currentCultureName = culture.name;
												updateCultureButtonStates();
												updateInfoFrame();
												selectedPattern = null;
												selectedPatternId = null;
												loadAllCultureData();
										}
								});
						
						// Listen to language changes
						$(propApi).on("stelPropertyChanged:StelLocaleMgr.appLanguage", 
								function(evt, data) {
										var newLang = data.value;
										if (newLang && newLang !== currentLanguage) {
												console.log("[SkyCulture] Language changed to: " + newLang);
												currentLanguage = newLang;
												// Reload cultures to get newly translated names
												refresh();
										}
								});
				}
		}

    // =====================================================================
    // 14. INITIALIZATION
    // =====================================================================

    /**
     * Initialize the sky culture module.
     * 
     * @param {string} cultureContainerSelector - Selector for culture buttons container
     * @param {Object} patternsContainers - Selectors for pattern containers
     * @param {string} iframeSelector - Selector for info iframe
     * @param {Object} options - Additional options
     */
    function init(cultureContainerSelector, patternsContainers, iframeSelector, options) {
        if (isInitialized) {
            console.warn("[SkyCulture] Module already initialized");
            return;
        }
        
        $cultureContainer = $(cultureContainerSelector || "#skyculture-buttons-container");
        options = options || {};
        $cultureCount = $(options.cultureCount || "#skyculture-culture-count");
        
        if (patternsContainers) {
            $constellationsContainer = $(patternsContainers.constellations || 
                "#constellations-buttons-container");
            $asterismsContainer = $(patternsContainers.asterisms || 
                "#asterisms-buttons-container");
            $zodiacContainer = $(patternsContainers.zodiac || 
                "#zodiac-buttons-container");
            $lunarContainer = $(patternsContainers.lunar || 
                "#lunar-buttons-container");
            $starsContainer = $(patternsContainers.stars || 
                "#stars-buttons-container");
						// Artwork container
						$artworkContainer = $(patternsContainers.artwork || 
								"#artwork-container");
				}
        
        $infoFrame = $(iframeSelector || "#vo_skycultureinfo");
        
        if (!$cultureContainer || !$cultureContainer.length) {
            console.error("[SkyCulture] Culture container not found");
            return;
        }
        
        // Get initial language
        getCurrentLanguage();
        
        setupSyncListeners();
        
        if ($("#skyculture-patterns-tabs").length) {
            $("#skyculture-patterns-tabs").tabs({ heightStyle: "content" });
        }
        
        loadSkyCultures(function(success) { 
            if (success) {
                isInitialized = true;
                console.log("[SkyCulture] Module initialized successfully v4.0.0 - Direct index.json loading");
            }
        // Create constellations tour toolbar
        createConstellationsTourToolbar();
				// Create asterisms tour toolbar
        createAsterismsTourToolbar();
				//Create Lunar Mansions tour toolbar
				createLunarMansionsTourToolbar();
				//Creat Star tour toolbar
				createStarsTourToolbar();
        });
				

    }
		
    // ========================================================================
    // TOUR BUTTON FUNCTIONS
    // ========================================================================

    /**
     * Update the active state of constellation buttons during tour.
     * 
     * @param {string} constellationName - Name of currently active constellation
     */
    function updateActiveConstellationButton(constellationName) {
        if (!$constellationsContainer) return;
        
        $constellationsContainer.find('.pattern-btn').removeClass('active');
        $constellationsContainer.find('.pattern-btn').each(function() {
            var $btn = $(this);
            var btnName = $btn.data('pattern-name');
            var searchName = $btn.data('search-name');
            if (btnName === constellationName || searchName === constellationName) {
                $btn.addClass('active');
            }
        });
    }

    /**
     * Update the selection count display in multi-select mode
     */
    function updateSelectionCount() {
        var count = selectedConstellationIds.size;
        if ($constSelectionCount && $constSelectionCount.length) {
            $constSelectionCount.text(count + ' ' + (count === 1 ? _tr("selected") : _tr("selected")));
        }
        
        // Also update the generate button state (disable if nothing selected)
        if ($constTourGenerateStaticBtn && $constTourGenerateStaticBtn.length) {
            $constTourGenerateStaticBtn.prop('disabled', count === 0 && isMultiSelectModeActive);
        }
    }
    
    /**
     * Clear all selections in multi-select mode
     */
    function clearMultiSelection() {
        selectedConstellationIds.clear();
        if ($constellationsContainer && $constellationsContainer.length) {
            // Just clear styles without re-rendering
            $constellationsContainer.find('.pattern-btn').removeClass('selected-constellation');
            $constellationsContainer.find('.pattern-btn').css('background', '');
            $constellationsContainer.find('.pattern-btn').css('border-color', '');
            $constellationsContainer.find('.pattern-btn').css('color', '');
            $constellationsContainer.find('.pattern-btn').removeClass('active');
        }
        updateSelectionCount();
    }
    
    /**
     * Get selected constellation objects for multi-select mode
     * @returns {Array|null} Array of selected constellation objects or null if none selected
     */
    function getSelectedConstellations() {
        if (!isMultiSelectModeActive || selectedConstellationIds.size === 0) {
            return null;
        }
        
        var selected = [];
        var allConstellations = currentPatternsData.constellations;
        
        if (!allConstellations) return null;
        
        for (var i = 0; i < allConstellations.length; i++) {
            var con = allConstellations[i];
            if (selectedConstellationIds.has(con.id)) {
                selected.push(con);
            }
        }
        
        return selected;
    }
    
    /**
     * Update constellation button behavior based on multi-select mode
     */
    function updateConstellationButtonsBehavior() {
        if (!$constellationsContainer || !$constellationsContainer.length) return;
        
        if (isMultiSelectModeActive) {
            // First, re-render the buttons WITHOUT the original click handler
            var currentPatterns = currentPatternsData.constellations;
            if (currentPatterns && currentPatterns.length) {
                // Re-render with handler disabled
                renderButtons($constellationsContainer, currentPatterns, "constellations", null, true);
                
                // Then attach multi-select behavior
                $constellationsContainer.find('.pattern-btn').off('click.constellation').on('click.constellation', function(e) {
                    e.preventDefault();
                    e.stopPropagation();
                    
                    var $btn = $(this);
                    var patternId = $btn.data('pattern-id');
                    var patternName = $btn.data('pattern-name');
                    
                    if (!patternId) return false;
                    
                    if (selectedConstellationIds.has(patternId)) {
                        // Deselect
                        selectedConstellationIds.delete(patternId);
                        $btn.removeClass('selected-constellation');
                        $btn.css('background', '');
                        $btn.css('border-color', '');
                        $btn.css('color', '');
                        $btn.removeClass('active');
                    } else {
                        // Select
                        selectedConstellationIds.add(patternId);
                        $btn.addClass('selected-constellation');
                        $btn.css('background', 'linear-gradient(#DCDBDA, #8A8C8E)');
                        $btn.css('border-color', '#000000');
                        $btn.css('color', '#000000');
                    }
                    
                    updateSelectionCount();
                    return false;
                });
            }
        } else {
            // Normal mode: re-render with original handler
            var currentPatterns = currentPatternsData.constellations;
            if (currentPatterns && currentPatterns.length) {
                renderButtons($constellationsContainer, currentPatterns, "constellations", 
                    function(patternName, patternType, searchName) {
                        stelUtils.toggleConstellationHighlight(searchName || patternName);
                        updateAllButtonStates();
                    },
                    false  // Handler enabled
                );
            }
            
            // Also clear any selected constellation highlight from previous multi-select
            stelUtils.clearConstellationHighlight();
        }
    }

    /**
     * Show a temporary notification message.
     * Uses the existing notification system if available.
     * 
     * @param {string} message - Message to display
     */
    function showTourNotification(message) {
        // Try to use existing notification system from stellarium-utils
        if (stelUtils && typeof stelUtils.showNotification === 'function') {
            stelUtils.showNotification(message);
        } else {
            // Fallback: create temporary notification div
						var escapedMessage = escapeHtml(message);
            var $notification = $(
                '<div class="tour-notification" style="position:fixed;bottom:20px;right:20px;' +
                'background:linear-gradient(#5D5F62, #3A3C3E);color:#DCDBDA;padding:8px 15px;' +
                'border-radius:4px;z-index:10000;font-size:11px;border:1px solid #2A2C2E;box-shadow:0 2px 8px rgba(0,0,0,0.2);">' + 
                escapedMessage + '</div>'
            );
            $('body').append($notification);
            setTimeout(function() {
                $notification.fadeOut(300, function() { $notification.remove(); });
            }, 3000);
        }
    }

    /**
     * Generate a complete SSC script for constellation tour using dynamic constellation retrieval.
     * Uses ConstellationMgr.getConstellationsEnglishNames() to fetch names dynamically - works with any culture.
     * 
     * Features:
     * - Dynamic constellation list (no hardcoded array needed)
     * - Automatic translation support via i18n.inc
     * - Smooth zoom transitions (autoZoomIn + zoomTo)
     * - Constellation art display during tour
     * - GUI hiding for immersive experience
     * - Proper state restoration
     * - Welcome message with instructions
     * @returns {void}		 
     * @function generateConstellationsTourScriptDynamic
     */
    function generateConstellationsTourScriptDynamic() {
        var cultureName = currentCultureName || currentCultureId;
        var cultureId = currentCultureId;
        var totalConstellations = currentPatternsData.constellations ? currentPatternsData.constellations.length : 0;
        
        // Get current values from UI (same as static version)
        var duration = ($constTourDuration && $constTourDuration.length) ? parseFloat($constTourDuration.val()) : 4;
        var isolate = ($constTourIsolate && $constTourIsolate.length) ? $constTourIsolate.is(':checked') : true;
        var pickLastOnly = ($constTourPickLastOnly && $constTourPickLastOnly.length) ? $constTourPickLastOnly.is(':checked') : false;				
        var zoomFov = ($constTourZoomFov && $constTourZoomFov.length) ? parseFloat($constTourZoomFov.val()) : 40;
        var overviewFov = ($constTourOverviewFov && $constTourOverviewFov.length) ? parseFloat($constTourOverviewFov.val()) : 90;
        
        var fontSize = ($constTourFontSize && $constTourFontSize.length) ? parseInt($constTourFontSize.val()) : 16;
        var showLines = ($constTourShowLines && $constTourShowLines.length) ? $constTourShowLines.is(':checked') : true;				
        var lineThickness = ($constTourLineThickness && $constTourLineThickness.length) ? parseInt($constTourLineThickness.val()) : 2;
        var boundariesThickness = ($constTourBoundariesThickness && $constTourBoundariesThickness.length) ? parseInt($constTourBoundariesThickness.val()) : 1;
        var hullsThickness = ($constTourHullsThickness && $constTourHullsThickness.length) ? parseInt($constTourHullsThickness.val()) : 1;
        
        var showArt = ($constTourShowArt && $constTourShowArt.length) ? $constTourShowArt.is(':checked') : true;
        var showBoundaries = ($constTourShowBoundaries && $constTourShowBoundaries.length) ? $constTourShowBoundaries.is(':checked') : false;
        var showHulls = ($constTourShowHulls && $constTourShowHulls.length) ? $constTourShowHulls.is(':checked') : false;
        
        var artIntensity = ($constTourArtIntensity && $constTourArtIntensity.length) ? (parseFloat($constTourArtIntensity.val()) / 100).toFixed(2) : 0.80;
        var artFade = ($constTourArtFade && $constTourArtFade.length) ? parseFloat($constTourArtFade.val()) : 2.0;
        var linesFade = ($constTourLinesFade && $constTourLinesFade.length) ? parseFloat($constTourLinesFade.val()) : 1.0;
        var labelsFade = ($constTourLabelsFade && $constTourLabelsFade.length) ? parseFloat($constTourLabelsFade.val()) : 1.0;
        var boundariesFade = ($constTourBoundariesFade && $constTourBoundariesFade.length) ? parseFloat($constTourBoundariesFade.val()) : 1.0;
        var hullsFade = ($constTourHullsFade && $constTourHullsFade.length) ? parseFloat($constTourHullsFade.val()) : 1.0;
        
        // Color values (same as static)
        var linesColorR = ($('#const-lines-color-r').length) ? (parseFloat($('#const-lines-color-r').val()) / 100).toFixed(2) : 0.80;
        var linesColorG = ($('#const-lines-color-g').length) ? (parseFloat($('#const-lines-color-g').val()) / 100).toFixed(2) : 0.80;
        var linesColorB = ($('#const-lines-color-b').length) ? (parseFloat($('#const-lines-color-b').val()) / 100).toFixed(2) : 0.80;
        
        var labelsColorR = ($('#const-labels-color-r').length) ? (parseFloat($('#const-labels-color-r').val()) / 100).toFixed(2) : 1.00;
        var labelsColorG = ($('#const-labels-color-g').length) ? (parseFloat($('#const-labels-color-g').val()) / 100).toFixed(2) : 1.00;
        var labelsColorB = ($('#const-labels-color-b').length) ? (parseFloat($('#const-labels-color-b').val()) / 100).toFixed(2) : 1.00;
        
        var boundariesColorR = ($('#const-boundaries-color-r').length) ? (parseFloat($('#const-boundaries-color-r').val()) / 100).toFixed(2) : 0.80;
        var boundariesColorG = ($('#const-boundaries-color-g').length) ? (parseFloat($('#const-boundaries-color-g').val()) / 100).toFixed(2) : 0.30;
        var boundariesColorB = ($('#const-boundaries-color-b').length) ? (parseFloat($('#const-boundaries-color-b').val()) / 100).toFixed(2) : 0.30;
        
        var hullsColorR = ($('#const-hulls-color-r').length) ? (parseFloat($('#const-hulls-color-r').val()) / 100).toFixed(2) : 0.60;
        var hullsColorG = ($('#const-hulls-color-g').length) ? (parseFloat($('#const-hulls-color-g').val()) / 100).toFixed(2) : 0.20;
        var hullsColorB = ($('#const-hulls-color-b').length) ? (parseFloat($('#const-hulls-color-b').val()) / 100).toFixed(2) : 0.20;
        
        if (totalConstellations === 0) {
            showTourNotification(_tr("No constellations available for this culture"));
            return;
        }
        
        // Build script header
        var script = '';
        script += '//\n';
        script += '// ============================================================\n';
        script += '// Name: ' + cultureName + ' Constellations Tour (Dynamic)\n';
        script += '// ============================================================\n';
        script += '// Version: 1.0\n';
        script += '// Author: Stellarium Remote Control\n';
        script += '// License: Public Domain\n';
        script += '// Culture: ' + cultureId + '\n';
        script += '// Duration per Constellation: ' + duration + ' seconds\n';
        script += '// Zoom FOV: ' + zoomFov + ' degrees\n';
        script += '// Overview FOV: ' + overviewFov + ' degrees\n';
        script += '// Font Size: ' + fontSize + ' pixels\n';
        script += '// Line Thickness: ' + lineThickness + ' pixels\n';
        script += '// Boundaries Thickness: ' + boundariesThickness + ' pixels\n';
        script += '// Hulls Thickness: ' + hullsThickness + ' pixels\n';
        script += '// Isolate Mode: ' + (isolate ? 'ON' : 'OFF') + '\n';
        script += '// Show Artwork: ' + (showArt ? 'ON' : 'OFF') + '\n';
        script += '// Show Boundaries: ' + (showBoundaries ? 'ON' : 'OFF') + '\n';
        script += '// Show Hulls: ' + (showHulls ? 'ON' : 'OFF') + '\n';
        script += '// Art Intensity: ' + artIntensity + '\n';
        script += '// Generated: ' + new Date().toLocaleString() + '\n';
        script += '// ============================================================\n';
        script += '//\n';
        script += '// Description: This script uses ConstellationMgr.getConstellationsEnglishNames()\n';
        script += '//              to dynamically fetch constellation names from the current\n';
        script += '//              sky culture. It works with any culture without modification.\n';
        script += '// ============================================================\n\n';
        
        // Include translation support
        script += '// ============================================================\n';
        script += '// TRANSLATION SUPPORT\n';
        script += '// ============================================================\n';
        script += 'include("i18n.inc");\n\n';
        
        // Culture verification
        script += '// ============================================================\n';
        script += '// CULTURE VERIFICATION\n';
        script += '// ============================================================\n';
        script += 'var currentCulture = core.getSkyCulture();\n';
        script += 'var requiredCulture = "' + cultureId + '";\n';
        script += 'core.debug(tr("Current culture: ") + currentCulture);\n';
        script += 'core.debug(tr("Required culture: ") + requiredCulture);\n\n';
        script += 'if (currentCulture !== requiredCulture) {\n';
        script += '    core.debug(tr("Switching to required sky culture: ") + requiredCulture);\n';
        script += '    core.setSkyCulture(requiredCulture);\n';
        script += '    core.wait(1);\n';
        script += '}\n\n';
        
        // Initial setup (same as static)
        script += '// ============================================================\n';
        script += '// INITIAL SETUP\n';
        script += '// ============================================================\n';
        script += 'core.clear("starchart");\n';
        script += 'LandscapeMgr.setFlagLandscape(false);\n';
        script += 'core.setGuiVisible(false);\n';
        script += 'StelMovementMgr.setFlagTracking(false);\n\n';
        
        // Save original settings (abbreviated for dynamic)
        script += '// ============================================================\n';
        script += '// SAVE ORIGINAL SETTINGS\n';
        script += '// ============================================================\n';
        script += 'var oldArtState = ConstellationMgr.getFlagArt();\n';
        script += 'var oldBoundariesState = ConstellationMgr.getFlagBoundaries();\n';
        script += 'var oldHullsState = ConstellationMgr.getFlagHulls();\n';
        script += 'var oldLinesState = ConstellationMgr.getFlagLines();\n';
        script += 'var oldLabelsState = ConstellationMgr.getFlagLabels();\n';
        script += 'var oldIsolateState = ConstellationMgr.getFlagIsolateSelected();\n';
				script += 'var oldPickLastState = ConstellationMgr.getFlagConstellationPick();\n'
        script += 'var oldFontSize = ConstellationMgr.getFontSize();\n';
        script += 'var oldLineThickness = ConstellationMgr.getConstellationLineThickness();\n';
        script += 'var oldArtIntensity = ConstellationMgr.getArtIntensity();\n';
        script += 'var oldLinesColor = ConstellationMgr.getLinesColor();\n';
        script += 'var oldLabelsColor = ConstellationMgr.getLabelsColor();\n';
        script += 'var oldBoundariesColor = ConstellationMgr.getBoundariesColor();\n';
        script += 'var oldHullsColor = ConstellationMgr.getHullsColor();\n';
        script += 'var oldLinesFade = ConstellationMgr.getLinesFadeDuration();\n';
        script += 'var oldLabelsFade = ConstellationMgr.getLabelsFadeDuration();\n';
        script += 'var oldBoundariesFade = ConstellationMgr.getBoundariesFadeDuration();\n';
        script += 'var oldHullsFade = ConstellationMgr.getHullsFadeDuration();\n';
        script += 'var oldArtFade = ConstellationMgr.getArtFadeDuration();\n\n';
        
        // Configure appearance (same as static)
        script += '// ============================================================\n';
        script += '// CONFIGURE APPEARANCE\n';
        script += '// ============================================================\n';
        script += 'var linesColor = core.vec3f(' + linesColorR + ', ' + linesColorG + ', ' + linesColorB + ');\n';
        script += 'ConstellationMgr.setLinesColor(linesColor);\n';
        script += 'var labelsColor = core.vec3f(' + labelsColorR + ', ' + labelsColorG + ', ' + labelsColorB + ');\n';
        script += 'ConstellationMgr.setLabelsColor(labelsColor);\n';
        script += 'var boundariesColor = core.vec3f(' + boundariesColorR + ', ' + boundariesColorG + ', ' + boundariesColorB + ');\n';
        script += 'ConstellationMgr.setBoundariesColor(boundariesColor);\n';
        script += 'var hullsColor = core.vec3f(' + hullsColorR + ', ' + hullsColorG + ', ' + hullsColorB + ');\n';
        script += 'ConstellationMgr.setHullsColor(hullsColor);\n\n';
        
        script += 'ConstellationMgr.setFontSize(' + fontSize + ');\n';
        script += 'ConstellationMgr.setConstellationLineThickness(' + lineThickness + ');\n';
        script += 'ConstellationMgr.setBoundariesThickness(' + boundariesThickness + ');\n';
        script += 'ConstellationMgr.setHullsThickness(' + hullsThickness + ');\n\n';
        
        script += 'ConstellationMgr.setArtIntensity(' + artIntensity + ');\n';
        script += 'ConstellationMgr.setArtFadeDuration(' + artFade + ');\n';
        script += 'ConstellationMgr.setLinesFadeDuration(' + linesFade + ');\n';
        script += 'ConstellationMgr.setLabelsFadeDuration(' + labelsFade + ');\n';
        script += 'ConstellationMgr.setBoundariesFadeDuration(' + boundariesFade + ');\n';
        script += 'ConstellationMgr.setHullsFadeDuration(' + hullsFade + ');\n\n';
        
        script += 'ConstellationMgr.setFlagArt(' + (showArt ? 'true' : 'false') + ');\n';
        script += 'ConstellationMgr.setFlagBoundaries(' + (showBoundaries ? 'true' : 'false') + ');\n';
        script += 'ConstellationMgr.setFlagHulls(' + (showHulls ? 'true' : 'false') + ');\n';
        script += 'ConstellationMgr.setFlagLines(' + (showLines ? 'true' : 'false') + ');\n';
        script += 'ConstellationMgr.setFlagLabels(true);\n';
        script += 'ConstellationMgr.setFlagIsolateSelected(' + (isolate ? 'true' : 'false') + ');\n\n';
        script += 'ConstellationMgr.setFlagConstellationPick(' + (pickLastOnly ? 'true' : 'false') + ');\n\n';				
        
        // Welcome message
        script += '// ============================================================\n';
        script += '// WELCOME MESSAGE\n';
        script += '// ============================================================\n';
        script += 'var msg = tr("Welcome to the dynamic ");\n';
        script += 'msg += "' + cultureName + '";\n';
        script += 'msg += tr(" constellations tour!\\n\\nThis script dynamically fetches constellation names.\\n\\nPress Ctrl+T to show toolbar.");\n';
        script += 'var labelId = LabelMgr.labelScreen(msg, 100, 100, false, 16, "#ffaa66");\n';
        script += 'LabelMgr.setLabelShow(labelId, true);\n';
        script += 'core.wait(6);\n';
        script += 'LabelMgr.setLabelShow(labelId, false);\n';
        script += 'LabelMgr.deleteLabel(labelId);\n\n';
        
        // Dynamic constellation retrieval
        script += '// ============================================================\n';
        script += '// GET CONSTELLATION LIST\n';
        script += '// ============================================================\n';
        script += 'var constellations = ConstellationMgr.getConstellationsEnglishNames();\n';
        script += 'core.debug(tr("Total constellations found: ") + constellations.length);\n\n';
        
        // Tour configuration
        script += '// ============================================================\n';
        script += '// TOUR CONFIGURATION\n';
        script += '// ============================================================\n';
        script += 'var DURATION = ' + duration + ';\n';
        script += 'var ZOOM_FOV = ' + zoomFov + ';\n';
        script += 'var OVERVIEW_FOV = ' + overviewFov + ';\n\n';
        
        // Tour execution
        // ============================================================
        // TOUR EXECUTION - Use core.selectConstellationByName
        // ============================================================
        script += '// ============================================================\n';
        script += '// TOUR EXECUTION\n';
        script += '// ============================================================\n';
        script += 'core.debug(tr("Starting constellations tour..."));\n';
        script += 'core.debug(tr("Total constellations in array: ") + constellations.length);\n\n';
        
        script += 'for (var i = 0; i < constellations.length; i++) {\n';
        script += '    var constName = constellations[i];\n';
        script += '    var currentNum = i + 1;\n';
        script += '    \n';
        script += '    core.debug(tr("Visiting constellation") + " " + currentNum + "/" + constellations.length + ": " + constName);\n';
        script += '    \n';
        script += '    // Select constellation (does not auto-enable isolation)\n';
        script += '    core.selectConstellationByName(constName);\n';
        script += '    \n';
        script += '    // Move view to center on the constellation\n';
        script += '    core.moveToObject(constName, 2);\n';
        script += '    core.wait(1);\n';
        script += '    \n';
        script += '    // Zoom in for detailed view\n';
        script += '    StelMovementMgr.zoomTo(ZOOM_FOV, 2);\n';
        script += '    core.wait(1);\n';
        script += '    \n';
        script += '    // Display constellation name on screen\n';
        script += '    var nameLabel = LabelMgr.labelScreen(constName, 50, 50, false, ' + fontSize + ', "#ffaa66");\n';
        script += '    LabelMgr.setLabelShow(nameLabel, true);\n';
        script += '    core.wait(DURATION);\n';
        script += '    LabelMgr.deleteLabel(nameLabel);\n';
        script += '    \n';
        script += '    // Zoom out for overview before next constellation\n';
        script += '    StelMovementMgr.zoomTo(OVERVIEW_FOV, 2);\n';
        script += '    core.wait(1);\n';
        script += '}\n\n';
        
        // Reset and cleanup (same as static)
        script += '// ============================================================\n';
        script += '// RESET AND CLEANUP\n';
        script += '// ============================================================\n';
        script += 'core.debug(tr("Tour completed! Restoring original settings..."));\n\n';
        script += 'ConstellationMgr.deselectConstellations();\n';
        script += 'ConstellationMgr.setFlagArt(oldArtState);\n';
        script += 'ConstellationMgr.setFlagBoundaries(oldBoundariesState);\n';
        script += 'ConstellationMgr.setFlagHulls(oldHullsState);\n';
        script += 'ConstellationMgr.setFlagLines(oldLinesState);\n';
        script += 'ConstellationMgr.setFlagLabels(oldLabelsState);\n';
        script += 'ConstellationMgr.setFlagIsolateSelected(oldIsolateState);\n';
        script += 'ConstellationMgr.setFlagConstellationPick(oldPickLastState);\n\n';				
        script += 'ConstellationMgr.setFontSize(oldFontSize);\n';
        script += 'ConstellationMgr.setConstellationLineThickness(oldLineThickness);\n';
        script += 'ConstellationMgr.setArtIntensity(oldArtIntensity);\n';
        script += 'ConstellationMgr.setLinesColor(oldLinesColor);\n';
        script += 'ConstellationMgr.setLabelsColor(oldLabelsColor);\n';
        script += 'ConstellationMgr.setBoundariesColor(oldBoundariesColor);\n';
        script += 'ConstellationMgr.setHullsColor(oldHullsColor);\n';
        script += 'ConstellationMgr.setLinesFadeDuration(oldLinesFade);\n';
        script += 'ConstellationMgr.setLabelsFadeDuration(oldLabelsFade);\n';
        script += 'ConstellationMgr.setBoundariesFadeDuration(oldBoundariesFade);\n';
        script += 'ConstellationMgr.setHullsFadeDuration(oldHullsFade);\n';
        script += 'ConstellationMgr.setArtFadeDuration(oldArtFade);\n\n';
        
        script += 'GridLinesMgr.setFlagGridlines(true);\n';
        script += 'core.setGuiVisible(true);\n';
        script += 'core.clear("natural");\n';
        script += 'StelMovementMgr.zoomTo(100, 3);\n';
        script += 'core.moveToAltAzi(40.0, 180.0, 3.0);\n\n';
        script += 'core.debug(tr("Constellations tour completed successfully!"));\n';
        
        // Insert into editor
        if (window._stelScriptEditor && typeof window._stelScriptEditor.setEditorContent === 'function') {
            window._stelScriptEditor.setEditorContent(script);
            switchToScriptEditor();
            showTourNotification(_tr("Dynamic constellations tour script generated and inserted into editor"));
        } else {
            copyToClipboard(script);
            showTourNotification(_tr("Script copied to clipboard"));
        }
    }
		
		 /**
     * Generate a complete SSC script for constellation tour using a static array.
     * The array contains all constellation names extracted from the current culture's index.json.
     * 
     * This method is useful for:
     * - Large cultures where dynamic retrieval might be slow
     * - Situations where constellation names need to be verified
     * - Creating standalone scripts that don't depend on current culture settings
     * 
     * WARNING: For cultures with many constellations (150+), the generated array will be very large.
     * If the script is too large to send via API, it will be saved as a file.
     * 
     * Generate a static SSC script for constellations tour with embedded array.
     * Forces the correct sky culture at runtime to ensure all constellation names are recognized.
     * Includes full visual customization: colors, line thickness, boundaries, hulls, artwork, fade effects.
     * @returns {void}
     * 
     * @function generateConstellationsTourScriptStatic
     */
    function generateConstellationsTourScriptStatic() {
			  // Determine which constellations to use (Multi-Select or All)
        var constellationsToUse = null;
        var isMultiSelect = isMultiSelectModeActive && selectedConstellationIds.size > 0;
        
        if (isMultiSelect) {
            constellationsToUse = getSelectedConstellations();
            if (!constellationsToUse || constellationsToUse.length === 0) {
                showTourNotification(_tr("No constellations selected. Please select at least one constellation."));
                return;
            }
            showTourNotification(_tr("Generating script for ") + constellationsToUse.length + _tr(" selected constellations"));
        } else {
            constellationsToUse = currentPatternsData.constellations;
            if (!constellationsToUse || constellationsToUse.length === 0) {
                showTourNotification(_tr("No constellations available for this culture"));
                return;
            }
        }
        
        var constellations = constellationsToUse;
        var totalConstellations = constellations.length;
        var cultureName = currentCultureName || currentCultureId;
        var cultureId = currentCultureId;
        
        // Add indicator in header if multi-select mode was used
        var selectionNote = '';
        if (isMultiSelect) {
            selectionNote = '// Selected Constellations: ' + selectedConstellationIds.size + ' out of ' + currentPatternsData.constellations.length + '\n';
            selectionNote += '// This script includes only the constellations manually selected by the user.\n';
        }
			
      //  var constellations = currentPatternsData.constellations;
        //var cultureName = currentCultureName || currentCultureId;
        //var cultureId = currentCultureId;
       // var totalConstellations = constellations ? constellations.length : 0;
        
        // Get current values from UI
        var duration = ($constTourDuration && $constTourDuration.length) ? parseFloat($constTourDuration.val()) : 4;
        var isolate = ($constTourIsolate && $constTourIsolate.length) ? $constTourIsolate.is(':checked') : true;
        var pickLastOnly = ($constTourPickLastOnly && $constTourPickLastOnly.length) ? $constTourPickLastOnly.is(':checked') : false;				
        var zoomFov = ($constTourZoomFov && $constTourZoomFov.length) ? parseFloat($constTourZoomFov.val()) : 30;
        var overviewFov = ($constTourOverviewFov && $constTourOverviewFov.length) ? parseFloat($constTourOverviewFov.val()) : 60;
        
        var fontSize = ($constTourFontSize && $constTourFontSize.length) ? parseInt($constTourFontSize.val()) : 16;
        var showLines = ($constTourShowLines && $constTourShowLines.length) ? $constTourShowLines.is(':checked') : true;				
        var lineThickness = ($constTourLineThickness && $constTourLineThickness.length) ? parseInt($constTourLineThickness.val()) : 2;
        var boundariesThickness = ($constTourBoundariesThickness && $constTourBoundariesThickness.length) ? parseInt($constTourBoundariesThickness.val()) : 1;
        var hullsThickness = ($constTourHullsThickness && $constTourHullsThickness.length) ? parseInt($constTourHullsThickness.val()) : 1;
        
        var showArt = ($constTourShowArt && $constTourShowArt.length) ? $constTourShowArt.is(':checked') : true;
        var showBoundaries = ($constTourShowBoundaries && $constTourShowBoundaries.length) ? $constTourShowBoundaries.is(':checked') : false;
        var showHulls = ($constTourShowHulls && $constTourShowHulls.length) ? $constTourShowHulls.is(':checked') : false;
        
        var artIntensity = ($constTourArtIntensity && $constTourArtIntensity.length) ? (parseFloat($constTourArtIntensity.val()) / 100).toFixed(2) : 0.80;
        var artFade = ($constTourArtFade && $constTourArtFade.length) ? parseFloat($constTourArtFade.val()) : 2.0;
        var linesFade = ($constTourLinesFade && $constTourLinesFade.length) ? parseFloat($constTourLinesFade.val()) : 1.0;
        var labelsFade = ($constTourLabelsFade && $constTourLabelsFade.length) ? parseFloat($constTourLabelsFade.val()) : 1.0;
        var boundariesFade = ($constTourBoundariesFade && $constTourBoundariesFade.length) ? parseFloat($constTourBoundariesFade.val()) : 1.0;
        var hullsFade = ($constTourHullsFade && $constTourHullsFade.length) ? parseFloat($constTourHullsFade.val()) : 1.0;
        
        // Color values
        var linesColorR = ($('#const-lines-color-r').length) ? (parseFloat($('#const-lines-color-r').val()) / 100).toFixed(2) : 0.80;
        var linesColorG = ($('#const-lines-color-g').length) ? (parseFloat($('#const-lines-color-g').val()) / 100).toFixed(2) : 0.80;
        var linesColorB = ($('#const-lines-color-b').length) ? (parseFloat($('#const-lines-color-b').val()) / 100).toFixed(2) : 0.80;
        
        var labelsColorR = ($('#const-labels-color-r').length) ? (parseFloat($('#const-labels-color-r').val()) / 100).toFixed(2) : 1.00;
        var labelsColorG = ($('#const-labels-color-g').length) ? (parseFloat($('#const-labels-color-g').val()) / 100).toFixed(2) : 1.00;
        var labelsColorB = ($('#const-labels-color-b').length) ? (parseFloat($('#const-labels-color-b').val()) / 100).toFixed(2) : 1.00;
        
        var boundariesColorR = ($('#const-boundaries-color-r').length) ? (parseFloat($('#const-boundaries-color-r').val()) / 100).toFixed(2) : 0.80;
        var boundariesColorG = ($('#const-boundaries-color-g').length) ? (parseFloat($('#const-boundaries-color-g').val()) / 100).toFixed(2) : 0.30;
        var boundariesColorB = ($('#const-boundaries-color-b').length) ? (parseFloat($('#const-boundaries-color-b').val()) / 100).toFixed(2) : 0.30;
        
        var hullsColorR = ($('#const-hulls-color-r').length) ? (parseFloat($('#const-hulls-color-r').val()) / 100).toFixed(2) : 0.60;
        var hullsColorG = ($('#const-hulls-color-g').length) ? (parseFloat($('#const-hulls-color-g').val()) / 100).toFixed(2) : 0.20;
        var hullsColorB = ($('#const-hulls-color-b').length) ? (parseFloat($('#const-hulls-color-b').val()) / 100).toFixed(2) : 0.20;
        
        if (!constellations || totalConstellations === 0) {
            showTourNotification(_tr("No constellations available for this culture"));
            return;
        }
        
        var isLargeCulture = totalConstellations > 200;
        
        // Build script header
        var script = '';
        script += '//\n';
        script += '// ============================================================\n';
        script += '// Name: ' + cultureName + ' Constellations Tour\n';
        script += '// ============================================================\n';
        script += '// Version: 1.0\n';
        script += '// Author: Stellarium Remote Control\n';
        script += '// License: Public Domain\n';
        script += '// Culture: ' + cultureId + '\n';
        script += '// Total Constellations: ' + totalConstellations + '\n';
        script += '// Duration per Constellation: ' + duration + ' seconds\n';
        script += '// Zoom FOV: ' + zoomFov + ' degrees\n';
        script += '// Overview FOV: ' + overviewFov + ' degrees\n';
        script += '// Select Last Only: ' + (pickLastOnly ? 'ON' : 'OFF') + '\n';
        script += '// Font Size: ' + fontSize + ' pixels\n';
        script += '// Show Lines: ' + (showLines ? 'ON' : 'OFF') + '\n';
        script += '// Line Thickness: ' + lineThickness + ' pixels\n';
        script += '// Boundaries Thickness: ' + boundariesThickness + ' pixels\n';
        script += '// Hulls Thickness: ' + hullsThickness + ' pixels\n';
        script += '// Isolate Mode: ' + (isolate ? 'ON' : 'OFF') + '\n';
        script += '// Show Artwork: ' + (showArt ? 'ON' : 'OFF') + '\n';
        script += '// Show Boundaries: ' + (showBoundaries ? 'ON' : 'OFF') + '\n';
        script += '// Show Hulls: ' + (showHulls ? 'ON' : 'OFF') + '\n';
        script += '// Art Intensity: ' + artIntensity + '\n';
        script += '// Art Fade Duration: ' + artFade + ' seconds\n';
        script += '// Lines Fade Duration: ' + linesFade + ' seconds\n';
        script += '// Labels Fade Duration: ' + labelsFade + ' seconds\n';
        script += '// Boundaries Fade Duration: ' + boundariesFade + ' seconds\n';
        script += '// Hulls Fade Duration: ' + hullsFade + ' seconds\n';
        script += '// Lines Color: RGB(' + linesColorR + ', ' + linesColorG + ', ' + linesColorB + ')\n';
        script += '// Labels Color: RGB(' + labelsColorR + ', ' + labelsColorG + ', ' + labelsColorB + ')\n';
        script += '// Boundaries Color: RGB(' + boundariesColorR + ', ' + boundariesColorG + ', ' + boundariesColorB + ')\n';
        script += '// Hulls Color: RGB(' + hullsColorR + ', ' + hullsColorG + ', ' + hullsColorB + ')\n';
        script += '// Generated: ' + new Date().toLocaleString() + '\n';
        script += '// ============================================================\n';
        
        if (isLargeCulture) {
            script += '//\n';
            script += '// ⚠️  IMPORTANT NOTICE for Large Culture ⚠️\n';
            script += '//\n';
            script += '// This culture has ' + totalConstellations + ' constellations.\n';
            script += '// If the script fails to send via API, copy and paste it manually\n';
            script += '// into Stellarium\'s Script Console, or save as .ssc file in:\n';
            script += '// - Windows: %USERPROFILE%\\Stellarium\\scripts\\\n';
            script += '// - macOS: ~/Library/Application Support/Stellarium/scripts/\n';
            script += '// - Linux: ~/.stellarium/scripts/\n';
            script += '//\n';
        }
        
        script += '// ============================================================\n\n';
        
        // Include translation support
        script += '// ============================================================\n';
        script += '// TRANSLATION SUPPORT\n';
        script += '// ============================================================\n';
        script += 'include("i18n.inc");\n\n';
        
        // Culture verification
        script += '// ============================================================\n';
        script += '// CULTURE VERIFICATION\n';
        script += '// ============================================================\n';
        script += 'var currentCulture = core.getSkyCulture();\n';
        script += 'var requiredCulture = "' + cultureId + '";\n';
        script += 'core.debug(tr("Current culture: ") + currentCulture);\n';
        script += 'core.debug(tr("Required culture: ") + requiredCulture);\n\n';
        script += 'if (currentCulture !== requiredCulture) {\n';
        script += '    core.debug(tr("Switching to required sky culture: ") + requiredCulture);\n';
        script += '    core.setSkyCulture(requiredCulture);\n';
        script += '    core.wait(1);\n';
        script += '    core.debug(tr("Culture switched."));\n';
        script += '}\n\n';
        
        // Initial setup
        script += '// ============================================================\n';
        script += '// INITIAL SETUP\n';
        script += '// ============================================================\n';
        script += 'core.clear("starchart");\n\n';
        script += 'GridLinesMgr.setFlagGridlines(false);\n\n';
        script += 'core.setGuiVisible(false);\n\n';
        script += 'StelMovementMgr.setFlagTracking(false);\n\n';
        
        // Save original settings
        script += '// ============================================================\n';
        script += '// SAVE ORIGINAL SETTINGS\n';
        script += '// ============================================================\n';
        script += 'var oldArtState = ConstellationMgr.getFlagArt();\n';
        script += 'var oldBoundariesState = ConstellationMgr.getFlagBoundaries();\n';
        script += 'var oldHullsState = ConstellationMgr.getFlagHulls();\n';
        script += 'var oldLinesState = ConstellationMgr.getFlagLines();\n';
        script += 'var oldLabelsState = ConstellationMgr.getFlagLabels();\n';
        script += 'var oldIsolateState = ConstellationMgr.getFlagIsolateSelected();\n';
				script += 'var oldPickLastState = ConstellationMgr.getFlagConstellationPick();\n'
        script += 'var oldFontSize = ConstellationMgr.getFontSize();\n';
        script += 'var oldLineThickness = ConstellationMgr.getConstellationLineThickness();\n';
        script += 'var oldBoundariesThickness = ConstellationMgr.getBoundariesThickness();\n';
        script += 'var oldHullsThickness = ConstellationMgr.getHullsThickness();\n';
        script += 'var oldArtIntensity = ConstellationMgr.getArtIntensity();\n';
        script += 'var oldLinesColor = ConstellationMgr.getLinesColor();\n';
        script += 'var oldLabelsColor = ConstellationMgr.getLabelsColor();\n';
        script += 'var oldBoundariesColor = ConstellationMgr.getBoundariesColor();\n';
        script += 'var oldHullsColor = ConstellationMgr.getHullsColor();\n';
        script += 'var oldLinesFade = ConstellationMgr.getLinesFadeDuration();\n';
        script += 'var oldLabelsFade = ConstellationMgr.getLabelsFadeDuration();\n';
        script += 'var oldBoundariesFade = ConstellationMgr.getBoundariesFadeDuration();\n';
        script += 'var oldHullsFade = ConstellationMgr.getHullsFadeDuration();\n';
        script += 'var oldArtFade = ConstellationMgr.getArtFadeDuration();\n\n';
        
        // Configure appearance
        script += '// ============================================================\n';
        script += '// CONFIGURE APPEARANCE\n';
        script += '// ============================================================\n';
        script += '// Set colors\n';
        script += 'var linesColor = core.vec3f(' + linesColorR + ', ' + linesColorG + ', ' + linesColorB + ');\n';
        script += 'ConstellationMgr.setLinesColor(linesColor);\n\n';
        script += 'var labelsColor = core.vec3f(' + labelsColorR + ', ' + labelsColorG + ', ' + labelsColorB + ');\n';
        script += 'ConstellationMgr.setLabelsColor(labelsColor);\n\n';
        script += 'var boundariesColor = core.vec3f(' + boundariesColorR + ', ' + boundariesColorG + ', ' + boundariesColorB + ');\n';
        script += 'ConstellationMgr.setBoundariesColor(boundariesColor);\n\n';
        script += 'var hullsColor = core.vec3f(' + hullsColorR + ', ' + hullsColorG + ', ' + hullsColorB + ');\n';
        script += 'ConstellationMgr.setHullsColor(hullsColor);\n\n';
        
        script += '// Set font size and line thickness\n';
        script += 'ConstellationMgr.setFontSize(' + fontSize + ');\n';
        script += 'ConstellationMgr.setConstellationLineThickness(' + lineThickness + ');\n';
        script += 'ConstellationMgr.setBoundariesThickness(' + boundariesThickness + ');\n';
        script += 'ConstellationMgr.setHullsThickness(' + hullsThickness + ');\n\n';
        
        script += '// Set art intensity and fade durations\n';
        script += 'ConstellationMgr.setArtIntensity(' + artIntensity + ');\n';
        script += 'ConstellationMgr.setArtFadeDuration(' + artFade + ');\n';
        script += 'ConstellationMgr.setLinesFadeDuration(' + linesFade + ');\n';
        script += 'ConstellationMgr.setLabelsFadeDuration(' + labelsFade + ');\n';
        script += 'ConstellationMgr.setBoundariesFadeDuration(' + boundariesFade + ');\n';
        script += 'ConstellationMgr.setHullsFadeDuration(' + hullsFade + ');\n\n';
        
        script += '// Set display states\n';
        script += 'ConstellationMgr.setFlagArt(' + (showArt ? 'true' : 'false') + ');\n';
        script += 'ConstellationMgr.setFlagBoundaries(' + (showBoundaries ? 'true' : 'false') + ');\n';
        script += 'ConstellationMgr.setFlagHulls(' + (showHulls ? 'true' : 'false') + ');\n';
        script += 'ConstellationMgr.setFlagLines(' + (showLines ? 'true' : 'false') + ');\n';
        script += 'ConstellationMgr.setFlagLabels(true);\n';
        script += 'ConstellationMgr.setFlagIsolateSelected(' + (isolate ? 'true' : 'false') + ');\n\n';
        script += 'ConstellationMgr.setFlagConstellationPick(' + (pickLastOnly ? 'true' : 'false') + ');\n\n';				
        
        // Welcome message
        script += '// ============================================================\n';
        script += '// WELCOME MESSAGE\n';
        script += '// ============================================================\n';
        script += 'var msg = tr("Welcome to the ");\n';
        script += 'msg += "' + cultureName + '";\n';
        script += 'msg += tr(" constellations tour!\\n\\nExploring celestial patterns across the sky.\\n\\nPress Ctrl+T to show toolbar at any time.");\n';
        script += 'var labelId = LabelMgr.labelScreen(msg, 100, 100, false, 16, "#ffaa66");\n';
        script += 'LabelMgr.setLabelShow(labelId, true);\n';
        script += 'core.wait(6);\n';
        script += 'LabelMgr.setLabelShow(labelId, false);\n';
        script += 'LabelMgr.deleteLabel(labelId);\n\n';
        
        // Constellation array
        script += '// ============================================================\n';
        script += '// CONSTELLATION LIST (Static Array)\n';
        script += '// ============================================================\n';
        script += '// This array contains ' + totalConstellations + ' constellation names\n';
        script += '// extracted from the ' + cultureName + ' sky culture data.\n';
        script += 'var constellations = [\n';
        
        for (var i = 0; i < constellations.length; i++) {
            var con = constellations[i];
            var name = con.searchName || con.name;
            var escapedName = name.replace(/"/g, '\\"');
            
            var commentText = '';
            if (con.englishName && con.englishName !== name) {
                commentText = con.englishName;
            } else if (con.nativeName && con.nativeName !== name) {
                commentText = con.nativeName;
            }
            
            var line = '    "' + escapedName + '"';
            if (i < constellations.length - 1) {
                line += ',';
            }
            if (commentText) {
                line += ' // ' + commentText;
            }
            script += line + '\n';
        }
        
        script += '];\n\n';
        
        // Tour configuration
        script += '// ============================================================\n';
        script += '// TOUR CONFIGURATION\n';
        script += '// ============================================================\n';
        script += 'var DURATION = ' + duration + ';  // Seconds per constellation\n';
        script += 'var ZOOM_FOV = ' + zoomFov + ';  // Field of view when zoomed in (degrees)\n';
        script += 'var OVERVIEW_FOV = ' + overviewFov + ';  // Field of view for overview\n\n';
        
        // Tour execution
        // ============================================================
        // TOUR EXECUTION - Use core.selectConstellationByName
        // ============================================================
        script += '// ============================================================\n';
        script += '// TOUR EXECUTION\n';
        script += '// ============================================================\n';
        script += 'core.debug(tr("Starting constellations tour..."));\n';
        script += 'core.debug(tr("Total constellations in array: ") + constellations.length);\n\n';
        
        script += 'for (var i = 0; i < constellations.length; i++) {\n';
        script += '    var constName = constellations[i];\n';
        script += '    var currentNum = i + 1;\n';
        script += '    \n';
        script += '    core.debug(tr("Visiting constellation") + " " + currentNum + "/" + constellations.length + ": " + constName);\n';
        script += '    \n';
        script += '    // Select constellation (does not auto-enable isolation)\n';
        script += '    core.selectConstellationByName(constName);\n';
        script += '    \n';
        script += '    // Move view to center on the constellation\n';
        script += '    core.moveToObject(constName, 2);\n';
        script += '    core.wait(1);\n';
        script += '    \n';
        script += '    // Zoom in for detailed view\n';
        script += '    StelMovementMgr.zoomTo(ZOOM_FOV, 2);\n';
        script += '    core.wait(1);\n';
        script += '    \n';
        script += '    // Display constellation name on screen\n';
        script += '    var nameLabel = LabelMgr.labelScreen(constName, 50, 50, false, ' + fontSize + ', "#ffaa66");\n';
        script += '    LabelMgr.setLabelShow(nameLabel, true);\n';
        script += '    core.wait(DURATION);\n';
        script += '    LabelMgr.deleteLabel(nameLabel);\n';
        script += '    \n';
        script += '    // Zoom out for overview before next constellation\n';
        script += '    StelMovementMgr.zoomTo(OVERVIEW_FOV, 2);\n';
        script += '    core.wait(1);\n';
        script += '}\n\n';
        
        // Reset and cleanup
        script += '// ============================================================\n';
        script += '// RESET AND CLEANUP\n';
        script += '// ============================================================\n';
        script += 'core.debug(tr("Tour completed! Restoring original settings..."));\n\n';
        script += 'ConstellationMgr.deselectConstellations();\n';
        script += 'ConstellationMgr.setFlagArt(oldArtState);\n';
        script += 'ConstellationMgr.setFlagBoundaries(oldBoundariesState);\n';
        script += 'ConstellationMgr.setFlagHulls(oldHullsState);\n';
        script += 'ConstellationMgr.setFlagLines(oldLinesState);\n';
        script += 'ConstellationMgr.setFlagLabels(oldLabelsState);\n';
        script += 'ConstellationMgr.setFlagIsolateSelected(oldIsolateState);\n';
        script += 'ConstellationMgr.setFlagConstellationPick(oldPickLastState);\n\n';				
        script += 'ConstellationMgr.setFontSize(oldFontSize);\n';
        script += 'ConstellationMgr.setConstellationLineThickness(oldLineThickness);\n';
        script += 'ConstellationMgr.setBoundariesThickness(oldBoundariesThickness);\n';
        script += 'ConstellationMgr.setHullsThickness(oldHullsThickness);\n';
        script += 'ConstellationMgr.setArtIntensity(oldArtIntensity);\n';
        script += 'ConstellationMgr.setLinesColor(oldLinesColor);\n';
        script += 'ConstellationMgr.setLabelsColor(oldLabelsColor);\n';
        script += 'ConstellationMgr.setBoundariesColor(oldBoundariesColor);\n';
        script += 'ConstellationMgr.setHullsColor(oldHullsColor);\n';
        script += 'ConstellationMgr.setLinesFadeDuration(oldLinesFade);\n';
        script += 'ConstellationMgr.setLabelsFadeDuration(oldLabelsFade);\n';
        script += 'ConstellationMgr.setBoundariesFadeDuration(oldBoundariesFade);\n';
        script += 'ConstellationMgr.setHullsFadeDuration(oldHullsFade);\n';
        script += 'ConstellationMgr.setArtFadeDuration(oldArtFade);\n\n';
        
        script += 'GridLinesMgr.setFlagGridlines(true);\n\n';
        script += 'core.setGuiVisible(true);\n\n';
        script += 'core.clear("natural");\n\n';
        script += 'StelMovementMgr.zoomTo(100, 3);\n';
        script += 'core.moveToAltAzi(40.0, 180.0, 3.0);\n\n';
        script += 'core.debug(tr("Constellations tour completed successfully!"));\n';
        
        // Check script size
        var scriptSize = new Blob([script]).size;
        var MAX_SAFE_SIZE = 50 * 1024;
        
        if (scriptSize > MAX_SAFE_SIZE && isLargeCulture) {
            var blob = new Blob([script], { type: 'text/plain;charset=utf-8' });
            var url = URL.createObjectURL(blob);
            var a = document.createElement('a');
            a.href = url;
            a.download = cultureId + '_constellations_tour.ssc';
            document.body.appendChild(a);
            a.click();
            document.body.removeChild(a);
            setTimeout(function() { URL.revokeObjectURL(url); }, 100);
            showTourNotification(_tr("Large constellations script saved as file: ") + cultureId + '_constellations_tour.ssc');
        } else if (window._stelScriptEditor && typeof window._stelScriptEditor.setEditorContent === 'function') {
            window._stelScriptEditor.setEditorContent(script);
            switchToScriptEditor();
            showTourNotification(_tr("Static constellations tour script generated and inserted into editor"));
        } else {
            copyToClipboard(script);
            showTourNotification(_tr("Script copied to clipboard"));
        }
    }

    /**
     * Switch to the Script Editor tab in the Actions panel.
     * This provides a seamless experience for users after generating scripts.
     */
    function switchToScriptEditor() {
        // Step 1: Switch to the main Actions tab (#tab_actions)
        var $mainTabs = $('#tabs');
        if ($mainTabs.length && typeof $mainTabs.tabs === 'function') {
            var actionsTabIndex = -1;
            $mainTabs.find('ul li a').each(function(idx, el) {
                if ($(el).attr('href') === '#tab_actions') {
                    actionsTabIndex = idx;
                    return false;
                }
            });
            if (actionsTabIndex !== -1) {
                $mainTabs.tabs('option', 'active', actionsTabIndex);
            }
        }
        
        // Step 2: Switch to the Script Editor sub-tab (first tab in actions-sub-tabs)
        var $subTabs = $('#actions-sub-tabs');
        if ($subTabs.length && typeof $subTabs.tabs === 'function') {
            $subTabs.tabs('option', 'active', 0);  // Script Editor is the first tab
        }
    }

		/**
		 * Create the tour control toolbar for constellations inside the constellations tab.
		 * Provides controls for generating dynamic/static scripts with full customization.
		 * Section headers and borders change color dynamically based on RGB picker selections.
		 * Optimized layout: Multi-column grid for better space utilization.
		 * 
		 * @function createConstellationsTourToolbar
		 */
		function createConstellationsTourToolbar() {
				console.log("[SkyCulture] createConstellationsTourToolbar() called");

				// Check if toolbar already exists
				if ($('#const-tour-controls-container').length > 0) {
						console.log("[SkyCulture] Constellations tour toolbar already exists, skipping");
						return;
				}

				// Find the constellations tab content
				var $constellationsTab = $('#patterns-tab-constellations');
				if (!$constellationsTab.length) {
						console.warn("[SkyCulture] Constellations tab not found");
						return;
				}

				// Find the container where buttons are displayed
				var $buttonsContainer = $constellationsTab.find('#constellations-buttons-container');
				if (!$buttonsContainer.length) {
						console.warn("[SkyCulture] Constellations buttons container not found");
						return;
				}

				// Create toolbar container - NO inline styles
				var $toolbarContainer = $('<div id="const-tour-controls-container" class="tour-controls-container"></div>');

				// Add section title - NO inline styles
				var $sectionTitle = $('<div class="tour-section-title">' + _tr("Constellations Tour Generator") + '</div>');

				// Create inner toolbar with multi-column grid layout - NO inline styles
				var $toolbar = $('<div class="tour-toolbar-grid"></div>');

				// ============================================================
				// HELPER FUNCTION: Update section header color
				// ============================================================
				function updateSectionColor(sectionId, r, g, b) {
						var $section = $('#' + sectionId);
						if ($section.length) {
								var hexColor = 'rgb(' + Math.floor(r * 255) + ',' + Math.floor(g * 255) + ',' + Math.floor(b * 255) + ')';
								$section.css('color', hexColor);
								$section.css('border-left-color', hexColor);
						}
				}

				// ============================================================
				// HELPER FUNCTION: Create color group with live section header update
				// ============================================================
				function createColorGroup(label, idPrefix, sectionId, defaultR, defaultG, defaultB) {
						var $group = $('<div class="tour-color-group"></div>');
						$group.append('<span class="tour-color-group-label">' + label + '</span>');

						var $sliders = $('<div class="tour-color-sliders"></div>');

						$sliders.append('<span class="tour-color-channel tour-color-channel-r">R</span>');
						var $rSlider = $('<input type="range" id="' + idPrefix + '-r" class="tour-color-slider" min="0" max="100" value="' + (defaultR * 100) + '" step="1">');
						$sliders.append($rSlider);

						$sliders.append('<span class="tour-color-channel tour-color-channel-g">G</span>');
						var $gSlider = $('<input type="range" id="' + idPrefix + '-g" class="tour-color-slider" min="0" max="100" value="' + (defaultG * 100) + '" step="1">');
						$sliders.append($gSlider);

						$sliders.append('<span class="tour-color-channel tour-color-channel-b">B</span>');
						var $bSlider = $('<input type="range" id="' + idPrefix + '-b" class="tour-color-slider" min="0" max="100" value="' + (defaultB * 100) + '" step="1">');
						$sliders.append($bSlider);

						var $preview = $('<div id="' + idPrefix + '-preview" class="tour-color-preview"></div>');
						$sliders.append($preview);

						var $valueDisplay = $('<span id="' + idPrefix + '-value" class="tour-color-value">' + defaultR.toFixed(2) + ', ' + defaultG.toFixed(2) + ', ' + defaultB.toFixed(2) + '</span>');
						$sliders.append($valueDisplay);

						$group.append($sliders);

						// Initialize preview with default color
						var initR = defaultR;
						var initG = defaultG;
						var initB = defaultB;
						var previewR = Math.floor(initR * 255);
						var previewG = Math.floor(initG * 255);
						var previewB = Math.floor(initB * 255);
						$preview.css('background', 'rgb(' + previewR + ',' + previewG + ',' + previewB + ')');

						function updateFromSliders() {
								var r = parseFloat($rSlider.val()) / 100;
								var g = parseFloat($gSlider.val()) / 100;
								var b = parseFloat($bSlider.val()) / 100;
								var previewR = Math.floor(r * 255);
								var previewG = Math.floor(g * 255);
								var previewB = Math.floor(b * 255);
								$preview.css('background', 'rgb(' + previewR + ',' + previewG + ',' + previewB + ')');
								$valueDisplay.text(r.toFixed(2) + ', ' + g.toFixed(2) + ', ' + b.toFixed(2));
								updateSectionColor(sectionId, r, g, b);
						}

						$rSlider.on('input', updateFromSliders);
						$gSlider.on('input', updateFromSliders);
						$bSlider.on('input', updateFromSliders);

						return $group;
				}

				// ============================================================
				// COLUMN 1: Script Generation + Timing Controls
				// ============================================================
				var $column1 = $('<div class="tour-column"></div>');

				// Script Generation Buttons
				var $genRow = $('<div class="tour-row-separator"></div>');
				$constTourGenerateDynamicBtn = $('<button id="btn-generate-const-dynamic" class="jquerybutton pattern-btn tour-generate-btn" title="' + _tr("Generates script using ConstellationMgr.getConstellationsEnglishNames() - works with any culture") + '">' + _tr("Generate Dynamic Constellation Tour Script") + '</button>');
				$genRow.append($constTourGenerateDynamicBtn);

				$constTourGenerateStaticBtn = $('<button id="btn-generate-const-static" class="jquerybutton pattern-btn tour-generate-btn" title="' + _tr("Generates script with embedded constellation name array - portable and self-contained") + '">' + _tr("Generate Static Array Constellation Tour Script") + '</button>');
				$genRow.append($constTourGenerateStaticBtn);

				// Multi-Select Mode checkbox and counter
				var $multiSelectWrapper = $('<div class="tour-multiselect-gold"></div>');
				$constMultiSelectMode = $('<input type="checkbox" id="const-multi-select-mode">');
				$multiSelectWrapper.append($constMultiSelectMode);
				$multiSelectWrapper.append('<label style="font-weight:bold;cursor:pointer;">' + _tr("Multi-Select") + '</label>');
				$multiSelectWrapper.append('<span class="tour-multiselect-hint">(' + _tr("Pick multiple") + ')</span>');

				$constSelectionCount = $('<span class="tour-selection-count">0 ' + _tr("selected") + '</span>');
				$multiSelectWrapper.append($constSelectionCount);

				$genRow.append($multiSelectWrapper);

				$column1.append($genRow);

				// Timing Controls Row
				var $timingRow = $('<div class="tour-row"></div>');

				var $durationWrapper = $('<div class="tour-control-group"></div>');
				$durationWrapper.append('<label class="tour-control-label-bold">' + _tr("Duration Per Const:") + '</label>');
				$constTourDuration = $('<input type="number" id="const-tour-duration" class="tour-input-number" value="4" min="1" max="15" step="0.5">');
				$durationWrapper.append($constTourDuration);
				$durationWrapper.append('<span>' + _tr("sec") + '</span>');
				$timingRow.append($durationWrapper);

				var $zoomWrapper = $('<div class="tour-control-group"></div>');
				$zoomWrapper.append('<label class="tour-control-label-bold">' + _tr("Zoom FOV:") + '</label>');
				$constTourZoomFov = $('<input type="number" id="const-zoom-fov" class="tour-input-number" value="40" min="5" max="90" step="1">');
				$zoomWrapper.append($constTourZoomFov);
				$zoomWrapper.append('<span>' + _tr("deg") + '</span>');
				$timingRow.append($zoomWrapper);

				var $overviewWrapper = $('<div class="tour-control-group"></div>');
				$overviewWrapper.append('<label class="tour-control-label-bold">' + _tr("Overview FOV:") + '</label>');
				$constTourOverviewFov = $('<input type="number" id="const-overview-fov" class="tour-input-number" value="90" min="20" max="120" step="1">');
				$overviewWrapper.append($constTourOverviewFov);
				$overviewWrapper.append('<span>' + _tr("deg") + '</span>');
				$timingRow.append($overviewWrapper);

				$column1.append($timingRow);

				// Selection Mode Controls (Isolate + Pick Last Only)
				var $selectionRow = $('<div class="tour-row-subtle"></div>');

				var $isolateWrapper = $('<div class="tour-control-group"></div>');
				$constTourIsolate = $('<input type="checkbox" id="const-tour-isolate" checked>');
				$isolateWrapper.append($constTourIsolate);
				$isolateWrapper.append('<label class="tour-control-label">' + _tr("Isolate Selected") + '</label>');
				$selectionRow.append($isolateWrapper);

				var $pickLastWrapper = $('<div class="tour-control-group"></div>');
				$constTourPickLastOnly = $('<input type="checkbox" id="const-tour-pick-last">');
				$pickLastWrapper.append($constTourPickLastOnly);
				$pickLastWrapper.append('<label class="tour-control-label">' + _tr("Select Last Only") + '</label>');
				$selectionRow.append($pickLastWrapper);

				$column1.append($selectionRow);

				// ============================================================
				// COLUMN 2: Lines + Labels + Artwork
				// ============================================================
				var $column2 = $('<div class="tour-column"></div>');

				// CONSTELLATION LINES Section
				var $rowLines = $('<div id="const-lines-section" class="tour-section-lines"></div>');
				$rowLines.append('<div id="const-lines-header" class="tour-section-header">' + _tr("LINES") + '</div>');

				var $linesSubRow = $('<div class="tour-row"></div>');

				var $showLinesWrapper = $('<div class="tour-control-group"></div>');
				$constTourShowLines = $('<input type="checkbox" id="const-show-lines" checked>');
				$showLinesWrapper.append($constTourShowLines);
				$showLinesWrapper.append('<label class="tour-control-label">' + _tr("Show") + '</label>');
				$linesSubRow.append($showLinesWrapper);

				var $lineThicknessWrapper = $('<div class="tour-control-group"></div>');
				$lineThicknessWrapper.append('<label class="tour-control-label">' + _tr("Thick:") + '</label>');
				$constTourLineThickness = $('<input type="number" id="const-line-thickness" class="tour-input-number" value="2" min="1" max="5" step="1">');
				$lineThicknessWrapper.append($constTourLineThickness);
				$lineThicknessWrapper.append('<span>' + _tr("px") + '</span>');
				$linesSubRow.append($lineThicknessWrapper);

				var $linesFadeWrapper = $('<div class="tour-control-group"></div>');
				$linesFadeWrapper.append('<label class="tour-control-label">' + _tr("Fade:") + '</label>');
				$constTourLinesFade = $('<input type="number" id="const-lines-fade" class="tour-input-number" value="1.0" min="0" max="5" step="0.1">');
				$linesFadeWrapper.append($constTourLinesFade);
				$linesFadeWrapper.append('<span>' + _tr("sec") + '</span>');
				$linesSubRow.append($linesFadeWrapper);

				$rowLines.append($linesSubRow);
				var $linesColorGroup = createColorGroup(_tr("Lines Color"), "const-lines-color", "const-lines-section", 0.30, 0.60, 0.90);
				$rowLines.append($linesColorGroup);
				$column2.append($rowLines);

				// CONSTELLATION LABELS Section
				var $rowLabels = $('<div id="const-labels-section" class="tour-section-labels"></div>');
				$rowLabels.append('<div id="const-labels-header" class="tour-section-header">' + _tr("LABELS") + '</div>');

				var $labelsSubRow = $('<div class="tour-row"></div>');

				var $fontWrapper = $('<div class="tour-control-group"></div>');
				$fontWrapper.append('<label class="tour-control-label">' + _tr("Size:") + '</label>');
				$constTourFontSize = $('<input type="number" id="const-font-size" class="tour-input-number" value="16" min="10" max="30" step="1">');
				$fontWrapper.append($constTourFontSize);
				$fontWrapper.append('<span>' + _tr("px") + '</span>');
				$labelsSubRow.append($fontWrapper);

				var $labelsFadeWrapper2 = $('<div class="tour-control-group"></div>');
				$labelsFadeWrapper2.append('<label class="tour-control-label">' + _tr("Fade:") + '</label>');
				$constTourLabelsFade = $('<input type="number" id="const-labels-fade" class="tour-input-number" value="1.0" min="0" max="5" step="0.1">');
				$labelsFadeWrapper2.append($constTourLabelsFade);
				$labelsFadeWrapper2.append('<span>' + _tr("sec") + '</span>');
				$labelsSubRow.append($labelsFadeWrapper2);

				$rowLabels.append($labelsSubRow);
				var $labelsColorGroup = createColorGroup(_tr("Labels Color"), "const-labels-color", "const-labels-section", 0.00, 0.50, 0.00);
				$rowLabels.append($labelsColorGroup);
				$column2.append($rowLabels);

				// CONSTELLATION ARTWORK Section
				var $rowArt = $('<div id="const-art-section" class="tour-section-artwork"></div>');
				$rowArt.append('<div id="const-art-header" class="tour-section-header">' + _tr("ARTWORK") + '</div>');

				var $artSubRow = $('<div class="tour-row"></div>');

				var $artWrapper = $('<div class="tour-control-group"></div>');
				$constTourShowArt = $('<input type="checkbox" id="const-show-art" checked>');
				$artWrapper.append($constTourShowArt);
				$artWrapper.append('<label class="tour-control-label">' + _tr("Show") + '</label>');
				$artSubRow.append($artWrapper);

				var $artIntensityWrapper = $('<div class="tour-control-group"></div>');
				$artIntensityWrapper.append('<label class="tour-control-label">' + _tr("Intensity:") + '</label>');
				$constTourArtIntensity = $('<input type="range" id="const-art-intensity" class="tour-input-range" min="0" max="100" value="80" step="1">');
				$artIntensityWrapper.append($constTourArtIntensity);
				var $artIntensityValue = $('<span id="const-art-intensity-value" class="tour-range-value">0.80</span>');
				$artIntensityWrapper.append($artIntensityValue);
				$artSubRow.append($artIntensityWrapper);

				var $artFadeWrapper = $('<div class="tour-control-group"></div>');
				$artFadeWrapper.append('<label class="tour-control-label">' + _tr("Fade:") + '</label>');
				$constTourArtFade = $('<input type="number" id="const-art-fade" class="tour-input-number" value="2.0" min="0" max="5" step="0.1">');
				$artFadeWrapper.append($constTourArtFade);
				$artFadeWrapper.append('<span>' + _tr("sec") + '</span>');
				$artSubRow.append($artFadeWrapper);

				$rowArt.append($artSubRow);
				$column1.append($rowArt);

				// Update art intensity value display
				$constTourArtIntensity.on('input', function() {
						var val = (parseFloat($(this).val()) / 100).toFixed(2);
						$artIntensityValue.text(val);
				});

				// ============================================================
				// COLUMN 3: Boundaries + Hulls
				// ============================================================
				var $column3 = $('<div class="tour-column"></div>');

				// CONSTELLATION BOUNDARIES Section
				var $rowBoundaries = $('<div id="const-boundaries-section" class="tour-section-boundaries"></div>');
				$rowBoundaries.append('<div id="const-boundaries-header" class="tour-section-header">' + _tr("BOUNDARIES") + '</div>');

				var $boundariesSubRow = $('<div class="tour-row"></div>');

				var $boundariesWrapper = $('<div class="tour-control-group"></div>');
				$constTourShowBoundaries = $('<input type="checkbox" id="const-show-boundaries">');
				$boundariesWrapper.append($constTourShowBoundaries);
				$boundariesWrapper.append('<label class="tour-control-label">' + _tr("Show") + '</label>');
				$boundariesSubRow.append($boundariesWrapper);

				var $boundariesThicknessWrapper = $('<div class="tour-control-group"></div>');
				$boundariesThicknessWrapper.append('<label class="tour-control-label">' + _tr("Thick:") + '</label>');
				$constTourBoundariesThickness = $('<input type="number" id="const-boundaries-thickness" class="tour-input-number" value="1" min="1" max="5" step="1">');
				$boundariesThicknessWrapper.append($constTourBoundariesThickness);
				$boundariesThicknessWrapper.append('<span>' + _tr("px") + '</span>');
				$boundariesSubRow.append($boundariesThicknessWrapper);

				var $boundariesFadeWrapper2 = $('<div class="tour-control-group"></div>');
				$boundariesFadeWrapper2.append('<label class="tour-control-label">' + _tr("Fade:") + '</label>');
				$constTourBoundariesFade = $('<input type="number" id="const-boundaries-fade" class="tour-input-number" value="1.0" min="0" max="5" step="0.1">');
				$boundariesFadeWrapper2.append($constTourBoundariesFade);
				$boundariesFadeWrapper2.append('<span>' + _tr("sec") + '</span>');
				$boundariesSubRow.append($boundariesFadeWrapper2);

				$rowBoundaries.append($boundariesSubRow);
				var $boundariesColorGroup = createColorGroup(_tr("Boundaries Color"), "const-boundaries-color", "const-boundaries-section", 0.80, 0.30, 0.30);
				$rowBoundaries.append($boundariesColorGroup);
				$column3.append($rowBoundaries);

				// CONSTELLATION HULLS Section
				var $rowHulls = $('<div id="const-hulls-section" class="tour-section-hulls"></div>');
				$rowHulls.append('<div id="const-hulls-header" class="tour-section-header">' + _tr("HULLS (Areas)") + '</div>');

				var $hullsSubRow = $('<div class="tour-row"></div>');

				var $hullsWrapper = $('<div class="tour-control-group"></div>');
				$constTourShowHulls = $('<input type="checkbox" id="const-show-hulls">');
				$hullsWrapper.append($constTourShowHulls);
				$hullsWrapper.append('<label class="tour-control-label">' + _tr("Show") + '</label>');
				$hullsSubRow.append($hullsWrapper);

				var $hullsThicknessWrapper = $('<div class="tour-control-group"></div>');
				$hullsThicknessWrapper.append('<label class="tour-control-label">' + _tr("Thick:") + '</label>');
				$constTourHullsThickness = $('<input type="number" id="const-hulls-thickness" class="tour-input-number" value="1" min="1" max="5" step="1">');
				$hullsThicknessWrapper.append($constTourHullsThickness);
				$hullsThicknessWrapper.append('<span>' + _tr("px") + '</span>');
				$hullsSubRow.append($hullsThicknessWrapper);

				var $hullsFadeWrapper2 = $('<div class="tour-control-group"></div>');
				$hullsFadeWrapper2.append('<label class="tour-control-label">' + _tr("Fade:") + '</label>');
				$constTourHullsFade = $('<input type="number" id="const-hulls-fade" class="tour-input-number" value="1.0" min="0" max="5" step="0.1">');
				$hullsFadeWrapper2.append($constTourHullsFade);
				$hullsFadeWrapper2.append('<span>' + _tr("sec") + '</span>');
				$hullsSubRow.append($hullsFadeWrapper2);

				$rowHulls.append($hullsSubRow);
				var $hullsColorGroup = createColorGroup(_tr("Hulls Color"), "const-hulls-color", "const-hulls-section", 1.00, 0.70, 0.40);
				$rowHulls.append($hullsColorGroup);
				$column3.append($rowHulls);

				// ============================================================
				// ASSEMBLE ALL COLUMNS INTO TOOLBAR
				// ============================================================
				$toolbar.append($column1);
				$toolbar.append($column2);
				$toolbar.append($column3);

				$toolbarContainer.append($sectionTitle);
				$toolbarContainer.append($toolbar);

				// Insert toolbar AFTER the buttons container (at the bottom of the tab)
				$buttonsContainer.after($toolbarContainer);

				// Bind events
				$constTourGenerateDynamicBtn.on('click', generateConstellationsTourScriptDynamic);
				$constTourGenerateStaticBtn.on('click', generateConstellationsTourScriptStatic);

				// Bind multi-select mode change event
				if ($constMultiSelectMode) {
						$constMultiSelectMode.on('change', function() {
								isMultiSelectModeActive = $(this).is(':checked');

								if (isMultiSelectModeActive) {
										console.log("[SkyCulture] Entering multi-select mode");
										if (stelUtils && typeof stelUtils.clearConstellationHighlight === 'function') {
												stelUtils.clearConstellationHighlight();
										}
										selectedPattern = null;
										selectedPatternId = null;
										clearMultiSelection();
										updateConstellationButtonsBehavior();
								} else {
										console.log("[SkyCulture] Exiting multi-select mode");
										clearMultiSelection();
										selectedConstellationIds.clear();
										updateConstellationButtonsBehavior();
										updateSelectionCount();
								}
						});
				}

				console.log("[SkyCulture] Constellations tour toolbar created successfully with optimized multi-column layout");
		}
	
    /**
     * Copy text to clipboard using the shared utility from scripteditor.
     * Falls back to local implementation if scripteditor is not available.
     * 
     * @param {string} text - The text to copy to clipboard
     */
    function copyToClipboard(text) {
        // Try to use the shared copy function from scripteditor
        if (window._stelScriptEditor && typeof window._stelScriptEditor.copyToClipboard === 'function') {
            window._stelScriptEditor.copyToClipboard(text);
            console.log("[SkyCulture] Copied to clipboard via scripteditor");
            return;
        }
        
        // Fallback: create temporary textarea
        console.log("[SkyCulture] Using fallback clipboard method");
        var textarea = document.createElement('textarea');
        textarea.value = text;
        // Make sure textarea is off-screen
        textarea.style.position = 'fixed';
        textarea.style.left = '-9999px';
        textarea.style.top = '-9999px';
        textarea.style.opacity = '0';
        document.body.appendChild(textarea);
        textarea.select();
        textarea.setSelectionRange(0, text.length);
        try {
            var success = document.execCommand('copy');
            if (!success) {
                console.error("[SkyCulture] execCommand failed");
            }
        } catch(e) {
            console.error("[SkyCulture] Copy failed:", e);
        }
        document.body.removeChild(textarea);
    }

		// ========================================================================
		// ASTERISMS MULTI-SELECT FUNCTIONS
		// ========================================================================

		/**
		 * Update the selection count display for asterisms in multi-select mode.
		 * Updates the counter badge and enables/disables the generate button
		 * based on whether any asterisms are selected.
		 */
		function updateAsterismSelectionCount() {
				var count = selectedAsterismIds.size;
				if ($asterSelectionCount && $asterSelectionCount.length) {
						$asterSelectionCount.text(count + ' ' + (count === 1 ? _tr("selected") : _tr("selected")));
				}
				
				// Update generate button state (disable if nothing selected)
				if ($asterTourGenerateStaticBtn && $asterTourGenerateStaticBtn.length) {
						$asterTourGenerateStaticBtn.prop('disabled', count === 0 && isAsterMultiSelectModeActive);
				}
		}

		/**
		 * Clear all selections in multi-select mode for asterisms.
		 * Removes all selected IDs and resets button styles.
		 */
		function clearAsterMultiSelection() {
				selectedAsterismIds.clear();
				if ($asterismsContainer && $asterismsContainer.length) {
						// Clear styles without re-rendering
						$asterismsContainer.find('.pattern-btn').removeClass('selected-asterism');
						$asterismsContainer.find('.pattern-btn').css('background', '');
						$asterismsContainer.find('.pattern-btn').css('border-color', '');
						$asterismsContainer.find('.pattern-btn').css('color', '');
						$asterismsContainer.find('.pattern-btn').removeClass('active');
				}
				updateAsterismSelectionCount();
		}

		/**
		 * Get selected asterism objects for multi-select mode.
		 * 
		 * @returns {Array|null} Array of selected asterism objects or null if none selected
		 */
		function getSelectedAsterisms() {
				if (!isAsterMultiSelectModeActive || selectedAsterismIds.size === 0) {
						return null;
				}
				
				var selected = [];
				var allAsterisms = currentPatternsData.asterisms;
				
				if (!allAsterisms) return null;
				
				for (var i = 0; i < allAsterisms.length; i++) {
						var ast = allAsterisms[i];
						if (selectedAsterismIds.has(ast.id)) {
								selected.push(ast);
						}
				}
				
				return selected;
		}

		/**
		 * Update asterism button behavior based on multi-select mode.
		 * 
		 * When multi-select mode is ON:
		 * - Re-renders buttons without the original click handler
		 * - Attaches custom click handler that toggles selection state
		 * - Selected buttons get special styling
		 * 
		 * When multi-select mode is OFF:
		 * - Re-renders buttons with original navigation handler
		 * - Clears any selection state
		 */
		function updateAsterismButtonsBehavior() {
				if (!$asterismsContainer || !$asterismsContainer.length) return;
				
				if (isAsterMultiSelectModeActive) {
						// Get current patterns data
						var currentPatterns = currentPatternsData.asterisms;
						if (currentPatterns && currentPatterns.length) {
								// Re-render with handler disabled (original handler is the third parameter)
								// We pass null as onClickHandler and true as disableOriginalHandler
								renderButtons($asterismsContainer, currentPatterns, "asterisms", null, true);
								
								// Then attach multi-select behavior
								$asterismsContainer.find('.pattern-btn').off('click.asterism').on('click.asterism', function(e) {
										e.preventDefault();
										e.stopPropagation();
										
										var $btn = $(this);
										var patternId = $btn.data('pattern-id');
										var patternName = $btn.data('pattern-name');
										
										if (!patternId) return false;
										
										if (selectedAsterismIds.has(patternId)) {
												// Deselect: remove from set and clear styling
												selectedAsterismIds.delete(patternId);
												$btn.removeClass('selected-asterism');
												$btn.css('background', '');
												$btn.css('border-color', '');
												$btn.css('color', '');
												$btn.removeClass('active');
												console.log("[SkyCulture] Asterism deselected:", patternName, "ID:", patternId);
										} else {
												// Select: add to set and apply selected styling
												selectedAsterismIds.add(patternId);
												$btn.addClass('selected-asterism');
												$btn.css('background', 'linear-gradient(#DCDBDA, #8A8C8E)');
												$btn.css('border-color', '#000000');
												$btn.css('color', '#000000');
												console.log("[SkyCulture] Asterism selected:", patternName, "ID:", patternId);
										}
										
										updateAsterismSelectionCount();
										return false;
								});
						}
				} else {
						// Normal mode: re-render with original handler
						var currentPatterns = currentPatternsData.asterisms;
						if (currentPatterns && currentPatterns.length) {
								renderButtons($asterismsContainer, currentPatterns, "asterisms", 
										function(patternName, patternType, searchName, patternId) {
												stelUtils.goToAsterism(searchName || patternName, patternId);
												updateAllButtonStates();
										},
										false  // Handler enabled
								);
						}
						
						// Clear any selected asterism highlight from previous multi-select
						clearAsterMultiSelection();
				}
		}
		
		/**
		 * Create the tour control toolbar for asterisms inside the asterisms tab.
		 * Provides buttons for generating dynamic/static scripts and (optionally) starting tours.
		 * 
		 * @function createAsterismsTourToolbar
		 */
		function createAsterismsTourToolbar() {
				console.log("[SkyCulture] createAsterismsTourToolbar() called");

				// Check if toolbar already exists
				if ($('#aster-tour-controls-container').length > 0) {
						console.log("[SkyCulture] Asterisms tour toolbar already exists, skipping");
						return;
				}

				// Find the asterisms tab content
				var $asterismsTab = $('#patterns-tab-asterisms');
				if (!$asterismsTab.length) {
						console.warn("[SkyCulture] Asterisms tab not found");
						return;
				}

				// Find the container where buttons are displayed
				var $buttonsContainer = $asterismsTab.find('#asterisms-buttons-container');
				if (!$buttonsContainer.length) {
						console.warn("[SkyCulture] Asterisms buttons container not found");
						return;
				}

				// Create toolbar container - NO inline styles
				var $toolbarContainer = $('<div id="aster-tour-controls-container" class="tour-controls-container"></div>');

				// Add section title - NO inline styles
				var $sectionTitle = $('<div class="tour-section-title">' + _tr("Asterisms Tour Generator") + '</div>');

				// Create inner toolbar with flex layout - NO inline styles
				var $toolbar = $('<div class="tour-toolbar-flex"></div>');

				// ============================================================
				// HELPER FUNCTION: Create color group with live preview
				// ============================================================
				function createAsterColorGroup(label, idPrefix, defaultR, defaultG, defaultB) {
						var $group = $('<div class="tour-color-group"></div>');
						$group.append('<span class="tour-color-group-label">' + label + '</span>');

						var $sliders = $('<div class="tour-color-sliders"></div>');

						$sliders.append('<span class="tour-color-channel tour-color-channel-r">R</span>');
						var $rSlider = $('<input type="range" id="' + idPrefix + '-r" class="tour-color-slider" min="0" max="100" value="' + (defaultR * 100) + '" step="1">');
						$sliders.append($rSlider);

						$sliders.append('<span class="tour-color-channel tour-color-channel-g">G</span>');
						var $gSlider = $('<input type="range" id="' + idPrefix + '-g" class="tour-color-slider" min="0" max="100" value="' + (defaultG * 100) + '" step="1">');
						$sliders.append($gSlider);

						$sliders.append('<span class="tour-color-channel tour-color-channel-b">B</span>');
						var $bSlider = $('<input type="range" id="' + idPrefix + '-b" class="tour-color-slider" min="0" max="100" value="' + (defaultB * 100) + '" step="1">');
						$sliders.append($bSlider);

						var $preview = $('<div id="' + idPrefix + '-preview" class="tour-color-preview"></div>');
						$sliders.append($preview);

						var $valueDisplay = $('<span id="' + idPrefix + '-value" class="tour-color-value">' + defaultR.toFixed(2) + ', ' + defaultG.toFixed(2) + ', ' + defaultB.toFixed(2) + '</span>');
						$sliders.append($valueDisplay);

						$group.append($sliders);

						function updateFromSliders() {
								var r = parseFloat($rSlider.val()) / 100;
								var g = parseFloat($gSlider.val()) / 100;
								var b = parseFloat($bSlider.val()) / 100;
								var previewR = Math.floor(r * 255);
								var previewG = Math.floor(g * 255);
								var previewB = Math.floor(b * 255);
								$preview.css('background', 'rgb(' + previewR + ',' + previewG + ',' + previewB + ')');
								$valueDisplay.text(r.toFixed(2) + ', ' + g.toFixed(2) + ', ' + b.toFixed(2));
						}

						$rSlider.on('input', updateFromSliders);
						$gSlider.on('input', updateFromSliders);
						$bSlider.on('input', updateFromSliders);

						return $group;
				}

				// ============================================================
				// FIRST ROW: Main buttons and basic controls
				// ============================================================
				var $firstRow = $('<div class="tour-row"></div>');

				// Generate Static Array Script button
				$asterTourGenerateStaticBtn = $('<button id="btn-generate-aster-static" class="jquerybutton pattern-btn tour-generate-btn" title="' + _tr("Generates script with embedded asterism name array") + '">' + _tr("Generate Asterisms Script") + '</button>');
				$firstRow.append($asterTourGenerateStaticBtn);

				// ============================================================
				// MULTI-SELECT MODE SECTION FOR ASTERISMS
				// ============================================================
				var $multiSelectWrapper = $('<div class="tour-multiselect-purple"></div>');
				$asterMultiSelectMode = $('<input type="checkbox" id="aster-multi-select-mode">');
				$multiSelectWrapper.append($asterMultiSelectMode);
				$multiSelectWrapper.append('<label style="font-weight:bold;cursor:pointer;">' + _tr("Multi-Select") + '</label>');
				$multiSelectWrapper.append('<span class="tour-multiselect-hint">(' + _tr("Pick multiple asterisms") + ')</span>');

				$asterSelectionCount = $('<span class="tour-selection-count">0 ' + _tr("selected") + '</span>');
				$multiSelectWrapper.append($asterSelectionCount);

				$firstRow.append($multiSelectWrapper);

				// Duration label and input
				var $durationWrapper = $('<div class="tour-control-group"></div>');
				$durationWrapper.append('<label class="tour-control-label">' + _tr("Duration:") + '</label>');
				$asterTourDuration = $('<input type="number" id="aster-tour-duration" class="tour-input-number" value="3" min="1" max="10" step="0.5">');
				$durationWrapper.append($asterTourDuration);
				$durationWrapper.append('<span>' + _tr("sec") + '</span>');
				$firstRow.append($durationWrapper);

				// Zoom FOV input
				var $zoomWrapper = $('<div class="tour-control-group"></div>');
				$zoomWrapper.append('<label class="tour-control-label">' + _tr("Zoom FOV:") + '</label>');
				var $zoomFov = $('<input type="number" id="aster-zoom-fov" class="tour-input-number" value="25" min="5" max="60" step="1">');
				$zoomWrapper.append($zoomFov);
				$zoomWrapper.append('<span>' + _tr("deg") + '</span>');
				$firstRow.append($zoomWrapper);

				var $overviewWrapper = $('<div class="tour-control-group"></div>');
				$overviewWrapper.append('<label class="tour-control-label">' + _tr("Overview FOV:") + '</label>');
				var $overviewFov = $('<input type="number" id="aster-overview-fov" class="tour-input-number" value="60" min="25" max="90" step="1">');
				$overviewWrapper.append($overviewFov);
				$overviewWrapper.append('<span>' + _tr("deg") + '</span>');
				$firstRow.append($overviewWrapper);

				// Isolate checkbox
				var $isolateWrapper = $('<div class="tour-control-group"></div>');
				$asterTourIsolate = $('<input type="checkbox" id="aster-tour-isolate" checked>');
				$isolateWrapper.append($asterTourIsolate);
				$isolateWrapper.append('<label class="tour-control-label">' + _tr("Isolate Selected") + '</label>');
				$firstRow.append($isolateWrapper);

				// Show Ray Helpers option
				var $rayHelpersWrapper = $('<div class="tour-control-group"></div>');
				var $showRayHelpers = $('<input type="checkbox" id="aster-show-rayhelpers" checked>');
				$rayHelpersWrapper.append($showRayHelpers);
				$rayHelpersWrapper.append('<label class="tour-control-label">' + _tr("Show Ray Helpers") + '</label>');
				$firstRow.append($rayHelpersWrapper);

				// ============================================================
				// SECOND ROW: Appearance controls (Font, Thickness)
				// ============================================================
				var $secondRow = $('<div class="tour-row"></div>');

				// Font size input
				var $fontWrapper = $('<div class="tour-control-group"></div>');
				$fontWrapper.append('<label class="tour-control-label">' + _tr("Font Size:") + '</label>');
				var $fontSize = $('<input type="number" id="aster-font-size" class="tour-input-number" value="16" min="10" max="30" step="1">');
				$fontWrapper.append($fontSize);
				$fontWrapper.append('<span>' + _tr("px") + '</span>');
				$secondRow.append($fontWrapper);

				// Line thickness input (Asterism Lines)
				var $lineThicknessWrapper = $('<div class="tour-control-group tour-control-group-highlight"></div>');
				$lineThicknessWrapper.append('<label class="tour-control-label-bold">' + _tr("Line Thick:") + '</label>');
				var $lineThickness = $('<input type="number" id="aster-line-thickness" class="tour-input-number" value="2" min="1" max="5" step="1">');
				$lineThicknessWrapper.append($lineThickness);
				$lineThicknessWrapper.append('<span>' + _tr("px") + '</span>');
				$secondRow.append($lineThicknessWrapper);

				// Ray Helper thickness input
				var $rayThicknessWrapper = $('<div class="tour-control-group tour-control-group-highlight"></div>');
				$rayThicknessWrapper.append('<label class="tour-control-label-bold">' + _tr("Ray Thick:") + '</label>');
				var $rayHelperThickness = $('<input type="number" id="aster-rayhelper-thickness" class="tour-input-number" value="1" min="1" max="5" step="1">');
				$rayThicknessWrapper.append($rayHelperThickness);
				$rayThicknessWrapper.append('<span>' + _tr("px") + '</span>');
				$secondRow.append($rayThicknessWrapper);

				// ============================================================
				// THIRD ROW: Color controls
				// ============================================================
				var $colorsContainer = $('<div class="tour-row"></div>');

				var $linesColorGroup = createAsterColorGroup(_tr("Lines Color"), "aster-lines-color", 1.00, 0.00, 0.20);
				var $labelsColorGroup = createAsterColorGroup(_tr("Labels Color"), "aster-labels-color", 0.00, 1.00, 1.00);
				var $rayHelpersColorGroup = createAsterColorGroup(_tr("Ray Helpers Color"), "aster-rayhelpers-color", 0.50, 1.00, 0.50);

				$colorsContainer.append($linesColorGroup);
				$colorsContainer.append($labelsColorGroup);
				$colorsContainer.append($rayHelpersColorGroup);

				// ============================================================
				// FOURTH ROW: Fade duration controls
				// ============================================================
				var $fourthRow = $('<div class="tour-row"></div>');

				var $fadeContainer = $('<div class="tour-control-group-highlight"></div>');
				$fadeContainer.append('<span class="tour-control-label-bold">' + _tr("Fade Duration:") + '</span>');

				var $linesFadeWrapper = $('<div class="tour-control-group"></div>');
				$linesFadeWrapper.append('<label class="tour-control-label">' + _tr("Lines:") + '</label>');
				var $linesFade = $('<input type="number" id="aster-lines-fade" class="tour-input-number" value="1.0" min="0" max="5" step="0.1">');
				$linesFadeWrapper.append($linesFade);
				$linesFadeWrapper.append('<span>' + _tr("sec") + '</span>');
				$fadeContainer.append($linesFadeWrapper);

				var $labelsFadeWrapper = $('<div class="tour-control-group"></div>');
				$labelsFadeWrapper.append('<label class="tour-control-label">' + _tr("Labels:") + '</label>');
				var $labelsFade = $('<input type="number" id="aster-labels-fade" class="tour-input-number" value="1.0" min="0" max="5" step="0.1">');
				$labelsFadeWrapper.append($labelsFade);
				$labelsFadeWrapper.append('<span>' + _tr("sec") + '</span>');
				$fadeContainer.append($labelsFadeWrapper);

				var $rayHelpersFadeWrapper = $('<div class="tour-control-group"></div>');
				$rayHelpersFadeWrapper.append('<label class="tour-control-label">' + _tr("Ray Helpers:") + '</label>');
				var $rayHelpersFade = $('<input type="number" id="aster-rayhelpers-fade" class="tour-input-number" value="1.0" min="0" max="5" step="0.1">');
				$rayHelpersFadeWrapper.append($rayHelpersFade);
				$rayHelpersFadeWrapper.append('<span>' + _tr("sec") + '</span>');
				$fadeContainer.append($rayHelpersFadeWrapper);

				$fourthRow.append($fadeContainer);

				// ============================================================
				// ASSEMBLE ALL ROWS
				// ============================================================
				$toolbar.append($firstRow);
				$toolbar.append($secondRow);
				$toolbar.append($colorsContainer);
				$toolbar.append($fourthRow);

				$toolbarContainer.append($sectionTitle);
				$toolbarContainer.append($toolbar);

				// Insert toolbar AFTER the buttons container (at the bottom of the tab)
				$buttonsContainer.after($toolbarContainer);

				// ============================================================
				// BIND EVENTS
				// ============================================================

				// Bind generate button
				$asterTourGenerateStaticBtn.on('click', generateAsterismsTourScriptStatic);

				// Bind multi-select mode change event
				if ($asterMultiSelectMode) {
						$asterMultiSelectMode.on('change', function() {
								isAsterMultiSelectModeActive = $(this).is(':checked');

								if (isAsterMultiSelectModeActive) {
										console.log("[SkyCulture] Entering multi-select mode for asterisms");
										selectedPattern = null;
										selectedPatternId = null;
										clearAsterMultiSelection();
										updateAsterismButtonsBehavior();
								} else {
										console.log("[SkyCulture] Exiting multi-select mode for asterisms");
										clearAsterMultiSelection();
										selectedAsterismIds.clear();
										updateAsterismButtonsBehavior();
										updateAsterismSelectionCount();
								}
						});
				}

				console.log("[SkyCulture] Asterisms tour toolbar created successfully with multi-select support");
		}
		
		/**
		 * Generate a static SSC script for asterisms tour with embedded array.
		 * 
		 * ENHANCED: Supports multi-select mode - if multi-select is active and
		 * at least one asterism is selected, the script will only include the
		 * selected asterisms. Otherwise, it includes all asterisms.
		 * 
		 * Features:
		 * - Culture verification and automatic switching
		 * - Multi-select support (only selected asterisms when in multi-select mode)
		 * - Includes proper script header with selection note
		 * - Full visual customization
		 * 
		 * @function generateAsterismsTourScriptStatic
		 */
		function generateAsterismsTourScriptStatic() {
				// Determine which asterisms to use (Multi-Select or All)
				var asterismsToUse = null;
				var isMultiSelect = isAsterMultiSelectModeActive && selectedAsterismIds.size > 0;
				
				if (isMultiSelect) {
						asterismsToUse = getSelectedAsterisms();
						if (!asterismsToUse || asterismsToUse.length === 0) {
								showTourNotification(_tr("No asterisms selected. Please select at least one asterism."));
								return;
						}
						showTourNotification(_tr("Generating script for ") + asterismsToUse.length + _tr(" selected asterisms"));
				} else {
						asterismsToUse = currentPatternsData.asterisms;
						if (!asterismsToUse || asterismsToUse.length === 0) {
								showTourNotification(_tr("No asterisms available for this culture"));
								return;
						}
				}
				
				var asterisms = asterismsToUse;
				var totalAsterisms = asterisms.length;
				var cultureName = currentCultureName || currentCultureId;
				var cultureId = currentCultureId;
				
				// Add indicator in header if multi-select mode was used
				var selectionNote = '';
				if (isMultiSelect) {
						selectionNote = '// Selected Asterisms: ' + selectedAsterismIds.size + ' out of ' + (currentPatternsData.asterisms ? currentPatternsData.asterisms.length : 0) + '\n';
						selectionNote += '// This script includes only the asterisms manually selected by the user.\n';
				}
				
				// Get current values from UI
				var duration = ($asterTourDuration && $asterTourDuration.length) ? parseFloat($asterTourDuration.val()) : 3;
				var isolate = ($asterTourIsolate && $asterTourIsolate.length) ? $asterTourIsolate.is(':checked') : true;
				var showRayHelpers = ($('#aster-show-rayhelpers').length) ? $('#aster-show-rayhelpers').is(':checked') : true;
				
				// Get zoom FOV value
				var zoomFov = ($('#aster-zoom-fov').length) ? parseFloat($('#aster-zoom-fov').val()) : 25;
				
				// Get Overview FOV value
				var overviewFov = ($('#aster-overview-fov').length) ? parseFloat($('#aster-overview-fov').val()) : 60;
				
				// Get color values from color pickers (convert from 0-100 range to 0.0-1.0)
				var linesColorR = ($('#aster-lines-color-r').length) ? (parseFloat($('#aster-lines-color-r').val()) / 100).toFixed(2) : 0.80;
				var linesColorG = ($('#aster-lines-color-g').length) ? (parseFloat($('#aster-lines-color-g').val()) / 100).toFixed(2) : 0.80;
				var linesColorB = ($('#aster-lines-color-b').length) ? (parseFloat($('#aster-lines-color-b').val()) / 100).toFixed(2) : 0.80;
				
				var rayHelpersColorR = ($('#aster-rayhelpers-color-r').length) ? (parseFloat($('#aster-rayhelpers-color-r').val()) / 100).toFixed(2) : 0.50;
				var rayHelpersColorG = ($('#aster-rayhelpers-color-g').length) ? (parseFloat($('#aster-rayhelpers-color-g').val()) / 100).toFixed(2) : 1.00;
				var rayHelpersColorB = ($('#aster-rayhelpers-color-b').length) ? (parseFloat($('#aster-rayhelpers-color-b').val()) / 100).toFixed(2) : 0.50;
				
				var labelsColorR = ($('#aster-labels-color-r').length) ? (parseFloat($('#aster-labels-color-r').val()) / 100).toFixed(2) : 1.00;
				var labelsColorG = ($('#aster-labels-color-g').length) ? (parseFloat($('#aster-labels-color-g').val()) / 100).toFixed(2) : 1.00;
				var labelsColorB = ($('#aster-labels-color-b').length) ? (parseFloat($('#aster-labels-color-b').val()) / 100).toFixed(2) : 1.00;
				
				// Validate ranges
				linesColorR = Math.min(1.00, Math.max(0.00, parseFloat(linesColorR))).toFixed(2);
				linesColorG = Math.min(1.00, Math.max(0.00, parseFloat(linesColorG))).toFixed(2);
				linesColorB = Math.min(1.00, Math.max(0.00, parseFloat(linesColorB))).toFixed(2);
				
				rayHelpersColorR = Math.min(1.00, Math.max(0.00, parseFloat(rayHelpersColorR))).toFixed(2);
				rayHelpersColorG = Math.min(1.00, Math.max(0.00, parseFloat(rayHelpersColorG))).toFixed(2);
				rayHelpersColorB = Math.min(1.00, Math.max(0.00, parseFloat(rayHelpersColorB))).toFixed(2);
				
				labelsColorR = Math.min(1.00, Math.max(0.00, parseFloat(labelsColorR))).toFixed(2);
				labelsColorG = Math.min(1.00, Math.max(0.00, parseFloat(labelsColorG))).toFixed(2);
				labelsColorB = Math.min(1.00, Math.max(0.00, parseFloat(labelsColorB))).toFixed(2);
				
				var fontSize = ($('#aster-font-size').length) ? parseInt($('#aster-font-size').val()) : 16;
				var lineThickness = ($('#aster-line-thickness').length) ? parseInt($('#aster-line-thickness').val()) : 2;
				var rayHelperThickness = ($('#aster-rayhelper-thickness').length) ? parseInt($('#aster-rayhelper-thickness').val()) : 1;
				
				var linesFadeDuration = ($('#aster-lines-fade').length) ? parseFloat($('#aster-lines-fade').val()) : 1.0;
				var labelsFadeDuration = ($('#aster-labels-fade').length) ? parseFloat($('#aster-labels-fade').val()) : 1.0;
				var rayHelpersFadeDuration = ($('#aster-rayhelpers-fade').length) ? parseFloat($('#aster-rayhelpers-fade').val()) : 1.0;
				
				if (!asterisms || totalAsterisms === 0) {
						showTourNotification(_tr("No asterisms available for this culture"));
						return;
				}
				
				// Build script header
				var script = '';
				script += '//\n';
				script += '// ============================================================\n';
				script += '// Name: ' + cultureName + ' Asterisms Tour (Static Array)\n';
				script += '// ============================================================\n';
				script += '// Version: 1.0\n';
				script += '// Author: Stellarium Remote Control\n';
				script += '// License: Public Domain\n';
				script += '// Culture: ' + cultureId + '\n';
				script += selectionNote;
				script += '// Total Asterisms in this script: ' + totalAsterisms + '\n';
				script += '// Duration per Asterism: ' + duration + ' seconds\n';
				script += '// Zoom FOV: ' + zoomFov + ' degrees\n';
				script += '// Overview FOV: ' + overviewFov + ' degrees\n';
				script += '// Font Size: ' + fontSize + ' pixels\n';
				script += '// Line Thickness: ' + lineThickness + ' pixels\n';
				script += '// Ray Helper Thickness: ' + rayHelperThickness + ' pixels\n';
				script += '// Isolate Mode: ' + (isolate ? 'ON' : 'OFF') + '\n';
				script += '// Show Ray Helpers: ' + (showRayHelpers ? 'ON' : 'OFF') + '\n';
				script += '// Lines Color: RGB(' + linesColorR + ', ' + linesColorG + ', ' + linesColorB + ')\n';
				script += '// Labels Color: RGB(' + labelsColorR + ', ' + labelsColorG + ', ' + labelsColorB + ')\n';
				script += '// Ray Helpers Color: RGB(' + rayHelpersColorR + ', ' + rayHelpersColorG + ', ' + rayHelpersColorB + ')\n';
				script += '// Generated: ' + new Date().toLocaleString() + '\n';
				script += '// ============================================================\n';
				script += '//\n';
				
				script += '// ============================================================\n\n';
				
				// Include translation support
				script += '// ============================================================\n';
				script += '// TRANSLATION SUPPORT\n';
				script += '// ============================================================\n';
				script += 'include("i18n.inc");\n\n';
				
				// Culture verification
				script += '// ============================================================\n';
				script += '// CULTURE VERIFICATION\n';
				script += '// ============================================================\n';
				script += 'var currentCulture = core.getSkyCulture();\n';
				script += 'var requiredCulture = "' + cultureId + '";\n';
				script += 'core.debug(tr("Current culture: ") + currentCulture);\n';
				script += 'core.debug(tr("Required culture: ") + requiredCulture);\n\n';
				script += 'if (currentCulture !== requiredCulture) {\n';
				script += '    core.debug(tr("Switching to required sky culture: ") + requiredCulture);\n';
				script += '    core.setSkyCulture(requiredCulture);\n';
				script += '    core.wait(1);\n';
				script += '    core.debug(tr("Culture switched."));\n';
				script += '}\n\n';
				
				// Initial setup
				script += '// ============================================================\n';
				script += '// INITIAL SETUP\n';
				script += '// ============================================================\n';
				script += 'core.clear("starchart");\n\n';
				script += 'GridLinesMgr.setFlagGridlines(false);\n\n';
				script += 'core.setGuiVisible(false);\n\n';
				script += 'StelMovementMgr.setFlagTracking(false);\n\n';
				
				// Save original settings
				script += '// ============================================================\n';
				script += '// SAVE ORIGINAL SETTINGS\n';
				script += '// ============================================================\n';
				script += 'var oldLinesState = AsterismMgr.getFlagLines();\n';
				script += 'var oldLabelsState = AsterismMgr.getFlagLabels();\n';
				script += 'var oldRayHelpersState = AsterismMgr.getFlagRayHelpers();\n';
				script += 'var oldIsolateState = AsterismMgr.getFlagIsolateAsterismSelected();\n';
				script += 'var oldLineThickness = AsterismMgr.getAsterismLineThickness();\n';
				script += 'var oldRayHelperThickness = AsterismMgr.getRayHelperThickness();\n';
				script += 'var oldFontSize = AsterismMgr.getFontSize();\n';
				script += 'var oldLinesColor = AsterismMgr.getLinesColor();\n';
				script += 'var oldLabelsColor = AsterismMgr.getLabelsColor();\n';
				script += 'var oldRayHelpersColor = AsterismMgr.getRayHelpersColor();\n';
				script += 'var oldLinesFade = AsterismMgr.getLinesFadeDuration();\n';
				script += 'var oldLabelsFade = AsterismMgr.getLabelsFadeDuration();\n';
				script += 'var oldRayHelpersFade = AsterismMgr.getRayHelpersFadeDuration();\n\n';
				
				// Configure appearance
				script += '// ============================================================\n';
				script += '// CONFIGURE APPEARANCE\n';
				script += '// ============================================================\n';
				script += 'var linesColor = core.vec3f(' + linesColorR + ', ' + linesColorG + ', ' + linesColorB + ');\n';
				script += 'AsterismMgr.setLinesColor(linesColor);\n';
				script += 'core.debug(tr("Asterism lines color set"));\n\n';
				script += 'var labelsColor = core.vec3f(' + labelsColorR + ', ' + labelsColorG + ', ' + labelsColorB + ');\n';
				script += 'AsterismMgr.setLabelsColor(labelsColor);\n';
				script += 'core.debug(tr("Asterism labels color set"));\n\n';
				script += 'var rayHelpersColor = core.vec3f(' + rayHelpersColorR + ', ' + rayHelpersColorG + ', ' + rayHelpersColorB + ');\n';
				script += 'AsterismMgr.setRayHelpersColor(rayHelpersColor);\n';
				script += 'core.debug(tr("Ray helpers color set"));\n\n';
				script += 'AsterismMgr.setFontSize(' + fontSize + ');\n';
				script += 'core.debug(tr("Asterism label font size set to ") + ' + fontSize + ');\n\n';
				script += 'AsterismMgr.setAsterismLineThickness(' + lineThickness + ');\n';
				script += 'core.debug(tr("Asterism line thickness set to ") + ' + lineThickness + ');\n\n';
				script += 'AsterismMgr.setRayHelperThickness(' + rayHelperThickness + ');\n';
				script += 'core.debug(tr("Ray helper thickness set to ") + ' + rayHelperThickness + ');\n\n';
				script += 'AsterismMgr.setLinesFadeDuration(' + linesFadeDuration + ');\n';
				script += 'AsterismMgr.setLabelsFadeDuration(' + labelsFadeDuration + ');\n';
				script += 'AsterismMgr.setRayHelpersFadeDuration(' + rayHelpersFadeDuration + ');\n';
				script += 'core.debug(tr("Fade durations configured"));\n\n';
				script += 'AsterismMgr.setFlagLines(true);\n';
				script += 'AsterismMgr.setFlagLabels(true);\n';
				script += 'AsterismMgr.setFlagRayHelpers(' + (showRayHelpers ? 'true' : 'false') + ');\n';
				script += 'AsterismMgr.setFlagIsolateAsterismSelected(' + (isolate ? 'true' : 'false') + ');\n\n';
				
				// Disable constellations for clean view
				script += 'ConstellationMgr.deselectConstellations();\n';
				script += 'ConstellationMgr.setFlagArt(false);\n';
				script += 'ConstellationMgr.setFlagBoundaries(false);\n';
				script += 'ConstellationMgr.setFlagLines(false);\n';
				script += 'ConstellationMgr.setFlagLabels(false);\n\n';
				
				// Welcome message
				script += '// ============================================================\n';
				script += '// WELCOME MESSAGE\n';
				script += '// ============================================================\n';
				script += 'var msg = tr("Welcome to the ");\n';
				script += 'msg += "' + cultureName + '";\n';
				script += 'msg += tr(" asterisms tour!\\n\\nExploring celestial patterns across the sky.\\n\\nPress Ctrl+T to show toolbar at any time.");\n';
				script += 'var labelId = LabelMgr.labelScreen(msg, 100, 100, false, 16, "#ffaa66");\n';
				script += 'LabelMgr.setLabelShow(labelId, true);\n';
				script += 'core.wait(6);\n';
				script += 'LabelMgr.setLabelShow(labelId, false);\n';
				script += 'LabelMgr.deleteLabel(labelId);\n\n';
				
				// Asterism array
				script += '// ============================================================\n';
				script += '// ASTERISM LIST (Static Array)\n';
				script += '// ============================================================\n';
				script += '// This array contains ' + totalAsterisms + ' asterism names\n';
				script += '// extracted from the ' + cultureName + ' sky culture data.\n';
				if (isMultiSelect) {
						script += '// NOTE: This is a CUSTOM list containing only manually selected asterisms.\n';
				}
				script += 'var asterisms = [\n';
				
				for (var i = 0; i < asterisms.length; i++) {
						var ast = asterisms[i];
						var name = ast.searchName || ast.name;
						var escapedName = name.replace(/"/g, '\\"');
						
						var commentText = '';
						if (ast.englishName && ast.englishName !== name) {
								commentText = ast.englishName;
						} else if (ast.nativeName && ast.nativeName !== name) {
								commentText = ast.nativeName;
						}
						
						var line = '    "' + escapedName + '"';
						if (i < asterisms.length - 1) {
								line += ',';
						}
						if (commentText) {
								line += ' // ' + commentText;
						}
						script += line + '\n';
				}
				
				script += '];\n\n';
				
				// Tour configuration
				script += '// ============================================================\n';
				script += '// TOUR CONFIGURATION\n';
				script += '// ============================================================\n';
				script += 'var DURATION = ' + duration + ';  // Seconds per asterism\n';
				script += 'var ZOOM_FOV = ' + zoomFov + ';  // Field of view when zoomed in (degrees)\n';
				script += 'var OVERVIEW_FOV = ' + overviewFov + ';  // Field of view for overview\n\n';
				
				// Tour execution
				script += '// ============================================================\n';
				script += '// TOUR EXECUTION\n';
				script += '// ============================================================\n';
				script += 'core.debug(tr("Starting asterisms tour..."));\n';
				script += 'core.debug(tr("Total asterisms in array: ") + asterisms.length);\n\n';
				
				script += 'for (var i = 0; i < asterisms.length; i++) {\n';
				script += '    var asterismName = asterisms[i];\n';
				script += '    var currentNum = i + 1;\n';
				script += '    \n';
				script += '    core.debug(tr("Visiting asterism") + " " + currentNum + "/" + asterisms.length + ": " + asterismName);\n';
				script += '    \n';
				script += '    AsterismMgr.selectAsterism(asterismName);\n';
				script += '    \n';
				script += '    core.moveToObject(asterismName, 2);\n';
				script += '    core.wait(1);\n';
				script += '    \n';
				script += '    StelMovementMgr.zoomTo(ZOOM_FOV, 2);\n';
				script += '    core.wait(1);\n';
				script += '    \n';
				script += '    var nameLabel = LabelMgr.labelScreen(asterismName, 50, 50, false, ' + fontSize + ', "#ffaa66");\n';
				script += '    LabelMgr.setLabelShow(nameLabel, true);\n';
				script += '    core.wait(DURATION);\n';
				script += '    LabelMgr.deleteLabel(nameLabel);\n';
				script += '    \n';
				script += '    AsterismMgr.deselectAsterisms();\n';
				script += '    \n';
				script += '    StelMovementMgr.zoomTo(OVERVIEW_FOV, 2);\n';
				script += '    core.wait(1);\n';
				script += '}\n\n';
				
				// Reset and cleanup
				script += '// ============================================================\n';
				script += '// RESET AND CLEANUP\n';
				script += '// ============================================================\n';
				script += 'core.debug(tr("Tour completed! Restoring original settings..."));\n\n';
				script += 'AsterismMgr.deselectAsterisms();\n';
				script += 'AsterismMgr.setFlagLines(oldLinesState);\n';
				script += 'AsterismMgr.setFlagLabels(oldLabelsState);\n';
				script += 'AsterismMgr.setFlagRayHelpers(oldRayHelpersState);\n';
				script += 'AsterismMgr.setFlagIsolateAsterismSelected(oldIsolateState);\n';
				script += 'AsterismMgr.setAsterismLineThickness(oldLineThickness);\n';
				script += 'AsterismMgr.setRayHelperThickness(oldRayHelperThickness);\n';
				script += 'AsterismMgr.setFontSize(oldFontSize);\n';
				script += 'AsterismMgr.setLinesColor(oldLinesColor);\n';
				script += 'AsterismMgr.setLabelsColor(oldLabelsColor);\n';
				script += 'AsterismMgr.setRayHelpersColor(oldRayHelpersColor);\n';
				script += 'AsterismMgr.setLinesFadeDuration(oldLinesFade);\n';
				script += 'AsterismMgr.setLabelsFadeDuration(oldLabelsFade);\n';
				script += 'AsterismMgr.setRayHelpersFadeDuration(oldRayHelpersFade);\n\n';
				script += 'GridLinesMgr.setFlagGridlines(true);\n\n';
				script += 'core.setGuiVisible(true);\n\n';
				script += 'core.clear("natural");\n\n';
				script += 'StelMovementMgr.zoomTo(100, 3);\n';
				script += 'core.moveToAltAzi(40.0, 180.0, 3.0);\n\n';
				script += 'core.debug(tr("Asterisms tour completed successfully!"));\n';
				
				// Insert into Script Editor or save as file
				var scriptSize = new Blob([script]).size;
				var MAX_SAFE_SIZE = 50 * 1024;
				
				if (scriptSize > MAX_SAFE_SIZE && totalAsterisms > 200) {
						var blob = new Blob([script], { type: 'text/plain;charset=utf-8' });
						var url = URL.createObjectURL(blob);
						var a = document.createElement('a');
						a.href = url;
						a.download = cultureId + '_asterisms_tour' + (isMultiSelect ? '_selected' : '') + '.ssc';
						document.body.appendChild(a);
						a.click();
						document.body.removeChild(a);
						setTimeout(function() { URL.revokeObjectURL(url); }, 100);
						showTourNotification(_tr("Large asterisms script saved as file: ") + cultureId + '_asterisms_tour.ssc');
				} else if (window._stelScriptEditor && typeof window._stelScriptEditor.setEditorContent === 'function') {
						window._stelScriptEditor.setEditorContent(script);
						switchToScriptEditor();
						showTourNotification(_tr("Asterisms tour script generated and inserted into editor") + 
								(isMultiSelect ? (" (" + totalAsterisms + " selected)") : ""));
				} else {
						copyToClipboard(script);
						showTourNotification(_tr("Script copied to clipboard") + 
								(isMultiSelect ? (" (" + totalAsterisms + " selected)") : ""));
				}
		}
		
		// ========================================================================
    // LUNAR MANSIONS MULTI-SELECT FUNCTIONS
    // ========================================================================

    /**
     * Update the selection count display for lunar mansions in multi-select mode.
     * Updates the counter badge and enables/disables the generate button
     * based on whether any lunar mansions are selected.
     */
    function updateLunarSelectionCount() {
        var count = selectedLunarMansionIds.size;
        if ($lunarSelectionCount && $lunarSelectionCount.length) {
            $lunarSelectionCount.text(count + ' ' + (count === 1 ? _tr("selected") : _tr("selected")));
        }
        
        // Update generate button state (disable if nothing selected)
        if ($lunarTourGenerateStaticBtn && $lunarTourGenerateStaticBtn.length) {
            $lunarTourGenerateStaticBtn.prop('disabled', count === 0 && isLunarMultiSelectModeActive);
        }
    }

    /**
     * Clear all selections in multi-select mode for lunar mansions.
     * Removes all selected IDs and resets button styles.
     */
    function clearLunarMultiSelection() {
        selectedLunarMansionIds.clear();
        if ($lunarContainer && $lunarContainer.length) {
            // Clear styles without re-rendering
            $lunarContainer.find('.pattern-btn').removeClass('selected-lunar');
            $lunarContainer.find('.pattern-btn').css('background', '');
            $lunarContainer.find('.pattern-btn').css('border-color', '');
            $lunarContainer.find('.pattern-btn').css('color', '');
            $lunarContainer.find('.pattern-btn').removeClass('active');
        }
        updateLunarSelectionCount();
    }

    /**
     * Get selected lunar mansion objects for multi-select mode.
     * 
     * @returns {Array|null} Array of selected lunar mansion objects or null if none selected
     */
    function getSelectedLunarMansions() {
        if (!isLunarMultiSelectModeActive || selectedLunarMansionIds.size === 0) {
            return null;
        }
        
        var selected = [];
        var allLunarMansions = currentPatternsData.lunar;
        
        if (!allLunarMansions) return null;
        
        for (var i = 0; i < allLunarMansions.length; i++) {
            var mansion = allLunarMansions[i];
            if (selectedLunarMansionIds.has(mansion.id)) {
                selected.push(mansion);
            }
        }
        
        return selected;
    }

    /**
     * Update lunar mansion button behavior based on multi-select mode.
     * 
     * When multi-select mode is ON:
     * - Re-renders buttons without the original click handler
     * - Attaches custom click handler that toggles selection state
     * - Selected buttons get special styling
     * 
     * When multi-select mode is OFF:
     * - Re-renders buttons with original navigation handler
     * - Clears any selection state
     */
    function updateLunarButtonsBehavior() {
        if (!$lunarContainer || !$lunarContainer.length) return;
        
        if (isLunarMultiSelectModeActive) {
            // Get current patterns data
            var currentPatterns = currentPatternsData.lunar;
            if (currentPatterns && currentPatterns.length) {
                // Re-render with handler disabled
                renderButtons($lunarContainer, currentPatterns, "lunar", null, true);
                
                // Then attach multi-select behavior
                $lunarContainer.find('.pattern-btn').off('click.lunar').on('click.lunar', function(e) {
                    e.preventDefault();
                    e.stopPropagation();
                    
                    var $btn = $(this);
                    var patternId = $btn.data('pattern-id');
                    var patternName = $btn.data('pattern-name');
                    
                    if (!patternId) return false;
                    
                    if (selectedLunarMansionIds.has(patternId)) {
                        // Deselect: remove from set and clear styling
                        selectedLunarMansionIds.delete(patternId);
                        $btn.removeClass('selected-lunar');
                        $btn.css('background', '');
                        $btn.css('border-color', '');
                        $btn.css('color', '');
                        $btn.removeClass('active');
                        console.log("[SkyCulture] Lunar mansion deselected:", patternName, "ID:", patternId);
                    } else {
                        // Select: add to set and apply selected styling
                        selectedLunarMansionIds.add(patternId);
                        $btn.addClass('selected-lunar');
                        $btn.css('background', 'linear-gradient(#DCDBDA, #8A8C8E)');
                        $btn.css('border-color', '#000000');
                        $btn.css('color', '#000000');
                        console.log("[SkyCulture] Lunar mansion selected:", patternName, "ID:", patternId);
                    }
                    
                    updateLunarSelectionCount();
                    return false;
                });
            }
        } else {
            // Normal mode: re-render with original handler
            var currentPatterns = currentPatternsData.lunar;
            if (currentPatterns && currentPatterns.length) {
                renderButtons($lunarContainer, currentPatterns, "lunar", 
                    function(patternName, patternType, searchName, patternId) {
                        stelUtils.goToLunarMansion(searchName || patternName, patternId);
                        updateAllButtonStates();
                    },
                    false  // Handler enabled
                );
            }
            
            // Clear any selected lunar mansion highlight from previous multi-select
            clearLunarMultiSelection();
        }
    }
		
		// ========================================================================
    // LUNAR MANSIONS TOUR SCRIPT GENERATION
    // ========================================================================

    /**
     * Generate a static SSC script for lunar mansions tour with embedded array.
     * 
     * This function generates a complete Stellarium script that tours through
     * all lunar mansions (or selected ones if multi-select mode is active)
     * defined in the current sky culture.
     * 
     * Features:
     * - Culture verification and automatic switching
     * - Multi-select support (only selected mansions when in multi-select mode)
     * - Disables constellation lines/art/boundaries for clean view
     * - Shows lunar system lines with custom color
     * - Smooth zoom transitions between mansions
     * - Welcome message with culture name
     * - Proper state restoration after tour
     * 
     * The script uses the English names of lunar mansions for navigation.
     * 
     * @function generateLunarMansionsTourScriptStatic
     */
    function generateLunarMansionsTourScriptStatic() {
        // Determine which lunar mansions to use (Multi-Select or All)
        var mansionsToUse = null;
        var isMultiSelect = isLunarMultiSelectModeActive && selectedLunarMansionIds.size > 0;
        
        if (isMultiSelect) {
            mansionsToUse = getSelectedLunarMansions();
            if (!mansionsToUse || mansionsToUse.length === 0) {
                showTourNotification(_tr("No lunar mansions selected. Please select at least one mansion."));
                return;
            }
            showTourNotification(_tr("Generating script for ") + mansionsToUse.length + _tr(" selected lunar mansions"));
        } else {
            mansionsToUse = currentPatternsData.lunar;
            if (!mansionsToUse || mansionsToUse.length === 0) {
                showTourNotification(_tr("No lunar mansions available for this culture"));
                return;
            }
        }
        
        var lunarMansions = mansionsToUse;
        var totalMansions = lunarMansions.length;
        var cultureName = currentCultureName || currentCultureId;
        var cultureId = currentCultureId;
        
        // Add indicator in header if multi-select mode was used
        var selectionNote = '';
        if (isMultiSelect) {
            selectionNote = '// Selected Lunar Mansions: ' + selectedLunarMansionIds.size + ' out of ' + (currentPatternsData.lunar ? currentPatternsData.lunar.length : 0) + '\n';
            selectionNote += '// This script includes only the lunar mansions manually selected by the user.\n';
        }
        
        // Get current values from UI
        var duration = ($lunarTourDuration && $lunarTourDuration.length) ? parseFloat($lunarTourDuration.val()) : 4;
        var zoomFov = ($lunarTourZoomFov && $lunarTourZoomFov.length) ? parseFloat($lunarTourZoomFov.val()) : 30;
        var overviewFov = ($lunarTourOverviewFov && $lunarTourOverviewFov.length) ? parseFloat($lunarTourOverviewFov.val()) : 90;
        
        var fontSize = ($lunarTourFontSize && $lunarTourFontSize.length) ? parseInt($lunarTourFontSize.val()) : 16;
        var showLabels = ($lunarTourShowLabels && $lunarTourShowLabels.length) ? $lunarTourShowLabels.is(':checked') : true;
        var showLines = ($lunarTourShowLines && $lunarTourShowLines.length) ? $lunarTourShowLines.is(':checked') : true;
        
        // Color values (convert from 0-100 range to 0.0-1.0)
        var linesColorR = ($('#lunar-lines-color-r').length) ? (parseFloat($('#lunar-lines-color-r').val()) / 100).toFixed(2) : 0.70;
        var linesColorG = ($('#lunar-lines-color-g').length) ? (parseFloat($('#lunar-lines-color-g').val()) / 100).toFixed(2) : 0.30;
        var linesColorB = ($('#lunar-lines-color-b').length) ? (parseFloat($('#lunar-lines-color-b').val()) / 100).toFixed(2) : 0.80;
        
        // Validate color ranges
        linesColorR = Math.min(1.00, Math.max(0.00, parseFloat(linesColorR))).toFixed(2);
        linesColorG = Math.min(1.00, Math.max(0.00, parseFloat(linesColorG))).toFixed(2);
        linesColorB = Math.min(1.00, Math.max(0.00, parseFloat(linesColorB))).toFixed(2);
            
        if (!lunarMansions || totalMansions === 0) {
            showTourNotification(_tr("No lunar mansions available for this culture"));
            return;
        }
        
        // Build script header
        var script = '';
        script += '//\n';
        script += '// ============================================================\n';
        script += '// Name: ' + cultureName + ' Lunar Mansions Tour\n';
        script += '// ============================================================\n';
        script += '// Version: 1.0\n';
        script += '// Author: Stellarium Remote Control\n';
        script += '// License: Public Domain\n';
        script += '// Culture: ' + cultureId + '\n';
        script += selectionNote;
        script += '// Total Lunar Mansions: ' + totalMansions + '\n';
        script += '// Duration per Mansion: ' + duration + ' seconds\n';
        script += '// Zoom FOV: ' + zoomFov + ' degrees\n';
        script += '// Overview FOV: ' + overviewFov + ' degrees\n';
        script += '// Font Size: ' + fontSize + ' pixels\n';
        script += '// Show Labels: ' + (showLabels ? 'ON' : 'OFF') + '\n';
        script += '// Show Lines: ' + (showLines ? 'ON' : 'OFF') + '\n';
        script += '// Lines Color: RGB(' + linesColorR + ', ' + linesColorG + ', ' + linesColorB + ')\n';
        script += '// Generated: ' + new Date().toLocaleString() + '\n';
        script += '// ============================================================\n';
        script += '\n';
        
        // Include translation support
        script += '// ============================================================\n';
        script += '// TRANSLATION SUPPORT\n';
        script += '// ============================================================\n';
        script += 'include("i18n.inc");\n\n';
        
        // Culture verification
        script += '// ============================================================\n';
        script += '// CULTURE VERIFICATION\n';
        script += '// ============================================================\n';
        script += 'var currentCulture = core.getSkyCulture();\n';
        script += 'var requiredCulture = "' + cultureId + '";\n';
        script += 'core.debug(tr("Current culture: ") + currentCulture);\n';
        script += 'core.debug(tr("Required culture: ") + requiredCulture);\n\n';
        script += 'if (currentCulture !== requiredCulture) {\n';
        script += '    core.debug(tr("Switching to required sky culture: ") + requiredCulture);\n';
        script += '    core.setSkyCulture(requiredCulture);\n';
        script += '    core.wait(1);\n';
        script += '    core.debug(tr("Culture switched."));\n';
        script += '}\n\n';
        
        // Check if lunar system exists
        script += '// ============================================================\n';
        script += '// CHECK LUNAR SYSTEM AVAILABILITY\n';
        script += '// ============================================================\n';
        script += '// Note: This culture may or may not have lunar mansions defined.\n';
        script += '// The script will attempt to run; if no lines appear, the culture\n';
        script += '// simply does not have lunar mansion data.\n\n';
        
        // Initial setup
        script += '// ============================================================\n';
        script += '// INITIAL SETUP\n';
        script += '// ============================================================\n';
        script += 'core.clear("starchart");\n\n';
        script += 'GridLinesMgr.setFlagGridlines(false);\n\n';
        script += 'core.setGuiVisible(false);\n\n';
        script += 'StelMovementMgr.setFlagTracking(false);\n\n';
        
        // Save original settings
        script += '// ============================================================\n';
        script += '// SAVE ORIGINAL SETTINGS\n';
        script += '// ============================================================\n';
        script += '// Save current lunar system display states\n';
        script += 'var oldLunarState = ConstellationMgr.getFlagLunarSystem();\n';
        script += 'var oldLunarColor = ConstellationMgr.getLunarSystemColor();\n\n';
        script += '// Save current constellation display states\n';
        script += 'var oldConstLinesState = ConstellationMgr.getFlagLines();\n';
        script += 'var oldConstBoundariesState = ConstellationMgr.getFlagBoundaries();\n';
        script += 'var oldConstLabelsState = ConstellationMgr.getFlagLabels();\n';
        script += 'var oldConstArtState = ConstellationMgr.getFlagArt();\n\n';
        script += '// Save current asterism display states\n';
        script += 'var oldAsterLinesState = AsterismMgr.getFlagLines();\n';
        script += 'var oldAsterLabelsState = AsterismMgr.getFlagLabels();\n\n';
        
        // Disable distracting elements
        script += '// ============================================================\n';
        script += '// DISABLE DISTRACTING ELEMENTS\n';
        script += '// ============================================================\n';
        script += 'ConstellationMgr.deselectConstellations();\n';
        script += 'ConstellationMgr.setFlagArt(false);\n';
        script += 'ConstellationMgr.setFlagBoundaries(false);\n';
        script += 'ConstellationMgr.setFlagLines(false);\n';
        script += 'ConstellationMgr.setFlagLabels(false);\n\n';
        script += 'AsterismMgr.setFlagLines(false);\n';
        script += 'AsterismMgr.setFlagLabels(false);\n\n';
        
        // Configure lunar system appearance
        script += '// ============================================================\n';
        script += '// CONFIGURE LUNAR SYSTEM APPEARANCE\n';
        script += '// ============================================================\n';
        script += 'ConstellationMgr.setFlagLunarSystem(' + (showLines ? 'true' : 'false') + ');\n';
        script += 'var lunarColor = core.vec3f(' + linesColorR + ', ' + linesColorG + ', ' + linesColorB + ');\n';
        script += 'ConstellationMgr.setLunarSystemColor(lunarColor);\n';
        script += 'core.debug(tr("Lunar system lines color set"));\n\n';
        
        // Welcome message
        script += '// ============================================================\n';
        script += '// WELCOME MESSAGE\n';
        script += '// ============================================================\n';
        script += 'var msg = tr("Welcome to the ");\n';
        script += 'msg += "' + cultureName + '";\n';
        script += 'msg += tr(" lunar mansions tour!\\n\\nLunar mansions are divisions of the Moon\\\'s path across the sky.\\n\\nEach culture has its own unique system.\\n\\nPress Ctrl+T to show toolbar at any time.");\n';
        script += 'var labelId = LabelMgr.labelScreen(msg, 100, 100, false, 16, "#ffaa66");\n';
        script += 'LabelMgr.setLabelShow(labelId, true);\n';
        script += 'core.wait(6);\n';
        script += 'LabelMgr.setLabelShow(labelId, false);\n';
        script += 'LabelMgr.deleteLabel(labelId);\n\n';
        
        // Lunar mansions array
        script += '// ============================================================\n';
        script += '// LUNAR MANSIONS LIST (Static Array)\n';
        script += '// ============================================================\n';
        script += '// This array contains ' + totalMansions + ' lunar mansion names\n';
        script += '// extracted from the ' + cultureName + ' sky culture data.\n';
        if (isMultiSelect) {
            script += '// NOTE: This is a CUSTOM list containing only manually selected lunar mansions.\n';
        }
        script += 'var lunarMansions = [\n';
        
        for (var i = 0; i < lunarMansions.length; i++) {
            var mansion = lunarMansions[i];
            var name = mansion.searchName || mansion.name;
            var escapedName = name.replace(/"/g, '\\"');
            
            var commentText = '';
            if (mansion.englishName && mansion.englishName !== name) {
                commentText = mansion.englishName;
            } else if (mansion.nativeName && mansion.nativeName !== name) {
                commentText = mansion.nativeName;
            }
            
            var line = '    "' + escapedName + '"';
            if (i < lunarMansions.length - 1) {
                line += ',';
            }
            if (commentText) {
                line += ' // ' + commentText;
            }
            script += line + '\n';
        }
        
        script += '];\n\n';
        
        // Tour configuration
        script += '// ============================================================\n';
        script += '// TOUR CONFIGURATION\n';
        script += '// ============================================================\n';
        script += 'var DURATION = ' + duration + ';  // Seconds per lunar mansion\n';
        script += 'var ZOOM_FOV = ' + zoomFov + ';  // Field of view when zoomed in (degrees)\n';
        script += 'var OVERVIEW_FOV = ' + overviewFov + ';  // Field of view for overview\n';
        script += 'var SHOW_LABELS = ' + (showLabels ? 'true' : 'false') + ';  // Show mansion names on screen\n';
        script += 'var LABEL_FONT_SIZE = ' + fontSize + ';  // Font size for labels\n\n';
        
        // Tour execution
        script += '// ============================================================\n';
        script += '// TOUR EXECUTION\n';
        script += '// ============================================================\n';
        script += 'core.debug(tr("Starting lunar mansions tour..."));\n';
        script += 'core.debug(tr("Total lunar mansions in array: ") + lunarMansions.length);\n\n';
        
        script += 'for (var i = 0; i < lunarMansions.length; i++) {\n';
        script += '    var mansionName = lunarMansions[i];\n';
        script += '    var currentNum = i + 1;\n';
        script += '    \n';
        script += '    core.debug(tr("Visiting lunar mansion") + " " + currentNum + "/" + lunarMansions.length + ": " + mansionName);\n';
        script += '    \n';
        script += '    // Center view on the lunar mansion\n';
        script += '    core.moveToObject(mansionName, 2);\n';
        script += '    core.wait(1);\n';
        script += '    \n';
        script += '    // Zoom in for detailed view\n';
        script += '    StelMovementMgr.zoomTo(ZOOM_FOV, 2);\n';
        script += '    core.wait(1);\n';
        script += '    \n';
        script += '    // Display mansion name on screen if enabled\n';
        script += '    var nameLabel = null;\n';
        script += '    if (SHOW_LABELS) {\n';
        script += '        nameLabel = LabelMgr.labelScreen(mansionName, 50, 50, false, LABEL_FONT_SIZE, "#ffaa66");\n';
        script += '        LabelMgr.setLabelShow(nameLabel, true);\n';
        script += '    }\n';
        script += '    core.wait(DURATION);\n';
        script += '    if (nameLabel !== null) {\n';
        script += '        LabelMgr.deleteLabel(nameLabel);\n';
        script += '    }\n';
        script += '    \n';
        script += '    // Zoom out for overview before next mansion\n';
        script += '    StelMovementMgr.zoomTo(OVERVIEW_FOV, 2);\n';
        script += '    core.wait(1);\n';
        script += '}\n\n';
        
        // Reset and cleanup
        script += '// ============================================================\n';
        script += '// RESET AND CLEANUP\n';
        script += '// ============================================================\n';
        script += 'core.debug(tr("Tour completed! Restoring original settings..."));\n\n';
        script += '// Restore lunar system state\n';
        script += 'ConstellationMgr.setFlagLunarSystem(oldLunarState);\n';
        script += 'ConstellationMgr.setLunarSystemColor(oldLunarColor);\n\n';
        script += '// Restore constellation state\n';
        script += 'ConstellationMgr.setFlagLines(oldConstLinesState);\n';
        script += 'ConstellationMgr.setFlagBoundaries(oldConstBoundariesState);\n';
        script += 'ConstellationMgr.setFlagLabels(oldConstLabelsState);\n';
        script += 'ConstellationMgr.setFlagArt(oldConstArtState);\n\n';
        script += '// Restore asterism state\n';
        script += 'AsterismMgr.setFlagLines(oldAsterLinesState);\n';
        script += 'AsterismMgr.setFlagLabels(oldAsterLabelsState);\n\n';
        script += '// Restore grids\n';
        script += 'GridLinesMgr.setFlagGridlines(true);\n\n';
        script += '// Show GUI again\n';
        script += 'core.setGuiVisible(true);\n\n';
        script += '// Reset to natural sky view\n';
        script += 'core.clear("natural");\n\n';
        script += '// Reset view to default position\n';
        script += 'StelMovementMgr.zoomTo(100, 3);\n';
        script += 'core.moveToAltAzi(40.0, 180.0, 3.0);\n\n';
        script += 'core.debug(tr("Lunar mansions tour completed successfully!"));\n';
        
        // Output the script
        var scriptSize = new Blob([script]).size;
        var MAX_SAFE_SIZE = 50 * 1024;
        
        if (scriptSize > MAX_SAFE_SIZE) {
            var blob = new Blob([script], { type: 'text/plain;charset=utf-8' });
            var url = URL.createObjectURL(blob);
            var a = document.createElement('a');
            a.href = url;
            a.download = cultureId + '_lunar_mansions_tour' + (isMultiSelect ? '_selected' : '') + '.ssc';
            document.body.appendChild(a);
            a.click();
            document.body.removeChild(a);
            setTimeout(function() { URL.revokeObjectURL(url); }, 100);
            showTourNotification(_tr("Lunar mansions script saved as file: ") + cultureId + '_lunar_mansions_tour.ssc');
        } else if (window._stelScriptEditor && typeof window._stelScriptEditor.setEditorContent === 'function') {
            window._stelScriptEditor.setEditorContent(script);
            switchToScriptEditor();
            showTourNotification(_tr("Lunar mansions tour script generated and inserted into editor") + 
                (isMultiSelect ? (" (" + totalMansions + " selected)") : ""));
        } else {
            copyToClipboard(script);
            showTourNotification(_tr("Script copied to clipboard") + 
                (isMultiSelect ? (" (" + totalMansions + " selected)") : ""));
        }
    }
		
		// ========================================================================
    // LUNAR MANSIONS TOUR TOOLBAR CREATION
    // ========================================================================

		/**
		 * Create the tour control toolbar for lunar mansions inside the lunar mansions tab.
		 * Provides controls for generating scripts with full customization.
		 * 
		 * Features:
		 * - Generate static script with embedded lunar mansion array
		 * - Multi-select mode for picking specific mansions
		 * - Duration, zoom, and overview controls
		 * - Color pickers for lines
		 * - Font size and line thickness controls
		 * 
		 * @function createLunarMansionsTourToolbar
		 */
		function createLunarMansionsTourToolbar() {
				console.log("[SkyCulture] createLunarMansionsTourToolbar() called");

				// Check if toolbar already exists
				if ($('#lunar-tour-controls-container').length > 0) {
						console.log("[SkyCulture] Lunar mansions tour toolbar already exists, skipping");
						return;
				}

				// Find the lunar mansions tab content
				var $lunarTab = $('#patterns-tab-lunar');
				if (!$lunarTab.length) {
						console.warn("[SkyCulture] Lunar mansions tab not found");
						return;
				}

				// Find the container where buttons are displayed
				var $buttonsContainer = $lunarTab.find('#lunar-buttons-container');
				if (!$buttonsContainer.length) {
						console.warn("[SkyCulture] Lunar mansions buttons container not found");
						return;
				}

				// Create toolbar container - NO inline styles
				var $toolbarContainer = $('<div id="lunar-tour-controls-container" class="tour-controls-container"></div>');

				// Add section title - NO inline styles
				var $sectionTitle = $('<div class="tour-section-title">' + _tr("Lunar Mansions Tour Generator") + '</div>');

				// Create inner toolbar with multi-column grid layout - NO inline styles
				var $toolbar = $('<div class="tour-toolbar-grid"></div>');

				// ============================================================
				// HELPER FUNCTION: Create color group with live preview
				// ============================================================
				function createLunarColorGroup(label, idPrefix, defaultR, defaultG, defaultB) {
						var $group = $('<div class="tour-color-group"></div>');
						$group.append('<span class="tour-color-group-label">' + label + '</span>');

						var $sliders = $('<div class="tour-color-sliders"></div>');

						$sliders.append('<span class="tour-color-channel tour-color-channel-r">R</span>');
						var $rSlider = $('<input type="range" id="' + idPrefix + '-r" class="tour-color-slider" min="0" max="100" value="' + (defaultR * 100) + '" step="1">');
						$sliders.append($rSlider);

						$sliders.append('<span class="tour-color-channel tour-color-channel-g">G</span>');
						var $gSlider = $('<input type="range" id="' + idPrefix + '-g" class="tour-color-slider" min="0" max="100" value="' + (defaultG * 100) + '" step="1">');
						$sliders.append($gSlider);

						$sliders.append('<span class="tour-color-channel tour-color-channel-b">B</span>');
						var $bSlider = $('<input type="range" id="' + idPrefix + '-b" class="tour-color-slider" min="0" max="100" value="' + (defaultB * 100) + '" step="1">');
						$sliders.append($bSlider);

						var $preview = $('<div id="' + idPrefix + '-preview" class="tour-color-preview"></div>');
						$sliders.append($preview);

						var $valueDisplay = $('<span id="' + idPrefix + '-value" class="tour-color-value">' + defaultR.toFixed(2) + ', ' + defaultG.toFixed(2) + ', ' + defaultB.toFixed(2) + '</span>');
						$sliders.append($valueDisplay);

						$group.append($sliders);

						// Initialize preview
						var previewR = Math.floor(defaultR * 255);
						var previewG = Math.floor(defaultG * 255);
						var previewB = Math.floor(defaultB * 255);
						$preview.css('background', 'rgb(' + previewR + ',' + previewG + ',' + previewB + ')');

						function updateFromSliders() {
								var r = parseFloat($rSlider.val()) / 100;
								var g = parseFloat($gSlider.val()) / 100;
								var b = parseFloat($bSlider.val()) / 100;
								var previewR = Math.floor(r * 255);
								var previewG = Math.floor(g * 255);
								var previewB = Math.floor(b * 255);
								$preview.css('background', 'rgb(' + previewR + ',' + previewG + ',' + previewB + ')');
								$valueDisplay.text(r.toFixed(2) + ', ' + g.toFixed(2) + ', ' + b.toFixed(2));
								// Update section border-left color
								$('#lunar-lines-section').css('border-left-color', 'rgb(' + previewR + ',' + previewG + ',' + previewB + ')');
						}

						$rSlider.on('input', updateFromSliders);
						$gSlider.on('input', updateFromSliders);
						$bSlider.on('input', updateFromSliders);

						return $group;
				}

				// ============================================================
				// COLUMN 1: Script Generation + Timing Controls
				// ============================================================
				var $column1 = $('<div class="tour-column"></div>');

				// Script Generation Row
				var $genRow = $('<div class="tour-row-separator"></div>');
				$lunarTourGenerateStaticBtn = $('<button id="btn-generate-lunar-static" class="jquerybutton pattern-btn tour-generate-btn" title="' + _tr("Generates script with embedded lunar mansion name array") + '">' + _tr("Generate Lunar Mansions Tour Script") + '</button>');
				$genRow.append($lunarTourGenerateStaticBtn);

				// Multi-Select Mode checkbox and counter
				var $multiSelectWrapper = $('<div class="tour-multiselect-green"></div>');
				$lunarMultiSelectMode = $('<input type="checkbox" id="lunar-multi-select-mode">');
				$multiSelectWrapper.append($lunarMultiSelectMode);
				$multiSelectWrapper.append('<label style="font-weight:bold;cursor:pointer;">' + _tr("Multi-Select") + '</label>');
				$multiSelectWrapper.append('<span class="tour-multiselect-hint">(' + _tr("Pick multiple mansions") + ')</span>');

				$lunarSelectionCount = $('<span class="tour-selection-count">0 ' + _tr("selected") + '</span>');
				$multiSelectWrapper.append($lunarSelectionCount);

				$genRow.append($multiSelectWrapper);
				$column1.append($genRow);

				// Timing Controls Row
				var $timingRow = $('<div class="tour-row"></div>');

				var $durationWrapper = $('<div class="tour-control-group"></div>');
				$durationWrapper.append('<label class="tour-control-label-bold">' + _tr("Duration Per Mansion:") + '</label>');
				$lunarTourDuration = $('<input type="number" id="lunar-tour-duration" class="tour-input-number" value="4" min="1" max="15" step="0.5">');
				$durationWrapper.append($lunarTourDuration);
				$durationWrapper.append('<span>' + _tr("sec") + '</span>');
				$timingRow.append($durationWrapper);

				var $zoomWrapper = $('<div class="tour-control-group"></div>');
				$zoomWrapper.append('<label class="tour-control-label-bold">' + _tr("Zoom FOV:") + '</label>');
				$lunarTourZoomFov = $('<input type="number" id="lunar-zoom-fov" class="tour-input-number" value="30" min="5" max="90" step="1">');
				$zoomWrapper.append($lunarTourZoomFov);
				$zoomWrapper.append('<span>' + _tr("deg") + '</span>');
				$timingRow.append($zoomWrapper);

				var $overviewWrapper = $('<div class="tour-control-group"></div>');
				$overviewWrapper.append('<label class="tour-control-label-bold">' + _tr("Overview FOV:") + '</label>');
				$lunarTourOverviewFov = $('<input type="number" id="lunar-overview-fov" class="tour-input-number" value="90" min="20" max="120" step="1">');
				$overviewWrapper.append($lunarTourOverviewFov);
				$overviewWrapper.append('<span>' + _tr("deg") + '</span>');
				$timingRow.append($overviewWrapper);

				$column1.append($timingRow);

				// ============================================================
				// COLUMN 2: Lunar System Lines Controls
				// ============================================================
				var $column2 = $('<div class="tour-column"></div>');

				// LABELS Section (Simplified - no color picker)
				var $rowLabels = $('<div id="lunar-labels-section" class="tour-section-lunar"></div>');
				$rowLabels.append('<div id="lunar-labels-header" class="tour-section-header">' + _tr("SCREEN LABELS") + '</div>');

				var $labelsSubRow = $('<div class="tour-row"></div>');

				var $showLabelsWrapper = $('<div class="tour-control-group"></div>');
				$lunarTourShowLabels = $('<input type="checkbox" id="lunar-show-labels" checked>');
				$showLabelsWrapper.append($lunarTourShowLabels);
				$showLabelsWrapper.append('<label class="tour-control-label">' + _tr("Show on Screen") + '</label>');
				$labelsSubRow.append($showLabelsWrapper);

				var $fontWrapper = $('<div class="tour-control-group"></div>');
				$fontWrapper.append('<label class="tour-control-label">' + _tr("Font Size:") + '</label>');
				$lunarTourFontSize = $('<input type="number" id="lunar-font-size" class="tour-input-number" value="16" min="10" max="30" step="1">');
				$fontWrapper.append($lunarTourFontSize);
				$fontWrapper.append('<span>' + _tr("px") + '</span>');
				$labelsSubRow.append($fontWrapper);

				$rowLabels.append($labelsSubRow);
				$column2.append($rowLabels);

				// LUNAR SYSTEM LINES Section
				var $rowLines = $('<div id="lunar-lines-section" class="tour-section-lunar"></div>');
				$rowLines.append('<div id="lunar-lines-header" class="tour-section-header">' + _tr("LUNAR SYSTEM LINES") + '</div>');

				var $linesSubRow = $('<div class="tour-row"></div>');

				var $showLinesWrapper = $('<div class="tour-control-group"></div>');
				$lunarTourShowLines = $('<input type="checkbox" id="lunar-show-lines" checked>');
				$showLinesWrapper.append($lunarTourShowLines);
				$showLinesWrapper.append('<label class="tour-control-label">' + _tr("Show Lines") + '</label>');
				$linesSubRow.append($showLinesWrapper);

				var $lineThicknessWrapper = $('<div class="tour-control-group"></div>');
				$lineThicknessWrapper.append('<label class="tour-control-label">' + _tr("Thickness:") + '</label>');
				$lunarTourLineThickness = $('<input type="number" id="lunar-line-thickness" class="tour-input-number" value="2" min="1" max="5" step="1">');
				$lineThicknessWrapper.append($lunarTourLineThickness);
				$lineThicknessWrapper.append('<span>' + _tr("px") + '</span>');
				$linesSubRow.append($lineThicknessWrapper);

				$rowLines.append($linesSubRow);
				var $linesColorGroup = createLunarColorGroup(_tr("Lines Color"), "lunar-lines-color", 0.00, 1.00, 0.50);
				$rowLines.append($linesColorGroup);
				$column2.append($rowLines);

				// ============================================================
				// COLUMN 3: Info Note
				// ============================================================
				var $column3 = $('<div class="tour-column"></div>');

				// Add info note about lunar mansions
				var $infoNote = $('<div class="tour-info-note">' +
						'<div class="info-title">' + _tr("About Lunar Mansions") + '</div>' +
						'<div class="info-text">' +
						_tr("Lunar mansions (also called lunar stations or lunar houses) are divisions of the Moon's path across the sky. Different cultures have different systems:") + '<br>' +
						'• ' + _tr("Arabic: 28 mansions (Manazil al-Qamar)") + '<br>' +
						'• ' + _tr("Chinese: 28 mansions (Xiu)") + '<br>' +
						'• ' + _tr("Indian: 28 mansions (Nakshatras)") + '<br><br>' +
						'<span style="color:#81C784;">' + _tr("Lines color can be customized above. Screen labels use a fixed color (#ffaa66) for readability.") + '</span>' +
						'</div></div>');
				$column3.append($infoNote);

				// ============================================================
				// ASSEMBLE ALL COLUMNS INTO TOOLBAR
				// ============================================================
				$toolbar.append($column1);
				$toolbar.append($column2);
				$toolbar.append($column3);

				$toolbarContainer.append($sectionTitle);
				$toolbarContainer.append($toolbar);

				// Insert toolbar AFTER the buttons container (at the bottom of the tab)
				$buttonsContainer.after($toolbarContainer);

				// ============================================================
				// BIND EVENTS
				// ============================================================
				$lunarTourGenerateStaticBtn.on('click', generateLunarMansionsTourScriptStatic);

				// Bind multi-select mode change event
				if ($lunarMultiSelectMode) {
						$lunarMultiSelectMode.on('change', function() {
								isLunarMultiSelectModeActive = $(this).is(':checked');

								if (isLunarMultiSelectModeActive) {
										console.log("[SkyCulture] Entering multi-select mode for lunar mansions");
										selectedPattern = null;
										selectedPatternId = null;
										clearLunarMultiSelection();
										updateLunarButtonsBehavior();
								} else {
										console.log("[SkyCulture] Exiting multi-select mode for lunar mansions");
										clearLunarMultiSelection();
										selectedLunarMansionIds.clear();
										updateLunarButtonsBehavior();
										updateLunarSelectionCount();
								}
						});
				}

				console.log("[SkyCulture] Lunar mansions tour toolbar created successfully");
		}

    // ========================================================================
    // STARS TOUR SCRIPT GENERATION
    // ========================================================================

    /**
     * Generate a static SSC script for stars tour with embedded array.
     * 
     * This function generates a complete Stellarium script that tours through
     * all stars (or selected ones) defined in the current sky culture.
     * 
     * Features:
     * - Culture verification and automatic switching
     * - Multi-select support (only selected stars)
     * - Handles multiple object types: HIP stars, Messier, NGC, IC, DSO, planets
     * - Disables constellations, asterisms, lunar mansions for clean view
     * - Configurable star display settings
     * - Smooth zoom transitions
     * - Welcome message with culture name
     * - Proper state restoration after tour
     * 
     * @function generateStarsTourScriptStatic
     */
    function generateStarsTourScriptStatic() {
        // Determine which stars to use (Multi-Select or All)
        var starsToUse = null;
        var isMultiSelect = isStarsMultiSelectModeActive && selectedStarIds.size > 0;
        
        if (isMultiSelect) {
            starsToUse = getSelectedStars();
            if (!starsToUse || starsToUse.length === 0) {
                showTourNotification(_tr("No stars selected. Please select at least one star."));
                return;
            }
            showTourNotification(_tr("Generating script for ") + starsToUse.length + _tr(" selected stars/objects"));
        } else {
            starsToUse = currentPatternsData.stars;
            if (!starsToUse || starsToUse.length === 0) {
                showTourNotification(_tr("No stars/objects available for this culture"));
                return;
            }
        }
        
        var stars = starsToUse;
        var totalStars = stars.length;
        var cultureName = currentCultureName || currentCultureId;
        var cultureId = currentCultureId;
        
        // Add indicator in header if multi-select mode was used
        var selectionNote = '';
        if (isMultiSelect) {
            selectionNote = '// Selected Stars/Objects: ' + selectedStarIds.size + ' out of ' + (currentPatternsData.stars ? currentPatternsData.stars.length : 0) + '\n';
            selectionNote += '// This script includes only the stars/objects manually selected by the user.\n';
        }
        
        // Get current values from UI
        var duration = ($starsTourDuration && $starsTourDuration.length) ? parseFloat($starsTourDuration.val()) : 3;
        var zoomFov = ($starsTourZoomFov && $starsTourZoomFov.length) ? parseFloat($starsTourZoomFov.val()) : 10;
        var overviewFov = ($starsTourOverviewFov && $starsTourOverviewFov.length) ? parseFloat($starsTourOverviewFov.val()) : 60;
        
        var fontSize = ($starsTourFontSize && $starsTourFontSize.length) ? parseInt($starsTourFontSize.val()) : 16;
        var showLabels = ($starsTourShowLabels && $starsTourShowLabels.length) ? $starsTourShowLabels.is(':checked') : true;
        var labelsAmount = ($starsTourLabelsAmount && $starsTourLabelsAmount.length) ? parseFloat($starsTourLabelsAmount.val()) : 6.0;
        var showAdditionalNames = ($starsTourShowAdditionalNames && $starsTourShowAdditionalNames.length) ? $starsTourShowAdditionalNames.is(':checked') : true;
        var showDblStars = ($starsTourShowDblStars && $starsTourShowDblStars.length) ? $starsTourShowDblStars.is(':checked') : false;
        var showVarStars = ($starsTourShowVarStars && $starsTourShowVarStars.length) ? $starsTourShowVarStars.is(':checked') : false;
        var showHIPNumbers = ($starsTourShowHIPNumbers && $starsTourShowHIPNumbers.length) ? $starsTourShowHIPNumbers.is(':checked') : false;
        
        var absoluteScale = ($starsTourAbsoluteScale && $starsTourAbsoluteScale.length) ? parseFloat($starsTourAbsoluteScale.val()) : 1.0;
        var relativeScale = ($starsTourRelativeScale && $starsTourRelativeScale.length) ? parseFloat($starsTourRelativeScale.val()) : 1.0;
        var showTwinkle = ($starsTourShowTwinkle && $starsTourShowTwinkle.length) ? $starsTourShowTwinkle.is(':checked') : true;
        var twinkleAmount = ($starsTourTwinkleAmount && $starsTourTwinkleAmount.length) ? parseFloat($starsTourTwinkleAmount.val()) : 0.3;
        var showSpiky = ($starsTourShowSpiky && $starsTourShowSpiky.length) ? $starsTourShowSpiky.is(':checked') : false;
        var showHalo = ($starsTourShowHalo && $starsTourShowHalo.length) ? $starsTourShowHalo.is(':checked') : true;
        var useMagnitudeLimit = ($starsTourUseMagLimit && $starsTourUseMagLimit.length) ? $starsTourUseMagLimit.is(':checked') : false;
        var customMagLimit = ($starsTourMagLimit && $starsTourMagLimit.length) ? parseFloat($starsTourMagLimit.val()) : 8.0;
        
        if (!stars || totalStars === 0) {
            showTourNotification(_tr("No stars/objects available for this culture"));
            return;
        }
        
        // Build script header
        var script = '';
        script += '//\n';
        script += '// Name: ' + cultureName + ' Stars and Celestial Objects Tour\n';
        script += '// ============================================================\n';
        script += '// Version: 1.0\n';
        script += '// Author: Stellarium Remote Control\n';
        script += '// License: Public Domain\n';
        script += '// Culture: ' + cultureId + '\n';
        script += selectionNote;
        script += '// Total Objects in this script: ' + totalStars + '\n';
        script += '// Duration per Object: ' + duration + ' seconds\n';
        script += '// Zoom FOV: ' + zoomFov + ' degrees\n';
        script += '// Overview FOV: ' + overviewFov + ' degrees\n';
        script += '// Font Size: ' + fontSize + ' pixels\n';
        script += '// Labels Amount: ' + labelsAmount + '\n';
        script += '// Additional Names: ' + (showAdditionalNames ? 'ON' : 'OFF') + '\n';
        script += '// Double Stars: ' + (showDblStars ? 'ON' : 'OFF') + '\n';
        script += '// Variable Stars: ' + (showVarStars ? 'ON' : 'OFF') + '\n';
        script += '// HIP Numbers: ' + (showHIPNumbers ? 'ON' : 'OFF') + '\n';
        script += '// Absolute Scale: ' + absoluteScale + '\n';
        script += '// Relative Scale: ' + relativeScale + '\n';
        script += '// Twinkle: ' + (showTwinkle ? 'ON' : 'OFF') + '\n';
        script += '// Twinkle Amount: ' + twinkleAmount + '\n';
        script += '// Spiky Stars: ' + (showSpiky ? 'ON' : 'OFF') + '\n';
        script += '// Star Halos: ' + (showHalo ? 'ON' : 'OFF') + '\n';
        script += '// Magnitude Limit: ' + (useMagnitudeLimit ? 'ON (limit=' + customMagLimit + ')' : 'OFF') + '\n';
        script += '// Generated: ' + new Date().toLocaleString() + '\n';
        script += '// ============================================================\n';
        script += '//\n';
        script += '// Description: This script tours culturally significant stars and\n';
        script += '// celestial objects from the ' + cultureName + ' sky culture.\n';
        script += '// ============================================================\n\n';
        
        // Include translation support
        script += '// ============================================================\n';
        script += '// TRANSLATION SUPPORT\n';
        script += '// ============================================================\n';
        script += 'include("i18n.inc");\n\n';
        
        // Culture verification
        script += '// ============================================================\n';
        script += '// CULTURE VERIFICATION\n';
        script += '// ============================================================\n';
        script += 'var currentCulture = core.getSkyCulture();\n';
        script += 'var requiredCulture = "' + cultureId + '";\n';
        script += 'core.debug(tr("Current culture: ") + currentCulture);\n';
        script += 'core.debug(tr("Required culture: ") + requiredCulture);\n\n';
        script += 'if (currentCulture !== requiredCulture) {\n';
        script += '    core.debug(tr("Switching to required sky culture: ") + requiredCulture);\n';
        script += '    core.setSkyCulture(requiredCulture);\n';
        script += '    core.wait(2);\n';
        script += '    core.debug(tr("Culture switched."));\n';
        script += '}\n\n';
        
        // Initial setup
        script += '// ============================================================\n';
        script += '// INITIAL SETUP\n';
        script += '// ============================================================\n';
        script += 'core.clear("starchart");\n\n';
        script += 'GridLinesMgr.setFlagGridlines(false);\n\n';
        script += 'core.setGuiVisible(false);\n\n';
        script += 'StelMovementMgr.setFlagTracking(false);\n\n';
        
        // Save original settings
        script += '// ============================================================\n';
        script += '// SAVE ORIGINAL SETTINGS\n';
        script += '// ============================================================\n';
        script += 'var oldStarsState = StarMgr.getFlagStars();\n';
        script += 'var oldLabelsState = StarMgr.getFlagLabels();\n';
        script += 'var oldLabelsAmount = StarMgr.getLabelsAmount();\n';
        script += 'var oldAdditionalNames = StarMgr.getFlagAdditionalNames();\n';
        script += 'var oldDblStars = StarMgr.getFlagDblStarsDesignation();\n';
        script += 'var oldVarStars = StarMgr.getFlagVarStarsDesignation();\n';
        script += 'var oldHIPDesignation = StarMgr.getFlagHIPDesignation();\n\n';
        script += 'var oldAbsoluteScale = StelSkyDrawer.getAbsoluteStarScale();\n';
        script += 'var oldRelativeScale = StelSkyDrawer.getRelativeStarScale();\n';
        script += 'var oldTwinkleFlag = StelSkyDrawer.getFlagTwinkle();\n';
        script += 'var oldTwinkleAmount = StelSkyDrawer.getTwinkleAmount();\n';
        script += 'var oldSpikyFlag = StelSkyDrawer.getFlagStarSpiky();\n';
        script += 'var oldHaloFlag = StelSkyDrawer.getFlagDrawBigStarHalo();\n';
        script += 'var oldMagLimitFlag = StelSkyDrawer.getFlagStarMagnitudeLimit();\n';
        script += 'var oldCustomMagLimit = StelSkyDrawer.getCustomStarMagnitudeLimit();\n\n';
        script += 'var oldConstLinesState = ConstellationMgr.getFlagLines();\n';
        script += 'var oldConstLabelsState = ConstellationMgr.getFlagLabels();\n';
        script += 'var oldConstArtState = ConstellationMgr.getFlagArt();\n';
        script += 'var oldConstBoundariesState = ConstellationMgr.getFlagBoundaries();\n';
        script += 'var oldAsterLinesState = AsterismMgr.getFlagLines();\n';
        script += 'var oldAsterLabelsState = AsterismMgr.getFlagLabels();\n';
        script += 'var oldLunarState = ConstellationMgr.getFlagLunarSystem();\n\n';
        
        // Disable distractions
        script += '// ============================================================\n';
        script += '// DISABLE DISTRACTING ELEMENTS\n';
        script += '// ============================================================\n';
        script += 'ConstellationMgr.deselectConstellations();\n';
        script += 'ConstellationMgr.setFlagArt(false);\n';
        script += 'ConstellationMgr.setFlagBoundaries(false);\n';
        script += 'ConstellationMgr.setFlagLines(false);\n';
        script += 'ConstellationMgr.setFlagLabels(false);\n\n';
        script += 'AsterismMgr.setFlagLines(false);\n';
        script += 'AsterismMgr.setFlagLabels(false);\n\n';
        script += 'ConstellationMgr.setFlagLunarSystem(false);\n\n';
        script += 'GridLinesMgr.setFlagGridlines(false);\n\n';
        
        // Configure star display
        script += '// ============================================================\n';
        script += '// CONFIGURE STAR DISPLAY\n';
        script += '// ============================================================\n';
        script += 'StarMgr.setFlagStars(true);\n\n';
        script += 'StarMgr.setFlagLabels(' + (showLabels ? 'true' : 'false') + ');\n';
        script += 'StarMgr.setLabelsAmount(' + labelsAmount + ');\n';
        script += 'StarMgr.setFlagAdditionalNames(' + (showAdditionalNames ? 'true' : 'false') + ');\n';
        script += 'StarMgr.setFlagDblStarsDesignation(' + (showDblStars ? 'true' : 'false') + ');\n';
        script += 'StarMgr.setFlagVarStarsDesignation(' + (showVarStars ? 'true' : 'false') + ');\n';
        script += 'StarMgr.setFlagHIPDesignation(' + (showHIPNumbers ? 'true' : 'false') + ');\n\n';
        script += 'StelSkyDrawer.setAbsoluteStarScale(' + absoluteScale + ');\n';
        script += 'StelSkyDrawer.setRelativeStarScale(' + relativeScale + ');\n';
        script += 'StelSkyDrawer.setFlagTwinkle(' + (showTwinkle ? 'true' : 'false') + ');\n';
        script += 'StelSkyDrawer.setTwinkleAmount(' + twinkleAmount + ');\n';
        script += 'StelSkyDrawer.setFlagStarSpiky(' + (showSpiky ? 'true' : 'false') + ');\n';
        script += 'StelSkyDrawer.setFlagDrawBigStarHalo(' + (showHalo ? 'true' : 'false') + ');\n';
        script += 'StelSkyDrawer.setFlagStarMagnitudeLimit(' + (useMagnitudeLimit ? 'true' : 'false') + ');\n';
        script += 'if (' + (useMagnitudeLimit ? 'true' : 'false') + ') {\n';
        script += '    StelSkyDrawer.setCustomStarMagnitudeLimit(' + customMagLimit + ');\n';
        script += '}\n\n';
        script += 'StarMgr.setFontSize(' + fontSize + ');\n\n';
        
        // Welcome message
        script += '// ============================================================\n';
        script += '// WELCOME MESSAGE\n';
        script += '// ============================================================\n';
        script += 'var msg = tr("Welcome to the ");\n';
        script += 'msg += "' + cultureName + '";\n';
        script += 'msg += tr(" stars tour!\\n\\nExploring culturally significant stars.\\n\\nPress Ctrl+T to show toolbar.");\n';
        script += 'var labelId = LabelMgr.labelScreen(msg, 100, 100, false, 16, "#ffaa66");\n';
        script += 'LabelMgr.setLabelShow(labelId, true);\n';
        script += 'core.wait(6);\n';
        script += 'LabelMgr.setLabelShow(labelId, false);\n';
        script += 'LabelMgr.deleteLabel(labelId);\n\n';
        
        // Stars array
        script += '// ============================================================\n';
        script += '// STARS AND CELESTIAL OBJECTS LIST\n';
        script += '// ============================================================\n';
        script += 'var celestialObjects = [\n';
        
				for (var i = 0; i < stars.length; i++) {
						var star = stars[i];
						// Using cleaned star.id (without pre "NAME")
						var objectId = star.id;  // This will be "Mars" instead of "NAME Mars"
						var escapedId = objectId.replace(/"/g, '\\"');
						
						var commentText = '';
						// sutable commented comment (Native , English, ... names)
						if (star.englishName && star.englishName !== objectId && star.englishName !== star.name) {
								commentText = star.englishName;
						} else if (star.nativeName && star.nativeName !== objectId && star.nativeName !== star.name) {
								commentText = star.nativeName;
						} else if (star.name && star.name !== objectId) {
								commentText = star.name;
						}
						
						// Add commented comment to solar system object
						if (star.isSolarSystem) {
								if (commentText) {
										commentText = "Planet: " + commentText;
								} else {
										commentText = "Solar system object";
								}
						}
						
						if (commentText && commentText.length > 60) {
								commentText = commentText.substring(0, 57) + '...';
						}
						
						var line = '    "' + escapedId + '"';
						if (i < stars.length - 1) {
								line += ',';
						}
						if (commentText) {
								line += ' // ' + commentText;
						}
						script += line + '\n';
				}
								
        script += '];\n\n';
        
        // Tour configuration
        script += '// ============================================================\n';
        script += '// TOUR CONFIGURATION\n';
        script += '// ============================================================\n';
        script += 'var DURATION = ' + duration + ';\n';
        script += 'var ZOOM_FOV = ' + zoomFov + ';\n';
        script += 'var OVERVIEW_FOV = ' + overviewFov + ';\n';
        script += 'var SHOW_LABELS = ' + (showLabels ? 'true' : 'false') + ';\n\n';
        
        // Tour execution
        script += '// ============================================================\n';
        script += '// TOUR EXECUTION\n';
        script += '// ============================================================\n';
        script += 'core.debug(tr("Starting stars tour..."));\n';
        script += 'core.debug(tr("Total objects: ") + celestialObjects.length);\n\n';
        
        script += 'for (var i = 0; i < celestialObjects.length; i++) {\n';
        script += '    var objId = celestialObjects[i];\n';
        script += '    var currentNum = i + 1;\n';
        script += '    \n';
        script += '    core.debug(tr("Visiting object") + " " + currentNum + "/" + celestialObjects.length + ": " + objId);\n';
        script += '    \n';
        script += '    // Select and move to object\n';
        script += '    core.selectObjectByName(objId, true);\n';
        script += '    core.moveToObject(objId, 2);\n';
        script += '    core.wait(1);\n';
        script += '    \n';
        script += '    // Zoom in for detailed view\n';
        script += '    StelMovementMgr.zoomTo(ZOOM_FOV, 2);\n';
        script += '    core.wait(1);\n';
        script += '    \n';
        script += '    // Display name on screen if enabled\n';
        script += '    var nameLabel = null;\n';
        script += '    if (SHOW_LABELS) {\n';
        script += '        nameLabel = LabelMgr.labelScreen(objId, 50, 50, false, ' + fontSize + ', "#ffaa66");\n';
        script += '        LabelMgr.setLabelShow(nameLabel, true);\n';
        script += '    }\n';
        script += '    core.wait(DURATION);\n';
        script += '    if (nameLabel !== null) {\n';
        script += '        LabelMgr.deleteLabel(nameLabel);\n';
        script += '    }\n';
        script += '    \n';
        script += '    // Clear selection\n';
        script += '    core.selectObjectByName("", false);\n';
        script += '    \n';
        script += '    // Zoom out for overview\n';
        script += '    StelMovementMgr.zoomTo(OVERVIEW_FOV, 2);\n';
        script += '    core.wait(1);\n';
        script += '}\n\n';
        
        // Reset and cleanup
        script += '// ============================================================\n';
        script += '// RESET AND CLEANUP\n';
        script += '// ============================================================\n';
        script += 'core.debug(tr("Tour completed! Restoring original settings..."));\n\n';
        script += 'core.selectObjectByName("", false);\n\n';
        script += 'StarMgr.setFontSize(12);\n';				
        script += 'StarMgr.setFlagStars(oldStarsState);\n';
        script += 'StarMgr.setFlagLabels(oldLabelsState);\n';
        script += 'StarMgr.setLabelsAmount(oldLabelsAmount);\n';
        script += 'StarMgr.setFlagAdditionalNames(oldAdditionalNames);\n';
        script += 'StarMgr.setFlagDblStarsDesignation(oldDblStars);\n';
        script += 'StarMgr.setFlagVarStarsDesignation(oldVarStars);\n';
        script += 'StarMgr.setFlagHIPDesignation(oldHIPDesignation);\n\n';
        script += 'StelSkyDrawer.setAbsoluteStarScale(oldAbsoluteScale);\n';
        script += 'StelSkyDrawer.setRelativeStarScale(oldRelativeScale);\n';
        script += 'StelSkyDrawer.setFlagTwinkle(oldTwinkleFlag);\n';
        script += 'StelSkyDrawer.setTwinkleAmount(oldTwinkleAmount);\n';
        script += 'StelSkyDrawer.setFlagStarSpiky(oldSpikyFlag);\n';
        script += 'StelSkyDrawer.setFlagDrawBigStarHalo(oldHaloFlag);\n';
        script += 'StelSkyDrawer.setFlagStarMagnitudeLimit(oldMagLimitFlag);\n';
        script += 'StelSkyDrawer.setCustomStarMagnitudeLimit(oldCustomMagLimit);\n\n';
        script += 'ConstellationMgr.setFlagLines(oldConstLinesState);\n';
        script += 'ConstellationMgr.setFlagLabels(oldConstLabelsState);\n';
        script += 'ConstellationMgr.setFlagArt(oldConstArtState);\n';
        script += 'ConstellationMgr.setFlagBoundaries(oldConstBoundariesState);\n';
        script += 'AsterismMgr.setFlagLines(oldAsterLinesState);\n';
        script += 'AsterismMgr.setFlagLabels(oldAsterLabelsState);\n';
        script += 'ConstellationMgr.setFlagLunarSystem(oldLunarState);\n\n';
        script += 'GridLinesMgr.setFlagGridlines(true);\n\n';
        script += 'core.setGuiVisible(true);\n\n';
        script += 'core.clear("natural");\n\n';
        script += 'StelMovementMgr.zoomTo(100, 3);\n';
        script += 'core.moveToAltAzi(40.0, 180.0, 3.0);\n\n';
        script += 'core.debug(tr("Stars tour completed successfully!"));\n';
        
        // Output the script
        var scriptSize = new Blob([script]).size;
        var MAX_SAFE_SIZE = 50 * 1024;
        
        if (scriptSize > MAX_SAFE_SIZE && totalStars > 500) {
            var blob = new Blob([script], { type: 'text/plain;charset=utf-8' });
            var url = URL.createObjectURL(blob);
            var a = document.createElement('a');
            a.href = url;
            a.download = cultureId + '_stars_tour' + (isMultiSelect ? '_selected' : '') + '.ssc';
            document.body.appendChild(a);
            a.click();
            document.body.removeChild(a);
            setTimeout(function() { URL.revokeObjectURL(url); }, 100);
            showTourNotification(_tr("Large stars script saved as file: ") + cultureId + '_stars_tour.ssc');
        } else if (window._stelScriptEditor && typeof window._stelScriptEditor.setEditorContent === 'function') {
            window._stelScriptEditor.setEditorContent(script);
            switchToScriptEditor();
            showTourNotification(_tr("Stars tour script generated and inserted into editor") + 
                (isMultiSelect ? (" (" + totalStars + " selected)") : ""));
        } else {
            copyToClipboard(script);
            showTourNotification(_tr("Script copied to clipboard") + 
                (isMultiSelect ? (" (" + totalStars + " selected)") : ""));
        }
    }

		
		// ========================================================================
    // STARS TOUR TOOLBAR CREATION
    // ========================================================================

		/**
		 * Create the tour control toolbar for stars inside the stars tab.
		 * Provides controls for generating scripts with full customization.
		 * 
		 * Features:
		 * - Generate static script with embedded celestial objects array
		 * - Multi-select mode for picking specific stars/objects
		 * - Duration, zoom, and overview controls
		 * - Label customization (amount, additional names, HIP, double/variable stars)
		 * - Star appearance controls (absolute/relative scale, twinkle, spikes, halo)
		 * - Magnitude limit control
		 * 
		 * @function createStarsTourToolbar
		 */
		function createStarsTourToolbar() {
				console.log("[SkyCulture] createStarsTourToolbar() called");

				// Check if toolbar already exists
				if ($('#stars-tour-controls-container').length > 0) {
						console.log("[SkyCulture] Stars tour toolbar already exists, skipping");
						return;
				}

				// Find the stars tab content
				var $starsTab = $('#patterns-tab-stars');
				if (!$starsTab.length) {
						console.warn("[SkyCulture] Stars tab not found");
						return;
				}

				// Find the container where buttons are displayed
				var $buttonsContainer = $starsTab.find('#stars-buttons-container');
				if (!$buttonsContainer.length) {
						console.warn("[SkyCulture] Stars buttons container not found");
						return;
				}

				// Create toolbar container - NO inline styles
				var $toolbarContainer = $('<div id="stars-tour-controls-container" class="tour-controls-container"></div>');

				// Add section title - NO inline styles
				var $sectionTitle = $('<div class="tour-section-title">' + _tr("Stars Tour Generator") + '</div>');

				// Create inner toolbar with multi-column grid layout - NO inline styles
				var $toolbar = $('<div class="tour-toolbar-grid"></div>');

				// ============================================================
				// COLUMN 1: Script Generation + Timing Controls
				// ============================================================
				var $column1 = $('<div class="tour-column"></div>');

				// Script Generation Row
				var $genRow = $('<div class="tour-row-separator"></div>');
				$starsTourGenerateStaticBtn = $('<button id="btn-generate-stars-static" class="jquerybutton pattern-btn tour-generate-btn" title="' + _tr("Generates script with embedded star/object array") + '">' + _tr("Generate Stars Tour Script") + '</button>');
				$genRow.append($starsTourGenerateStaticBtn);

				// Multi-Select Mode checkbox and counter
				var $multiSelectWrapper = $('<div class="tour-multiselect-orange"></div>');
				$starsMultiSelectMode = $('<input type="checkbox" id="stars-multi-select-mode">');
				$multiSelectWrapper.append($starsMultiSelectMode);
				$multiSelectWrapper.append('<label style="font-weight:bold;cursor:pointer;">' + _tr("Multi-Select") + '</label>');
				$multiSelectWrapper.append('<span class="tour-multiselect-hint">(' + _tr("Pick multiple stars/objects") + ')</span>');

				$starsSelectionCount = $('<span class="tour-selection-count">0 ' + _tr("selected") + '</span>');
				$multiSelectWrapper.append($starsSelectionCount);

				$genRow.append($multiSelectWrapper);
				$column1.append($genRow);

				// Timing Controls Row
				var $timingRow = $('<div class="tour-row"></div>');

				var $durationWrapper = $('<div class="tour-control-group"></div>');
				$durationWrapper.append('<label class="tour-control-label-bold">' + _tr("Duration Per Object:") + '</label>');
				$starsTourDuration = $('<input type="number" id="stars-tour-duration" class="tour-input-number" value="3" min="1" max="15" step="0.5">');
				$durationWrapper.append($starsTourDuration);
				$durationWrapper.append('<span>' + _tr("sec") + '</span>');
				$timingRow.append($durationWrapper);

				var $zoomWrapper = $('<div class="tour-control-group"></div>');
				$zoomWrapper.append('<label class="tour-control-label-bold">' + _tr("Zoom FOV:") + '</label>');
				$starsTourZoomFov = $('<input type="number" id="stars-zoom-fov" class="tour-input-number" value="10" min="5" max="60" step="1">');
				$zoomWrapper.append($starsTourZoomFov);
				$zoomWrapper.append('<span>' + _tr("deg") + '</span>');
				$timingRow.append($zoomWrapper);

				var $overviewWrapper = $('<div class="tour-control-group"></div>');
				$overviewWrapper.append('<label class="tour-control-label-bold">' + _tr("Overview FOV:") + '</label>');
				$starsTourOverviewFov = $('<input type="number" id="stars-overview-fov" class="tour-input-number" value="60" min="20" max="120" step="1">');
				$overviewWrapper.append($starsTourOverviewFov);
				$overviewWrapper.append('<span>' + _tr("deg") + '</span>');
				$timingRow.append($overviewWrapper);

				$column1.append($timingRow);

				// ============================================================
				// COLUMN 2: Labels Controls
				// ============================================================
				var $column2 = $('<div class="tour-column"></div>');

				// STAR LABELS Section
				var $rowLabels = $('<div id="stars-labels-section" class="tour-section-artwork"></div>');
				$rowLabels.append('<div id="stars-labels-header" class="tour-section-header">' + _tr("STAR LABELS") + '</div>');

				var $labelsSubRow = $('<div class="tour-row"></div>');

				var $showLabelsWrapper = $('<div class="tour-control-group"></div>');
				$starsTourShowLabels = $('<input type="checkbox" id="stars-show-labels" checked>');
				$showLabelsWrapper.append($starsTourShowLabels);
				$showLabelsWrapper.append('<label class="tour-control-label">' + _tr("Show Labels") + '</label>');
				$labelsSubRow.append($showLabelsWrapper);

				var $labelsAmountWrapper = $('<div class="tour-control-group"></div>');
				$labelsAmountWrapper.append('<label class="tour-control-label">' + _tr("Labels Amount:") + '</label>');
				$starsTourLabelsAmount = $('<input type="range" id="stars-labels-amount" class="tour-input-range" value="6" min="0" max="10" step="0.1">');
				$labelsAmountWrapper.append($starsTourLabelsAmount);
				var $labelsAmountValue = $('<span id="stars-labels-amount-value" class="tour-range-value">6.0</span>');
				$labelsAmountWrapper.append($labelsAmountValue);
				$labelsSubRow.append($labelsAmountWrapper);

				$rowLabels.append($labelsSubRow);

				// Label type checkboxes
				var $labelTypesRow = $('<div class="tour-row"></div>');

				var $additionalNamesWrapper = $('<div class="tour-control-group"></div>');
				$starsTourShowAdditionalNames = $('<input type="checkbox" id="stars-show-additional-names" checked>');
				$additionalNamesWrapper.append($starsTourShowAdditionalNames);
				$additionalNamesWrapper.append('<label class="tour-control-label">' + _tr("Additional Names") + '</label>');
				$labelTypesRow.append($additionalNamesWrapper);

				var $dblStarsWrapper = $('<div class="tour-control-group"></div>');
				$starsTourShowDblStars = $('<input type="checkbox" id="stars-show-dbl-stars">');
				$dblStarsWrapper.append($starsTourShowDblStars);
				$dblStarsWrapper.append('<label class="tour-control-label">' + _tr("Double Stars") + '</label>');
				$labelTypesRow.append($dblStarsWrapper);

				var $varStarsWrapper = $('<div class="tour-control-group"></div>');
				$starsTourShowVarStars = $('<input type="checkbox" id="stars-show-var-stars">');
				$varStarsWrapper.append($starsTourShowVarStars);
				$varStarsWrapper.append('<label class="tour-control-label">' + _tr("Variable Stars") + '</label>');
				$labelTypesRow.append($varStarsWrapper);

				var $hipNumbersWrapper = $('<div class="tour-control-group"></div>');
				$starsTourShowHIPNumbers = $('<input type="checkbox" id="stars-show-hip-numbers">');
				$hipNumbersWrapper.append($starsTourShowHIPNumbers);
				$hipNumbersWrapper.append('<label class="tour-control-label">' + _tr("HIP Numbers") + '</label>');
				$labelTypesRow.append($hipNumbersWrapper);

				$rowLabels.append($labelTypesRow);

				// Font size
				var $fontRow = $('<div class="tour-row"></div>');

				var $fontWrapper = $('<div class="tour-control-group"></div>');
				$fontWrapper.append('<label class="tour-control-label">' + _tr("Font Size:") + '</label>');
				$starsTourFontSize = $('<input type="number" id="stars-font-size" class="tour-input-number" value="16" min="10" max="30" step="1">');
				$fontWrapper.append($starsTourFontSize);
				$fontWrapper.append('<span>' + _tr("px") + '</span>');
				$fontRow.append($fontWrapper);

				$rowLabels.append($fontRow);

				$column2.append($rowLabels);

				// ============================================================
				// COLUMN 3: Star Appearance Controls
				// ============================================================
				var $column3 = $('<div class="tour-column"></div>');

				// STAR APPEARANCE Section
				var $rowAppearance = $('<div id="stars-appearance-section" class="tour-section-artwork"></div>');
				$rowAppearance.append('<div id="stars-appearance-header" class="tour-section-header">' + _tr("STAR APPEARANCE") + '</div>');

				// Absolute/Relative scales
				var $scaleRow = $('<div class="tour-row"></div>');

				var $absScaleWrapper = $('<div class="tour-control-group"></div>');
				$absScaleWrapper.append('<label class="tour-control-label">' + _tr("Absolute Scale:") + '</label>');
				$starsTourAbsoluteScale = $('<input type="range" id="stars-absolute-scale" class="tour-input-range" value="1.0" min="0.3" max="3.0" step="0.01">');
				$absScaleWrapper.append($starsTourAbsoluteScale);
				var $absScaleValue = $('<span id="stars-absolute-scale-value" class="tour-range-value">1.00</span>');
				$absScaleWrapper.append($absScaleValue);
				$scaleRow.append($absScaleWrapper);

				var $relScaleWrapper = $('<div class="tour-control-group"></div>');
				$relScaleWrapper.append('<label class="tour-control-label">' + _tr("Relative Scale:") + '</label>');
				$starsTourRelativeScale = $('<input type="range" id="stars-relative-scale" class="tour-input-range" value="1.0" min="0.5" max="2.5" step="0.01">');
				$relScaleWrapper.append($starsTourRelativeScale);
				var $relScaleValue = $('<span id="stars-relative-scale-value" class="tour-range-value">1.00</span>');
				$relScaleWrapper.append($relScaleValue);
				$scaleRow.append($relScaleWrapper);

				$rowAppearance.append($scaleRow);

				// Twinkle controls
				var $twinkleRow = $('<div class="tour-row"></div>');

				var $showTwinkleWrapper = $('<div class="tour-control-group"></div>');
				$starsTourShowTwinkle = $('<input type="checkbox" id="stars-show-twinkle" checked>');
				$showTwinkleWrapper.append($starsTourShowTwinkle);
				$showTwinkleWrapper.append('<label class="tour-control-label">' + _tr("Twinkle") + '</label>');
				$twinkleRow.append($showTwinkleWrapper);

				var $twinkleAmountWrapper = $('<div class="tour-control-group"></div>');
				$twinkleAmountWrapper.append('<label class="tour-control-label">' + _tr("Twinkle Amount:") + '</label>');
				$starsTourTwinkleAmount = $('<input type="range" id="stars-twinkle-amount" class="tour-input-range" value="0.3" min="0" max="1.5" step="0.01">');
				$twinkleAmountWrapper.append($starsTourTwinkleAmount);
				var $twinkleAmountValue = $('<span id="stars-twinkle-amount-value" class="tour-range-value">0.30</span>');
				$twinkleAmountWrapper.append($twinkleAmountValue);
				$twinkleRow.append($twinkleAmountWrapper);

				$rowAppearance.append($twinkleRow);

				// Spiky stars and halo
				var $effectRow = $('<div class="tour-row"></div>');

				var $showSpikyWrapper = $('<div class="tour-control-group"></div>');
				$starsTourShowSpiky = $('<input type="checkbox" id="stars-show-spiky">');
				$showSpikyWrapper.append($starsTourShowSpiky);
				$showSpikyWrapper.append('<label class="tour-control-label">' + _tr("Spiky Stars") + '</label>');
				$effectRow.append($showSpikyWrapper);

				var $showHaloWrapper = $('<div class="tour-control-group"></div>');
				$starsTourShowHalo = $('<input type="checkbox" id="stars-show-halo" checked>');
				$showHaloWrapper.append($starsTourShowHalo);
				$showHaloWrapper.append('<label class="tour-control-label">' + _tr("Star Halos") + '</label>');
				$effectRow.append($showHaloWrapper);

				$rowAppearance.append($effectRow);

				// Magnitude limit
				var $magLimitRow = $('<div class="tour-row"></div>');

				var $useMagLimitWrapper = $('<div class="tour-control-group"></div>');
				$starsTourUseMagLimit = $('<input type="checkbox" id="stars-use-mag-limit">');
				$useMagLimitWrapper.append($starsTourUseMagLimit);
				$useMagLimitWrapper.append('<label class="tour-control-label">' + _tr("Use Magnitude Limit") + '</label>');
				$magLimitRow.append($useMagLimitWrapper);

				var $magLimitValueWrapper = $('<div class="tour-control-group"></div>');
				$magLimitValueWrapper.append('<label class="tour-control-label">' + _tr("Limit:") + '</label>');
				$starsTourMagLimit = $('<input type="range" id="stars-mag-limit" class="tour-input-range" value="8.0" min="1" max="10" step="0.1">');
				$magLimitValueWrapper.append($starsTourMagLimit);
				var $magLimitValue = $('<span id="stars-mag-limit-value" class="tour-range-value">8.0</span>');
				$magLimitValueWrapper.append($magLimitValue);
				$magLimitRow.append($magLimitValueWrapper);

				$rowAppearance.append($magLimitRow);

				$column3.append($rowAppearance);

				// ============================================================
				// ASSEMBLE ALL COLUMNS INTO TOOLBAR
				// ============================================================
				$toolbar.append($column1);
				$toolbar.append($column2);
				$toolbar.append($column3);

				$toolbarContainer.append($sectionTitle);
				$toolbarContainer.append($toolbar);

				// Insert toolbar AFTER the buttons container (at the bottom of the tab)
				$buttonsContainer.after($toolbarContainer);

				// ============================================================
				// BIND UI EVENT HANDLERS
				// ============================================================

				// Update value displays for sliders
				$starsTourLabelsAmount.on('input', function() {
						var val = parseFloat($(this).val()).toFixed(1);
						$('#stars-labels-amount-value').text(val);
				});

				$starsTourAbsoluteScale.on('input', function() {
						var val = parseFloat($(this).val()).toFixed(2);
						$('#stars-absolute-scale-value').text(val);
				});

				$starsTourRelativeScale.on('input', function() {
						var val = parseFloat($(this).val()).toFixed(2);
						$('#stars-relative-scale-value').text(val);
				});

				$starsTourTwinkleAmount.on('input', function() {
						var val = parseFloat($(this).val()).toFixed(2);
						$('#stars-twinkle-amount-value').text(val);
				});

				$starsTourMagLimit.on('input', function() {
						var val = parseFloat($(this).val()).toFixed(1);
						$('#stars-mag-limit-value').text(val);
				});

				// Bind generate button
				$starsTourGenerateStaticBtn.on('click', generateStarsTourScriptStatic);

				// Enable/disable magnitude limit slider when checkbox toggles
				$starsTourUseMagLimit.on('change', function() {
						var isEnabled = $(this).is(':checked');
						$starsTourMagLimit.prop('disabled', !isEnabled);
						$('#stars-mag-limit-value').css('opacity', isEnabled ? 1 : 0.5);
				});

				// Initialize magnitude limit slider state
				$starsTourMagLimit.prop('disabled', !$starsTourUseMagLimit.is(':checked'));

				// Bind multi-select mode change event
				if ($starsMultiSelectMode) {
						$starsMultiSelectMode.on('change', function() {
								isStarsMultiSelectModeActive = $(this).is(':checked');

								if (isStarsMultiSelectModeActive) {
										console.log("[SkyCulture] Entering multi-select mode for stars");
										selectedPattern = null;
										selectedPatternId = null;
										clearStarsMultiSelection();
										updateStarsButtonsBehavior();
								} else {
										console.log("[SkyCulture] Exiting multi-select mode for stars");
										clearStarsMultiSelection();
										selectedStarIds.clear();
										updateStarsButtonsBehavior();
										updateStarsSelectionCount();
								}
						});
				}

				console.log("[SkyCulture] Stars tour toolbar created successfully");
		}

		
    // ========================================================================
    // STARS MULTI-SELECT FUNCTIONS
    // ========================================================================

    /**
     * Update the selection count display for stars in multi-select mode.
     * Updates the counter badge and enables/disables the generate button
     * based on whether any stars are selected.
     */
    function updateStarsSelectionCount() {
        var count = selectedStarIds.size;
        if ($starsSelectionCount && $starsSelectionCount.length) {
            $starsSelectionCount.text(count + ' ' + (count === 1 ? _tr("selected") : _tr("selected")));
        }
        
        // Update generate button state (disable if nothing selected)
        if ($starsTourGenerateStaticBtn && $starsTourGenerateStaticBtn.length) {
            $starsTourGenerateStaticBtn.prop('disabled', count === 0 && isStarsMultiSelectModeActive);
        }
    }

    /**
     * Clear all selections in multi-select mode for stars.
     * Removes all selected IDs and resets button styles.
     */
    function clearStarsMultiSelection() {
        selectedStarIds.clear();
        if ($starsContainer && $starsContainer.length) {
            // Clear styles without re-rendering
            $starsContainer.find('.pattern-btn').removeClass('selected-star');
            $starsContainer.find('.pattern-btn').css('background', '');
            $starsContainer.find('.pattern-btn').css('border-color', '');
            $starsContainer.find('.pattern-btn').css('color', '');
            $starsContainer.find('.pattern-btn').removeClass('active');
        }
        updateStarsSelectionCount();
    }

    /**
     * Get selected star objects for multi-select mode.
     * 
     * @returns {Array|null} Array of selected star objects or null if none selected
     */
    function getSelectedStars() {
        if (!isStarsMultiSelectModeActive || selectedStarIds.size === 0) {
            return null;
        }
        
        var selected = [];
        var allStars = currentPatternsData.stars;
        
        if (!allStars) return null;
        
        for (var i = 0; i < allStars.length; i++) {
            var star = allStars[i];
            if (selectedStarIds.has(star.id)) {
                selected.push(star);
            }
        }
        
        return selected;
    }

    /**
     * Update star button behavior based on multi-select mode.
     * 
     * When multi-select mode is ON:
     * - Re-renders buttons without the original click handler
     * - Attaches custom click handler that toggles selection state
     * - Selected buttons get special styling
     * 
     * When multi-select mode is OFF:
     * - Re-renders buttons with original navigation handler
     * - Clears any selection state
     */
    function updateStarsButtonsBehavior() {
        if (!$starsContainer || !$starsContainer.length) return;
        
        if (isStarsMultiSelectModeActive) {
            // Get current patterns data
            var currentPatterns = currentPatternsData.stars;
            if (currentPatterns && currentPatterns.length) {
                // Re-render with handler disabled
                renderButtons($starsContainer, currentPatterns, "stars", null, true);
                
                // Then attach multi-select behavior
                $starsContainer.find('.pattern-btn').off('click.stars').on('click.stars', function(e) {
                    e.preventDefault();
                    e.stopPropagation();
                    
                    var $btn = $(this);
                    var patternId = $btn.data('pattern-id');
                    var patternName = $btn.data('pattern-name');
                    
                    if (!patternId) return false;
                    
                    if (selectedStarIds.has(patternId)) {
                        // Deselect: remove from set and clear styling
                        selectedStarIds.delete(patternId);
                        $btn.removeClass('selected-star');
                        $btn.css('background', '');
                        $btn.css('border-color', '');
                        $btn.css('color', '');
                        $btn.removeClass('active');
                        console.log("[SkyCulture] Star deselected:", patternName, "ID:", patternId);
                    } else {
                        // Select: add to set and apply selected styling
                        selectedStarIds.add(patternId);
                        $btn.addClass('selected-star');
                        $btn.css('background', 'linear-gradient(#DCDBDA, #8A8C8E)');
                        $btn.css('border-color', '#000000');
                        $btn.css('color', '#000000');
                        console.log("[SkyCulture] Star selected:", patternName, "ID:", patternId);
                    }
                    
                    updateStarsSelectionCount();
                    return false;
                });
            }
        } else {
            // Normal mode: re-render with original handler
            var currentPatterns = currentPatternsData.stars;
            if (currentPatterns && currentPatterns.length) {
                renderButtons($starsContainer, currentPatterns, "stars", 
                    function(patternName, patternType, searchName, patternId) {
                        stelUtils.goToObject(patternId, DEFAULT_FOV.star, 'star', currentCultureRawData);
                        updateAllButtonStates();
                    },
                    false  // Handler enabled
                );
            }
            
            // Clear any selected star highlight from previous multi-select
            clearStarsMultiSelection();
        }
    }

    // =====================================================================
    // 15. PUBLIC API
    // =====================================================================

    return {
        // Initialization
        init: init,
        refresh: refresh,
        
        // Culture management
        setCulture: setSkyCulture,
        getCurrentCulture: function() { return currentCultureId; },
        getAvailableCultures: function() { return availableCultures.slice(); },
        
        // Pattern selection
        getSelectedPattern: function() { return selectedPattern; },
        getSelectedPatternId: function() { return selectedPatternId; },
        
        // Data refresh
        refreshData: function() { loadAllCultureData(); },
        
        // Synchronization control
        isSyncEnabled: function() { return syncEnabled; },
        setSyncEnabled: function(enabled) { syncEnabled = enabled; },
        
        // Language
        getCurrentLanguage: getCurrentLanguage
    };
});