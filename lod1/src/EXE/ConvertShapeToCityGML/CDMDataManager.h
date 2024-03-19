#pragma once
#include <Windows.h>
#include <string>
#include <algorithm>
#include <iterator>
#include <map>
#include "../../LIB/CommonUtil/CGeoUtil.h"
#include "DMType.h"
#include "shapefil.h"
#include <atlstr.h>
#include <variant>

class AttributeData
{
public:
	std::string className;
	std::string attrName;
	std::variant<int, double, std::string> attrValue;

	AttributeData(
		const std::string cName,
		const std::string aName,
		const std::variant<int, double, std::string> value)
	{
		className = cName;
		attrName = aName;
		attrValue = value;
	}

};

class AttributeDataManager
{
public:
	int id;
	std::vector<AttributeData> attrDataVec;
	AttributeDataManager(const int inputId) : id(inputId) {}
};

class CDMDataManager
{
public:
	struct RoadEdgeData
	{
		DMCode                      nDMCode;        //!< DMコード(大分類)
		DMRoadCode                  nRoadCode;      //!< DMコード(小分類)
		std::vector<CVector3D>      vecPolyline;    //!< ポリライン頂点列

		/*!
		 * コンストラクタ
		 */
		RoadEdgeData()
		{
			nDMCode = DMCode::UNCATEGORIZED;
			nRoadCode = DMRoadCode::UNCATEGORIZED_ROAD;
		}

		/*!
		 * デストラクタ
		 */
		virtual ~RoadEdgeData() {}

		/*!
		 * コピーコンストラクタ
		 */
		RoadEdgeData(const RoadEdgeData& x) { *this = x; }

		/*!
		 * 代入演算子
		 */
		RoadEdgeData& operator=(const RoadEdgeData& x)
		{
			if (this != &x)
			{
				nDMCode = x.nDMCode;
				nRoadCode = x.nRoadCode;
				std::copy(x.vecPolyline.begin(), x.vecPolyline.end(), std::back_inserter(vecPolyline));
			}
			return *this;
		}
	};

	struct RoadFacilitiesData
	{
		std::vector<CVector3D>  vecPolyline;            //!< ポリライン頂点列
		DMCode                  nDMCode;                //!< DMコード(大分類)
		DMRoadFacilitiesCode    nRoadFacilitiesCode;    //!< DMコード(小分類)
		DMGeometryType          nGeometryType;          //!< 図形区分

		/*!
		 * コンストラクタ
		 */
		RoadFacilitiesData()
		{
			nDMCode = DMCode::UNCATEGORIZED;
			nRoadFacilitiesCode = DMRoadFacilitiesCode::UNCATEGORIZED_ROAD_FACILITIES;
			nGeometryType = DMGeometryType::UNCLASSIFIED;
		}

		/*!
		 * デストラクタ
		 */
		virtual ~RoadFacilitiesData() {}

		/*!
		 * コピーコンストラクタ
		 */
		RoadFacilitiesData(const RoadFacilitiesData& x) { *this = x; }

		/*!
		 * 代入演算子
		 */
		RoadFacilitiesData& operator=(const RoadFacilitiesData& x)
		{
			if (this != &x)
			{
				//nId = x.nId;
				nDMCode = x.nDMCode;
				nRoadFacilitiesCode = x.nRoadFacilitiesCode;
				nGeometryType = x.nGeometryType;
				std::copy(x.vecPolyline.begin(), x.vecPolyline.end(), std::back_inserter(vecPolyline));
			}
			return *this;
		}
	};

	enum class AttributeClassName
	{
		TRAN_ROAD_INTEGER,
		TRAN_ROAD_STRING,
		URO_ROAD_STRUCTURE,
		URO_TRAFFIC_INTEGER,
		URO_TRAFFIC_DOUBLE,
		URO_TRAFFIC_STRING,
		URO_TRANSPORTATION_DATA_QUALITY,
		URO_DM
	};

	CDMDataManager(void) {}     //!< コンストラクタ
	~CDMDataManager(void) {}    //!< デストラクタ
	bool ReadRoadPolygonShapeFile(std::vector<AttributeDataManager>& codeDataVec);   // 道路縁shape読み込み
	std::map<std::string, std::vector<int>> GetTranRoadIntegerCodeData();
	std::map<std::string, std::vector<int>> GetUroRoadStructureCodeData();
	std::map<std::string, std::vector<int>> GetUroTrafficIntegerCodeData();
	std::map<std::string, std::vector<int>> GetUroTransportationDataQualityCodeData();
	std::map<std::string, std::vector<int>> GetUroDmCodeData();
	std::map<std::string, std::vector<double>> GetUroTrafficDoubleCodeData();
	std::map<std::string, std::vector<std::string>> GetTranRoadStringCodeData();
	std::map<std::string, std::vector<std::string>> GetUroTrafficStringCodeData();
protected:

private:
	// 整数属性値データ配列マップ
	std::map<std::string, std::vector<int>> m_TranRoadIntegerCodeDataMap;
	std::map<std::string, std::vector<int>> m_UroRoadStructureCodeDataMap;
	std::map<std::string, std::vector<int>> m_UroTrafficIntegerCodeDataMap;
	std::map<std::string, std::vector<int>> m_UroTransportationDataQualityCodeDataMap;
	std::map<std::string, std::vector<int>> m_UroDmCodeDataMap;

	// 小数属性値データ配列マップ
	std::map<std::string, std::vector<double>> m_UroTrafficDoubleCodeDataMap;

	// 文字列属性値データ配列マップ
	std::map<std::string, std::vector<std::string>> m_TranRoadStringCodeDataMap;
	std::map<std::string, std::vector<std::string>> m_UroTrafficStringCodeDataMap;

	std::string UTF8toSJIS(const char* bufUTF8);
	std::string AttributeClassNameToString(AttributeClassName className);
};
