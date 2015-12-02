#ifndef OBJLOADER_INCLUDED_
#define OBJLOADER_INCLUDED_

#include <vector>
#include <string>
#include <stdint.h>

// structure for reading the obj model data
struct tVertex
{
    float   x,y,z;
    float   u,v;
	float   nx, ny, nz;
};

typedef int32_t ObjIndexInt;

struct tSegment
{
    std::string m_name;
    int m_start;
    int m_end;
};

struct tObjModel
{
    std::vector<tSegment>   m_segments;
    std::vector<tVertex>    m_vertices;
	std::vector<ObjIndexInt>     m_indices;
};

void objLoader(const char *filename, tObjModel &model);

#endif