//
// Name:        Provide simple translation option for scripts
// License:     Public Domain
// Author:      Matthew Gates
// Description: Simple translation functions for scripts.
//              set translations with the setTr function, then use tr(string)
//		everywhere in your script where you want to get a translated
//		string.  The current application language is taken from the
//		Application Language setting.   See core.setAppLanguage and
//              core.getAppLanguage for details.
//

// declare a global variable to store saved state
var translationStrings = new Array();

// call this function with some string ID
function tr(str)
{
	lang = core.getAppLanguage();
	// core.debug("tr: getting " + lang + " translation for " + str);
	if (translationStrings[lang]==undefined) 
	{ 	
		// core.debug("tr: no lang array");
		return str; 
	}
	else if (translationStrings[lang][str]==undefined) 
	{ 
		// core.debug("tr: no phrase array");
		return str; 
	}
	else 
	{ 
		// core.debug("tr: got it");
		return translationStrings[lang][str]; 
	}
}

function setTr(lang, original, translation)
{
	// core.debug("setTr: " + lang + ", " + original + ", " + translation);
	if (translationStrings[lang]==undefined)
	{
		// core.debug("setTr: making new lang array");
		translationStrings[lang] = new Array();
	}

	translationStrings[lang][original] = translation;

}

function dumpTr() 
{
	core.debug("We have the following translations:");
	for (lang in translationStrings)
	{
		core.debug("Language: " + lang);
		strings = translationStrings[lang];
		for (phrase in strings)
		{
			core.debug(" - " + phrase + " -> " + strings[phrase]);
		}
	}

}

