function WriteLeftMenu(divID, aID, divClassName, aClassName)
{
document.write("<div id=\"divID1779\" class=\"headerLeftMenuInActive\"><a id=\"aID1779\" href=\"#\" OnMouseOver=\"link('_dir','helpsrc0',this)\" class=\"leftMenuLinkHeadInActive\">helpsrc</a></div>\n");
if(divID != "" && aID != "")
{
document.getElementById(divID).className = divClassName;
document.getElementById(aID).className = aClassName;
}
}
