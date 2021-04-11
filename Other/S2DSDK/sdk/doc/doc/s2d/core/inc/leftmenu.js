function WriteLeftMenu(divID, aID, divClassName, aClassName)
{
document.write("<div id=\"divID1454\" class=\"headerLeftMenuInActive\"><a id=\"aID1454\" href=\"#\" OnMouseOver=\"link('_dir','inc0',this)\" class=\"leftMenuLinkHeadInActive\">inc</a></div>\n");
document.write("<div class=\"paragraphLeftMenu\">Classes</div>\n");
document.write("<div id=\"divID197\" class=\"leftMenuInActive\"><a id=\"aID197\" href=\"#\" OnMouseOver=\"link('_class','BasicShader115',this)\" class=\"leftMenuLinkInActive\">BasicShader</a></div>\n");
document.write("<div id=\"divID196\" class=\"leftMenuInActive\"><a id=\"aID196\" href=\"#\" OnMouseOver=\"link('_class','Draw2D115',this)\" class=\"leftMenuLinkInActive\">Draw2D</a></div>\n");
document.write("<div id=\"divID198\" class=\"leftMenuInActive\"><a id=\"aID198\" href=\"#\" OnMouseOver=\"link('_class','Texture115',this)\" class=\"leftMenuLinkInActive\">Texture</a></div>\n");
document.write("<div class=\"paragraphLeftMenu\">Structs</div>\n");
document.write("<div id=\"divID195\" class=\"leftMenuInActive\"><a id=\"aID195\" href=\"#\" OnMouseOver=\"link('_class','Point115',this)\" class=\"leftMenuLinkInActive\">Point</a></div>\n");
document.write("<div id=\"divID194\" class=\"leftMenuInActive\"><a id=\"aID194\" href=\"#\" OnMouseOver=\"link('_class','Rect115',this)\" class=\"leftMenuLinkInActive\">Rect</a></div>\n");
if(divID != "" && aID != "")
{
document.getElementById(divID).className = divClassName;
document.getElementById(aID).className = aClassName;
}
}
