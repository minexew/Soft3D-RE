function WriteLeftMenu(divID, aID, divClassName, aClassName)
{
document.write("<div id=\"divID0\" class=\"headerLeftMenuInActive\"><a id=\"aID0\" href=\"#\" OnMouseOver=\"link('','index',this)\" class=\"leftMenuLinkHeadInActive\">Default mainpage</a></div>\n");
document.write("<div class=\"paragraphLeftMenu\">Directories</div>\n");
document.write("<div id=\"divID1457\" class=\"leftMenuInActive\"><a id=\"aID1457\" href=\"#\" OnMouseOver=\"link('_dir','doc/s2d/s2d0',this)\" class=\"leftMenuLinkInActive\">s2d</a></div>\n");
document.write("<div class=\"paragraphLeftMenu\">Miscellaneous</div>\n");
document.write("<div id=\"divID181\" class=\"leftMenuInActive\"><a id=\"aID181\" href=\"#\" OnMouseOver=\"link('_statsindex','common/statistics/index',this)\" class=\"leftMenuLinkInActive\">Statistics</a></div>\n");
if(divID != "" && aID != "")
{
document.getElementById(divID).className = divClassName;
document.getElementById(aID).className = aClassName;
}
}
