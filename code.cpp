#include <iostream>
#include <fstream>
#include <algorithm>
#include <vector>
#include <map>
#include <string>
#include <memory>
#include <filesystem>
#include "NvTriStrip.h"
#include "proto/model.pb.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/cimport.h>

using namespace std;
static const int defaultFlags = aiProcess_FlipWindingOrder | aiProcess_Triangulate | aiProcess_PreTransformVertices | aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals;

int main(int argc, char *argv[])
{
    string runName;
    vector<string> loadFilenames;
    string FMT = "PNT";
    //int defaultFlags = aiProcessPreset_TargetRealtime_MaxQuality;
    bool noStrip = false;
    for (int argi = 0; argi < argc; ++argi)
    {
        //cout << argv[argi] << endl;
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
                    return -1;
                }

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
    SetStitchStrips(true);
    //  EnableRestart(65535);
    Assimp::Importer Importer;
    for (auto &loadFilename : loadFilenames)
    {
        auto pScene = Importer.ReadFile(loadFilename, defaultFlags);
        if (!pScene)
        {
            cerr << loadFilename << " error: " << Importer.GetErrorString() << endl;
            return -1;
        }
        else
        {
            for (unsigned int meshIndex = 0; meshIndex < pScene->mNumMeshes; ++meshIndex)
            {
                auto paiMesh = pScene->mMeshes[meshIndex];
                auto vertexCount = paiMesh->mNumVertices;

                //aiColor3D pColor{};
                //pScene->mMaterials[paiMesh->mMaterialIndex]->Get(AI_MATKEY_COLOR_DIFFUSE, pColor);
                //const aiVector3D Zero3D{};
                map<unsigned int, unsigned int> indexMap;
                {
                    vector<float> vertexBuffer;
                    //map<aiVector3D,map<aiVector3D,map<vector<float>,int>>> pPosMap;
                    //multimap<vector<float>,>
                    map<vector<float>, int> vertexMap;
                    for (unsigned int vertIndex = 0; vertIndex < paiMesh->mNumVertices; ++vertIndex)
                    {
                        vector<float> vertexPoint;

                        for (auto fm : FMT)
                        {
                            auto vertexBufferSize = vertexBuffer.size();
                            switch (fm)
                            {
                            case 'P':
                                if (paiMesh->HasPositions())
                                {
                                    const auto &pPos = paiMesh->mVertices[vertIndex];
                                    vertexPoint.push_back(pPos.x);
                                    vertexPoint.push_back(-pPos.y);
                                    vertexPoint.push_back(pPos.z);
                                    //auto [ignore, bInset] =
                                    //pPosMap.try_emplace(pPos, vertIndex);
                                    //if (!bInset)
                                    //pIndexMap.try_emplace(vertIndex, pPosMap[pPos]);
                                }
                                break;
                            case 'N':
                                if (paiMesh->HasNormals())
                                {
                                    const auto &pNormal = paiMesh->mNormals[vertIndex];
                                    vertexPoint.push_back(pNormal.x);
                                    vertexPoint.push_back(-pNormal.y);
                                    vertexPoint.push_back(pNormal.z);
                                }
                                break;
                            case 'T':
                                if (paiMesh->HasTextureCoords(0))
                                {
                                    const auto &pTexCoord = paiMesh->mTextureCoords[0][vertIndex];
                                    vertexPoint.push_back(pTexCoord.x);
                                    vertexPoint.push_back(pTexCoord.y);
                                }
                                break;
                            case 'C':
                                if (paiMesh->HasVertexColors(vertIndex))
                                {
                                    const auto &pTangent = paiMesh->mTangents[vertIndex];
                                    vertexPoint.push_back(pTangent.x);
                                    vertexPoint.push_back(pTangent.y);
                                    vertexPoint.push_back(pTangent.z);
                                }
                                break;
                            case 'A':
                                if (paiMesh->HasTangentsAndBitangents())
                                {
                                    const auto &pTangent = paiMesh->mTangents[vertIndex];
                                    vertexPoint.push_back(pTangent.x);
                                    vertexPoint.push_back(pTangent.y);
                                    vertexPoint.push_back(pTangent.z);
                                }
                                break;
                            default:
                                cerr << "CANNOT GET:" << fm << endl;
                                return -2;
                            }
                            copy(begin(vertexPoint), end(vertexPoint), back_inserter(vertexBuffer));
                            vertexMap.try_emplace(vertexPoint, vertIndex);
                            indexMap.try_emplace(vertIndex, vertexMap[vertexPoint]);
                            if (vertexBufferSize == vertexBuffer.size())
                            {
                                cerr << "NO:" << fm << " in model mesh" << endl;
                                return -1;
                            }
                        }
                    }

                    /*
                    auto iterMap = vertexMap.begin();
                    while (iterMap != vertexMap.end())
                    {
                        auto iterMapNext = iterMap;
                        ++iterMapNext;

                        while (iterMapNext != vertexMap.end() && equal(iterMap->first.begin(), iterMap->first.begin() + 3, iterMapNext->first.begin()))
                        {
                            cout << "merg " << iterMap->second << " to " << iterMapNext->second << endl;
                            ++iterMapNext;
                        }
                        //merg iterMap->iterMapNext
                        iterMap = iterMapNext;
                    }
                    //save vertexBuffer file in meshIndex dir
                    */
                    string path = loadFilename + "_" + to_string(meshIndex) + "_" + FMT + "_vert.bin";
                    cout << path << '\n'
                         << "paiMesh->mNumVertices" << paiMesh->mNumVertices << '\n'
                         << "vertexBuffer.size()" << vertexBuffer.size() << endl;

                    ofstream filebuffer{path, ios::out | ofstream::binary};
                    copy(vertexBuffer.begin(), vertexBuffer.end(), ostreambuf_iterator<char>(filebuffer));
                }
                {
                    std::vector<uint16_t> indexBuffer;
                    for (unsigned int facesIndex = 0; facesIndex < paiMesh->mNumFaces; ++facesIndex)
                    {
                        const auto &Face = paiMesh->mFaces[facesIndex];

                        if (Face.mNumIndices != 3)
                            continue;
                        indexBuffer.push_back(Face.mIndices[0]);
                        indexBuffer.push_back(Face.mIndices[1]);
                        indexBuffer.push_back(Face.mIndices[2]);
                    }

                    { //save indexBuffer file in meshIndex dir
                        string path = loadFilename + "_" + to_string(meshIndex) + "_indx.bin";
                        cout << path << '\n'
                             << "paiMesh->mNumFaces" << paiMesh->mNumFaces << '\n'
                             << "indexBuffer.size()" << indexBuffer.size() << endl;
                        ofstream filebuffer{path, ios::out | ofstream::binary};
                        copy(indexBuffer.begin(), indexBuffer.end(), ostreambuf_iterator<char>(filebuffer));
                    }
                    //cout << "pIndexMap.size()" << pIndexMap.size() << endl;
                    for_each(indexBuffer.begin(), indexBuffer.end(), [&](auto &n) { cout << n << ' '; });
                    cout << endl;
                    for_each(indexBuffer.begin(), indexBuffer.end(), [&](auto &n) { n = indexMap[n]; });
                    for_each(indexBuffer.begin(), indexBuffer.end(), [&](auto &n) { cout << n << ' '; });
                    cout << endl;

                    unique_ptr<PrimitiveGroup[]> prims;
                    unsigned short numprims;
                    {
                        PrimitiveGroup *pprims;
                        bool done = GenerateStrips(indexBuffer.data(), indexBuffer.size(), &pprims, &numprims, true);
                        prims.reset(pprims);
                    }
                    string extIndexFileName;
                    for (unsigned int primidx = 0; primidx < numprims; ++primidx)
                    {
                        PrimitiveGroup &pg = prims[primidx];
                        cout << "pg.type:" << pg.type << endl;
                        cout << "pg.numIndices:" << pg.numIndices << endl;
                        for (int i = 0; i < pg.numIndices; ++i)
                        {
                            cout << pg.indices[i] << ' ';
                        }
                        cout << endl;
                        string path = loadFilename + "_" + to_string(meshIndex) + "_strip_" + to_string(primidx) + "_" + to_string(pg.type) + "_indx.bin";
                        extIndexFileName = path;

                        std::cout << '\n'
                                  << extIndexFileName << std::endl;
                        ofstream filebuffer{path, ios::out | ios::binary};
                        filebuffer.write(reinterpret_cast<const char *>(pg.indices), sizeof(unsigned short) * pg.numIndices);
                    }

                    std::basic_ifstream<uint16_t> infile{extIndexFileName, ios::binary};
                    if (infile)
                    {
                        infile.unsetf(ios::skipws);
                        std::vector<uint16_t> indexExtra{};
                        std::copy(std::istreambuf_iterator<uint16_t>{infile},{},std::back_insert_iterator{indexExtra});
                        std::for_each(indexExtra.begin(), indexExtra.end(),
                                      [&](uint16_t n) { std::cout << n << ' '; });
                        std::cout << '\n'
                                  << extIndexFileName << std::endl;
                    }
                    ptfile::Model mode{};
                    mode.add_indices();
                    //save prims strip files in meshIndex dir
                }
            }
        }
    }
    //cleanup
}