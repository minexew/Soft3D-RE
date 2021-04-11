function WriteLeftMenu(divID, aID, divClassName, aClassName)
{
document.write("<div id=\"divID1462\" class=\"headerLeftMenuInActive\"><a id=\"aID1462\" href=\"#\" OnMouseOver=\"link('_dir','doc0',this)\" class=\"leftMenuLinkHeadInActive\">doc</a></div>\n");
if(divID != "" && aID != "")
{
document.getElementById(divID).className = divClassName;
document.getElementById(aID).className = aClassName;
}
}
