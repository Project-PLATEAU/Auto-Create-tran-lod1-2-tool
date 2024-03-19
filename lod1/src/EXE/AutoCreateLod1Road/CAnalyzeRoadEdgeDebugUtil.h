#pragma once
#include <vector>
#include <string>
#include <filesystem>
#include "AnalyzeRoadEdgeCommon.h"
#include "CShapeManager.h"
#include "CFileUtil.h"

/*!
 * @brief 道路縁解析処理のデバッグ用クラス
*/
class CAnalyzeRoadEdgeDebugUtil
{
public:
    CAnalyzeRoadEdgeDebugUtil() {}      //!< コンストラクタ
    ~CAnalyzeRoadEdgeDebugUtil() {}     //!< デストラクタ

    /*!
     * @brief  現在の作業ディレクトリパスの取得
     * @return 作業ディレクトリパス
     */
    static std::string GetCurrentPath()
    {
        std::filesystem::path path = std::filesystem::current_path();
        return path.string();
    }

    /*!
     * @brief フォルダ作成
     * @param strPath [in] フォルダパス
     * @return 処理結果
     * @retval true     作成成功 or 既存フォルダ
     * @retval false    作成失敗
     */
    static bool CreateFolder(std::string strPath)
    {
        bool bRet = CFileUtil::IsExistPath(strPath);
        if (!bRet)
        {
            // 存在しない場合
            bRet = std::filesystem::create_directories(strPath);
        }
        return bRet;
    }

    // 点のshapefile出力
    bool OutputMultiPointsToShp(
        const BoostMultiPoints &points,
        std::string strShpPath);

    // ポリラインのshapefile出力
    bool OutputPolylinesToShp(
        const BoostMultiLines &polylines,
        std::string strShpPath);

    // ポリゴンのshapefile出力
    bool OutputPolygonsToShp(
        const BoostMultiPolygon &polygons,
        std::string strShpPath,
        const bool bHole = false);

    // 注目範囲の可視化
    void OutputProcArea(
        std::string strShpPath,
        const double dInputMinX,
        const double dInputMinY,
        const int nRow,
        const int nColumn,
        const double dProcWidth,
        const double dProcHeight);

protected:

private:

};
