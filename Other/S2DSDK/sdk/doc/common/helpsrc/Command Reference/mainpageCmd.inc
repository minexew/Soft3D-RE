!!! Creating a front page (index page) for the project ( mainpage )
The mainpage for your project, is made like any other page except the keyword 
$\\mainpage$ is used instead of $\\page$. There should be only 1 mainpage in a 
project and normally located in the root of one of the input directories, although 
it may be placed anywhere. The mainpage is the content of the $index.html$ file and 
therefore the first page that the user will see.

Actually the mainpage can be viewed as the directory ($\\dir$-tag, dirCmd) page of a 
"virtual" toplevel directory. (one could view it as a $\\maindir$ command perhaps! )
!Syntax
\code
/** \mainpage "Project Name"
Introduction to the project....
\endds_doc \endcode
!Additional directories on mainpage left menu
If you want more than one directory to appear on the left menu of the mainpage 
simply specify more input directories in the config file and thet will all appear
on the left menu. This feature in used for generating this help.

!Additional pages on the left menu of the mainpage
It's possible to add extra pages to the left menu of the mainpage by specifying the 
pages in the config file (MAINPAGE_LEFT_MENU_PAGES) like this:\n
\code
MAINPAGE_LEFT_MENU_PAGES    = MyPage "My Other Page" OneMorePage
\endcode
\sa pageCmd
\note The mainpage does not have a brief description.
