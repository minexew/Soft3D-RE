#ifndef INHERITANCE_CLASSES_H
#define INHERITANCE_CLASSES_H

/** Base class for the other classes in this directory. */
class BaseClass
{
public:

  virtual int           InheritDocFromBaseClass(int iInt);
  virtual void          Override();
  void                  AllInheritedBaseClass();
};




/** Class derived from BaseClass. */
class DerivedLevel_1 : public BaseClass
{
public:

  virtual int           InheritDocFromBaseClass(int iInt);
  virtual void          Override();
  void                  AllInheritedDerivedLevel_1();
};


/** Class derived from DerivedLevel_1. */
class DerivedLevel_2 : public DerivedLevel_1
{
public:

  virtual int           InheritDocFromBaseClass(int iInt);
  virtual void          Override();
  void                  AllInheritedDerivedLevel_2();
};



/** Class Final_1 is derived from DerivedLevel_2. */
class Final_1 : public DerivedLevel_2
{
public:

  virtual int           InheritDocFromBaseClass(int iInt);
  virtual void          Override();
  void                  AllInheritedFinal_1();
};


/** Class Final_2 is derived from DerivedLevel_2. */
class Final_2 : public DerivedLevel_2
{
public:

  virtual int           InheritDocFromBaseClass(int iInt);
  virtual void          Override();
  void                  AllInheritedFinal_2();
};


/// DOXYS_OFF

/*! class B */
class B {};

/*! class C */
class C {};

/*! 
\page TestDot

Class relations expressed via an inline dot graph:
\dot
digraph example {
    node [shape=record, fontname=Helvetica, fontsize=10];
    b [ label="class B" URL="\ref B"];
    c [ label="class C" URL="\ref C"];
    b -> c [ arrowhead="open", style="dashed" ];
}
\enddot
Note that the classes in the above graph are clickable 
(in the HTML output).
*/


#endif
