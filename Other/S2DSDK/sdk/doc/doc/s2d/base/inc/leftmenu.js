function WriteLeftMenu(divID, aID, divClassName, aClassName)
{
document.write("<div id=\"divID1453\" class=\"headerLeftMenuInActive\"><a id=\"aID1453\" href=\"#\" OnMouseOver=\"link('_dir','inc0',this)\" class=\"leftMenuLinkHeadInActive\">inc</a></div>\n");
document.write("<div class=\"paragraphLeftMenu\">Classes</div>\n");
document.write("<div id=\"divID183\" class=\"leftMenuInActive\"><a id=\"aID183\" href=\"#\" OnMouseOver=\"link('_class','AudioData115',this)\" class=\"leftMenuLinkInActive\">AudioData</a></div>\n");
document.write("<div id=\"divID184\" class=\"leftMenuInActive\"><a id=\"aID184\" href=\"#\" OnMouseOver=\"link('_class','AudioPlayer115',this)\" class=\"leftMenuLinkInActive\">AudioPlayer</a></div>\n");
document.write("<div id=\"divID185\" class=\"leftMenuInActive\"><a id=\"aID185\" href=\"#\" OnMouseOver=\"link('_class','FixPoint115',this)\" class=\"leftMenuLinkInActive\">FixPoint</a></div>\n");
document.write("<div id=\"divID187\" class=\"leftMenuInActive\"><a id=\"aID187\" href=\"#\" OnMouseOver=\"link('_class','Int64115',this)\" class=\"leftMenuLinkInActive\">Int64</a></div>\n");
document.write("<div id=\"divID192\" class=\"leftMenuInActive\"><a id=\"aID192\" href=\"#\" OnMouseOver=\"link('_class','Pixel115',this)\" class=\"leftMenuLinkInActive\">Pixel</a></div>\n");
document.write("<div id=\"divID191\" class=\"leftMenuInActive\"><a id=\"aID191\" href=\"#\" OnMouseOver=\"link('_class','RandomGenerator115',this)\" class=\"leftMenuLinkInActive\">RandomGenerator</a></div>\n");
document.write("<div id=\"divID188\" class=\"leftMenuInActive\"><a id=\"aID188\" href=\"#\" OnMouseOver=\"link('_class','Uint64115',this)\" class=\"leftMenuLinkInActive\">Uint64</a></div>\n");
document.write("<div class=\"paragraphLeftMenu\">Structs</div>\n");
document.write("<div id=\"divID189\" class=\"leftMenuInActive\"><a id=\"aID189\" href=\"#\" OnMouseOver=\"link('_class','FILEHandle115',this)\" class=\"leftMenuLinkInActive\">FILEHandle</a></div>\n");
document.write("<div id=\"divID186\" class=\"leftMenuInActive\"><a id=\"aID186\" href=\"#\" OnMouseOver=\"link('_class','ImageInfo115',this)\" class=\"leftMenuLinkInActive\">ImageInfo</a></div>\n");
document.write("<div id=\"divID190\" class=\"leftMenuInActive\"><a id=\"aID190\" href=\"#\" OnMouseOver=\"link('_class','Log115',this)\" class=\"leftMenuLinkInActive\">Log</a></div>\n");
document.write("<div id=\"divID193\" class=\"leftMenuInActive\"><a id=\"aID193\" href=\"#\" OnMouseOver=\"link('_class','VID115',this)\" class=\"leftMenuLinkInActive\">VID</a></div>\n");
if(divID != "" && aID != "")
{
document.getElementById(divID).className = divClassName;
document.getElementById(aID).className = aClassName;
}
}
