#include "pch.h"
#include "CReadParamFile.h"
#include "CFileUtil.h"

CReadParamFile CReadParamFile::m_instance;  // インスタンス

/*!
 * @brief コンストラクタ
 */
CReadParamFile::CReadParamFile(void)
    : m_nJPZone(0),
      m_strRoadSHPPath(""),
      m_strRoadFacilitiesPointSHPPath(""),
      m_strRoadFacilitiesLineSHPPath(""),
      m_strRoadFacilitiesPolygonSHPPath(""),
      m_strOutputFolderPath(""),
      m_strDMCodeAttr(""),
      m_strGeometryTypeAttr(""),
      m_strMinArea(""),
      m_minArea(0),
      m_strMaxDistance(""),
      m_maxDistance(0),
      m_strRoadBeforeDivisionShpFilePath(""),
      m_strDividedRoadShpFilePath(""),
      m_strIntersectionShpFilePath(""),
      m_nRegionWidth(0),
      m_nRegionHeight(0),
      m_nThreadNum(2)
{

}

/*!
 * @brief デストラクタ
 */
CReadParamFile::~CReadParamFile(void)
{
}

/*!
 * @brief 初期化
 * @param strParamPath パラメータファイルパス
 * @return 処理結果
 * @retval true     成功
 * @retval false    失敗
 */
bool CReadParamFile::Initialize(std::string strParamPath)
{
    bool bRet = false;
    CINIFileIO inifile;
    std::string SETTING = "Setting";
    std::string DM_ATTR_SETTING = "DM_Attribute";
    std::string ERR_CHECK_SETTING = "ErrCheck";

    // file open
    bRet = inifile.Open(strParamPath);
    if (bRet)
    {
        // パラメータ取得
        // 系番号
        m_nJPZone = inifile.GetInt(SETTING, "JPZone", 0);

        // 道路縁SHPパス
        m_strRoadSHPPath = inifile.GetString(SETTING, "RoadSHPPath", "");

        // 道路施設(点)SHPパス
        m_strRoadFacilitiesPointSHPPath = inifile.GetString(SETTING, "RoadFacilitiesPointSHPPath", "");

        // 道路施設(線)SHPパス
        m_strRoadFacilitiesLineSHPPath = inifile.GetString(SETTING, "RoadFacilitiesLineSHPPath", "");

        // 道路施設(面)SHPパス
        m_strRoadFacilitiesPolygonSHPPath = inifile.GetString(SETTING, "RoadFacilitiesPolygonSHPPath", "");

        // 道路縁解析領域の幅(m)
        m_nRegionWidth = inifile.GetInt(SETTING, "RegionWidth", 0);

        // 道路縁解析領域の高さ(m)
        m_nRegionHeight = inifile.GetInt(SETTING, "RegionHeight", 0);

        // マルチスレッド数
        m_nThreadNum = inifile.GetInt(SETTING, "ThreadNum", 0);

        // 出力フォルダパス
        m_strOutputFolderPath = inifile.GetString(SETTING, "OutputFolderPath", "");

        // DMコード属性名
        m_strDMCodeAttr = inifile.GetString(DM_ATTR_SETTING, "DMCode", "");

        // DMコード属性名
        m_strGeometryTypeAttr = inifile.GetString(DM_ATTR_SETTING, "GeometryType", "");

        // ポリゴンの面積の最小値
        m_strMinArea = inifile.GetString(ERR_CHECK_SETTING, "MinArea", "");

        // 車道交差部ポリゴン中心と交差点の距離の最大値
        m_strMaxDistance = inifile.GetString(ERR_CHECK_SETTING, "MaxDistance", "");

        // 分割前の道路ポリゴンのファイルパス(Debug)
        m_strRoadBeforeDivisionShpFilePath = inifile.GetString(ERR_CHECK_SETTING, "inputRoadBeforeDivisionShpFilePath", "");

        // 分割後の道路ポリゴンのファイルパス(Debug)
        m_strDividedRoadShpFilePath = inifile.GetString(ERR_CHECK_SETTING, "inputDividedShpFilePath", "");

        // 交差点情報のファイルパス(Debug)
        m_strIntersectionShpFilePath = inifile.GetString(ERR_CHECK_SETTING, "inputIntersectionShpFilePath", "");

        if (checkParam())
        {
            // パラメータエラー有り
            bRet = false;
        }
        else
        {
            // 文字列を数値に変換
            m_minArea = std::stof(m_strMinArea);
            m_maxDistance = std::stof(m_strMaxDistance);
        }
    }

    return bRet;
}

/*!
 * @brief エラーチェック用のファイルパスが設定されているかどうか
 * @return エラーチェック用のファイルパスが設定されている
 * @retval true     ファイルパスが設定されている
 * @retval false    ファイルパスが設定されていない
 */
bool CReadParamFile::IsErrCheckFromShp()
{
    if (m_strRoadBeforeDivisionShpFilePath.empty() || m_strDividedRoadShpFilePath.empty() || m_strIntersectionShpFilePath.empty())
    {
        return false;
    }

    return true;
}

/*!
 * @brief パラメータのエラーチェック
 * @return エラー結果
 * @retval true     エラー有り
 * @retval false    エラー無し
 */
bool CReadParamFile::checkParam(void)
{
    bool bRet = false;
    if (m_nJPZone < 1 || 19 < m_nJPZone
        || m_strRoadSHPPath.empty() || !CFileUtil::IsExistPath(m_strRoadSHPPath)
        || (!m_strRoadFacilitiesPointSHPPath.empty() && !CFileUtil::IsExistPath(m_strRoadFacilitiesPointSHPPath))
        || (!m_strRoadFacilitiesLineSHPPath.empty() && !CFileUtil::IsExistPath(m_strRoadFacilitiesLineSHPPath))
        || (!m_strRoadFacilitiesPolygonSHPPath.empty() && !CFileUtil::IsExistPath(m_strRoadFacilitiesPolygonSHPPath))
        || m_nRegionWidth < 1 || m_nRegionHeight < 1
        || m_nThreadNum < 1
        || m_strOutputFolderPath.empty()
        || m_strDMCodeAttr.empty()
        || m_strGeometryTypeAttr.empty()
        || m_strMinArea.empty()
        || m_strMaxDistance.empty())
    {
        // 系番号範囲外
        // 道路縁shapeファイルパスが空 or ファイルが存在しない
        // 設備shapeファイルパスが設定済みかつファイルが存在しない場合
        // 出力フォルダパスが空
        // DMコード,レコードタイプ,図形区分属性名が空
        // 注目領域幅、高さ、マルチスレッド数が未設定
        bRet = true;
    }
    return bRet;
}

/*!
 * @brief  SHPファイルの平面直角座標系の系番号設定のゲッター
 * @return 系番号(1 - 19)
 */
int CReadParamFile::GetJPZone()
{
    return m_nJPZone;
}

/*!
 * @brief  道路縁SHPファイルパス設定のゲッター
 * @return 道路縁SHPファイルパス
*/
std::string CReadParamFile::GetRoadSHPPath()
{
    return m_strRoadSHPPath;
}

/*!
 * @brief  道路施設(点)SHPファイルパス設定のゲッター
 * @return 道路施設(点)SHPファイルパス
*/
std::string CReadParamFile::GetRoadFacilitiesPointSHPPath()
{
    return m_strRoadFacilitiesPointSHPPath;
}

/*!
 * @brief  道路施設(線)SHPファイルパス設定のゲッター
 * @return 道路施設(線)SHPファイルパス
*/
std::string CReadParamFile::GetRoadFacilitiesLineSHPPath()
{
    return m_strRoadFacilitiesLineSHPPath;
}

/*!
 * @brief  道路施設(面)SHPファイルパス設定のゲッター
 * @return 道路施設(面)SHPファイルパス
*/
std::string CReadParamFile::GetRoadFacilitiesPolygonSHPPath()
{
    return m_strRoadFacilitiesPolygonSHPPath;
}

/*!
 * @brief  道路縁解析処理の注目領域幅(m)のゲッター
 * @return 道路縁解析処理の注目領域幅(m)
 */
int CReadParamFile::GetRegionWidth()
{
    return m_nRegionWidth;
}

/*!
 *@brief  道路縁解析処理の注目領域高さ(m)のゲッター
 *@return 道路縁解析処理の注目領域高さ(m)
 */
int CReadParamFile::GetRegionHeight()
{
    return m_nRegionHeight;
}

/*!
 *@brief  マルチスレッド数のゲッター
 *@return マルチスレッド数
 */
int CReadParamFile::GetThreadNum()
{
    return m_nThreadNum;
}

/*!
 * @brief  出力フォルダパス設定のゲッター
 * @return 出力フォルダパス
*/
std::string CReadParamFile::GetOutputFolderPath()
{
    return m_strOutputFolderPath;
}

/*!
 * @brief  DMコード属性名設定のゲッター
 * @return DMコード属性名
*/
std::string CReadParamFile::GetDMCodeAttribute()
{
    return m_strDMCodeAttr;
}

/*!
 * @brief  D図形区分コードの属性名設定のゲッター
 * @return 図形区分コードの属性名
*/
std::string CReadParamFile::GetGeometryTypeAttribute()
{
    return m_strGeometryTypeAttr;
}

/*!
 * @brief  レコードタイプ属性名設定のゲッター
 * @return レコードタイプ属性名
*/
double CReadParamFile::GetMinArea()
{
    return m_minArea;
}

/*!
 * @brief  D図形区分コードの属性名設定のゲッター
 * @return 図形区分コードの属性名
*/
double CReadParamFile::GetMaxDistance()
{
    return m_maxDistance;
}

std::string CReadParamFile::GetRoadBeforeDivisionShpFilePath()
{
    return m_strRoadBeforeDivisionShpFilePath;
}

std::string CReadParamFile::GetDividedRoadShpFilePath()
{
    return m_strDividedRoadShpFilePath;
}

std::string CReadParamFile::GetIntersectionShpFilePath()
{
    return m_strIntersectionShpFilePath;
}
