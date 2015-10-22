define(["jquery", "jquery-ui"], function($, jqueryui) {
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

	//disable forced spinner rounding
	//http://stackoverflow.com/questions/19063956/ui-spinner-bug-when-changing-value-using-mouse-up-and-down

	/* //disabled for now until globalize.js works
        proto = $.ui.spinner.prototype;
        $.extend(proto, {
            _adjustValue: function(value) {
                var base, aboveMin,
                    options = this.options;

                // fix precision from bad JS floating point math
                //value = parseFloat(value.toFixed(this._precision()));

                // clamp the value
                if (options.max !== null && value > options.max) {
                    return options.max;
                }
                if (options.min !== null && value < options.min) {
                    return options.min;
                }

                return value;
            }
        });
*/
});