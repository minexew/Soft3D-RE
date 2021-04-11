#ifndef _OVERLOAD_H_
#define _OVERLOAD_H_



/** 
Overloaded functions example output. 

This class is just to show how DoxyS handles overloaded functions. We have chosen
to show class member functions in this example so that it can be seen that DoxyS can 
distinguish between functions declared #const# or not.
Click on the "Public"-tab to see how it looks!
*/
class Overload
{
public:
  /** Brief description of #Overload()# function here. 
      \include overload_raw_include.inc */
  void                Overload();

  /** Brief description of #Overload() const# function here. 
      \include overload_raw_include.inc */
  void                Overload() const;

  /** Brief description of #Overload(int iParam)# function here. 
      \include overload_raw_include.inc */
  void                Overload(int iParam);

  /** Brief description of #Overload(int iParam) const# function here. 
      \include overload_raw_include.inc */
  void                Overload(int iParam) const;

  /** Brief description of #Overload(int iParam, float fParam)# function here. 
      \include overload_raw_include.inc */
  void                Overload(int iParam, float fParam);

  /** Brief description of #Overload(int iParam, float fParam) const# function here. 
      \include overload_raw_include.inc */
  void                Overload(int iParam, float fParam) const;

  /** Brief description of #Overload(int iParam, float fParam, const std::string& sString)# function here. 
      \include overload_raw_include.inc */
  void                Overload(int iParam, float fParam, const std::string& sString);

  /** Brief description of #Overload(int iParam, float fParam, const std::string& sString) const# function here. 
      \include overload_raw_include.inc */
  void                Overload(int iParam, float fParam, const std::string& sString) const;

};




#endif




















