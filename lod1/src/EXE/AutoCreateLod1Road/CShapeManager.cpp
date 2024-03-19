#include "pch.h"
#include "CShapeManager.h"
#include "CFileUtil.h"

/*!
 * @brief �R���X�g���N�^
*/
CShapeManager::CShapeManager(void)
{
    // �n���h��
    hSHP = NULL;
    hDBF = NULL;

    nShapeType = SHPT_NULL; // ���
    nEntities = 0;          // �v�f��
    nField = 0;             // field��
}

/*!
 * @brief �f�X�g���N�^
*/
CShapeManager::~CShapeManager(void)
{
    Close();
}

/*!
 * @brief   open shape file
 * @param   strShpPath  shp�t�@�C���p�X
 * @return  ��������
 * @retval  true        ����
 * @retval  false       ���s
 * @note    shp�t�@�C���Ɠ��K�w��dbf�t�@�C�������݂��邱��
*/
bool CShapeManager::Open(std::string strShpPath)
{
    std::string strDbfPath = CFileUtil::ChangeFileNameExt(strShpPath, ".dbf");

    if (!GetFUtil()->IsExistPath(strShpPath))
    {
        return false;
    }
    if (!GetFUtil()->IsExistPath(strDbfPath))
    {
        return false;
    }

    // open shape file
    hSHP = SHPOpen(strShpPath.c_str(), "r");
    if (hSHP == NULL)
    {
        return false;
    }
    // open dbf file
    hDBF = DBFOpen(strDbfPath.c_str(), "rb");
    if (hDBF == NULL)
    {
        SHPClose(hSHP);
        return false;
    }

    // �v�f���A��ʁA�o�E���f�B���O���̎擾
    SHPGetInfo(hSHP, &nEntities, &nShapeType, adfMinBound, adfMaxBound);

    // �v�f���̊m�F
    if (nEntities <= 0)
    {
        Close();
        return false;
    }

    // field��
    nField = DBFGetFieldCount(hDBF);

    return true;
}

/*!
 * @brief   open shape file without dbf file.
 * @param   strShpPath  shp�t�@�C���p�X
 * @return  ��������
 * @retval  true        ����
 * @retval  false       ���s
*/
bool CShapeManager::OpenWithoutDbf(std::string strShpPath)
{
    if (!GetFUtil()->IsExistPath(strShpPath))
    {
        return false;
    }

    // open shape file
    hSHP = SHPOpen(strShpPath.c_str(), "r");
    if (hSHP == NULL)
    {
        return false;
    }

    // �v�f���A��ʁA�o�E���f�B���O���̎擾
    SHPGetInfo(hSHP, &nEntities, &nShapeType, adfMinBound, adfMaxBound);

    // �v�f���̊m�F
    if (nEntities <= 0)
    {
        Close();
        return false;
    }

    // field��
    nField = 0;

    return true;
}

/*!
 * @brief close shape file
 */
void CShapeManager::Close(void)
{
    if (hSHP != NULL)
    {
        SHPClose(hSHP);
        hSHP = NULL;
    }
    if (hDBF != NULL)
    {
        DBFClose(hDBF);
        hDBF = NULL;
    }
}

/*!
 * @brief �|���S����shape file�o��
 * @param[in] polygons          �|���S���Q
 * @param[in] strShpPath        �o�� shape file �p�X
 * @param[in] vecFields         �����t�B�[���h��`�f�[�^
 * @param[in] vecAttrRecords    �����l�f�[�^
 * @param[in] bHole             ���̏����o���t���O
 * @return  ��������
 * @retval  true        ����
 * @retval  false       ���s
*/
bool CShapeWriter::OutputPolygons(
    const BoostMultiPolygon &polygons,
    std::string strShpPath,
    const std::vector<CShapeAttribute::AttributeFieldData> &vecFields,
    const std::vector<CShapeAttribute::AttributeDataRecord> &vecAttrRecords,
    const bool bHole)
{
    const double dZ = 0;
    int nShpType = SHPT_POLYGON;    // ������
    if (bHole)
        nShpType = SHPT_MULTIPATCH; // ���L��

    // create shape file
    SHPHandle hSHP = SHPCreate(strShpPath.c_str(), nShpType);
    if (hSHP == NULL)
    {
        return false;
    }

    int nShapeId = 0;
    for (BoostMultiPolygon::const_iterator itPoly = polygons.cbegin();
        itPoly != polygons.cend(); itPoly++, nShapeId++)
    {
        // 1���̃|���S���̏�������

        if (bHole)
        {
            // ���L��
            // ���_���ƃp�[�c�擪�C���f�b�N�X
            int nPtNum = static_cast<int>(itPoly->outer().size());
            std::vector<int> vecPartStartIdx;
            vecPartStartIdx.push_back(0);
            for (auto itInner = itPoly->inners().cbegin();
                itInner != itPoly->inners().cend(); itInner++)
            {
                vecPartStartIdx.push_back(nPtNum);
                nPtNum += static_cast<int>(itInner->size());
            }

            // ���_�z��̊i�[
            double *pX, *pY, *pZ;
            pX = new double[nPtNum];
            pY = new double[nPtNum];
            pZ = new double[nPtNum];
            // �O�֊s
            int idx = 0;
            for (; idx < static_cast<int>(itPoly->outer().size()); idx++)
            {
                pX[idx] = (itPoly->outer().data() + idx)->x();
                pY[idx] = (itPoly->outer().data() + idx)->y();
                pZ[idx] = dZ;
            }
            // ��
            for (auto itInner = itPoly->inners().cbegin();
                itInner != itPoly->inners().cend(); itInner++)
            {
                for (int tmpIdx = 0; tmpIdx < static_cast<int>(itInner->size()); tmpIdx++, idx++)
                {
                    pX[idx] = (itInner->data() + tmpIdx)->x();
                    pY[idx] = (itInner->data() + tmpIdx)->y();
                    pZ[idx] = dZ;
                }
            }

            // �p�[�c�擪�C���f�b�N�X�z��ƃp�[�c�^�C�v�̊i�[
            int nParts = static_cast<int>(vecPartStartIdx.size()); // �p�[�c��
            int *pPartStart, *pPartType;
            pPartStart = new int[nParts];
            pPartType = new int[nParts];
            for (int i = 0; i < nParts; i++)
            {
                pPartStart[i] = vecPartStartIdx[i];
                if (i == 0)
                    pPartType[i] = SHPP_OUTERRING;
                else
                    pPartType[i] = SHPP_INNERRING;
            }

            SHPObject *pShpObj = SHPCreateObject(
                SHPT_MULTIPATCH, nShapeId, nParts, pPartStart, pPartType, nPtNum, pX, pY, pZ, NULL);
            delete[] pPartStart;
            delete[] pPartType;
            SHPWriteObject(hSHP, -1, pShpObj);
            SHPDestroyObject(pShpObj);

            delete[] pX;
            delete[] pY;
            delete[] pZ;
        }
        else
        {
            // ������
            int nPtNum = static_cast<int>(itPoly->outer().size());
            double *pX, *pY, *pZ;
            pX = new double[nPtNum];
            pY = new double[nPtNum];
            pZ = new double[nPtNum];

            for (int idx = 0; idx < nPtNum; idx++)
            {
                pX[idx] = (itPoly->outer().data() + idx)->x();
                pY[idx] = (itPoly->outer().data() + idx)->y();
                pZ[idx] = dZ;
            }
            SHPObject *pShpObj = SHPCreateSimpleObject(nShpType, nPtNum, pX, pY, pZ);
            SHPWriteObject(hSHP, -1, pShpObj);
            SHPDestroyObject(pShpObj);

            delete[] pX;
            delete[] pY;
            delete[] pZ;
        }
    }

    SHPClose(hSHP);

    // �������̏�������
    return writeAttribute(strShpPath, vecFields, vecAttrRecords, polygons.size());
}


/*!
 * @brief �|�����C����shape file �o��
 * @param[in] polylines         �|�����C���W��
 * @param[in] strShpPath        shp�t�@�C���p�X
 * @param[in] vecFields         �����t�B�[���h��`�f�[�^
 * @param[in] vecAttrRecords    �����l�f�[�^
 * @return  ��������
 * @retval  true    ����
 * @retval  false   ���s
 */
bool CShapeWriter::OutputPolylines(
    const BoostMultiLines &polylines,
    std::string strShpPath,
    const std::vector<CShapeAttribute::AttributeFieldData> &vecFields,
    const std::vector<CShapeAttribute::AttributeDataRecord> &vecAttrRecords)
{
    const double dZ = 0;

    // create shape file
    SHPHandle hSHP = SHPCreate(strShpPath.c_str(), SHPT_ARC);
    if (hSHP == NULL)
    {
        return false;
    }

    for (BoostMultiLines::const_iterator itLine = polylines.cbegin();
        itLine != polylines.cend(); itLine++)
    {
        // 1�{���̃|�����C���̏�������
        int nPtNum = static_cast<int>(itLine->size());
        double *pX, *pY, *pZ;
        pX = new double[nPtNum];
        pY = new double[nPtNum];
        pZ = new double[nPtNum];

        for (int idx = 0; idx < nPtNum; idx++)
        {
            pX[idx] = (itLine->data() + idx)->x();
            pY[idx] = (itLine->data() + idx)->y();
            pZ[idx] = dZ;
        }
        SHPObject *pShpObj = SHPCreateSimpleObject(SHPT_ARC, nPtNum, pX, pY, pZ);
        SHPWriteObject(hSHP, -1, pShpObj);
        SHPDestroyObject(pShpObj);

        delete[] pX;
        delete[] pY;
        delete[] pZ;
    }

    SHPClose(hSHP);

    // �������̏�������
    return writeAttribute(strShpPath, vecFields, vecAttrRecords, polylines.size());
}

/*!
 * @brief �_��shape file�o��
 * @param[in] points            �_�Q
 * @param[in] strShpPath        shp�t�@�C���p�X
 * @param[in] vecFields         �����t�B�[���h��`�f�[�^
 * @param[in] vecAttrRecords    �����l�f�[�^
 * @return  ��������
 * @retval  true    ����
 * @retval  false   ���s
*/
bool CShapeWriter::OutputMultiPoints(
    const BoostMultiPoints &points,
    std::string strShpPath,
    const std::vector<CShapeAttribute::AttributeFieldData> &vecFields,
    const std::vector<CShapeAttribute::AttributeDataRecord> &vecAttrRecords)
{
    const double dZ = 0;

    // create shape file
    SHPHandle hSHP = SHPCreate(strShpPath.c_str(), SHPT_POINT);
    if (hSHP == NULL)
    {
        return false;
    }

    int nPtNum = static_cast<int>(points.size());

    for (int idx = 0; idx < nPtNum; idx++)
    {
        double dX = (points.data() + idx)->x();
        double dY = (points.data() + idx)->y();
        SHPObject *pShpObj = SHPCreateSimpleObject(SHPT_POINT, 1, &dX, &dY, &dZ);
        SHPWriteObject(hSHP, -1, pShpObj);
        SHPDestroyObject(pShpObj);
    }

    SHPClose(hSHP);

    // �������̏�������
    return writeAttribute(strShpPath, vecFields, vecAttrRecords, points.size());
}

/*!
 * @brief �������̏�������
 * @param[in] strShpPath        shape�t�@�C���p�X
 * @param[in] vecFields         �����t�B�[���h�Q
 * @param[in] vecAttrRecords    �������Q
 * @param[in] shapeNum          �`����
 * @return  ��������
 * @retval  true    ����
 * @retval  false   ���s
*/
bool CShapeWriter::writeAttribute(
    std::string strShpPath,
    const std::vector<CShapeAttribute::AttributeFieldData> &vecFields,
    const std::vector<CShapeAttribute::AttributeDataRecord> &vecAttrRecords,
    const size_t shapeNum)
{
    if (vecFields.size() > 0)
    {
        // create dbf file
        std::string strDbfPath = CFileUtil::ChangeFileNameExt(strShpPath, ".dbf");
        DBFHandle hDBF = DBFCreate(strDbfPath.c_str());
        if (hDBF == NULL)
        {
            return false;
        }

        for (std::vector<CShapeAttribute::AttributeFieldData>::const_iterator itField = vecFields.cbegin();
            itField != vecFields.cend(); itField++)
        {
            if (itField->fieldType == CShapeAttribute::AttributeFieldType::ATTR_FIELD_TYPE_INT)
            {
                // ����
                DBFAddField(hDBF, itField->strName.c_str(), DBFFieldType::FTInteger,
                    itField->nWidth, itField->nDecimals);
            }
            else if (itField->fieldType == CShapeAttribute::AttributeFieldType::ATTR_FIELD_TYPE_DOUBLE)
            {
                // ����
                DBFAddField(hDBF, itField->strName.c_str(), DBFFieldType::FTDouble,
                    itField->nWidth, itField->nDecimals);
            }
            else if (itField->fieldType == CShapeAttribute::AttributeFieldType::ATTR_FIELD_TYPE_STRING)
            {
                // ������
                DBFAddField(hDBF, itField->strName.c_str(), DBFFieldType::FTString,
                    itField->nWidth, itField->nDecimals);
            }
        }

        for (std::vector<CShapeAttribute::AttributeDataRecord>::const_iterator itAttr = vecAttrRecords.cbegin();
            itAttr != vecAttrRecords.cend(); itAttr++)
        {
            // 1���R�[�h���̑���
            if (vecFields.size() != itAttr->vecAttribute.size()
                || itAttr->nShapeId < 0 || itAttr->nShapeId >= shapeNum)
            {
                // ���������قȂ�
                // shape id���s���ȏꍇ��skip
                continue;
            }
            // �����l�̏�������
            int nAttrId = 0;
            for (std::vector<CShapeAttribute::AttributeData>::const_iterator itData = itAttr->vecAttribute.cbegin();
                itData != itAttr->vecAttribute.cend(); itData++, nAttrId++)
            {
                if (itData->dataType == CShapeAttribute::AttributeDataType::ATTR_DATA_TYPE_INT)
                {
                    // ����
                    DBFWriteIntegerAttribute(hDBF, itAttr->nShapeId, nAttrId, itData->nValue);
                }
                else if (itData->dataType == CShapeAttribute::AttributeDataType::ATTR_DATA_TYPE_DOUBLE)
                {
                    // ����
                    DBFWriteDoubleAttribute(hDBF, itAttr->nShapeId, nAttrId, itData->dValue);
                }
                else if (itData->dataType == CShapeAttribute::AttributeDataType::ATTR_DATA_TYPE_STRING)
                {
                    // ������
                    DBFWriteStringAttribute(hDBF, itAttr->nShapeId, nAttrId, itData->strValue.c_str());
                }
                else if (itData->dataType == CShapeAttribute::AttributeDataType::ATTR_DATA_TYPE_NULL)
                {
                    // NULL
                    DBFWriteNULLAttribute(hDBF, itAttr->nShapeId, nAttrId);
                }
            }
        }

        DBFClose(hDBF);
    }

    return true;
}

/*!
 * @brief �|���S����shape file�ǂݍ���
 * @param[in]   strShpPath      shape�t�@�C���p�X
 * @param[out]  polygons        �|���S���Q
 * @param[out]  vecFields       �����t�B�[���h�Q
 * @param[out]  vecAttrRecords  �������Q
 * @return      ��������
 * @retval      true    ����
 * @retval      false   ���s
*/
bool CShapeReader::ReadPolygons(
    const std::string strShpPath,
    BoostMultiPolygon &polygons,
    std::vector<CShapeAttribute::AttributeFieldData> &vecFields,
    std::vector<CShapeAttribute::AttributeDataRecord> &vecAttrRecords)
{
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
        && shpMng.nShapeType != SHPT_MULTIPATCH)
    {
        shpMng.Close();
        return false;
    }

    // �f�[�^�擾
    readPolygons(shpMng, polygons);

    // �����̎擾
    readAttribute(shpMng, vecFields, vecAttrRecords);

    // close shape file
    shpMng.Close();

    return true;
}

/*!
 * @brief
 * @param[in]   strShpPath  shape�t�@�C���p�X
 * @param[out]  polygons    �|���S���Q
 * @return      ��������
 * @retval      true        ����
 * @retval      false       ���s
 * @note        �|���S���f�[�^�݂̂��擾�������͖�������
*/
bool CShapeReader::ReadPolygons(
    const std::string &strShpPath,
    BoostMultiPolygon &polygons)
{
    polygons.clear();

    // open shape file
    CShapeManager shpMng = CShapeManager();
    if (!shpMng.OpenWithoutDbf(strShpPath))
    {
        return false;
    }

    // type�m�F
    if (shpMng.nShapeType != SHPT_POLYGON
        && shpMng.nShapeType != SHPT_POLYGONZ
        && shpMng.nShapeType != SHPT_POLYGONM
        && shpMng.nShapeType != SHPT_MULTIPATCH)
    {
        shpMng.Close();
        return false;
    }

    // �f�[�^�擾
    readPolygons(shpMng, polygons);

    // close shape file
    shpMng.Close();
    return true;
}

/*!
 * @brief �|�����C���ǂݍ���(�����f�[�^�͖���)
 * @param[in]   strShpPath      shape�t�@�C���p�X
 * @param[out]  popolylinesints �|�����C���R
 * @return      ��������
 * @retval      true        ����
 * @retval      false       ���s
*/
bool CShapeReader::ReadPolylines(
    const std::string &strShpPath,
    BoostMultiLines &polylines)
{
    polylines.clear();

    // open shape file
    CShapeManager shpMng = CShapeManager();
    if (!shpMng.OpenWithoutDbf(strShpPath))
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

    // �f�[�^�擾
    SHPObject *psElem;
    for (int i = 0; i < shpMng.nEntities; i++)
    {
        psElem = SHPReadObject(shpMng.hSHP, i);
        if (psElem)
        {
            // �p�[�g����
            std::vector<std::vector<BoostPoint>> vecParts;
            if (separateParts(psElem, vecParts) > 0)
            {
                for (std::vector<std::vector<BoostPoint>>::iterator it = vecParts.begin();
                    it != vecParts.end(); it++)
                {
                    BoostPolyline polyline;
                    std::copy(it->begin(), it->end(), std::back_inserter(polyline));
                    bg::correct(polyline);
                    if (bg::is_valid(polyline))
                        polylines.push_back(polyline);
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
 * @brief �}���`�|�C���g�ǂݍ���(�����f�[�^�͖���)
 * @param[in]   strShpPath  shape�t�@�C���p�X
 * @param[out]  points      ���_�Q
 * @return      ��������
 * @retval      true        ����
 * @retval      false       ���s
*/
bool CShapeReader::ReadPoints(const std::string &strShpPath, BoostMultiPoints &points)
{
    points.clear();

    // open shape file
    CShapeManager shpMng = CShapeManager();
    if (!shpMng.OpenWithoutDbf(strShpPath))
    {
        return false;
    }

    // type�m�F
    if (shpMng.nShapeType != SHPT_POINT)
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
            BoostPoint pt(psElem->padfX[0], psElem->padfY[0]);

            bg::correct(pt);
            if (bg::is_valid(pt) == true)
            {
                points.emplace_back(pt);
            }

        }
        SHPDestroyObject(psElem);
    }

    // close shape file
    shpMng.Close();
    return true;
}

/*!
 * @brief       �p�[�g����
 * @param[in]   psElem   1���R�[�h����shape�f�[�^�̃|�C���^
 * @param[out]  vecParts �p�[�g������̍��W�f�[�^�z��
 * @return      ������̃p�[�g�f�[�^��
*/
int CShapeReader::separateParts(
    SHPObject *psElem,
    std::vector<std::vector<BoostPoint>> &vecParts)
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

            std::vector<BoostPoint> vecPoints;
            // ���_��̎擾
            for (int nPt = nStart; nPt < nEnd; nPt++)
            {
                // ���ʒ��p���W�̑z��
                BoostPoint pt(psElem->padfX[nPt], psElem->padfY[nPt]);
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
 * @brief �����ǂݍ���
 * @param[in]   shpMng          �V�F�[�v�}�l�[�W���[
 * @param[out]  vecFields       �����t�B�[���h�Q
 * @param[out]  vecAttrRecords  �������Q
 * @return ���R�[�h��
*/
int CShapeReader::readAttribute(
    CShapeManager &shpMng,
    std::vector<CShapeAttribute::AttributeFieldData> &vecFields,
    std::vector<CShapeAttribute::AttributeDataRecord> &vecAttrRecords)
{
    vecFields.clear();
    vecAttrRecords.clear();

    // �����t�B�[���h�̎擾
    for (int i = 0; i < shpMng.nField; i++)
    {
        CShapeAttribute::AttributeFieldData field;

        char szTitle[11];
        int nWidth, nDecimals;
        DBFFieldType eType = DBFGetFieldInfo(shpMng.hDBF, i, szTitle, &nWidth, &nDecimals);

        switch (eType)
        {
        case FTString:
            field.fieldType = CShapeAttribute::AttributeFieldType::ATTR_FIELD_TYPE_STRING;
            break;
        case FTInteger:
            field.fieldType = CShapeAttribute::AttributeFieldType::ATTR_FIELD_TYPE_INT;
            break;
        case FTDouble:
            field.fieldType = CShapeAttribute::AttributeFieldType::ATTR_FIELD_TYPE_DOUBLE;
            break;
        case FTLogical:
        default:
            break;
        }
        field.strName = szTitle;
        field.nWidth = nWidth;
        field.nDecimals = nDecimals;

        vecFields.emplace_back(field);
    }

    // �������R�[�h�̎擾
    for (int i = 0; i < shpMng.nEntities; i++)
    {
        CShapeAttribute::AttributeDataRecord record;
        record.nShapeId = i;

        for (int j = 0; j < shpMng.nField; j++)
        {
            CShapeAttribute::AttributeData attrData;

            char szTitle[11];
            int nWidth, nDecimals;
            DBFFieldType eType = DBFGetFieldInfo(shpMng.hDBF, i, szTitle, &nWidth, &nDecimals);

            switch (eType)
            {
            case FTString:
                attrData.dataType = CShapeAttribute::AttributeDataType::ATTR_DATA_TYPE_STRING;
                attrData.strValue = DBFReadStringAttribute(shpMng.hDBF, i, j);
                break;
            case FTInteger:
                attrData.dataType = CShapeAttribute::AttributeDataType::ATTR_DATA_TYPE_INT;
                attrData.nValue = DBFReadIntegerAttribute(shpMng.hDBF, i, j);
                break;
            case FTDouble:
                attrData.dataType = CShapeAttribute::AttributeDataType::ATTR_DATA_TYPE_DOUBLE;
                attrData.dValue = DBFReadDoubleAttribute(shpMng.hDBF, i, j);
                break;
            case FTLogical:
                attrData.dataType = CShapeAttribute::AttributeDataType::ATTR_DATA_TYPE_STRING;
                attrData.strValue = DBFReadLogicalAttribute(shpMng.hDBF, i, j);
                break;
            default:
                attrData.dataType = CShapeAttribute::AttributeDataType::ATTR_DATA_TYPE_NULL;
                break;
            }

            record.vecAttribute.emplace_back(attrData);
        }

        vecAttrRecords.emplace_back(record);
    }

    return static_cast<int>(vecAttrRecords.size());
}

/*!
 * @brief �|���S����shape file�ǂݍ���
 * @param[in]   shpMng      �V�F�[�v�}�l�[�W���[
 * @param[out]  polygons    �|���S���Q
 * @return      ���R�[�h��
*/
int CShapeReader::readPolygons(
    CShapeManager &shpMng,
    BoostMultiPolygon &polygons)
{
    polygons.clear();

    for (int i = 0; i < shpMng.nEntities; i++)
    {
        SHPObject *psElem = SHPReadObject(shpMng.hSHP, i);
        if (psElem)
        {
            // polygon�̎擾
            // �p�[�g����
            std::vector<std::vector<BoostPoint>> vecParts;
            if (separateParts(psElem, vecParts) > 0)
            {
                BoostPolygon polygon;

                if (shpMng.nShapeType == SHPT_MULTIPATCH)
                {
                    int nIndex = 0;
                    for (std::vector<std::vector<BoostPoint>>::iterator it = vecParts.begin();
                        it != vecParts.end(); it++, nIndex++)
                    {
                        if (psElem->panPartType[nIndex] == SHPP_OUTERRING)
                        {
                            // �O��
                            for (auto itPt = it->begin(); itPt != it->end(); itPt++)
                            {
                                polygon.outer().push_back(*itPt);
                            }
                        }
                        else if (psElem->panPartType[nIndex] == SHPP_INNERRING)
                        {
                            // ��
                            polygon.inners().push_back(BoostPolygon::ring_type());
                            for (auto itPt = it->begin(); itPt != it->end(); itPt++)
                            {
                                polygon.inners().back().push_back(*itPt);
                            }
                        }
                    }
                }
                else
                {
                    // �O��
                    for (auto itPt = vecParts[0].begin(); itPt != vecParts[0].end(); itPt++)
                    {
                        polygon.outer().push_back(*itPt);
                    }
                }

                bg::correct(polygon);
                if (bg::is_valid(polygon))
                    polygons.push_back(polygon);


            }
        }
        SHPDestroyObject(psElem);
    }

    return static_cast<int>(polygons.size());
}
