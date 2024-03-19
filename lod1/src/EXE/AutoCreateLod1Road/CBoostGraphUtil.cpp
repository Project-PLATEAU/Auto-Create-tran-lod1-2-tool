#include "pch.h"
#include "CBoostGraphUtil.h"
#include "CAnalyzeRoadEdgeGeomUtil.h"

/*!
 * @brief 無向グラフの作成
 * @param[in] lines ポリライン群
 * @return    無向グラフ
*/
BoostUndirectedGraph CBoostGraphUtil::CreateGraph(
    BoostMultiLines &lines)
{
    // 無向グラフの作成
    BoostUndirectedGraph graph;
    std::vector<BoostVertexDesc> vecDesc;
    for (BoostMultiLines::const_iterator itLine = lines.cbegin();
        itLine != lines.cend(); itLine++)
    {
        BoostVertexDesc prevDesc = 0;
        for (BoostPolyline::const_iterator itPt = itLine->cbegin();
            itPt != itLine->cend(); itPt++)
        {
            // グラフ内頂点に同一座標が存在しないか探索
            std::vector<BoostVertexDesc>::iterator itTargetDesc;
            for (itTargetDesc = vecDesc.begin(); itTargetDesc != vecDesc.end(); itTargetDesc++)
            {
                if (CAnalyzeRoadEdgeGeomUtil::CheckPointEqual(graph[*itTargetDesc].pt, *itPt))
                {
                    // 座標が等しい場合
                    break;
                }
            }

            if (itTargetDesc == vecDesc.end())
            {
                // グラフ内頂点に同一座標の頂点がない場合は追加
                BoostVertexProperty val(*itPt);
                vecDesc.push_back(boost::add_vertex(val, graph));
                itTargetDesc = vecDesc.end() - 1;
                graph[*itTargetDesc].desc = *itTargetDesc;
            }

            if (itPt > itLine->cbegin())
            {
                // エッジ追加
                auto edge = boost::add_edge(prevDesc, *itTargetDesc, graph);
                // エッジのプロパティに頂点情報を紐づける
                graph[edge.first].vertexDesc1 = prevDesc;
                graph[edge.first].vertexDesc2 = *itTargetDesc;
                graph[edge.first].dLength = CBoostGraphUtil::EdgeLength(graph, edge.first); // エッジ長
            }
            prevDesc = *itTargetDesc;   // 次エッジ追加用に1つ前の頂点情報を更新
        }
    }

    return graph;
}
