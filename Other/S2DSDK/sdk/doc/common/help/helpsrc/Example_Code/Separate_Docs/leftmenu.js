function WriteLeftMenu(divID, aID, divClassName, aClassName)
{
document.write("<div id=\"divID1786\" class=\"headerLeftMenuInActive\"><a id=\"aID1786\" href=\"#\" OnMouseOver=\"link('_dir','SeparateDocs0',this)\" class=\"leftMenuLinkHeadInActive\">Separate Docs</a></div>\n");
document.write("<div class=\"paragraphLeftMenu\">Classes</div>\n");
document.write("<div id=\"divID1619\" class=\"leftMenuInActive\"><a id=\"aID1619\" href=\"#\" OnMouseOver=\"link('_class','NamespaceClass5138',this)\" class=\"leftMenuLinkInActive\">NamespaceClass</a></div>\n");
document.write("<div id=\"divID1620\" class=\"leftMenuInActive\"><a id=\"aID1620\" href=\"#\" OnMouseOver=\"link('_class','SepDocClass0',this)\" class=\"leftMenuLinkInActive\">SepDocClass</a></div>\n");
if(divID != "" && aID != "")
{
document.getElementById(divID).className = divClassName;
document.getElementById(aID).className = aClassName;
}
}
