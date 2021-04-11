function WriteLeftMenu(divID, aID, divClassName, aClassName)
{
document.write("<div id=\"divID1458\" class=\"headerLeftMenuInActive\"><a id=\"aID1458\" href=\"#\" OnMouseOver=\"link('_dir','base0',this)\" class=\"leftMenuLinkHeadInActive\">base</a></div>\n");
document.write("<div class=\"paragraphLeftMenu\">Directories</div>\n");
document.write("<div id=\"divID1453\" class=\"leftMenuInActive\"><a id=\"aID1453\" href=\"#\" OnMouseOver=\"link('_dir','inc/inc0',this)\" class=\"leftMenuLinkInActive\">inc</a></div>\n");
if(divID != "" && aID != "")
{
document.getElementById(divID).className = divClassName;
document.getElementById(aID).className = aClassName;
}
}
