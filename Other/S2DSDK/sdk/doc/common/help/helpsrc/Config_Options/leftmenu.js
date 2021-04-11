function WriteLeftMenu(divID, aID, divClassName, aClassName)
{
document.write("<div id=\"divID1782\" class=\"headerLeftMenuInActive\"><a id=\"aID1782\" href=\"#\" OnMouseOver=\"link('_dir','ConfigOptions0',this)\" class=\"leftMenuLinkHeadInActive\">Config Options</a></div>\n");
document.write("<div class=\"paragraphLeftMenu\">Pages</div>\n");
document.write("<div id=\"divID1722\" class=\"leftMenuInActive\"><a id=\"aID1722\" href=\"#\" OnMouseOver=\"link('_page','DocParser0',this)\" class=\"leftMenuLinkInActive\">DocParser</a></div>\n");
document.write("<div id=\"divID1723\" class=\"leftMenuInActive\"><a id=\"aID1723\" href=\"#\" OnMouseOver=\"link('_page','DotOptions0',this)\" class=\"leftMenuLinkInActive\">DotOptions</a></div>\n");
document.write("<div id=\"divID1724\" class=\"leftMenuInActive\"><a id=\"aID1724\" href=\"#\" OnMouseOver=\"link('_page','FormulaLatexOptions0',this)\" class=\"leftMenuLinkInActive\">FormulaLatexOptions</a></div>\n");
document.write("<div id=\"divID1725\" class=\"leftMenuInActive\"><a id=\"aID1725\" href=\"#\" OnMouseOver=\"link('_page','GeneralOptions0',this)\" class=\"leftMenuLinkInActive\">GeneralOptions</a></div>\n");
document.write("<div id=\"divID1726\" class=\"leftMenuInActive\"><a id=\"aID1726\" href=\"#\" OnMouseOver=\"link('_page','HeaderAndFooter0',this)\" class=\"leftMenuLinkInActive\">HeaderAndFooter</a></div>\n");
document.write("<div id=\"divID1727\" class=\"leftMenuInActive\"><a id=\"aID1727\" href=\"#\" OnMouseOver=\"link('_page','IncludeInOutput0',this)\" class=\"leftMenuLinkInActive\">IncludeInOutput</a></div>\n");
document.write("<div id=\"divID1728\" class=\"leftMenuInActive\"><a id=\"aID1728\" href=\"#\" OnMouseOver=\"link('_page','InputOptions0',this)\" class=\"leftMenuLinkInActive\">InputOptions</a></div>\n");
document.write("<div id=\"divID1729\" class=\"leftMenuInActive\"><a id=\"aID1729\" href=\"#\" OnMouseOver=\"link('_page','Inputparsing0',this)\" class=\"leftMenuLinkInActive\">Inputparsing</a></div>\n");
document.write("<div id=\"divID1730\" class=\"leftMenuInActive\"><a id=\"aID1730\" href=\"#\" OnMouseOver=\"link('_page','LanguageOptions0',this)\" class=\"leftMenuLinkInActive\">LanguageOptions</a></div>\n");
document.write("<div id=\"divID1731\" class=\"leftMenuInActive\"><a id=\"aID1731\" href=\"#\" OnMouseOver=\"link('_page','LayoutHtml0',this)\" class=\"leftMenuLinkInActive\">LayoutHtml</a></div>\n");
document.write("<div id=\"divID1732\" class=\"leftMenuInActive\"><a id=\"aID1732\" href=\"#\" OnMouseOver=\"link('_page','LeftMenu0',this)\" class=\"leftMenuLinkInActive\">LeftMenu</a></div>\n");
document.write("<div id=\"divID1733\" class=\"leftMenuInActive\"><a id=\"aID1733\" href=\"#\" OnMouseOver=\"link('_page','MessagesOptions0',this)\" class=\"leftMenuLinkInActive\">MessagesOptions</a></div>\n");
document.write("<div id=\"divID1734\" class=\"leftMenuInActive\"><a id=\"aID1734\" href=\"#\" OnMouseOver=\"link('_page','OutputCustomization0',this)\" class=\"leftMenuLinkInActive\">OutputCustomization</a></div>\n");
document.write("<div id=\"divID1735\" class=\"leftMenuInActive\"><a id=\"aID1735\" href=\"#\" OnMouseOver=\"link('_page','PreprocessorOptions0',this)\" class=\"leftMenuLinkInActive\">PreprocessorOptions</a></div>\n");
document.write("<div id=\"divID1736\" class=\"leftMenuInActive\"><a id=\"aID1736\" href=\"#\" OnMouseOver=\"link('_page','SearchPageOptions0',this)\" class=\"leftMenuLinkInActive\">SearchPageOptions</a></div>\n");
document.write("<div id=\"divID1737\" class=\"leftMenuInActive\"><a id=\"aID1737\" href=\"#\" OnMouseOver=\"link('_page','SpecialOptions0',this)\" class=\"leftMenuLinkInActive\">SpecialOptions</a></div>\n");
if(divID != "" && aID != "")
{
document.getElementById(divID).className = divClassName;
document.getElementById(aID).className = aClassName;
}
}
