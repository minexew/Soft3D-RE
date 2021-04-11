
#include <iostream>

#include "../HowtoDocument.h"

void MyExampleFunction()
{
  using namespace std;
  cout<<"Hello World\n";

  // Lets try calling some of the functions in the HowToDoc class
  HowToDocument howToDoc1;
  HowToDocument howToDoc2;

  howToDoc1.DocumentingParametersJavaStyle(5, howToDoc2, 3.5f);
  howToDoc1.BriefToNewline();
  howToDoc1.OriginalDoxygenDocCommentStyle();
}