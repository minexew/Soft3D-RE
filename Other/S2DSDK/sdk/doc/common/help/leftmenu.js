function WriteLeftMenu(divID, aID, divClassName, aClassName)
{
document.write("<div id=\"divID0\" class=\"headerLeftMenuInActive\"><a id=\"aID0\" href=\"#\" OnMouseOver=\"link('','index',this)\" class=\"leftMenuLinkHeadInActive\">Help</a></div>\n");
document.write("<div class=\"paragraphLeftMenu\">Directories</div>\n");
document.write("<div id=\"divID1783\" class=\"leftMenuInActive\"><a id=\"aID1783\" href=\"#\" OnMouseOver=\"link('_dir','helpsrc/Command_Reference/CommandReference0',this)\" class=\"leftMenuLinkInActive\">Command Reference</a></div>\n");
document.write("<div id=\"divID1782\" class=\"leftMenuInActive\"><a id=\"aID1782\" href=\"#\" OnMouseOver=\"link('_dir','helpsrc/Config_Options/ConfigOptions0',this)\" class=\"leftMenuLinkInActive\">Config Options</a></div>\n");
document.write("<div id=\"divID1780\" class=\"leftMenuInActive\"><a id=\"aID1780\" href=\"#\" OnMouseOver=\"link('_dir','helpsrc/Documenting/Documenting0',this)\" class=\"leftMenuLinkInActive\">Documenting</a></div>\n");
document.write("<div id=\"divID1784\" class=\"leftMenuInActive\"><a id=\"aID1784\" href=\"#\" OnMouseOver=\"link('_dir','helpsrc/Example_Code/ExampleCode0',this)\" class=\"leftMenuLinkInActive\">Example Code</a></div>\n");
document.write("<div id=\"divID1781\" class=\"leftMenuInActive\"><a id=\"aID1781\" href=\"#\" OnMouseOver=\"link('_dir','helpsrc/Navigating_In_Doxygen/NavigatingInDoxygen0',this)\" class=\"leftMenuLinkInActive\">Navigating In Doxygen</a></div>\n");
document.write("<div class=\"paragraphLeftMenu\">Pages</div>\n");
document.write("<div id=\"divID1709\" class=\"leftMenuInActive\"><a id=\"aID1709\" href=\"#\" OnMouseOver=\"link('_page','CompilingDoxyS0',this)\" class=\"leftMenuLinkInActive\">Compiling DoxyS</a></div>\n");
document.write("<div id=\"divID1707\" class=\"leftMenuInActive\"><a id=\"aID1707\" href=\"#\" OnMouseOver=\"link('_page','InstallingDoxyS0',this)\" class=\"leftMenuLinkInActive\">Installing DoxyS</a></div>\n");
document.write("<div id=\"divID1708\" class=\"leftMenuInActive\"><a id=\"aID1708\" href=\"#\" OnMouseOver=\"link('_page','RunningDoxyS0',this)\" class=\"leftMenuLinkInActive\">Running DoxyS</a></div>\n");
document.write("<div class=\"paragraphLeftMenu\">Miscellaneous</div>\n");
document.write("<div id=\"divID1610\" class=\"leftMenuInActive\"><a id=\"aID1610\" href=\"#\" OnMouseOver=\"link('_statsindex','common/statistics/index',this)\" class=\"leftMenuLinkInActive\">Statistics</a></div>\n");
if(divID != "" && aID != "")
{
document.getElementById(divID).className = divClassName;
document.getElementById(aID).className = aClassName;
}
}
