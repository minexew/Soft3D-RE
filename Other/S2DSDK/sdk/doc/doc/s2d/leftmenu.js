function WriteLeftMenu(divID, aID, divClassName, aClassName)
{
document.write("<div id=\"divID1457\" class=\"headerLeftMenuInActive\"><a id=\"aID1457\" href=\"#\" OnMouseOver=\"link('_dir','s2d0',this)\" class=\"leftMenuLinkHeadInActive\">s2d</a></div>\n");
document.write("<div class=\"paragraphLeftMenu\">Directories</div>\n");
document.write("<div id=\"divID1458\" class=\"leftMenuInActive\"><a id=\"aID1458\" href=\"#\" OnMouseOver=\"link('_dir','base/base0',this)\" class=\"leftMenuLinkInActive\">base</a></div>\n");
document.write("<div id=\"divID1459\" class=\"leftMenuInActive\"><a id=\"aID1459\" href=\"#\" OnMouseOver=\"link('_dir','core/core0',this)\" class=\"leftMenuLinkInActive\">core</a></div>\n");
document.write("<div id=\"divID1460\" class=\"leftMenuInActive\"><a id=\"aID1460\" href=\"#\" OnMouseOver=\"link('_dir','engine/engine0',this)\" class=\"leftMenuLinkInActive\">engine</a></div>\n");
document.write("<div id=\"divID1461\" class=\"leftMenuInActive\"><a id=\"aID1461\" href=\"#\" OnMouseOver=\"link('_dir','math/math0',this)\" class=\"leftMenuLinkInActive\">math</a></div>\n");
if(divID != "" && aID != "")
{
document.getElementById(divID).className = divClassName;
document.getElementById(aID).className = aClassName;
}
}
