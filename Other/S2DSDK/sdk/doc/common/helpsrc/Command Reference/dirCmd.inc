!!! Documenting directories ( dir )
The directory file (usually named #$DirName.dir$#) is intended for an overview
and/or introduction to what is found in the given directory. So the basic idea 
is that every directory/subdirectori must have a $.dir$ file which can be viewed 
as a sort of intro-page for that given directory.

}Documenting directories is a central part of the DoxyS documentation philosophy.}

!Syntax
\code
/**
\dir 
Normal page-like documentation goes here ...
\endds_doc
\endcode


Directories (and subdirectories with pages and directory descriptions 
(\\dir-tag) together form an easy way of making organized online documentation for just 
about anything (not just code). In fact all the HELP pages in DoxyS is made by a 
combination of directories and pages (most of our homepage is made the same way). This 
can be useful for any kind of (fx. technical documentation) as it is written in plain 
textfiles and thus can be stored effectively in version control systems like CVS allowing 
several people to work together on the same documentation. 
