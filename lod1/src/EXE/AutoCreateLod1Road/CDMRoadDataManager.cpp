#include "pch.h"
#include "CDMRoadDataManager.h"
#include "shapefil.h"
#include "StringEx.h"
#include "CFileIO.h"
#include "CGeoUtil.h"
#include "CShapeManager.h"
#include "CReadParamFile.h"

/*!
 * @brief     道路縁shapeファイル読み込み
 *
 * @return    処理結果
 * @retval    true    成功
 * @retval    false   失敗
*/
bool CDMRoadDataManager::ReadRoadEdgeShapeFile(void)
{
    // 設定ファイル情報からSHPパスと分類コード属性名を取得
    std::string strShpPath = GetCreateParam()->GetRoadSHPPath();
    std::string strCodeAttrName = GetCreateParam()->GetDMCodeAttribute();

    // open shape file
    CShapeManager shpMng = CShapeManager();
    if (!shpMng.Open(strShpPath))
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

    // 取得属性のfield index
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
    // field indexの取得確認
    if (nCodeIndex < 0)
    {
        shpMng.Close();
        return false;
    }

    // データ取得
    SHPObject* psElem;
    for (int i = 0; i < shpMng.nEntities; i++)
    {
        psElem = SHPReadObject(shpMng.hSHP, i);
        if (psElem)
        {
            // 属性取得
            int nCode = DBFReadIntegerAttribute(shpMng.hDBF, i, nCodeIndex);    // DM Code

            // DM Codeを上2桁(大分類)、下2桁(小分類)に分割
            int nDMCodeUpper, nDMCodeLower;
            separateDMCode(nCode, nDMCodeUpper, nDMCodeLower);

            // パート分割
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
 * @brief   道路施設shapeファイル読み込み
 * @note    施設情報なしの場合も正常終了
*/
void CDMRoadDataManager::ReadRoadFacilitiesShapeFile(void)
{
    // 設定ファイル情報から分類コード属性名を取得
    std::vector<RoadFacilitiesDataType> types;
    types.push_back(RoadFacilitiesDataType::ROAD_FACILITIES_LINE_DATA);
    types.push_back(RoadFacilitiesDataType::ROAD_FACILITIES_POINT_DATA);
    types.push_back(RoadFacilitiesDataType::ROAD_FACILITIES_POLYGON_DATA);

    m_vecRoadFacilitiesData.clear();
    for (std::vector<RoadFacilitiesDataType>::const_iterator itType = types.cbegin();
        itType != types.cend(); itType++)
    {
        // 道路施設shpパス
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
 * @brief DMコード分割
 * @param[in]   nCode   DMコード
 * @param[out]  nUpper  DMコード(上2桁)
 * @param[out]  nLower  DMコード(下2桁)
 * @return
*/
void CDMRoadDataManager::separateDMCode(int nCode, int &nUpper, int &nLower)
{
    nUpper = nCode / 100;
    nLower = nCode % 100;
}

/*!
 * @brief DMコード結合
 * @param nUpper DMコード(上2桁)
 * @param nLower DMコード(下2桁)
 * @return DMコード
*/
int CDMRoadDataManager::combineDMCode(int nUpper, int nLower)
{
    return nUpper * 100 + nLower;
}

/*!
 * @brief 精度区分コード分割
 * @param[in]   nCode   DMコード
 * @param[out]  nUpper  DMコード(上2桁)
 * @param[out]  nLower  DMコード(下2桁)
 * @return
*/
void CDMRoadDataManager::separateAccuracyCode(int nCode, int& nUpper, int& nLower)
{
    nUpper = nCode / 10;
    nLower = nCode % 10;
}

/*!
 * @brief パート分割
 * @param[in]   psElem      1レコード分のshapeデータのポインタ
 * @param[out]  vecParts    パート分割後の座標データ配列
 * @return 分割後のパートデータ数
*/
int CDMRoadDataManager::separateParts(
    SHPObject* psElem,
    std::vector<std::vector<CVector3D>>& vecParts)
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

            std::vector<CVector3D> vecPoints;
            // 頂点列の取得
            for (int nPt = nStart; nPt < nEnd; nPt++)
            {
                // 平面直角座標の想定
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
 * @brief 道路施設shapeファイル読み込み(内部関数)
 * @param[in] strShpPath    shapeファイルパス
 * @param[in] type          読み込むshapeファイルの形状タイプ
 * @return    処理結果
 * @retval    true    成功
 * @retval    false   失敗
*/
bool CDMRoadDataManager::readRoadFacilitiesShapeFile(const std::string &strShpPath, RoadFacilitiesDataType type)
{
    // 設定ファイル情報からSHPパスと分類コード属性名を取得
    std::string strCodeAttrName = GetCreateParam()->GetDMCodeAttribute();
    std::string strGeometryTypeAttrName = GetCreateParam()->GetGeometryTypeAttribute();

    // データ選別用のDMコード(道路橋(高架部), 道路のトンネル)
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

    // type確認
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

    // 取得属性のfield index
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
                nGeometryTypeIndex = i; // 図形区分
            }
        }
    }
    // field indexの取得確認
    if (nCodeIndex < 0 || nGeometryTypeIndex < 0)
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
            // 属性取得
            int nCode = DBFReadIntegerAttribute(shpMng.hDBF, i, nCodeIndex);            // DM Code
            int nGeoType = DBFReadIntegerAttribute(shpMng.hDBF, i, nGeometryTypeIndex); // 図形区分

            // データ選別
            if (nCode == ROAD_BRIDGE_CODE || nCode == TUNNEL_CODE)
            {
                // 道路橋(高架)または 道路のトンネルの場合
                // DM Codeを上2桁(大分類)、下2桁(小分類)に分割
                int nDMCodeUpper, nDMCodeLower;
                separateDMCode(nCode, nDMCodeUpper, nDMCodeLower);

                if (type == RoadFacilitiesDataType::ROAD_FACILITIES_LINE_DATA
                    || type == RoadFacilitiesDataType::ROAD_FACILITIES_POLYGON_DATA)
                {
                    // パート分割
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