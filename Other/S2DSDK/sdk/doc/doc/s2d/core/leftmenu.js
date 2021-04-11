function WriteLeftMenu(divID, aID, divClassName, aClassName)
{
document.write("<div id=\"divID1459\" class=\"headerLeftMenuInActive\"><a id=\"aID1459\" href=\"#\" OnMouseOver=\"link('_dir','core0',this)\" class=\"leftMenuLinkHeadInActive\">core</a></div>\n");
document.write("<div class=\"paragraphLeftMenu\">Directories</div>\n");
document.write("<div id=\"divID1454\" class=\"leftMenuInActive\"><a id=\"aID1454\" href=\"#\" OnMouseOver=\"link('_dir','inc/inc0',this)\" class=\"leftMenuLinkInActive\">inc</a></div>\n");
if(divID != "" && aID != "")
{
document.getElementById(divID).className = divClassName;
document.getElementById(aID).className = aClassName;
}
}
