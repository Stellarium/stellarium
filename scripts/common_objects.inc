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

var herschel400 = new Array();
var h400 = [   40,  129,  136,  157,  185,  205,  225,  246,  247,  253,  278,  288,  381,  404,  436,  457,
              488,  524,  559,  584,  596,  598,  613,  615,  637,  650,  654,  659,  663,  720,  752,  772,
              779,  869,  884,  891,  908,  936, 1022, 1023, 1027, 1052, 1055, 1084, 1245, 1342, 1407, 1444,
             1501, 1502, 1513, 1528, 1535, 1545, 1647, 1664, 1788, 1817, 1857, 1907, 1931, 1961, 1964, 1980,
             1999, 2022, 2024, 2126, 2129, 2158, 2169, 2185, 2186, 2194, 2204, 2215, 2232, 2244, 2251, 2264,
             2266, 2281, 2286, 2301, 2304, 2311, 2324, 2335, 2343, 2353, 2354, 2355, 2360, 2362, 2371, 2372,
             2392, 2395, 2403, 2419, 2420, 2421, 2422, 2423, 2438, 2440, 2479, 2482, 2489, 2506, 2509, 2527,
             2539, 2548, 2567, 2571, 2613, 2627, 2655, 2681, 2683, 2742, 2768, 2775, 2782, 2787, 2811, 2841,
             2859, 2903, 2950, 2964, 2974, 2976, 2985, 3034, 3077, 3079, 3115, 3147, 3166, 3169, 3184, 3190,
             3193, 3198, 3226, 3227, 3242, 3245, 3277, 3294, 3310, 3344, 3377, 3379, 3384, 3395, 3412, 3414,
             3432, 3486, 3489, 3504, 3521, 3556, 3593, 3607, 3608, 3610, 3613, 3619, 3621, 3626, 3628, 3631,
             3640, 3655, 3665, 3675, 3686, 3726, 3729, 3810, 3813, 3877, 3893, 3898, 3900, 3912, 3938, 3941,
             3945, 3949, 3953, 3962, 3982, 3992, 3998, 4026, 4027, 4030, 4036, 4039, 4041, 4051, 4085, 4088,
             4102, 4111, 4143, 4147, 4150, 4151, 4179, 4203, 4214, 4216, 4245, 4251, 4258, 4261, 4273, 4274,
             4278, 4281, 4293, 4303, 4314, 4346, 4350, 4361, 4365, 4371, 4394, 4414, 4419, 4429, 4435, 4438,
             4442, 4448, 4449, 4450, 4459, 4473, 4477, 4478, 4485, 4490, 4494, 4526, 4527, 4535, 4536, 4546,
             4548, 4550, 4559, 4565, 4570, 4594, 4596, 4618, 4631, 4636, 4643, 4654, 4656, 4660, 4665, 4666,
             4689, 4697, 4698, 4699, 4725, 4753, 4754, 4762, 4781, 4800, 4845, 4856, 4866, 4900, 4958, 4995,
             5005, 5033, 5054, 5195, 5248, 5273, 5322, 5363, 5364, 5466, 5473, 5474, 5557, 5566, 5576, 5631,
             5634, 5676, 5689, 5694, 5746, 5846, 5866, 5897, 5907, 5982, 6118, 6144, 6171, 6207, 6217, 6229,
             6235, 6284, 6287, 6293, 6304, 6316, 6342, 6355, 6356, 6369, 6401, 6426, 6440, 6445, 6451, 6514,
             6517, 6520, 6522, 6528, 6540, 6543, 6544, 6553, 6568, 6569, 6583, 6624, 6629, 6633, 6638, 6642,
             6645, 6664, 6712, 6755, 6756, 6781, 6802, 6818, 6823, 6826, 6830, 6834, 6866, 6882, 6885, 6905,
             6910, 6934, 6939, 6940, 6946, 7000, 7006, 7008, 7009, 7044, 7062, 7086, 7128, 7142, 7160, 7209,
             7217, 7243, 7296, 7331, 7380, 7448, 7479, 7510, 7606, 7662, 7686, 7723, 7727, 7789, 7790, 7814];
for(var i=0; i<h400.length; i++)
{
    herschel400.push("NGC " + h400[i]);
}
