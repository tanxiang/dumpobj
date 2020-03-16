#include <iostream>
#include <vector>
#include <string>
#include "NvTriStrip.h"

using namespace std;

int main()
{
   vector<string> msg {"Hello", "C++", "World", "from", "VS Code", "and the C++ extension!"};

   for (auto& word : msg)
   {
      cout << word << " ";
   }
   cout << endl;

vector<unsigned short> indices;
    //Call NvTriStrip to generate the strips
    PrimitiveGroup *prims;
    unsigned short numprims;
    bool done = GenerateStrips(&indices[0], indices.size(), &prims, &numprims);
}