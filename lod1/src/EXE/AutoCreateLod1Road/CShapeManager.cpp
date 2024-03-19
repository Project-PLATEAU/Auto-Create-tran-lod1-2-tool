#include "pch.h"
#include "CShapeManager.h"
#include "CFileUtil.h"

/*!
 * @brief コンストラクタ
*/
CShapeManager::CShapeManager(void)
{
    // ハンドル
    hSHP = NULL;
    hDBF = NULL;

    nShapeType = SHPT_NULL; // 種別
    nEntities = 0;          // 要素数
    nField = 0;             // field数
}

/*!
 * @brief デストラクタ
*/
CShapeManager::~CShapeManager(void)
{
    Close();
}

/*!
 * @brief   open shape file
 * @param   strShpPath  shpファイルパス
 * @return  処理結果
 * @retval  true        成功
 * @retval  false       失敗
 * @note    shpファイルと同階層にdbfファイルが存在すること
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

    // 要素数、種別、バウンディング情報の取得
    SHPGetInfo(hSHP, &nEntities, &nShapeType, adfMinBound, adfMaxBound);

    // 要素数の確認
    if (nEntities <= 0)
    {
        Close();
        return false;
    }

    // field数
    nField = DBFGetFieldCount(hDBF);

    return true;
}

/*!
 * @brief   open shape file without dbf file.
 * @param   strShpPath  shpファイルパス
 * @return  処理結果
 * @retval  true        成功
 * @retval  false       失敗
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

    // 要素数、種別、バウンディング情報の取得
    SHPGetInfo(hSHP, &nEntities, &nShapeType, adfMinBound, adfMaxBound);

    // 要素数の確認
    if (nEntities <= 0)
    {
        Close();
        return false;
    }

    // field数
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
 * @brief ポリゴンのshape file出力
 * @param[in] polygons          ポリゴン群
 * @param[in] strShpPath        出力 shape file パス
 * @param[in] vecFields         属性フィールド定義データ
 * @param[in] vecAttrRecords    属性値データ
 * @param[in] bHole             穴の書き出しフラグ
 * @return  処理結果
 * @retval  true        成功
 * @retval  false       失敗
*/
bool CShapeWriter::OutputPolygons(
    const BoostMultiPolygon &polygons,
    std::string strShpPath,
    const std::vector<CShapeAttribute::AttributeFieldData> &vecFields,
    const std::vector<CShapeAttribute::AttributeDataRecord> &vecAttrRecords,
    const bool bHole)
{
    const double dZ = 0;
    int nShpType = SHPT_POLYGON;    // 穴無し
    if (bHole)
        nShpType = SHPT_MULTIPATCH; // 穴有り

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
        // 1個分のポリゴンの書き込み

        if (bHole)
        {
            // 穴有り
            // 頂点数とパーツ先頭インデックス
            int nPtNum = static_cast<int>(itPoly->outer().size());
            std::vector<int> vecPartStartIdx;
            vecPartStartIdx.push_back(0);
            for (auto itInner = itPoly->inners().cbegin();
                itInner != itPoly->inners().cend(); itInner++)
            {
                vecPartStartIdx.push_back(nPtNum);
                nPtNum += static_cast<int>(itInner->size());
            }

            // 頂点配列の格納
            double *pX, *pY, *pZ;
            pX = new double[nPtNum];
            pY = new double[nPtNum];
            pZ = new double[nPtNum];
            // 外輪郭
            int idx = 0;
            for (; idx < static_cast<int>(itPoly->outer().size()); idx++)
            {
                pX[idx] = (itPoly->outer().data() + idx)->x();
                pY[idx] = (itPoly->outer().data() + idx)->y();
                pZ[idx] = dZ;
            }
            // 穴
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

            // パーツ先頭インデックス配列とパーツタイプの格納
            int nParts = static_cast<int>(vecPartStartIdx.size()); // パーツ数
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
            // 穴無し
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

    // 属性情報の書き込み
    return writeAttribute(strShpPath, vecFields, vecAttrRecords, polygons.size());
}


/*!
 * @brief ポリラインのshape file 出力
 * @param[in] polylines         ポリライン集合
 * @param[in] strShpPath        shpファイルパス
 * @param[in] vecFields         属性フィールド定義データ
 * @param[in] vecAttrRecords    属性値データ
 * @return  処理結果
 * @retval  true    成功
 * @retval  false   失敗
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
        // 1本分のポリラインの書き込み
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

    // 属性情報の書き込み
    return writeAttribute(strShpPath, vecFields, vecAttrRecords, polylines.size());
}

/*!
 * @brief 点のshape file出力
 * @param[in] points            点群
 * @param[in] strShpPath        shpファイルパス
 * @param[in] vecFields         属性フィールド定義データ
 * @param[in] vecAttrRecords    属性値データ
 * @return  処理結果
 * @retval  true    成功
 * @retval  false   失敗
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

    // 属性情報の書き込み
    return writeAttribute(strShpPath, vecFields, vecAttrRecords, points.size());
}

/*!
 * @brief 属性情報の書き込み
 * @param[in] strShpPath        shapeファイルパス
 * @param[in] vecFields         属性フィールド群
 * @param[in] vecAttrRecords    属性情報群
 * @param[in] shapeNum          形状情報数
 * @return  処理結果
 * @retval  true    成功
 * @retval  false   失敗
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
                // 整数
                DBFAddField(hDBF, itField->strName.c_str(), DBFFieldType::FTInteger,
                    itField->nWidth, itField->nDecimals);
            }
            else if (itField->fieldType == CShapeAttribute::AttributeFieldType::ATTR_FIELD_TYPE_DOUBLE)
            {
                // 実数
                DBFAddField(hDBF, itField->strName.c_str(), DBFFieldType::FTDouble,
                    itField->nWidth, itField->nDecimals);
            }
            else if (itField->fieldType == CShapeAttribute::AttributeFieldType::ATTR_FIELD_TYPE_STRING)
            {
                // 文字列
                DBFAddField(hDBF, itField->strName.c_str(), DBFFieldType::FTString,
                    itField->nWidth, itField->nDecimals);
            }
        }

        for (std::vector<CShapeAttribute::AttributeDataRecord>::const_iterator itAttr = vecAttrRecords.cbegin();
            itAttr != vecAttrRecords.cend(); itAttr++)
        {
            // 1レコード分の属性
            if (vecFields.size() != itAttr->vecAttribute.size()
                || itAttr->nShapeId < 0 || itAttr->nShapeId >= shapeNum)
            {
                // 属性数が異なる
                // shape idが不正な場合はskip
                continue;
            }
            // 属性値の書き込み
            int nAttrId = 0;
            for (std::vector<CShapeAttribute::AttributeData>::const_iterator itData = itAttr->vecAttribute.cbegin();
                itData != itAttr->vecAttribute.cend(); itData++, nAttrId++)
            {
                if (itData->dataType == CShapeAttribute::AttributeDataType::ATTR_DATA_TYPE_INT)
                {
                    // 整数
                    DBFWriteIntegerAttribute(hDBF, itAttr->nShapeId, nAttrId, itData->nValue);
                }
                else if (itData->dataType == CShapeAttribute::AttributeDataType::ATTR_DATA_TYPE_DOUBLE)
                {
                    // 実数
                    DBFWriteDoubleAttribute(hDBF, itAttr->nShapeId, nAttrId, itData->dValue);
                }
                else if (itData->dataType == CShapeAttribute::AttributeDataType::ATTR_DATA_TYPE_STRING)
                {
                    // 文字列
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
 * @brief ポリゴンのshape file読み込み
 * @param[in]   strShpPath      shapeファイルパス
 * @param[out]  polygons        ポリゴン群
 * @param[out]  vecFields       属性フィールド群
 * @param[out]  vecAttrRecords  属性情報群
 * @return      処理結果
 * @retval      true    成功
 * @retval      false   失敗
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

    // type確認
    if (shpMng.nShapeType != SHPT_POLYGON
        && shpMng.nShapeType != SHPT_POLYGONZ
        && shpMng.nShapeType != SHPT_POLYGONM
        && shpMng.nShapeType != SHPT_MULTIPATCH)
    {
        shpMng.Close();
        return false;
    }

    // データ取得
    readPolygons(shpMng, polygons);

    // 属性の取得
    readAttribute(shpMng, vecFields, vecAttrRecords);

    // close shape file
    shpMng.Close();

    return true;
}

/*!
 * @brief
 * @param[in]   strShpPath  shapeファイルパス
 * @param[out]  polygons    ポリゴン群
 * @return      処理結果
 * @retval      true        成功
 * @retval      false       失敗
 * @note        ポリゴンデータのみを取得し属性は無視する
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

    // type確認
    if (shpMng.nShapeType != SHPT_POLYGON
        && shpMng.nShapeType != SHPT_POLYGONZ
        && shpMng.nShapeType != SHPT_POLYGONM
        && shpMng.nShapeType != SHPT_MULTIPATCH)
    {
        shpMng.Close();
        return false;
    }

    // データ取得
    readPolygons(shpMng, polygons);

    // close shape file
    shpMng.Close();
    return true;
}

/*!
 * @brief ポリライン読み込み(属性データは無視)
 * @param[in]   strShpPath      shapeファイルパス
 * @param[out]  popolylinesints ポリライン軍
 * @return      処理結果
 * @retval      true        成功
 * @retval      false       失敗
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

    // type確認
    if (shpMng.nShapeType != SHPT_ARC
        && shpMng.nShapeType != SHPT_ARCZ
        && shpMng.nShapeType != SHPT_ARCM)
    {
        shpMng.Close();
        return false;
    }

    // データ取得
    SHPObject *psElem;
    for (int i = 0; i < shpMng.nEntities; i++)
    {
        psElem = SHPReadObject(shpMng.hSHP, i);
        if (psElem)
        {
            // パート分割
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
 * @brief マルチポイント読み込み(属性データは無視)
 * @param[in]   strShpPath  shapeファイルパス
 * @param[out]  points      頂点群
 * @return      処理結果
 * @retval      true        成功
 * @retval      false       失敗
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

    // type確認
    if (shpMng.nShapeType != SHPT_POINT)
    {
        shpMng.Close();
        return false;
    }

    // データ取得
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
 * @brief       パート分割
 * @param[in]   psElem   1レコード分のshapeデータのポインタ
 * @param[out]  vecParts パート分割後の座標データ配列
 * @return      分割後のパートデータ数
*/
int CShapeReader::separateParts(
    SHPObject *psElem,
    std::vector<std::vector<BoostPoint>> &vecParts)
{
    vecParts.clear();

    if (psElem)
    {
        // パート分割
        for (int nPart = 0; nPart < psElem->nParts; nPart++)
        {
            int nStart = psElem->panPartStart[nPart];
            int nEnd = psElem->nVertices;   // 初期値:頂点数
            if (nPart < psElem->nParts - 1)
            {
                nEnd = psElem->panPartStart[nPart + 1]; //次パートの開始インデックス
            }

            std::vector<BoostPoint> vecPoints;
            // 頂点列の取得
            for (int nPt = nStart; nPt < nEnd; nPt++)
            {
                // 平面直角座標の想定
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
 * @brief 属性読み込み
 * @param[in]   shpMng          シェープマネージャー
 * @param[out]  vecFields       属性フィールド群
 * @param[out]  vecAttrRecords  属性情報群
 * @return レコード数
*/
int CShapeReader::readAttribute(
    CShapeManager &shpMng,
    std::vector<CShapeAttribute::AttributeFieldData> &vecFields,
    std::vector<CShapeAttribute::AttributeDataRecord> &vecAttrRecords)
{
    vecFields.clear();
    vecAttrRecords.clear();

    // 属性フィールドの取得
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

    // 属性レコードの取得
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
 * @brief ポリゴンのshape file読み込み
 * @param[in]   shpMng      シェープマネージャー
 * @param[out]  polygons    ポリゴン群
 * @return      レコード数
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
            // polygonの取得
            // パート分割
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
                            // 外周
                            for (auto itPt = it->begin(); itPt != it->end(); itPt++)
                            {
                                polygon.outer().push_back(*itPt);
                            }
                        }
                        else if (psElem->panPartType[nIndex] == SHPP_INNERRING)
                        {
                            // 穴
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
                    // 外周
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
