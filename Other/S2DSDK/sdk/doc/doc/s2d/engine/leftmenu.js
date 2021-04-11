function WriteLeftMenu(divID, aID, divClassName, aClassName)
{
document.write("<div id=\"divID1460\" class=\"headerLeftMenuInActive\"><a id=\"aID1460\" href=\"#\" OnMouseOver=\"link('_dir','engine0',this)\" class=\"leftMenuLinkHeadInActive\">engine</a></div>\n");
document.write("<div class=\"paragraphLeftMenu\">Directories</div>\n");
document.write("<div id=\"divID1455\" class=\"leftMenuInActive\"><a id=\"aID1455\" href=\"#\" OnMouseOver=\"link('_dir','inc/inc0',this)\" class=\"leftMenuLinkInActive\">inc</a></div>\n");
if(divID != "" && aID != "")
{
document.getElementById(divID).className = divClassName;
document.getElementById(aID).className = aClassName;
}
}
