function WriteLeftMenu(divID, aID, divClassName, aClassName)
{
document.write("<div id=\"divID1781\" class=\"headerLeftMenuInActive\"><a id=\"aID1781\" href=\"#\" OnMouseOver=\"link('_dir','NavigatingInDoxygen0',this)\" class=\"leftMenuLinkHeadInActive\">Navigating In Doxygen</a></div>\n");
document.write("<div class=\"paragraphLeftMenu\">Pages</div>\n");
document.write("<div id=\"divID1721\" class=\"leftMenuInActive\"><a id=\"aID1721\" href=\"#\" OnMouseOver=\"link('_page','AnonymousTypes0',this)\" class=\"leftMenuLinkInActive\">AnonymousTypes</a></div>\n");
if(divID != "" && aID != "")
{
document.getElementById(divID).className = divClassName;
document.getElementById(aID).className = aClassName;
}
}
