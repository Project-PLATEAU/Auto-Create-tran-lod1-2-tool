#include "CReadParamFile.h"
#include "ReadAttributeFile.h"
#include "CShapeManager.h"
#include "CDMDataManager.h"
#include "CGeoUtil.h"
#include "MeshSplitter.h"
#include "OutputCityGML.h"

#include <set>
#include <cstring>
#include <variant>

int main(int argc, char* argv[])
{
	if (argc != 2)
	{
		// パラメータエラー
		std::cout << "Usage : ConvertShapeToCityGML.exe conv_param.txt" << std::endl;
		return -1;
	}

	// パラメータファイル読み込み
	std::string strParamPath(argv[1]);
	GetCreateParam()->Initialize(strParamPath);
	GetCreateParam()->Print();

	// マップファイル読み込み
	std::string strAttributePath(GetCreateParam()->GetMapFilePath());
	GetAttributeInstance()->Initialize(strAttributePath);
	GetAttributeInstance()->Print();

	// シェイプファイル読み込み
	std::string strShapefilePath(GetCreateParam()->GetRoadSHPPath());
	CShapeManager* csm = new CShapeManager();
	csm->Open(strShapefilePath);
	csm->Print();

	// shapeファイル読み込み
	CDMDataManager roadPolygonShpMng = CDMDataManager();
	std::vector<AttributeDataManager> attrDataVec;
	if (roadPolygonShpMng.ReadRoadPolygonShapeFile(attrDataVec) == false)
	{
		// 失敗
		std::cout << "Error : Read road edge shape file." << std::endl;
		return -1;
	}

	// 座標変換
	int JPZone = GetCreateParam()->GetJPZone();
	std::cout << "JPZone: " << JPZone << std::endl;
	MeshSplitter* meshSplitter = new MeshSplitter();

	// メッシュコード
	std::set<CString> meshCodes;
	std::set<CString> targetMeshCodes;
	int meshLevel = 3; // 3次メッシュ(基準地域メッシュ)
	while (true)
	{
		// 面積計算
		std::vector<PolygonData> polygonData = meshSplitter->FindMaxAreaPolygonMesh(csm->psShapeVec, meshCodes, targetMeshCodes, JPZone, meshLevel);
		targetMeshCodes.clear();

		// CityGML出力
		OutputCityGML* outputPtr = new OutputCityGML();

		if (outputPtr->Run(polygonData, meshCodes, targetMeshCodes, attrDataVec, JPZone, meshLevel, GetCreateParam()->GetOutputFolderPath()))
		{
			break;
		}
		meshCodes.clear();
		meshLevel++;
	}

	csm->Close();

	return 0;
}