function WriteLeftMenu(divID, aID, divClassName, aClassName)
{
document.write("<div id=\"divID1610\" class=\"headerLeftMenuInActive\"><a id=\"aID1610\" href=\"#\" OnMouseOver=\"link('_statsindex','index',this)\" class=\"leftMenuLinkHeadInActive\">Statistics</a></div>\n");
document.write("<div class=\"paragraphLeftMenu\">Pages</div>\n");
document.write("<div id=\"divID1795\" class=\"leftMenuInActive\"><a id=\"aID1795\" href=\"#\" OnMouseOver=\"link('_statsgeneral','stats_general',this)\" class=\"leftMenuLinkInActive\">General</a></div>\n");
document.write("<div id=\"divID1796\" class=\"leftMenuInActive\"><a id=\"aID1796\" href=\"#\" OnMouseOver=\"link('_statsundoc','stats_undoc',this)\" class=\"leftMenuLinkInActive\">Undocumented</a></div>\n");
document.write("<div id=\"divID1797\" class=\"leftMenuInActive\"><a id=\"aID1797\" href=\"#\" OnMouseOver=\"link('_statsmsg','stats_msg_msg',this)\" class=\"leftMenuLinkInActive\">Msg-Messages</a></div>\n");
document.write("<div id=\"divID1798\" class=\"leftMenuInActive\"><a id=\"aID1798\" href=\"#\" OnMouseOver=\"link('_statsmsg','stats_msg_warn',this)\" class=\"leftMenuLinkInActive\">Msg-Warnings</a></div>\n");
document.write("<div id=\"divID1799\" class=\"leftMenuInActive\"><a id=\"aID1799\" href=\"#\" OnMouseOver=\"link('_statsmsg','stats_msg_err',this)\" class=\"leftMenuLinkInActive\">Msg-Errors</a></div>\n");
document.write("<div id=\"divID1800\" class=\"leftMenuInActive\"><a id=\"aID1800\" href=\"#\" OnMouseOver=\"link('_statsmsg','stats_msg_dbg',this)\" class=\"leftMenuLinkInActive\">Msg-Debug</a></div>\n");
if(divID != "" && aID != "")
{
document.getElementById(divID).className = divClassName;
document.getElementById(aID).className = aClassName;
}
}
