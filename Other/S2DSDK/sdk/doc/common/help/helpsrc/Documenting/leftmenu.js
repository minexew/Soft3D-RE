function WriteLeftMenu(divID, aID, divClassName, aClassName)
{
document.write("<div id=\"divID1780\" class=\"headerLeftMenuInActive\"><a id=\"aID1780\" href=\"#\" OnMouseOver=\"link('_dir','Documenting0',this)\" class=\"leftMenuLinkHeadInActive\">Documenting</a></div>\n");
document.write("<div class=\"paragraphLeftMenu\">Pages</div>\n");
document.write("<div id=\"divID1710\" class=\"leftMenuInActive\"><a id=\"aID1710\" href=\"#\" OnMouseOver=\"link('_page','Autolinkgeneration0',this)\" class=\"leftMenuLinkInActive\">Auto link generation</a></div>\n");
document.write("<div id=\"divID1711\" class=\"leftMenuInActive\"><a id=\"aID1711\" href=\"#\" OnMouseOver=\"link('_page','Directories0',this)\" class=\"leftMenuLinkInActive\">Directories</a></div>\n");
document.write("<div id=\"divID1712\" class=\"leftMenuInActive\"><a id=\"aID1712\" href=\"#\" OnMouseOver=\"link('_page','Documentingthecode0',this)\" class=\"leftMenuLinkInActive\">Documenting the code</a></div>\n");
document.write("<div id=\"divID1713\" class=\"leftMenuInActive\"><a id=\"aID1713\" href=\"#\" OnMouseOver=\"link('_page','ExcludingInput0',this)\" class=\"leftMenuLinkInActive\">Excluding Input</a></div>\n");
document.write("<div id=\"divID1714\" class=\"leftMenuInActive\"><a id=\"aID1714\" href=\"#\" OnMouseOver=\"link('_page','Formulas0',this)\" class=\"leftMenuLinkInActive\">Formulas</a></div>\n");
document.write("<div id=\"divID1715\" class=\"leftMenuInActive\"><a id=\"aID1715\" href=\"#\" OnMouseOver=\"link('_page','Images0',this)\" class=\"leftMenuLinkInActive\">Images</a></div>\n");
document.write("<div id=\"divID1716\" class=\"leftMenuInActive\"><a id=\"aID1716\" href=\"#\" OnMouseOver=\"link('_page','MainPage0',this)\" class=\"leftMenuLinkInActive\">MainPage</a></div>\n");
document.write("<div id=\"divID1717\" class=\"leftMenuInActive\"><a id=\"aID1717\" href=\"#\" OnMouseOver=\"link('_page','Pages0',this)\" class=\"leftMenuLinkInActive\">Pages</a></div>\n");
document.write("<div id=\"divID1718\" class=\"leftMenuInActive\"><a id=\"aID1718\" href=\"#\" OnMouseOver=\"link('_page','SpecialSections0',this)\" class=\"leftMenuLinkInActive\">Special Sections</a></div>\n");
document.write("<div id=\"divID1719\" class=\"leftMenuInActive\"><a id=\"aID1719\" href=\"#\" OnMouseOver=\"link('_page','TextFormatting0',this)\" class=\"leftMenuLinkInActive\">Text Formatting</a></div>\n");
document.write("<div id=\"divID1720\" class=\"leftMenuInActive\"><a id=\"aID1720\" href=\"#\" OnMouseOver=\"link('_page','Units0',this)\" class=\"leftMenuLinkInActive\">Units</a></div>\n");
if(divID != "" && aID != "")
{
document.getElementById(divID).className = divClassName;
document.getElementById(aID).className = aClassName;
}
}
