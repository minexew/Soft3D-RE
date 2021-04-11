function WriteLeftMenu(divID, aID, divClassName, aClassName)
{
document.write("<div id=\"divID1785\" class=\"headerLeftMenuInActive\"><a id=\"aID1785\" href=\"#\" OnMouseOver=\"link('_dir','Inheritance0',this)\" class=\"leftMenuLinkHeadInActive\">Inheritance</a></div>\n");
document.write("<div class=\"paragraphLeftMenu\">Classes</div>\n");
document.write("<div id=\"divID1613\" class=\"leftMenuInActive\"><a id=\"aID1613\" href=\"#\" OnMouseOver=\"link('_class','BaseClass0',this)\" class=\"leftMenuLinkInActive\">BaseClass</a></div>\n");
document.write("<div id=\"divID1614\" class=\"leftMenuInActive\"><a id=\"aID1614\" href=\"#\" OnMouseOver=\"link('_class','DerivedLevel_10',this)\" class=\"leftMenuLinkInActive\">DerivedLevel_1</a></div>\n");
document.write("<div id=\"divID1615\" class=\"leftMenuInActive\"><a id=\"aID1615\" href=\"#\" OnMouseOver=\"link('_class','DerivedLevel_20',this)\" class=\"leftMenuLinkInActive\">DerivedLevel_2</a></div>\n");
document.write("<div id=\"divID1616\" class=\"leftMenuInActive\"><a id=\"aID1616\" href=\"#\" OnMouseOver=\"link('_class','Final_10',this)\" class=\"leftMenuLinkInActive\">Final_1</a></div>\n");
document.write("<div id=\"divID1617\" class=\"leftMenuInActive\"><a id=\"aID1617\" href=\"#\" OnMouseOver=\"link('_class','Final_20',this)\" class=\"leftMenuLinkInActive\">Final_2</a></div>\n");
if(divID != "" && aID != "")
{
document.getElementById(divID).className = divClassName;
document.getElementById(aID).className = aClassName;
}
}
