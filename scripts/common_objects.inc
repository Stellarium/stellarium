//
// Name: common objects include file
// License: Public Domain
// Author: Matthew Gates, Alexander Wolf
// Description: Define arrays containing object name (in English).  
//              Defines: planets, constellations, nebulae
//

var planets = new Array("Sun", "Mercury", "Venus", "Moon", "Mars", "Phobos", "Deimos", 
"Jupiter", "Io", "Europa", "Ganymede", "Callisto", "Saturn", "Mimas", "Enceladus", "Tethys", 
"Dione", "Rhea", "Titan", "Hyperion", "Iapetus", "Uranus", "Miranda", "Ariel", "Umbriel", 
"Titania", "Oberon", "Neptune", "Pluto", "Charon", "Solar", "C/2006", "Ceres", "Pallas", "Juno", 
"Vesta", "Eris"); 

var constellations = new Array("Aquila", "Andromeda", "Sculptor", "Ara", "Libra", "Cetus", "Aries",
"Scutum", "Pyxis", "Bootes", "Caelum", "Chamaeleon", "Cancer", "Capricornus", "Carina", "Cassiopeia",
"Centaurus", "Cepheus", "Coma Berenices", "Canes Venatici", "Auriga", "Columba", "Circinus", "Crater",
"Corona Australis", "Corona Borealis", "Corvus", "Crux", "Cygnus", "Delphinus", "Dorado", "Draco",
"Norma", "Eridanus", "Sagitta", "Fornax", "Gemini", "Camelopardalis", "Canis Major", "Ursa Major",
"Grus", "Hercules", "Horologium", "Hydra", "Hydrus", "Indus", "Lacerta", "Monoceros", "Lepus", "Leo",
"Lupus", "Lynx", "Lyra", "Antlia", "Microscopium", "Musca", "Octans", "Apus", "Ophiuchus", "Orion",
"Pavo", "Pegasus", "Pictor", "Perseus", "Equuleus", "Canis Minor", "Leo Minor", "Vulpecula",
"Ursa Minor", "Phoenix", "Pisces", "Piscis Austrinus", "Volans", "Puppis", "Reticulum", "Sagittarius",
"Scorpius", "Scutum", "Serpens", "Sextans", "Mensa", "Taurus", "Telescopium", "Tucana", "Triangulum",
"Triangulum Australe", "Aquarius", "Virgo", "Vela");

var messier = new Array();
var caldwell = new Array();
for(var i=1;i<=109;i++) 
{ 
    messier.push("M" + i); 
    caldwell.push("C" + i); 
}
messier.push("M110");

var neb = new Array("Horsehead nebula", "California nebula", "Rosette nebula", "NGC253", "NGC6946", "NGC869");
var nebulae = neb.concat(messier);

