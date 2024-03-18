define(["jquery", "jquery.ui.touch-punch"], function($, jqueryui) {
	"use strict";

	//fix for jQuery UI autocomplete to respect the size of the "of" positional parameter
	//http://bugs.jqueryui.com/ticket/9056
	var proto = $.ui.autocomplete.prototype;
	$.extend(proto, {
		_resizeMenu: function() {
			var ul = this.menu.element;
			ul.outerWidth(Math.max(
				// Firefox wraps long text (possibly a rounding bug)
				// so we add 1px to avoid the wrapping (#7513)
				ul.width("").outerWidth() + 1, ('of' in this.options.position) ? this.options.position.of.outerWidth() : this.element.outerWidth()
			));
		}
	});

	$.widget("ui.spinner", $.ui.spinner, {
		//override spinner _adjustValue to round to the Globalize format, if possible
		_adjustValue: function(value) {
			var options = this.options;

			//convert precision to the given number format, if possible
			if (window.Globalize && this.options.numberFormat) {
				//convert to string and parse again
				//ugly, but working
				value = Globalize.format(value, this.options.numberFormat, this.options.culture);
				value = Globalize.parseFloat(value, 10, this.options.culture);
			} else {
				//use the default jQuery UI way - this precision depends on min,max and step precision
				value = parseFloat(value.toFixed(this._precision()));
			}

			// clamp the value
			if (options.max !== null && value > options.max) {
				return options.max;
			}
			if (options.min !== null && value < options.min) {
				return options.min;
			}

			return value;
		},

		_create: function() {
			this._super();

			//register additional events to react to keyboard and touch input
			this._on({

				//better touch support for buttons with repeating
				"touchstart .ui-spinner-button": function(event) {
					if (this._start(event) === false) {
						return;
					}

					this._repeat(null, $(event.currentTarget).hasClass("ui-spinner-up") ? 1 : -1, event);
					//prevent scrolling
					event.preventDefault();
				},

				//prevent scrolling
				"touchmove .ui-spinner-button": function(event) {
					event.preventDefault();
				},

				"touchend .ui-spinner-button": function(event) {
					this._stop();
				},

				focus: function(event) {
					//make sure this is correct before input happens
					this.prevVal = this.value();
				},

				//ensure number is correctly formatted after focus loss
				blur: function(event) {
					this.element.val(this._format(this.value()));
				},

				keydown: function(event) {
					this.prevVal = this.value();
					//prevent leading zeroes
					var elem = this.element[0];
					if ('selectionStart' in elem) {
						if (event.which === 48 && elem.selectionStart === 0 && elem.selectionEnd === 0)
							event.preventDefault();
					}

				},

				input: function(event) {
					//this.value() performs parsing and returns null on invalid value (e.g. when alphabetic input etc.)
					var val = this.value();
					//previous is set by jQuery UI on focus
					//prevVal is set on focus AND keydown for rolling back invalid keyboard/paste input
					console.log("previous: " + this.previous + ",prevVal: " + this.prevVal + ",val: " + val);

					if (!val) {
						//value set failed, roll back to previous value
						val = this.prevVal;
						this.value(val);
					} else {
						//force rounding to precision and do bounds checking
						var newval = this._adjustValue(val);
						if (newval !== val) {
							val = newval;
							this.value(val);
						}
					}

					console.log("val after adjust: " + val);

					if (val !== this.prevVal) {
						this._trigger("userinput", event, {
							value: this.value()
						});
					}
				}
			});
		},

		//select whole input field when spin is performed
		_repeat: function(i, steps, event) {
			this._super(i, steps, event);

			//if we have reasonable suspicion this is a touch device (set by touch-punch), dont select text to avoid bringing the keyboard up
			if (!$.support.touch)
				this.element.select();
		},

		//override spin to fire input event
		_spin: function(step, event) {
			var val = this.value();
			this._super(step, event);
			if (val !== this.value())
				this._trigger("userinput", event, {
					value: this.value()
				});
		},

		//re-implement _value to prevent caret jumping around on input
		_value: function(value, allowAny) {
			var parsed;
			if (value !== "") {
				parsed = this._parse(value);
				if (parsed !== null) {
					if (!allowAny) {
						parsed = this._adjustValue(parsed);
					}
					value = this._format(parsed);
				}
			}
			//add additional check if UI really needs updating
			if (value !== this.value()) {
				this.element.val(value);
				this._refresh();
			}
		}

	});
});