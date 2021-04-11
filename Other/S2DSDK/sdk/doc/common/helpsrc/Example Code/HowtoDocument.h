#ifndef _HOWTO_DOCUMENT_H_
#define _HOWTO_DOCUMENT_H_

#include <stdio.h>

/** HowTo document your code. This class serves as an example on how to write
documentation using DoxyS. For those familiar with DoxyS "ancestor" }Doxygen} by
Dimitri Van Heesch most of this will not be new. 

The basic idea with this HowToDocument class is that different functions (and a couple)
of member variables demonstrate the most common ways of documenting code using DoxyS. 
The functions are given meaning full names like: Parameters. Most of the 
functions are found on the #"Public"-tab# for this class.

!For the impatient 
These are the functions/pages that cover the most basic ways of documenting:
- BriefAndDescription
- Parameters
- ReturnValue
- MainPage

!Source is located here (click on the "Source"-tab) 
- HowtoDocument.h
- HowtoDocument.cpp

!Code documentation block
Code documentation blocks are designated by using special style comment blocks
like shown here:



-# #Normal multiline# 
  \code 
  /** 
  Brief description.
  Main documentation / description. 
  \endds_doc \endcode
-# #Original Doxygen multiline# 
  \code 
  /*! 
  Brief description.
  Main documentation / description. 
  \endds_doc \endcode
-# #C++ single line style# 
  \code 
  /// Brief description.
  /// Main documentation / description. 
  \endcode


!IMPORTANT - REMEMBER
Functions can just as easily be documented in the cpp-file, which is generally
recommended. This keeps the header file compact (which many people prefers) and allows
you to modify the documentation without having to recompile more than just a single cpp
file. 

\todo We do have stuff to do! Sections like \\todo, \\note, \\sa, \\version
      \\author etc. can expand more lines. The section ends at a blank line
      or if a new \\todo, \\note etc. section is encountered.
\note This a note
\todo We do have more stuff to do!
\sa BriefAndDescription, ReturnValue.
\version 1.0
\author N.F.S Grundtvig (The members of this class inherits the author by default.)

Possible to continue general description after \\note, \\todo etc. In fact you 
can interchange them as you like. Just remember (like done here) to make a blank 
line after the last \\todo etc. section (in this case the last is \\author).
*/
class HowToDocument
{

public:

  void                BriefAndDescription();
  void                BriefToNewline();
  void                BriefCmd();

  void                Parameters( int iSize, HowToDocument& howToDocument, float fMAX2 = 2.1f );
  void                ParametersJavaStyle( int iSize, HowToDocument& howToDocument, float fMAX2 = 2.1f);

  /** 
  Template parameter documentation.
  Template parameters }must} be documented using the Java-style $\\param$ keyword. 
  This might however change in a future version of DoxyS allowing for the standard way
  also. This convention also applies when documenting class template parameters.
  \param T Description of a template parameter.
  \param COUNT One more template parameter 

  !This is how it looks in the source:
  \code
  \param T Description of a template parameter.
  \param COUNT One more template parameter 
  \endcode
  \sa Parameters, ParametersJavaStyle
  */
  template <class T, int COUNT >  
  void                ParametersTemplates()
  {}

  void                ExceptionsThrown();


  /** 
  Combining the ways to doc parameters for the template.
  \param T Description of a template parameter. #Remember:# Template parameters
  {must{ be described using $\\param$ keyword.
  \param COUNT One more template parameter
  \return This is the return doc. 

  !This is how it looks in the source:
  \code
  \param T Description of a template parameter. #Remember:# Template parameters
  {must{ be described using $\\param$ keyword.
  \param COUNT One more template parameter
  \return This is the return doc. 
  template <class T, int COUNT = 5 >  
  int                 ParameterDocCombined( int iSize,                    ///< Parameter documentation for iSize
                                            HowToDocument& howToDocument, ///< Parameter documentation for TestDocBent
                                            float fMAX2 = 2.1f            ///< Parameter documentation for fMAX2
                                           )
  {}

  \endcode
  */
  template <class T, int COUNT >  
  int                 ParameterDocCombined( int iSize,                    ///< Parameter documentation for iSize
                                            HowToDocument& howToDocument, ///< Parameter documentation for TestDocBent
                                            float fMAX2 = 2.1f            ///< Parameter documentation for fMAX2
                                           )
  {}

  void                IncludeDocFromBody();
  void                OrgDoxygenDocComment();
  void                CppDocCommentStyle();



  void                ExampleInclude();
  void                ExampleInline();
  void                InlineFormattedCode();
  void                Author();
  void                Bug();
  void                Deprecated();
  void                Internal();
  void                Note();
  void                Remark();
  int                 ReturnValue() const;
  void                SeeAlso();
  void                Todo();
  void                Version();
  void                Since();
  void                Warning();



  friend const bool	    operator ==(const HowToDocument& lhs, const HowToDocument& rhs );

	/** 
  Enumeration documentation. This must usually be done in the header file.
  The enumeration will appear on the overview list ("Public", "Protected, "Private 
  tabs ) for classes, on directory "Globals" (if not part of a class) etc. The 
  individual enum values, are shown in a table on the doc-page for the enumeration 
  as is shown above.

  !Anonymous enumerations
  Anonymous enumerations are not shown on any of the above lists. However the enum values
  are instead shown on all of these lists. This is quite useful if for instance one uses
  anonymous enum values as integer constants in classes.

  !This is how it looks in the source:
  \code
  /** 
  Enumeration documentation. This must usually be done in the header file.
	enum Enum      { eEnumVal1,    ///< eEnumVal1 enum #value# description.
                   eEnumVal2,    ///< eEnumVal2 enum #value# description.
                   eEnumVal3     ///< eEnumVal3 enum #value# description
  };
  \endds_doc \endcode
  */
	enum Enum	{ eEnumVal1,    ///< eEnumVal1 enum #value# description.
              eEnumVal2,    ///< eEnumVal2 enum #value# description.
              eEnumVal3     ///< eEnumVal3 enum #value# description
  };


	enum 			{ eAnonEnumVal1,    ///< eAnonEnumVal1 anonymous enum #value# description.
              eAnonEnumVal2,    ///< eAnonEnumVal2 anonymous enum #value# description.
              eAnonEnumVal3     ///< eAnonEnumVal3 anonymous enum #value# description
  };



private:
  int                 m_iMemberVarShortDoc;   ///< Short single line documentation. Using #///<# - style after the declaration on same line (All is brief)
  int                 m_iMemberVarLongDoc;    /**< Longer single line documentation. Using $#/**< ... \endds_doc#$ - style. Here brief is to punctuation as normal.*/

  /** Documenting member before the declaration as normal. Here we place the
      documentation for a member before the declaration (and not after it on the 
      same line using #///<# ) as we normallly would do since these descriptions 
      rarely would take more than a single line.

      !This is how it looks in the source:
      \code
      /** 
      Documenting member before the declaration as normal. Here we place the ...
      \endds_doc 
      int                 m_iMemberVarLongStandardDoc; 
      \endcode
      */
  int                 m_iMemberVarLongStandardDoc; 
};


void RelatedFunction();

/** 
Documenting parameters for macro. use the $\\param$ keyword.
!This is how it looks in the code:
\code
/**
Documenting parameters for macro. use the $\\param$ keyword.
\param x MACRO_PARAMETERS Param x description
\param y MACRO_PARAMETERS Param y description
\return  MACRO_PARAMETERS return value description
\endds_doc \endcode
\param x MACRO_PARAMETERS Param x description
\param y MACRO_PARAMETERS Param y description
\return  MACRO_PARAMETERS return value description
\sa ParametersJavaStyle
*/
#define MACRO_PARAMETERS(x,y) { printf("(%d, %d)\n", x, y); }

#endif




















