function WriteLeftMenu(divID, aID, divClassName, aClassName)
{
document.write("<div id=\"divID1784\" class=\"headerLeftMenuInActive\"><a id=\"aID1784\" href=\"#\" OnMouseOver=\"link('_dir','ExampleCode0',this)\" class=\"leftMenuLinkHeadInActive\">Example Code</a></div>\n");
document.write("<div class=\"paragraphLeftMenu\">Directories</div>\n");
document.write("<div id=\"divID1785\" class=\"leftMenuInActive\"><a id=\"aID1785\" href=\"#\" OnMouseOver=\"link('_dir','Inheritance/Inheritance0',this)\" class=\"leftMenuLinkInActive\">Inheritance</a></div>\n");
document.write("<div id=\"divID1786\" class=\"leftMenuInActive\"><a id=\"aID1786\" href=\"#\" OnMouseOver=\"link('_dir','Separate_Docs/SeparateDocs0',this)\" class=\"leftMenuLinkInActive\">Separate Docs</a></div>\n");
document.write("<div class=\"paragraphLeftMenu\">Classes</div>\n");
document.write("<div id=\"divID1626\" class=\"leftMenuInActive\"><a id=\"aID1626\" href=\"#\" OnMouseOver=\"link('_class','ClassInUnit0',this)\" class=\"leftMenuLinkInActive\">ClassInUnit</a></div>\n");
document.write("<div id=\"divID1612\" class=\"leftMenuInActive\"><a id=\"aID1612\" href=\"#\" OnMouseOver=\"link('_class','HowToDocument0',this)\" class=\"leftMenuLinkInActive\">HowToDocument</a></div>\n");
document.write("<div id=\"divID1618\" class=\"leftMenuInActive\"><a id=\"aID1618\" href=\"#\" OnMouseOver=\"link('_class','Overload0',this)\" class=\"leftMenuLinkInActive\">Overload</a></div>\n");
document.write("<div id=\"divID1622\" class=\"leftMenuInActive\"><a id=\"aID1622\" href=\"#\" OnMouseOver=\"link('_class','Template23971',this)\" class=\"leftMenuLinkInActive\">Template<></a></div>\n");
document.write("<div class=\"paragraphLeftMenu\">Units</div>\n");
document.write("<div id=\"divID1787\" class=\"leftMenuInActive\"><a id=\"aID1787\" href=\"#\" OnMouseOver=\"link('_unit','Unit_h0',this)\" class=\"leftMenuLinkInActive\">Unit.h</a></div>\n");
if(divID != "" && aID != "")
{
document.getElementById(divID).className = divClassName;
document.getElementById(aID).className = aClassName;
}
}
