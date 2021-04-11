#include "inheritance_classes.h"

// -----------------
// --- BaseClass ---
// -----------------

/** 
Shows how documentation are inherited down hierarchy. This function
is overridden i all the classes in the hierarchy, but the documentation 
is only written here in the base class. The other versions of this function 
are not documented. 
\note This function (as well as Override) shows how overridden functions are 
      displayed in DoxyS output.
\return Some integer is returned. */
int BaseClass::InheritDocFromBaseClass(int iInt   ///< Parameter iInt description.
                                              )
{
  cout<<"BaseClass::InheritDocFromBaseClass(int iInt)";
  return 0;
}

/** 
Shows how overridden functions are displayed (BaseClass). This function is overridden in 
all the classes in the hierarchy. This function (as well as Override) shows 
how overridden functions are  displayed in DoxyS output.*/
BaseClass::Override()
{
  cout<<"BaseClass::Override()";
}

/// "Show all inherited" demo from BaseClass.
/// Demonstrates how the #Show all inherited# icon can be used to show
/// all public and protected functions that a class has including the ones 
/// inherited from other classes in the hierarchy. 
BaseClass::AllInheritedBaseClass()
{
  cout<<"BaseClass::AllInheritedBaseClass()";
}


// ----------------------
// --- DerivedLevel_1 ---
// ----------------------

int DerivedLevel_1::InheritDocFromBaseClass(int iInt)
{
  cout<<"DerivedLevel_1::InheritDocFromBaseClass(int iInt)";
  return 1;
}

/** 
Shows how overridden functions are displayed (DerivedLevel_1). This function 
overrides the one from BaseClass ( ie. BaseClass::Override ).*/
DerivedLevel_1::Override()
{
  cout<<"DerivedLevel_1::Override()";
}

/// "Show all inherited" demo from DerivedLevel_1.
/// Demonstrates how the #Show all inherited# icon can be used to show
/// all public and protected functions that a class has including the ones 
/// inherited from other classes in the hierarchy. 
DerivedLevel_1::AllInheritedDerivedLevel_1()
{
  cout<<"DerivedLevel_1::AllInheritedDerivedLevel_1()";
}



// ----------------------
// --- DerivedLevel_2 ---
// ----------------------

int DerivedLevel_2::InheritDocFromBaseClass(int iInt)
{
  cout<<"DerivedLevel_2::InheritDocFromBaseClass(int iInt)";
  return 2;
}

/** 
Shows how overridden functions are displayed (DerivedLevel_2). This function 
overrides the one from DerivedLevel_1 ( ie. DerivedLevel_1::Override() ).*/
DerivedLevel_2::Override()
{
  cout<<"DerivedLevel_2::Override()";
}

/// "Show all inherited" demo from DerivedLevel_2.
/// Demonstrates how the #Show all inherited# icon can be used to show
/// all public and protected functions that a class has including the ones 
/// inherited from other classes in the hierarchy. 
DerivedLevel_2::AllInheritedDerivedLevel_2()
{
  cout<<"DerivedLevel_2::AllInheritedDerivedLevel_2()";
}


// ---------------
// --- Final_1 ---
// ---------------

int Final_1::InheritDocFromBaseClass(int iInt)
{
  cout<<"Final_1::InheritDocFromBaseClass(int iInt)";
  return 3;
}

/** 
Shows how overridden functions are displayed (Final_1). This function 
overrides the one from DerivedLevel_2 ( ie. DerivedLevel_2::Override() ).*/
Final_1::Override()
{
  cout<<"Final_1::Override()";
}

/// "Show all inherited" demo from Final_1.
/// Demonstrates how the #Show all inherited# icon can be used to show
/// all public and protected functions that a class has including the ones 
/// inherited from other classes in the hierarchy. 
Final_1::AllInheritedFinal_1()
{
  cout<<"Final_1::AllInheritedFinal_1()";
}


// ---------------
// --- Final_2 ---
// ---------------

int Final_2::InheritDocFromBaseClass(int iInt)
{
  cout<<"Final_2::InheritDocFromBaseClass(int iInt)";
  return 4;
}

/** 
Shows how overridden functions are displayed (Final_2). This function 
overrides the one from DerivedLevel_2 ( ie. DerivedLevel_2::Override() ).*/
Final_2::Override()
{
  cout<<"Final_2::Override()";
}

/// "Show all inherited" demo from Final_2.
/// Demonstrates how the #Show all inherited# icon can be used to show
/// all public and protected functions that a class has including the ones 
/// inherited from other classes in the hierarchy. 
Final_2::AllInheritedFinal_2()
{
  cout<<"Final_2::AllInheritedFinal_2()";
}



#endif
