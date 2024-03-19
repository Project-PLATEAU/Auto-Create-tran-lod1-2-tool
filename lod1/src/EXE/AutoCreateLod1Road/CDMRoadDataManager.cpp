#include "pch.h"
#include "CDMRoadDataManager.h"
#include "shapefil.h"
#include "StringEx.h"
#include "CFileIO.h"
#include "CGeoUtil.h"
#include "CShapeManager.h"
#include "CReadParamFile.h"

/*!
 * @brief     ���H��shape�t�@�C���ǂݍ���
 *
 * @return    ��������
 * @retval    true    ����
 * @retval    false   ���s
*/
bool CDMRoadDataManager::ReadRoadEdgeShapeFile(void)
{
    // �ݒ�t�@�C����񂩂�SHP�p�X�ƕ��ރR�[�h���������擾
    std::string strShpPath = GetCreateParam()->GetRoadSHPPath();
    std::string strCodeAttrName = GetCreateParam()->GetDMCodeAttribute();

    // open shape file
    CShapeManager shpMng = CShapeManager();
    if (!shpMng.Open(strShpPath))
    {
        return false;
    }

    // type�m�F
    if (shpMng.nShapeType != SHPT_ARC
        && shpMng.nShapeType != SHPT_ARCZ
        && shpMng.nShapeType != SHPT_ARCM)
    {
        shpMng.Close();
        return false;
    }

    // �擾������field index
    int nCodeIndex = -1;
    char pszTitle[20];
    int nWidth, nDecimals;
    for (int i = 0; i < shpMng.nField; i++)
    {
        DBFFieldType eType = DBFGetFieldInfo(shpMng.hDBF, i, pszTitle, &nWidth, &nDecimals);
        std::string strTitle(pszTitle);
        if (eType == DBFFieldType::FTInteger)
        {
            if (strTitle == strCodeAttrName)
            {
                nCodeIndex = i; // DM Code
            }
        }
    }
    // field index�̎擾�m�F
    if (nCodeIndex < 0)
    {
        shpMng.Close();
        return false;
    }

    // �f�[�^�擾
    SHPObject* psElem;
    for (int i = 0; i < shpMng.nEntities; i++)
    {
        psElem = SHPReadObject(shpMng.hSHP, i);
        if (psElem)
        {
            // �����擾
            int nCode = DBFReadIntegerAttribute(shpMng.hDBF, i, nCodeIndex);    // DM Code

            // DM Code����2��(�啪��)�A��2��(������)�ɕ���
            int nDMCodeUpper, nDMCodeLower;
            separateDMCode(nCode, nDMCodeUpper, nDMCodeLower);

            // �p�[�g����
            std::vector<std::vector<CVector3D>> vecParts;
            if (separateParts(psElem, vecParts) > 0)
            {
                for (std::vector<std::vector<CVector3D>>::iterator it = vecParts.begin();
                    it != vecParts.end(); it++)
                {
                    RoadEdgeData roadEdge = RoadEdgeData();
                    roadEdge.nDMCode = static_cast<DMCode>(nDMCodeUpper);
                    roadEdge.nRoadCode = static_cast<DMRoadCode>(nDMCodeLower);
                    std::copy(it->begin(), it->end(), std::back_inserter(roadEdge.vecPolyline));
                    m_vecRoadEdgeData.push_back(roadEdge);
                }
            }
        }
        SHPDestroyObject(psElem);
    }

    // close shape file
    shpMng.Close();
    return true;
}

/*!
 * @brief   ���H�{��shape�t�@�C���ǂݍ���
 * @note    �{�ݏ��Ȃ��̏ꍇ������I��
*/
void CDMRoadDataManager::ReadRoadFacilitiesShapeFile(void)
{
    // �ݒ�t�@�C����񂩂番�ރR�[�h���������擾
    std::vector<RoadFacilitiesDataType> types;
    types.push_back(RoadFacilitiesDataType::ROAD_FACILITIES_LINE_DATA);
    types.push_back(RoadFacilitiesDataType::ROAD_FACILITIES_POINT_DATA);
    types.push_back(RoadFacilitiesDataType::ROAD_FACILITIES_POLYGON_DATA);

    m_vecRoadFacilitiesData.clear();
    for (std::vector<RoadFacilitiesDataType>::const_iterator itType = types.cbegin();
        itType != types.cend(); itType++)
    {
        // ���H�{��shp�p�X
        std::string strShpPath;
        if (*itType == RoadFacilitiesDataType::ROAD_FACILITIES_LINE_DATA)
            strShpPath = GetCreateParam()->GetRoadFacilitiesLineSHPPath();

        else if (*itType == RoadFacilitiesDataType::ROAD_FACILITIES_POINT_DATA)
            strShpPath = GetCreateParam()->GetRoadFacilitiesPointSHPPath();

        else if (*itType == RoadFacilitiesDataType::ROAD_FACILITIES_POLYGON_DATA)
            strShpPath = GetCreateParam()->GetRoadFacilitiesPolygonSHPPath();

        if (!strShpPath.empty())
            readRoadFacilitiesShapeFile(strShpPath, *itType);
    }
}

/*!
 * @brief DM�R�[�h����
 * @param[in]   nCode   DM�R�[�h
 * @param[out]  nUpper  DM�R�[�h(��2��)
 * @param[out]  nLower  DM�R�[�h(��2��)
 * @return
*/
void CDMRoadDataManager::separateDMCode(int nCode, int &nUpper, int &nLower)
{
    nUpper = nCode / 100;
    nLower = nCode % 100;
}

/*!
 * @brief DM�R�[�h����
 * @param nUpper DM�R�[�h(��2��)
 * @param nLower DM�R�[�h(��2��)
 * @return DM�R�[�h
*/
int CDMRoadDataManager::combineDMCode(int nUpper, int nLower)
{
    return nUpper * 100 + nLower;
}

/*!
 * @brief ���x�敪�R�[�h����
 * @param[in]   nCode   DM�R�[�h
 * @param[out]  nUpper  DM�R�[�h(��2��)
 * @param[out]  nLower  DM�R�[�h(��2��)
 * @return
*/
void CDMRoadDataManager::separateAccuracyCode(int nCode, int& nUpper, int& nLower)
{
    nUpper = nCode / 10;
    nLower = nCode % 10;
}

/*!
 * @brief �p�[�g����
 * @param[in]   psElem      1���R�[�h����shape�f�[�^�̃|�C���^
 * @param[out]  vecParts    �p�[�g������̍��W�f�[�^�z��
 * @return ������̃p�[�g�f�[�^��
*/
int CDMRoadDataManager::separateParts(
    SHPObject* psElem,
    std::vector<std::vector<CVector3D>>& vecParts)
{
    vecParts.clear();

    if (psElem)
    {
        // �p�[�g����
        for (int nPart = 0; nPart < psElem->nParts; nPart++)
        {
            int nStart = psElem->panPartStart[nPart];
            int nEnd = psElem->nVertices;   // �����l:���_��
            if (nPart < psElem->nParts - 1)
            {
                nEnd = psElem->panPartStart[nPart + 1]; //���p�[�g�̊J�n�C���f�b�N�X
            }

            std::vector<CVector3D> vecPoints;
            // ���_��̎擾
            for (int nPt = nStart; nPt < nEnd; nPt++)
            {
                // ���ʒ��p���W�̑z��
                CVector3D pt(psElem->padfX[nPt], psElem->padfY[nPt], psElem->padfZ[nPt]);
                vecPoints.push_back(pt);
            }
            if (vecPoints.size() > 0)
            {
                vecParts.push_back(vecPoints);
            }
        }
    }

    return (int)(vecParts.size());
}

/*!
 * @brief ���H�{��shape�t�@�C���ǂݍ���(�����֐�)
 * @param[in] strShpPath    shape�t�@�C���p�X
 * @param[in] type          �ǂݍ���shape�t�@�C���̌`��^�C�v
 * @return    ��������
 * @retval    true    ����
 * @retval    false   ���s
*/
bool CDMRoadDataManager::readRoadFacilitiesShapeFile(const std::string &strShpPath, RoadFacilitiesDataType type)
{
    // �ݒ�t�@�C����񂩂�SHP�p�X�ƕ��ރR�[�h���������擾
    std::string strCodeAttrName = GetCreateParam()->GetDMCodeAttribute();
    std::string strGeometryTypeAttrName = GetCreateParam()->GetGeometryTypeAttribute();

    // �f�[�^�I�ʗp��DM�R�[�h(���H��(���˕�), ���H�̃g���l��)
    const int ROAD_BRIDGE_CODE = combineDMCode(
        static_cast<int>(DMCode::TRANSPORTATION_FACILITIES_ROAD_FACIRITIES),
        static_cast<int>(DMRoadFacilitiesCode::ROAD_BRIDGE));
    const int TUNNEL_CODE = combineDMCode(
        static_cast<int>(DMCode::TRANSPORTATION_FACILITIES_ROAD_FACIRITIES),
        static_cast<int>(DMRoadFacilitiesCode::ROAD_TUNNELS));

    // open shape file
    CShapeManager shpMng = CShapeManager();
    if (!shpMng.Open(strShpPath))
    {
        return false;
    }

    // type�m�F
    bool bErr = false;
    if (type == RoadFacilitiesDataType::ROAD_FACILITIES_POINT_DATA)
    {
        if (shpMng.nShapeType != SHPT_POINT
            && shpMng.nShapeType != SHPT_POINTZ
            && shpMng.nShapeType != SHPT_POINTM)
            bErr = true;
    }
    else if (type == RoadFacilitiesDataType::ROAD_FACILITIES_LINE_DATA)
    {
        if (shpMng.nShapeType != SHPT_ARC
            && shpMng.nShapeType != SHPT_ARCZ
            && shpMng.nShapeType != SHPT_ARCM)
            bErr = true;
    }
    else if (type == RoadFacilitiesDataType::ROAD_FACILITIES_POLYGON_DATA)
    {
        if (shpMng.nShapeType != SHPT_POLYGON
            && shpMng.nShapeType != SHPT_POLYGONZ
            && shpMng.nShapeType != SHPT_POLYGONM)
            bErr = true;
    }
    else
    {
        bErr = true;
    }
    if (bErr)
    {
        shpMng.Close();
        return false;
    }

    // �擾������field index
    int nCodeIndex = -1;
    int nGeometryTypeIndex = -1;
    char pszTitle[20];
    int nWidth, nDecimals;
    for (int i = 0; i < shpMng.nField; i++)
    {
        DBFFieldType eType = DBFGetFieldInfo(shpMng.hDBF, i, pszTitle, &nWidth, &nDecimals);
        std::string strTitle(pszTitle);
        if (eType == DBFFieldType::FTInteger)
        {
            if (strTitle == strCodeAttrName)
            {
                nCodeIndex = i; // DM Code
            }
            else if (strTitle == strGeometryTypeAttrName)
            {
                nGeometryTypeIndex = i; // �}�`�敪
            }
        }
    }
    // field index�̎擾�m�F
    if (nCodeIndex < 0 || nGeometryTypeIndex < 0)
    {
        shpMng.Close();
        return false;
    }

    // �f�[�^�擾
    SHPObject *psElem;
    for (int i = 0; i < shpMng.nEntities; i++)
    {
        psElem = SHPReadObject(shpMng.hSHP, i);
        if (psElem)
        {
            // �����擾
            int nCode = DBFReadIntegerAttribute(shpMng.hDBF, i, nCodeIndex);            // DM Code
            int nGeoType = DBFReadIntegerAttribute(shpMng.hDBF, i, nGeometryTypeIndex); // �}�`�敪

            // �f�[�^�I��
            if (nCode == ROAD_BRIDGE_CODE || nCode == TUNNEL_CODE)
            {
                // ���H��(����)�܂��� ���H�̃g���l���̏ꍇ
                // DM Code����2��(�啪��)�A��2��(������)�ɕ���
                int nDMCodeUpper, nDMCodeLower;
                separateDMCode(nCode, nDMCodeUpper, nDMCodeLower);

                if (type == RoadFacilitiesDataType::ROAD_FACILITIES_LINE_DATA
                    || type == RoadFacilitiesDataType::ROAD_FACILITIES_POLYGON_DATA)
                {
                    // �p�[�g����
                    std::vector<std::vector<CVector3D>> vecParts;
                    if (separateParts(psElem, vecParts) > 0)
                    {
                        for (std::vector<std::vector<CVector3D>>::iterator it = vecParts.begin();
                            it != vecParts.end(); it++)
                        {
                            RoadFacilitiesData roadFacility = RoadFacilitiesData();
                            roadFacility.nDMCode = static_cast<DMCode>(nDMCodeUpper);
                            roadFacility.nRoadFacilitiesCode = static_cast<DMRoadFacilitiesCode>(nDMCodeLower);
                            roadFacility.nGeometryType = static_cast<DMGeometryType>(nGeoType);
                            roadFacility.nRoadFacilitiesDataType = type;
                            std::copy(it->begin(), it->end(), std::back_inserter(roadFacility.vecPolyline));
                            m_vecRoadFacilitiesData.push_back(roadFacility);
                        }
                    }
                }
                else
                {
                    if (psElem->nVertices > 0)
                    {
                        RoadFacilitiesData roadFacility = RoadFacilitiesData();
                        roadFacility.nDMCode = static_cast<DMCode>(nDMCodeUpper);
                        roadFacility.nRoadFacilitiesCode = static_cast<DMRoadFacilitiesCode>(nDMCodeLower);
                        roadFacility.nGeometryType = static_cast<DMGeometryType>(nGeoType);
                        roadFacility.nRoadFacilitiesDataType = type;
                        CVector3D pt(psElem->padfX[0], psElem->padfY[0], psElem->padfZ[0]);
                        roadFacility.vecPolyline.push_back(pt);
                        m_vecRoadFacilitiesData.push_back(roadFacility);
                    }

                }

            }
        }
        SHPDestroyObject(psElem);
    }

    // close shape file
    shpMng.Close();
    return true;
}