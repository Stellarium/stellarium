define(["jquery"], function($) {
	"use strict";

	//a simple manager for flag-like checkboxes, which have their associated bitflag set in HTML
	function Flags(parentElement, setterFunction) {
		if (!(this instanceof Flags))
			return new Flags(parentElement, setterFunction);

		this.$elem = $(parentElement);
		this.setter = setterFunction;
		this.currentValue = 0;


		//add an event handler
		this.$elem.on("click", "input[type='checkbox']", {flags: this}, function(evt) {

			var bitfield = parseInt(evt.target.value, 16);

			if (!isNaN(bitfield)) {
				if (evt.target.checked)
					evt.data.flags.currentValue |= bitfield;
				else
					evt.data.flags.currentValue &= ~bitfield;

				evt.data.flags.setter(evt.data.flags.currentValue);
			} else {
				console.log("invalid flag checkbox value!");
			}

		});
	}

	//set a new value, toggling checkboxes with respect to the bitfields they represent
	Flags.prototype.setValue = function(val) {
		//only do this if value really changed, for performance
		if (val !== this.currentValue) {
			//iterate over checkbox children
			this.$elem.find('input[type="checkbox"]').each(function() {
				var bitfield = parseInt(this.value, 16);

				if (!isNaN(bitfield)) {
					this.checked = (bitfield & val);
				} else {
					console.log("invalid flag checkbox value!");
				}
			});

			this.currentValue = val;
		}
	};

	return Flags;

});