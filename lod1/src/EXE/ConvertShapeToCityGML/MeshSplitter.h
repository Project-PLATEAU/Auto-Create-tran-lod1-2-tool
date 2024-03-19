#pragma once
#include <atlstr.h>
#include <vector>
#include "shapefil.h"
#include <map>
#include <set>

// 2D座標の構造体
struct Point
{
	double x;
	double y;
};
// ポリゴンの情報を格納するクラス
class PolygonData
{
public:
	int id;
	std::vector<Point> vertices;
	CString meshCode;
	PolygonData(const int inputId, const CString& code) : id(inputId), meshCode(code) {}
};



class MeshSplitter
{
public:
	enum Datum
	{
		DATUM_DATUM_NONE = -1,
		DATUM_TKY_XY = 0,						//!< 日本測地平面直角
		DATUM_TKY_XY_01 = 1,
		DATUM_TKY_XY_02 = 2,
		DATUM_TKY_XY_03 = 3,
		DATUM_TKY_XY_04 = 4,
		DATUM_TKY_XY_05 = 5,
		DATUM_TKY_XY_06 = 6,
		DATUM_TKY_XY_07 = 7,
		DATUM_TKY_XY_08 = 8,
		DATUM_TKY_XY_09 = 9,
		DATUM_TKY_XY_10 = 10,
		DATUM_TKY_XY_11 = 11,
		DATUM_TKY_XY_12 = 12,
		DATUM_TKY_XY_13 = 13,
		DATUM_TKY_XY_14 = 14,
		DATUM_TKY_XY_15 = 15,
		DATUM_TKY_XY_16 = 16,
		DATUM_TKY_XY_17 = 17,
		DATUM_TKY_XY_18 = 18,
		DATUM_TKY_XY_19 = 19,
		DATUM_TKY_BL_FLAT = 20,						//!< 日本測地緯度経度
		DATUM_TKY_BL_SPHERE = 21,
		DATUM_JGD_XY = 100,							//!< 世界測地平面直角
		DATUM_JGD_XY_01 = 101,
		DATUM_JGD_XY_02 = 102,
		DATUM_JGD_XY_03 = 103,
		DATUM_JGD_XY_04 = 104,
		DATUM_JGD_XY_05 = 105,
		DATUM_JGD_XY_06 = 106,
		DATUM_JGD_XY_07 = 107,
		DATUM_JGD_XY_08 = 108,
		DATUM_JGD_XY_09 = 109,
		DATUM_JGD_XY_10 = 110,
		DATUM_JGD_XY_11 = 111,
		DATUM_JGD_XY_12 = 112,
		DATUM_JGD_XY_13 = 113,
		DATUM_JGD_XY_14 = 114,
		DATUM_JGD_XY_15 = 115,
		DATUM_JGD_XY_16 = 116,
		DATUM_JGD_XY_17 = 117,
		DATUM_JGD_XY_18 = 118,
		DATUM_JGD_XY_19 = 119,
		DATUM_JGD_BL_FLAT = 120,					//!< ITRF,GRS80
		DATUM_JGD_BL_SPHERE = 121,
		DATUM_TKY_UTM = 200,						//!< 日本測地UTM
		DATUM_TKY_UTM_01 = 201,
		DATUM_TKY_UTM_02 = 202,
		DATUM_TKY_UTM_03 = 203,
		DATUM_TKY_UTM_04 = 204,
		DATUM_TKY_UTM_05 = 205,
		DATUM_TKY_UTM_06 = 206,
		DATUM_TKY_UTM_07 = 207,
		DATUM_TKY_UTM_08 = 208,
		DATUM_TKY_UTM_09 = 209,
		DATUM_TKY_UTM_10 = 210,
		DATUM_TKY_UTM_11 = 211,
		DATUM_TKY_UTM_12 = 212,
		DATUM_TKY_UTM_13 = 213,
		DATUM_TKY_UTM_14 = 214,
		DATUM_TKY_UTM_15 = 215,
		DATUM_TKY_UTM_16 = 216,
		DATUM_TKY_UTM_17 = 217,
		DATUM_TKY_UTM_18 = 218,
		DATUM_TKY_UTM_19 = 219,
		DATUM_TKY_UTM_20 = 220,
		DATUM_TKY_UTM_21 = 221,
		DATUM_TKY_UTM_22 = 222,
		DATUM_TKY_UTM_23 = 223,
		DATUM_TKY_UTM_24 = 224,
		DATUM_TKY_UTM_25 = 225,
		DATUM_TKY_UTM_26 = 226,
		DATUM_TKY_UTM_27 = 227,
		DATUM_TKY_UTM_28 = 228,
		DATUM_TKY_UTM_29 = 229,
		DATUM_TKY_UTM_30 = 230,
		DATUM_TKY_UTM_31 = 231,
		DATUM_TKY_UTM_32 = 232,
		DATUM_TKY_UTM_33 = 233,
		DATUM_TKY_UTM_34 = 234,
		DATUM_TKY_UTM_35 = 235,
		DATUM_TKY_UTM_36 = 236,
		DATUM_TKY_UTM_37 = 237,
		DATUM_TKY_UTM_38 = 238,
		DATUM_TKY_UTM_39 = 239,
		DATUM_TKY_UTM_40 = 240,
		DATUM_TKY_UTM_41 = 241,
		DATUM_TKY_UTM_42 = 242,
		DATUM_TKY_UTM_43 = 243,
		DATUM_TKY_UTM_44 = 244,
		DATUM_TKY_UTM_45 = 245,
		DATUM_TKY_UTM_46 = 246,
		DATUM_TKY_UTM_47 = 247,
		DATUM_TKY_UTM_48 = 248,
		DATUM_TKY_UTM_49 = 249,
		DATUM_TKY_UTM_50 = 250,
		DATUM_TKY_UTM_51 = 251,
		DATUM_TKY_UTM_52 = 252,
		DATUM_TKY_UTM_53 = 253,
		DATUM_TKY_UTM_54 = 254,
		DATUM_TKY_UTM_55 = 255,
		DATUM_TKY_UTM_56 = 256,
		DATUM_TKY_UTM_57 = 257,
		DATUM_TKY_UTM_58 = 258,
		DATUM_TKY_UTM_59 = 259,
		DATUM_TKY_UTM_60 = 260,
		DATUM_JGD_UTM = 300,						//!< 世界測地UTM
		DATUM_JGD_UTM_01 = 301,
		DATUM_JGD_UTM_02 = 302,
		DATUM_JGD_UTM_03 = 303,
		DATUM_JGD_UTM_04 = 304,
		DATUM_JGD_UTM_05 = 305,
		DATUM_JGD_UTM_06 = 306,
		DATUM_JGD_UTM_07 = 307,
		DATUM_JGD_UTM_08 = 308,
		DATUM_JGD_UTM_09 = 309,
		DATUM_JGD_UTM_10 = 310,
		DATUM_JGD_UTM_11 = 311,
		DATUM_JGD_UTM_12 = 312,
		DATUM_JGD_UTM_13 = 313,
		DATUM_JGD_UTM_14 = 314,
		DATUM_JGD_UTM_15 = 315,
		DATUM_JGD_UTM_16 = 316,
		DATUM_JGD_UTM_17 = 317,
		DATUM_JGD_UTM_18 = 318,
		DATUM_JGD_UTM_19 = 319,
		DATUM_JGD_UTM_20 = 320,
		DATUM_JGD_UTM_21 = 321,
		DATUM_JGD_UTM_22 = 322,
		DATUM_JGD_UTM_23 = 323,
		DATUM_JGD_UTM_24 = 324,
		DATUM_JGD_UTM_25 = 325,
		DATUM_JGD_UTM_26 = 326,
		DATUM_JGD_UTM_27 = 327,
		DATUM_JGD_UTM_28 = 328,
		DATUM_JGD_UTM_29 = 329,
		DATUM_JGD_UTM_30 = 330,
		DATUM_JGD_UTM_31 = 331,
		DATUM_JGD_UTM_32 = 332,
		DATUM_JGD_UTM_33 = 333,
		DATUM_JGD_UTM_34 = 334,
		DATUM_JGD_UTM_35 = 335,
		DATUM_JGD_UTM_36 = 336,
		DATUM_JGD_UTM_37 = 337,
		DATUM_JGD_UTM_38 = 338,
		DATUM_JGD_UTM_39 = 339,
		DATUM_JGD_UTM_40 = 340,
		DATUM_JGD_UTM_41 = 341,
		DATUM_JGD_UTM_42 = 342,
		DATUM_JGD_UTM_43 = 343,
		DATUM_JGD_UTM_44 = 344,
		DATUM_JGD_UTM_45 = 345,
		DATUM_JGD_UTM_46 = 346,
		DATUM_JGD_UTM_47 = 347,
		DATUM_JGD_UTM_48 = 348,
		DATUM_JGD_UTM_49 = 349,
		DATUM_JGD_UTM_50 = 350,
		DATUM_JGD_UTM_51 = 351,
		DATUM_JGD_UTM_52 = 352,
		DATUM_JGD_UTM_53 = 353,
		DATUM_JGD_UTM_54 = 354,
		DATUM_JGD_UTM_55 = 355,
		DATUM_JGD_UTM_56 = 356,
		DATUM_JGD_UTM_57 = 357,
		DATUM_JGD_UTM_58 = 358,
		DATUM_JGD_UTM_59 = 359,
		DATUM_JGD_UTM_60 = 360,
		DATUM_TWD67_TM2 = 4000,
		DATUM_TWD67_TM2_119 = 4119,
		DATUM_TWD67_TM2_121 = 4121,
		DATUM_TWD97_TM2 = 5000,
		DATUM_TWD97_TM2_119 = 5119,
		DATUM_TWD97_TM2_121 = 5121,
		DATUM_DATUM_NUM
	};

	MeshSplitter();
	~MeshSplitter();

	void Run();
	static bool GetMeshCodeArea(
		const CString& strCode,				//!< in		メッシュコード
		Datum		datum,						//!< in		測地系
		int	meshLevel,							//!< in		メッシュレベル
		std::pair<double, double>& vLB,			//!< out	左下
		std::pair<double, double>& vRB,			//!< out	右下
		std::pair<double, double>& vRT,			//!< out	右上
		std::pair<double, double>& vLT			//!< out	左上
	);

	static bool Get1stMeshLB(
		const double& dLatDeg,	//!< in 緯度(度)
		const double& dLonDeg,	//!< in 経度(度)
		CString& strCode,		//!< out メッシュコード
		double& dLBLatDeg,		//!< out 左下緯度(度)
		double& dLBLonDeg		//!< out 左下経度(度)
	);

	static bool Get2ndMeshLB(
		const double& dLatDeg,	//!< in 緯度(度)
		const double& dLonDeg,	//!< in 経度(度)
		CString& strCode,		//!< out メッシュコード
		double& dLBLatDeg,		//!< out 左下緯度(度)
		double& dLBLonDeg		//!< out 左下経度(度)
	);

	static bool Get2ndMesh(
		const double& dLatDeg,	//!< in 緯度(度)
		const double& dLonDeg,	//!< in 経度(度)
		CString& strCode		//!< out メッシュコード
	);

	static bool Get3rdMeshLB(
		const double& dLatDeg,	//!< in 緯度(度)
		const double& dLonDeg,	//!< in 経度(度)
		CString& strCode,		//!< out メッシュコード
		double& dLBLatDeg,		//!< out 左下緯度(度)
		double& dLBLonDeg		//!< out 左下経度(度)
	);

	static bool Get3rdMesh(
		const double& dLatDeg,	//!< in 緯度(度)
		const double& dLonDeg,	//!< in 経度(度)
		CString& strCode		//!< out メッシュコード
	);

	static bool GetRecursiveMeshLB(
		const double& dLatDeg,	//!< in 緯度(度)
		const double& dLonDeg,	//!< in 経度(度)
		CString& strCode,		//!< out メッシュコード
		double& dLBLatDeg,		//!< out 左下緯度(度)
		double& dLBLonDeg,		//!< out 左下経度(度)
		int meshLevel			//!< in メッシュレベル
	);

	static bool GetRecursiveMesh(
		const double& dLatDeg,	//!< in 緯度(度)
		const double& dLonDeg,	//!< in 経度(度)
		CString& strCode,		//!< out メッシュコード
		int meshLevel			//!< in メッシュレベル
	);


	static bool GetMeshCode(
		const double& dLat,				//!< in		緯度
		const double& dLon,				//!< in		経度
		int meshLevel,					//!< in		メッシュレベル 3:3次メッシュ、4:4次メッシュ	
		CString& strCode				//!< out	メッシュコード
	);

	std::vector<PolygonData> FindMaxAreaPolygonMesh(
		std::vector<SHPObject*> shapeVec,
		std::set<CString>& meshCodes,
		std::set<CString>& targetMeshCodes,
		int JPZone,
		int meshLevel
	);
private:

};

