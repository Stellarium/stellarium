/* jshint expr: true */
/**
 * PS4/PS5 Controller Module for Stellarium Remote Control
 * 
 * This module provides comprehensive gamepad support for Stellarium,
 * allowing users to control the planetarium using PlayStation,
 * Xbox, and other compatible controllers.
 * 
 * Features:
 * - Multi-platform support (Windows, Linux, macOS)
 * - Automatic device detection from SDL database
 * - Button mapping customization
 * - Joystick calibration
 * - Real-time FOV control
 * - Tab navigation
 * - Stellarium actions integration
 * 
 * @module gpcontroller
 * @requires jquery
 * @requires settings
 * @requires api/remotecontrol
 * @requires api/viewcontrol
 * @requires api/actions
 * @requires jquery-ui
 * 
 * @author Stellarium Contributors
 * @license GPLv2+
 * @version 1.0.0
 */

define(["jquery", "settings", "api/remotecontrol", "api/viewcontrol", "api/actions", "jquery-ui"], 
function($, settings, rc, viewcontrol, actions) {
    "use strict";
    
    // =====================================================================
    // SECTION 1: GAMEPAD DATABASE (from SDL)
    // =====================================================================
    // This database is compiled from the SDL (Simple DirectMedia Layer)
    // game controller database. It includes mappings for most common
    // controllers across different operating systems.
    // =====================================================================
    
    /**
     * Comprehensive Gamepad Database based on SDL mappings
     * Contains mappings for PlayStation, Xbox, Nintendo, Logitech, and generic controllers
     * @constant {Object}
     */
    const GAMEPAD_DATABASE = {
        // ========== Sony PlayStation 4 (CUH-ZCT1E) ==========
        'sony_ps4_cuh_zct1e': {
            id: 'sony_ps4_cuh_zct1e',
            name: 'Sony PlayStation 4 Controller (CUH-ZCT1E)',
            sdl_id: '030000004c050000c405000000000000',
            vendor_id: '054c',
            product_id: '05c4',
            patterns: ['054c-05c4', 'sony', 'playstation', 'dualshock', 'wireless controller'],
            axes: { leftX: 0, leftY: 1, rightX: 2, rightY: 3, l2: 5, r2: 4 },
            buttons: { cross: 0, circle: 1, square: 2, triangle: 3, l1: 4, r1: 5, l2: 6, r2: 7, share: 8, options: 9, l3: 10, r3: 11, ps: 16, touchpad: 17 },
            dpad: { up: 12, down: 13, left: 14, right: 15 },
            buttonCount: 17
        },
        
        // ========== Sony PlayStation 4 (v2) ==========
        'sony_ps4_v2': {
            id: 'sony_ps4_v2',
            name: 'Sony PlayStation 4 Controller (v2)',
            sdl_id: '030000004c050000cc09000000000000',
            vendor_id: '054c',
            product_id: '09cc',
            patterns: ['054c-09cc', 'sony', 'playstation', 'dualshock'],
            axes: { leftX: 0, leftY: 1, rightX: 2, rightY: 3, l2: 5, r2: 4 },
            buttons: { cross: 0, circle: 1, square: 2, triangle: 3, l1: 4, r1: 5, l2: 6, r2: 7, share: 8, options: 9, l3: 10, r3: 11, ps: 16, touchpad: 17 },
            dpad: { up: 12, down: 13, left: 14, right: 15 },
            buttonCount: 17
        },
        
        // ========== Sony DualSense (PS5) ==========
        'sony_ps5_dualsense': {
            id: 'sony_ps5_dualsense',
            name: 'Sony PlayStation 5 DualSense Controller',
            sdl_id: '030000004c050000e60c000000000000',
            vendor_id: '054c',
            product_id: '0ce6',
            patterns: ['054c-0ce6', 'sony', 'playstation', 'dualsense', 'ps5'],
            axes: { leftX: 0, leftY: 1, rightX: 2, rightY: 3, l2: 3, r2: 4 },
            buttons: { cross: 1, circle: 2, square: 0, triangle: 3, l1: 4, r1: 5, l2: 6, r2: 7, share: 8, options: 9, l3: 10, r3: 11, ps: 16,	touchpad: 17 },
            dpad: { up: 12, down: 13, left: 14, right: 15 },
            buttonCount: 17
        },
        
        // ========== Sony PlayStation 3 ==========
        'sony_ps3': {
            id: 'sony_ps3',
            name: 'Sony PlayStation 3 Controller',
            sdl_id: '030000004c0500006802000000000000',
            vendor_id: '054c',
            product_id: '0268',
            patterns: ['054c-0268', 'sony', 'playstation', 'ps3'],
            axes: { leftX: 0, leftY: 1, rightX: 2, rightY: 3, l2: 12, r2: 13 },
            buttons: { cross: 13, circle: 14, square: 15, triangle: 12, l1: 10, r1: 11, l2: 8, r2: 9, select: 0, start: 3, l3: 1, r3: 2, ps: 16 },
            dpad: 'axes',
            buttonCount: 17
        },
        
        // ========== Microsoft Xbox 360 ==========
        'microsoft_xbox_360': {
            id: 'microsoft_xbox_360',
            name: 'Microsoft Xbox 360 Controller',
            sdl_id: '030000005e0400008e02000000000000',
            vendor_id: '045e',
            product_id: '028e',
            patterns: ['045e-028e', 'microsoft', 'xbox', '360'],
            axes: { leftX: 0, leftY: 1, rightX: 2, rightY: 3, l2: 4, r2: 5 },
            buttons: { a: 0, b: 1, x: 2, y: 3, lb: 4, rb: 5, back: 6, start: 7, guide: 8, l3: 9, r3: 10 },
            dpad: { up: 11, down: 12, left: 13, right: 14 },
            buttonCount: 15
        },
        
        // ========== Microsoft Xbox One ==========
        'microsoft_xbox_one': {
            id: 'microsoft_xbox_one',
            name: 'Microsoft Xbox One Controller',
            sdl_id: '030000005e040000d102000000000000',
            vendor_id: '045e',
            product_id: '02d1',
            patterns: ['045e-02d1', 'microsoft', 'xbox', 'one'],
            axes: { leftX: 0, leftY: 1, rightX: 2, rightY: 3, l2: 4, r2: 5 },
            buttons: { a: 0, b: 1, x: 2, y: 3, lb: 4, rb: 5, back: 6, start: 7, guide: 8, l3: 9, r3: 10 },
            dpad: { up: 11, down: 12, left: 13, right: 14 },
            buttonCount: 15
        },
        
        // ========== Microsoft Xbox Series X|S ==========
        'microsoft_xbox_series': {
            id: 'microsoft_xbox_series',
            name: 'Microsoft Xbox Series X|S Controller',
            sdl_id: '030000005e040000b102000000000000',
            vendor_id: '045e',
            product_id: '02b1',
            patterns: ['045e-02b1', 'microsoft', 'xbox', 'series'],
            axes: { leftX: 0, leftY: 1, rightX: 2, rightY: 3, l2: 4, r2: 5 },
            buttons: { a: 0, b: 1, x: 2, y: 3, lb: 4, rb: 5, back: 6, start: 7, guide: 8, l3: 9, r3: 10 },
            dpad: { up: 11, down: 12, left: 13, right: 14 },
            buttonCount: 15
        },
        
        // ========== Nintendo Switch Pro Controller ==========
        'nintendo_switch_pro': {
            id: 'nintendo_switch_pro',
            name: 'Nintendo Switch Pro Controller',
            sdl_id: '030000007e0500000920000000000000',
            vendor_id: '057e',
            product_id: '2009',
            patterns: ['057e-2009', 'nintendo', 'switch', 'pro'],
            axes: { leftX: 0, leftY: 1, rightX: 2, rightY: 3 },
            buttons: { a: 0, b: 1, x: 2, y: 3, l: 4, r: 5, zl: 6, zr: 7, minus: 8, plus: 9, l3: 10, r3: 11, home: 12, capture: 13 },
            dpad: { up: 14, down: 15, left: 16, right: 17 },
            buttonCount: 18
        },
        
        // ========== Nintendo Switch Joy-Con (Left) ==========
        'nintendo_joycon_left': {
            id: 'nintendo_joycon_left',
            name: 'Nintendo Switch Joy-Con (Left)',
            sdl_id: '030000007e0500000620000000000000',
            vendor_id: '057e',
            product_id: '2006',
            patterns: ['057e-2006', 'nintendo', 'switch', 'joycon', 'left'],
            axes: { leftX: 0, leftY: 1 },
            buttons: { left: 0, down: 1, up: 2, right: 3, sl: 4, sr: 5, minus: 6, l: 7, zl: 8, l3: 9 },
            dpad: 'buttons',
            buttonCount: 10
        },
        
        // ========== Nintendo Switch Joy-Con (Right) ==========
        'nintendo_joycon_right': {
            id: 'nintendo_joycon_right',
            name: 'Nintendo Switch Joy-Con (Right)',
            sdl_id: '030000007e0500000720000000000000',
            vendor_id: '057e',
            product_id: '2007',
            patterns: ['057e-2007', 'nintendo', 'switch', 'joycon', 'right'],
            axes: { rightX: 0, rightY: 1 },
            buttons: { a: 0, x: 1, b: 2, y: 3, sl: 4, sr: 5, plus: 6, r: 7, zr: 8, r3: 9, home: 10 },
            dpad: 'buttons',
            buttonCount: 11
        },
        
        // ========== Logitech F310 ==========
        'logitech_f310': {
            id: 'logitech_f310',
            name: 'Logitech F310 Gamepad',
            sdl_id: '030000006d04000018c2000000000000',
            vendor_id: '046d',
            product_id: 'c218',
            patterns: ['046d-c218', 'logitech', 'f310'],
            axes: { leftX: 0, leftY: 1, rightX: 2, rightY: 3 },
            buttons: { a: 0, b: 1, x: 2, y: 3, lb: 4, rb: 5, back: 6, start: 7, l3: 8, r3: 9 },
            dpad: { up: 10, down: 11, left: 12, right: 13 },
            buttonCount: 14
        },
        
        // ========== Logitech F710 ==========
        'logitech_f710': {
            id: 'logitech_f710',
            name: 'Logitech F710 Wireless Gamepad',
            sdl_id: '030000006d040000c219000000000000',
            vendor_id: '046d',
            product_id: 'c219',
            patterns: ['046d-c219', 'logitech', 'f710'],
            axes: { leftX: 0, leftY: 1, rightX: 2, rightY: 3 },
            buttons: { a: 0, b: 1, x: 2, y: 3, lb: 4, rb: 5, back: 6, start: 7, l3: 8, r3: 9 },
            dpad: { up: 10, down: 11, left: 12, right: 13 },
            buttonCount: 14
        },
        
        // ========== Logitech G29 Racing Wheel ==========
        'logitech_g29': {
            id: 'logitech_g29',
            name: 'Logitech G29 Racing Wheel',
            sdl_id: '030000006d0400002683000000000000',
            vendor_id: '046d',
            product_id: 'c268',
            patterns: ['046d-c268', 'logitech', 'g29', 'wheel'],
            axes: { wheel: 0, throttle: 2, brake: 3, clutch: 4 },
            buttons: { cross: 0, circle: 1, square: 2, triangle: 3, l1: 4, r1: 5, l2: 6, r2: 7, share: 8, options: 9, l3: 10, r3: 11, ps: 12 },
            buttonCount: 13
        },
        
        // ========== 8BitDo SN30 Pro ==========
        '8bitdo_sn30_pro': {
            id: '8bitdo_sn30_pro',
            name: '8BitDo SN30 Pro Controller',
            sdl_id: '03000000202800000900000000000000',
            vendor_id: '2dc8',
            product_id: '0009',
            patterns: ['2dc8-0009', '8bitdo', 'sn30'],
            axes: { leftX: 0, leftY: 1, rightX: 2, rightY: 3 },
            buttons: { a: 0, b: 1, x: 2, y: 3, l1: 4, r1: 5, l2: 6, r2: 7, select: 8, start: 9, l3: 10, r3: 11 },
            dpad: { up: 12, down: 13, left: 14, right: 15 },
            buttonCount: 16
        },
        
        // ========== 8BitDo Pro 2 ==========
        '8bitdo_pro2': {
            id: '8bitdo_pro2',
            name: '8BitDo Pro 2 Controller',
            sdl_id: '03000000202800005001000000000000',
            vendor_id: '2dc8',
            product_id: '0150',
            patterns: ['2dc8-0150', '8bitdo', 'pro2'],
            axes: { leftX: 0, leftY: 1, rightX: 2, rightY: 5 },
            buttons: { a: 0, b: 1, x: 2, y: 3, l1: 4, r1: 5, l2: 6, r2: 7, select: 8, start: 9, l3: 10, r3: 11 },
            dpad: { up: 12, down: 13, left: 14, right: 15 },
            buttonCount: 16
        },
        
        // ========== Razer Raiju ==========
        'razer_raiju': {
            id: 'razer_raiju',
            name: 'Razer Raiju Mobile Controller',
            sdl_id: '03000000321500000010000000000000',
            vendor_id: '1532',
            product_id: '1000',
            patterns: ['1532-1000', 'razer', 'raiju'],
            axes: { leftX: 0, leftY: 1, rightX: 2, rightY: 5 },
            buttons: { cross: 0, circle: 1, square: 2, triangle: 3, l1: 4, r1: 5, l2: 6, r2: 7, share: 8, options: 9, l3: 10, r3: 11, ps: 12, m1: 13, m2: 14, m3: 15, m4: 16 },
            buttonCount: 17
        },
        
        // ========== Razer Wolverine ==========
        'razer_wolverine': {
            id: 'razer_wolverine',
            name: 'Razer Wolverine Tournament Controller',
            sdl_id: '03000000321500000e07000000000000',
            vendor_id: '1532',
            product_id: '070e',
            patterns: ['1532-070e', 'razer', 'wolverine'],
            axes: { leftX: 0, leftY: 1, rightX: 2, rightY: 3 },
            buttons: { a: 0, b: 1, x: 2, y: 3, lb: 4, rb: 5, lt: 6, rt: 7, back: 8, start: 9, l3: 10, r3: 11 },
            buttonCount: 12
        },
        
        // ========== Steam Controller ==========
        'steam_controller': {
            id: 'steam_controller',
            name: 'Steam Controller',
            sdl_id: '03000000de280000ff11000000000000',
            vendor_id: '28de',
            product_id: '11ff',
            patterns: ['28de-11ff', 'steam'],
            axes: { leftX: 0, leftY: 1, rightX: 2, rightY: 3, l2: 4, r2: 5 },
            buttons: { a: 0, b: 1, x: 2, y: 3, lb: 4, rb: 5, back: 6, start: 7, guide: 8, l3: 9, r3: 10, left_touch: 11, right_touch: 12 },
            buttonCount: 13
        },
        
        // ========== Steam Deck ==========
        'steam_deck': {
            id: 'steam_deck',
            name: 'Steam Deck Controller',
            sdl_id: '03000000de280000024d000000000000',
            vendor_id: '28de',
            product_id: '4d02',
            patterns: ['28de-4d02', 'steam', 'deck'],
            axes: { leftX: 0, leftY: 1, rightX: 2, rightY: 3, l2: 4, r2: 5 },
            buttons: { a: 0, b: 1, x: 2, y: 3, l1: 4, r1: 5, l2: 6, r2: 7, select: 8, start: 9, l3: 10, r3: 11, steam: 12, quick: 13 },
            buttonCount: 14
        },
        
        // ========== Generic USB Gamepad ==========
        'generic_usb': {
            id: 'generic_usb',
            name: 'Generic USB Gamepad',
            patterns: ['usb', 'gamepad'],
            axes: { leftX: 0, leftY: 1, rightX: 2, rightY: 3 },
            buttons: { a: 0, b: 1, x: 2, y: 3, l1: 4, r1: 5, l2: 6, r2: 7, select: 8, start: 9, l3: 10, r3: 11 },
            buttonCount: 12
        },
        
        // ========== Generic Bluetooth Gamepad ==========
        'generic_bluetooth': {
            id: 'generic_bluetooth',
            name: 'Generic Bluetooth Gamepad',
            patterns: ['bluetooth', 'ble'],
            axes: { leftX: 0, leftY: 1, rightX: 2,
						// rightY: 5  // SDL standard
						rightY: 3 // hope thisworks fine you can change it for your compatiplity depends on OS palotform + browser 
						},
            buttons: { a: 0, b: 1, x: 2, y: 3, l1: 4, r1: 5, l2: 6, r2: 7, select: 8, start: 9, l3: 10, r3: 11 },
            buttonCount: 12
        }
    };

    /**
     * Detect the current operating system
     * @returns {string} Operating system identifier: 'windows', 'linux', 'mac', or 'unknown'
     */
    function detectOS() {
        const platform = window.navigator.platform;
        
        if (/Mac|iPod|iPhone|iPad/.test(platform)) return 'mac';
        if (/Win/.test(platform)) return 'windows';
        if (/Linux/.test(platform)) return 'linux';
        return 'unknown';
    }

    /**
     * Adjust axis mappings based on operating system
     * Different OSes may report axes in different orders
     * 
     * @param {Object} baseAxes - Base axis mapping from database
     * @param {string} os - Operating system
     * @param {string} deviceId - Device identifier
     * @returns {Object} Adjusted axis mapping for the current OS
     */
    function adjustAxesForOS(baseAxes, os, deviceId) {
        const axes = { ...baseAxes };
        
        // Special adjustments for PlayStation controllers
        if (deviceId.includes('ps4') || deviceId.includes('ps5')) {
            switch(os) {
                case 'linux':
                    axes.rightY = 3;  // Linux often uses axis 3 for right Y
                    break;
                case 'mac':
                    axes.rightY = 4;  // macOS often uses axis 4 for right Y
                    break;
                // Windows typically uses axis 5 (default)
            }
        }
        
        // Special adjustments for Xbox controllers
        if (deviceId.includes('xbox')) {
            switch(os) {
                case 'linux':
                    axes.rightY = 4;  // Linux may use axis 4 for right Y
                    break;
            }
        }
        
        return axes;
    }

    /**
     * Extract USB vendor and product IDs from gamepad ID string
     * Gamepad IDs often contain strings like "054c-05c4" or "vendor: 054c product: 05c4"
     * 
     * @param {string} id - Gamepad ID string
     * @returns {Object|null} Object with vendor_id and product_id, or null if not found
     */
    function extractUSBIds(id) {
        const patterns = [
            /([0-9a-f]{4})-([0-9a-f]{4})/i,           // Format: 054c-05c4
            /vendor:?\s*([0-9a-f]{4}).*product:?\s*([0-9a-f]{4})/i, // Format: vendor: 054c product: 05c4
            /vid[_-]?([0-9a-f]{4}).*pid[_-]?([0-9a-f]{4})/i        // Format: vid_054c pid_05c4
        ];
        
        for (const pattern of patterns) {
            const match = id.match(pattern);
            if (match) {
                return {
                    vendor_id: match[1].toLowerCase(),
                    product_id: match[2].toLowerCase()
                };
            }
        }
        return null;
    }

    /**
     * Detect gamepad type from its ID string using the comprehensive database
     * 
     * @param {string} id - Gamepad ID string from the browser API
     * @returns {Object} Gamepad information object with mappings
     */
    function detectGamepadType(id) {
        const id_lower = id.toLowerCase();
        const os = detectOS();
        const usbIds = extractUSBIds(id);
        
        console.log("=== Gamepad Detection ===");
        console.log("Full ID:", id);
        console.log("OS:", os);
        if (usbIds) {
            console.log("USB IDs:", usbIds.vendor_id + ":" + usbIds.product_id);
        }
        
        // First, try to match by USB IDs (most accurate)
        if (usbIds) {
            for (const [key, device] of Object.entries(GAMEPAD_DATABASE)) {
                if (device.vendor_id === usbIds.vendor_id && 
                    device.product_id === usbIds.product_id) {
                    console.log("✓ Exact USB ID match:", device.name);
                    return {
                        ...device,
                        axes: adjustAxesForOS(device.axes, os, device.id)
                    };
                }
            }
        }
        
        // Second, try pattern matching
        for (const [key, device] of Object.entries(GAMEPAD_DATABASE)) {
            if (device.patterns) {
                for (const pattern of device.patterns) {
                    if (id_lower.includes(pattern.toLowerCase())) {
                        console.log("✓ Pattern match:", device.name, "via", pattern);
                        return {
                            ...device,
                            axes: adjustAxesForOS(device.axes, os, device.id)
                        };
                    }
                }
            }
        }
        
        // Fallback to generic based on connection type
        console.log("No match found, using generic mapping");
        if (id_lower.includes('bluetooth') || id_lower.includes('bt')) {
            return GAMEPAD_DATABASE.generic_bluetooth;
        }
        return GAMEPAD_DATABASE.generic_usb;
    }
    
    // =====================================================================
    // SECTION 2: CONSTANTS AND DEFAULT MAPPINGS
    // =====================================================================
    
    /**
     * Standard PlayStation button indices
     * These constants map to the standard PS4/PS5 button layout
     * @constant {Object}
     */
    const PS_BUTTONS = {
        CROSS: 0,
        CIRCLE: 1,
        SQUARE: 2,
        TRIANGLE: 3,
        L1: 4,
        R1: 5,
        L2: 6,
        R2: 7,
        SHARE: 8,
        OPTIONS: 9,
        L3: 10,
        R3: 11,
        DPAD_UP: 12,
        DPAD_DOWN: 13,
        DPAD_LEFT: 14,
        DPAD_RIGHT: 15,
        PS: 16,
        TOUCHPAD: 17
    };
    
    /**
     * Human-readable button names for display
     * @constant {Object}
     */
    const BUTTON_NAMES = {
        [PS_BUTTONS.CROSS]: "✕ Cross",
        [PS_BUTTONS.CIRCLE]: "◯ Circle",
        [PS_BUTTONS.SQUARE]: "□ Square",
        [PS_BUTTONS.TRIANGLE]: "△ Triangle",
        [PS_BUTTONS.L1]: "L1",
        [PS_BUTTONS.R1]: "R1",
        [PS_BUTTONS.L2]: "L2",
        [PS_BUTTONS.R2]: "R2",
        [PS_BUTTONS.SHARE]: "Share",
        [PS_BUTTONS.OPTIONS]: "Options",
        [PS_BUTTONS.L3]: "L3",
        [PS_BUTTONS.R3]: "R3",
        [PS_BUTTONS.PS]: "PS",
        [PS_BUTTONS.TOUCHPAD]: "Touchpad",
        12: "D-Pad Up",
        13: "D-Pad Down",
        14: "D-Pad Left",
        15: "D-Pad Right"
    };
    
    /**
     * Available Stellarium actions that can be mapped to buttons
     * @constant {Object}
     */
    const AVAILABLE_ACTIONS = {
        // View actions
        "actionGoto_Selected_Object": "Center on selected object",
        "actionSet_Tracking": "Toggle tracking",
        "actionSelect_Current_Object": "Select object at center",
        
        // Constellation actions
        "actionShow_Constellation_Lines": "Toggle constellation lines",
        "actionShow_Constellation_Labels": "Toggle constellation labels",
        "actionShow_Constellation_Art": "Toggle constellation art",
        "actionShow_Constellation_Boundaries": "Toggle constellation boundaries",
        
        // Grid actions
        "actionShow_Equatorial_Grid": "Toggle equatorial grid",
        "actionShow_Azimuthal_Grid": "Toggle azimuthal grid",
        "actionShow_Galactic_Grid": "Toggle galactic grid",
        
        // Display actions
        "actionShow_Atmosphere": "Toggle atmosphere",
        "actionShow_Ground": "Toggle ground",
        "actionShow_Fog": "Toggle fog",
        "actionShow_Nebulas": "Toggle deep-sky objects",
        "actionShow_Planets_Labels": "Toggle planet labels",
        "actionShow_Cardinal_Points": "Toggle cardinal points",
        "actionShow_Night_Mode": "Toggle night mode",
        
        // Time actions
        "actionPause_Time": "Pause time",
        "actionSet_Time_Now": "Set time to now",
        "actionIncrease_Time_Speed": "Increase time speed",
        "actionDecrease_Time_Speed": "Decrease time speed",
        
        // Other actions
        "actionSwitch_Equatorial_Mount": "Toggle equatorial mount",
        "actionSet_Full_Screen_Global": "Toggle fullscreen",
        
        // Special actions
        "tab_previous": "Previous tab",
        "tab_next": "Next tab",
        "reset_view": "Reset view direction",
        "reset_zoom": "Reset zoom to 60°",
        "stop_movement": "Stop movement",
        "none": "No action"
    };
    
    // ==================== Button Mappings ====================
    let buttonMappings = {};
    
    /**
     * Initialize default button mappings for PlayStation-style controllers
     */
    function initDefaultMappings() {
        buttonMappings = {
            [PS_BUTTONS.CROSS]: { name: BUTTON_NAMES[PS_BUTTONS.CROSS], action: "actionShow_Night_Mode" },
            [PS_BUTTONS.CIRCLE]: { name: BUTTON_NAMES[PS_BUTTONS.CIRCLE], action: "actionShow_Constellation_Art" },
            [PS_BUTTONS.SQUARE]: { name: BUTTON_NAMES[PS_BUTTONS.SQUARE], action: "actionShow_Constellation_Lines" },
            [PS_BUTTONS.TRIANGLE]: { name: BUTTON_NAMES[PS_BUTTONS.TRIANGLE], action: "actionShow_Constellation_Boundaries" },
            [PS_BUTTONS.L1]: { name: BUTTON_NAMES[PS_BUTTONS.L1], action: "actionShow_Equatorial_Grid" },
            [PS_BUTTONS.R1]: { name: BUTTON_NAMES[PS_BUTTONS.R1], action: "actionShow_Azimuthal_Grid" },
            [PS_BUTTONS.L2]: { name: BUTTON_NAMES[PS_BUTTONS.L2], action: "actionDecrease_Time_Speed" },
            [PS_BUTTONS.R2]: { name: BUTTON_NAMES[PS_BUTTONS.R2], action: "actionIncrease_Time_Speed" },
            [PS_BUTTONS.SHARE]: { name: BUTTON_NAMES[PS_BUTTONS.SHARE], action: "actionShow_Atmosphere" },
            [PS_BUTTONS.OPTIONS]: { name: BUTTON_NAMES[PS_BUTTONS.OPTIONS], action: "actionShow_Ground" },
            [PS_BUTTONS.L3]: { name: BUTTON_NAMES[PS_BUTTONS.L3], action: "actionSwitch_Equatorial_Mount" },
            [PS_BUTTONS.R3]: { name: BUTTON_NAMES[PS_BUTTONS.R3], action: "reset_zoom" },
            [PS_BUTTONS.PS]: { name: BUTTON_NAMES[PS_BUTTONS.PS], action: "actionSet_Full_Screen_Global" },
            [PS_BUTTONS.TOUCHPAD]: { name: BUTTON_NAMES[PS_BUTTONS.TOUCHPAD], action: "actionShow_Nebulas" },
            12: { name: "D-Pad Up", action: "actionIncrease_Time_Speed" },
            13: { name: "D-Pad Down", action: "actionDecrease_Time_Speed" },
            14: { name: "D-Pad Left", action: "tab_previous" },
            15: { name: "D-Pad Right", action: "tab_next" }
        };
    }
    
    initDefaultMappings();
    const DEFAULT_BUTTON_MAPPINGS = JSON.parse(JSON.stringify(buttonMappings));
    
    // =====================================================================
    // SECTION 3: PRIVATE VARIABLES
    // =====================================================================
    
    let gamepadIndex = null;              // Index of connected gamepad
    let animationFrame = null;            // RAF handle for animation loop
    let lastLeftX = 0, lastLeftY = 0;     // Last left stick values
    let lastRightX = 0, lastRightY = 0;   // Last right stick values
    let buttonStates = {};                 // Current button states
    let currentGamepadInfo = null;         // Current gamepad info object
    let isPolling = false;                 // Polling state
    
    /**
     * Calibration data structure
     * Stores offset values for joystick calibration
     */
    let calibrationData = {
        leftX: { raw: 0, calibrated: 0, offset: 0 },
        leftY: { raw: 0, calibrated: 0, offset: 0 },
        rightX: { raw: 0, calibrated: 0, offset: 0 },
        rightY: { raw: 0, calibrated: 0, offset: 0 },
        isCalibrated: false,
        calibrationMode: false,
        samples: [],
        sampleCount: 0
    };
    
    /**
     * Controller settings with defaults
     */
    let controllerSettings = {
        sensitivity: 1.0,      // Joystick sensitivity multiplier
        deadzone: 0.15,        // Deadzone to ignore small movements
        invertX: false,        // Invert horizontal axis
        invertY: false,        // Invert vertical axis
        zoomSpeed: 0.05,       // Zoom speed factor
        movementSpeed: 5.0,    // View movement speed
        autoDetectDevice: true  // Auto-detect device type
    };
    
    let currentFov = 60;        // Current field of view
    let $viewFov, $viewFovText; // UI element references
    
    // =====================================================================
    // SECTION 4: INITIALIZATION
    // =====================================================================
    
    /**
     * Initialize the controller module
     * Called when the UI is ready
     */
    function init() {
        console.log("Gamepad Controller: Initializing with SDL database");
        console.log("Database contains", Object.keys(GAMEPAD_DATABASE).length, "devices");
        
        loadButtonMappings();
        loadSettings();
        loadCalibrationData();
        
        // Cache UI elements
        $viewFov = $("#view_fov");
        $viewFovText = $("#view_fov_text");
        
        setupUI();
        setupGamepadListeners();
        setupFOVIntegration();
        startAnimationLoop();
        
        // Setup event handlers
        $("#gp-reset-mappings").click(resetButtonMappings);
        $("#gp-save-mappings").click(() => {
            saveButtonMappings();
            showNotification("Button mappings saved successfully!");
        });
        
        $("#gp-calibrate").click(startCalibration);
    }
    
    /**
     * Show a temporary notification message
     * @param {string} message - Message to display
     */
    function showNotification(message) {
        const $notification = $('<div class="ui-state-highlight" style="position:fixed;top:20px;right:20px;padding:10px;z-index:9999;border-radius:5px;">' + message + '</div>');
        $('body').append($notification);
        setTimeout(() => $notification.fadeOut(() => $notification.remove()), 2000);
    }
    
    // =====================================================================
    // SECTION 5: CALIBRATION
    // =====================================================================
    
    /**
     * Start the calibration process
     * Collects samples from joysticks at rest to determine center offsets
     */
    function startCalibration() {
        if (gamepadIndex === null) {
            showNotification("Please connect a controller first");
            return;
        }
        
        calibrationData.calibrationMode = true;
        calibrationData.samples = [];
        calibrationData.sampleCount = 0;
        calibrationData.isCalibrated = false;
        
        $("#gp-calibrate").text("Calibrating...").addClass("calibrating");
        $("#gp-controller").addClass("calibrating");
        
        showNotification("Move both joysticks in circles, then press any button to finish");
        
        // Auto-finish after 5 seconds
        setTimeout(() => {
            if (calibrationData.calibrationMode) {
                finishCalibration();
            }
        }, 5000);
    }
    
    /**
     * Collect a calibration sample from the current gamepad state
     * @param {Gamepad} gp - Gamepad object
     */
    function collectCalibrationSample(gp) {
        if (!calibrationData.calibrationMode) return;
        
        // Get axis indices from current device info
        let rightXAxe = 2;
        let rightYAxe = 5;
        
        if (currentGamepadInfo) {
            rightXAxe = currentGamepadInfo.axes.rightX;
            rightYAxe = currentGamepadInfo.axes.rightY;
        }
        
        calibrationData.samples.push({
            leftX: gp.axes[0] || 0,
            leftY: gp.axes[1] || 0,
            rightX: gp.axes[rightXAxe] || 0,
            rightY: gp.axes[rightYAxe] || 0,
            timestamp: Date.now()
        });
        
        calibrationData.sampleCount++;
        updateCalibrationDisplay();
        
        // Check if any button pressed to finish
        for (let i = 0; i < gp.buttons.length; i++) {
            if (gp.buttons[i].pressed) {
                finishCalibration();
                break;
            }
        }
    }
    
    /**
     * Finish calibration and calculate offset values
     */
    function finishCalibration() {
        if (!calibrationData.calibrationMode || calibrationData.samples.length === 0) {
            calibrationData.calibrationMode = false;
            return;
        }
        
        // Calculate average offsets
        let sumLeftX = 0, sumLeftY = 0, sumRightX = 0, sumRightY = 0;
        
        calibrationData.samples.forEach(sample => {
            sumLeftX += sample.leftX;
            sumLeftY += sample.leftY;
            sumRightX += sample.rightX;
            sumRightY += sample.rightY;
        });
        
        calibrationData.leftX.offset = sumLeftX / calibrationData.samples.length;
        calibrationData.leftY.offset = sumLeftY / calibrationData.samples.length;
        calibrationData.rightX.offset = sumRightX / calibrationData.samples.length;
        calibrationData.rightY.offset = sumRightY / calibrationData.samples.length;
        
        calibrationData.isCalibrated = true;
        calibrationData.calibrationMode = false;
        
        saveCalibrationData();
        
        $("#gp-calibrate").text("Calibrate").removeClass("calibrating");
        $("#gp-controller").removeClass("calibrating");
        
        showNotification("Calibration complete!");
        updateCalibrationDisplay();
    }
    
    /**
     * Apply calibration offset to a raw axis value
     * @param {number} value - Raw axis value
     * @param {number} offset - Calibration offset
     * @returns {number} Calibrated value
     */
    function applyCalibration(value, offset) {
        if (!calibrationData.isCalibrated) return value;
        let calibrated = value - offset;
        return Math.max(-1, Math.min(1, calibrated));
    }
    
    /**
     * Save calibration data to localStorage
     */
    function saveCalibrationData() {
        try {
            localStorage.setItem('gamepad_calibration', JSON.stringify({
                leftX: calibrationData.leftX.offset,
                leftY: calibrationData.leftY.offset,
                rightX: calibrationData.rightX.offset,
                rightY: calibrationData.rightY.offset,
                timestamp: Date.now()
            }));
        } catch(e) {
            console.warn("Gamepad: Could not save calibration data", e);
        }
    }
    
    /**
     * Load calibration data from localStorage
     */
    function loadCalibrationData() {
        try {
            const saved = localStorage.getItem('gamepad_calibration');
            if (saved) {
                const data = JSON.parse(saved);
                calibrationData.leftX.offset = data.leftX || 0;
                calibrationData.leftY.offset = data.leftY || 0;
                calibrationData.rightX.offset = data.rightX || 0;
                calibrationData.rightY.offset = data.rightY || 0;
                calibrationData.isCalibrated = true;
                updateCalibrationDisplay();
            }
        } catch(e) {
            console.warn("Gamepad: Could not load calibration data", e);
        }
    }
    
    /**
     * Update the calibration display in the UI
     */
    function updateCalibrationDisplay() {
        if ($("#gp-calibration-values").length === 0) return;
        
        let html = '';
        
        if (calibrationData.calibrationMode) {
            html = '<div class="calibration-status">Calibrating... Collected ' + 
                   calibrationData.sampleCount + ' samples</div>';
        } else if (calibrationData.isCalibrated) {
            html = '<div class="calibration-values">' +
                   '<div class="calibration-value-item"><span class="calibration-value-label">Left X:</span> ' +
                   '<span class="calibration-value-number">' + calibrationData.leftX.offset.toFixed(3) + '</span></div>' +
                   '<div class="calibration-value-item"><span class="calibration-value-label">Left Y:</span> ' +
                   '<span class="calibration-value-number">' + calibrationData.leftY.offset.toFixed(3) + '</span></div>' +
                   '<div class="calibration-value-item"><span class="calibration-value-label">Right X:</span> ' +
                   '<span class="calibration-value-number">' + calibrationData.rightX.offset.toFixed(3) + '</span></div>' +
                   '<div class="calibration-value-item"><span class="calibration-value-label">Right Y:</span> ' +
                   '<span class="calibration-value-number">' + calibrationData.rightY.offset.toFixed(3) + '</span></div>' +
                   '</div>';
        }
        
        $("#gp-calibration-values").html(html);
    }
    
    // =====================================================================
    // SECTION 6: FOV INTEGRATION
    // =====================================================================
    
    /**
     * Setup integration with Stellarium's FOV system
     */
    function setupFOVIntegration() {
        $(viewcontrol).on("fovChanged", (evt, fov) => {
            currentFov = fov;
            updateFovDisplay(fov);
            updateMainUI_FOV(fov);
        });
        
        if ($viewFov.length) {
            $viewFov.on("slidechange slide", (evt, ui) => {
                if (evt.type === 'slidechange' || evt.type === 'slide') {
                    setTimeout(() => {
                        $.getJSON('/api/main/view', (data) => {
                            if (data && data.fov && Math.abs(data.fov - currentFov) > 0.01) {
                                currentFov = data.fov;
                                updateFovDisplay(currentFov);
                            }
                        });
                    }, 50);
                }
            });
        }
        
        loadCurrentFov();
    }
    
    /**
     * Update the main UI FOV display
     * @param {number} fov - Field of view in degrees
     */
    function updateMainUI_FOV(fov) {
        if ($viewFovText.length) {
            $viewFovText.text(fov.toPrecision(3));
        }
        
        if ($viewFov.length && $viewFov.hasClass('ui-slider')) {
            const minFov = 0.001389;
            const maxFov = 360;
            const fovSteps = 1000;
            const s = Math.pow((fov - minFov) / (maxFov - minFov), 1/4);
            const slVal = Math.round(s * fovSteps);
            $viewFov.slider("value", fovSteps - slVal);
        }
    }
    
    /**
     * Load current FOV from Stellarium API
     */
    function loadCurrentFov() {
        $.getJSON('/api/main/view', (data) => {
            if (data && data.fov) {
                currentFov = data.fov;
                updateFovDisplay(currentFov);
            }
        });
    }
    
    // =====================================================================
    // SECTION 7: SETTINGS MANAGEMENT
    // =====================================================================
    
    /**
     * Load controller settings from localStorage
     */
    function loadSettings() {
        try {
            const saved = localStorage.getItem('gamepad_controller_settings');
            if (saved) {
                controllerSettings = JSON.parse(saved);
            }
        } catch(e) {
            console.warn("Gamepad: Could not load settings", e);
        }
    }
    
    /**
     * Save controller settings to localStorage
     */
    function saveSettings() {
        try {
            localStorage.setItem('gamepad_controller_settings', JSON.stringify(controllerSettings));
        } catch(e) {
            console.warn("Gamepad: Could not save settings", e);
        }
    }
    
    /**
     * Load button mappings from localStorage
     */
    function loadButtonMappings() {
        try {
            const saved = localStorage.getItem('gamepad_button_mappings');
            if (saved) {
                const parsed = JSON.parse(saved);
                buttonMappings = { ...buttonMappings, ...parsed };
            }
        } catch(e) {
            console.warn("Gamepad: Could not load button mappings", e);
        }
    }
    
    /**
     * Save button mappings to localStorage
     */
    function saveButtonMappings() {
        try {
            localStorage.setItem('gamepad_button_mappings', JSON.stringify(buttonMappings));
        } catch(e) {
            console.warn("Gamepad: Could not save button mappings", e);
        }
    }
    
    /**
     * Reset button mappings to defaults
     */
    function resetButtonMappings() {
        buttonMappings = JSON.parse(JSON.stringify(DEFAULT_BUTTON_MAPPINGS));
        saveButtonMappings();
        populateButtonCustomization();
        showNotification("Button mappings reset to defaults");
    }
    
    // =====================================================================
    // SECTION 8: UI SETUP
    // =====================================================================
    
    /**
     * Setup UI components and event handlers
     */
    function setupUI() {
        if ($("#gp-controller").length === 0) return;
        
        // Sensitivity slider
        $("#gp-sensitivity").slider({
            min: 0.1, max: 3.0, step: 0.1,
            value: controllerSettings.sensitivity,
            slide: (e, ui) => {
                controllerSettings.sensitivity = ui.value;
                $("#gp-sensitivity-value").text(ui.value.toFixed(1));
                saveSettings();
            }
        });
        
        // Deadzone slider
        $("#gp-deadzone").slider({
            min: 0, max: 0.5, step: 0.01,
            value: controllerSettings.deadzone,
            slide: (e, ui) => {
                controllerSettings.deadzone = ui.value;
                $("#gp-deadzone-value").text(ui.value.toFixed(2));
                updateStickPreview('left', 0, 0);
                updateStickPreview('right', 0, 0);
                saveSettings();
            }
        });
        
        // Zoom speed slider
        $("#gp-zoom-speed").slider({
            min: 0.01, max: 0.3, step: 0.01,
            value: controllerSettings.zoomSpeed,
            slide: (e, ui) => {
                controllerSettings.zoomSpeed = ui.value;
                $("#gp-zoom-speed-value").text(ui.value.toFixed(2));
                saveSettings();
            }
        });
        
        // Invert checkboxes
        $("#gp-invert-x").prop('checked', controllerSettings.invertX)
            .change(function() {
                controllerSettings.invertX = $(this).is(":checked");
                saveSettings();
            });
        
        $("#gp-invert-y").prop('checked', controllerSettings.invertY)
            .change(function() {
                controllerSettings.invertY = $(this).is(":checked");
                saveSettings();
            });
        
        $("#gp-auto-detect").prop('checked', controllerSettings.autoDetectDevice)
            .change(function() {
                controllerSettings.autoDetectDevice = $(this).is(":checked");
                saveSettings();
            });
        
        $("#gp-connect").click(checkForGamepads);
        
        // Update display values
        $("#gp-sensitivity-value").text(controllerSettings.sensitivity.toFixed(1));
        $("#gp-deadzone-value").text(controllerSettings.deadzone.toFixed(2));
        $("#gp-zoom-speed-value").text(controllerSettings.zoomSpeed.toFixed(2));
        
        updateCalibrationDisplay();
    }
    
    /**
     * Populate the button customization UI
     * Shows different button groups based on connected controller
     */
    function populateButtonCustomization() {
        const container = $("#gp-button-customization");
        if (container.length === 0) return;
        
        container.empty();
        
        // Show message if no controller connected
        if (gamepadIndex === null || !currentGamepadInfo) {
            container.html('<div class="no-device-message">' +
                          '<p style="color: #4CAF50; text-align: center; padding: 20px;">' +
                          '🔴 Connect a controller to customize buttons</p></div>');
            return;
        }
        
        // Group buttons by category
        const buttonGroups = {
            'Face Buttons': [PS_BUTTONS.CROSS, PS_BUTTONS.CIRCLE, PS_BUTTONS.SQUARE, PS_BUTTONS.TRIANGLE],
            'Shoulder & Triggers': [PS_BUTTONS.L1, PS_BUTTONS.R1, PS_BUTTONS.L2, PS_BUTTONS.R2],
            'System Buttons': [PS_BUTTONS.SHARE, PS_BUTTONS.OPTIONS, PS_BUTTONS.PS, PS_BUTTONS.TOUCHPAD],
            'Stick Buttons': [PS_BUTTONS.L3, PS_BUTTONS.R3],
            'D-Pad': [12, 13, 14, 15]
        };
        
        const groupsContainer = $('<div class="button-groups-container"></div>');
        
        Object.entries(buttonGroups).forEach(([groupName, buttonIndices]) => {
            const groupDiv = $('<div class="button-group"></div>');
            groupDiv.append(`<div class="button-group-header">${groupName}</div>`);
            
            buttonIndices.forEach(buttonId => {
                const mapping = buttonMappings[buttonId];
                if (!mapping) return;
                
                const itemDiv = $('<div class="button-item"></div>');
                itemDiv.append(`<span class="button-name">${mapping.name}</span>`);
                
                // Create dropdown with available actions
                const select = $('<select class="button-select"></select>');
                select.data('button-id', buttonId);
                
                Object.entries(AVAILABLE_ACTIONS).forEach(([actionKey, actionDesc]) => {
                    const option = $(`<option value="${actionKey}">${actionDesc}</option>`);
                    if (mapping.action === actionKey) {
                        option.attr('selected', 'selected');
                    }
                    select.append(option);
                });
                
                select.change(function() {
                    const btnId = $(this).data('button-id');
                    const newAction = $(this).val();
                    buttonMappings[btnId].action = newAction;
                });
                
                itemDiv.append(select);
                groupDiv.append(itemDiv);
            });
            
            groupsContainer.append(groupDiv);
        });
        
        container.append(groupsContainer);
    }
    
    // =====================================================================
    // SECTION 9: GAMEPAD MANAGEMENT
    // =====================================================================
    
    /**
     * Check for connected gamepads
     */
    function checkForGamepads() {
        const gamepads = navigator.getGamepads ? navigator.getGamepads() : [];
        for (let i = 0; i < gamepads.length; i++) {
            if (gamepads[i]) {
                handleGamepadConnected(gamepads[i]);
                break;
            }
        }
    }
    
    /**
     * Handle gamepad connection event
     * @param {Gamepad} gp - Connected gamepad
     */
    function handleGamepadConnected(gp) {
        if (controllerSettings.autoDetectDevice) {
            currentGamepadInfo = detectGamepadType(gp.id);
        } else {
            currentGamepadInfo = GAMEPAD_DATABASE.generic_usb;
        }
        
        console.log("Gamepad Connected:", gp.id);
        console.log("Detected as:", currentGamepadInfo.name);
        console.log("Axes mapping:", currentGamepadInfo.axes);
        
        gamepadIndex = gp.index;
        
        // Format device info with USB IDs if available
        let deviceDisplayName = currentGamepadInfo.name;
        const usbIds = extractUSBIds(gp.id);
        if (usbIds) {
            deviceDisplayName += ` (${usbIds.vendor_id}:${usbIds.product_id})`;
        }
        
        updateConnectionStatus(true, deviceDisplayName);
        populateButtonCustomization();
    }
    
    /**
     * Setup gamepad event listeners
     */
    function setupGamepadListeners() {
        window.addEventListener("gamepadconnected", (e) => {
            handleGamepadConnected(e.gamepad);
        });
        
        window.addEventListener("gamepaddisconnected", (e) => {
            if (gamepadIndex === e.gamepad.index) {
                console.log("Gamepad Disconnected");
                gamepadIndex = null;
                currentGamepadInfo = null;
                updateConnectionStatus(false);
                stopAllMovements();
                populateButtonCustomization();
            }
        });
        
        checkForGamepads();
    }
    
    /**
     * Update connection status in UI
     * @param {boolean} connected - Connection status
     * @param {string} deviceName - Device name to display
     */
    function updateConnectionStatus(connected, deviceName) {
        if ($("#gp-status").length === 0) return;
        
        const $status = $("#gp-status .status-indicator");
        const $deviceName = $("#gp-device-name");
        const $connectionStatus = $("#gp-connection-status");
        
        if (connected) {
            $status.html("🟢 Connected").css("color", "#4CAF50");
            $deviceName.text(deviceName || "Unknown Device");
            $connectionStatus.text("Connected").css("color", "#4CAF50");
        } else {
            $status.html("🔴 Disconnected").css("color", "#f44336");
            $deviceName.text("-");
            $connectionStatus.text("Disconnected").css("color", "#f44336");
        }
    }
    
    // =====================================================================
    // SECTION 10: ANIMATION LOOP AND INPUT PROCESSING
    // =====================================================================
    
    /**
     * Start the animation loop for gamepad polling
     */
    function startAnimationLoop() {
        if (animationFrame) cancelAnimationFrame(animationFrame);
        isPolling = true;
        
        function poll() {
            if (gamepadIndex !== null && isPolling) {
                const gp = navigator.getGamepads()[gamepadIndex];
                if (gp) {
                    processGamepadInput(gp);
                }
            }
            animationFrame = requestAnimationFrame(poll);
        }
        poll();
    }
    
    /**
     * Apply deadzone to analog value
     * @param {number} value - Raw analog value
     * @returns {number} Value with deadzone applied
     */
    function applyDeadzone(value) {
        return Math.abs(value) > controllerSettings.deadzone ? value : 0;
    }
    
    /**
     * Process gamepad input
     * @param {Gamepad} gp - Gamepad object
     */
    function processGamepadInput(gp) {
        // Get axis indices from current device info
        let leftXAxe = 0, leftYAxe = 1;
        let rightXAxe = 2, rightYAxe = 5;
        
        if (currentGamepadInfo) {
            leftXAxe = currentGamepadInfo.axes.leftX;
            leftYAxe = currentGamepadInfo.axes.leftY;
            rightXAxe = currentGamepadInfo.axes.rightX;
            rightYAxe = currentGamepadInfo.axes.rightY;
        }
        
        // Read raw axis values
        let rawLeftX = gp.axes[leftXAxe] || 0;
        let rawLeftY = gp.axes[leftYAxe] || 0;
        let rawRightX = gp.axes[rightXAxe] || 0;
        let rawRightY = gp.axes[rightYAxe] || 0;
        
        // Apply calibration
        let leftX = applyCalibration(rawLeftX, calibrationData.leftX.offset);
        let leftY = applyCalibration(rawLeftY, calibrationData.leftY.offset);
        let rightX = applyCalibration(rawRightX, calibrationData.rightX.offset);
        let rightY = applyCalibration(rawRightY, calibrationData.rightY.offset);
        
        // Collect calibration samples if in calibration mode
        if (calibrationData.calibrationMode) {
            collectCalibrationSample(gp);
        }
        
        // Apply deadzone
        leftX = applyDeadzone(leftX);
        leftY = applyDeadzone(leftY);
        rightX = applyDeadzone(rightX);
        rightY = applyDeadzone(rightY);
        
        // Apply invert settings
        if (controllerSettings.invertX) {
            leftX = -leftX;
            rightX = -rightX;
        }
        if (controllerSettings.invertY) {
            leftY = -leftY;
            rightY = -rightY;
        }
        
        // Apply sensitivity to left stick only (right stick is for zoom)
        leftX *= controllerSettings.sensitivity;
        leftY *= controllerSettings.sensitivity;
        
        // ==================== Left Stick - View Movement ====================
        if (leftX !== lastLeftX || leftY !== lastLeftY) {
            lastLeftX = leftX;
            lastLeftY = leftY;
            
            if (leftX !== 0 || leftY !== 0) {
                // Cubic curve for smoother control
                const moveX = controllerSettings.movementSpeed * leftX * leftX * leftX;
                const moveY = controllerSettings.movementSpeed * leftY * leftY * leftY;
                sendMovement(moveX, -moveY);
            } else {
                stopMovement();
            }
        }
        
        // ==================== Right Stick - Zoom Control ====================
        // Only use vertical axis (Y) for zoom
        if (rightY !== lastRightY) {
            lastRightY = rightY;
            
            if (Math.abs(rightY) > 0.05) {
                const zoomFactor = 1 - (Math.pow(rightY, 3) * controllerSettings.zoomSpeed * 2);
                let newFov = currentFov * zoomFactor;
                newFov = Math.max(0.5, Math.min(180, newFov));
                
                if (Math.abs(newFov - currentFov) > 0.01) {
                    viewcontrol.setFOV(newFov);
                    currentFov = newFov;
                    updateFovDisplay(currentFov);
                }
            }
        } else if (rightY === 0 && lastRightY !== 0) {
            lastRightY = 0;
        }
        
        // ==================== Button Processing ====================
        const previousButtonStates = { ...buttonStates };
        
        gp.buttons.forEach((button, index) => {
            buttonStates[index] = button.pressed;
        });
        
        gp.buttons.forEach((button, index) => {
            const isPressedNow = buttonStates[index];
            const wasPressed = previousButtonStates[index] || false;
            const mapping = buttonMappings[index];
            
            if (!mapping) return;
            
            // Handle button press (only on new press, not hold)
            if (isPressedNow && !wasPressed) {
                handleButtonPress(index);
            }
            
            // Handle button release for specific actions
            if (!isPressedNow && wasPressed) {
                if (mapping.action === "stop_movement") {
                    stopMovement();
                }
            }
        });
        
        // ==================== Update Joystick Preview ====================
        updateStickPreview('left', leftX, leftY);
        updateStickPreview('right', rightX, rightY);
    }
    
    // =====================================================================
    // SECTION 11: CONTROL FUNCTIONS
    // =====================================================================
    
    /**
     * Send movement command to Stellarium
     * @param {number} x - X movement (-5 to 5)
     * @param {number} y - Y movement (-5 to 5)
     */
    function sendMovement(x, y) {
        $.ajax({
            url: '/api/main/move',
            type: 'POST',
            data: { x: x, y: y }
        });
    }
    
    /**
     * Stop all movement
     */
    function stopMovement() {
        $.ajax({
            url: '/api/main/move',
            type: 'POST',
            data: { x: 0, y: 0 }
        });
    }
    
    /**
     * Stop all movements and reset state
     */
    function stopAllMovements() {
        stopMovement();
        lastLeftX = lastLeftY = 0;
        lastRightX = lastRightY = 0;
    }
    
    /**
     * Update FOV display in UI
     * @param {number} fov - Field of view in degrees
     */
    function updateFovDisplay(fov) {
        $("#gp-current-fov").text(fov.toFixed(2) + "°");
    }
    
    // =====================================================================
    // SECTION 12: BUTTON HANDLERS
    // =====================================================================
    
    /**
     * Handle button press event
     * @param {number} buttonIndex - Index of pressed button
     */
    function handleButtonPress(buttonIndex) {
        if (!buttonMappings[buttonIndex]) {
            return;
        }
        
        const mapping = buttonMappings[buttonIndex];
        
        switch(mapping.action) {
            case "tab_previous":
                navigateTab(-1);
                break;
            case "tab_next":
                navigateTab(1);
                break;
            case "reset_view":
                sendMovement(0, 0);
                break;
            case "reset_zoom":
                viewcontrol.setFOV(60);
                break;
            case "stop_movement":
                stopMovement();
                break;
            case "none":
                break;
            default:
                if (mapping.action && mapping.action.startsWith('action')) {
                    actions.execute(mapping.action);
                }
                break;
        }
    }
    
    /**
     * Navigate between tabs in the main interface
     * @param {number} direction - Direction (-1 for previous, 1 for next)
     */
    function navigateTab(direction) {
        const $tabs = $("#tabs");
        const current = $tabs.tabs("option", "active");
        const maxTabs = $tabs.find(".ui-tabs-nav li").length - 1;
        const newTab = current + direction;
        
        if (newTab >= 0 && newTab <= maxTabs) {
            $tabs.tabs("option", "active", newTab);
        }
    }
    
    // =====================================================================
    // SECTION 13: JOYSTICK PREVIEW
    // =====================================================================
    
    /**
     * Update joystick preview canvas
     * @param {string} stick - 'left' or 'right'
     * @param {number} x - X axis value
     * @param {number} y - Y axis value
     */
    function updateStickPreview(stick, x, y) {
        const canvasId = stick === 'left' ? 'gp-left-stick-canvas' : 'gp-right-stick-canvas';
        const canvas = document.getElementById(canvasId);
        if (!canvas) return;
        
        const ctx = canvas.getContext('2d');
        const w = canvas.width;
        const h = canvas.height;
        
        // Clear canvas
        ctx.clearRect(0, 0, w, h);
        
        // Background
        ctx.fillStyle = '#222';
        ctx.fillRect(0, 0, w, h);
        
        // Crosshair
        ctx.strokeStyle = '#444';
        ctx.lineWidth = 1;
        ctx.beginPath();
        ctx.moveTo(w/2, 0);
        ctx.lineTo(w/2, h);
        ctx.stroke();
        ctx.beginPath();
        ctx.moveTo(0, h/2);
        ctx.lineTo(w, h/2);
        ctx.stroke();
        
        // Deadzone circle
        ctx.strokeStyle = '#666';
        ctx.setLineDash([2, 2]);
        ctx.beginPath();
        ctx.arc(w/2, h/2, (w/2) * controllerSettings.deadzone, 0, Math.PI * 2);
        ctx.stroke();
        ctx.setLineDash([]);
        
        // Center point
        ctx.fillStyle = '#555';
        ctx.beginPath();
        ctx.arc(w/2, h/2, 3, 0, Math.PI * 2);
        ctx.fill();
        
        // Stick position
        const stickX = w/2 + (x * (w/2 - 8));
        const stickY = h/2 + (y * (h/2 - 8));
        
        // Draw stick position with gradient
        const gradient = ctx.createRadialGradient(stickX, stickY, 2, stickX, stickY, 8);
        gradient.addColorStop(0, '#4CAF50');
        gradient.addColorStop(1, '#2E7D32');
        
        ctx.fillStyle = gradient;
        ctx.beginPath();
        ctx.arc(stickX, stickY, 6, 0, Math.PI * 2);
        ctx.fill();
        
        // Border
        ctx.strokeStyle = '#666';
        ctx.lineWidth = 1;
        ctx.strokeRect(0, 0, w, h);
    }
    
    // =====================================================================
    // SECTION 14: PUBLIC API
    // =====================================================================
    
    return {
        /**
         * Initialize the controller module
         */
        init: init,
        
        /**
         * Manually connect to a gamepad
         */
        connect: checkForGamepads,
        
        /**
         * Disconnect the current gamepad
         */
        disconnect: function() {
            gamepadIndex = null;
            isPolling = false;
            updateConnectionStatus(false);
            stopAllMovements();
            if (animationFrame) {
                cancelAnimationFrame(animationFrame);
                animationFrame = null;
            }
            populateButtonCustomization();
        },
        
        /**
         * Check if a gamepad is currently connected
         * @returns {boolean} True if connected
         */
        isConnected: function() {
            return gamepadIndex !== null;
        },
        
        /**
         * Get current controller settings
         * @returns {Object} Controller settings
         */
        getSettings: function() {
            return { ...controllerSettings };
        },
        
        /**
         * Reset controller settings to defaults
         */
        resetSettings: function() {
            controllerSettings = {
                sensitivity: 1.0,
                deadzone: 0.15,
                invertX: false,
                invertY: false,
                zoomSpeed: 0.05,
                movementSpeed: 5.0,
                autoDetectDevice: true
            };
            saveSettings();
            location.reload();
        },
        
        /**
         * Start joystick calibration
         */
        startCalibration: startCalibration,
        
        /**
         * Get current calibration data
         * @returns {Object} Calibration data
         */
        getCalibrationData: function() {
            return { ...calibrationData };
        },
        
        /**
         * Get the gamepad database
         * @returns {Object} Gamepad database
         */
        getDatabase: function() {
            return { ...GAMEPAD_DATABASE };
        },
        
        /**
         * Get number of supported devices
         * @returns {number} Device count
         */
        getDeviceCount: function() {
            return Object.keys(GAMEPAD_DATABASE).length;
        }
    };
});