function WriteLeftMenu(divID, aID, divClassName, aClassName)
{
document.write("<div id=\"divID181\" class=\"headerLeftMenuInActive\"><a id=\"aID181\" href=\"#\" OnMouseOver=\"link('_statsindex','index',this)\" class=\"leftMenuLinkHeadInActive\">Statistics</a></div>\n");
document.write("<div class=\"paragraphLeftMenu\">Pages</div>\n");
document.write("<div id=\"divID1510\" class=\"leftMenuInActive\"><a id=\"aID1510\" href=\"#\" OnMouseOver=\"link('_statsgeneral','stats_general',this)\" class=\"leftMenuLinkInActive\">General</a></div>\n");
document.write("<div id=\"divID1511\" class=\"leftMenuInActive\"><a id=\"aID1511\" href=\"#\" OnMouseOver=\"link('_statsundoc','stats_undoc',this)\" class=\"leftMenuLinkInActive\">Undocumented</a></div>\n");
document.write("<div id=\"divID1512\" class=\"leftMenuInActive\"><a id=\"aID1512\" href=\"#\" OnMouseOver=\"link('_statsmsg','stats_msg_msg',this)\" class=\"leftMenuLinkInActive\">Msg-Messages</a></div>\n");
document.write("<div id=\"divID1513\" class=\"leftMenuInActive\"><a id=\"aID1513\" href=\"#\" OnMouseOver=\"link('_statsmsg','stats_msg_warn',this)\" class=\"leftMenuLinkInActive\">Msg-Warnings</a></div>\n");
document.write("<div id=\"divID1514\" class=\"leftMenuInActive\"><a id=\"aID1514\" href=\"#\" OnMouseOver=\"link('_statsmsg','stats_msg_err',this)\" class=\"leftMenuLinkInActive\">Msg-Errors</a></div>\n");
document.write("<div id=\"divID1515\" class=\"leftMenuInActive\"><a id=\"aID1515\" href=\"#\" OnMouseOver=\"link('_statsmsg','stats_msg_dbg',this)\" class=\"leftMenuLinkInActive\">Msg-Debug</a></div>\n");
if(divID != "" && aID != "")
{
document.getElementById(divID).className = divClassName;
document.getElementById(aID).className = aClassName;
}
}
