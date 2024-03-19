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
		DMCode                      nDMCode;        //!< DM�R�[�h(�啪��)
		DMRoadCode                  nRoadCode;      //!< DM�R�[�h(������)
		std::vector<CVector3D>      vecPolyline;    //!< �|�����C�����_��

		/*!
		 * �R���X�g���N�^
		 */
		RoadEdgeData()
		{
			nDMCode = DMCode::UNCATEGORIZED;
			nRoadCode = DMRoadCode::UNCATEGORIZED_ROAD;
		}

		/*!
		 * �f�X�g���N�^
		 */
		virtual ~RoadEdgeData() {}

		/*!
		 * �R�s�[�R���X�g���N�^
		 */
		RoadEdgeData(const RoadEdgeData& x) { *this = x; }

		/*!
		 * ������Z�q
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
		std::vector<CVector3D>  vecPolyline;            //!< �|�����C�����_��
		DMCode                  nDMCode;                //!< DM�R�[�h(�啪��)
		DMRoadFacilitiesCode    nRoadFacilitiesCode;    //!< DM�R�[�h(������)
		DMGeometryType          nGeometryType;          //!< �}�`�敪

		/*!
		 * �R���X�g���N�^
		 */
		RoadFacilitiesData()
		{
			nDMCode = DMCode::UNCATEGORIZED;
			nRoadFacilitiesCode = DMRoadFacilitiesCode::UNCATEGORIZED_ROAD_FACILITIES;
			nGeometryType = DMGeometryType::UNCLASSIFIED;
		}

		/*!
		 * �f�X�g���N�^
		 */
		virtual ~RoadFacilitiesData() {}

		/*!
		 * �R�s�[�R���X�g���N�^
		 */
		RoadFacilitiesData(const RoadFacilitiesData& x) { *this = x; }

		/*!
		 * ������Z�q
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

	CDMDataManager(void) {}     //!< �R���X�g���N�^
	~CDMDataManager(void) {}    //!< �f�X�g���N�^
	bool ReadRoadPolygonShapeFile(std::vector<AttributeDataManager>& codeDataVec);   // ���H��shape�ǂݍ���
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
	// ���������l�f�[�^�z��}�b�v
	std::map<std::string, std::vector<int>> m_TranRoadIntegerCodeDataMap;
	std::map<std::string, std::vector<int>> m_UroRoadStructureCodeDataMap;
	std::map<std::string, std::vector<int>> m_UroTrafficIntegerCodeDataMap;
	std::map<std::string, std::vector<int>> m_UroTransportationDataQualityCodeDataMap;
	std::map<std::string, std::vector<int>> m_UroDmCodeDataMap;

	// ���������l�f�[�^�z��}�b�v
	std::map<std::string, std::vector<double>> m_UroTrafficDoubleCodeDataMap;

	// �����񑮐��l�f�[�^�z��}�b�v
	std::map<std::string, std::vector<std::string>> m_TranRoadStringCodeDataMap;
	std::map<std::string, std::vector<std::string>> m_UroTrafficStringCodeDataMap;

	std::string UTF8toSJIS(const char* bufUTF8);
	std::string AttributeClassNameToString(AttributeClassName className);
};
