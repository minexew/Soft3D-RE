#include "HowtoDocument.h"


/** 
The brief description ends at first punctuation mark (or see BriefToNewline). 
The description continues here with what you find the users of the documented 
entity might want to know. All fomatting like: #bold#, }italics}, Headlines
lists, tables, images, examples etc. work here.

All shown here applies to all kinds of 
entities (functions, macros, variables, namespaces etc.) not just functions and 
classes. 

!Brief on multiple lines
Although it's not recommended to let the brief description expand multiple lines it 
is possible using the #\\brief# command see: BriefCmd().

!This is how it looks in the source:
\code
/** 
The brief description ends at first punctuation mark (or see BriefToNewline ). 
The description continues here with what you find the users of the documented 
entity might want to know. All fomatting like: #bold#, }italics}, Headlines
lists, tables, images, examples etc. work here. ...
\endds_doc \endcode
*/
void  HowToDocument::BriefAndDescription()
{
  printf("BriefAndDescription");
}


/** 
The brief description ends at first newline 
if no punctuation mark is encountered. 

!This is how it looks in the source:
\code
/** 
The brief description ends at first newline 
if no punctuation mark is encountered. 
\endds_doc \endcode
*/
void  HowToDocument::BriefToNewline()
{
  printf("BriefToNewline");
}

/** 
\brief This is a brief description which spans several lines
of text. It continues until a blank line or until a new section command is encountered

The description then continues here... 

!This is how it looks in the source:
\code
/** 
\brief This is a brief description which spans several lines 
of text and continues until a blank line or untill a new section command is encountered

The description then continues here...
\endds_doc \endcode
\include section_commands.inc
\note This is mainly provided for compatibility with Doxygen or in the rare cases where 
      you for some reason }needs} to span the brief over several lines.
*/
void  HowToDocument::BriefCmd()
{
  printf("BriefCmd");
}


/** 
Parameter documentation - normal style in CPP file.
This function do have an implementation so we recommend clicking
on the Source-tab in your browser to view the code.

!This is how it looks in the source:
\code
void  HowToDocument::Parameters( int iSize,                     ///< Parameter documentation for iSize
                                 HowToDocument& howToDocument,  ///< Parameter documentation for howToDocument
                                 float fMAX2)                   ///< Parameter documentation for fMAX2
\endcode
\sa ParametersJavaStyle, ParametersTemplates
*/
void  HowToDocument::Parameters( int iSize,                     ///< Parameter documentation for iSize
                                 HowToDocument& howToDocument,  ///< Parameter documentation for howToDocument
                                 float fMAX2)                   ///< Parameter documentation for fMAX2
{ // Here we don't need to put the ending parenthesis after 2.1f on a newline, 
  // since the statement doesn't end until the ending '}'
  float fFloat = iSize * 3 + fMAX2;
}

/** Parameter documentation - JavaDoc style.
    Here we can write all 3 parameters on the same line in the declaration, but
    the "price" is that we have to repeat the name of the parameter.
    \param iSize Parameter documentation for iSize
    \param howToDocument Parameter documentation for howToDocument
    \param fMAX2 Parameter documentation for fMAX2 
    \sa ParameterDocCombined() $bold$ 
    
    !This is how it looks in the source:
    \code
    \param iSize Parameter documentation for iSize
    \param howToDocument Parameter documentation for howToDocument
    \param fMAX2 Parameter documentation for fMAX2 
    \endcode
    \sa ParametersTemplates, Parameters
    */
void HowToDocument::ParametersJavaStyle( int iSize, HowToDocument& howToDocument, float fMAX2)
{
  printf("ParametersJavaStyle");
}




/**
Documenting exceptions thrown by a function.

This is done using the keyword: #$\\throw$#.
\throw ExceptionName Description of exception that can be thrown.

!This is how it looks in the source:
\code
\throw ExceptionName Description of exception that can be thrown.
\endcode

\note To provide compatibility with original Doxygen the following aliases for the
      \\throw command also works: #$\\throws$#, #$\\exception$# and #$\\exceptions$#
*/
void HowToDocument::ExceptionsThrown()
{
  if ( true )
  {
    throw 12;
  }
}


/** 
Include documentation from function body. Here we show how documentation 
can be included also from the body of a function. Please "click" on the
"Source"-tab in your browser to view the code and see how this is done!

!This is how it looks in the source:
\code
void  HowToDocument::IncludeDocFromBody() 
{ // Here we don't need to put the ending parenthesis after 2.1f on a newline, 
  // since the statement doesn't end until the ending '}'
  float fFloat = iSize * 3 + fMAX2;
  /// !Inlined documentation from function body.
  /// You can add documentation from within the function body this way.
  /// It is just concatenated to the normal documentation block.\n
  /// This only works if you have set #HIDE_IN_BODY_DOCS# to #NO# in the config file (default).\n
  /// #}As of right now it does not work with \\note \\todo etc., but that's on our todo-list :-)#}
}
\endcode
*/
void  HowToDocument::IncludeDocFromBody() 
{ // Here we don't need to put the ending parenthesis after 2.1f on a newline, 
  // since the statement doesn't end until the ending '}'
  int   iSize   = 4;
  float fMAX2   = 120.0f;
  float fFloat  = iSize * 3 + fMAX2;
  /// !Inlined documentation from function body.
  /// You can add documentation from within the function body this way.
  /// It is just concatenated to the normal documentation block.\n
  /// This only works if you have set #HIDE_IN_BODY_DOCS# to #NO# in the config file (default).\n
  /// #}As of right now it does not work with \\note \\todo etc., but that's on our todo-list :-)#}
}


/*! 
Using original doxygen style doc-comments. Using #$\d_doc$# and #$\endds_doc$# 
to mark for documentation. 

!This is how it looks in the source:
\code
/*! 
Using original doxygen style doc-comments.
\endds_doc \endcode
*/
void HowToDocument::OrgDoxygenDocComment()
{
  printf("OrgDoxygenDocComment()");
}




/// Document using multiple lines preceeded with ///.
/// To document this function. The C++ style single line comments (use: #///# ) 
/// are joined to one single block. Works everywhere not just for functions.
/// 
/// !This is how it looks in the source:
/// \code
/// /// Document using multiple lines preceeded with ///.
/// /// To document this function. The C++ style single line comments (use: #///# ) 
/// /// are joined to one single block. Works everywhere not just for functions.
/// \endcode
void HowToDocument::CppDocCommentStyle()
{
  printf("CppDocCommentStyle");
}


/** 
How to include an example. 

Use: $\\example relative_path_to_file_name$. Alternatively if the file is
not found relative to the current directory, then the path is considered to
be relative to the example path set in DoxySFile (EXAMPLE_PATH). 
That is, the example is looked for here: \n
$EXAMPLE_PATH/relative_path_to_file_name$

!Inline formatted code in place
Examples are }always} put into the "example" section, which is at the end of 
the generated documentation page for the entity. In case you want to inline
formatted code directly in in the documentation refer to InlineFormattedCode.

\example examples/myexample.cpp

\note This works different from the original Doxygen.
\note It's possible to include as many examples as possible.
\note Examples also works in page and directory documentation.
\sa ExampleInline(), InlineFormattedCode().

!This is how it looks in the source:
\code
\example examples/myexample.cpp
\endcode
*/
void HowToDocument::ExampleInclude()
{
  printf("ExampleInclude");
}



/** 
How to write an example (ie not including from file). 

Use: $\\example \\endexample$. 

!Inline formatted code in place
Examples are }always} put into the "example" section, which is at the end of 
the generated documentation page for the entity. In case you want to inline
formatted code directly in in the documentation refer to InlineFormattedCode.


\example 
void exampleFunction()
{
  cout<<"This is an inlined example!\n";
}
\endexample

\note It's possible to make as many examples as possible.
\note Examples also works in page and directory documentation.
\sa ExampleInclude(), InlineFormattedCode().

!This is how it looks in the source:
\code
\example 
void exampleFunction()
{
  cout<<"This is an inlined example!\n";
}
\endexample
\endcode
*/
void HowToDocument::ExampleInline()
{
  printf("ExampleInline");
}



/** 
How to inline formatted code in the documentation. 

Use: $\\code \\endcode$. 

!Inline formatted code in place
Examples are }always} put into the "example" section, which is at the end of 
the generated documentation page for the entity. When using \\code \\endcode 
statements you can inline code where you want.

!For example like this:
\code 
void inlinedCode()
{
  cout<<"This is inlined code ie. not and example!\n";
  cout<<"Se also!\n";
  HowToDocument::ExampleInline();
  HowToDocument::ExampleInclude();
}
\endcode

Then you can continue writing documentation here...

\sa ExampleInclude(), ExampleInline().
\note Use $\\pre \\endpre$ to output raw un-formatted text, or the $\\verbatim
      \\endverbatim$, which is the commands used in the original Doxygen. 

!This is how it looks in the source:
||\pre
\code
void inlinedCode()
{
  cout<<"This is inlined code ie. not and example!\n";
  cout<<"Se also!\n";
  HowToDocument::ExampleInline();
  HowToDocument::ExampleInclude();
}
\endcode
\endpre
*/
void HowToDocument::InlineFormattedCode()
{
  printf("InlineFormattedCode");
}


/**  
Example of function with different author than of the containing class. 
\author Bjarne Stroustrup (This "overrides" the default class' author). 

!This is how it looks in the source:
\code
\author Bjarne Stroustrup (This "overrides" the default class' author). 
\endcode
*/
void HowToDocument::Author()
{
  printf("Author");
}


/**
Documenting a bug.

This is done using the keyword: #$\\bug$#.
\bug This is the description of the bug.

!This is how it looks in the source:
\code
\bug This is the description of the bug.
\endcode
\include section_commands.inc
*/
void HowToDocument::Bug()
{
  printf("Bug");
}


/** 
Example of deprecated function. 
\deprecated This function is deprecated use NonDeprecatedFunction instead. 

!This is how it looks in the source:
\code
\deprecated This function is deprecated use NonDeprecatedFunction instead. 
\endcode
\include section_commands.inc
*/
void HowToDocument::Deprecated()
{
  printf("Deprecated");
}



/**
Making internal documentation.

This is done using the keyword: #$\\internal$#.

Internal comments are intended for use by the developers of the code/library and not 
for the users of the library. You can use INTERNAL_DOCS in the config file to show or 
hide the internal documentation.

\internal This is an internal comment.

!This is how it looks in the source:
\code
\internal This is an internal comment.
\endcode
\include section_commands.inc
\sa Todo (\\todo)
*/
void HowToDocument::Internal()
{
  printf("Internal");
}

/**
Making notes to the reader of the documentation.

This is done using the keyword: #$\\note$#.
\note This is a note. Note this!

!This is how it looks in the source:
\code
\note This is a note. Note this!
\endcode
\include section_commands.inc
*/
void HowToDocument::Note()
{
  printf("Note");
}

/**
Making remarks to the reader of the documentation.

This is done using the keyword: #$\\remark$#.
\remark This is a remark. Remarkable :-)

!This is how it looks in the source:
\code
\remark This is a remark. Remarkable :-)
\endcode
\include section_commands.inc
*/
void HowToDocument::Remark()
{
  printf("Remark");
}

/** 
Documenting return values.
\return This is the return value documentation. It can be as long as
you may wish, like other standard sections.

!This is how it looks in the source:
\code
\return This is the return value documentation. It can be as long as
you may wish, like other standard sections.
\endcode
\include section_commands.inc
\note To provide compatibility with original Doxygen the following aliases for the
      \\return command also works: #$\\returns$#, #$\\retval$#
*/
int HowToDocument::ReturnValue() const
{
  printf("ReturnValue");
  return 7;
}


/**
Making "See Also" sections.

This is done using the keyword: #$\\sa$#.

\sa Note (that's a great command)!

!This is how it looks in the source:
\code
\sa Note (that's a great command)!
\endcode
\include section_commands.inc
\note To provide compatibility with original Doxygen the following alias for the
      \\sa command also works: #$\\see$#
*/
void HowToDocument::SeeAlso()
{
  printf("SeeAlso");
}

/**
Making todo's (internal use mainly).

This is done using the keyword: #$\\todo$#.

Todo comments are intended for use by the developers of the code/library and not 
for the users of the library. You can use TODO_DOCS in the config file to show or 
hide the todo documentation.

\todo We have stuff todo!

!This is how it looks in the source:
\code
\todo We have stuff todo!
\endcode
\include section_commands.inc
\sa Internal (\\internal)
*/
void HowToDocument::Todo()
{
  printf("Todo");
}

/**
Designating a version.

This is done using the keyword: #$\\version$#.

\version 0.86

No special formatting is required ans any text will do.
It's possible to use multiple version tags, in which case the version 
strings used are listed comma separated.
!This is how it looks in the source:
\code
\version 0.86
\version 1.06 
\endcode
\include section_commands.inc
*/
void HowToDocument::Version()
{
  printf("version");
}

/**
Information on when this entity was added to the API.

This is done using the keyword: #$\\since$#.

\since In the API since version 0.86

No special formatting is required ans any text will do.
!This is how it looks in the source:
\code
\since In the API since version 0.86
\endcode
\include section_commands.inc
*/
void HowToDocument::Since()
{
  printf("since");
}


/**
Making warning sections.

This is done using the keyword: #$\\warning$#.

\warning This is a warning, be careful!

!This is how it looks in the source:
\code
\warning This is a warning, be careful!
\endcode
\include section_commands.inc
\sa Note (\\note)
*/
void HowToDocument::Warning()
{
  printf("Warning");
}


/// Friend functions goes on the related tab in the HTML output.
const bool	operator ==( const HowToDocument& lhs,		///< (in) Left operand.
                         const HowToDocument& rhs  	///< (in) Right operand.
                       )
{
  printf("operator==");
  return false;
}


/** Related function when declared in class' header-file.
    This function is declared in the same header file as HowToDocument and therefore 
    it's related to this class. 
    \sa MEMBER_SAME_FILE_RELATE_TO_CLASSES, MEMBER_SAME_FILE_RELATE_TO_STRUCTS options.*/
void RelatedFunction()
{
  printf("RelatedFunction()");
}
