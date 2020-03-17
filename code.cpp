#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <filesystem>
#include "NvTriStrip.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/cimport.h>

using namespace std;

int main(int argc, char *argv[])
{
    string runName;
    vector<string> loadFilenames;
    string FMT = "PNT";
    int defaultFlags = aiProcess_FlipWindingOrder | aiProcess_Triangulate | aiProcess_PreTransformVertices | aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals;
    bool noStrip = false;
    for (int argi = 0; argi < argc; ++argi)
    {
        cout << argv[argi] << endl;
        if (!argi)
            runName = argv[argi];
        else
        {
            if (strcmp(argv[argi], "-cs") == 0 ||
                strcmp(argv[argi], "--cachesize") == 0)
            {
                ++argi;

                int cachesize = 16;
                int num = sscanf(argv[argi], "%d", &cachesize);
                if (num != 1)
                {
                    cerr << "Error reading cache size for -cs argument" << endl;
                    exit(-1);
                }

                //cerr << "Cache Size " << cachesize << endl;
                SetCacheSize(cachesize);
            }
            if (strcmp(argv[argi], "-fm") == 0 ||
                strcmp(argv[argi], "--fromat") == 0)
            {
                ++argi;
                FMT = argv[argi];
            }
            else if (strcmp(argv[argi], "-ns") == 0 ||
                     strcmp(argv[argi], "--nostrip") == 0)
            {
                noStrip = true;
            }
            else
            {
                loadFilenames.emplace_back(argv[argi]);
            }
        }
    }
    Assimp::Importer Importer;
    for (auto &loadFilename : loadFilenames)
    {
        auto pScene = Importer.ReadFile(loadFilename, defaultFlags);
        if (pScene)
        {
            for (unsigned int meshIndex = 0; meshIndex < pScene->mNumMeshes; ++meshIndex)
            {
                auto paiMesh = pScene->mMeshes[meshIndex];
                auto vertexCount = paiMesh->mNumVertices;

                //aiColor3D pColor{};
                //pScene->mMaterials[paiMesh->mMaterialIndex]->Get(AI_MATKEY_COLOR_DIFFUSE, pColor);
                //const aiVector3D Zero3D{};
                std::vector<float> vertexBuffer;

                for (unsigned int vertIndex = 0; vertIndex < paiMesh->mNumVertices; ++vertIndex)
                {
                    for (auto fm : FMT)
                    {
                        auto vertexBufferSize = vertexBuffer.size();
                        switch (fm)
                        {
                        case 'P':
                            if (paiMesh->HasPositions())
                            {
                                const auto &pPos = paiMesh->mVertices[vertIndex];
                                vertexBuffer.push_back(pPos.x);
                                vertexBuffer.push_back(-pPos.y);
                                vertexBuffer.push_back(pPos.z);
                            }
                            break;
                        case 'N':
                            if (paiMesh->HasNormals())
                            {
                                const auto &pNormal = paiMesh->mNormals[vertIndex];
                                vertexBuffer.push_back(pNormal.x);
                                vertexBuffer.push_back(-pNormal.y);
                                vertexBuffer.push_back(pNormal.z);
                            }
                            break;
                        case 'T':
                            if (paiMesh->HasTextureCoords(0))
                            {
                                const auto &pTexCoord = paiMesh->mTextureCoords[0][vertIndex];
                                vertexBuffer.push_back(pTexCoord.x);
                                vertexBuffer.push_back(pTexCoord.y);
                            }
                            break;
                        case 'C':
                            if (paiMesh->HasVertexColors(vertIndex))
                            {
                                const auto &pTangent = paiMesh->mTangents[vertIndex];
                                vertexBuffer.push_back(pTangent.x);
                                vertexBuffer.push_back(pTangent.y);
                                vertexBuffer.push_back(pTangent.z);
                            }
                            break;
                        case 'A':
                            if (paiMesh->HasTangentsAndBitangents())
                            {
                                const auto &pTangent = paiMesh->mTangents[vertIndex];
                                vertexBuffer.push_back(pTangent.x);
                                vertexBuffer.push_back(pTangent.y);
                                vertexBuffer.push_back(pTangent.z);
                            }
                            break;
                        default:
                            cerr << "CANNOT GET:" << fm << endl;
                            return -2;
                        }
                        if (vertexBufferSize == vertexBuffer.size())
                        {
                            cerr << "NO:" << fm << " in model mesh" << endl;
                            return -1;
                        }
                    }
                }
                //save vertexBuffer file in meshIndex dir

                std::vector<uint16_t> indexBuffer;
                for (unsigned int facesIndex = 0; facesIndex < paiMesh->mNumFaces; ++facesIndex)
                {
                    const auto &Face = paiMesh->mFaces[facesIndex];
                }
                //save indexBuffer file in meshIndex dir

                unique_ptr<PrimitiveGroup[]> prims;
                unsigned short numprims;
                {
                    PrimitiveGroup *pprims;
                    bool done = GenerateStrips(indexBuffer.data(), indexBuffer.size(), &pprims, &numprims);
                    prims.reset(pprims);
                }
                //save prims strip files in meshIndex dir
            }
        }
    }

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