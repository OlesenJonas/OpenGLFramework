#include "obj.h"
#include <MikkTSpace/mikktspace.h>
#include <Misc/StringSplitter.h>

#include <fstream>
#include <iostream>

namespace OBJ
{
    struct IndirectVertex
    {
        uint32_t pIndex;
        uint32_t uvIndex;
        uint32_t nIndex;

        IndirectVertex() = default;
        explicit IndirectVertex(std::string_view vertexString)
        {
            auto indices = StringViewSplitter(vertexString, '/');
            pIndex = strtol(indices[0].data(), nullptr, 10);
            uvIndex = strtol(indices[1].data(), nullptr, 10);
            nIndex = strtol(indices[2].data(), nullptr, 10);
        }
    };
    struct Face
    {
        IndirectVertex vertices[3];
    };
    struct MikkTSpaceUserData
    {
        std::vector<glm::vec3>* ps;
        std::vector<glm::vec3>* ns;
        std::vector<glm::vec2>* uvs;
        std::vector<glm::vec4>* ts;
        std::vector<Face>* fs;
    };

    int mktGetNumFaces(const SMikkTSpaceContext* pContext)
    {
        auto* userData = reinterpret_cast<MikkTSpaceUserData*>(pContext->m_pUserData);
        return static_cast<int>(userData->fs->size());
    }
    int mktGetNumVerticesOfFace(const SMikkTSpaceContext* pContext, const int iFace)
    {
        (void)pContext;
        (void)iFace;
        return 3;
    }
    void
    mktGetPosition(const SMikkTSpaceContext* pContext, float fvPosOut[], const int iFace, const int iVert)
    {
        auto* userData = reinterpret_cast<MikkTSpaceUserData*>(pContext->m_pUserData);
        const Face& face = (*(userData->fs))[iFace];
        const IndirectVertex& vert = face.vertices[iVert];
        const glm::vec3& pos = (*(userData->ps))[vert.pIndex - 1];
        memcpy(fvPosOut, &pos.x, 3 * sizeof(float));
    }
    void mktGetNormal(const SMikkTSpaceContext* pContext, float fvNormOut[], const int iFace, const int iVert)
    {
        auto* userData = reinterpret_cast<MikkTSpaceUserData*>(pContext->m_pUserData);
        const Face& face = (*(userData->fs))[iFace];
        const IndirectVertex& vert = face.vertices[iVert];
        const glm::vec3& nrm = (*(userData->ns))[vert.nIndex - 1];
        memcpy(fvNormOut, &nrm.x, 3 * sizeof(float));
    }
    void
    mktGetTexCoord(const SMikkTSpaceContext* pContext, float fvTexcOut[], const int iFace, const int iVert)
    {
        auto* userData = reinterpret_cast<MikkTSpaceUserData*>(pContext->m_pUserData);
        const Face& face = (*(userData->fs))[iFace];
        const IndirectVertex& vert = face.vertices[iVert];
        const glm::vec2& uv = (*(userData->uvs))[vert.uvIndex - 1];
        memcpy(fvTexcOut, &uv.x, 2 * sizeof(float));
    }
    void mktSetTSpaceBasic(
        const SMikkTSpaceContext* pContext, const float fvTangent[3], const float fSign, const int iFace,
        const int iVert)
    {
        auto* userData = reinterpret_cast<MikkTSpaceUserData*>(pContext->m_pUserData);
        int vertIndex = iFace * 3 + iVert;
        glm::vec4& tangent = (*(userData->ts))[vertIndex];
        memcpy(&tangent.x, fvTangent, 3 * sizeof(float));
        tangent.w = fSign;
        assert(tangent.w == 1.0f || tangent.w == -1.0f && "Tangent sign does not have abs value of 1.0f");
    }
} // namespace OBJ

std::vector<VertexStruct> OBJ::load(const std::string& path)
{
    /*
        TODO:
            dont try to calculate tangents if mesh doesnt have UVs
            construct index buffer
    */

    std::ifstream file(path.c_str(), std::ios::in | std::ios::binary);

    std::string line;

    // parsing the file in two steps currently
    // 1. count how many elements there are, then allocate storage for them
    // 2. go through all lines again, now parsing them and filling the allocated buffers
    // could do this in one go with a vector that grows with each new line, not sure why I chose to do it this
    // way
    bool objectFound = false;
    decltype(file)::pos_type lastLinePos;
    int positionEntries = 0;
    decltype(file)::pos_type positionStartPos;
    int normalEntries = 0;
    decltype(file)::pos_type normalStartPos;
    int uvEntries = 0;
    decltype(file)::pos_type uvStartPos;
    int faceEntries = 0;
    decltype(file)::pos_type faceStartPos;
    // uint32_t positionEntries = 0;
    // decltype(file)::pos_type positionStartPos;
    while(lastLinePos = file.tellg(), getline(file, line))
    {
        if(line[0] == '#')
        {
            continue;
        }
        // skip everything until an object is started (for now at least)
        std::string_view lineType{line.data(), line.find(' ')};
        if(!objectFound)
        {
            if(lineType != "o")
            {
                continue;
            }
            objectFound = true;
        }
        else
        {
            if(lineType == "o")
            {
                // another object already found -> this is object number 2+, skip for now
                //  todo: some warning, "more than one object found"
                break;
            }
            else if(lineType == "v")
            {
                if(positionEntries == 0)
                {
                    positionStartPos = lastLinePos;
                }
                positionEntries++;
            }
            else if(lineType == "vn")
            {
                if(normalEntries == 0)
                {
                    normalStartPos = lastLinePos;
                }
                normalEntries++;
            }
            else if(lineType == "vt")
            {
                if(uvEntries == 0)
                {
                    uvStartPos = lastLinePos;
                }
                uvEntries++;
            }
            else if(lineType == "f")
            {
                if(faceEntries == 0)
                {
                    faceStartPos = lastLinePos;
                }
                faceEntries++;
            }
            else
            {
                // todo:
                // warning, add to log, : unrecognized line in obj #linenumber: #line ...
            }
        }
    }
    file.clear(); // clear eof flag

    // todo: or one large buffer containing everything
    std::vector<glm::vec3> positions(positionEntries);
    std::vector<glm::vec3> normals(normalEntries);
    std::vector<glm::vec2> uvs(uvEntries);
    std::vector<Face> faces(faceEntries);

    file.seekg(positionStartPos, std::ios::beg);
    // file.seekg(positionStartPos);
    for(auto i = 0; i < positionEntries; i++)
    {
        getline(file, line);
        auto lineEntries = StringViewSplitter(line, ' ');
        // strtof automatically stops converting after the "float part" of the string ended, so string_view
        // data() is enough
        positions[i] = {
            strtof(lineEntries[1].data(), nullptr),
            strtof(lineEntries[2].data(), nullptr),
            strtof(lineEntries[3].data(), nullptr)};
    }
    file.clear(); // clear eof flag

    file.seekg(normalStartPos);
    for(auto i = 0; i < normalEntries; i++)
    {
        getline(file, line);
        auto lineEntries = StringViewSplitter(line, ' ');
        normals[i] = {
            strtof(lineEntries[1].data(), nullptr),
            strtof(lineEntries[2].data(), nullptr),
            strtof(lineEntries[3].data(), nullptr)};
    }
    file.clear(); // clear eof flag

    file.seekg(uvStartPos);
    for(auto i = 0; i < uvEntries; i++)
    {
        getline(file, line);
        auto lineEntries = StringViewSplitter(line, ' ');
        uvs[i] = {strtof(lineEntries[1].data(), nullptr), strtof(lineEntries[2].data(), nullptr)};
    }
    file.clear(); // clear eof flag

    file.seekg(faceStartPos);
    for(auto i = 0; i < faceEntries; i++)
    {
        getline(file, line);
        auto lineEntries = StringViewSplitter(line, ' ');
        if(lineEntries[0] != "f")
        {
            continue;
        }
        if(lineEntries.size() > 4)
        {
            std::cout << "Model contains quad, skipping it: " << path << std::endl;
            continue;
        }
        // strtof automatically stops converting after the "float part" of the string ended, so
        // string_view data() is enough
        faces[i] = {
            IndirectVertex{lineEntries[1]}, IndirectVertex{lineEntries[2]}, IndirectVertex{lineEntries[3]}};
    }
    file.clear(); // clear eof flag

    // todo: be smarter with the following (generating tangenst & constructing vertices from tangents)!
    // eg: dont write tangents directly into some buffer, only store them if theyre unique! and add just the
    // index/ptr to that value to the IndirectVertexStruct (ie: replicate the .obj format for the tangents
    // aswell)
    // then, similarly for creating the vertices:
    // use an index buffer, dont construct a VertexStruct for every vertex, use the v/vt/vn(/tangent) index as
    // a key to only construct unique vertices and fill the buffer with those!

    std::vector<glm::vec4> tangents(faces.size() * 3);

    MikkTSpaceUserData mktUserData = {&positions, &normals, &uvs, &tangents, &faces};
    SMikkTSpaceContext mktContext = {nullptr};
    SMikkTSpaceInterface mktInterface = {nullptr};
    mktContext.m_pUserData = &mktUserData;
    mktContext.m_pInterface = &mktInterface;
    mktInterface.m_getNumFaces = mktGetNumFaces;
    mktInterface.m_getNumVerticesOfFace = mktGetNumVerticesOfFace;
    mktInterface.m_getPosition = mktGetPosition;
    mktInterface.m_getTexCoord = mktGetTexCoord;
    mktInterface.m_getNormal = mktGetNormal;
    mktInterface.m_setTSpaceBasic = mktSetTSpaceBasic;
    assert(genTangSpaceDefault(&mktContext));

    std::vector<VertexStruct> vertices(faces.size() * 3);
    for(int i = 0; i < faces.size(); i++)
    {
        for(int vert = 0; vert < 3; vert++)
        {
            // indices start at 1 in obj file
            vertices[i * 3 + vert] = {
                positions[faces[i].vertices[vert].pIndex - 1],
                normals[faces[i].vertices[vert].nIndex - 1],
                uvs[faces[i].vertices[vert].uvIndex - 1],
                tangents[i * 3 + vert]};
        }
    }
    file.close();

    return vertices;
    // when doing index buffer, returning vertices *and* indices can be handled like this:
    // https://stackoverflow.com/questions/51521031/return-stdtuple-and-move-semantics-copy-elision
}
