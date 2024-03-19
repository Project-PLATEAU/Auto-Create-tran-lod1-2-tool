#include "pch.h"
#include "CDMDataManager.h"
#include "shapefil.h"
#include "StringEx.h"
#include "CFileIO.h"
#include "CGeoUtil.h"
#include "CShapeManager.h"
#include "CReadParamFile.h"
#include "ReadAttributeFile.h"


/*!
 * @brief     ���H�|���S��shape�t�@�C���ǂݍ���
 *
 * @return    ��������
 * @retval    true    ����
 * @retval    false   ���s
*/
bool CDMDataManager::ReadRoadPolygonShapeFile(std::vector<AttributeDataManager>& codeDataVec)
{
	// �ݒ�t�@�C����񂩂�SHP�p�X�ƕ��ރR�[�h���������擾
	std::string strShpPath = GetCreateParam()->GetRoadSHPPath();
	std::vector<std::pair<std::string, std::string>> tranRoadIntegerAttrNames;
	std::vector<std::pair<std::string, std::string>> tranRoadStringAttrNames;
	std::vector<std::pair<std::string, std::string>> uroRoadStructureAttrNames;
	std::vector<std::pair<std::string, std::string>> uroTrafficIntegerVolumeAttrNames;
	std::vector<std::pair<std::string, std::string>> uroTrafficDoubleVolumeAttrNames;
	std::vector<std::pair<std::string, std::string>> uroTrafficStringVolumeAttrNames;
	std::vector<std::pair<std::string, std::string>> uroTransportationDataQualityAttrNames;
	std::vector<std::pair<std::string, std::string>> uroTranDmAttrNames;
	GetAttributeInstance()->GetTranRoad()->GetTranRoadIntegerAttributes(tranRoadIntegerAttrNames);
	GetAttributeInstance()->GetTranRoad()->GetTranRoadStringAttributes(tranRoadStringAttrNames);
	GetAttributeInstance()->GetUroRoadStructureAttribute()->GetUroRoadStructureAttributes(uroRoadStructureAttrNames);
	GetAttributeInstance()->GetUroTrafficVolumeAttribute()->GetUroTrafficIntegerVolumeAttributes(uroTrafficIntegerVolumeAttrNames);
	GetAttributeInstance()->GetUroTrafficVolumeAttribute()->GetUroTrafficDoubleVolumeAttributes(uroTrafficDoubleVolumeAttrNames);
	GetAttributeInstance()->GetUroTrafficVolumeAttribute()->GetUroTrafficStringVolumeAttributes(uroTrafficStringVolumeAttrNames);
	GetAttributeInstance()->GetUroTransportationDataQualityAttribute()->GetUroTransportationDataQualityAttributes(uroTransportationDataQualityAttrNames);
	GetAttributeInstance()->GetUroDmAttribute()->GetUroDmAttributes(uroTranDmAttrNames);
	std::map<AttributeClassName, std::vector<std::pair<std::string, std::string>>> attrNamesMap;
	attrNamesMap[AttributeClassName::TRAN_ROAD_INTEGER] = tranRoadIntegerAttrNames;
	attrNamesMap[AttributeClassName::TRAN_ROAD_STRING] = tranRoadStringAttrNames;
	attrNamesMap[AttributeClassName::URO_ROAD_STRUCTURE] = uroRoadStructureAttrNames;
	attrNamesMap[AttributeClassName::URO_TRAFFIC_INTEGER] = uroTrafficIntegerVolumeAttrNames;
	attrNamesMap[AttributeClassName::URO_TRAFFIC_DOUBLE] = uroTrafficDoubleVolumeAttrNames;
	attrNamesMap[AttributeClassName::URO_TRAFFIC_STRING] = uroTrafficStringVolumeAttrNames;
	attrNamesMap[AttributeClassName::URO_TRANSPORTATION_DATA_QUALITY] = uroTransportationDataQualityAttrNames;
	attrNamesMap[AttributeClassName::URO_DM] = uroTranDmAttrNames;

	// open shape file
	CShapeManager shpMng = CShapeManager();
	if (!shpMng.Open(strShpPath))
	{
		return false;
	}

	// type�m�F
	if (shpMng.nShapeType != SHPT_POLYGON
		&& shpMng.nShapeType != SHPT_POLYGONZ
		&& shpMng.nShapeType != SHPT_POLYGONM
		&& shpMng.nShapeType != SHPT_MULTIPATCH
		)
	{
		shpMng.Close();
		std::cout << "NOT POLYGON file " << std::endl;
		return false;
	}

	int id = 0;
	for (int index = 0; index < shpMng.nEntities; index++)
	{
		AttributeDataManager attrDataMng(id++);
		if (shpMng.nShapeType == SHPT_MULTIPATCH)
		{
			int parts = shpMng.psShapeVec[index]->nParts;
			// ���t���|���S���������ꍇ�ǂݔ�΂�
			if (parts >= 2)
			{
				id--;
				continue;
			}
		}
		for (auto& attrNames : attrNamesMap)
		{
			for (std::pair<std::string, std::string> attrName : attrNames.second)
			{
				// �擾������field index
				int nCodeIndex = -1;
				char pszTitle[20];
				int nWidth, nDecimals;
				for (int i = 0; i < shpMng.nField; i++)
				{
					DBFFieldType eType = DBFGetFieldInfo(shpMng.hDBF, i, pszTitle, &nWidth, &nDecimals);
					std::string strTitle(pszTitle);
					strTitle = UTF8toSJIS(strTitle.c_str());
					if (strTitle == attrName.second)
					{
						nCodeIndex = i; // DM Code
					}
				}

				// field index�̎擾�m�F
				if (nCodeIndex < 0)
				{
					continue;
				}

				// �f�[�^�擾
				SHPObject* psElem;
				std::pair<std::string, int> integerCodeData;				//!< ���������l�f�[�^�z��
				std::pair<std::string, double> doubleCodeData;				//!< ���������l�f�[�^�z��
				std::pair<std::string, std::string> stringCodeData;			//!< �����񑮐��l�f�[�^�z��
				int iCode;
				double dCode;
				std::string sCode;
				psElem = SHPReadObject(shpMng.hSHP, index);
				if (psElem)
				{
					switch (attrNames.first)
					{
						// �����l�������̏ꍇ
					case AttributeClassName::TRAN_ROAD_INTEGER:
					case AttributeClassName::URO_ROAD_STRUCTURE:
					case AttributeClassName::URO_TRAFFIC_INTEGER:
					case AttributeClassName::URO_TRANSPORTATION_DATA_QUALITY:
					case AttributeClassName::URO_DM:
						// �����擾
						iCode = DBFReadIntegerAttribute(shpMng.hDBF, index, nCodeIndex);
						integerCodeData = std::make_pair(attrName.first, iCode);
						attrDataMng.attrDataVec.push_back(AttributeData(AttributeClassNameToString(attrNames.first), attrName.first, iCode));
						break;

						// �����l�������̏ꍇ
					case AttributeClassName::URO_TRAFFIC_DOUBLE:
						dCode = DBFReadDoubleAttribute(shpMng.hDBF, index, nCodeIndex);
						doubleCodeData = std::make_pair(attrName.first, dCode);
						attrDataMng.attrDataVec.push_back(AttributeData(AttributeClassNameToString(attrNames.first), attrName.first, dCode));
						break;

						// �����l��������̏ꍇ
					case AttributeClassName::TRAN_ROAD_STRING:
					case AttributeClassName::URO_TRAFFIC_STRING:
						sCode = DBFReadStringAttribute(shpMng.hDBF, index, nCodeIndex);
						stringCodeData = std::make_pair(attrName.first, sCode);
						attrDataMng.attrDataVec.push_back(AttributeData(AttributeClassNameToString(attrNames.first), attrName.first, sCode));
						break;

					default:
						std::cout << "�s���ȑ������ł��B" << std::endl;
						break;
					}
				}
				SHPDestroyObject(psElem);
			}
		}
		//1�񂲂ƂɊi�[
		codeDataVec.push_back(attrDataMng);
	}

	return true;
}

std::map<std::string, std::vector<int>> CDMDataManager::GetTranRoadIntegerCodeData()
{
	return m_TranRoadIntegerCodeDataMap;
}

std::map<std::string, std::vector<int>> CDMDataManager::GetUroRoadStructureCodeData()
{
	return m_UroRoadStructureCodeDataMap;
}

std::map<std::string, std::vector<int>> CDMDataManager::GetUroTrafficIntegerCodeData()
{
	return m_UroTrafficIntegerCodeDataMap;
}

std::map<std::string, std::vector<int>> CDMDataManager::GetUroTransportationDataQualityCodeData()
{
	return m_UroTransportationDataQualityCodeDataMap;
}

std::map<std::string, std::vector<int>> CDMDataManager::GetUroDmCodeData()
{
	return m_UroDmCodeDataMap;
}

std::map<std::string, std::vector<double>> CDMDataManager::GetUroTrafficDoubleCodeData()
{
	return m_UroTrafficDoubleCodeDataMap;
}

std::map<std::string, std::vector<std::string>> CDMDataManager::GetTranRoadStringCodeData()
{
	return m_TranRoadStringCodeDataMap;
}

std::map<std::string, std::vector<std::string>> CDMDataManager::GetUroTrafficStringCodeData()
{
	return m_UroTrafficStringCodeDataMap;
}


std::string CDMDataManager::UTF8toSJIS(const char* bufUTF8)
{
#define	BUFSIZE	10000
	wchar_t bufUnicode[BUFSIZE];
	char	szSJIS[BUFSIZE];

	// �܂�Uniocde�ɕϊ�����
	// �T�C�Y���v�Z����
	int iLenUnicode = MultiByteToWideChar(CP_UTF8, 0, bufUTF8, strlen(bufUTF8) + 1, NULL, 0);

	if (iLenUnicode <= sizeof(bufUnicode) / sizeof(bufUnicode[0]))
	{
		MultiByteToWideChar(CP_UTF8, 0, bufUTF8, strlen(bufUTF8) + 1, bufUnicode, BUFSIZE);
		// ���ɁAUniocde����ShiftJis�ɕϊ�����
		// �T�C�Y���v�Z����
		int iLenUtf8 = WideCharToMultiByte(CP_ACP, 0, bufUnicode, iLenUnicode, NULL, 0, NULL, NULL);
		{
			WideCharToMultiByte(CP_ACP, 0, bufUnicode, iLenUnicode, szSJIS, BUFSIZE, NULL, NULL);
		}
	}
	return	std::string(szSJIS);

}

std::string CDMDataManager::AttributeClassNameToString(AttributeClassName className)
{
	switch (className)
	{
	case CDMDataManager::AttributeClassName::TRAN_ROAD_INTEGER:
	case CDMDataManager::AttributeClassName::TRAN_ROAD_STRING:
		return "tran:Road";
		break;
	case CDMDataManager::AttributeClassName::URO_ROAD_STRUCTURE:
		return "uro:RoadStructureAttribute";
		break;
	case CDMDataManager::AttributeClassName::URO_TRAFFIC_INTEGER:
	case CDMDataManager::AttributeClassName::URO_TRAFFIC_DOUBLE:
	case CDMDataManager::AttributeClassName::URO_TRAFFIC_STRING:
		return "uro:TrafficVolumeAttribute";
		break;
	case CDMDataManager::AttributeClassName::URO_TRANSPORTATION_DATA_QUALITY:
		return "uro:TransportationDataQualityAttribute";
		break;
	case CDMDataManager::AttributeClassName::URO_DM:
		return "uro:DmAttribute";
		break;
	default:
		break;
	}

	return "";
}
