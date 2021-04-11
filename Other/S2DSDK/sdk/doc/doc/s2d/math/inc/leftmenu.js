function WriteLeftMenu(divID, aID, divClassName, aClassName)
{
document.write("<div id=\"divID1456\" class=\"headerLeftMenuInActive\"><a id=\"aID1456\" href=\"#\" OnMouseOver=\"link('_dir','inc0',this)\" class=\"leftMenuLinkHeadInActive\">inc</a></div>\n");
document.write("<div class=\"paragraphLeftMenu\">Classes</div>\n");
document.write("<div id=\"divID208\" class=\"leftMenuInActive\"><a id=\"aID208\" href=\"#\" OnMouseOver=\"link('_class','Color3115',this)\" class=\"leftMenuLinkInActive\">Color3</a></div>\n");
document.write("<div id=\"divID209\" class=\"leftMenuInActive\"><a id=\"aID209\" href=\"#\" OnMouseOver=\"link('_class','Color4115',this)\" class=\"leftMenuLinkInActive\">Color4</a></div>\n");
if(divID != "" && aID != "")
{
document.getElementById(divID).className = divClassName;
document.getElementById(aID).className = aClassName;
}
}
