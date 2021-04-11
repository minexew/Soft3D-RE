function WriteLeftMenu(divID, aID, divClassName, aClassName)
{
document.write("<div id=\"divID1455\" class=\"headerLeftMenuInActive\"><a id=\"aID1455\" href=\"#\" OnMouseOver=\"link('_dir','inc0',this)\" class=\"leftMenuLinkHeadInActive\">inc</a></div>\n");
document.write("<div class=\"paragraphLeftMenu\">Classes</div>\n");
document.write("<div id=\"divID199\" class=\"leftMenuInActive\"><a id=\"aID199\" href=\"#\" OnMouseOver=\"link('_class','Engine115',this)\" class=\"leftMenuLinkInActive\">Engine</a></div>\n");
document.write("<div id=\"divID201\" class=\"leftMenuInActive\"><a id=\"aID201\" href=\"#\" OnMouseOver=\"link('_class','FadeShader115',this)\" class=\"leftMenuLinkInActive\">FadeShader</a></div>\n");
document.write("<div id=\"divID200\" class=\"leftMenuInActive\"><a id=\"aID200\" href=\"#\" OnMouseOver=\"link('_class','Input115',this)\" class=\"leftMenuLinkInActive\">Input</a></div>\n");
document.write("<div id=\"divID202\" class=\"leftMenuInActive\"><a id=\"aID202\" href=\"#\" OnMouseOver=\"link('_class','State26434',this)\" class=\"leftMenuLinkInActive\">State<></a></div>\n");
document.write("<div id=\"divID205\" class=\"leftMenuInActive\"><a id=\"aID205\" href=\"#\" OnMouseOver=\"link('_class','StateMachine26434',this)\" class=\"leftMenuLinkInActive\">StateMachine<></a></div>\n");
document.write("<div id=\"divID203\" class=\"leftMenuInActive\"><a id=\"aID203\" href=\"#\" OnMouseOver=\"link('_class','StateParam115',this)\" class=\"leftMenuLinkInActive\">StateParam</a></div>\n");
document.write("<div id=\"divID204\" class=\"leftMenuInActive\"><a id=\"aID204\" href=\"#\" OnMouseOver=\"link('_class','StateSet26434',this)\" class=\"leftMenuLinkInActive\">StateSet<></a></div>\n");
document.write("<div id=\"divID207\" class=\"leftMenuInActive\"><a id=\"aID207\" href=\"#\" OnMouseOver=\"link('_class','StringManager115',this)\" class=\"leftMenuLinkInActive\">StringManager</a></div>\n");
document.write("<div id=\"divID206\" class=\"leftMenuInActive\"><a id=\"aID206\" href=\"#\" OnMouseOver=\"link('_class','VisString115',this)\" class=\"leftMenuLinkInActive\">VisString</a></div>\n");
if(divID != "" && aID != "")
{
document.getElementById(divID).className = divClassName;
document.getElementById(aID).className = aClassName;
}
}
