function WriteLeftMenu(divID, aID, divClassName, aClassName)
{
document.write("<div id=\"divID1461\" class=\"headerLeftMenuInActive\"><a id=\"aID1461\" href=\"#\" OnMouseOver=\"link('_dir','math0',this)\" class=\"leftMenuLinkHeadInActive\">math</a></div>\n");
document.write("<div class=\"paragraphLeftMenu\">Directories</div>\n");
document.write("<div id=\"divID1456\" class=\"leftMenuInActive\"><a id=\"aID1456\" href=\"#\" OnMouseOver=\"link('_dir','inc/inc0',this)\" class=\"leftMenuLinkInActive\">inc</a></div>\n");
if(divID != "" && aID != "")
{
document.getElementById(divID).className = divClassName;
document.getElementById(aID).className = aClassName;
}
}
