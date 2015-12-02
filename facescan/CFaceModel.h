#ifndef __OBJ_MODEL__
#define __OBJ_MODEL__

#include "ObjLoader.h"
#include <map>
#include "CPUTMath.h"

class CPUTModel;
class CPUTTexture;

class CFaceModel
{
public:

	CFaceModel();
	~CFaceModel();

	void LoadObjFilename(const std::string &filename, bool landmarks = true);
	CPUTModel *LoadCPUTModel(CPUTTexture **outColorTexture);

	bool LoadLandmarks();

	std::map<std::string, float3> Landmarks;

	float3 AABBMin;
	float3 AABBMax;

private:
	std::string mObjFilename;
	tObjModel mObjModel;

	// values that were applied to the objmodel after it was loaded
	// in an effort to normalize the mesh
	float mVertScale;
	float3 mVertOffset;
	
	bool LoadLandmarks( const std::string &filename, std::map<std::string, float3> &outMap, int formatVersion = 0 );
	
};

#endif // __OBJ_MODEL__