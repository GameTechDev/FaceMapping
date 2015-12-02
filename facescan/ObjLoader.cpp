#include "ObjLoader.h"

#include <stdio.h>
#include <windows.h>


#define ARRAY_SIZE(x)   (sizeof(x)/sizeof(x[0]))

enum
{
    UNKNOWN,
    VERTEX,
    NORMAL,
    TEXCOORD,
    FACE,
    GROUP,
    USEMATERIAL,
    MATERIALLIB,
    COMMENT
};

struct tPair
{
    int value;
    const char *string;
};

static const tPair objTokens[] = 
{
    {   UNKNOWN,    ""              },
    {   VERTEX,     "v"             },
    {   NORMAL,     "vn"            },
    {   TEXCOORD,   "vt"            },
    {   FACE,       "f"             },
    {   GROUP,      "g"             },
    {   USEMATERIAL,"usemtl"        },
    {   MATERIALLIB,"mtllib"        },
    {   COMMENT,    "#"             },

};

int getToken(const char *value)
{
    int token = UNKNOWN;

    for ( int i=0; i<ARRAY_SIZE(objTokens); ++i)
    {
        if (!strcmp(objTokens[i].string,value))
        {
            token = objTokens[i].value;
            break;
        }
    }

    return token;
}

void skipLine(FILE *fp)
{
    // read characters till the end of the line
    while ((!feof(fp)) && (fgetc(fp)!='\n'))
    {
    }
}

struct tVector
{
    float m[3];
};

struct tVectorI
{
    int m[3];
};

float min(float a, float b)
{
    return (a<b) ? a:b;
}

tVector min( const tVector &a, const tVector &b)
{
    tVector ret;

    ret.m[0] = min(a.m[0], b.m[0]);
    ret.m[1] = min(a.m[1], b.m[1]);
    ret.m[2] = min(a.m[2], b.m[2]);

    return ret;
}

float max(float a, float b)
{
    return (a>b) ? a:b;
}

tVector max( const tVector &a, const tVector &b)
{
    tVector ret;

    ret.m[0] = max(a.m[0], b.m[0]);
    ret.m[1] = max(a.m[1], b.m[1]);
    ret.m[2] = max(a.m[2], b.m[2]);

    return ret;
}

void pushVector(FILE *fp, std::vector<tVector> &vecs)
{
    tVector tmp;

    fscanf(fp,"%f %f %f", &tmp.m[0], &tmp.m[1], &tmp.m[2]);
    vecs.push_back(tmp);
}

bool operator == ( const tVectorI &v0, const tVectorI &v1)
{
    return ((v0.m[0] == v1.m[0]) && (v0.m[1] == v1.m[1]) && (v0.m[2] == v1.m[2]));
}

#define UNIQUE_VERTICES 0
// create a unique index set for the data
void pushVectorI(FILE *fp, std::vector<tVectorI> &vecs, std::vector<ObjIndexInt> &uniqueIndices)
{
    tVectorI tmp;

    fscanf(fp,"%d/%d/%d", &tmp.m[0], &tmp.m[1], &tmp.m[2]);

    // obj files are indexed from 1
    tmp.m[0]-=1;
    tmp.m[1]-=1;
    tmp.m[2]-=1;

    // should we re-assign to create unique vertices?
#if UNIQUE_VERTICES

    std::vector<tVectorI>::iterator it = std::find(vecs.begin(), vecs.end(), tmp);

    if ( it == vecs.end())
    {
        uniqueIndices.push_back(vecs.size());
        vecs.push_back(tmp);
    }
    else
    {
        uniqueIndices.push_back(it-vecs.begin());
    }
#else
	uniqueIndices.push_back((ObjIndexInt)vecs.size());
    vecs.push_back(tmp);
#endif


}

void objLoader(const char *filename, tObjModel &model)
{
    FILE *fp = 0;
	
    fp = fopen(filename,"r");
	if (fp == NULL)
	{
		Sleep(1000);
		int errnoresult = errno;
		printf( "  err %d \n", errnoresult );
		fp = fopen(filename, "r"); 
	}

    // structures for loading the data
    std::vector<tVector>    tempVertices;
    std::vector<tVector>    tempNormals;
    std::vector<tVector>    tempUVs;

    std::vector<tVectorI>   tempFaces;
    
    model.m_segments.clear();
    model.m_indices.clear();
    model.m_vertices.clear();

    if ( fp != 0 )
    {
        while (!feof(fp))
        {
            char lineHeader[32];

            // read the first token from the line
            fscanf(fp,"%s", lineHeader);

            int token = getToken(lineHeader);

            switch(token)
            {
                case   UNKNOWN:     skipLine(fp);   break;
                case   VERTEX:      pushVector(fp,tempVertices);    break;
                case   NORMAL:      pushVector(fp,tempNormals);     break;
                case   TEXCOORD:    pushVector(fp,tempUVs);         break;
                case   FACE:        
                    pushVectorI(fp,tempFaces,model.m_indices);      
                    pushVectorI(fp,tempFaces,model.m_indices);      
                    pushVectorI(fp,tempFaces,model.m_indices);      
                    break;    
                case   GROUP:
                    {
                        char name[256];
                        fscanf(fp,"%s",name);

                        if ( model.m_segments.size() > 0 )
                        {
							model.m_segments.back().m_end = (int)model.m_indices.size();
                        }


                        tSegment newSegment;

                        newSegment.m_name.append(name);
						newSegment.m_start = (int)model.m_indices.size();
                        newSegment.m_end   = 0;

                        model.m_segments.push_back(newSegment);
    
                    }
                // all these cases fall through and we skip the line
                case   USEMATERIAL:
                case   MATERIALLIB:
                case   COMMENT:    
                default:    skipLine(fp);   break;
            };

            if ( model.m_segments.size() > 0 )
            {
                model.m_segments.back().m_end = (int)model.m_indices.size();
            }
        }

        // find a bounding box?
        if ( tempVertices.size() > 0 )
        {
            tVector vmin = tempVertices[0];
            tVector vmax = tempVertices[0];

            for ( std::vector<tVector>::const_iterator it = tempVertices.begin(); it != tempVertices.end(); ++it )
            {
                vmin = min(vmin,*it);
                vmax = max(vmax,*it);
            }

            tVector bsize;

            bsize.m[0] = vmax.m[0] - vmin.m[0];
            bsize.m[1] = vmax.m[1] - vmin.m[1];
            bsize.m[2] = vmax.m[2] - vmin.m[2];
        }

        // to complete the model
        // generate the interleaved vertex data from the temporary face indices
        for ( std::vector<tVectorI>::iterator it = tempFaces.begin(); it != tempFaces.end(); ++it )
        {
            tVertex tmp;

            tVector &v  = tempVertices[it->m[0]];
            tVector &vt = tempUVs[it->m[1]];
            tVector &vn = tempNormals[it->m[2]];

            
            tmp.x = v.m[0];
            tmp.y = v.m[1];
            tmp.z = v.m[2];

            // normals are inverted on the model!
            tmp.nx = vn.m[0];
            tmp.ny = vn.m[1];
            tmp.nz = vn.m[2];

            tmp.u = vt.m[0];
            tmp.v = vt.m[1];

            model.m_vertices.push_back(tmp);
        }

        fclose(fp);
    }
}