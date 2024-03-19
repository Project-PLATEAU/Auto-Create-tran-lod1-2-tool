#pragma once

#include <vector>
#include <mutex>
#include "CGeoUtil.h"
#include "AnalyzeRoadEdgeCommon.h"
#include "CDMRoadDataManager.h"
#include "CRoadData.h"
#include "CRoadCenterLine.h"
#include "CQueue.h"
#include "boost/format.hpp"

/*!
 * @brief 排他制御付きインデックスリスト
*/
class CProcessingSet
{
public:
    CProcessingSet() {};    //!< コンストラクタ

    /*!
     * @brief コンストラクタ
    */
    CProcessingSet(CProcessingSet const &other)
    {
        std::lock_guard<std::mutex> lock(other.m_mutex);
        m_set = other.m_set;
    }

    CProcessingSet &operator=(const CProcessingSet &) = delete;  // 代入演算によるコピーの禁止
    ~CProcessingSet() {};   //!< デストラクタ

    /*!
     * @brief 追加
     * @param[in] val 入力値
    */
    void insert(int val)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_set.insert(val);
    }

    /*!
     * @brief 追加
     * @param[in]   val 入力値
     * @param[out]  strMsg  格納中データの一覧
    */
    void insert(int val, std::string &strMsg)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_set.insert(val);
        strMsg = _print();
    }

    /*!
     * @brief 削除
     * @param[in] key   値
    */
    void erase(int &key)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_set.find(key);
        if (it != m_set.end())
            m_set.erase(it);
    }

    /*!
     * @brief 削除
     * @param[in]   key     値
     * @param[out]  strMsg  格納中データの一覧
    */
    void erase(int &key, std::string &strMsg)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_set.find(key);
        if (it != m_set.end())
            m_set.erase(it);

        strMsg = _print();
    }

    /*!
     * @brief  空判定
     * @return 判定結果
     * @retval true     データなし
     * @retval false    データあり
    */
    bool empty() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_set.empty();
    }

    /*!
     * @brief データ数
     * @return データ数
    */
    size_t size() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_set.size();
    }

    /*!
     * @brief 表示用文字列
     * @return インデックス一覧(文字列)
    */
    std::string print() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return _print();
    }

protected:
private:
    mutable std::mutex m_mutex;     //!< 排他制御用
    std::set<int> m_set;            //!< インデックス格納用

    std::string _print() const
    {
        std::string strMsg;
        for (auto it = m_set.cbegin(); it != m_set.cend(); it++)
        {
            if (it == m_set.cbegin())
            {
                strMsg += (boost::format("%d") % *it).str();
            }
            else
            {
                strMsg += (boost::format(", %d") % *it).str();
            }
        }

        if (strMsg.empty())
            strMsg = "None";

        return strMsg;
    }
};


class CAnalyzeRoadEdgeManager
{
public:
    /*!
     * @brief コンストラクタ
    */
    CAnalyzeRoadEdgeManager()
    {
        m_vecOutputRoadData.clear();
        m_roadEdges.clear();
        m_bridges.clear();
        m_tunnels.clear();
        m_errMsg.clear();
        m_dInputMinX = 0;
        m_dInputMinY = 0;
        m_dProcWidth = 1500.0;
        m_dProcHeight = 2000.0;
        m_nRow = 0;
        m_nColumn = 0;

#ifdef _OUTPUT_MODEL_INTERMEDIATE_FILE
        m_strDebugFolderPath = "";
#endif
    }
    ~CAnalyzeRoadEdgeManager() {} //!< デストラクタ

    // 道路縁解析
    void Process(
        const std::vector<std::vector<CVector3D>> &vecRoadEdges,
        const std::vector<std::vector<CVector3D>> &vecBridges,
        const std::vector<BoostPairLine> &vecTunnels,
        const double dProcWidth = 1500.0, const double dProcHeight = 2000.0);

    // 道路ポリゴン出力
    bool OutputResultFile();

protected:

private:
    std::vector<CRoadData> m_vecOutputRoadData;     //!< 道路ポリゴンデータ(shp出力用)
    std::vector<std::vector<std::string>> m_errMsg; //!< エラーチェック結果

    BoostMultiLines m_roadEdges;                    //!< 入力道路縁
    BoostMultiLines m_bridges;                      //!< 入力道路橋
    std::vector<BoostPairLine> m_tunnels;           //!< 入力トンネル
    std::mutex m_mutex;                             //!< 排他制御用
    CQueue<std::pair<int, int>> m_regions;          //!< 処理対象小領域
    CProcessingSet m_processingIdxs;                //!< 処理中インデックスリスト

    double m_dInputMinX;        //!< 入力道路縁の最小x座標
    double m_dInputMinY;        //!< 入力道路縁の最小y座標
    double m_dProcWidth;        //!< 注目領域幅(m)
    double m_dProcHeight;       //!< 注目領域高さ(m)
    int m_nRow;                 //!< 注目領域の行数
    int m_nColumn;              //!< 注目領域の列数

#ifdef _OUTPUT_MODEL_INTERMEDIATE_FILE
    std::string m_strDebugFolderPath;               //!< デバッグ用フォルダパス
#endif

    // 処理領域数の算出
    void calcProcNum(
        const BoostMultiLines &roadEdges,
        const double dProcWidth,
        const double dProcHeight,
        int &nRow,
        int &nColumn,
        double &dMinX,
        double &dMinY);

    // 道路ポリゴン出力
    bool outputRoadPolygons(
        const std::string &strShpPath,
        std::vector<CRoadData> polygons,
        const bool bHole = false);

    // マルチスレッド用道路縁解析処理
    void analyze();

    // 道路縁解析開始ログ
    void startAnalysis(int nTarget, int total);

    // 道路縁解析終了ログ
    void stopAnalysis(int nTarget, int total, bool bError);
};

/*!
 * @brief 道路縁解析クラス
*/
class CAnalyzeRoadEdge
{
public:
    /*!
     * @brief コンストラクタ
    */
    CAnalyzeRoadEdge()
    {
#ifdef _OUTPUT_MODEL_INTERMEDIATE_FILE
        m_strDebugFolderPath = "";
#endif
    }

    /*!
     * @brief コンストラクタ(debug用)
    */
    CAnalyzeRoadEdge(std::string &strDebugFolderPath)
    {
#ifdef _OUTPUT_MODEL_INTERMEDIATE_FILE
        m_strDebugFolderPath = strDebugFolderPath;
#endif
    }
    ~CAnalyzeRoadEdge() {} //!< デストラクタ

    // 道路縁解析
    std::vector<CRoadData> Process(
        std::vector<std::vector<std::string>> &errMsg,
        BoostMultiLines &targetRoadEdges,
        BoostMultiLines &targetBridges,
        std::vector<BoostPairLine> &targetTunnels,
        BoostBox &targetProcArea,
        const double dInputMinX,
        const double dInputMinY,
        const double dProcWidth,
        const double dProcHeight,
        const int nRow,
        const int nColumn,
        const int nX,
        const int nY);

    // 処理対象データの抽出
    static BoostMultiLines ExtractTargetData(
        const BoostBox &area,
        const BoostMultiLines &multiLines,
        const bool bClip = false);

    // 処理対象データの抽出
    static std::vector<BoostPairLine> ExtractTargetData(
        const BoostBox &area,
        const std::vector<BoostPairLine> &pairLines,
        const bool bClip = false);

    // 処理対象データの抽出
    static BoostMultiLines ExtractTargetData(
        const BoostMultiPolygon &area,
        const BoostMultiLines &multiLines,
        const bool bClip = false);

protected:

private:

#ifdef _OUTPUT_MODEL_INTERMEDIATE_FILE
    std::string m_strDebugFolderPath;           //!< デバッグ用フォルダパス
#endif
    // ポリライン整形処理
    void shapingRoadEdge(
        BoostPolyline &polyline,
        const double dLengthTh = 0.10,
        const double dAngleTh = 90.0);

    // 道路縁と重畳する探索対象線の取得
    BoostMultiLines getOverlapEdges(
        const BoostMultiLines &roadEdges,
        const BoostMultiLines &searchLines,
        const double dBufDist = 0.01,
        const double dLengthTh = 0.10,
        const double dLengthDiffTh = 0.10);

    // 道路縁と重畳する探索対象線の取得
    std::vector<BoostPairLine> getOverlapEdges(
        const BoostMultiLines &roadEdges,
        const std::vector<BoostPairLine> &searchLines,
        const double dBufDist = 0.01,
        const double dLengthTh = 0.10,
        const double dLengthDiffTh = 0.10);

    // 立体交差探索
    void searchMultiLevelCrossing(
        BoostMultiLines &roadEdges,
        const BoostMultiLines &bridges,
        const std::vector<BoostPairLine> &tunnels,
        BoostMultiLines &upperRoadEdge,
        BoostMultiLines &middleRoadEdge,
        std::vector<BoostPairLine> &lowerRoadEdge,
        const double dLengthTh = 0.10);

    // 道路縁のループ化処理時の外輪郭補正処理
    void outerRingCorrection(
        std::vector<BoostPoint> &vecPolyline,
        const BoostPolygon &outlinePolygon);

    // 道路縁のループ化
    void looping(
        BoostMultiLines &roadedge,
        BoostMultiPolygon &blocks,
        BoostMultiPolygon &errBlocks,
        const double dAreaTh = 1.0,
        const double dOpeningBuffer = 0.1);

    // オープニング処理
    BoostMultiPolygon opening(
        BoostPolygon &polygon,
        const double dBuffer);

    // ポリゴンの減算処理(スパイクノイズ/自己交差解消機能付き)
    BoostMultiPolygon difference(
        BoostMultiPolygon &polygons,
        BoostPolygon &polygon,
        const double dOpeningBuffer = 0.1);

    // 道路ポリゴンの作成
    void createRoadPolygon(
        const BoostMultiLines &roadedges,
        const BoostMultiPolygon &blocks,
        BoostMultiPolygon &roadPolygons,
        BoostPolygon &concaveHull,
        const double dSampling = 1.0,
        const double dAreaTh = 1.0,
        const double dOpeningBuffer = 0.1);

    // shp出力処理用の注目エリアと近傍エリアの作成
    void createAreas(
        const int nRow,
        const int nColumn,
        const double dProcWidth,
        const double dProcHeight,
        const double dInputMinX,
        const double dInputMinY,
        const int nX,
        const int nY,
        BoostMultiPolygon &areas,
        int &nTargetIdx);

    // 面積占有率の確認
    int checkAreaOccupancyRate(
        const BoostPolygon &target,
        const BoostMultiPolygon &areas);

    // 注目エリアのポリゴンを取得する
    void selectPolygon(
        std::vector<CRoadData> &roadData,
        const int nRow,
        const int nColumn,
        const double dProcWidth,
        const double dProcHeight,
        const double dInputMinX,
        const double dInputMinY,
        const int nX,
        const int nY,
        std::vector<CRoadData> &dstData);

    // 注目エリアのポリゴンを取得する
    void selectPolygon(
        BoostMultiPolygon &polygons,
        const int nRow,
        const int nColumn,
        const double dProcWidth,
        const double dProcHeight,
        const double dInputMinX,
        const double dInputMinY,
        const int nX,
        const int nY,
        std::vector<CRoadData> &dstData);

    // エラーチェック
    std::vector<std::vector<std::string>> errorCheck(
        std::vector<CRoadData> &roadData,
        BoostMultiPolygon &roadPolygons,
        std::vector<CCrossingData> &crosses,
        BoostBox &targetArea);

};

