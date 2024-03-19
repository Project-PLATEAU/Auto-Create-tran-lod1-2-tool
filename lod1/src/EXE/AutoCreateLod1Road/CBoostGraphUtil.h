#pragma once
#include "AnalyzeRoadEdgeCommon.h"


/*!
 * @brief グラフのユーティリティクラス
*/
class CBoostGraphUtil
{
public:
    /*!
     * @brief エッジの長さ
     * @param[in] graph     無向グラフ
     * @param[in] edgeDesc  エッジのデスクリプター
     * @return    エッジの長さ
    */
    static double EdgeLength(const BoostUndirectedGraph &graph, BoostEdgeDesc &edgeDesc)
    {
        BoostPolyline line;
        line.push_back(graph[graph[edgeDesc].vertexDesc1].pt);
        line.push_back(graph[graph[edgeDesc].vertexDesc2].pt);
        return bg::length(line);
    }

    // 無向グラフの作成
    static BoostUndirectedGraph CreateGraph(
        BoostMultiLines &lines);
};
