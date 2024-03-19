#pragma once

#include "AnalyzeRoadEdgeCommon.h"
#include "CGeoUtil.h"
#include <algorithm>

class CRoadCenterLine
{
public:
    CRoadCenterLine();      //!< コンストラクタ
    ~CRoadCenterLine() {}   //!< コンストラクタ

    /*!
     * @brief 道路中心線のゲッター
    */
    BoostMultiLines CenterLines() { return m_centerLines; }

    // 交差点近傍の道路中心線を取得する
    BoostMultiLines CenterLines(
        BoostPoint &crossPt,
        BoostPolygon &area);

    /*!
     * @brief 整形前道路中心線のゲッター
    */
    BoostMultiLines InternalCenterLines() { return m_internalCenterLines; }

    /*!
     * @brief 収縮道路ポリゴン群のゲッター(debug)
    */
    BoostMultiPolygon ShrinkRoadPolygons() { return m_shrinkRoadPolygons; }

    /*!
     * @brief 発見済み閉路ポリゴン群のゲッター(debug)
    */
    BoostMultiPolygon Cycles() { return m_cycles; }

    // 道路中心線作成
    void CreateCenterLine(
        const BoostMultiPolygon &roadPolygons,
        const double dReso = 0.5,
        const double dShrink = -0.025);

    // 交差点取得(座標情報のみ)
    BoostMultiPoints GetCrossPoints();

    /*!
     * @brief  交差点取得(分岐数付き)
     * @return 交差点情報群
    */
    std::vector<CCrossingData> GetCrossPointsWithBranchNum() { return m_crossings; };

    // 注目エリアの交差点取得(分岐数付き)
    std::vector<CCrossingData> GetCrossPointsWithBranchNum(const BoostBox &area);

protected:

private:
    BoostMultiLines m_centerLines;          //!< 道路中心線
    BoostMultiLines m_internalCenterLines;  //!< 整形前道路中心線
    BoostMultiPolygon m_shrinkRoadPolygons; //!< 収縮道路
    BoostMultiPolygon m_cycles;             //!< 発見した閉路
    std::vector<CCrossingData> m_crossings; //!< 交差点データ
    BoostUndirectedGraph m_graph;           //!< 道路中心線の無向グラフ
    BoostVertexRTree m_rtree; //!< 交差点探索用Tree

    // 道路中心線作成準備
    void prepareRoadCenterLine(
        const BoostMultiPolygon &roadPolygons,
        const double dSampling,
        BoostMultiLines &roadEdges);

    // ボロノイ分割線の取得
    void getVoronoiEdges(
        const BoostMultiPolygon &roadPolygons,
        const double dReso,
        const double dShrink,
        BoostMultiLines &voronoiEdges);

    // 無向グラフを利用した道路中心線のノイズ除去
    void deleteNoise(
        BoostUndirectedGraph &graph,
        const double dAngleTh = 100.0);

    // 三角形ノイズの除去
    bool deleteTriangleNoise(
        BoostUndirectedGraph &graph,
        const double dAngleTh = 100.0);

    // スパイクノイズの除去
    bool deleteSpikeNoise(
        BoostUndirectedGraph &graph,
        const double dAngleDiffTh = 5.0,
        const double dLengthTh = 10.0);

    // グラフの簡略化
    void simplifyGraph(
        BoostUndirectedGraph &graph,
        const double dAngleDiffTh = 1.0);

    // 閉路探索用の部分グラフの作成
    bool createSubGraph(
        BoostDirectedGraph &graph,
        BoostDVertexDesc vertexDesc,
        const double dBuffer,
        BoostDirectedGraph &subGraph,
        std::map<BoostDVertexDesc, BoostDVertexDesc> &mapGlobalToSub,
        std::map<BoostDVertexDesc, BoostDVertexDesc> &mapSubToGlobal);

    // 閉路ノイズ除去
    bool deleteCycleNoise(
        BoostDirectedGraph &graph,
        const double dAreaTh = 50.0);

    // 閉路ノイズ除去
    bool deleteCycleNoiseUsingSubGraph(
        BoostDirectedGraph &graph,
        const double dBuffer = 20.0,
        const double dAreaTh = 50.0);

    // 有向グラフの作成
    BoostDirectedGraph createDirectedGraph(const BoostUndirectedGraph &undirectedGraph);

    // 無向グラフの作成
    BoostUndirectedGraph createUndirectedGraph(const BoostDirectedGraph &directedGraph);


    // 交差点マージ
    void mergeCrossing(
        BoostUndirectedGraph &graph,
        const double dDistTh = 5.0);

    // 交差点のノイズ点除去
    std::vector<CCrossingData> deleteNoiseCrossing(
        const BoostUndirectedGraph &graph,
        const BoostMultiPolygon &roadPolygons,
        const double dBuffer);

};

/*!
 * @brief 線分
*/
struct CEdge
{
public:
    CVector2D m_start;    //!< 始点
    CVector2D m_end;      //!< 終点
    std::vector<CVector2D> m_middlePoints;  //!< 始終点間に挿入する中点列

    /*!
     * @brief コンストラクタ
    */
    CEdge()
    {
        m_start.x = 0;
        m_start.y = 0;
        m_end.x = 0;
        m_end.y = 0;
        m_middlePoints.clear();
    }

    /*!
     * @brief コンストラクタ
     * @param[in] start 始点
     * @param[in] end   終点
    */
    CEdge(const CVector2D &start, const CVector2D &end)
    {
        m_start = start;
        m_end = end;
    }

    /*!
     * @brief コピーコンストラクタ
     * @param[in] edge コピー元エッジ
    */
    CEdge(const CEdge &edge) { *this = edge; }

    /*!
     * @brief 代入演算子
    */
    CEdge &operator = (const CEdge &edge)
    {
        if (&edge != this)
        {
            m_start = edge.m_start;
            m_end = edge.m_end;
            std::copy(edge.m_middlePoints.begin(), edge.m_middlePoints.end(), std::back_inserter(m_middlePoints));
        }
        return *this;
    }

    /*!
     * @brief 比較演算子
    */
    bool operator < (const CEdge &edge) const
    {
        double dLen1 = (m_end - m_start).Length();
        double dLen2 = (edge.m_end - edge.m_start).Length();

        return dLen1 < dLen2;
    }

    /*!
     * @brief 比較演算子
    */
    bool operator > (const CEdge &edge) const
    {
        double dLen1 = (m_end - m_start).Length();
        double dLen2 = (edge.m_end - edge.m_start).Length();

        return dLen1 > dLen2;
    }

    CVector2D GetVector() { return m_end - m_start; }   //!< ベクトル取得
    std::vector<CVector2D> GetPoints()                  //!< ポリラインの取得
    {
        std::vector<CVector2D> pts;
        std::copy(m_middlePoints.begin(), m_middlePoints.end(), std::back_inserter(pts));
        pts.insert(pts.begin(), m_start);
        pts.push_back(m_end);
        return pts;
    }

    /*!
     * @brief 頂点の存在確認
     * @param[in] pt 頂点
     * @return 結果
     * @retval true     同一頂点が存在する
     * @retval false    同一頂点が存在しない
    */
    bool IsExitPt(const CVector2D &pt)
    {
        bool bRet = false;
        std::vector<CVector2D> pts = GetPoints();
        for (auto itPt = pts.begin(); itPt != pts.end(); itPt++)
        {
            if (CEpsUtil::Equal(itPt->x, pt.x) && CEpsUtil::Equal(itPt->y, pt.y))
            {
                bRet = true;
                break;
            }
        }
        return bRet;
    }
protected:
private:


};