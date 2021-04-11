#ifndef _DOXYS_OFF_ON_H_
#define _DOXYS_OFF_ON_H_

/** 
\file DOXYS_OFF_ON.h
!This file shows how to turn DoxyS on and off in the source code.
To turn of DoxyS parsing (text is skipped completely by pre-processor) 
type this on a single line in the source file outside any (other) 
documentation blocks.

!To turn OFF:
||/// DOXYS_OFF


!To turn ON:
||/// DOXYS_ON

When DoxyS is turned off, this state continues until the $/// DOXYS_ON$
statement is encountered or until the end of the file is reached. Ie. there
is an implicit "turn on" statement at the end of a file.

!See the source for this file, to see how it's done (click "Source"-tab).

*/

/// DOXYS_OFF_ON.h This function is }not} skipped by doxygen.
void  FunctionIncluded_1();

/// DOXYS_OFF

/// This function is skipped by doxygen. It will not appear in the output
void  FunctionNotIncluded_1();

/// DOXYS_ON


/// DOXYS_OFF_ON.h This function is }not} skipped by doxygen.
void  FunctionIncluded_2();

///DOXYS_OFF

/// This function is skipped by doxygen. It will not appear in the output
void  FunctionNotIncluded_3();

/// This function is skipped by doxygen. It will not appear in the output
void  FunctionNotIncluded_3();


#endif

