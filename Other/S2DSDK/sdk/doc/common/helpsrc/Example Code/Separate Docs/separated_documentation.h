#ifndef _SEPARATED_DOCUMENTATION_H_
#define _SEPARATED_DOCUMENTATION_H_


#define DEFINE    12

#define MACRO(x,y) { printf("(%d, %d)\n", x, y); }


typedef int TypeDef;

int     FreeFunction(int iInt);




namespace SepDocNamespace
{
  void  NamespaceFunction(int iInt);

  class NamespaceClass
  {
    public:
      void  ClassFunction(int iInt) const;
  };

}

 
class SepDocClass
{
  public:
    void  ClassFunction(int iInt) const;

    template <class T1, class T2>
    void  ClassTemplateFunction(int iInt)
    {
      cout<<"ClassTemplateFunction(int)\n";
    }

    int     m_iClassVariable;

    enum    ClassEnum { eClassEnumVal1, eClassEnumVal2 };
    
    union ClassUnion
    {
      int     m_iClassUnionIntMember;
      float   m_iClassUnionFloatMember;
    };


};



#endif

