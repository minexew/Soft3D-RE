!!! Pages in DoxyS ( page )
Pages in DoxyS are intended to be used for any kind of information related
to the code. For example You can use pages to put addtional documentation or HowTo's 
in a directory containing code. The pages show up on the left menu like classes and 
directories. 

!Syntax
\code
/**
\page PageName
page content goes here ...
\endds_doc
\endcode

!or:
!Syntax
\code
/**
\page "Page Name With Spaces"
page content goes here ...
\endds_doc
\endcode

Directories (and subdirectories with pages and directory descriptions 
(\\dir-tag) together form an easy way of making organized online documentation for just 
about anything (not just code). In fact all the HELP pages in DoxyS is made by a 
combination of directories and pages (most of our homepage is made the same way). This 
can be useful for any kind of (fx. technical documentation) as it is written in plain 
textfiles and thus can be stored effectively in version control systems like CVS allowing 
several people to work together on the same documentation. 

Right now You }must} edit your documentation pages in a standard text editor, but we will be 
making a WYSIWYG editor for our Wiki-like formatting syntax, making it nicer to make 
documentation pages with formatting, headlines, tables, pictures etc. 

! About grouping functionality from original Doxygen
Also pages can be used as a replacement for the grouping functionality that currently
is unsupported by DoxyS. (This is by design because we find it a bit complicated to use 
and pages can accomplish the same thing, by just making a page listing the classes or 
whatever in a table, bulleted list or in a any other way you prefer. DoxyS will make the 
links to the classes etc. for you. It could look something like this:

\code
/**
\page "My Grouped Classes"
!!! Group of related classes
These classes are related even though the are not all in same dir. Here follows 
a table listing the classes (clik on the classes to go the class' description):

|| Name || Comments
| HowToDocument | Only thing that relates these entities is that they are doc-style examples ...
| SepDocClass | Only thing that relates these entities is that they are doc-style examples ...
| ClassInUnit | Only thing that relates these entities is that they are doc-style examples ...
| Page | Only thing that relates these entities is that they are doc-style examples ...
\endds_doc
\endcode

!Yielding the following output:
!!! Group of related classes
These classes are related even though the are not all in same dir. Here follows 
a table listing the classes (clik on the classes to go the class' description):

|| Name || Comments
| HowToDocument | Only thing that relates these entities is that they are doc-style examples ...
| SepDocClass | Only thing that relates these entities is that they are doc-style examples ...
| ClassInUnit | Only thing that relates these entities is that they are doc-style examples ...
| Page | Only thing that relates these entities is that they are doc-style examples ...


(}Some of the above classes are in fact in the same directory, which is perfectly OK. They
, however allready have a place where they are presented togeher, namely on the directory page 
in the directory where they belong. But this is also just a simple example!})
