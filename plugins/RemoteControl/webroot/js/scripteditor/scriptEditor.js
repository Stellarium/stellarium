/* ========================================================================
 * scriptEditor.js - Advanced Script Editor for Stellarium Remote Control
 * ========================================================================
 * 
 * This module provides a full-featured script editor with CodeMirror,
 * syntax highlighting, autocomplete, validation, and script execution.
 * 
 * Features:
 * - CodeMirror editor with Stellarium syntax highlighting
 * - Case-sensitive function validation (GREEN=valid, RED=invalid, ORANGE=case error)
 * - Real-time autocomplete with Ctrl+Space (supports namespace filtering)
 * - Auto-validation toggle (OFF by default)
 * - Revalidate button with full document case-sensitive checking
 * - Parameter hint popup when typing '(' after function
 * - Function detail popup with description, parameters, and examples
 * - Quick Reference panel with categorized functions and snippets
 * - Search/filter in Quick Reference
 * - Script execution via Remote Control API
 * - Output panel with JSON syntax highlighting
 * - Installed scripts browser with run/load buttons
 * - Execution history with reload capability (localStorage)
 * - File operations (open/save .ssc files)
 * - Metadata/header dialog for script documentation
 * - Collect Examples button to import examples from module JSON files
 * - Line counter in toolbar (auto-updates)
 * - Copy entire editor content to clipboard
 * - Keyboard shortcuts help dialog
 * - Go to line dialog (Ctrl+G / Ctrl+L)
 * - Fallback textarea editor when CodeMirror unavailable
 * 
 * Technical Implementation:
 * - Dynamic module names from modules-index.json
 * - Strict case-sensitive function validation
 * - Real-time validation with debouncing (1000ms)
 * - Autocomplete with dot (.) detection for namespace filtering
 * - Info panel positioning relative to autocomplete dropdown
 * - Example insertion at cursor or specific line number
 * - Integration with showNotification from stellarium-utils
 * - Status tracking via activeScriptChanged event
 * - Continuous polling for direct script execution status
 * 
 * @module scriptEditor
 * @requires jquery
 * @requires api/scripts
 * @requires api/remotecontrol
 * @requires api/properties
 * @requires scripteditor/scriptReference
 * @requires ui/stellarium-utils
 * 
 * @author kutaibaa akraa (GitHub: @kutaibaa-akraa)
 * @date 2026-06-06
 * @license GPLv2+
 * @version 1.0.0
 * 
 * ======================================================================== */

define(["jquery", "api/scripts", "api/remotecontrol", "api/properties", "scripteditor/scriptReference", "ui/stellarium-utils"], 
    function($, scriptApi, rc, propApi, scriptReference, stelUtils) {
    "use strict";
    
		// Using "showNotification" function from stellarium-utils.js
    var showNotification = stelUtils.showNotification;


    // =====================================================================
    // PRIVATE VARIABLES
    // =====================================================================

    var codeMirrorInstance = null;
    // Expose for debugging
    window._cm = codeMirrorInstance;		
    var isRunning = false;
    var currentRunningScriptId = null;
    var executionHistory = [];
    var maxHistoryEntries = 20;
    var dom = {};
    
    // Autocomplete related variables
		var autocompleteInfoPanel = null;
    var currentAutocompleteData = null;
    var activeHintsContainer = null;
		
		// =====================================================================
		// EXAMPLE CODE CACHE FOR INFO PANEL
		// =====================================================================

		/**
		 * Global cache for storing example code to avoid HTML attribute encoding issues.
		 * This solves the problem where long examples with special characters
		 * would break when stored in data attributes.
		 * 
		 * @type {Object}
		 */
		var exampleCodeCache = {};
		var exampleCounter = 0;

		var _tr = rc.tr;
		
		// =====================================================================
		// CODEMIRROR LOADING CHECK
		// =====================================================================

		/**
		 * Wait for CodeMirror to be fully loaded before initialization.
		 * CodeMirror is loaded via script tags, not as AMD module.
		 * 
		 * @param {Function} callback - Function to call when CodeMirror is ready
		 * @param {number} retries - Current retry count (internal use)
		 */
		function waitForCodeMirror(callback, retries) {
				retries = retries || 0;
				
				if (typeof CodeMirror !== 'undefined') {
						console.log('[ScriptEditor] CodeMirror is ready');
						callback();
				} else if (retries < 30) {
						console.log('[ScriptEditor] Waiting for CodeMirror... (' + (retries + 1) + '/30)');
						setTimeout(function() {
								waitForCodeMirror(callback, retries + 1);
						}, 100);
				} else {
						console.error('[ScriptEditor] CodeMirror failed to load after 3 seconds');
						// Fallback: create a basic textarea without syntax highlighting
						createFallbackEditor();
				}
		}

		/**
		 * Create a fallback textarea editor when CodeMirror fails to load.
		 */
		function createFallbackEditor() {
				var ta = document.getElementById('script-editor');
				if (!ta) return;
				
				console.warn('[ScriptEditor] Using fallback textarea editor (no syntax highlighting)');
				ta.style.fontFamily = 'monospace';
				ta.style.fontSize = '13px';
				ta.style.lineHeight = '1.5';
				ta.style.background = '#1A1C1E';
				ta.style.color = '#DCDBDA';
				ta.style.border = '1px solid #3A3C3E';
				ta.style.borderRadius = '4px';
				ta.style.padding = '10px';
				ta.style.width = '100%';
				ta.style.height = '450px';
				ta.style.resize = 'vertical';
				
				// Store reference
				codeMirrorInstance = null;
				window._cm = null;
				
				// Load saved content
				try {
						var saved = localStorage.getItem('stellarium-script-editor');
						if (saved) ta.value = saved;
				} catch(e) {}
				
				// Save on change
				$(ta).on('input', function() {
						try {
								localStorage.setItem('stellarium-script-editor', ta.value);
						} catch(e) {}
				});
		}
		
		// =====================================================================
		// LINE COUNTER FUNCTIONS
		// =====================================================================

		/**
		 * Updates the line count display in the toolbar.
		 * @param {number} lineCount - Current number of lines in the editor
		 */
		function updateLineCountDisplay(lineCount) {
				var $lineCountSpan = $('#editor-line-count');
				var $badge = $('.line-counter-badge');
				
				if (!$lineCountSpan.length) return;
				
				$lineCountSpan.text(lineCount);
				
				// Add/remove empty class for visual feedback
				if (lineCount === 0) {
						$badge.addClass('empty');
				} else {
						$badge.removeClass('empty');
				}
		}

		/**
		 * Gets the current line count from the editor.
		 * @returns {number} Number of lines in the editor
		 */
		function getCurrentLineCount() {
				if (codeMirrorInstance) {
						return codeMirrorInstance.lineCount();
				} else {
						var ta = document.getElementById('script-editor');
						if (ta && ta.value) {
								return ta.value.split('\n').length;
						}
				}
				return 0;
		}

		/**
		 * Refreshes the line count display based on current editor state.
		 * This is called after any change to the editor content.
		 */
		function refreshLineCount() {
				var lineCount = getCurrentLineCount();
				updateLineCountDisplay(lineCount);
		}

		/**
		 * Build snippets section from scriptReference data
		 */
		function buildSnippetsSection() {
				var $container = $('#ref-snippets');
				if (!$container.length) return;
				
				var snippets = [];
				if (typeof scriptReference !== 'undefined' && scriptReference.getSnippets) {
						snippets = scriptReference.getSnippets();
				}
				
				if (snippets.length === 0) {
						$container.html('<span class="loading-placeholder">' + _tr('No snippets available') + '</span>');
						return;
				}
				
				var html = '';
				for (var i = 0; i < snippets.length; i++) {
						var s = snippets[i];
						var snippetDataJson = escapeAttr(JSON.stringify({
								id: s.id,
								name: s.name,
								code: s.code,
								description: s.description,
								type: 'snippet'
						}));
						
						html += '<div class="ref-item ref-snippet-item" data-snippet-data="' + snippetDataJson + '">';
						html += '<span class="ref-name">' + escapeHtml(s.name) + '</span>';
						html += '<span class="ref-desc">' + escapeHtml((s.description || '').substring(0, 80)) + '</span>';
						html += '</div>';
				}
				$container.html(html);
		}

    // =====================================================================
    // HELPER FUNCTIONS
    // =====================================================================

    function escapeHtml(text) {
        if (!text) return '';
        return String(text).replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;').replace(/"/g, '&quot;');
    }

		/**
		 * Escape HTML special characters for use in HTML attributes.
		 * Handles long strings and special characters properly.
		 * 
		 * @param {string} text - The string to escape
		 * @returns {string} Escaped string safe for HTML attribute values
		 */
		function escapeAttr(text) {
				if (!text) return '';
				return String(text)
						.replace(/&/g, '&amp;')
						.replace(/"/g, '&quot;')
						.replace(/'/g, '&#39;')
						.replace(/</g, '&lt;')
						.replace(/>/g, '&gt;')
						.replace(/\n/g, '&#10;')    // Preserve newlines as entities
						.replace(/\r/g, '&#13;');   // Preserve carriage returns
		}

    // =====================================================================
    // OUTPUT FUNCTIONS
    // =====================================================================

    function addOutput(type, message, rawData) {
        if (!dom.$output || !dom.$output.length) return;
        var now = new Date();
        var time = now.toLocaleTimeString();
        dom.$output.find('.output-placeholder').remove();
        var $line = $('<div class="output-line ' + type + '"></div>');
        $line.html('<span class="time">[' + time + ']</span> ' + escapeHtml(message));
        dom.$output.append($line);
        if (rawData !== undefined && rawData !== null) {
            var $dataBlock = createDataBlock(rawData);
            if ($dataBlock) dom.$output.append($dataBlock);
        }
        dom.$output.scrollTop(dom.$output[0].scrollHeight);
    }

    function createDataBlock(data) {
        var formattedContent, dataType = 'text';
        if (typeof data === 'string') {
            var trimmed = data.trim();
            if ((trimmed.startsWith('{') && trimmed.endsWith('}')) || (trimmed.startsWith('[') && trimmed.endsWith(']'))) {
                try { formattedContent = JSON.stringify(JSON.parse(trimmed), null, 2); dataType = 'json'; }
                catch(e) { formattedContent = trimmed; }
            } else if (trimmed.startsWith('<') && trimmed.includes('>')) {
                formattedContent = trimmed; dataType = 'html';
            } else { formattedContent = trimmed; }
        } else if (typeof data === 'object') {
            try { formattedContent = JSON.stringify(data, null, 2); dataType = 'json'; }
            catch(e) { formattedContent = String(data); }
        } else { formattedContent = String(data); }
        if (!formattedContent || formattedContent.trim().length === 0) return null;
        var maxLength = 50000, truncated = false;
        if (formattedContent.length > maxLength) {
            formattedContent = formattedContent.substring(0, maxLength) + '\n\n... [truncated, ' + (formattedContent.length - maxLength) + ' more characters]';
            truncated = true;
        }
        var $block = $(
            '<div class="output-data-block">' +
            '<div class="output-data-header">' +
            '<span class="output-data-type">' + dataType.toUpperCase() + ' Response' + (truncated ? ' (truncated)' : '') + '</span>' +
            '<button class="output-copy-btn jquerybutton" title="Copy to clipboard">Copy</button>' +
            '</div>' +
            '<pre class="output-data-content">' + escapeHtml(formattedContent) + '</pre>' +
            '</div>'
        );
        $block.find('.output-copy-btn').on('click', function() {
            var text = $block.find('pre').text();
            copyToClipboard(text);
            var $btn = $(this), orig = $btn.text();
            $btn.text('Copied!');
            setTimeout(function() { $btn.text(orig); }, 3000);
        });
        if (dataType === 'json') applyJsonHighlighting($block.find('pre'));
        return $block;
    }

    function applyJsonHighlighting($pre) {
        var html = $pre.html();
        html = html.replace(/(&quot;)([^&]+)(&quot;)(\s*:)/g, '<span class="json-key">$1$2$3</span>$4');
        html = html.replace(/(: )(&quot;)([^&]*?)(&quot;)/g, '$1<span class="json-string">$2$3$4</span>');
        html = html.replace(/(: )(-?\d+\.?\d*([eE][+-]?\d+)?)/g, '$1<span class="json-number">$2</span>');
        html = html.replace(/(: )(true|false|null)/g, '$1<span class="json-boolean">$2</span>');
        $pre.html(html);
    }

    function copyToClipboard(text) {
        if (navigator.clipboard && navigator.clipboard.writeText) {
            navigator.clipboard.writeText(text).catch(function() { fallbackCopy(text); });
        } else { fallbackCopy(text); }
    }

    function fallbackCopy(text) {
        var textarea = document.createElement('textarea');
        textarea.value = text;
        textarea.style.position = 'fixed';
        textarea.style.opacity = '0';
        document.body.appendChild(textarea);
        textarea.select();
        try { document.execCommand('copy'); } catch(e) {}
        document.body.removeChild(textarea);
    }

    function clearOutput() {
        if (dom.$output && dom.$output.length) {
            dom.$output.html('<span class="output-placeholder">' + rc.tr('Output will appear here after running scripts...') + '</span>');
        }
    }

    // =====================================================================
    // FUNCTION DETAIL POPUP
    // =====================================================================

		/**
		 * Shows a popup dialog with function details including parameters,
		 * return type, example, and insert options.
		 * 
		 * @param {Object} fnData - Function data object
		 * @returns {void}
		 */
		function showFunctionPopup(fnData) {
				// Remove any existing popup
				$('#ref-detail-popup').remove();
				$(document).off('keydown.refPopup');

				var example = fnData.example || fnData.firstExample || '';
				
				// Generate a cache ID for the example if it exists
				var exampleCacheId = null;
				if (example && example.trim()) {
						exampleCacheId = 'ref_ex_' + (++exampleCounter) + '_' + Date.now();
						exampleCodeCache[exampleCacheId] = example;
						setTimeout(function() {
								if (exampleCodeCache[exampleCacheId]) {
										delete exampleCodeCache[exampleCacheId];
								}
						}, 300000);
				}
				
				// Get total lines for placeholder
				var totalLines = codeMirrorInstance ? codeMirrorInstance.lineCount() : 1000;
				var currentLine = getCurrentLineNumber();
				var linePlaceholder = '1-' + totalLines;
				
				var funcName = fnData.signature ? fnData.signature.split('(')[0] : (fnData.fullName || fnData.prefix || fnData.name || 'function');
				var signature = fnData.fullSignature || fnData.signature || fnData.prefix || funcName;
				var description = fnData.description || fnData.desc || 'No description available.';
				var moduleName = fnData.moduleName || '';
				var returnType = (fnData.returnType && fnData.returnType.type) ? fnData.returnType.type : (fnData.returnType || 'void');
				var params = fnData.parameters || [];
				
				// Escape all content
				var escapedFuncName = escapeHtml(funcName);
				var escapedSignature = escapeHtml(signature);
				var escapedDescription = escapeHtml(description);
				var escapedModuleName = escapeHtml(moduleName);
				var escapedReturnType = escapeHtml(returnType);
				
				// Build parameters HTML
				var paramsHtml = '';
				if (params && params.length > 0) {
						paramsHtml = '<div class="ref-popup-section"><div class="ref-popup-section-title">Parameters</div>';
						for (var i = 0; i < params.length; i++) {
								var p = params[i];
								var pName = escapeHtml(p.name || '');
								var pType = escapeHtml(p.type || '');
								var pDesc = escapeHtml(p.description || p.desc || '');
								var isRequired = p.required === true;
								
								paramsHtml += '<div class="ref-popup-param">';
								paramsHtml += '<span class="param-name">' + pName + '</span>';
								paramsHtml += '<span class="param-type">' + pType + '</span>';
								if (isRequired) {
										paramsHtml += '<span class="param-badge required">required</span>';
								} else {
										paramsHtml += '<span class="param-badge optional">optional</span>';
								}
								if (pDesc) {
										paramsHtml += '<div class="param-desc">' + pDesc + '</div>';
								}
								paramsHtml += '</div>';
						}
						paramsHtml += '</div>';
				}
				
				// Build example HTML with buttons
				var exampleHtml = '';
				if (example && example.trim()) {
						var formattedExample = formatExampleForDisplay(example);
						var escapedExample = escapeHtml(formattedExample);
						
						exampleHtml = '<div class="ref-popup-section">' +
								'<div class="ref-popup-section-title">Example</div>' +
								'<pre class="ref-popup-code ref-popup-example-code">' + escapedExample + '</pre>' +
								'<div class="ref-popup-example-actions">' +
								'<div class="ref-detail-footer">' +
								'<button class="ref-popup-insert-example jquerybutton" data-example-cache-id="' + (exampleCacheId || '') + '">Insert Example (at cursor)</button>' +
								'<div class="ref-popup-insert-line-container">' +
								'<span class="ref-popup-line-label">at line:</span>' +
								'<input type="number" class="ref-popup-line-input" value="' + currentLine + '" min="1" max="' + totalLines + '" placeholder="' + linePlaceholder + '">' +
								'<button class="ref-popup-insert-at-line jquerybutton" data-example-cache-id="' + (exampleCacheId || '') + '">Insert at Line</button>' +
								'</div>' +
								'</div>' +
								'</div>' +
								'</div>';
				}
				
				var overlay = $(
						'<div id="ref-detail-popup" class="ref-detail-overlay">' +
						'<div class="ref-detail-dialog">' +
						'<div class="ref-detail-header">' +
						'<div class="ref-detail-title">' + escapedFuncName + '</div>' +
						'<button class="ref-detail-close">&times;</button>' +
						'</div>' +
						'<div class="ref-detail-body">' +
						(moduleName ? '<div class="ref-detail-module">Module: ' + escapedModuleName + '</div>' : '') +
						'<div class="ref-popup-section">' +
						'<div class="ref-popup-section-title">Signature</div>' +
						'<pre class="ref-popup-code">' + escapedSignature + '</pre>' +
						'</div>' +
						'<div class="ref-popup-section">' +
						'<div class="ref-popup-section-title">Description</div>' +
						'<div class="ref-popup-text">' + escapedDescription + '</div>' +
						'</div>' +
						paramsHtml +
						(returnType && returnType !== 'void' ? '<div class="ref-detail-returns">Returns: <strong>' + escapedReturnType + '</strong></div>' : '') +
						exampleHtml +
						'<button class="jquerybutton ref-popup-close-btn">Close</button>' +
						'</div>' +
						'</div>' +
						'</div>'
				);

				$('body').append(overlay);

				var closePopup = function() {
						overlay.remove();
						$(document).off('keydown.refPopup');
				};

				$('.ref-detail-close, .ref-popup-close-btn').on('click', closePopup);

				overlay.on('click', function(e) {
						if (e.target === overlay[0]) closePopup();
				});

				// Handle Insert Example at Cursor button
				overlay.find('.ref-popup-insert-example').on('click', function() {
						var $btn = $(this);
						var cacheId = $btn.data('example-cache-id');
						var exampleCode = cacheId && exampleCodeCache[cacheId] ? exampleCodeCache[cacheId] : example;
						
						if (exampleCode && exampleCode.trim()) {
								insertAtCursor(exampleCode);
								addOutput('success', 'Example inserted at cursor');
								closePopup();
						} else {
								addOutput('error', 'No example available for this function');
						}
				});

				// Handle Insert at Line button
				overlay.find('.ref-popup-insert-at-line').on('click', function() {
						var $btn = $(this);
						var $lineInput = overlay.find('.ref-popup-line-input');
						var targetLine = parseInt($lineInput.val(), 10);
						var cacheId = $btn.data('example-cache-id');
						var exampleCode = cacheId && exampleCodeCache[cacheId] ? exampleCodeCache[cacheId] : example;
						
						if (!exampleCode || !exampleCode.trim()) {
								addOutput('error', 'No example available for this function');
								return;
						}
						
						if (isNaN(targetLine) || targetLine < 1) {
								targetLine = currentLine;
						}
						
						insertExampleAtLine(exampleCode, targetLine);
						
						var originalText = $btn.html();
						$btn.html('Inserted');
						setTimeout(function() {
								$btn.html(originalText);
						}, 1500);
						
						closePopup();
				});

				// Handle Enter key in line number input
				overlay.find('.ref-popup-line-input').on('keydown', function(e) {
						if (e.key === 'Enter') {
								e.preventDefault();
								e.stopPropagation();
								overlay.find('.ref-popup-insert-at-line').click();
						}
				});

				$(document).on('keydown.refPopup', function(e) {
						if (e.key === 'Escape') closePopup();
				});
		}

    // =====================================================================
    // SCRIPT STATUS - Listens to activeScriptChanged from ui/scripts.js
    // =====================================================================

    function refreshAllUIStates() {
        // Run buttons: disabled when script is running
        if (dom.$btnRun) {
            dom.$btnRun.prop('disabled', isRunning);
            dom.$btnRun.toggleClass('ui-state-disabled', isRunning);
        }
        if (dom.$btnRunSelection) {
            dom.$btnRunSelection.prop('disabled', isRunning);
            dom.$btnRunSelection.toggleClass('ui-state-disabled', isRunning);
        }
        
        // Stop button: enabled ONLY when script is running
        if (dom.$btnStop) {
            dom.$btnStop.prop('disabled', !isRunning);
            dom.$btnStop.toggleClass('ui-state-disabled', !isRunning);
        }
        
        // Editor visual state
        var $container = $('#script-editor-container');
        if ($container.length) {
            if (isRunning) {
                $container.addClass('editor-running');
            } else {
                $container.removeClass('editor-running');
            }
        }
        
        // Status text
        var $statusText = $('#script-status-text');
        var $statusIndicator = $('#script-status-indicator');
        
        if ($statusText.length) {
            if (isRunning && currentRunningScriptId) {
                $statusText.text('Running: ' + currentRunningScriptId);
            } else if (isRunning && currentRunningScriptId === null) {
                $statusText.text('Running direct script...');
            } else if (isRunning) {
                $statusText.text('Script is running...');
            } else {
                $statusText.text('No script running');
            }
        }
        
        if ($statusIndicator && $statusIndicator.length) {
            $statusIndicator.toggleClass('running', isRunning);
        }
        
        // Update installed scripts buttons
        updateInstalledScriptButtons();
    }

    function updateInstalledScriptButtons() {
        $('.script-item').each(function() {
            var $runBtn = $(this).find('.script-run-btn');
            if ($runBtn.length) {
                $runBtn.prop('disabled', isRunning);
                $runBtn.css('opacity', isRunning ? '0.4' : '1');
                $runBtn.attr('title', isRunning ? 'Stop current script first' : 'Run Script');
            }
        });
    }

    // =====================================================================
    // SCRIPT EXECUTION
    // =====================================================================

    function getEditorContent() {
        if (codeMirrorInstance) return codeMirrorInstance.getValue();
        var ta = document.getElementById('script-editor');
        return ta ? ta.value : '';
    }

    function setEditorContent(content) {
        if (codeMirrorInstance) { codeMirrorInstance.setValue(content); codeMirrorInstance.refresh(); }
        else { var ta = document.getElementById('script-editor'); if (ta) ta.value = content; }
    }

    function getSelectedText() {
        if (codeMirrorInstance) {
            var sel = codeMirrorInstance.getSelection();
            if (sel && sel.trim()) {
                console.log('[ScriptEditor] getSelectedText (CodeMirror):', sel.substring(0, 50));
                return sel;
            }
            // If no selection, get current line
            var line = codeMirrorInstance.getLine(codeMirrorInstance.getCursor().line);
            console.log('[ScriptEditor] getSelectedText (current line):', line.substring(0, 50));
            return line;
        }
        var ta = document.getElementById('script-editor');
        if (ta) {
            var s = ta.selectionStart, e = ta.selectionEnd;
            if (s === e) {
                var v = ta.value, ls = v.lastIndexOf('\n', s - 1) + 1, le = v.indexOf('\n', e);
                if (le === -1) le = v.length;
                var line = v.substring(ls, le);
                console.log('[ScriptEditor] getSelectedText (textarea line):', line.substring(0, 50));
                return line;
            }
            var selected = ta.value.substring(s, e);
            console.log('[ScriptEditor] getSelectedText (textarea selection):', selected.substring(0, 50));
            return selected;
        }
        return '';
    }

		function runScript() {
				var code = getEditorContent().trim();
				if (!code) { 
						addOutput('error', 'No code to run.');
						showNotification(_tr("No code to run"), "error");
						return; 
				}
				if (isRunning) { 
						addOutput('error', 'A script is already running. Stop it first.'); 
						showNotification(_tr("Script already running"), "error");
						return; 
				}
				
				addOutput('info', '▶ Sending script for execution...');
				showNotification(_tr("Running script..."), "info");
				isRunning = true; 
				currentRunningScriptId = null; 
				refreshAllUIStates();
				sendCode(code);
				startScriptPolling();
		}

    function runSelection() {
        var code = getSelectedText().trim();
        if (!code) { addOutput('error', 'No code selected.'); return; }
        if (isRunning) { addOutput('error', 'A script is already running.'); return; }
        addOutput('info', 'Running selected code...');
        isRunning = true; currentRunningScriptId = null; refreshAllUIStates();
        sendCode(code);
        startScriptPolling();
    }

    function sendCode(code) {
        if (typeof scriptApi !== 'undefined' && scriptApi.runDirectScript) {
            scriptApi.runDirectScript(code, true);
        } else if (typeof rc !== 'undefined' && rc.postCmd) {
            rc.postCmd('/api/scripts/direct', { code: code, useIncludes: true }, null, function(data) {
                if (data && data.trim() && data.trim() !== 'ok') { 
                    addOutput('success', 'Response received:', data); 
                }
            });
        }
        addToHistory(code);
        
        // Force status check after delays to catch completion
        setTimeout(function() {
            $.ajax({
                url: '/api/scripts/status',
                method: 'GET',
                dataType: 'json',
                timeout: 3000
            }).done(function(data) {
                var running = data.scriptIsRunning || false;
                var scriptName = data.runningScriptId || '';
                
                if (!running && isRunning) {
                    // Script has already finished
                    isRunning = false;
                    currentRunningScriptId = null;
                    refreshAllUIStates();
                    addOutput('success', 'Script execution completed.');
                } else if (running && !isRunning) {
                    // Script is still running (update state)
                    isRunning = true;
                    currentRunningScriptId = scriptName || null;
                    refreshAllUIStates();
                    // Continue checking
                    scheduleDirectScriptCheck();
                }
            });
        }, 3000);
    }
    
    /**
     * Continuously polls /api/scripts/status when a direct script is running.
     * Unlike installed scripts (which have a scriptId), direct scripts from the
     * editor have no server-side identifier. This function fills that gap by
     * polling until the script completes or isForceStopped.
     * 
     * Uses exponential backoff for polling interval:
     * - First 10 checks: every 2 seconds
     * - After 10 checks: every 5 seconds
     * - After 30 checks: every 10 seconds
     * This prevents flooding the server during long-running scripts.
     * 
     * The polling stops ONLY when:
     * 1. The server reports scriptIsRunning === false
     * 2. isRunning is set to false externally (user clicks Stop)
     */
    function scheduleDirectScriptCheck() {
        var checkCount = 0;
        
        function getPollInterval() {
            if (checkCount < 10) return 2000;   // First 20 seconds: check every 2s
            if (checkCount < 30) return 5000;   // Next 100 seconds: check every 5s
            return 10000;                         // After that: check every 10s
        }
        
        function check() {
            // Stop polling if script was stopped by user or module was reset
            if (!isRunning) return;
            
            checkCount++;
            
            setTimeout(function() {
                // Double-check isRunning hasn't changed during timeout
                if (!isRunning) return;
                
                $.ajax({
                    url: '/api/scripts/status',
                    method: 'GET',
                    dataType: 'json',
                    timeout: 3000
                }).done(function(data) {
                    var running = data.scriptIsRunning || false;
                    var scriptName = data.runningScriptId || '';
                    
                    if (!running && isRunning) {
                        // Script has finished
                        console.log('[ScriptEditor] Direct script completed (check #' + checkCount + ')');
                        isRunning = false;
                        currentRunningScriptId = null;
                        refreshAllUIStates();
                        addOutput('success', 'Script execution completed.');
                        // Do NOT schedule another check - we're done
                    } else if (running && isRunning) {
                        // Still running, schedule next check with backoff
                        console.log('[ScriptEditor] Direct script still running (check #' + checkCount + ', next in ' + (getPollInterval()/1000) + 's)');
                        check();
                    }
                    // If !running && !isRunning: already stopped, nothing to do
                }).fail(function(xhr, status, error) {
                    console.warn('[ScriptEditor] Status check failed (#' + checkCount + '):', status);
                    // On network error, retry with longer delay but don't stop polling
                    if (isRunning) check();
                });
            }, getPollInterval());
        }
        
        console.log('[ScriptEditor] Starting continuous polling for direct script...');
        check();
    }

		function stopScript() {
				if (!isRunning) { 
						addOutput('info', 'No script is currently running.'); 
						showNotification(_tr("No script running"), "info");
						return; 
				}
				addOutput('info', 'Stopping script...');
				showNotification(_tr("Stopping script..."), "info");
				if (typeof scriptApi !== 'undefined' && scriptApi.stopScript) scriptApi.stopScript();
				else if (typeof rc !== 'undefined' && rc.postCmd) rc.postCmd('/api/scripts/stop', {});
				isRunning = false; 
				currentRunningScriptId = null; 
				refreshAllUIStates();
				startScriptPolling();
		}

    function runInstalledScript(scriptId) {
        if (isRunning) { addOutput('error', 'A script is already running.'); return; }
        addOutput('info', 'Running installed script: ' + scriptId);
        isRunning = true; currentRunningScriptId = scriptId; refreshAllUIStates();
        if (typeof scriptApi !== 'undefined' && scriptApi.runScript) scriptApi.runScript(scriptId);
    }

    // =====================================================================
    // FILE OPERATIONS
    // =====================================================================

    function saveScriptToFile() {
        var code = getEditorContent();
        if (!code || !code.trim()) { addOutput('error', 'No code to save.'); return; }
        var defaultName = 'stellarium_script_' + new Date().toISOString().slice(0, 19).replace(/:/g, '-') + '.ssc';
        var nm = code.match(/\/\/\s*Name:\s*(.+)/i);
        if (nm && nm[1]) {
            var sn = nm[1].trim().replace(/[^a-zA-Z0-9\s]/g, '').replace(/\s+/g, '_').toLowerCase();
            if (sn) defaultName = sn + '.ssc';
        }
        var blob = new Blob([code], { type: 'text/plain;charset=utf-8' });
        var url = URL.createObjectURL(blob), a = document.createElement('a');
        a.href = url; a.download = defaultName;
        document.body.appendChild(a); a.click(); document.body.removeChild(a);
        URL.revokeObjectURL(url);
        addOutput('success', 'Script saved: ' + defaultName);
    }

    function openScriptFromFile() {
        var fi = $('#script-editor-file-input');
        if (!fi.length) { fi = $('<input type="file" id="script-editor-file-input" accept=".ssc,.js,.txt" style="display:none;">'); $('body').append(fi); }
        fi.off('change').on('change', function(e) {
            var file = e.target.files[0]; if (!file) return;
            var reader = new FileReader();
            reader.onload = function(ev) {
                var content = ev.target.result;
                if (!content || !content.trim()) { addOutput('error', 'Empty file.'); fi.val(''); return; }
                if (getEditorContent().trim() && !confirm(rc.tr('Replace current content?'))) { fi.val(''); return; }
                setEditorContent(content);
								refreshLineCount();
                try { localStorage.setItem('stellarium-script-editor', content); } catch(ex) {}
                addOutput('success', 'Loaded: ' + file.name);
                fi.val('');
            };
            reader.onerror = function() { addOutput('error', 'Failed to read file.'); fi.val(''); };
            reader.readAsText(file, 'UTF-8');
        });
        fi.click();
    }

		/**
		 * Copies the entire content of the editor to the clipboard.
		 */
		function copyEditorContent() {
				var content = '';
				if (codeMirrorInstance) {
						content = codeMirrorInstance.getValue();
				} else {
						var ta = document.getElementById('script-editor');
						if (ta) content = ta.value;
				}
				
				if (!content || content.trim() === '') {
						addOutput('error', 'Nothing to copy - editor is empty');
						showNotification(_tr("Editor is empty, nothing to copy"), "error");
						return;
				}
				
				copyToClipboard(content);
				
				var charCount = content.length;
				var lineCount = content.split('\n').length;
				var message = _tr("Copied") + " " + charCount + " " + _tr("characters") + " (" + lineCount + " " + _tr("lines") + ")";
				
				addOutput('success', message);
				showNotification(message, "success");
		}

    // =====================================================================
    // INSTALLED SCRIPTS
    // =====================================================================

    function loadInstalledScripts() {
        if (!dom.$scriptsList || !dom.$scriptsList.length) return;
        if (typeof scriptApi !== 'undefined' && scriptApi.loadScriptList) {
            scriptApi.loadScriptList(function(data) {
                var html = '';
                if (data && data.length) {
                    for (var i = 0; i < data.length; i++) {
                        var script = data[i], name = script.split('/').pop();
                        html += '<div class="script-item"><span class="script-icon">S</span>';
                        html += '<span class="script-name" title="' + escapeAttr(script) + '">' + escapeHtml(name) + '</span>';
                        html += '<div class="script-item-actions">';
                        html += '<button class="script-action-btn script-run-btn" data-script="' + escapeAttr(script) + '" title="Run Script">&#9654;</button>';
                        html += '<button class="script-action-btn script-load-btn" data-script="' + escapeAttr(script) + '" title="Load into Editor">&#128196;</button>';
                        html += '</div></div>';
                    }
                } else { html = '<div class="loading-placeholder">' + rc.tr('No scripts available') + '</div>'; }
                dom.$scriptsList.html(html);
                dom.$scriptsList.find('.script-run-btn').on('click', function(e) { e.stopPropagation(); runInstalledScript($(this).data('script')); });
                dom.$scriptsList.find('.script-load-btn').on('click', function(e) { e.stopPropagation(); loadInstalledScriptContent($(this).data('script')); });
                updateInstalledScriptButtons();
            });
        } else { dom.$scriptsList.html('<div class="loading-placeholder">' + rc.tr('Script API not available') + '</div>'); }
    }

    function loadInstalledScriptContent(scriptId) {
        $.ajax({
            url: '/api/scripts/info', data: { id: scriptId, html: 'false' }, dataType: 'json',
            success: function(data) {
                addOutput('info', 'Script: ' + scriptId);
                if (data && data.description) addOutput('info', 'Description: ' + data.description);
            },
            error: function() { addOutput('error', 'Failed to load script info.'); }
        });
    }

    // =====================================================================
    // EXECUTION HISTORY
    // =====================================================================

    function addToHistory(code) {
        executionHistory.unshift({ time: new Date(), code: code, preview: code.substring(0, 100).replace(/\n/g, ' ') });
        if (executionHistory.length > maxHistoryEntries) executionHistory.pop();
        saveHistory(); renderHistory();
    }

    function renderHistory() {
        if (!dom.$history || !dom.$history.length) return;
        if (!executionHistory.length) { dom.$history.html('<span class="output-placeholder">' + rc.tr('No execution history yet') + '</span>'); return; }
        var html = '';
        for (var i = 0; i < executionHistory.length; i++) {
            var item = executionHistory[i];
            html += '<div class="history-item" data-index="' + i + '"><div class="history-time">' + item.time.toLocaleTimeString() + '</div><div class="history-code">' + escapeHtml(item.preview) + '</div></div>';
        }
        dom.$history.html(html);
        dom.$history.find('.history-item').on('click', function() {
            var idx = parseInt($(this).data('index'));
            if (executionHistory[idx]) { setEditorContent(executionHistory[idx].code); if (codeMirrorInstance) codeMirrorInstance.focus(); }
        });
    }

    function saveHistory() {
        try {
            var toSave = executionHistory.map(function(i) { return { time: i.time.getTime(), code: i.code, preview: i.preview }; });
            localStorage.setItem('stellarium-script-history', JSON.stringify(toSave));
        } catch(e) {}
    }

    function loadHistory() {
        try {
            var saved = localStorage.getItem('stellarium-script-history');
            if (saved) { executionHistory = JSON.parse(saved).map(function(i) { i.time = new Date(i.time); return i; }); renderHistory(); }
        } catch(e) {}
    }

    // =====================================================================
    // QUICK REFERENCE (DYNAMIC + SEARCH + POPUP)
    // =====================================================================

		/**
		 * Build the Quick Reference panel using unified data from scriptReference.
		 * Displays categorized functions and snippets with search functionality.
		 * Supports example insertion at cursor or specific line numbers.
		 */
		function buildQuickReference() {
				var $quickRef = $('#quick-reference');
				if (!$quickRef.length) return;

				$quickRef.empty();

				if (!scriptReference.isLoaded()) {
						$quickRef.html('<div class="loading-placeholder">' + _tr("Loading API reference...") + '</div>');
						setTimeout(buildQuickReference, 500);
						return;
				}

				var categorizedData = scriptReference.getCategorizedReference();
				var moduleIds = Object.keys(categorizedData);
				
				// Get snippets from scriptReference
				var snippets = [];
				if (typeof scriptReference.getSnippets === 'function') {
						snippets = scriptReference.getSnippets();
				}
				console.log('[QuickRef] Loading ' + snippets.length + ' snippets');
				
				var html = '';
				
				// Add function categories
				for (var i = 0; i < moduleIds.length; i++) {
						var moduleId = moduleIds[i];
						var category = categorizedData[moduleId];
						var items = category.items || [];
						
						if (items.length === 0) continue;
						
						var displayName = category.name;
						displayName = displayName.replace(' Manager', '').replace(' Plugin', '').replace(' API', '');
						
						html += '<div class="ref-category" data-category-id="' + escapeAttr(moduleId) + '">';
						html += '<div class="ref-cat-header" data-cat="' + escapeAttr(moduleId) + '">';
						html += escapeHtml(displayName);
						html += ' <span class="ref-cat-count">(' + items.length + ')</span>';
						html += '</div>';
						html += '<div class="ref-cat-items" id="ref-' + escapeAttr(moduleId) + '"></div>';
						html += '</div>';
				}
				
				// Add snippets section
				html += '<div class="ref-category ref-category-snippets" data-category-id="snippets">';
				html += '<div class="ref-cat-header" data-cat="snippets">';
				html += _tr('Code Snippets');
				
				if (snippets.length > 0) {
						html += ' <span class="ref-cat-count">(' + snippets.length + ')</span>';
				}
				html += '</div>';
				html += '<div class="ref-cat-items" id="ref-snippets"></div>';
				html += '</div>';
				
				$quickRef.html(html);
				
				// Populate function items for each category
				for (var j = 0; j < moduleIds.length; j++) {
						var moduleId = moduleIds[j];
						var category = categorizedData[moduleId];
						var items = category.items || [];
						
						if (items.length === 0) continue;
						
						var $container = $('#ref-' + moduleId);
						var categoryHtml = '';
						
						for (var k = 0; k < items.length; k++) {
								var fn = items[k];
								var funcName = fn.fullName || fn.prefix;
								var shortDesc = fn.desc || '';
								
								if (shortDesc.length > 80) {
										shortDesc = shortDesc.substring(0, 77) + '...';
								}
								
								var fnDataJson = escapeAttr(JSON.stringify({
										name: fn.name,
										fullName: fn.fullName,
										signature: fn.signature,
										fullSignature: fn.fullSignature,
										description: fn.desc,
										parameters: fn.parameters,
										returnType: fn.returnType,
										examples: fn.examples,
										firstExample: fn.firstExample,
										moduleId: fn.moduleId,
										moduleName: fn.moduleName,
										category: fn.category,
										tags: fn.tags
								}));
								
								categoryHtml += '<div class="ref-item" data-func-data="' + fnDataJson + '">';
								categoryHtml += '<span class="ref-name">' + escapeHtml(funcName) + '</span>';
								categoryHtml += '<span class="ref-desc">' + escapeHtml(shortDesc) + '</span>';
								categoryHtml += '</div>';
						}
						
						$container.html(categoryHtml);
				}
				
				// Populate snippets section
				var $snippetsContainer = $('#ref-snippets');
				if ($snippetsContainer.length) {
						if (snippets.length === 0) {
								$snippetsContainer.html('<div class="loading-placeholder">' + _tr('No snippets available') + '</div>');
						} else {
								var snippetsHtml = '';
								for (var s = 0; s < snippets.length; s++) {
										var snippet = snippets[s];
										var snippetDataJson = escapeAttr(JSON.stringify({
												id: snippet.id,
												name: snippet.name,
												code: snippet.code,
												description: snippet.description,
												type: 'snippet'
										}));
										
										snippetsHtml += '<div class="ref-item ref-snippet-item" data-snippet-data="' + snippetDataJson + '">';
										snippetsHtml += '<span class="ref-name">' + escapeHtml(snippet.name) + '</span>';
										snippetsHtml += '<span class="ref-desc">' + escapeHtml((snippet.description || '').substring(0, 80)) + '</span>';
										snippetsHtml += '</div>';
								}
								$snippetsContainer.html(snippetsHtml);
						}
				}
				
				bindQuickReferenceEvents();
				setupQuickReferenceSearch();
				
				console.log('[QuickRef] Built with ' + moduleIds.length + ' categories and ' + snippets.length + ' snippets');
		}

		/**
		 * Setup search/filter functionality for Quick Reference
		 */
		function setupQuickReferenceSearch() {
				var $searchInput = $('#quick-reference-search');
				if (!$searchInput.length) return;
				
				$searchInput.off('input').on('input', function() {
						var query = $(this).val().toLowerCase().trim();
						filterQuickReference(query);
				});
		}

		/**
		 * Filter Quick Reference items based on search query
		 * 
		 * @param {string} query - Search query
		 */
		function filterQuickReference(query) {
				var $quickRef = $('#quick-reference');
				if (!$quickRef.length) return;
				
				var hasResults = false;
				
				if (query === '') {
						// Show all categories and collapse them
						$quickRef.find('.ref-category').show();
						$quickRef.find('.ref-cat-items').removeClass('expanded');
						$quickRef.find('.ref-cat-header').removeClass('expanded');
						$quickRef.find('.ref-item').show();
						return;
				}
				
				// Search through all items
				$quickRef.find('.ref-category').each(function() {
						var $category = $(this);
						var $header = $category.find('.ref-cat-header');
						var catName = $header.text().toLowerCase();
						var $items = $category.find('.ref-item');
						var anyMatch = false;
						
						// Check category name match
						var catMatches = catName.indexOf(query) >= 0;
						
						// Check each item
						$items.each(function() {
								var $item = $(this);
								var itemText = $item.text().toLowerCase();
								var itemMatches = itemText.indexOf(query) >= 0;
								
								if (itemMatches) {
										$item.show();
										anyMatch = true;
								} else {
										$item.hide();
								}
						});
						
						if (catMatches || anyMatch) {
								$category.show();
								hasResults = true;
								// Expand category if it has matching items
								if (anyMatch) {
										$category.find('.ref-cat-items').addClass('expanded');
										$header.addClass('expanded');
								}
						} else {
								$category.hide();
						}
				});
				
				// Show no results message if needed
				if (!hasResults) {
						var $noResults = $quickRef.find('.no-results-message');
						if ($noResults.length === 0) {
								$quickRef.append('<div class="no-results-message" style="padding: 20px; text-align: center; color: #8A8C8E;">' + _tr('No matching functions found') + '</div>');
						} else {
								$noResults.show();
						}
				} else {
						$quickRef.find('.no-results-message').remove();
				}
		}

		function bindQuickReferenceEvents() {
				// Category toggle (expand/collapse)
				$('#quick-reference').off('click', '.ref-cat-header').on('click', '.ref-cat-header', function(e) {
						e.stopPropagation();
						var $header = $(this);
						var $category = $header.closest('.ref-category');
						var $itemsContainer = $category.find('.ref-cat-items');
						
						$header.toggleClass('expanded');
						$itemsContainer.toggleClass('expanded');
				});
				
				// Function item click - show popup with details
				$('#quick-reference').off('click', '.ref-item').on('click', '.ref-item', function(e) {
						e.stopPropagation();
						var $item = $(this);
						
						console.log('[QuickRef] Item clicked:', $item);
						
						// ============================================================
						// CASE 1: Snippet item
						// ============================================================
						var snippetData = $item.data('snippet-data');
						if (snippetData) {
								console.log('[QuickRef] Snippet clicked:', snippetData);
								showFunctionPopup({
										signature: snippetData.name,
										description: snippetData.description || '',
										example: snippetData.code,
										moduleName: 'Code Snippet',
										parameters: [],
										returnType: 'snippet'
								});
								return;
						}
						
						// ============================================================
						// CASE 2: Snippet from old system (data-snippet)
						// ============================================================
						var snippetKey = $item.data('snippet');
						if (snippetKey && codeSnippets[snippetKey]) {
								var snippet = codeSnippets[snippetKey];
								showFunctionPopup({
										signature: snippet.name,
										description: snippet.desc,
										example: snippet.code,
										moduleName: 'Code Snippet',
										parameters: [],
										returnType: 'snippet'
								});
								return;
						}
						
						// ============================================================
						// CASE 3: Function item from JSON (data-func-data)
						// ============================================================
						var funcData = $item.data('func-data');
						if (funcData) {
								console.log('[QuickRef] Function from data-func-data:', funcData);
								showFunctionPopup(funcData);
								return;
						}
						
						// ============================================================
						// CASE 4: Function item from built-in (data-func-index)
						// ============================================================
						var funcIndex = $item.data('func-index');
						if (funcIndex && scriptReference.isLoaded()) {
								console.log('[QuickRef] Function from index:', funcIndex);
								var parts = funcIndex.split(':');
								var moduleId = parts[0];
								var idx = parseInt(parts[1]);
								var funcs = scriptReference.getModuleFunctions(moduleId);
								if (funcs && funcs[idx]) {
										showFunctionPopup(funcs[idx]);
								}
								return;
						}
						
						// ============================================================
						// CASE 5: Built-in index from old system
						// ============================================================
						var builtinIndex = $item.data('builtin-index');
						if (builtinIndex) {
								console.log('[QuickRef] Built-in index:', builtinIndex);
								var parts = builtinIndex.split(':');
								var cat = parts[0];
								var idx = parseInt(parts[1]);
								var catItems = [];
								for (var i = 0; i < autocompleteItems.length; i++) {
										if (autocompleteItems[i].category === cat) catItems.push(autocompleteItems[i]);
								}
								if (catItems[idx]) {
										showFunctionPopup(catItems[idx]);
								}
								return;
						}
						
						// ============================================================
						// CASE 6: Try to find by name
						// ============================================================
						var funcName = $item.find('.ref-name').text();
						if (funcName && scriptReference.isLoaded()) {
								console.log('[QuickRef] Looking by name:', funcName);
								var allItems = scriptReference.getAutocompleteItems();
								for (var i = 0; i < allItems.length; i++) {
										var item = allItems[i];
										var itemName = item.fullName || item.prefix;
										if (itemName === funcName || item.name === funcName) {
												showFunctionPopup(item);
												return;
										}
								}
						}
						
						console.warn('[QuickRef] Could not find data for clicked item');
				});
		}

		/**
		 * Build quick reference from built-in data (fallback when JSON fails)
		 */
		function buildQuickReferenceFromBuiltIn() {
				console.log('[ScriptEditor] Building quick reference from built-in data');
				
				// Get autocomplete items from scriptReference (which will use built-in fallback)
				var autocompleteItems = scriptReference.getAutocompleteItems();
				
				// Group by category
				var cats = {};
				for (var i = 0; i < autocompleteItems.length; i++) {
						var item = autocompleteItems[i];
						var cat = item.category || 'other';
						if (!cats[cat]) cats[cat] = [];
						cats[cat].push(item);
				}
				
				var $quickRef = $('#quick-reference');
				$quickRef.empty();
				
				var catKeys = Object.keys(cats);
				for (var c = 0; c < catKeys.length; c++) {
						var cat = catKeys[c];
						var items = cats[cat];
						
						var $category = $(
								'<div class="ref-category">' +
								'<div class="ref-cat-header" data-cat="' + cat + '">' +
								escapeHtml(cat.charAt(0).toUpperCase() + cat.slice(1)) +
								' <span class="ref-cat-count">(' + items.length + ')</span>' +
								'</div>' +
								'<div class="ref-cat-items" id="ref-' + cat + '"></div>' +
								'</div>'
						);
						
						$quickRef.append($category);
						
						var $container = $('#ref-' + cat);
						var html = '';
						
						for (var j = 0; j < items.length; j++) {
								var it = items[j];
								var fnName = it.fullName || it.prefix;
								
								// Store function data
								var fnDataJson = escapeAttr(JSON.stringify({
										name: it.name,
										fullName: it.fullName,
										signature: it.signature,
										description: it.desc,
										parameters: it.parameters,
										returnType: it.returnType,
										examples: it.examples,
										firstExample: it.firstExample,
										moduleId: it.moduleId,
										category: it.category
								}));
								
								html += '<div class="ref-item" data-func-data="' + fnDataJson + '">';
								html += '<span class="ref-name">' + escapeHtml(fnName) + '</span>';
								html += '<span class="ref-desc">' + escapeHtml((it.desc || '').substring(0, 80)) + '</span>';
								html += '</div>';
						}
						
						$container.html(html);
				}
				
				// Add snippets section if there are snippets
				var snippets = scriptReference.getSnippets();
				if (snippets.length > 0) {
						var $snippetsSection = $(
								'<div class="ref-category">' +
								'<div class="ref-cat-header" data-cat="snippets">' +
								_tr('Code Snippets') +
								' <span class="ref-cat-count">(' + snippets.length + ')</span>' +
								'</div>' +
								'<div class="ref-cat-items" id="ref-snippets"></div>' +
								'</div>'
						);
						
						$quickRef.append($snippetsSection);
						
						var $snippetsContainer = $('#ref-snippets');
						var snippetsHtml = '';
						
						for (var s = 0; s < snippets.length; s++) {
								var snippet = snippets[s];
								var snippetDataJson = escapeAttr(JSON.stringify({
										id: snippet.id,
										name: snippet.name,
										code: snippet.code,
										description: snippet.description,
										type: 'snippet'
								}));
								
								snippetsHtml += '<div class="ref-item ref-snippet-item" data-snippet-data="' + snippetDataJson + '">';
								snippetsHtml += '<span class="ref-name">' + escapeHtml(snippet.name) + '</span>';
								snippetsHtml += '<span class="ref-desc">' + escapeHtml((snippet.description || '').substring(0, 80)) + '</span>';
								snippetsHtml += '</div>';
						}
						
						$snippetsContainer.html(snippetsHtml);
				}
				
				bindQuickReferenceEvents();
				setupQuickReferenceSearch();
		}

    function buildSnippetsSection() {
        var $sc = $('#ref-snippets');
        if (!$sc || !$sc.length) return;

        var sk = Object.keys(codeSnippets);
        if (sk.length === 0) { $sc.html('<span style="color:rgb(150,150,150);font-size:10px;">No snippets available</span>'); return; }

        var sh = '';
        for (var k = 0; k < sk.length; k++) {
            var key = sk[k];
            var s = codeSnippets[key];
            sh += '<div class="ref-item" data-snippet="' + key + '">';
            sh += '<span class="ref-name">' + escapeHtml(s.name) + '</span>';
            sh += '<span class="ref-desc">' + escapeHtml(s.desc) + '</span>';
            sh += '</div>';
        }
        $sc.html(sh);
    }


    function insertAtCursor(text) {
        if (codeMirrorInstance) { codeMirrorInstance.replaceSelection(text); codeMirrorInstance.focus(); }
        else {
            var ta = document.getElementById('script-editor');
            if (ta) {
                var s = ta.selectionStart, e = ta.selectionEnd;
                ta.value = ta.value.substring(0, s) + text + ta.value.substring(e);
                ta.selectionStart = ta.selectionEnd = s + text.length;
                ta.focus();
            }
        }
    }
		
		// =====================================================================
    // GO TO LINE DIALOG
    // =====================================================================

		/**
		 * Shows a dialog to input a line number and jump to that line in CodeMirror.
		 * 
		 * @param {CodeMirror} cm - The CodeMirror instance
		 */
		function showGoToLineDialog(cm) {
				if (!cm) return;
				
				// Get total number of lines
				var totalLines = cm.lineCount();
				var currentLine = cm.getCursor().line + 1; // 1-indexed for user display
				
				// Create dialog overlay if it doesn't exist
				var $dialogOverlay = $('#goto-line-dialog');
				if (!$dialogOverlay.length) {
						$dialogOverlay = $(
								'<div id="goto-line-dialog" class="goto-line-overlay" style="display:none;position:fixed;top:0;left:0;right:0;bottom:0;background:rgba(0,0,0,0.5);z-index:10001;align-items:center;justify-content:center;">' +
								'<div class="goto-line-dialog" style="background:linear-gradient(#5D5F62, #3A3C3E);border:1px solid #2A2C2E;border-radius:6px;padding:15px 20px;min-width:300px;box-shadow:0 4px 12px rgba(0,0,0,0.4);">' +
								'<div style="margin-bottom:10px;font-weight:bold;color:#DCDBDA;">' + _tr("Go to Line") + '</div>' +
								'<div style="margin-bottom:10px;">' +
								'<input type="number" id="goto-line-input" class="goto-line-input" style="width:100%;padding:6px 10px;background:#3A3C3E;color:#DCDBDA;border:1px solid #5D5F62;border-radius:3px;font-family:monospace;font-size:12px;" placeholder="' + _tr("Line number") + ' (1 - ' + totalLines + ')">' +
								'</div>' +
								'<div style="display:flex;gap:10px;justify-content:flex-end;">' +
								'<button id="goto-line-cancel" class="jquerybutton" style="background:linear-gradient(#5D5F62, #3A3C3E);border:1px solid #2A2C2E;border-radius:3px;padding:4px 12px;font-size:11px;color:#000;cursor:pointer;">' + _tr("Cancel") + '</button>' +
								'<button id="goto-line-submit" class="jquerybutton" style="background:linear-gradient(#5D5F62, #3A3C3E);border:1px solid #2A2C2E;border-radius:3px;padding:4px 12px;font-size:11px;color:#000;cursor:pointer;">' + _tr("Go") + '</button>' +
								'</div>' +
								'</div>' +
								'</div>'
						);
						$('body').append($dialogOverlay);
				}
				
				// Update the placeholder with current line and total lines
				$('#goto-line-input').attr('placeholder', _tr("Line number") + ' (1 - ' + totalLines + ')');
				$('#goto-line-input').val(currentLine);
				$('#goto-line-input').focus();
				$('#goto-line-input').select();
				
				// Show the dialog
				$dialogOverlay.css('display', 'flex');
				
				// CRITICAL: Prevent Enter key from propagating to CodeMirror
				// This is the key fix - we must stop event propagation
				function preventEnterPropagation(e) {
						if (e.key === 'Enter') {
								e.preventDefault();
								e.stopPropagation();
								$('#goto-line-submit').click();
								return false;
						}
				}
				
				// Handle Go button click
				$('#goto-line-submit').off('click').on('click', function() {
						var lineNum = parseInt($('#goto-line-input').val(), 10);
						if (!isNaN(lineNum) && lineNum >= 1 && lineNum <= totalLines) {
								// Convert to 0-indexed for CodeMirror
								var lineIndex = lineNum - 1;
								
								// Scroll to line and set cursor WITHOUT creating a new line
								cm.scrollIntoView({line: lineIndex, ch: 0}, 50);
								cm.setCursor({line: lineIndex, ch: 0});
								
								// Focus the editor but keep cursor at the chosen line
								cm.focus();
								
								$dialogOverlay.hide();
						} else {
								// Show error feedback
								var $input = $('#goto-line-input');
								$input.css('border-color', '#F92672');
								setTimeout(function() {
										$input.css('border-color', '#5D5F62');
								}, 500);
						}
				});
				
				// Handle Cancel button click
				$('#goto-line-cancel').off('click').on('click', function() {
						$dialogOverlay.hide();
						cm.focus();
				});
				
				// Handle Enter key - use both keydown and keypress for reliability
				$('#goto-line-input').off('keydown.gotoLine').on('keydown.gotoLine', preventEnterPropagation);
				$('#goto-line-input').off('keypress.gotoLine').on('keypress.gotoLine', preventEnterPropagation);
				
				// Handle Escape key to close dialog
				$('#goto-line-input').off('keydown.gotoEscape').on('keydown.gotoEscape', function(e) {
						if (e.key === 'Escape') {
								e.preventDefault();
								e.stopPropagation();
								$dialogOverlay.hide();
								cm.focus();
								return false;
						}
				});
				
				// Close dialog when clicking outside
				$dialogOverlay.off('click').on('click', function(e) {
						if (e.target === $dialogOverlay[0]) {
								$dialogOverlay.hide();
								cm.focus();
						}
				});
		}

		// =====================================================================
    // KEYBOARD SHORTCUTS HELP DIALOG
    // =====================================================================

    /**
     * Shows a comprehensive keyboard shortcuts help dialog.
     * Displays all available shortcuts organized by category.
     * 
     * Categories include:
     * - Script Execution
     * - Editing
     * - Line Operations
     * - Cut, Copy, Paste (without selection)
     * - File Operations
     * - Search & Navigation
     * - Selection
     * 
     * @returns {void}
     */
    function showShortcutsHelpDialog() {
        // Remove existing dialog if any
        $('#shortcuts-help-dialog').remove();
        
        // Define all shortcuts with categories
        var shortcutCategories = [
            {
                name: "Script Execution",
                shortcuts: [
                    { keys: ["Ctrl", "Enter"], desc: "Run entire script" },
                    { keys: ["Ctrl", "Shift", "Enter"], desc: "Run selected code only" }
                ]
            },
            {
                name: "Editing",
                shortcuts: [
                    { keys: ["Ctrl", "Space"], desc: "Autocomplete suggestions" },
                    { keys: ["Ctrl", "/"], desc: "Toggle line comment" },
                    { keys: ["Tab"], desc: "Indent (insert spaces)" },
                    { keys: ["Ctrl", "]"], desc: "Increase indentation" },
                    { keys: ["Ctrl", "["], desc: "Decrease indentation" },
                    { keys: ["Ctrl", "Z"], desc: "Undo" },
                    { keys: ["Ctrl", "Y"], desc: "Redo" },
                    { keys: ["Ctrl", "Shift", "Z"], desc: "Redo (alternative)" }
                ]
            },
            {
                name: "Line Operations",
                shortcuts: [
                    { keys: ["Ctrl", "D"], desc: "Delete current line" },
                    { keys: ["Ctrl", "K"], desc: "Delete from cursor to end of line" },
                    { keys: ["Ctrl", "U"], desc: "Delete from cursor to start of line" },
                    { keys: ["Alt", "↑"], desc: "Move current line up" },
                    { keys: ["Alt", "↓"], desc: "Move current line down" },
                    { keys: ["Ctrl", "Shift", "↑"], desc: "Copy current line up" },
                    { keys: ["Ctrl", "Shift", "↓"], desc: "Copy current line down" },
                    { keys: ["Ctrl", "Shift", "K"], desc: "Delete line (alternative)" }
                ]
            },
            {
                name: "Cut, Copy, Paste (without selection)",
                shortcuts: [
                    { keys: ["Ctrl", "X"], desc: "Cut current line" },
                    { keys: ["Ctrl", "C"], desc: "Copy current line" },
                    { keys: ["Ctrl", "V"], desc: "Paste" }
                ]
            },
            {
                name: "File Operations",
                shortcuts: [
                    { keys: ["Ctrl", "S"], desc: "Save script to file" },
                    { keys: ["Ctrl", "O"], desc: "Open script from file" }
                ]
            },
            {
                name: "Search & Navigation",
                shortcuts: [
                    { keys: ["Ctrl", "F"], desc: "Find text" },
                    { keys: ["Ctrl", "H"], desc: "Find and replace" },
                    { keys: ["Ctrl", "G"], desc: "Go to line number" },
                    { keys: ["Ctrl", "L"], desc: "Go to line number (alternative)" },
                    { keys: ["Ctrl", "Home"], desc: "Go to beginning of document" },
                    { keys: ["Ctrl", "End"], desc: "Go to end of document" },
                    { keys: ["Home"], desc: "Go to beginning of line" },
                    { keys: ["End"], desc: "Go to end of line" }
                ]
            },
            {
                name: "Selection",
                shortcuts: [
                    { keys: ["Ctrl", "A"], desc: "Select all" },
                    { keys: ["Shift", "←/→"], desc: "Select by character" },
                    { keys: ["Shift", "↑/↓"], desc: "Select by line" },
                    { keys: ["Ctrl", "Shift", "←/→"], desc: "Select by word" }
                ]
            }
        ];
        
        // Build HTML
        var html = '';
        html += '<div id="shortcuts-help-dialog" class="shortcuts-dialog-overlay">';
        html += '<div class="shortcuts-dialog">';
        html += '<div class="shortcuts-dialog-header">';
        html += '<h3>⌨️ Keyboard Shortcuts</h3>';
        html += '<button class="shortcuts-dialog-close">&times;</button>';
        html += '</div>';
        html += '<div class="shortcuts-dialog-body">';
        html += '<p class="shortcuts-hint">Use these shortcuts to work more efficiently in the Script Editor.</p>';
        
        for (var c = 0; c < shortcutCategories.length; c++) {
            var category = shortcutCategories[c];
            html += '<div class="shortcuts-category">';
            html += '<h4>' + category.name + '</h4>';
            html += '<table class="shortcuts-table">';
            
            for (var s = 0; s < category.shortcuts.length; s++) {
                var shortcut = category.shortcuts[s];
                html += '<tr>';
                html += '<td class="shortcut-keys">';
                for (var k = 0; k < shortcut.keys.length; k++) {
                    html += '<kbd>' + shortcut.keys[k] + '</kbd>';
                    if (k < shortcut.keys.length - 1) html += '+';
                }
                html += '</td>';
                html += '<td class="shortcut-desc">' + shortcut.desc + '</td>';
                html += '</tr>';
            }
            
            html += '</table>';
            html += '</div>';
        }
        
        html += '<div class="shortcuts-note">';
        html += '<strong>Note:</strong> These shortcuts work in the Script Editor area. ';
        html += 'Some shortcuts may behave differently when text is selected.';
        html += '</div>';
        html += '</div>';
        html += '<div class="shortcuts-dialog-footer">';
        html += '<button class="jquerybutton shortcuts-close-btn">Close</button>';
        html += '</div>';
        html += '</div>';
        html += '</div>';
        
        $('body').append(html);
        
        var $dialog = $('#shortcuts-help-dialog');
        
        // Close handlers
        function closeDialog() {
            $dialog.remove();
            $(document).off('keydown.shortcutsDialog');
        }
        
        $('.shortcuts-dialog-close, .shortcuts-close-btn').on('click', closeDialog);
        
        $dialog.on('click', function(e) {
            if (e.target === $dialog[0]) closeDialog();
        });
        
        $(document).on('keydown.shortcutsDialog', function(e) {
            if (e.key === 'Escape') closeDialog();
        });
    }

    // =====================================================================
    // AUTOCOMPLETE INDEXING - Pre-built for O(1) lookup performance
    // =====================================================================

		/**
		 * Provides autocomplete suggestions based on current cursor context.
		 * FIXED: Reliable dot detection using line text, not tokens.
		 * FIXED: Correct insertion for namespace functions.
		 * 
		 * @param {CodeMirror} cm - CodeMirror editor instance
		 * @returns {Object|null} Hint object with list, from, to, data, render, select
		 */
		function getAutocompleteHints(cm) {
				console.log('[AutoComplete] getAutocompleteHints called');

				if (typeof scriptReference === 'undefined' || !scriptReference.isLoaded()) {
						if (scriptReference && !scriptReference.isLoaded()) {
								scriptReference.init().then(function() {
										if (cm.state.completionActive) cm.execCommand('autocomplete');
										else CodeMirror.commands.autocomplete(cm);
								});
						}
						return null;
				}

				var cursor = cm.getCursor();
				var line = cm.getLine(cursor.line);
				var curPos = cursor.ch;

				// ============================================================
				// DETECT CONTEXT
				// ============================================================
				
				// Check if we are inside function parentheses
				var isInsideParens = false;
				var parenDepth = 0;
				for (var i = 0; i < curPos; i++) {
						if (line[i] === '(') parenDepth++;
						else if (line[i] === ')') parenDepth--;
				}
				isInsideParens = (parenDepth > 0);
				
				// Check if we are after a dot
				var dotPos = -1;
				for (var i = curPos - 1; i >= 0; i--) {
						if (line[i] === '.') { dotPos = i; break; }
				}
				
				// Check if there is an opening parenthesis before the dot
				var hasOpenParenBeforeDot = false;
				if (dotPos !== -1) {
						for (var i = 0; i < dotPos; i++) {
								if (line[i] === '(') {
										hasOpenParenBeforeDot = true;
										break;
								}
						}
				}
				
				var isVariableContext = isInsideParens && (dotPos === -1 || dotPos < getLastOpenParenPos(line, curPos));
				
				function getLastOpenParenPos(line, pos) {
						for (var i = pos - 1; i >= 0; i--) {
								if (line[i] === '(') return i;
						}
						return -1;
				}
				
				// ============================================================
				// CASE 1: Variable context (inside parentheses)
				// ============================================================
				if (isVariableContext) {
						// ========================================================
						// STEP 1: Find the FULL variable name if it exists (for replacement)
						// ========================================================
						// Find start of current word (where variable name begins)
						var wordStart = curPos;
						while (wordStart > 0 && /[a-zA-Z_$][a-zA-Z0-9_$]*/.test(line[wordStart - 1])) wordStart--;
						
						// Find end of current word (where variable name ends)
						var wordEnd = wordStart;
						while (wordEnd < line.length && /[a-zA-Z_$][a-zA-Z0-9_$]*/.test(line[wordEnd])) {
								wordEnd++;
						}						
											
						// ========================================================
						// STEP 2: Determine search text (what user typed) vs replacement range
						// ========================================================
						var partialText = line.substring(wordStart, curPos);  // What user typed (for search)
						var fromCh, toCh;
						
						// Check if cursor is inside an existing variable name
						if (wordEnd > wordStart && curPos >= wordStart && curPos <= wordEnd) {
								// Cursor inside existing variable name -> replace the WHOLE name
								fromCh = wordStart;
								toCh = wordEnd;
						} else {
								// Cursor at end, typing new variable name -> replace only typed part
								fromCh = wordStart;
								toCh = curPos;
						}
						
						// ========================================================
						// STEP 3: Get user variables and literals
						// ========================================================
						var userVars = getUserVariables(cm);
						var results = [], resultsData = [], seen = {};
						
						// Add user variables (search based on partialText)
						for (var v = 0; v < userVars.length; v++) {
								var varName = userVars[v];
								if (partialText === '' || varName.toLowerCase().indexOf(partialText.toLowerCase()) !== -1) {
										if (!seen[varName]) {
												seen[varName] = true;
												results.push(varName);
												resultsData.push({
														fullName: varName,
														name: varName,
														desc: 'User variable',
														moduleId: 'variables',
														category: 'variables',
														type: 'variable'
												});
										}
								}
						}
						
						// Add literals (true, false, null)
						//var literals = ['true', 'false', 'null'];
						var literals = getDynamicLiterals();
						for (var l = 0; l < literals.length; l++) {
								var lit = literals[l];
								if (partialText === '' || lit.toLowerCase().indexOf(partialText.toLowerCase()) !== -1) {
										if (!seen[lit]) {
												seen[lit] = true;
												results.push(lit);
												resultsData.push({
														fullName: lit,
														name: lit,
														desc: 'Literal value',
														moduleId: 'literals',
														category: 'literals',
														type: 'literal'
												});
										}
								}
						}
						
						// Sort by relevance: those starting with partialText first
						results.sort(function(a, b) {
								var aLow = a.toLowerCase(), bLow = b.toLowerCase();
								var pLow = partialText.toLowerCase();
								var aStarts = aLow.indexOf(pLow) === 0;
								var bStarts = bLow.indexOf(pLow) === 0;
								if (aStarts && !bStarts) return -1;
								if (!aStarts && bStarts) return 1;
								return aLow.localeCompare(bLow);
						});
						
						if (results.length === 0) return null;
						currentAutocompleteData = resultsData.slice();
						
						return {
								list: results,
								from: CodeMirror.Pos(cursor.line, fromCh),
								to: CodeMirror.Pos(cursor.line, toCh),
								data: resultsData,
								render: function(el, self, data) {
										var item = data[self.pos];
										var name = self.list[self.pos];
										var type = item ? (item.type || '') : '';
										var additionalClass = (type === 'variable') ? ' autocomplete-variable' : (type === 'literal' ? ' autocomplete-literal' : '');
										el.innerHTML = '<span class="autocomplete-name' + additionalClass + '">' + escapeHtml(name) + '</span>' +
																	 '<span class="autocomplete-category">' + escapeHtml(type) + '</span>';
								},
								select: function(selected, self) {
										var cmInst = self.cm;
										var from = self.from;
										var to = self.to;
										// Replace the entire variable name if needed
										cmInst.replaceRange(selected, from, to);
										var newPos = { line: from.line, ch: from.ch + selected.length };
										cmInst.setCursor(newPos);
								}
						};
				}
				
				// ============================================================
				// CASE 2: Function context (after dot)
				// ============================================================
				if (dotPos !== -1 && !isVariableContext) {
						// Extract namespace
						var nsStart = dotPos - 1;
						while (nsStart >= 0 && /[a-zA-Z_$][a-zA-Z0-9_$]*/.test(line[nsStart])) nsStart--;
						nsStart++;
						var namespace = line.substring(nsStart, dotPos);
						
						// ========================================================
						// IMPORTANT: For search, use only the text the user typed
						// ========================================================
						// Find the start of the current word (what user is typing)
						var wordStart = curPos;
						while (wordStart > dotPos + 1 && /[a-zA-Z_$][a-zA-Z0-9_$]*/.test(line[wordStart - 1])) wordStart--;
						var partialText = line.substring(wordStart, curPos);
						
						// ========================================================
						// For replacement range, find the FULL function name if it exists
						// ========================================================
						var funcStart = dotPos + 1;
						var funcEnd = funcStart;
						while (funcEnd < line.length && /[a-zA-Z_$][a-zA-Z0-9_$]*/.test(line[funcEnd])) {
								funcEnd++;
						}
						
						var fromCh, toCh;
						
						// Check if there's an existing function name and cursor is inside it
						if (funcEnd > funcStart && curPos >= funcStart && curPos <= funcEnd) {
								// Cursor inside existing function name -> replace the WHOLE name
								fromCh = funcStart;
								toCh = funcEnd;
						} else {
								// Cursor at end, typing new function name -> replace only typed part
								fromCh = wordStart;
								toCh = curPos;
						}
						
						var allItems = scriptReference.getAutocompleteItems();
						if (!allItems.length) return null;
						
						var results = [], resultsData = [], seen = {};
						var prefix = namespace + '.';
						
						for (var i = 0; i < allItems.length; i++) {
								var item = allItems[i];
								var full = item.fullName;
								if (full && full.indexOf(prefix) === 0) {
										var funcPart = full.substring(prefix.length);
										// Search based on what user typed (partialText), not the full name
										if (partialText === '' || funcPart.toLowerCase().indexOf(partialText.toLowerCase()) !== -1) {
												if (!seen[funcPart]) {
														seen[funcPart] = true;
														results.push(funcPart);
														resultsData.push(item);
												}
										}
								}
						}
						
						// Sort by relevance: those starting with partialText first
						results.sort(function(a, b) {
								var aLow = a.toLowerCase(), bLow = b.toLowerCase();
								var pLow = partialText.toLowerCase();
								var aStarts = aLow.indexOf(pLow) === 0;
								var bStarts = bLow.indexOf(pLow) === 0;
								if (aStarts && !bStarts) return -1;
								if (!aStarts && bStarts) return 1;
								return aLow.localeCompare(bLow);
						});
						
						if (results.length === 0) return null;
						currentAutocompleteData = resultsData.slice();
						
						return {
								list: results,
								from: CodeMirror.Pos(cursor.line, fromCh),
								to: CodeMirror.Pos(cursor.line, toCh),
								data: resultsData,
								render: function(el, self, data) {
										var item = data[self.pos];
										var name = self.list[self.pos];
										var cat = item ? (item.moduleId || '') : '';
										el.innerHTML = '<span class="autocomplete-name">' + escapeHtml(name) + '</span>' +
																	 '<span class="autocomplete-category">' + escapeHtml(cat) + '</span>';
								},
								select: function(selected, self) {
										var cmInst = self.cm;
										var from = self.from;
										var to = self.to;
										cmInst.replaceRange(selected, from, to);
										cmInst.setCursor({ line: from.line, ch: from.ch + selected.length });
								}
						};
				}
				
				// ============================================================
				// CASE 3: Manual invocation (Ctrl+Space)
				// ============================================================
				var start = curPos;
				while (start > 0 && /[a-zA-Z_$][a-zA-Z0-9_$]*/.test(line[start - 1])) start--;
				var partialText = line.substring(start, curPos);
				var fromCh = start;
				var toCh = curPos;
				
				var allItems = scriptReference.getAutocompleteItems();
				if (!allItems.length) return null;
				
				var results = [], resultsData = [], seen = {};
				var isManual = (cm.state.completionActive === undefined || !cm.state.completionActive.isOpen) && partialText === '';
				
				for (var i = 0; i < allItems.length; i++) {
						var item = allItems[i];
						var full = item.fullName;
						if (!full) continue;
						if (isManual) {
								if (!seen[full]) { seen[full] = true; results.push(full); resultsData.push(item); }
						} else if (partialText && full.toLowerCase().indexOf(partialText.toLowerCase()) !== -1) {
								if (!seen[full]) { seen[full] = true; results.push(full); resultsData.push(item); }
						}
				}
				
				results.sort(function(a, b) {
						var aLow = a.toLowerCase(), bLow = b.toLowerCase();
						var pLow = partialText.toLowerCase();
						var aStarts = aLow.indexOf(pLow) === 0;
						var bStarts = bLow.indexOf(pLow) === 0;
						if (aStarts && !bStarts) return -1;
						if (!aStarts && bStarts) return 1;
						return aLow.localeCompare(bLow);
				});
				
				if (results.length === 0) return null;
				currentAutocompleteData = resultsData.slice();
				
				return {
						list: results,
						from: CodeMirror.Pos(cursor.line, fromCh),
						to: CodeMirror.Pos(cursor.line, toCh),
						data: resultsData,
						render: function(el, self, data) {
								var item = data[self.pos];
								var name = self.list[self.pos];
								var cat = item ? (item.moduleId || '') : '';
								el.innerHTML = '<span class="autocomplete-name">' + escapeHtml(name) + '</span>' +
															 '<span class="autocomplete-category">' + escapeHtml(cat) + '</span>';
						},
						select: function(selected, self) {
								var cmInst = self.cm;
								var from = self.from;
								var to = self.to;
								cmInst.replaceRange(selected, from, to);
								cmInst.setCursor({ line: from.line, ch: from.ch + selected.length });
						}
				};
		}

		/**
		 * Get dynamic literals from scriptReference and hardcoded constants
		 * @returns {Array} Array of literal values for autocomplete
		 */
		function getDynamicLiterals() {
				// Hardcoded JavaScript literals and Stellarium constants
				var literals = [
						// JavaScript built-in literals
						'true', 'false', 'null', 'undefined', 'NaN', 'Infinity',
						
						// Mathematical constants
						'PI', 'TWO_PI', 'DEG_TO_RAD', 'RAD_TO_DEG', 'E', 'LN2', 'LN10', 
						'LOG2E', 'LOG10E', 'SQRT1_2', 'SQRT2',
						
						// Stellarium color constants (Vec3f presets)
						'Color_White', 'Color_Black', 'Color_Red', 'Color_Green', 'Color_Blue',
						'Color_Yellow', 'Color_Cyan', 'Color_Magenta', 'Color_Orange', 'Color_Purple',
						
						// Direction constants
						'North', 'South', 'East', 'West', 'Center', 'Zenith', 'Nadir',
						
						// Common string values used in Stellarium API
						'utc', 'local', 'J2000', 'B1950', 'now', 'auto', 'none', 'all',
						'Earth', 'Moon', 'Sun', 'Mars', 'Jupiter', 'Saturn', 'Mercury', 'Venus',
						
						// Frame types (for loadSkyImage)
						'EqJ2000', 'EqDate', 'AzAlt', 'Galactic', 'Supergalactic',
						
						// Projection types (for setProjectionMode)
						'ProjectionPerspective', 'ProjectionFisheye', 'ProjectionMercator',
						'ProjectionHammer', 'ProjectionMollweide', 'ProjectionCylinder',
						'ProjectionOrthographic', 'ProjectionStereographic', 'ProjectionEqualArea',
						'ProjectionMiller', 'ProjectionSinusoidal',
						
						// Sky culture label styles
						'none', 'modern', 'byname', 'ipa', 'translated', 'translit', 'pronounce', 'native',
						
						// Clear states
						'natural', 'starchart', 'deepspace', 'galactic', 'supergalactic',
						
						// Selected object info levels
						'AllInfo', 'DefaultInfo', 'ShortInfo', 'None', 'Custom',
						
						// Mount modes
						'equatorial', 'azimuthal'
				];
				
				// Try to extract additional literals from scriptReference if loaded
				if (scriptReference && scriptReference.isLoaded()) {
						var allItems = scriptReference.getAutocompleteItems();
						var extractedLiterals = [];
						
						for (var i = 0; i < allItems.length; i++) {
								var item = allItems[i];
								if (!item.parameters) continue;
								
								for (var p = 0; p < item.parameters.length; p++) {
										var param = item.parameters[p];
										var desc = param.description || '';
										
										// Extract quoted string literals from parameter descriptions
										var quotedMatches = desc.match(/'([^']+)'/g);
										if (quotedMatches) {
												for (var q = 0; q < quotedMatches.length; q++) {
														var literal = quotedMatches[q].replace(/'/g, '');
														if (extractedLiterals.indexOf(literal) === -1 && 
																literal.length > 0 && literal !== 'true' && literal !== 'false') {
																extractedLiterals.push(literal);
														}
												}
										}
								}
						}
						
						// Add extracted literals
						literals = literals.concat(extractedLiterals);
				}
				
				// Remove duplicates
				var uniqueLiterals = [];
				for (var l = 0; l < literals.length; l++) {
						if (uniqueLiterals.indexOf(literals[l]) === -1) {
								uniqueLiterals.push(literals[l]);
						}
				}
				
				return uniqueLiterals;
		}

		/**
		 * Sets up parameter hint popup when user types '(' after a function name.
		 * Shows function parameters, types, and descriptions.
		 * 
		 * @param {CodeMirror} cm - CodeMirror editor instance
		 * @returns {void}
		 */
		function setupParameterHints(cm) {
			
			    if (!cm) {
        console.warn('[ParamHints] No CodeMirror instance');
        return;
    }
    console.log('[ParamHints] Initializing parameter hints...');

				if (!cm) return;
				
				var paramPopup = null;
				var popupTimeout = null;
				
				function hideParamPopup() {
						if (paramPopup) {
								paramPopup.remove();
								paramPopup = null;
						}
						if (popupTimeout) {
								clearTimeout(popupTimeout);
								popupTimeout = null;
						}
				}
				
				function showParamPopup(funcData, cursorPos) {
						hideParamPopup();
						
						if (!funcData || !funcData.parameters || funcData.parameters.length === 0) {
								return;
						}
						
						var html = '<div class="param-hint-popup">';
						html += '<div class="param-hint-header">';
						html += '<span class="param-hint-name">' + escapeHtml(funcData.fullName || funcData.name) + '</span>';
						html += '</div>';
						
						html += '<div class="param-hint-signature">';
						html += '<code>' + escapeHtml(funcData.fullSignature || funcData.signature) + '</code>';
						html += '</div>';
						
						html += '<div class="param-hint-list">';
						
						for (var i = 0; i < funcData.parameters.length; i++) {
								var p = funcData.parameters[i];
								html += '<div class="param-hint-item">';
								html += '<span class="param-hint-name">' + escapeHtml(p.name) + '</span>';
								html += '<span class="param-hint-type">' + escapeHtml(p.type || 'any') + '</span>';
								if (p.required) {
										html += '<span class="param-hint-required">required</span>';
								} else {
										html += '<span class="param-hint-optional">optional</span>';
								}
								if (p.description) {
										html += '<div class="param-hint-desc">' + escapeHtml(p.description) + '</div>';
								}
								html += '</div>';
						}
						
						html += '</div>';
						html += '<div class="param-hint-footer">';
						html += '<span class="param-hint-hint">Continue typing parameters...</span>';
						html += '</div>';
						html += '</div>';
						
						paramPopup = $(html);
						$('body').append(paramPopup);
						
						// Position popup near cursor
						var coords = cm.cursorCoords(cursorPos, 'window');
						paramPopup.css({
								position: 'fixed',
								left: coords.left + 20,
								top: coords.top + 25,
								zIndex: 10005
						});
						
						// Auto-hide after 8 seconds or on next key press
						popupTimeout = setTimeout(hideParamPopup, 8000);
						
						// Hide on any key press
						$(document).one('keydown.paramHint', function() {
								hideParamPopup();
						});
						
						// Hide on cursor movement
						cm.on('cursorActivity', function onCursorMove() {
								hideParamPopup();
								cm.off('cursorActivity', onCursorMove);
						});
				}
				
				// Find function by name (supports namespace like "core.setDate")
				function findFunctionByName(fullName) {
						if (!scriptReference.isLoaded()) return null;
						
						var allItems = scriptReference.getAutocompleteItems();
						for (var i = 0; i < allItems.length; i++) {
								var item = allItems[i];
								if (item.fullName === fullName || item.name === fullName) {
										return item;
								}
						}
						return null;
				}
				
				// Trigger when user types '('
				cm.on('inputRead', function(cmInstance, change) {
						if (change.text && change.text.length === 1 && change.text[0] === '(') {
								var cursor = cmInstance.getCursor();
								var line = cmInstance.getLine(cursor.line);
								var beforeCursor = line.substring(0, cursor.ch - 1);
								
								// Match function name (supports namespace.function)
								var funcMatch = beforeCursor.match(/([a-zA-Z_$][a-zA-Z0-9_$]*(\.[a-zA-Z_$][a-zA-Z0-9_$]*)?)\s*$/);
								
								if (funcMatch) {
										var funcName = funcMatch[1];
										var funcData = findFunctionByName(funcName);
										
										if (funcData) {
												showParamPopup(funcData, cursor);
										}
								}
						}
				});
		}

		/**
		 * Gets all user-defined variables from the current editor content.
		 * Includes var, let, const declarations and function parameters.
		 * FIXED: More comprehensive pattern matching
		 * 
		 * @param {CodeMirror} cm - CodeMirror editor instance
		 * @returns {Array} List of variable names
		 */
		function getUserVariables(cm) {
				if (!cm) return [];
				
				var content = cm.getValue();
				var variables = {};
				var match;
				
				// var declarations: var x, y, z;
				var varRegex = /\b(?:var|let|const)\s+([a-zA-Z_$][a-zA-Z0-9_$]*)\b/g;
				while ((match = varRegex.exec(content)) !== null) {
						variables[match[1]] = true;
				}
				
				// Multiple var declarations: var x = 1, y = 2, z = 3;
				var multiVarRegex = /\b(?:var|let|const)\s+([a-zA-Z_$][a-zA-Z0-9_$]*)\s*=/g;
				while ((match = multiVarRegex.exec(content)) !== null) {
						variables[match[1]] = true;
				}
				
				// Function parameters: function(name, age) { ... }
				var funcRegex = /function\s*\w*\s*\(([^)]*)\)/g;
				while ((match = funcRegex.exec(content)) !== null) {
						var params = match[1].split(',');
						for (var i = 0; i < params.length; i++) {
								var p = params[i].trim();
								if (p) variables[p] = true;
						}
				}
				
				// Arrow function parameters: (x, y) => or x =>
				var arrowRegex = /\(([^)]*)\)\s*=>/g;
				while ((match = arrowRegex.exec(content)) !== null) {
						var params = match[1].split(',');
						for (var i = 0; i < params.length; i++) {
								var p = params[i].trim();
								if (p) variables[p] = true;
						}
				}
				
				// Single parameter arrow functions: x => x * 2
				var singleArrowRegex = /([a-zA-Z_$][a-zA-Z0-9_$]*)\s*=>/g;
				while ((match = singleArrowRegex.exec(content)) !== null) {
						variables[match[1]] = true;
				}
				
				// Loop variables: for (var i = 0; i < n; i++)
				var loopRegex = /for\s*\(\s*(?:var|let|const)\s+([a-zA-Z_$][a-zA-Z0-9_$]*)\s*=/g;
				while ((match = loopRegex.exec(content)) !== null) {
						variables[match[1]] = true;
				}
				
				// Catch parameters: catch(error)
				var catchRegex = /catch\s*\(\s*([a-zA-Z_$][a-zA-Z0-9_$]*)\s*\)/g;
				while ((match = catchRegex.exec(content)) !== null) {
						variables[match[1]] = true;
				}
				
				// Object destructuring: const { name, age } = obj
				var destructureRegex = /{\s*([a-zA-Z_$][a-zA-Z0-9_$]*)\s*(?:,\s*([a-zA-Z_$][a-zA-Z0-9_$]*)\s*)*}/g;
				while ((match = destructureRegex.exec(content)) !== null) {
						for (var d = 1; d < match.length; d++) {
								if (match[d]) variables[match[d]] = true;
						}
				}
				
				return Object.keys(variables);
		}

		// =====================================================================
    // PINNABLE INFO PANEL WITH FIXED COPY FUNCTIONALITY
    // =====================================================================

    /**
     * Global cache for storing example code to avoid HTML attribute encoding issues.
     * This solves the problem where long examples with special characters
     * would break when stored in data attributes.
     * 
     * @type {Object}
     */
    var exampleCodeCache = {};
    var exampleCounter = 0;

	
		/**
		 * Insert example code at a specific line number.
		 * 
		 * @param {string} code - The code to insert
		 * @param {number} lineNumber - Target line number (1-indexed)
		 * @returns {void}
		 */
		function insertExampleAtLine(code, lineNumber) {
				if (!code || !code.trim()) {
						addOutput('error', 'No code to insert');
						return;
				}
				
				if (codeMirrorInstance) {
						var totalLines = codeMirrorInstance.lineCount();
						var targetLineIndex = Math.min(lineNumber - 1, totalLines);
						
						var insertCode = code;
						if (!insertCode.endsWith('\n')) {
								insertCode = insertCode + '\n';
						}
						
						codeMirrorInstance.replaceRange(insertCode, 
								{line: targetLineIndex, ch: 0},
								{line: targetLineIndex, ch: 0});
						
						var insertedLineCount = insertCode.split('\n').length - 1;
						codeMirrorInstance.setCursor({line: targetLineIndex + insertedLineCount, ch: 0});
						codeMirrorInstance.focus();
						
						addOutput('success', 'Example inserted at line ' + (targetLineIndex + 1));
				} else {
						var ta = document.getElementById('script-editor');
						if (!ta) return;
						
						var lines = ta.value.split('\n');
						var targetLineIndex = Math.min(lineNumber - 1, lines.length);
						
						lines.splice(targetLineIndex, 0, code);
						ta.value = lines.join('\n');
						
						var cursorPos = 0;
						for (var i = 0; i <= targetLineIndex; i++) {
								cursorPos += lines[i].length + 1;
						}
						ta.selectionStart = cursorPos;
						ta.selectionEnd = cursorPos;
						ta.focus();
						
						addOutput('success', 'Example inserted at line ' + (targetLineIndex + 1));
				}
		}

		/**
		 * Get current line number (1-indexed) from the editor cursor position.
		 * 
		 * @returns {number} Current line number (1-indexed), default 1 if editor not available
		 */
		function getCurrentLineNumber() {
				if (codeMirrorInstance) {
						return codeMirrorInstance.getCursor().line + 1;
				}
				var ta = document.getElementById('script-editor');
				if (ta) {
						var cursorPos = ta.selectionStart;
						var lines = ta.value.substring(0, cursorPos).split('\n');
						return lines.length;
				}
				return 1;
		}
    
    /**
     * Formats example code for better display (adds line numbers, trims).
     * 
     * @param {string} example - Raw example code
     * @returns {string} Formatted example
     */
    function formatExampleForDisplay(example) {
        if (!example) return '';
        
        // Remove excessive indentation
        var lines = example.split('\n');
        var minIndent = Infinity;
        
        for (var i = 0; i < lines.length; i++) {
            var line = lines[i];
            if (line.trim().length === 0) continue;
            var indent = line.match(/^\s*/)[0].length;
            if (indent < minIndent) minIndent = indent;
        }
        
        if (minIndent !== Infinity && minIndent > 0) {
            var indentRegex = new RegExp('^\\s{' + minIndent + '}');
            for (var j = 0; j < lines.length; j++) {
                lines[j] = lines[j].replace(indentRegex, '');
            }
        }
        
        // Limit length
        var result = lines.join('\n');
        if (result.length > 2000) {
            result = result.substring(0, 2000) + '\n... (truncated)';
        }
        
        return result;
    }

    // =====================================================================
    // REFACTORED initCodeMirror FUNCTION
    // =====================================================================

		// Autocomplete process
		var completionController = {
				active: false,           // Is autocomplete active?
				context: null,           // Autocomplete context { namespace, partial, isDot, timestamp }
				items: null,             // Items save localy
				timer: null,             // Timing to prevent doublication
				lockTime: 0,             // Lock time
				init: function() {
						this.active = false;
						this.context = null;
						this.items = null;
				},
				isLocked: function() {
						return (Date.now() - this.lockTime) < 300;
				},
				lock: function(duration) {
						this.lockTime = Date.now();
						if (this.timer) clearTimeout(this.timer);
						this.timer = setTimeout(function() { completionController.active = false; }, duration);
				}
		};

		/**
		 * Initializes the CodeMirror editor with Stellarium syntax highlighting.
		 * ENHANCED VERSION with proper case-sensitive validation.
		 * 
		 * Features:
		 * - Dynamic module names from modules-index.json
		 * - REAL case-sensitive validation (core.setdate vs core.setDate)
		 * - Proper tokenization with exact case matching
		 * - Revalidate button with full case checking
		 * - Auto-validation toggle with instant color updates
		 * 
		 * @function initCodeMirror
		 * @returns {void}
		 */
		function initCodeMirror() {
				var ta = document.getElementById('script-editor');
				if (!ta) { 
						console.warn('[ScriptEditor] Editor textarea not found'); 
						return; 
				}
				
				// =====================================================================
				// STEP 1: Check if CodeMirror is available
				// =====================================================================
				if (typeof CodeMirror === 'undefined') {
						console.warn('[ScriptEditor] CodeMirror not available, waiting...');
						waitForCodeMirror(initCodeMirror);
						return;
				}
				
				// =====================================================================
				// STEP 2: DATA STRUCTURES FOR DYNAMIC MODULE NAMES
				// =====================================================================
				var moduleNames = [];
				var builtinsMap = {};
				var functionsMap = {};  // Stores all valid namespace.function pairs
				var allValidFunctions = [];  // Array of {namespace, function, fullName}
				
				/**
				 * Build complete functions map from scriptReference
				 * This is CRITICAL for case-sensitive validation
				 */
				function buildFunctionsMap() {
						functionsMap = {};
						allValidFunctions = [];
						
						if (!scriptReference || !scriptReference.isLoaded()) {
								console.log('[Validation] ScriptReference not loaded, cannot build functions map');
								return false;
						}
						
						var items = scriptReference.getAutocompleteItems();
						if (!items || items.length === 0) {
								console.log('[Validation] No autocomplete items found');
								return false;
						}
						
						for (var i = 0; i < items.length; i++) {
								var item = items[i];
								var fullName = item.fullName;
								
								if (fullName && fullName.indexOf('.') !== -1) {
										var parts = fullName.split('.');
										var namespace = parts[0];
										var funcName = parts[1];
										
										// Store in map for O(1) lookup with EXACT case
										var key = namespace + '::' + funcName;
										functionsMap[key] = true;
										
										allValidFunctions.push({
												namespace: namespace,
												function: funcName,
												fullName: fullName,
												key: key
										});
								}
						}
						
						console.log('[Validation] Built functions map with ' + Object.keys(functionsMap).length + ' entries (case-sensitive)');
						return true;
				}
				
				/**
				 * Update module names from scriptReference
				 */
				function updateModuleNames() {
						moduleNames = [];
						builtinsMap = {};
						
						if (scriptReference && scriptReference.getModuleIndex) {
								var index = scriptReference.getModuleIndex();
								if (index && index.modules && Array.isArray(index.modules)) {
										for (var i = 0; i < index.modules.length; i++) {
												var mod = index.modules[i];
												var moduleId = mod.id;
												moduleNames.push(moduleId);
												builtinsMap[moduleId] = 'builtin';
												
												// Add module name without "Mgr" suffix as alias
												if (moduleId.indexOf('Mgr') !== -1) {
														var shortName = moduleId.replace('Mgr', '');
														if (shortName.length > 3 && !builtinsMap[shortName]) {
																builtinsMap[shortName] = 'builtin';
														}
												}
										}
										console.log('[CodeMirror] Loaded ' + moduleNames.length + ' module names dynamically');
								}
						}
						
						// Always include core as fallback
						if (!builtinsMap['core']) {
								builtinsMap['core'] = 'builtin';
								moduleNames.push('core');
						}
						
						// Build functions map after modules are loaded
						buildFunctionsMap();
				}
				
				/**
				 * Case-sensitive validation of function name
				 * Returns true ONLY if the function exists with EXACT case matching
				 */
				function isValidFunctionCaseSensitive(funcName, namespace) {
						// If scriptReference is not loaded yet, return false
						if (!scriptReference || !scriptReference.isLoaded()) {
								return false;
						}
						
						// Check if functionsMap is populated
						if (Object.keys(functionsMap).length === 0) {
								buildFunctionsMap();
						}
						
						var key = namespace + '::' + funcName;
						var isValid = functionsMap[key] === true;
						
						// Debug log for case errors (only log first few to avoid spam)
						if (!isValid && window._debugValidation) {
								// Check if it's a case-insensitive match (to report case errors)
								for (var i = 0; i < allValidFunctions.length; i++) {
										if (allValidFunctions[i].namespace.toLowerCase() === namespace.toLowerCase() &&
												allValidFunctions[i].function.toLowerCase() === funcName.toLowerCase()) {
												console.log('[CaseError] "' + funcName + '" should be "' + allValidFunctions[i].function + '"');
												break;
										}
								}
						}
						
						return isValid;
				}
				
				// Initial attempt to load module names
				updateModuleNames();
				
				// If scriptReference is not loaded yet, wait for it
				if (scriptReference && !scriptReference.isLoaded()) {
						scriptReference.init().then(function() {
								updateModuleNames();
								buildFunctionsMap();
								if (codeMirrorInstance) {
										codeMirrorInstance.setOption('mode', 'stellarium');
										codeMirrorInstance.refresh();
										console.log('[CodeMirror] Module names and functions map updated');
										// Revalidate after module names are loaded
										//setTimeout(function() {
										//		fullRevalidateAllFunctions();
										//}, 500);
								}
						});
				}
				
				// =====================================================================
				// STEP 3: ENHANCED STELLARIUM MODE WITH BETTER TOKENIZATION
				// =====================================================================
				if (CodeMirror.modes && !CodeMirror.modes.stellarium) {
						console.log('[CodeMirror] Registering enhanced stellarium mode with case-sensitive tokenization...');
						
						CodeMirror.defineMode('stellarium', function() {
								// JavaScript keywords (fixed)
								var keywords = [
										'var', 'function', 'if', 'else', 'for', 'while', 'do', 'switch',
										'case', 'break', 'continue', 'return', 'true', 'false', 'null',
										'new', 'this', 'typeof', 'instanceof', 'delete', 'in', 'try',
										'catch', 'finally', 'throw', 'include'
								];
								
								var keywordsMap = {};
								for (var i = 0; i < keywords.length; i++) {
										keywordsMap[keywords[i]] = 'keyword';
								}
								
								// Store reference to current builtinsMap
								var currentBuiltinsMap = builtinsMap;
								
								return {
										startState: function() {
												return { 
														inString: false, 
														stringChar: null,
														inComment: false,
														inBlockComment: false,
														lastToken: null,
														currentNamespace: null
												};
										},

										token: function(stream, state) {
												// Update builtins reference
												currentBuiltinsMap = builtinsMap;
												
												// ============================================================
												// LINE COMMENTS (// ...)
												// ============================================================
												if (stream.match('//')) {
														stream.skipToEnd();
														return 'comment';
												}
												
												// ============================================================
												// BLOCK COMMENTS (/* ... */) - Enhanced with whitespace handling
												// ============================================================
												
												// Check for start of block comment
												if (stream.match(/\/\*/)) {
														state.inBlockComment = true;
														return 'comment';
												}
												
												// Handle block comment content and closing tag
												if (state.inBlockComment) {
														// IMPORTANT: Skip whitespace before checking for closing tag
														// This fixes the issue where `*/` with indentation wasn't being recognized
														var spaces = stream.eatWhile(/\s/);
														
														// Check for closing tag `*/` (with or without preceding whitespace)
														if (stream.match(/\*\//)) {
																state.inBlockComment = false;
																return 'comment';
														}
														
														// If we consumed spaces but didn't find `*/`, we need to backtrack
														// because we might have consumed spaces that are part of the comment content
														if (spaces) {
																// Go back to where we started eating spaces
																stream.backUp(spaces);
														}
														
														// No closing tag found, consume rest of the line as comment
														stream.skipToEnd();
														return 'comment';
												}
												
												// ============================================================
												// STRINGS - Single and Double Quotes
												// ============================================================
												if (stream.match('"') || stream.match("'")) {
														var quote = stream.current();
														while (!stream.eol()) {
																var next = stream.next();
																if (next === '\\') {
																		stream.next(); // Skip escaped character
																		continue;
																}
																if (next === quote) {
																		break;
																}
														}
														return 'string';
												}
												
												// ============================================================
												// NUMBERS
												// ============================================================
												if (stream.match(/^-?\d+(\.\d+)?([eE][+-]?\d+)?/)) {
														return 'number';
												}
												
												// ============================================================
												// OPERATORS
												// ============================================================
												if (stream.match(/^[+\-*/%=<>!&|^~?:]+/)) {
														return 'operator';
												}
												
												// ============================================================
												// BRACKETS AND PARENTHESES
												// ============================================================
												if (stream.match(/^[{}()\[\]]/)) {
														var bracket = stream.current();
														if (bracket === ')' || bracket === ']' || bracket === '}') {
																return 'bracket-close';
														}
														return 'bracket';
												}
												
												// ============================================================
												// DOT TOKEN - Track for namespace.function pattern
												// ============================================================
												if (stream.peek() === '.') {
														state.lastToken = 'dot';
														stream.next();
														return null;
												}
												
												// ============================================================
												// IDENTIFIERS - Core tokenization with case preservation
												// ============================================================
												if (stream.match(/^[a-zA-Z_$][a-zA-Z0-9_$]*/)) {
														var word = stream.current();  // Preserves original case!
														
														// After dot - this is a FUNCTION name (case-sensitive!)
														if (state.lastToken === 'dot') {
																state.lastToken = null;
																// Store the function name for later validation
																// The color will be set by validateSingleFunction
																return 'function-pending';
														}
														
														// Check if it's a built-in module (namespace) - case-sensitive
														if (currentBuiltinsMap[word]) {
																return 'builtin';
														}
														
														// Check if it's a keyword - case-sensitive (keywords are lowercase)
														if (keywordsMap[word]) {
																return 'keyword';
														}
														
														// Default variable
														return 'variable';
												}
												
												// ============================================================
												// FALLBACK - Consume any other character
												// ============================================================
												stream.next();
												state.lastToken = null;
												return null;
										},
										
										copyState: function(state) {
												return {
														inString: state.inString,
														stringChar: state.stringChar,
														inComment: state.inComment,
														inBlockComment: state.inBlockComment,
														lastToken: state.lastToken,
														currentNamespace: state.currentNamespace
												};
										}
								};
						});
						
						CodeMirror.defineMIME('text/x-stellarium', 'stellarium');
						console.log('[CodeMirror] Enhanced stellarium mode registered');
				}
				
				// =====================================================================
				// STEP 4: Create CodeMirror instance
				// =====================================================================
				console.log('[CodeMirror] Creating CodeMirror instance...');
				
				codeMirrorInstance = CodeMirror.fromTextArea(ta, {
						mode: 'stellarium',
						lineNumbers: true,
						lineWrapping: true,
						indentUnit: 4,
						tabSize: 4,
						smartIndent: true,
						autofocus: false,
						spellcheck: false,
						styleActiveLine: true,
						matchBrackets: true,
						autoCloseBrackets: true,
						theme: 'default',
				    placeholder: '// Write Stellarium script code here...\n// Example:\n// core.setDate("2026-01-01T00:00:00", "utc");\n// core.moveToObject("Mars", 2);\n// core.wait(3);',
						
						extraKeys: {
								'Ctrl-Enter': function(cm) { runScript(); },
								'Ctrl-Shift-Enter': function(cm) { runSelection(); },
								'Ctrl-Space': function(cm) { CodeMirror.commands.autocomplete(cm); },
								'Ctrl-F': 'findPersistent',
								'Ctrl-H': 'replace',
								'Ctrl-G': function(cm) { showGoToLineDialog(cm); },
								'Ctrl-L': function(cm) { showGoToLineDialog(cm); },
								'Ctrl-S': function(cm) { saveScriptToFile(); },
								'Ctrl-O': function(cm) { openScriptFromFile(); },
								'Tab': function(cm) { cm.replaceSelection('    ', 'end'); },
								'Ctrl-/': function(cm) { toggleLineComment(cm); },
								'Alt-Up': function(cm) { cm.swapLineUp(); },
								'Alt-Down': function(cm) { cm.swapLineDown(); }
						},
						
						hintOptions: {
								hint: getAutocompleteHints,
								completeSingle: false,
								alignWithWord: false
						}
				});
				
				// =====================================================================
				// LINE COUNTER - Update on changes
				// =====================================================================

				// Update line counter when content loads
				setTimeout(function() {
						refreshLineCount();
				}, 100);

				// Update when content changes
				codeMirrorInstance.on('change', function(cm) {
						refreshLineCount();
				});

				// Update when performing operations like swapping lines or adding/deleting
				codeMirrorInstance.on('swapLineUp', function(cm) {
						refreshLineCount();
				});

				codeMirrorInstance.on('swapLineDown', function(cm) {
						refreshLineCount();
				});

				codeMirrorInstance.on('deleteLine', function(cm) {
						refreshLineCount();
				});

				// Update on paste or cut operations
				codeMirrorInstance.on('paste', function(cm) {
						setTimeout(function() { refreshLineCount(); }, 10);
				});

				// Update on delete operations (Ctrl+Shift+D or Ctrl+D)
				codeMirrorInstance.on('keydown', function(cm, event) {
						// Ensure that operations affecting line count are updated
						setTimeout(function() { refreshLineCount(); }, 5);
				});

				// Update on focus (to ensure after any external operations)
				codeMirrorInstance.on('focus', function() {
						refreshLineCount();
				});
				
				function toggleLineComment(cm) {
						var cursor = cm.getCursor();
						var line = cm.getLine(cursor.line);
						if (line.trim().indexOf('//') === 0) {
								cm.replaceRange(line.replace(/^\s*\/\/\s?/, ''), 
										{line: cursor.line, ch: 0}, 
										{line: cursor.line, ch: line.length});
						} else { 
								var indent = line.match(/^\s*/)[0]; 
								cm.replaceRange(indent + '// ' + line.trim(), 
										{line: cursor.line, ch: 0}, 
										{line: cursor.line, ch: line.length}); 
						}
				}
				
				// =====================================================================
				// STEP 5: CSS styles
				// =====================================================================
				if (!document.getElementById('stellarium-syntax-styles')) {
						var style = document.createElement('style');
						style.id = 'stellarium-syntax-styles';
						style.textContent = `
								.cm-builtin { color: #00CBFF !important; font-weight: bold; }
								.cm-function-valid { color: #A6E22E !important; font-weight: bold; }
								.cm-function-invalid { color: #F92672 !important; text-decoration: underline wavy #F92672; }
								.cm-function-pending { color: #8AD500 !important; font-style: italic; }
								.cm-keyword { color: #FF9D00 !important; font-weight: bold; }
								.cm-string { color: #E6DB74 !important; }
								.cm-number { color: #AE81FF !important; }
								.cm-comment { color: #FD971F !important; font-style: italic; }
								.cm-operator { color: #F92672 !important; }
								.cm-variable { color: #F8F8F2 !important; }
								.cm-bracket { color: #FD971F !important; font-weight: bold; }
								.cm-bracket-close { color: #F92672 !important; }
								.CodeMirror { background: #1A1C1E !important; color: #DCDBDA !important; }
								.CodeMirror-gutters { background: #2A2C2E !important; border-right: 1px solid #3A3C3E !important; }
								.CodeMirror-linenumber { color: #6A6C6E !important; }
								.CodeMirror-cursor { border-left: 2px solid #FD971F !important; }
								.CodeMirror-selected { background: rgba(253, 151, 31, 0.2) !important; }
								.CodeMirror-activeline-background { background: rgba(253, 151, 31, 0.08) !important; }
								.CodeMirror-matchingbracket { background: rgba(166, 226, 46, 0.2) !important; color: #A6E22E !important; border-bottom: 1px solid #A6E22E; }
								
								.cm-function-case-error {
										color: #F92672 !important;
										text-decoration: underline wavy #FF9D00 !important;
								}
						`;
						document.head.appendChild(style);
				}
				
				// =====================================================================
				// STEP 6:  VALIDATION SYSTEM (CASE-SENSITIVE)
				// =====================================================================

				// --- 6.1 Auto-validation Toggle (OFF by default) ---
				let autoValidationEnabled = false;
				let validationTimeout = null;

				const $autoValidationCheckbox = $('#auto-validation-checkbox');
				if ($autoValidationCheckbox.length) {
						$autoValidationCheckbox.prop('checked', autoValidationEnabled);
						
						$autoValidationCheckbox.on('change', function() {
								autoValidationEnabled = $(this).is(':checked');
								console.log(`[AutoValidation] ${autoValidationEnabled ? 'ENABLED' : 'DISABLED'}`);
								
								if (typeof addOutput === 'function') {
										addOutput('info', `Auto-validation ${autoValidationEnabled ? 'enabled' : 'disabled'}`);
								}
								
								if (typeof showNotification === 'function') {
										if (autoValidationEnabled) {
												showNotification(_tr("Auto-validation enabled - functions will be colored as you type"), "success");
										} else {
												showNotification(_tr("Auto-validation disabled - use Revalidate button to check syntax"), "info");
										}
								}
								
								if (autoValidationEnabled && codeMirrorInstance) {
										setTimeout(() => validateCurrentLine(codeMirrorInstance), 100);
								}
						});
				}

				function shouldAutoValidate() { return autoValidationEnabled; }

				// --- 6.2 Helper: Clear all validation marks ---
				function clearAllValidationMarks(cm) {
						if (!cm) return 0;
						const marks = cm.getAllMarks();
						let count = 0;
						for (const mark of marks) {
								const className = mark.className;
								if (className && (
										className.includes('function-valid') ||
										className.includes('function-invalid') ||
										className.includes('function-pending') ||
										className.includes('function-case-error')
								)) {
										mark.clear();
										count++;
								}
						}
						if (count > 0) console.log(`[Validation] Cleared ${count} marks`);
						return count;
				}

				// --- 6.3 Core Logic: Validate a single function using scriptReference ---
				function validateSingleFunction(cm, namespace, funcName, lineNum, startCh, endCh) {
						if (!funcName || funcName.trim() === '') return false;
						
						let isValid = false;
						let isCaseError = false;
						let isPending = false;
						
						if (scriptReference && scriptReference.isLoaded()) {
								isValid = scriptReference.isValidFunction(funcName, namespace);
								if (!isValid) {
										isCaseError = scriptReference.isCaseError(funcName, namespace);
								}
						} else {
								isPending = true; // API not loaded yet
						}
						
						let className = '';
						if (isPending) className = 'function-pending';
						else if (isValid) className = 'function-valid';
						else if (isCaseError) className = 'function-case-error';
						else className = 'function-invalid';
						
						const from = { line: lineNum, ch: startCh };
						const to = { line: lineNum, ch: endCh };
						
						// Remove existing marks to avoid duplicates
						cm.findMarks(from, to).forEach(mark => mark.clear());
						
						cm.markText(from, to, {
								className: `cm-${className}`,
								atomic: false,
								clearOnEnter: true
						});
						
						return isValid;
				}

				// --- 6.4 Validate all functions in a single line ---
				function validateLine(cm, lineNum) {
						if (!cm) return { count: 0, valid: 0, invalid: 0, caseErrors: 0 };
						const lineText = cm.getLine(lineNum);
						if (!lineText) return { count: 0, valid: 0, invalid: 0, caseErrors: 0 };
						
						const pattern = /([a-zA-Z_$][a-zA-Z0-9_$]*)\.([a-zA-Z_$][a-zA-Z0-9_$]*)/g;
						let match;
						const stats = { count: 0, valid: 0, invalid: 0, caseErrors: 0 };
						
						while ((match = pattern.exec(lineText)) !== null) {
								const namespace = match[1];
								const funcName = match[2];
								const funcStart = match.index + namespace.length + 1;
								const funcEnd = funcStart + funcName.length;
								
								stats.count++;
								const isValid = validateSingleFunction(cm, namespace, funcName, lineNum, funcStart, funcEnd);
								
								if (isValid) {
										stats.valid++;
								} else {
										stats.invalid++;
										if (scriptReference && scriptReference.isLoaded() && scriptReference.isCaseError(funcName, namespace)) {
												stats.caseErrors++;
										}
								}
						}
						return stats;
				}

				// --- 6.5 Validate current line only (for auto-validation) ---
				function validateCurrentLine(cm) {
						if (!cm) return;
						const cursor = cm.getCursor();
						const result = validateLine(cm, cursor.line);
						if (window._debugValidation && result.count > 0) {
								console.log(`[AutoValidation] Line ${cursor.line + 1}: ${result.valid} valid, ${result.invalid} invalid (${result.caseErrors} case errors)`);
						}
				}

				// --- 6.6 Perform full document validation (for Revalidate button) ---
				function fullRevalidateAllFunctions() {
						if (!codeMirrorInstance) {
								console.warn('[Revalidate] No editor instance');
								if (typeof showNotification === 'function') {
										showNotification(_tr("Editor not ready for revalidation"), "error");
								}
								return;
						}
						
						console.log('[Revalidate] ========== STARTING FULL CASE-SENSITIVE REVALIDATION ==========');
						if (typeof addOutput === 'function') {
								addOutput('info', '⟳ Revalidating all functions (case-sensitive)...');
						}
						if (typeof showNotification === 'function') {
								showNotification(_tr("Revalidating functions..."), "info");
						}
						
						// Clear all existing marks and force refresh
						clearAllValidationMarks(codeMirrorInstance);
						codeMirrorInstance.refresh();
						
						// If scriptReference is not loaded yet, wait for it
						if (!scriptReference || !scriptReference.isLoaded()) {
								console.log('[Revalidate] Waiting for scriptReference to load...');
								if (typeof addOutput === 'function') {
										addOutput('info', 'Waiting for API reference to load...');
								}
								if (scriptReference && scriptReference.init) {
										scriptReference.init()
												.then(() => performFullRevalidation())
												.catch(err => {
														console.error('[Revalidate] Failed to load scriptReference:', err);
														if (typeof showNotification === 'function') {
																showNotification(_tr("Revalidation failed: API reference not loaded"), "error");
														}
												});
								}
								return;
						}
						
						performFullRevalidation();
						
						function performFullRevalidation() {
								const lineCount = codeMirrorInstance.lineCount();
								let totalStats = { count: 0, valid: 0, invalid: 0, caseErrors: 0 };
								
								for (let i = 0; i < lineCount; i++) {
										const lineStats = validateLine(codeMirrorInstance, i);
										totalStats.count += lineStats.count;
										totalStats.valid += lineStats.valid;
										totalStats.invalid += lineStats.invalid;
										totalStats.caseErrors += lineStats.caseErrors;
								}
								
								codeMirrorInstance.refresh();
								
								console.log(`[Revalidate] Complete: ${totalStats.valid} valid, ${totalStats.invalid} invalid (${totalStats.caseErrors} case errors)`);
								
								if (typeof addOutput === 'function') {
										if (totalStats.caseErrors > 0) {
												addOutput('warning', `✅ Revalidation: ${totalStats.valid} valid, ${totalStats.invalid} invalid (${totalStats.caseErrors} case errors)`);
										} else if (totalStats.invalid > 0) {
												addOutput('warning', `✅ Revalidation: ${totalStats.valid} valid, ${totalStats.invalid} invalid`);
										} else {
												addOutput('success', `✅ Revalidation complete: ${totalStats.valid} valid functions found`);
										}
								}
								
								if (typeof showNotification === 'function') {
										if (totalStats.caseErrors > 0) {
												showNotification(_tr(`Revalidation: ${totalStats.valid} valid, ${totalStats.invalid} invalid (${totalStats.caseErrors} case errors)`), "warning");
										} else if (totalStats.invalid > 0) {
												showNotification(_tr(`Revalidation: ${totalStats.valid} valid, ${totalStats.invalid} invalid`), "warning");
										} else {
												showNotification(_tr("Revalidation complete: All functions valid"), "success");
										}
								}
						}
				}

				// --- 6.7 Expose global functions for buttons and debugging ---
				window.revalidateStelFunctions = fullRevalidateAllFunctions;
				window._validateAll = fullRevalidateAllFunctions;
				window._validateLine = () => validateCurrentLine(codeMirrorInstance);
				window._debugValidation = false;  // Set to true to enable debug logs

				// =====================================================================
				// STEP 7: REAL-TIME VALIDATION & EVENT HANDLERS
				// =====================================================================

				// 7.1 Trigger validation on typing (only if auto-validation is enabled)
				codeMirrorInstance.on('inputRead', (cm, change) => {
						// Trigger autocomplete on dot
						if (change.text && change.text.length === 1 && change.text[0] === '.') {
								setTimeout(() => {
										if (!cm.state.completionActive) {
												CodeMirror.commands.autocomplete(cm);
										}
								}, 50);
						}
						
						if (!shouldAutoValidate()) return;
						
						// Validate when a function name is likely completed
						if (change.text && change.text.length === 1) {
								const typedChar = change.text[0];
								const isCompletionChar = ['(', ')', ' ', ';', ',', '\n'].includes(typedChar);
								if (isCompletionChar) {
										setTimeout(() => validateCurrentLine(cm), 50);
								}
						}
				});

				// 7.2 Validate when cursor moves to a new line (only if auto-validation is enabled)
				let lastValidatedLine = -1;
				codeMirrorInstance.on('cursorActivity', (cm) => {
						if (!shouldAutoValidate()) return;
						const cursor = cm.getCursor();
						if (lastValidatedLine !== cursor.line) {
								lastValidatedLine = cursor.line;
								setTimeout(() => validateCurrentLine(cm), 100);
						}
				});

				// 7.3 Validate after content changes (with debouncing, only if auto-validation is enabled)
				codeMirrorInstance.on('change', (cm) => {
						// Save to localStorage on every change
						try {
								localStorage.setItem('stellarium-script-editor', cm.getValue());
						} catch(e) {}
						
						if (!shouldAutoValidate()) return;
						
						if (validationTimeout) clearTimeout(validationTimeout);
						validationTimeout = setTimeout(() => {
								validateCurrentLine(cm);
								validationTimeout = null;
						}, 1000);
				});

				// 7.4 Trigger autocomplete on dot (additional handler)
				codeMirrorInstance.on('keyup', (cm, event) => {
						if (event.key === '.' && (!cm.state.completionActive || !cm.state.completionActive.isOpen)) {
								setTimeout(() => CodeMirror.commands.autocomplete(cm), 10);
						}
				});

				// =====================================================================
				// STEP 8: Load saved content from localStorage
				// =====================================================================
				try {
						const savedCode = localStorage.getItem('stellarium-script-editor');
						if (savedCode && savedCode.trim()) {
								codeMirrorInstance.setValue(savedCode);
								setTimeout(() => {
										refreshLineCount();
										// Optional: auto-validate on load (disabled by default to save performance)
										// fullRevalidateAllFunctions();
								}, 500);
						}
				} catch(e) {
						console.warn('[CodeMirror] Could not load from localStorage:', e);
				}

				// =====================================================================
				// STEP 9: Setup parameter hints (if available)
				// =====================================================================
				if (typeof setupParameterHints === 'function') {
						setupParameterHints(codeMirrorInstance);
				}

				// =====================================================================
				// STEP 10: Keyboard shortcuts
				// =====================================================================
				codeMirrorInstance.on('keydown', (cm, event) => {
						if (event.ctrlKey && event.key === 's') {
								event.preventDefault();
								saveScriptToFile();
						}
						if (event.ctrlKey && event.key === 'o') {
								event.preventDefault();
								openScriptFromFile();
						}
						if (event.ctrlKey && event.key === 'g') {
								event.preventDefault();
								if (typeof showGoToLineDialog === 'function') {
										showGoToLineDialog(cm);
								}
						}
						if (event.ctrlKey && event.key === 'Enter' && !event.shiftKey) {
								event.preventDefault();
								runScript();
						}
						if (event.ctrlKey && event.shiftKey && event.key === 'Enter') {
								event.preventDefault();
								runSelection();
						}
				});

				// =====================================================================
				// STEP 11: Focus/blur handlers for styling
				// =====================================================================
				codeMirrorInstance.on('focus', () => {
						$('.script-editor-container').addClass('editor-focused');
				});
				codeMirrorInstance.on('blur', () => {
						$('.script-editor-container').removeClass('editor-focused');
				});

				// =====================================================================
				// STEP 12: Expose for debugging
				// =====================================================================
				window._cm = codeMirrorInstance;
				window._stelCodeMirrorInstance = codeMirrorInstance;

				// =====================================================================
				// STEP 13: Final refresh and resize handler
				// =====================================================================
				setTimeout(() => {
						codeMirrorInstance.refresh();
						codeMirrorInstance.setCursor({ line: 0, ch: 0 });
				}, 100);

				$(window).on('resize.codemirror', () => {
						if (codeMirrorInstance) {
								codeMirrorInstance.refresh();
						}
				});

				console.log('[CodeMirror] Initialized with UNIFIED CASE-SENSITIVE validation');
				console.log('[CodeMirror] Features:');
				console.log('  - ✓ Case-sensitive validation (using scriptReference)');
				console.log('  - ✓ Real-time validation (optional, disabled by default)');
				console.log('  - ✓ Revalidate button with full case checking');
				console.log('  - ✓ Auto-validation toggle');
				console.log('  - Valid function: GREEN bold');
				console.log('  - Invalid function: RED underline');
				console.log('  - Case error: ORANGE underline');
				console.log('  - Pending function: LIGHT GREEN italic');

		}

    // =====================================================================
    // METADATA DIALOG
    // =====================================================================

    function generateScriptMetadata(options) {
        var defaults = { name: 'My Script', author: 'Unknown', license: 'Public Domain', version: '1.0', shortcut: '', description: 'No description' };
        var meta = Object.assign({}, defaults, options);
        var h = '';
        h += '// Name: ' + meta.name + '\n// Author: ' + meta.author + '\n// License: ' + meta.license + '\n// Version: ' + meta.version + '\n';
        if (meta.shortcut) h += '// Shortcut: ' + meta.shortcut + '\n';
        h += '// Description: ' + meta.description + '\n\n';
        return h;
    }

    function showMetadataDialog() {
        $('#metadata-dialog-overlay').remove();
        var overlay = $(
            '<div id="metadata-dialog-overlay" class="metadata-dialog-overlay"><div class="metadata-dialog">' +
            '<div class="metadata-dialog-header"><h3>Script Header (Metadata)</h3><button class="metadata-dialog-close">&times;</button></div>' +
            '<div class="metadata-dialog-body">' +
            '<p class="metadata-hint">Enter script identification data.</p>' +
            '<div class="metadata-form-group"><label>Script Name <span style="color:#F92672;">*</span></label><input type="text" id="meta-name" placeholder="Example: Planetary Tour"></div>' +
            '<div class="metadata-form-group"><label>Author</label><input type="text" id="meta-author" placeholder="Example: Your Name"></div>' +
            '<div class="metadata-form-group"><label>License</label><select id="meta-license"><option value="Public Domain">Public Domain</option><option value="GPL-2.0">GNU GPL v2.0</option><option value="MIT">MIT</option><option value="CC-BY-4.0">CC BY 4.0</option><option value="All Rights Reserved">All Rights Reserved</option></select></div>' +
            '<div class="metadata-form-group"><label>Version</label><input type="text" id="meta-version" value="1.0"></div>' +
            '<div class="metadata-form-group"><label>Shortcut (optional)</label><input type="text" id="meta-shortcut" placeholder="Ctrl+D,J"></div>' +
            '<div class="metadata-form-group"><label>Description <span style="color:#F92672;">*</span></label><textarea id="meta-description" rows="3" placeholder="Description..."></textarea></div>' +
            '<div class="metadata-form-group"><label>Preview</label><pre id="metadata-preview" class="metadata-preview"></pre></div>' +
            '</div>' +
            '<div class="metadata-dialog-footer"><button class="jquerybutton" id="btn-insert-metadata">Insert at Top</button><button class="jquerybutton" id="btn-replace-metadata">Replace All</button><button class="jquerybutton" id="btn-cancel-metadata">Cancel</button></div>' +
            '</div></div>'
        );
        $('body').append(overlay);
        function updatePreview() {
            var meta = { name: $('#meta-name').val() || 'My Script', author: $('#meta-author').val() || 'Unknown', license: $('#meta-license').val(), version: $('#meta-version').val() || '1.0', shortcut: $('#meta-shortcut').val(), description: $('#meta-description').val() || 'No description' };
            $('#metadata-preview').text(generateScriptMetadata(meta));
        }
        $('#meta-name,#meta-author,#meta-license,#meta-version,#meta-shortcut,#meta-description').on('input change', updatePreview);
        updatePreview();
        $('.metadata-dialog-close, #btn-cancel-metadata').on('click', function() { overlay.remove(); });
        overlay.on('click', function(e) { if (e.target === overlay[0]) overlay.remove(); });
        $('#btn-insert-metadata').on('click', function() {
            var name = $('#meta-name').val().trim(), desc = $('#meta-description').val().trim();
            if (!name || !desc) { alert('Name and description required.'); return; }
            var header = generateScriptMetadata({ name: name, author: $('#meta-author').val() || 'Unknown', license: $('#meta-license').val(), version: $('#meta-version').val() || '1.0', shortcut: $('#meta-shortcut').val(), description: desc });
            setEditorContent(header + getEditorContent());
            overlay.remove(); addOutput('info', 'Header inserted: ' + name);
        });
        $('#btn-replace-metadata').on('click', function() {
            var name = $('#meta-name').val().trim(), desc = $('#meta-description').val().trim();
            if (!name || !desc) { alert('Name and description required.'); return; }
            if (getEditorContent().trim() && !confirm('Replace entire content?')) return;
            var header = generateScriptMetadata({ name: name, author: $('#meta-author').val() || 'Unknown', license: $('#meta-license').val(), version: $('#meta-version').val() || '1.0', shortcut: $('#meta-shortcut').val(), description: desc });
            setEditorContent(header + '// Write your script code here...\n\n');
            overlay.remove(); addOutput('info', 'New script: ' + name);
        });
        $(document).on('keydown.metadataDialog', function(e) { if (e.key === 'Escape') { overlay.remove(); $(document).off('keydown.metadataDialog'); } });
    }

		// =====================================================================
		// COLLECT EXAMPLES FROM MODULE JSON FILES
		// =====================================================================

		/**
		 * Loads a module JSON file and extracts all examples and optionally descriptions.
		 * @param {string} moduleId - Module ID (e.g., 'ConstellationMgr')
		 * @param {string} file - JSON filename (from modules-index.json)
		 * @param {boolean} includeDescription - Whether to include function description
		 * @returns {Promise<Array>} Array of {moduleId, functionName, description, example}
		 */
		function loadModuleExamplesWithDescription(moduleId, file, includeDescription) {
				return new Promise(function(resolve, reject) {
						$.ajax({
								url: '/js/scripteditor/data/' + file,
								dataType: 'json',
								cache: false,
								timeout: 5000
						}).done(function(data) {
								var examples = [];
								if (Array.isArray(data)) {
										data.forEach(function(func) {
												if (func.example && func.example.trim()) {
														var item = {
																moduleId: moduleId,
																functionName: func.name,
																example: func.example
														};
														if (includeDescription && func.description) {
																item.description = func.description;
														}
														examples.push(item);
												}
										});
								}
								resolve(examples);
						}).fail(function() {
								console.warn('[CollectExamples] Failed to load ' + moduleId);
								resolve([]);
						});
				});
		}

		/**
		 * Builds a script text from collected examples.
		 * @param {Array} examples - Array of example objects (may include description)
		 * @param {boolean} includeDescription - Whether to include the description
		 * @param {boolean} commentCode - Whether to comment out each line of code (add //)
		 * @returns {string} Formatted script with comments
		 */
		function buildExamplesScript(examples, includeDescription, commentCode) {
				var script = '';
				var currentModule = '';
				
				examples.forEach(function(ex) {
						if (ex.moduleId !== currentModule) {
								currentModule = ex.moduleId;
								script += '\n\n// ============================================================\n';
								script += '// MODULE: ' + currentModule + '\n';
								script += '// ============================================================\n\n';
						}
						
						script += '// Function: ' + ex.functionName + '\n';
						
						// Include description if available
						if (includeDescription && ex.description) {
								// Break description into multiple comment lines if it has newlines
								var descLines = ex.description.split('\n');
								descLines.forEach(function(line) {
										if (line.trim()) {
												script += '//   ' + line + '\n';
										}
								});
								script += '//\n';
						}
						
						// Process example code lines
						var codeLines = ex.example.split('\n');
						if (commentCode) {
								// Add // at the beginning of each line
								codeLines.forEach(function(line) {
										script += '//   ' + line + '\n';
								});
						} else {
								// Just indent the code (no //)
								codeLines.forEach(function(line) {
										script += '    ' + line + '\n';
								});
						}
						script += '\n';
				});
				
				return script.trim();
		}

		/**
		 * Shows a dialog to select modules, then collects examples and puts them in the editor.
		 */
		function collectExamplesFromModules() {
				if (!scriptReference || !scriptReference.getModuleIndex) {
						addOutput('error', 'Module index not available');
						return;
				}
				
				var moduleIndex = scriptReference.getModuleIndex();
				if (!moduleIndex || !moduleIndex.modules) {
						addOutput('error', 'Invalid module index');
						return;
				}
				
				// Prepare dialog content with new options
				var dialogHtml = '<div id="collect-examples-dialog" title="Select Modules">' +
						'<p>Choose which modules to load examples from:</p>' +
						'<div style="max-height: 300px; overflow-y: auto; border:1px solid #ccc; padding:8px; background: linear-gradient(#8C8C8C, #505050);">' +
						'<label><input type="checkbox" id="select-all-modules"> Select All</label><br>' +
						'<hr style="margin: 5px 0;">';
				
				moduleIndex.modules.forEach(function(mod) {
						dialogHtml += '<label><input type="checkbox" class="module-checkbox" value="' + mod.id + '" data-file="' + mod.file + '"> ' + mod.name + ' (' + mod.id + ')</label><br>';
				});
				
				dialogHtml += '</div><br>' +
						'<div style="margin: 10px 0;">' +
						'<label><input type="checkbox" id="clear-editor-first" checked> Clear editor before inserting</label><br>' +
						'<label><input type="checkbox" id="include-description" checked> Include function description (as comment)</label><br>' +
						'<label><input type="checkbox" id="comment-code"> Comment out the code (add // at start of each line)</label>' +
						'</div>' +
						'</div>';
				
				// Create dialog
				$('body').append('<div id="collect-examples-modal" style="display:none;">' + dialogHtml + '</div>');
				var $dialog = $('#collect-examples-modal').dialog({
						modal: true,
						width: 550,
						buttons: {
								'Load Examples': function() {
										var selectedModules = [];
										$('.module-checkbox:checked').each(function() {
												selectedModules.push({
														id: $(this).val(),
														file: $(this).data('file')
												});
										});
										if (selectedModules.length === 0) {
												addOutput('error', 'No modules selected');
												$(this).dialog('close');
												return;
										}
										var clearFirst = $('#clear-editor-first').is(':checked');
										var includeDescription = $('#include-description').is(':checked');
										var commentCode = $('#comment-code').is(':checked');
										
										$(this).dialog('close');
										
										// Show loading indicator
										addOutput('info', 'Loading examples from ' + selectedModules.length + ' module(s)...');
										
										// Load examples in parallel
										var promises = selectedModules.map(function(mod) {
												return loadModuleExamplesWithDescription(mod.id, mod.file, includeDescription);
										});
										
										Promise.all(promises).then(function(results) {
												var allExamples = [];
												results.forEach(function(examples) {
														allExamples = allExamples.concat(examples);
												});
												
												if (allExamples.length === 0) {
														addOutput('error', 'No examples found in selected modules');
														return;
												}
												
												var scriptText = buildExamplesScript(allExamples, includeDescription, commentCode);
												
												if (clearFirst) {
														// Clear editor
														if (codeMirrorInstance) {
																codeMirrorInstance.setValue('');
														} else {
																var ta = document.getElementById('script-editor');
																if (ta) ta.value = '';
														}
												}
												
												// Insert the script
												if (codeMirrorInstance) {
														var current = codeMirrorInstance.getValue();
														var newContent = (clearFirst ? '' : current + '\n\n') + scriptText;
														codeMirrorInstance.setValue(newContent);
														refreshLineCount();
														addOutput('success', 'Inserted ' + allExamples.length + ' examples into editor');
												} else {
														var ta = document.getElementById('script-editor');
														if (ta) {
																ta.value = (clearFirst ? '' : ta.value + '\n\n') + scriptText;
																addOutput('success', 'Inserted ' + allExamples.length + ' examples into editor');
														}
												}
										}).catch(function(err) {
												console.error(err);
												addOutput('error', 'Failed to load some modules');
										});
								},
								Cancel: function() {
										$(this).dialog('close');
								}
						},
						close: function() {
								$(this).remove();
						}
				});
				
				// Handle Select All
				$('#select-all-modules').off('change').on('change', function() {
						$('.module-checkbox').prop('checked', $(this).is(':checked'));
				});
		}

    // =====================================================================
    // INITIALIZATION
    // =====================================================================

		/**
		 * Initialize the Script Editor module.
		 * FIXED: Wait for scriptReference to load before initializing CodeMirror
		 */
		function init() {
				console.log('[ScriptEditor] Initializing...');
				
				dom = {
						$output: $('#script-output'), $btnRun: $('#btn-editor-run'), $btnRunSelection: $('#btn-editor-run-selection'),
						$btnStop: $('#btn-editor-stop'), $btnClear: $('#btn-editor-clear'), $btnSave: $('#btn-editor-save'),
						$btnOpen: $('#btn-editor-open'), $btnMetadata: $('#btn-editor-metadata'), $btnClearOutput: $('#btn-editor-clear-output'),
						$scriptsList: $('#installed-scripts-list'), $history: $('#execution-history'),
						$templatesSelect: $('#script-templates-select'), $filterInput: $('#scripts-filter-input'),
						$refSearchInput: $('#quick-reference-search'), $btnRevalidate: $('#btn-editor-revalidate'),
						$btnCollectExamples: $('#btn-collect-examples'), $btnCopyEditor: $('#btn-editor-copy')
				};
				
				waitForCodeMirror(function() {
						// FIRST: Create CodeMirror instance with basic mode
						initCodeMirror();
						
						// SECOND: Load scriptReference data
						console.log('[ScriptEditor] Loading scriptReference data...');
						
						scriptReference.init().then(function(allFunctions) {
								console.log('[ScriptEditor] Loaded ' + allFunctions.length + ' API functions from JSON');
								
								// THIRD: Update tokenizer to recognize functions
								if (codeMirrorInstance) {
										codeMirrorInstance.operation(function() {
												codeMirrorInstance.setOption('mode', 'stellarium');
										});
										console.log('[CodeMirror] Tokenizer updated with loaded functions');
								}
								
								// FOURTH: Build quick reference with loaded data
								buildQuickReference();
								
						}).catch(function(err) {
								console.warn('[ScriptEditor] JSON data not available, using built-in reference');
								buildQuickReferenceFromBuiltIn();
						});
						
						// FIFTH: Load other components
						loadInstalledScripts();
						loadHistory();
						refreshAllUIStates();
						bindEvents();
						
						// SIXTH: Setup parameter hints
						if (codeMirrorInstance) {
								setupParameterHints(codeMirrorInstance);
						}
				});
					
				console.log('[ScriptEditor] Initialized');
		}

    function bindEvents() {
        if (dom.$btnRun) dom.$btnRun.off('click').on('click', function(e) { e.preventDefault(); runScript(); });
        if (dom.$btnRunSelection) dom.$btnRunSelection.off('click').on('click', function(e) { e.preventDefault(); runSelection(); });
        if (dom.$btnStop) dom.$btnStop.off('click').on('click', function(e) { e.preventDefault(); stopScript(); });
        if (dom.$btnMetadata) dom.$btnMetadata.off('click').on('click', function(e) { e.preventDefault(); showMetadataDialog(); });
        if (dom.$btnClear) dom.$btnClear.off('click').on('click', function(e) { e.preventDefault(); if (confirm(rc.tr('Clear editor?'))) { setEditorContent(''); clearOutput(); refreshLineCount();} });
        if (dom.$btnSave) dom.$btnSave.off('click').on('click', function(e) { e.preventDefault(); saveScriptToFile(); });
        if (dom.$btnOpen) dom.$btnOpen.off('click').on('click', function(e) { e.preventDefault(); openScriptFromFile(); });
        if (dom.$btnClearOutput) dom.$btnClearOutput.off('click').on('click', function(e) { e.preventDefault(); clearOutput(); });

        if (dom.$filterInput) dom.$filterInput.off('input').on('input', function() {
            var f = $(this).val().toLowerCase();
            $('.script-item').each(function() { $(this).toggle(($(this).find('.script-name').text() || '').toLowerCase().indexOf(f) >= 0); });
        });
        $('#btn-refresh-scripts-list').off('click').on('click', function() { loadInstalledScripts(); });

        // Quick Reference search input
        if (dom.$refSearchInput && dom.$refSearchInput.length) {
            dom.$refSearchInput.off('input').on('input', function() {
                filterQuickReference($(this).val());
            });
        }

        // ActiveScriptChanged from ui/scripts.js
        if (typeof scriptApi !== 'undefined') {
            $(scriptApi).off('activeScriptChanged.scriptEditor').on('activeScriptChanged.scriptEditor', function(evt, scriptName) {
                var name = scriptName || '';
                
                if (name) {
                    // Script is running with a specific ID
                    isRunning = true;
                    currentRunningScriptId = name;
                } else if (typeof arguments[1] === 'undefined' || arguments[1] === '') {
                    // Explicitly stopped (empty string event)
                    isRunning = false;
                    currentRunningScriptId = null;
                }
                // If arguments[1] is undefined (event triggered without value), check status
                
                refreshAllUIStates();
                
                if (name) {
                    startScriptPolling();
                } else {
                    stopScriptPolling();
                }
            });
        }
        
        // Additional: force status check when our direct script might have ended
        // Listen to the global scriptApi for any script status changes
        if (typeof scriptApi !== 'undefined') {
            // Override runDirectScript to track when it's called from editor
            var originalRunDirectScript = scriptApi.runDirectScript;
            if (originalRunDirectScript && !scriptApi._editorWrapped) {
                scriptApi.runDirectScript = function(code, useIncludes) {
                    // Mark that we started a direct script
                    isRunning = true;
                    currentRunningScriptId = null; // null = direct script, no ID
                    refreshAllUIStates();
                    
                    // Call original
                    return originalRunDirectScript.call(scriptApi, code, useIncludes);
                };
                scriptApi._editorWrapped = true;
            }
        }
        
        // Backup property listener
        if (typeof propApi !== 'undefined') {
            $(propApi).off('stelPropertyChanged:StelScriptMgr.runningScriptId.scriptEditor').on('stelPropertyChanged:StelScriptMgr.runningScriptId.scriptEditor', function(evt, data) {
                var name = data.value || '';
                if (!isRunning && name) { isRunning = true; currentRunningScriptId = name; refreshAllUIStates(); }
                else if (isRunning && !name) { isRunning = false; currentRunningScriptId = null; refreshAllUIStates(); }
            });
        }
        
        if (typeof rc !== 'undefined') {
            $(rc).on('scriptStatusUpdated', function(evt, status) {
                if (status.isRunning !== isRunning) {
                    console.log('[ScriptEditor] Script status updated from server:', status);
                    isRunning = status.isRunning;
                    currentRunningScriptId = status.scriptId || null;
                    refreshAllUIStates();
                }
            });
        }
        
        // Additional global keyboard shortcuts for reliability
        $(window).off('keydown.scriptEditorGlobal').on('keydown.scriptEditorGlobal', function(e) {
            // Don't interfere with input fields or modals
            if ($(e.target).is('input, textarea, [contenteditable=true]')) {
                return;
            }
            
            if (e.ctrlKey && e.shiftKey && e.key === 'Enter') {
                e.preventDefault();
                e.stopPropagation();
                console.log('[ScriptEditor] Global shortcut: Ctrl+Shift+Enter');
                runSelection();
                return false;
            }
            
            if (e.ctrlKey && e.key === 'Enter' && !e.shiftKey) {
                e.preventDefault();
                e.stopPropagation();
                console.log('[ScriptEditor] Global shortcut: Ctrl+Enter');
                runScript();
                return false;
            }
        });
				
				 // Go to Line button
        $('#btn-editor-gotoline').off('click').on('click', function(e) {
            e.preventDefault();
            if (codeMirrorInstance) {
                showGoToLineDialog(codeMirrorInstance);
            }
        });
				
				if (dom.$btnCollectExamples) {
							dom.$btnCollectExamples.off('click').on('click', function(e) {
									e.preventDefault();
									collectExamplesFromModules();
							});
					}
					
					if (dom.$btnCopyEditor) {
							dom.$btnCopyEditor.off('click').on('click', function(e) {
									e.preventDefault();
									copyEditorContent();
							});
					}
				
				// Show all shortcuts button
				$('#btn-show-all-shortcuts').off('click').on('click', function(e) {
						e.preventDefault();
						showShortcutsHelpDialog();
				});
				
		// Revalidate button - using the validation system
		if (dom.$btnRevalidate) {
				dom.$btnRevalidate.off('click').on('click', function(e) {
						e.preventDefault();
						if (typeof window.revalidateStelFunctions === 'function') {
								window.revalidateStelFunctions();
						} else {
								console.error('[Revalidate] window.revalidateStelFunctions not defined!');
								// Fallback
								if (codeMirrorInstance && typeof fullRevalidateAllFunctions === 'function') {
										fullRevalidateAllFunctions();
								} else {
										console.error('[Revalidate] No validation function available!');
								}
						}
				});
		} else {
				// Fallback if dom.$btnRevalidate is not defined
				$('#btn-editor-revalidate').off('click').on('click', function(e) {
						e.preventDefault();
						if (typeof window.revalidateStelFunctions === 'function') {
								window.revalidateStelFunctions();
						}
				});
		}
    }
    
    // =====================================================================
    // CONTINUOUS SCRIPT STATUS MONITORING
    // =====================================================================
    
    var scriptPollingInterval = null;
    
    function startScriptPolling() {
        if (scriptPollingInterval) return;
        
        scriptPollingInterval = setInterval(function() {
            // Only poll if a script might be running
            if (!isRunning) return;
            
            $.ajax({
                url: '/api/scripts/status',
                method: 'GET',
                dataType: 'json',
                timeout: 3000,
                success: function(data) {
                    var running = data.scriptIsRunning || false;
                    var scriptName = data.runningScriptId || '';
                    
                    if (!running && isRunning) {
                        console.log('[ScriptEditor] Script completed (polling detected)');
                        isRunning = false;
                        currentRunningScriptId = null;
                        refreshAllUIStates();
                        addOutput('success', 'Script execution completed.');
                    } else if (running && !isRunning) {
                        console.log('[ScriptEditor] Script started (polling detected)');
                        isRunning = true;
                        currentRunningScriptId = scriptName || null;
                        refreshAllUIStates();
                    }
                },
                error: function() {
                    // Silent fail - don't flood console
                }
            });
        }, 2000); // Check every 2 seconds
    }
    
    function stopScriptPolling() {
        if (scriptPollingInterval) {
            clearInterval(scriptPollingInterval);
            scriptPollingInterval = null;
        }
    }
    
    // Expose insertAtCursor globally for Code Generator to use
    window._stelScriptEditor = {
        insertAtCursor: insertAtCursor,
        getEditorContent: getEditorContent,
        setEditorContent: setEditorContent,
        copyToClipboard: copyToClipboard
    };

    // =====================================================================
    // PUBLIC API
    // =====================================================================

    return {
        init: init,
        runScript: runScript,
        runSelection: runSelection,
        stopScript: stopScript,
        clearOutput: clearOutput,
        insertAtCursor: insertAtCursor,
        loadInstalledScripts: loadInstalledScripts,
        saveScriptToFile: saveScriptToFile,
        openScriptFromFile: openScriptFromFile,
    };
});