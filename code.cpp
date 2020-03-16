#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include "NvTriStrip.h"

#include <assimp/Importer.hpp> 
#include <assimp/scene.h>     
#include <assimp/postprocess.h>
#include <assimp/cimport.h>

using namespace std;

int main()
{
    vector<unsigned short> indices{1, 2, 3, 2, 3, 4, static_cast<unsigned short>(-1)};
    //Call NvTriStrip to generate the strips
    //PrimitiveGroup *prims;
    unique_ptr<PrimitiveGroup[]> prims;
    unsigned short numprims;
    {
        PrimitiveGroup *pprims;
        bool done = GenerateStrips(indices.data(), indices.size(), &pprims, &numprims);
        prims.reset(pprims);
    }
    cout << "numprims:" << numprims << endl;

    for (unsigned int primidx = 0; primidx < numprims; primidx++)
    {
        PrimitiveGroup &pg = prims[primidx];

        cout << "pg.type:" << pg.type << endl;
        cout << "pg.numIndices:" << pg.numIndices << endl;

        for (unsigned int i = 0; i < pg.numIndices; i++)
        {
            cout << pg.indices[i] << " ";
        }

        cout << endl;
    }

    //cleanup
}