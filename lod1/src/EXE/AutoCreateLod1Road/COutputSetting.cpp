#include "pch.h"
#include "COutputSetting.h"
#include "boost/format.hpp"
#include "CTime.h"

COutputSetting COutputSetting::m_instance;  // インスタンス

/*!
 * @brief コンストラクタ
 */
COutputSetting::COutputSetting(void)
    : m_strShpFilePath(""),
    m_strShpFilePathWithHoles(""),
    m_strErrFilePath("")
{

}

/*!
 * @brief デストラクタ
 */
COutputSetting::~COutputSetting(void)
{
}

/*!
 * @brief 初期化
 * @param[in]   strOutputFolderPath 出力フォルダパス
 * @param[in]   strShpFileName      shapeファイル名
 * @param[in]   strErrFileName      エラーチェック結果のファイル名
 * @return      処理結果
 * @retval      true     成功
 * @retval      false    失敗
*/
bool COutputSetting::Initialize(
    std::string strOutputFolderPath,
    std::string strShpFileName,
    std::string strErrFileName)
{
    // 出力フォルダが存在しない場合は作成(存在する場合は何もしない)
    bool bRet = COutputSetting::CreateFolder(strOutputFolderPath);

    // 出力shpファイルパス
    std::string strTempShpFileName = (boost::format("%s.shp") % strShpFileName).str();
    std::string strShpPath = CFileUtil::Combine(strOutputFolderPath, strTempShpFileName);   // 穴無し
    strTempShpFileName = (boost::format("%s_withHoles.shp") % strShpFileName).str();
    std::string strShpPathWithHoles = CFileUtil::Combine(strOutputFolderPath, strTempShpFileName);   // 穴あり
    // エラーチェック結果のファイルパス
    std::string strTempErrFileName = (boost::format("%s.csv") % strErrFileName).str();
    std::string strErrPath = CFileUtil::Combine(strOutputFolderPath, strTempErrFileName);

    if (CFileUtil::IsExistPath(strShpPath))
    {
        // 既存ファイルが存在する場合は出力ファイル名を変更する
        std::string strTime = CTime::GetCurrentTime().Format("%Y%m%d_%H%M%S");  // 現在時刻
        strTempShpFileName = (boost::format("%s_%s.shp") % strShpFileName % strTime).str();
        strShpPath = CFileUtil::Combine(strOutputFolderPath, strTempShpFileName);   // 穴無し

        strTempShpFileName = (boost::format("%s_withHoles_%s.shp") % strShpFileName % strTime).str();
        strShpPathWithHoles = CFileUtil::Combine(strOutputFolderPath, strTempShpFileName);  // 穴あり

        strTempErrFileName = (boost::format("%s_%s.csv") % strErrFileName % strTime).str();
        strErrPath = CFileUtil::Combine(strOutputFolderPath, strTempErrFileName);
    }

    m_strShpFilePath = strShpPath;
    m_strShpFilePathWithHoles = strShpPathWithHoles;
    m_strErrFilePath = strErrPath;

    return bRet;
}

/*!
 * @brief  shpファイルパス(穴無しポリゴン)
 * @return shpファイルパス(穴無しポリゴン)
*/
std::string COutputSetting::GetShpFilePath(void)
{
    return m_strShpFilePath;
}

/*!
 * @brief  shpファイルパス(穴有りポリゴン)
 * @return shpファイルパス(穴有りポリゴン)
*/
std::string COutputSetting::GetShpFilePathWithHoles(void)
{
    return m_strShpFilePathWithHoles;
}

/*!
 * @brief  エラーチェック結果のファイルパス
 * @return エラーチェック結果のファイルパス
*/
std::string COutputSetting::GetErrFilePath(void)
{
    return m_strErrFilePath;
}
