#include <iostream>
#include <array>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <vector>
#include <map>
#include <string>
#include <memory>
#include <filesystem>
#include "NvTriStrip.h"
//#include "proto/model.pb.h"
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
    string FMT = "P", FMTGet{};
    // int defaultFlags = aiProcessPreset_TargetRealtime_MaxQuality;
    bool noStrip = false;
    bool flStrip = true;
    bool saveSlist = false;

    float pointscale = 1.0f;

    for (int argi = 0; argi < argc; ++argi)
    {
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
            else if (strcmp(argv[argi], "-fm") == 0 ||
                     strcmp(argv[argi], "--fromat") == 0)
            {
                ++argi;
                FMT = argv[argi];
            }
            else if (strcmp(argv[argi], "-ps") == 0 ||
                     strcmp(argv[argi], "--pointscale") == 0)
            {
                ++argi;
                pointscale = std::stof(argv[argi]);
            }
            else if (strcmp(argv[argi], "-ns") == 0 ||
                     strcmp(argv[argi], "--nostrip") == 0)
            {
                noStrip = true;
            }
            else if (strcmp(argv[argi], "-fs") == 0 ||
                     strcmp(argv[argi], "--flipstrip") == 0)
            {
                flStrip = true;
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
        // ptfile::Model mode{};
        // mode.set_name(loadFilename);
        string saveDir = loadFilename + ".ext/";
        filesystem::create_directories(saveDir);
        string checkf;
        if (!pScene)
        {
            cerr << loadFilename << " error: " << Importer.GetErrorString() << endl;
            return -1;
        }

        if (pScene->HasLights())
        {
            cout << "mNumLights:" << pScene->mNumLights << endl;
            pScene->mMaterials[0];
        }
        if (pScene->HasMaterials())
        {
            std::vector<float> materialsBuff;
            cout << "NumMaterials:" << pScene->mNumMaterials << endl;
            for (unsigned int materialIndex = 0; materialIndex < pScene->mNumMaterials; ++materialIndex)
            {
                aiColor3D color{};

                pScene->mMaterials[materialIndex]->Get(AI_MATKEY_COLOR_AMBIENT, color);
                materialsBuff.emplace_back(color.r * 0.3);
                materialsBuff.emplace_back(color.g * 0.3);
                materialsBuff.emplace_back(color.b * 0.3);
                pScene->mMaterials[materialIndex]->Get(AI_MATKEY_COLOR_DIFFUSE, color);
                materialsBuff.emplace_back(color.r);
                materialsBuff.emplace_back(color.g);
                materialsBuff.emplace_back(color.b);
                pScene->mMaterials[materialIndex]->Get(AI_MATKEY_COLOR_SPECULAR, color);
                materialsBuff.emplace_back(color.r);
                materialsBuff.emplace_back(color.g);
                materialsBuff.emplace_back(color.b);
                float shininess;
                pScene->mMaterials[materialIndex]->Get(AI_MATKEY_SHININESS, shininess);
                materialsBuff.emplace_back(shininess);
            }
            string path = loadFilename + ".material.bin";
            ofstream filebuffer{path, ios::out | ofstream::binary};
            filebuffer.write(reinterpret_cast<char *>(materialsBuff.data()), materialsBuff.size() * sizeof(decltype(materialsBuff)::value_type));
        }
        vector<uint16_t> materials;
        for (unsigned int meshIndex = 0; meshIndex < pScene->mNumMeshes; ++meshIndex)
        {
            // auto saveMesh = mode.add_meshs();
            std::vector<uint16_t> indexBuffer;
            {
                auto paiMesh = pScene->mMeshes[meshIndex];
                materials.emplace_back(paiMesh->mMaterialIndex);
                cout << "meshIndex" << meshIndex << ":mMaterialIndex" << paiMesh->mMaterialIndex << '\n';

                auto vertexCount = paiMesh->mNumVertices;
                map<vector<float>, int> vertexMap;
                vector<vector<float>> vertexbufferOrg;
                for (unsigned int vertIndex = 0; vertIndex < paiMesh->mNumVertices; ++vertIndex)
                {
                    vector<float> vertexPoint;

                    for (auto fm : FMT)
                    {
                        switch (fm)
                        {
                        case 'P':
                            if (paiMesh->HasPositions())
                            {
                                const auto &pPos = paiMesh->mVertices[vertIndex];
                                vertexPoint.push_back(pPos.x * pointscale);
                                vertexPoint.push_back(pPos.y * pointscale);
                                vertexPoint.push_back(pPos.z * pointscale);
                                if (vertIndex == 0)
                                    FMTGet += fm;
                            }
                            break;
                        case 'N':
                            if (paiMesh->HasNormals())
                            {
                                const auto &pNormal = paiMesh->mNormals[vertIndex];
                                vertexPoint.push_back(pNormal.x);
                                vertexPoint.push_back(pNormal.y);
                                vertexPoint.push_back(pNormal.z);
                                if (vertIndex == 0)
                                    FMTGet += fm;
                            }
                            break;
                        case 'T':
                            if (paiMesh->HasTextureCoords(0))
                            {
                                const auto &pTexCoord = paiMesh->mTextureCoords[0][vertIndex];
                                vertexPoint.push_back(pTexCoord.x);
                                vertexPoint.push_back(pTexCoord.y);
                                if (vertIndex == 0)
                                    FMTGet += fm;
                            }
                            break;
                        case 't':
                            vertexPoint.push_back(0.0);
                            vertexPoint.push_back(0.0);
                            if (vertIndex == 0)
                                FMTGet += fm;
                            break;
                        case 'C':
                            if (paiMesh->HasVertexColors(vertIndex))
                            {
                                const auto &pTangent = paiMesh->mTangents[vertIndex];
                                vertexPoint.push_back(pTangent.x);
                                vertexPoint.push_back(pTangent.y);
                                vertexPoint.push_back(pTangent.z);
                                if (vertIndex == 0)
                                    FMTGet += fm;
                            }
                            break;
                        case 'A':
                            if (paiMesh->HasTangentsAndBitangents())
                            {
                                const auto &pTangent = paiMesh->mTangents[vertIndex];
                                vertexPoint.push_back(pTangent.x);
                                vertexPoint.push_back(pTangent.y);
                                vertexPoint.push_back(pTangent.z);
                                if (vertIndex == 0)
                                    FMTGet += fm;
                            }
                            break;
                        default:
                            cerr << "CANNOT GET:" << fm << endl;
                            return -2;
                        }
                    }

                    vertexbufferOrg.emplace_back(vertexPoint);
                    vertexMap.try_emplace(vertexPoint, vertIndex);
                }

                for (unsigned int facesIndex = 0; facesIndex < paiMesh->mNumFaces; ++facesIndex)
                {
                    const auto &Face = paiMesh->mFaces[facesIndex];

                    if (Face.mNumIndices != 3)
                        continue;
                    indexBuffer.push_back(Face.mIndices[0]);
                    indexBuffer.push_back(Face.mIndices[1]);
                    indexBuffer.push_back(Face.mIndices[2]);
                }

                for_each(indexBuffer.begin(), indexBuffer.end(), [&](auto &n)
                         { n = vertexMap[vertexbufferOrg[n]]; });

                map<int, vector<float>> remapVertexMap;
                vector<float> vertexBuffer;
                map<int, int> indexMap;
                for_each(indexBuffer.begin(), indexBuffer.end(), [&](auto &n)
                         { remapVertexMap.try_emplace(n, vertexbufferOrg[n]); });

                uint32_t reindexidx = 0;
                for (auto &val : remapVertexMap)
                {
                    indexMap.try_emplace(val.first, reindexidx++);
                    copy(val.second.begin(), val.second.end(), back_insert_iterator(vertexBuffer));
                }
                ostringstream spath;
                spath << saveDir << "mesh_" << std::setw(3) << std::setfill('0') << meshIndex << '_' << FMTGet << ".bin";
                cout << spath.str() << '\n'
                     << "paiMesh->mNumVertices " << paiMesh->mNumVertices  
                     << "vertexBuffer.size() " << vertexBuffer.size() <<endl;
                ofstream filebuffer{spath.str(), ios::out | ofstream::binary};
                filebuffer.write(reinterpret_cast<char *>(vertexBuffer.data()), vertexBuffer.size() * sizeof(decltype(vertexBuffer)::value_type));
                //*saveMesh->mutable_vertices() = {vertexBuffer.begin(), vertexBuffer.end()};
                for_each(indexBuffer.begin(), indexBuffer.end(), [&](auto &n)
                         { n = indexMap[n]; });
            }
            if (saveSlist)
            {
                ostringstream spath;
                spath << saveDir << "mesh_" << std::setw(3) << std::setfill('0') << meshIndex << "_index_" << '0' << "_slist"
                      << ".bin";
                ofstream filebuffer{spath.str(), ios::out | ios::binary};
                filebuffer.write(reinterpret_cast<const char *>(indexBuffer.data()), sizeof(unsigned short) * indexBuffer.size());
                ostringstream sdpath;
                sdpath << saveDir << "mesh_" << std::setw(3) << std::setfill('0') << meshIndex << "_index_" << '0' << "_slist_draw"
                       << ".bin";
                ofstream filebufferdraw{sdpath.str(), ios::out | ios::binary};
                array<uint32_t, 5> drawCmd{static_cast<uint32_t>(indexBuffer.size()), 1, 0, 0, 0};
                filebufferdraw.write(reinterpret_cast<const char *>(drawCmd.data()), sizeof(decltype(drawCmd)::value_type) * drawCmd.size());
            }
            {
                unique_ptr<PrimitiveGroup[]> prims;
                unsigned short numprims;
                {
                    PrimitiveGroup *pprims;
                    bool done = GenerateStrips(indexBuffer.data(), indexBuffer.size(), &pprims, &numprims, true);
                    prims.reset(pprims);
                }
                for (unsigned int primidx = 0; primidx < numprims; ++primidx)
                {
                    // auto drawstep = saveMesh->add_drawstep();
                    PrimitiveGroup &pg = prims[primidx];
                    cout << "pg.type:" << pg.type << endl;
                    // drawstep->set_type(ptfile::Model_Index_Type_strip);

                    cout << "pg.numIndices:" << pg.numIndices << endl;
                    //*drawstep->mutable_indices() = {pg.indices, pg.indices + pg.numIndices};
                    // for (int i = 0; i < pg.numIndices; ++i)
                    //{
                    //    cout << pg.indices[i] << ' ';
                    //}
                    // cout << endl;
                    {
                        ostringstream spath;
                        spath << saveDir << "mesh_" << std::setw(3) << std::setfill('0') << meshIndex << "_index_" << primidx << "_strip"
                              << ".bin";
                        ofstream filebuffer{spath.str(), ios::out | ios::binary};
                        filebuffer.write(reinterpret_cast<const char *>(pg.indices), sizeof(unsigned short) * pg.numIndices);
                    }

                    {
                        ostringstream sdpath;

                        sdpath << saveDir << "mesh_" << std::setw(3) << std::setfill('0') << meshIndex << "_index_" << primidx << "_strip_draw"
                               << ".bin";
                        ofstream filebufferdraw{sdpath.str(), ios::out | ios::binary};
                        array<uint32_t, 5> drawCmd{pg.numIndices, 1, 0, 0, 0};
                        filebufferdraw.write(reinterpret_cast<const char *>(drawCmd.data()), sizeof(decltype(drawCmd)::value_type) * drawCmd.size());
                    }
                }
            }
            if (flStrip)
            {
                unique_ptr<PrimitiveGroup[]> prims;
                for(int iIndex=1;iIndex<indexBuffer.size();iIndex+=3){
                    swap(indexBuffer[iIndex],indexBuffer[iIndex+1]);
                }
                unsigned short numprims;
                {
                    PrimitiveGroup *pprims;
                    bool done = GenerateStrips(indexBuffer.data(), indexBuffer.size(), &pprims, &numprims, true);
                    prims.reset(pprims);
                }
                for (unsigned int primidx = 0; primidx < numprims; ++primidx)
                {
                    PrimitiveGroup &pg = prims[primidx];
                    cout << "pg.type:" << pg.type << endl;
                    cout << "pg.numIndices:" << pg.numIndices << endl;
                    {
                        ostringstream spath;
                        spath << saveDir << "mesh_" << std::setw(3) << std::setfill('0') << meshIndex << "_index_" << primidx << "_fpstrip"
                              << ".bin";
                        ofstream filebuffer{spath.str(), ios::out | ios::binary};
                        filebuffer.write(reinterpret_cast<const char *>(pg.indices), sizeof(unsigned short) * pg.numIndices);
                    }

                    {
                        ostringstream sdpath;

                        sdpath << saveDir << "mesh_" << std::setw(3) << std::setfill('0') << meshIndex << "_index_" << primidx << "_fpstrip_draw"
                               << ".bin";
                        ofstream filebufferdraw{sdpath.str(), ios::out | ios::binary};
                        array<uint32_t, 5> drawCmd{pg.numIndices, 1, 0, 0, 0};
                        filebufferdraw.write(reinterpret_cast<const char *>(drawCmd.data()), sizeof(decltype(drawCmd)::value_type) * drawCmd.size());
                    }
                }
            }
        }
        string path = loadFilename + ".idx.bin";
        ofstream savemt{path, ios::out | ios::binary};
        savemt.write(reinterpret_cast<const char *>(materials.data()), sizeof(decltype(materials)::value_type) * materials.size());
    }
    // cleanup
}