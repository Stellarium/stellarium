/**
 * JustGage - a handy JavaScript plugin for generating and animating nice & clean dashboard gauges.
 * Copyright (c) 2012 Bojan Djuricic - pindjur(at)gmail(dot)com | http://www.madcog.com
 * Licensed under MIT.
 * Date: 31/07/2012
 * @author Bojan Djuricic  (@Toorshia)
 * @version 1.0
 *
 * http://www.justgage.com
 */

JustGage = function(config) { 
  
  if (!config.id) {alert("Missing id parameter for gauge!"); return false;} 
  if (!document.getElementById(config.id)) {alert("No element with id: \""+config.id+"\" found!"); return false;} 
  
  // configurable parameters
  this.config = 
  {
    // id : string 
    // this is container element id
    id : config.id,
    
    // title : string
    // gauge title 
    title : (config.title) ? config.title : "Title",
    
    // titleFontColor : string
    // color of gauge title 
    titleFontColor : (config.titleFontColor) ? config.titleFontColor : "#999999",
    
    // value : int
    // value gauge is showing 
    value : (config.value) ? config.value : 0,
    
    // valueFontColor : string
    // color of label showing current value
    valueFontColor : (config.valueFontColor) ? config.valueFontColor : "#010101",
    
    // min : int
    // min value
    min : (config.min) ? config.min : 0,
    
    // max : int
    // max value
    max : (config.max) ? config.max : 100,
    
    // showMinMax : bool
    // hide or display min and max values
    showMinMax : (config.showMinMax != null) ? config.showMinMax : true,
    
    // gaugeWidthScale : float
    // width of the gauge element
    gaugeWidthScale : (config.gaugeWidthScale) ? config.gaugeWidthScale : 1.0,
    
    // gaugeColor : string
    // background color of gauge element 
    gaugeColor : (config.gaugeColor) ? config.gaugeColor : "#edebeb",
    
    // label : string
    // text to show below value
    label : (config.label) ? config.label : "",
    
    // showInnerShadow : bool
    // give gauge element small amount of inner shadow 
    showInnerShadow : (config.showInnerShadow != null) ? config.showInnerShadow : true,
    
    // shadowOpacity : int
    // 0 ~ 1
    shadowOpacity : (config.shadowOpacity) ? config.shadowOpacity : 0.2,
    
    // shadowSize: int
    // inner shadow size
    shadowSize : (config.shadowSize) ? config.shadowSize : 5,
    
    // shadowVerticalOffset : int
    // how much shadow is offset from top 
    shadowVerticalOffset : (config.shadowVerticalOffset) ? config.shadowVerticalOffset : 3,
    
    // levelColors : string[]
    // colors of indicator, from lower to upper, in RGB format 
    levelColors : (config.levelColors) ? config.levelColors : percentColors,
    
    // levelColorsGradient : bool
    // whether to use gradual color change for value, or sector-based
    levelColorsGradient : (config.levelColorsGradient != null) ? config.levelColorsGradient : true,
    
    // labelFontColor : string
    // color of label showing label under value
    labelFontColor : (config.labelFontColor) ? config.labelFontColor : "#b3b3b3",
    
    // startAnimationTime : int
    // length of initial animation 
    startAnimationTime : (config.startAnimationTime) ? config.startAnimationTime : 700,
    
    // startAnimationType : string
    // type of initial animation (linear, >, <,  <>, bounce) 
    startAnimationType : (config.startAnimationType) ? config.startAnimationType : ">",
    
    // refreshAnimationTime : int
    // length of refresh animation 
    refreshAnimationTime : (config.refreshAnimationTime) ? config.refreshAnimationTime : 700,
    
    // refreshAnimationType : string
    // type of refresh animation (linear, >, <,  <>, bounce) 
    refreshAnimationType : (config.refreshAnimationType) ? config.refreshAnimationType : ">"
  };
  
  // overflow values
  if (config.value > this.config.max) this.config.value = this.config.max; 
  if (config.value < this.config.min) this.config.value = this.config.min;
  this.originalValue = config.value;
  
  // canvas
  this.canvas = Raphael(this.config.id, "100%", "100%");
  
  // canvas dimensions
  //var canvasW = document.getElementById(this.config.id).clientWidth;
  //var canvasH = document.getElementById(this.config.id).clientHeight;
  var canvasW = getStyle(document.getElementById(this.config.id), "width").slice(0, -2) * 1;
  var canvasH = getStyle(document.getElementById(this.config.id), "height").slice(0, -2) * 1;
  
  // widget dimensions
  var widgetW, widgetH;
  if ((canvasW / canvasH) > 1.25) {
    widgetW = 1.25 * canvasH;
    widgetH = canvasH;
  } else {
    widgetW = canvasW;
    widgetH = canvasW / 1.25;
  }
  
  // delta 
  var dx = (canvasW - widgetW)/2;
  var dy = (canvasH - widgetH)/2;
  
  // title 
  var titleFontSize = ((widgetH / 8) > 10) ? (widgetH / 10) : 10;
  var titleX = dx + widgetW / 2;
  var titleY = dy + widgetH / 6.5;
  
  // value 
  var valueFontSize = ((widgetH / 6.4) > 16) ? (widgetH / 6.4) : 16;
  var valueX = dx + widgetW / 2;
  var valueY = dy + widgetH / 1.4;
  
  // label 
  var labelFontSize = ((widgetH / 16) > 10) ? (widgetH / 16) : 10;
  var labelX = dx + widgetW / 2;
  //var labelY = dy + widgetH / 1.126760563380282;
  var labelY = valueY + valueFontSize / 2 + 6;
  
  // min 
  var minFontSize = ((widgetH / 16) > 10) ? (widgetH / 16) : 10;
  var minX = dx + (widgetW / 10) + (widgetW / 6.666666666666667 * this.config.gaugeWidthScale) / 2 ;
  var minY = dy + widgetH / 1.126760563380282;
  
  // max 
  var maxFontSize = ((widgetH / 16) > 10) ? (widgetH / 16) : 10;
  var maxX = dx + widgetW - (widgetW / 10) - (widgetW / 6.666666666666667 * this.config.gaugeWidthScale) / 2 ;
  var maxY = dy + widgetH / 1.126760563380282;
  
  // parameters
  this.params  = {
    canvasW : canvasW,
    canvasH : canvasH,
    widgetW : widgetW,
    widgetH : widgetH,
    dx : dx,
    dy : dy,
    titleFontSize : titleFontSize,
    titleX : titleX,
    titleY : titleY,
    valueFontSize : valueFontSize,
    valueX : valueX,
    valueY : valueY,
    labelFontSize : labelFontSize,
    labelX : labelX,
    labelY : labelY,
    minFontSize : minFontSize,
    minX : minX,
    minY : minY,
    maxFontSize : maxFontSize,
    maxX : maxX,
    maxY : maxY
  };
  
  // pki - custom attribute for generating gauge paths
  this.canvas.customAttributes.pki = function (value, min, max, w, h, dx, dy, gws) {
    
       var alpha = (1 - (value - min) / (max - min)) * Math.PI ,
          Ro = w / 2 - w / 10,
          Ri = Ro - w / 6.666666666666667 * gws,
          
          Cx = w / 2 + dx,
          Cy = h / 1.25 + dy,
          
          Xo = w / 2 + dx + Ro * Math.cos(alpha),
          Yo = h - (h - Cy) + dy - Ro * Math.sin(alpha),
          Xi = w / 2 + dx + Ri * Math.cos(alpha),
          Yi = h - (h - Cy) + dy - Ri * Math.sin(alpha),
          path;
    
      path += "M" + (Cx - Ri) + "," + Cy + " ";
      path += "L" + (Cx - Ro) + "," + Cy + " ";
      path += "A" + Ro + "," + Ro + " 0 0,1 " + Xo + "," + Yo + " ";
      path += "L" + Xi + "," + Yi + " ";
      path += "A" + Ri + "," + Ri + " 0 0,0 " + (Cx - Ri) + "," + Cy + " ";
      path += "z ";
      return { path: path };
  }  
  
  // gauge
  this.gauge = this.canvas.path().attr({
    "stroke": "none",
    "fill": this.config.gaugeColor,   
    pki: [this.config.max, this.config.min, this.config.max, this.params.widgetW, this.params.widgetH,  this.params.dx, this.params.dy, this.config.gaugeWidthScale]
  });
  this.gauge.id = this.config.id+"-gauge";
  
  // level
  this.level = this.canvas.path().attr({
    "stroke": "none",
    "fill": getColorForPercentage((this.config.value - this.config.min) / (this.config.max - this.config.min), this.config.levelColors, this.config.levelColorsGradient),  
    pki: [this.config.min, this.config.min, this.config.max, this.params.widgetW, this.params.widgetH,  this.params.dx, this.params.dy, this.config.gaugeWidthScale]
  });
  this.level.id = this.config.id+"-level";
  
  // title
  this.txtTitle = this.canvas.text(this.params.titleX, this.params.titleY, this.config.title);
  this.txtTitle. attr({
    "font-size":this.params.titleFontSize,
    "font-weight":"bold",
    "font-family":"Arial",
    "fill":this.config.titleFontColor,
    "fill-opacity":"1"         
  });
  this.txtTitle.id = this.config.id+"-txttitle";
  
  // value
  this.txtValue = this.canvas.text(this.params.valueX, this.params.valueY, this.originalValue);
  this.txtValue. attr({
    "font-size":this.params.valueFontSize,
    "font-weight":"bold",
    "font-family":"Arial",
    "fill":this.config.valueFontColor,
    "fill-opacity":"0"          
  });
  this.txtValue.id = this.config.id+"-txtvalue";
  
  // label
  this.txtLabel = this.canvas.text(this.params.labelX, this.params.labelY, this.config.label);
  this.txtLabel. attr({
    "font-size":this.params.labelFontSize,
    "font-weight":"normal",
    "font-family":"Arial",
    "fill":this.config.labelFontColor,   
    "fill-opacity":"0"
  });
  this.txtLabel.id = this.config.id+"-txtlabel";
  
  // min
  this.txtMin = this.canvas.text(this.params.minX, this.params.minY, this.config.min);
  this.txtMin. attr({
    "font-size":this.params.minFontSize,
    "font-weight":"normal",
    "font-family":"Arial",
    "fill":this.config.labelFontColor,   
    "fill-opacity": (this.config.showMinMax == true)? "1" : "0"
  });
  this.txtMin.id = this.config.id+"-txtmin";
  
  // max
  this.txtMax = this.canvas.text(this.params.maxX, this.params.maxY, this.config.max);
  this.txtMax. attr({
    "font-size":this.params.maxFontSize,
    "font-weight":"normal",
    "font-family":"Arial",
    "fill":this.config.labelFontColor,   
    "fill-opacity": (this.config.showMinMax == true)? "1" : "0"
  });
  this.txtMax.id = this.config.id+"-txtmax";
  
  var defs = this.canvas.canvas.childNodes[1];
  var svg = "http://www.w3.org/2000/svg";
  
  
  
  if (ie < 9) {
    onCreateElementNsReady(function() {
      this.generateShadow();
    });  
  } else {
    this.generateShadow(svg, defs);
  }
  
  // animate 
  this.level.animate({pki: [this.config.value, this.config.min, this.config.max, this.params.widgetW, this.params.widgetH,  this.params.dx, this.params.dy, this.config.gaugeWidthScale]},  this.config.startAnimationTime, this.config.startAnimationType);
  
  this.txtValue.animate({"fill-opacity":"1"}, this.config.startAnimationTime, this.config.startAnimationType); 
  this.txtLabel.animate({"fill-opacity":"1"}, this.config.startAnimationTime, this.config.startAnimationType);  
};

// refresh gauge level
JustGage.prototype.refresh = function(val) {
  // overflow values
  originalVal = val;
  if (val > this.config.max) {val = this.config.max;}
  if (val < this.config.min) {val = this.config.min;}
    
  var color = getColorForPercentage((val - this.config.min) / (this.config.max - this.config.min), this.config.levelColors, this.config.levelColorsGradient);
  this.canvas.getById(this.config.id+"-txtvalue").attr({"text":originalVal});
  this.canvas.getById(this.config.id+"-level").animate({pki: [val, this.config.min, this.config.max, this.params.widgetW, this.params.widgetH,  this.params.dx, this.params.dy, this.config.gaugeWidthScale], "fill":color},  this.config.refreshAnimationTime, this.config.refreshAnimationType);
};

var percentColors = [
  "#a9d70b",
  "#f9c802",
  "#ff0000"
]

JustGage.prototype.generateShadow = function(svg, defs) {
    // FILTER
    var gaussFilter=document.createElementNS(svg,"filter");
    gaussFilter.setAttribute("id", this.config.id + "-inner-shadow");
    defs.appendChild(gaussFilter);
    
    // offset
    var feOffset = document.createElementNS(svg,"feOffset");
    feOffset.setAttribute("dx", 0);
    feOffset.setAttribute("dy", this.config.shadowVerticalOffset);
    gaussFilter.appendChild(feOffset);
    
    // blur
    var feGaussianBlur = document.createElementNS(svg,"feGaussianBlur");
    feGaussianBlur.setAttribute("result","offset-blur");
    feGaussianBlur.setAttribute("stdDeviation", this.config.shadowSize);
    gaussFilter.appendChild(feGaussianBlur);
    
    // composite 1
    var feComposite1 = document.createElementNS(svg,"feComposite");
    feComposite1.setAttribute("operator","out");
    feComposite1.setAttribute("in", "SourceGraphic");
    feComposite1.setAttribute("in2","offset-blur");
    feComposite1.setAttribute("result","inverse");
    gaussFilter.appendChild(feComposite1);
    
    // flood
    var feFlood = document.createElementNS(svg,"feFlood");
    feFlood.setAttribute("flood-color","black");
    feFlood.setAttribute("flood-opacity", this.config.shadowOpacity);
    feFlood.setAttribute("result","color");
    gaussFilter.appendChild(feFlood);
    
    // composite 2
    var feComposite2 = document.createElementNS(svg,"feComposite");
    feComposite2.setAttribute("operator","in");
    feComposite2.setAttribute("in", "color");
    feComposite2.setAttribute("in2","inverse");
    feComposite2.setAttribute("result","shadow");
    gaussFilter.appendChild(feComposite2);
    
    // composite 3
    var feComposite3 = document.createElementNS(svg,"feComposite");
    feComposite3.setAttribute("operator","over");
    feComposite3.setAttribute("in", "shadow");
    feComposite3.setAttribute("in2","SourceGraphic");
    gaussFilter.appendChild(feComposite3);

    // set shadow
    if (this.config.showInnerShadow == true) {
      this.canvas.canvas.childNodes[2].setAttribute("filter", "url(#" + this.config.id + "-inner-shadow)");
      this.canvas.canvas.childNodes[3].setAttribute("filter", "url(#" + this.config.id + "-inner-shadow)");
    }
}

var getColorForPercentage = function(pct, col, grad) {
    
    var no = col.length;
    if (no === 1) return col[0];
    var inc = (grad) ? (1 / (no - 1)) : (1 / no);
    var colors = new Array();
    for (var i = 0; i < col.length; i++) {
      var percentage = (grad) ? (inc * i) : (inc * (i + 1));
      var rval = parseInt((cutHex(col[i])).substring(0,2),16);
      var gval = parseInt((cutHex(col[i])).substring(2,4),16);
      var bval = parseInt((cutHex(col[i])).substring(4,6),16);
      colors[i] = { pct: percentage, color: { r: rval, g: gval, b: bval  } };
    }
        
    if(pct == 0) return 'rgb(' + [colors[0].color.r, colors[0].color.g, colors[0].color.b].join(',') + ')';
      for (var i = 0; i < colors.length; i++) {
          if (pct <= colors[i].pct) {
            if (grad == true) {
              var lower = colors[i - 1];
              var upper = colors[i];
              var range = upper.pct - lower.pct;
              var rangePct = (pct - lower.pct) / range;
              var pctLower = 1 - rangePct;
              var pctUpper = rangePct;
              var color = {
                  r: Math.floor(lower.color.r * pctLower + upper.color.r * pctUpper),
                  g: Math.floor(lower.color.g * pctLower + upper.color.g * pctUpper),
                  b: Math.floor(lower.color.b * pctLower + upper.color.b * pctUpper)
              };
              return 'rgb(' + [color.r, color.g, color.b].join(',') + ')';
            } else {
              return 'rgb(' + [colors[i].color.r, colors[i].color.g, colors[i].color.b].join(',') + ')'; 
            }
          }
      }
} 

function getRandomInt (min, max) {
    return Math.floor(Math.random() * (max - min + 1)) + min;
}    

function cutHex(str) {return (str.charAt(0)=="#") ? str.substring(1,7):str}

function getStyle(oElm, strCssRule){
	var strValue = "";
	if(document.defaultView && document.defaultView.getComputedStyle){
		strValue = document.defaultView.getComputedStyle(oElm, "").getPropertyValue(strCssRule);
	}
	else if(oElm.currentStyle){
		strCssRule = strCssRule.replace(/\-(\w)/g, function (strMatch, p1){
			return p1.toUpperCase();
		});
		strValue = oElm.currentStyle[strCssRule];
	}
	return strValue;
}

 function onCreateElementNsReady(func) {
  if (document.createElementNS != undefined) {
    func();
  } else {
    setTimeout(function() { onCreateElementNsReady(func); }, 100);
  }
}

// ----------------------------------------------------------
// A short snippet for detecting versions of IE in JavaScript
// without resorting to user-agent sniffing
// ----------------------------------------------------------
// If you're not in IE (or IE version is less than 5) then:
// ie === undefined
// If you're in IE (>=5) then you can determine which version:
// ie === 7; // IE7
// Thus, to detect IE:
// if (ie) {}
// And to detect the version:
// ie === 6 // IE6
// ie > 7 // IE8, IE9 ...
// ie < 9 // Anything less than IE9
// ----------------------------------------------------------

// UPDATE: Now using Live NodeList idea from @jdalton

var ie = (function(){

    var undef,
        v = 3,
        div = document.createElement('div'),
        all = div.getElementsByTagName('i');

    while (
        div.innerHTML = '<!--[if gt IE ' + (++v) + ']><i></i><![endif]-->',
        all[0]
    );

    return v > 4 ? v : undef;

}());