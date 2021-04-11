#ifndef _OVERLOAD_H_
#define _OVERLOAD_H_



/** 
Template (specializations) example output. 
This class along with it's specializations is just intended for showing how 
DoxyS displays templates in the output. Key features are:

- Only the main (general) template is shown on overview tables and left menu.
- General templates' "Description"-tab has table listing all partial and full template specializations.
- The "Description"-tab of specializations has a link back to the general template.
- Works for class templates as well as function templates.
- Parameter descriptions from the general template are "inherited" so to speak to the
  (partial) specializations in case the specialization does not "re-document" them.

\param T1 Description of T1 from general template. Doc }inherited} to partial specializations if not "re-documented" there.
\param T2 Description of T2 from general template. Doc }inherited} to partial specializations if not "re-documented" there.
\param INT Description of INT from general template. Doc }inherited} to partial specializations if not "re-documented" there.
*/

template <class T1, class T2, int INT>
class Template
{
public:

};


/** Partial specialization of Template (T1 = float, T2 = double).
 */
template <int INT>
class Template<float, double, INT>
{
public:

};


/** Partial specialization of Template (INT parameter set to 5).
\param T2 Description of T2 from (partial) specialization.
*/
template <class T1, class T2>
class Template<T1, T2, 5>
{
public:

};


/** Full specialization of Template (T1 = float, T2 = double, INT = 7).
 */
template <>
class Template<float, double, 7>
{
public:

};


#endif




















