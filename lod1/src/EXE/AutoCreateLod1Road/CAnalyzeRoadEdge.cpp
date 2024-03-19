#include "pch.h"
#include "CAnalyzeRoadEdge.h"
#include "CAnalyzeRoadEdgeGeomUtil.h"
#include "CAnalyzeRoadEdgeDebugUtil.h"
#include "RoadModelErrorChecker.h"
#include "CReadParamFile.h"
#include "CEpsUtil.h"
#include "CConcaveHull.h"
#include "CBoostGraphUtil.h"
#include "COutputSetting.h"
#include "CFileUtil.h"
#include "CRoadDivision.h"
#include "CLog.h"
#include "boost/graph/undirected_dfs.hpp"
#include "boost/graph/dijkstra_shortest_paths.hpp"
#include <thread>

#ifdef _OUTPUT_MODEL_INTERMEDIATE_FILE
#include "CTime.h"
#include "CAnalyzeRoadEdgeDebugUtil.h"
#endif

/*!
 * @brief 処理対象データの抽出
 * @param[in] area          対象範囲
 * @param[in] multiLines    ポリライン集合
 * @param bClip             対象範囲でクリップするか否か
 * @return 処理対象のポリライン集合
*/
BoostMultiLines CAnalyzeRoadEdge::ExtractTargetData(
    const BoostBox &area,
    const BoostMultiLines &multiLines,
    const bool bClip)
{
    // 範囲内のポリラインを取得
    BoostPolygon areaPolygon;
    bg::convert(area, areaPolygon);
    BoostMultiPolygon polygons;
    polygons.push_back(areaPolygon);
    return ExtractTargetData(polygons, multiLines, bClip);
}

/*!
 * @brief 処理対象データの抽出
 * @param[in] area          対象範囲
 * @param[in] pairLines     ペアポリライン集合
 * @param bClip             対象範囲でクリップするか否か
 * @return 処理対象のポリライン集合
*/
std::vector<BoostPairLine> CAnalyzeRoadEdge::ExtractTargetData(
    const BoostBox &area,
    const std::vector<BoostPairLine> &pairLines,
    const bool bClip)
{
    // 範囲内のポリラインを取得
    BoostPolygon areaPolygon;
    bg::convert(area, areaPolygon);

    // 範囲内のポリラインを取得
    std::vector<BoostPairLine>  targetLines;
    for (auto pairLine : pairLines)
    {
        if (bClip)
        {
            // 対象範囲でクリップする場合
            BoostMultiLines tmpLines, tmpDstLines;
            tmpLines.push_back(pairLine.first);
            tmpLines.push_back(pairLine.second);
            bg::intersection(tmpLines, areaPolygon, tmpDstLines);
            if (!bg::is_empty(tmpDstLines) && tmpDstLines.size() == 2)
            {
                BoostPairLine newPairLine(tmpDstLines[0], tmpDstLines[1]);
                targetLines.push_back(newPairLine);
            }
        }
        else
        {
            // 対象範囲でクリップしない場合は、重畳しているポリラインを取得する
            // どちらかのポリラインが対象範囲と衝突しているか確認する
            if (!bg::disjoint(areaPolygon, pairLine.first)
                || !bg::disjoint(areaPolygon, pairLine.second))
            {
                targetLines.push_back(pairLine);
            }
        }
    }

    return targetLines;
}

/*!
 * @brief 処理対象データの抽出
 * @param[in] area          対象範囲
 * @param[in] multiLines    ポリライン集合
 * @param[in] bClip         対象範囲でクリップするか否か
 * @return 処理対象のポリライン集合
*/
BoostMultiLines CAnalyzeRoadEdge::ExtractTargetData(
    const BoostMultiPolygon &area,
    const BoostMultiLines &multiLines,
    const bool bClip)
{
    // 範囲内のポリラインを取得
    BoostMultiLines targetLines;
    if (bClip)
    {
        // 対象範囲でクリップする場合
        bg::intersection(multiLines, area, targetLines);
    }
    else
    {
        // 対象範囲でクリップしない場合は、重畳しているポリラインを取得する
        for (auto line : multiLines)
        {
            // 疎であるか確認
            if (!bg::disjoint(area, line))
            {
                targetLines.push_back(line);
            }
        }
    }
    return targetLines;
}

/*!
 * @brief ポリライン整形処理
 * @param[in/out] polyline  整形対象のポリライン
 * @param[in]     dLengthTh 整形確認用の線分の長さ閾値(m)
 * @param[in]     dAngleTh  整形確認用の2辺の角度閾値(deg)
*/
void CAnalyzeRoadEdge::shapingRoadEdge(
    BoostPolyline &polyline,
    const double dLengthTh,
    const double dAngleTh)
{
    if (polyline.size() > 2)
    {
        // 終点側の確認
        CVector2D pt1((polyline.end() - 1)->x(), (polyline.end() - 1)->y());
        CVector2D pt2((polyline.end() - 2)->x(), (polyline.end() - 2)->y());
        CVector2D pt3((polyline.end() - 3)->x(), (polyline.end() - 3)->y());
        CVector2D vec1 = pt1 - pt2;
        CVector2D vec2 = pt3 - pt2;
        double dAngle = CGeoUtil::Angle(vec1, vec2);

        if (vec1.Length() < dLengthTh && dAngle <= dAngleTh)
        {
            polyline.erase(polyline.end() - 1);   // 終点の削除
        }
    }

    if (polyline.size() > 2)
    {
        // 始点側の確認
        CVector2D pt1(polyline.begin()->x(), polyline.begin()->y());
        CVector2D pt2((polyline.begin() + 1)->x(), (polyline.begin() + 1)->y());
        CVector2D pt3((polyline.begin() + 2)->x(), (polyline.begin() + 2)->y());
        CVector2D vec1 = pt1 - pt2;
        CVector2D vec2 = pt3 - pt2;
        double dAngle = CGeoUtil::Angle(vec1, vec2);

        if (vec1.Length() < dLengthTh && dAngle <= dAngleTh)
        {
            polyline.erase(polyline.begin());   // 始点の削除
        }
    }
}

/*!
 * @brief 道路縁と重畳する探索対象線の取得
 * @param[in] roadEdges     道路縁
 * @param[in] searchLines   探索対象線
 * @param[in] dBufDist      バッファサイズ(m)
 * @param[in] dLengthTh     不要線分除去用の長さ閾値(m)
 * @param[in] dLengthDiffTh 探索対象と抽出ポリラインの長さ差分閾値(m)
 * @return 重畳線群
*/
BoostMultiLines CAnalyzeRoadEdge::getOverlapEdges(
    const BoostMultiLines &roadEdges,
    const BoostMultiLines &searchLines,
    const double dBufDist,
    const double dLengthTh,
    const double dLengthDiffTh)
{
    BoostMultiLines dstLines;

    for (auto itCurrentLine = searchLines.cbegin();
        itCurrentLine != searchLines.cend(); itCurrentLine++)
    {
        // 道路縁と道路橋に微量の位置ずれを確認している
        // 道路橋にバッファを付けて重畳領域を取得できるようにする
        // 探索対象をバッファリング
        BoostMultiPolygon searchPolygons = CAnalyzeRoadEdgeGeomUtil::Buffering(*itCurrentLine, dBufDist);

        // 探索対象に重畳する道路縁を取得する
        BoostMultiLines geoms;
        bg::intersection(roadEdges, searchPolygons, geoms);
        if (!bg::is_empty(geoms))
        {
            for (BoostMultiLines::iterator itLine = geoms.begin();
                itLine != geoms.end(); itLine++)
            {
                // 探索対象線にバッファを付けた関係で余分な短い線分を取得するため除去
                // 始終点に接続しているエッジが余分な線分の場合があるため確認
                shapingRoadEdge(*itLine, dLengthTh);

                // 探索対象の長さと乖離があるポリラインは除外する
                double dDiff = abs(bg::length(*itCurrentLine) - bg::length(*itLine));
                if (dDiff > dLengthDiffTh)
                    continue;

                dstLines.push_back(*itLine);  // 道路縁と重畳するする道路橋
            }
        }
    }

    return dstLines;
}

/*!
 * @brief 道路縁と重畳する探索対象線の取得
 * @param[in] roadEdges     道路縁
 * @param[in] searchLines   探索対象線
 * @param[in] dBufDist      バッファサイズ(m)
 * @param[in] dLengthTh     不要線分除去用の長さ閾値(m)
 * @param[in] dLengthDiffTh 探索対象と抽出ポリラインの長さ差分閾値(m)
 * @return 重畳線群
*/
std::vector<BoostPairLine> CAnalyzeRoadEdge::getOverlapEdges(
    const BoostMultiLines &roadEdges,
    const std::vector<BoostPairLine> &searchLines,
    const double dBufDist,
    const double dLengthTh,
    const double dLengthDiffTh)
{
    std::vector<BoostPairLine> dstLines;

    for (auto itPair = searchLines.cbegin(); itPair != searchLines.cend(); itPair++)
    {
        BoostMultiLines tmpLines;
        tmpLines.push_back(itPair->first);
        tmpLines.push_back(itPair->second);
        BoostMultiLines tmpDstLines;
        for (auto itCurrentLine = tmpLines.cbegin();
            itCurrentLine != tmpLines.cend(); itCurrentLine++)
        {
            BoostMultiPolygon searchPolygons = CAnalyzeRoadEdgeGeomUtil::Buffering(*itCurrentLine, dBufDist);

            // 探索対象に重畳する道路縁を取得する
            BoostMultiLines geoms;
            bg::intersection(roadEdges, searchPolygons, geoms);
            if (!bg::is_empty(geoms))
            {
                for (BoostMultiLines::iterator itLine = geoms.begin();
                    itLine != geoms.end(); itLine++)
                {
                    // 探索対象線にバッファを付けた関係で余分な短い線分を取得するため除去
                    // 始終点に接続しているエッジが余分な線分の場合があるため確認
                    shapingRoadEdge(*itLine, dLengthTh);

                    // 探索対象の長さと乖離があるポリラインは除外する
                    double dDiff = abs(bg::length(*itCurrentLine) - bg::length(*itLine));
                    if (dDiff > dLengthDiffTh)
                        continue;

                    tmpDstLines.push_back(*itLine);  // 道路縁と重畳するするトンネル
                }
            }
        }

        if (tmpDstLines.size() == 2)
        {
            BoostPairLine pairLine(tmpDstLines[0], tmpDstLines[1]);
            dstLines.push_back(pairLine);
        }
    }

    return dstLines;
}

/*!
 * @brief 立体交差探索
 * @param[in/out]   roadEdges       道路縁
 * @param[in]       bridges         道路橋(高架部)
 * @param[in]       tunnels         トンネル
 * @param[out]      upperRoadEdge   上層の道路縁
 * @param[out]      middleRoadEdge  中層の道路縁
 * @param[out]      lowerRoadEdge   下層の道路縁
 * @param[in]       dLengthTh       道路縁抽出時の長さ閾値(m)
*/
void CAnalyzeRoadEdge::searchMultiLevelCrossing(
    BoostMultiLines &roadEdges,
    const BoostMultiLines &bridges,
    const std::vector<BoostPairLine> &tunnels,
    BoostMultiLines &upperRoadEdge,
    BoostMultiLines &middleRoadEdge,
    std::vector<BoostPairLine> &lowerRoadEdge,
    const double dLengthTh)
{
    const double dDiffLengthTh = 0.001; // 道路縁と頂上している道路橋orトンネル判定用の長さ差分閾値(m)
    const double dBufDist = 0.01;       // バッファサイズ(m)
    bg::strategy::buffer::distance_symmetric<double> distStrategy(dBufDist);
    bg::strategy::buffer::join_miter joinStrategy;
    bg::strategy::buffer::end_flat endStrategy;
    bg::strategy::buffer::point_circle pointStrategy;
    bg::strategy::buffer::side_straight sideStrategy;

    middleRoadEdge = BoostMultiLines(roadEdges);

    if (bridges.size() > 0)
    {
        // 道路縁と道路橋に微量の位置ずれを確認
        // 道路橋にバッファを付けて重畳領域を取得できるようにする
        BoostMultiPolygon bridgePolygons;
        bg::buffer(
            bridges, bridgePolygons, distStrategy, sideStrategy,
            joinStrategy, endStrategy, pointStrategy);

        // 道路橋に重畳する道路縁を取得する
        BoostMultiLines geoms;
        bg::intersection(roadEdges, bridgePolygons, geoms);
        if (!bg::is_empty(geoms))
        {
            for (BoostMultiLines::iterator itLine = geoms.begin();
                itLine != geoms.end(); itLine++)
            {
                // 道路橋にバッファを付けた関係で余分な短い線分を取得するため除去
                // 始終点に接続しているエッジが余分な線分の場合があるため確認
                shapingRoadEdge(*itLine, dLengthTh);

                // 短いポリラインは除外する
                if (bg::length(*itLine) < dLengthTh)
                    continue;

                upperRoadEdge.push_back(*itLine);
            }
        }

        //道路橋の選別
        std::vector<BoostMultiLines::const_iterator> vecDelete;
        for (BoostMultiLines::const_iterator itLine = upperRoadEdge.cbegin();
            itLine != upperRoadEdge.cend(); itLine++)
        {
            // 交線候補の取得
            BoostMultiPolygon tmpPolygon = CAnalyzeRoadEdgeGeomUtil::Buffering(*itLine, dBufDist);
            const double dLength = bg::length(*itLine);
            BoostMultiLines polylines;
            bg::intersection(tmpPolygon, roadEdges, polylines);
            if (!bg::is_empty(polylines))
            {
                bool bCross = false;
                for (BoostMultiLines::const_iterator itTmpLine = polylines.cbegin(); itTmpLine != polylines.cend(); itTmpLine++)
                {
                    double dTmpLength = bg::length(*itTmpLine);
                    double diff = abs(dTmpLength - dLength);
                    if (CEpsUtil::Less(diff, dDiffLengthTh))
                    {
                        // 道路橋と重畳している道路縁のためスキップ
                        continue;
                    }

                    // 交線候補との交点を取得
                    BoostMultiPoints pts;
                    bg::intersection(*itLine, *itTmpLine, pts);
                    if (!bg::is_empty(pts))
                    {
                        for (BoostMultiPoints::const_iterator itPt = pts.cbegin();
                            itPt != pts.cend(); itPt++)
                        {
                            if (!CAnalyzeRoadEdgeGeomUtil::CheckPointEqual(*itPt, *(itLine->begin()))
                                && !CAnalyzeRoadEdgeGeomUtil::CheckPointEqual(*itPt, *(itLine->end() - 1)))
                            {
                                // 道路橋の始終点以外で交差が発生する場合
                                bCross = true;
                                break;
                            }
                        }
                    }
                    if (bCross)
                        break;
                }
                if (!bCross)
                    vecDelete.push_back(itLine);
            }
        }
        for (auto it = vecDelete.crbegin(); it != vecDelete.crend(); it++)
            upperRoadEdge.erase(*it);

        if (upperRoadEdge.size() > 0)
        {
            BoostMultiLines diffPolylines;
            bg::difference(middleRoadEdge, upperRoadEdge, diffPolylines);
            middleRoadEdge = diffPolylines;
        }
    }

    // トンネルを利用した階層分離
    if (tunnels.size() > 0)
    {
        //トンネルの選別
        BoostMultiLines tmpLowerRoadEdge;
        for (auto itPair = tunnels.begin(); itPair != tunnels.end(); itPair++)
        {
            BoostMultiLines lines;
            lines.push_back(itPair->first);
            lines.push_back(itPair->second);

            std::vector<bool> vecCross;
            for (auto itLine = lines.begin(); itLine != lines.end(); itLine++)
            {
                bool bCross = false;

                // 交線候補の取得
                BoostMultiPolygon tmpPolygon = CAnalyzeRoadEdgeGeomUtil::Buffering(*itLine, dBufDist);
                const double dLength = bg::length(*itLine);
                BoostMultiLines polylines;
                bg::intersection(tmpPolygon, roadEdges, polylines);
                if (!bg::is_empty(polylines))
                {
                    for (BoostMultiLines::const_iterator itTmpLine = polylines.cbegin(); itTmpLine != polylines.cend(); itTmpLine++)
                    {
                        double dTmpLength = bg::length(*itTmpLine);
                        double diff = abs(dTmpLength - dLength);
                        if (CEpsUtil::Less(diff, dDiffLengthTh))
                        {
                            // トンネルと重畳している道路縁のためスキップ
                            continue;
                        }

                        // 交線候補との交点を取得
                        BoostMultiPoints pts;
                        bg::intersection(*itLine, *itTmpLine, pts);
                        if (!bg::is_empty(pts))
                        {
                            for (BoostMultiPoints::const_iterator itPt = pts.cbegin();
                                itPt != pts.cend(); itPt++)
                            {
                                if (!CAnalyzeRoadEdgeGeomUtil::CheckPointEqual(*itPt, *(itLine->begin()))
                                    && !CAnalyzeRoadEdgeGeomUtil::CheckPointEqual(*itPt, *(itLine->end() - 1)))
                                {
                                    // トンネルの始終点以外で交差が発生する場合
                                    bCross = true;
                                    break;
                                }
                            }
                        }
                    }
                }
                vecCross.push_back(bCross);
            }

            if (vecCross[0] && vecCross[1])
            {
                // 立体交差しているトンネルの場合
                lowerRoadEdge.push_back(*itPair);
                tmpLowerRoadEdge.push_back(itPair->first);
                tmpLowerRoadEdge.push_back(itPair->second);
            }
        }

        if (tmpLowerRoadEdge.size() > 0)
        {
            BoostMultiLines diffPolylines;
            bg::difference(middleRoadEdge, tmpLowerRoadEdge, diffPolylines);
            middleRoadEdge = diffPolylines;
        }
    }
}

/*!
 * @brief 道路縁のループ化処理時の外輪郭補正処理
 * @param[in/out]   vecPolyline     補正対象の開路頂点列
 * @param[in]       outlinePolygon  補正に使用する外輪郭ポリゴン
*/
void CAnalyzeRoadEdge::outerRingCorrection(
    std::vector<BoostPoint> &vecPolyline,
    const BoostPolygon &outlinePolygon)
{
    // 探索経路が閉路であるか否か
    bool bClose = CAnalyzeRoadEdgeGeomUtil::CheckPointEqual(
        *vecPolyline.begin(), *(vecPolyline.end() - 1));
    if (!bClose)
    {
        // 開路の場合
        // データ境界による切断確認
        std::vector<BoostPoint> vecSrc;
        vecSrc.push_back(*vecPolyline.begin());      // 開路の始点
        vecSrc.push_back(*(vecPolyline.end() - 1));  // 開路の終点
        std::vector<int> vecTargetIdx;
        for (auto itSrcPt = vecSrc.begin(); itSrcPt != vecSrc.end(); itSrcPt++)
        {
            int targetIdx = -1;
            for (size_t t = 0; t < outlinePolygon.outer().size() - 1; t++)
            {
                BoostPolyline line;
                line.push_back(outlinePolygon.outer()[t]);
                line.push_back(outlinePolygon.outer()[t + 1]);
                if (!bg::disjoint(line, *itSrcPt))
                {
                    // 接触する場合
                    targetIdx = static_cast<int>(t);
                }
            }
            vecTargetIdx.push_back(targetIdx);
        }

        if (vecTargetIdx[0] != vecTargetIdx[1])
        {
            // データ境界の外輪郭線上の同一辺で分断されていない
            // 隣り合う2本の外輪郭線で切断されているか確認する
            int nMinIdx = vecTargetIdx[0];
            int nMaxIdx = vecTargetIdx[1];
            if (nMinIdx > nMaxIdx)
            {
                nMinIdx = vecTargetIdx[1];
                nMaxIdx = vecTargetIdx[0];
            }
            if (nMinIdx > -1)
            {
                BoostPolyline line1, line2;
                line1.push_back(*(outlinePolygon.outer().data() + nMinIdx));
                line1.push_back(*(outlinePolygon.outer().data() + nMinIdx + 1));
                line2.push_back(*(outlinePolygon.outer().data() + nMaxIdx));
                line2.push_back(*(outlinePolygon.outer().data() + nMaxIdx + 1));

                if (bg::intersects(line1, line2))
                {
                    // ポリラインと接触する外輪郭の辺同士が交差する
                    BoostMultiPoints crossPts;
                    bg::intersection(line1, line2, crossPts);
                    if (!bg::is_empty(crossPts))
                        vecPolyline.push_back(crossPts[0]);
                }
            }

            // TODO 離れている外輪郭線で切断されている場合が未考慮
        }
        //else
        //{
        //    // データ境界の外輪郭線上の同一辺で分断されているため始終点をつないで閉路にしてよい
        //    // 後段処理のbg::correctで始終点を同一にする補正が入るため何もしない
        //}
    }
}

/*!
 * @brief 道路縁のループ化
 * @param[in]  roadEdge         道路縁
 * @param[out] blocks           街区ポリゴン群
 * @param[out] errBlocks        エラー街区ポリゴン群
 * @param[in]  dAreaTh          街区の面積閾値
 * @param[in]  dOpeningBuffer   オープニング処理のバッファ距離
*/
void CAnalyzeRoadEdge::looping(
    BoostMultiLines &roadEdge,
    BoostMultiPolygon &blocks,
    BoostMultiPolygon &errBlocks,
    const double dAreaTh,
    const double dOpeningBuffer)
{
    // 入力道路縁のバウンディングボックス
    BoostBox bbox;
    bg::envelope(roadEdge, bbox);
    BoostPolygon areaPolygon;
    bg::convert(bbox, areaPolygon); // ポリゴン化

    // 無向グラフの作成
    BoostUndirectedGraph graph = CBoostGraphUtil::CreateGraph(roadEdge);

    // 深さ優先探索を使用して経路探索
    // 開路から探索
    std::vector<BoostVertexDesc> vecSearchDesc, vecCloseDesc;
    BOOST_FOREACH (BoostVertexDesc desc, boost::vertices(graph))
    {
        // 頂点の次数
        if (boost::degree(desc, graph) == 1)
        {
            // 開路の端点の場合
            vecSearchDesc.push_back(desc);
        }
        else if (boost::degree(desc, graph) == 2)
        {
            // 閉路上の点(次数3以上は立体交差していると考え一旦除外)
            vecCloseDesc.push_back(desc);
        }
    }

    // 開路、閉路の点の順に深さ優先探索行う
    vecSearchDesc.insert(vecSearchDesc.end(), vecCloseDesc.begin(), vecCloseDesc.end());
    std::vector<boost::default_color_type> vecVertexColor(boost::num_vertices(graph));
    auto idmap = get(boost::vertex_index, graph);
    auto vcmap = boost::make_iterator_property_map(vecVertexColor.begin(), idmap);
    for (auto it = vecSearchDesc.begin(); it != vecSearchDesc.end(); it++)
    {
        if (graph[*it].isSearched)
            continue;   // 探索済み
        std::vector<BoostVertexDesc> vecRoute;  // 経路
        CBoostDFSVisitor vis(vecRoute);
        boost::depth_first_visit(graph, *it, vis, vcmap);

        std::vector<BoostPoint> vecPts;
        for (auto itDesc = vecRoute.begin(); itDesc != vecRoute.end(); itDesc++)
        {
            graph[*itDesc].isSearched = true;   //探索済みに更新
            vecPts.push_back(graph[*itDesc].pt);   // ポリゴン作成のため頂点追加
        }

        // 街区ポリゴン化
        BoostPolygon polygon;
        std::copy(vecPts.begin(), vecPts.end(), std::back_inserter(polygon.outer()));
        bg::unique(polygon);
        bg::correct(polygon);
        // 自己交差の確認と解消
        BoostMultiPolygon newPolygons;
        CAnalyzeRoadEdgeGeomUtil::SelfIntersectionResolution(polygon, newPolygons);
        if (newPolygons.size() > 0)
        {
            // 面積最大のポリゴンを取得
            BoostMultiPolygon::iterator itTarget = newPolygons.begin();
            for (auto itTmp = newPolygons.begin(); itTmp != newPolygons.end(); itTmp++)
            {
                if (CEpsUtil::Greater(bg::area(*itTmp), bg::area(*itTarget)))
                {
                    itTarget = itTmp;
                }
            }

            // スパイクノイズ対策
            BoostMultiPolygon openingPolygons = opening(*itTarget, dOpeningBuffer);

            if (openingPolygons.size() > 0
                && bg::is_valid(openingPolygons.front()) &&
                CEpsUtil::GreaterEqual(CAnalyzeRoadEdgeGeomUtil::RoundN(bg::area(openingPolygons.front()), 1), dAreaTh))
                blocks.push_back(openingPolygons.front());
        }
    }
}

/*!
 * @brief オープニング処理
 * @param[in] polygon ポリゴン
 * @param[in] dBuffer オープニング処理のバッファサイズ
 * @return オープニング後のポリゴン
 * @note    収縮膨張時に自己交差が発生する場合があるため自己交差解消処理も搭載
*/
BoostMultiPolygon CAnalyzeRoadEdge::opening(
    BoostPolygon &polygon,
    const double dBuffer)
{
    // バッファリングストラテジー
    bg::strategy::buffer::distance_symmetric<double> shrinkDistStrategy(-dBuffer);
    bg::strategy::buffer::distance_symmetric<double> expansionDistStrategy(dBuffer);
    bg::strategy::buffer::join_miter joinStrategy;
    bg::strategy::buffer::end_flat endStrategy;
    bg::strategy::buffer::point_circle pointStrategy;
    bg::strategy::buffer::side_straight sideStrategy;

    //収縮時に穴が外輪郭線を超える場合にbufferの挙動がおかしくなる(外輪郭が消失し穴が外輪郭のポリゴンになる)ための対策
    BoostPolygon outerPolygon;
    std::copy(polygon.outer().begin(), polygon.outer().end(), std::back_inserter(outerPolygon.outer()));
    bg::correct(outerPolygon);
    BoostMultiPolygon innerPolygons;
    for (auto itInner = polygon.inners().begin(); itInner != polygon.inners().end(); itInner++)
    {
        BoostPolygon hole;
        std::copy(itInner->begin(), itInner->end(), std::back_inserter(hole.outer()));
        bg::correct(hole);
        innerPolygons.push_back(hole);
    }

    // 外輪郭は収縮、穴は膨張
    BoostMultiPolygon tmpOuter, tmpInners, shrinkPolygons, tmpShrinkPolygons;
    bg::buffer(
        outerPolygon, tmpOuter, shrinkDistStrategy, sideStrategy,
        joinStrategy, endStrategy, pointStrategy);
    bg::unique(tmpOuter);
    bg::correct(tmpOuter);
    // 収縮した際に自己交差したポリゴンが消失する場合の対応
    double dDiffArea = abs(bg::area(outerPolygon) - bg::area(tmpOuter));
    double dDiffAreaRatio = CAnalyzeRoadEdgeGeomUtil::RoundN(
        dDiffArea / bg::area(outerPolygon), 3);
    if (CEpsUtil::GreaterEqual(dDiffAreaRatio, 0.8))
    {
        // ポリゴンが消失したため入力ポリゴンを分割する
        // 残存ポリゴンを膨張
        BoostMultiPolygon tmpRemaining;
        bg::buffer(
            tmpOuter, tmpRemaining, expansionDistStrategy, sideStrategy,
            joinStrategy, endStrategy, pointStrategy);
        bg::unique(tmpRemaining);
        bg::correct(tmpRemaining);
        // 分割
        BoostMultiPolygon outerPolygons;
        bg::difference(outerPolygon, tmpRemaining, outerPolygons);
        outerPolygons.insert(outerPolygons.end(), tmpRemaining.begin(), tmpRemaining.end());
        // 再縮小
        bg::buffer(
            outerPolygons, tmpOuter, shrinkDistStrategy, sideStrategy,
            joinStrategy, endStrategy, pointStrategy);
        bg::unique(tmpOuter);
        bg::correct(tmpOuter);
    }

    bg::buffer(
        innerPolygons, tmpInners, expansionDistStrategy, sideStrategy,
        joinStrategy, endStrategy, pointStrategy);

    bg::unique(tmpInners);
    bg::correct(tmpInners);
    bg::difference(tmpOuter, tmpInners, tmpShrinkPolygons);
    // 水平線上に存在する頂点の削除(不要点が存在すると膨張収縮時に自己交差が発生することを確認)
    shrinkPolygons = CAnalyzeRoadEdgeGeomUtil::Simplify(tmpShrinkPolygons);

    BoostMultiPolygon srcErosionPolygons;
    for (auto itShrink = shrinkPolygons.begin();
        itShrink != shrinkPolygons.end(); itShrink++)
    {
        if (itShrink->outer().size() < 4)
            continue;

        if (bg::is_valid(*itShrink))
        {
            srcErosionPolygons.push_back(*itShrink);
        }
        else
        {
            // 無効ポリゴンの場合は自己交差の確認/解消
            BoostMultiPolygon polygons;
            CAnalyzeRoadEdgeGeomUtil::SelfIntersectionResolution(*itShrink, polygons);
            srcErosionPolygons.insert(srcErosionPolygons.end(), polygons.begin(), polygons.end());
        }
    }

    // 膨張
    BoostMultiPolygon erosionPolygons, tmpErosionPolygons;
    bg::buffer(
        srcErosionPolygons, tmpErosionPolygons, expansionDistStrategy, sideStrategy,
        joinStrategy, endStrategy, pointStrategy);
    bg::unique(tmpErosionPolygons);
    bg::correct(tmpErosionPolygons);
    erosionPolygons = CAnalyzeRoadEdgeGeomUtil::Simplify(tmpErosionPolygons);

    BoostMultiPolygon dstPolygons;
    for (auto itErosion = erosionPolygons.begin(); itErosion != erosionPolygons.end(); itErosion++)
    {
        if (itErosion->outer().size() < 4)
            continue;

        if (bg::is_valid(*itErosion))
        {
            dstPolygons.push_back(*itErosion);
        }
        else
        {

            // 無効ポリゴンの場合は自己交差の確認/解消
            BoostMultiPolygon polygons;
            CAnalyzeRoadEdgeGeomUtil::SelfIntersectionResolution(*itErosion, polygons);
            dstPolygons.insert(dstPolygons.end(), polygons.begin(), polygons.end());
        }
    }
    return dstPolygons;
}

/*!
 * @brief ポリゴンの減算処理(スパイクノイズ/自己交差解消機能付き)
 * @param[in] polygons          被減算領域
 * @param[in] polygon           減算領域
 * @param[in] dOpeningBuffer    オープニング処理のバッファ距離
 * @return 減算結果
*/
BoostMultiPolygon CAnalyzeRoadEdge::difference(
    BoostMultiPolygon &polygons,
    BoostPolygon &polygon,
    const double dOpeningBuffer)
{
    BoostMultiPolygon diffPolygons;
    for (auto itPoly = polygons.begin(); itPoly != polygons.end(); itPoly++)
    {
        if (bg::disjoint(*itPoly, polygon))
        {
            // 衝突しない場合
            diffPolygons.push_back(*itPoly);
        }
        else
        {
            // 領域を減算
            BoostMultiPolygon tmpDiff;
            if (bg::within(polygon, *itPoly))
            {
                BoostPolygon diffPolygon(*itPoly);
                diffPolygon.inners().push_back(polygon.outer());
                bg::correct(diffPolygon);
                bg::unique(diffPolygon);

                // スパイクノイズ対策
                BoostMultiPolygon openingPolygons = opening(diffPolygon, dOpeningBuffer);
                diffPolygons.insert(diffPolygons.end(), openingPolygons.begin(), openingPolygons.end());
            }
            else
            {
                bg::difference(*itPoly, polygon, tmpDiff);
                for (auto itDiff = tmpDiff.begin(); itDiff != tmpDiff.end(); itDiff++)
                {
                    // スパイクノイズ対策
                    BoostMultiPolygon openingPolygons = opening(*itDiff, dOpeningBuffer);
                    diffPolygons.insert(diffPolygons.end(), openingPolygons.begin(), openingPolygons.end());
                }
            }
        }
    }

    return diffPolygons;
}

/*!
 * @brief 道路ポリゴン作成
 * @param[in]   roadedges           道路縁
 * @param[in]   blocks              街区ポリゴン群
 * @param[out]  roadPolygons        道路ポリゴン群
 * @param[out]  concaveHull         凹包結果
 * @param[in]   dSampling           道路縁のサンプリング間隔
 * @param[in]   dAreaTh             道路ポリゴンの面積閾値
 * @param[in]   dOpeningBuffer      オープニング処理のバッファサイズ
*/
void CAnalyzeRoadEdge::createRoadPolygon(
    const BoostMultiLines &roadedges,
    const BoostMultiPolygon &blocks,
    BoostMultiPolygon &roadPolygons,
    BoostPolygon &concaveHull,
    const double dSampling,
    const double dAreaTh,
    const double dOpeningBuffer)
{
    roadPolygons.clear();
    const double dAreaEpsilon = 0.01;   // 面積誤差許容値(率)
    const double dConcavity = 2.25;     // 凹包の複雑度

    // 凹包用にサンプリング
    BoostMultiLines samplingEdges = CAnalyzeRoadEdgeGeomUtil::Sampling(roadedges, dSampling);

    // 凹包
    concaveHull = CConcaveHull::ConcaveHull(samplingEdges, dConcavity);

    // 道路ポリゴンの作成
    BoostPolygon roadPolygon(concaveHull);
    BoostMultiPolygon tmpSrcRoadPolygons = opening(roadPolygon, dOpeningBuffer);

    // 減算対象領域の取得
    BoostMultiPolygon subtrahend;
    for (BoostMultiPolygon::const_iterator it = blocks.cbegin();
        it != blocks.cend(); it++)
    {
        BoostMultiPolygon andRegion;
        bg::intersection(tmpSrcRoadPolygons, *it, andRegion);

        for (auto itAndRegion = andRegion.begin(); itAndRegion != andRegion.end(); itAndRegion++)
        {
            // スパイクノイズ対策
            BoostMultiPolygon openingPolygons = opening(*itAndRegion, dOpeningBuffer);

            for (auto itOpening = openingPolygons.begin(); itOpening != openingPolygons.end(); itOpening++)
            {
                BoostMultiPoints spikePts;
                if (bg::is_valid(*itOpening)
                    && !CAnalyzeRoadEdgeGeomUtil::CheckSpike(*itOpening, spikePts, true))
                {
                    double dDiffArea = CAnalyzeRoadEdgeGeomUtil::RoundN(abs(bg::area(*itOpening) - bg::area(*it)), 3);
                    if (CEpsUtil::LessEqual(bg::area(*itOpening), bg::area(*it))
                        || CEpsUtil::LessEqual(dDiffArea, dAreaEpsilon))
                    {
                        subtrahend.push_back(*itOpening);
                    }
                }
            }
        }
    }
    // 外輪郭線から遠い街区から減算するようにソート
    BoostPolyline outer;
    std::copy(concaveHull.outer().begin(), concaveHull.outer().end(), std::back_inserter(outer));
    std::sort(subtrahend.begin(), subtrahend.end(), [outer](auto const &a, auto const &b) {
            return bg::distance(outer, a) > bg::distance(outer, b);
        });

    // 減算処理
    for (auto it = subtrahend.begin(); it != subtrahend.end(); it++)
    {
        BoostMultiPolygon diffPolygons = difference(tmpSrcRoadPolygons, *it, dOpeningBuffer);

        // 減算された面積
        double dDiffArea = abs(bg::area(tmpSrcRoadPolygons) - bg::area(diffPolygons));
        double dDiffAreaEpsilon = CAnalyzeRoadEdgeGeomUtil::RoundN(
            abs(dDiffArea - bg::area(*it)) / bg::area(*it), 3);
        if (CEpsUtil::LessEqual(dDiffAreaEpsilon, dAreaEpsilon))
        {
            tmpSrcRoadPolygons = diffPolygons;
        }
    }

    roadPolygons = tmpSrcRoadPolygons;
}


/*!
 * @brief shp出力処理用の注目エリアと近傍エリアの作成
 * @param[in]   nRow        入力エリアの分割行数
 * @param[in]   nColumn     入力エリアの分割列数
 * @param[in]   dProcWidth  注目エリアの幅(m)
 * @param[in]   dProcHeight 注目エリアの高さ(m)
 * @param[in]   dInputMinX  入力エリアの最小x座標
 * @param[in]   dInputMinY  入力エリアの最小y座標
 * @param[in]   nX          注目エリアの行数
 * @param[in]   nY          注目エリアの列数
 * @param[[out] areas       注目エリアと近傍エリアのポリゴン群
 * @param[[out] nTargetIdx  注目エリアポリゴンのインデックス
*/
void CAnalyzeRoadEdge::createAreas(
    const int nRow,
    const int nColumn,
    const double dProcWidth,
    const double dProcHeight,
    const double dInputMinX,
    const double dInputMinY,
    const int nX,
    const int nY,
    BoostMultiPolygon &areas,
    int &nTargetIdx)
{
    areas.clear();
    nTargetIdx = -1;
    for (int nOffsetY = -1; nOffsetY < 2; nOffsetY++)
    {
        int nTempY = nY + nOffsetY;
        if (nTempY < 0 || nRow <= nTempY)
            continue;

        for (int nOffsetX = -1; nOffsetX < 2; nOffsetX++)
        {
            int nTempX = nX + nOffsetX;
            if (nTempX < 0 || nColumn <= nTempX)
                continue;

            double dMinX = dProcWidth * static_cast<double>(nTempX) + dInputMinX;
            double dMaxX = dMinX + dProcWidth;
            double dMinY = dProcHeight * static_cast<double>(nTempY) + dInputMinY;
            double dMaxY = dMinY + dProcHeight;
            BoostBox area = BoostBox(
                BoostPoint(dMinX, dMinY), BoostPoint(dMaxX, dMaxY));
            BoostPolygon areaPolygon;
            bg::convert(area, areaPolygon); // ポリゴン化

            if (nX == nTempX && nY == nTempY)
                nTargetIdx = static_cast<int>(areas.size());    // 注目範囲のインデックスを保持
            areas.push_back(areaPolygon);
        }
    }
}

/*!
 * @brief 面積占有率の確認
 * @param[in] target    注目ポリゴン
 * @param[in] areas     範囲ポリゴン群
 * @return    面積占有率が最大の範囲ポリゴンのインデックス
*/
int CAnalyzeRoadEdge::checkAreaOccupancyRate(
    const BoostPolygon &target,
    const BoostMultiPolygon &areas)
{
    double dTargetArea = bg::area(target);
    std::vector<double> vecAreaRate;
    for (auto itArea = areas.cbegin(); itArea != areas.cend(); itArea++)
    {
        double dArea = 0;

        // 重畳領域の取得
        BoostMultiPolygon polygons;
        bg::intersection(target, *itArea, polygons);
        if (bg::is_valid(polygons))
        {
            dArea = bg::area(polygons);
        }

        double dRate = dArea / dTargetArea;
        vecAreaRate.push_back(dRate);
    }
    auto itRate = std::max_element(vecAreaRate.begin(), vecAreaRate.end());
    return static_cast<int>(std::distance(vecAreaRate.begin(), itRate));
}

/*!
 * @brief 注目エリアのポリゴンを取得する
 * @param[in]   roadData    道路情報群
 * @param[in]   nRow        入力エリアの分割行数
 * @param[in]   nColumn     入力エリアの分割列数
 * @param[in]   dProcWidth  注目エリアの幅(m)
 * @param[in]   dProcHeight 注目エリアの高さ(m)
 * @param[in]   dInputMinX  入力エリアの最小x座標
 * @param[in]   dInputMinY  入力エリアの最小y座標
 * @param[in]   nX          注目エリアの行数
 * @param[in]   nY          注目エリアの列数
 * @param[out]  dstData     注目エリアの道路データ群
*/
void CAnalyzeRoadEdge::selectPolygon(
    std::vector<CRoadData> &roadData,
    const int nRow,
    const int nColumn,
    const double dProcWidth,
    const double dProcHeight,
    const double dInputMinX,
    const double dInputMinY,
    const int nX,
    const int nY,
    std::vector<CRoadData> &dstData)
{
    // 注目範囲と近傍範囲算出
    BoostMultiPolygon areas;
    int nTargetIdx;
    createAreas(nRow, nColumn, dProcWidth, dProcHeight,
        dInputMinX, dInputMinY, nX, nY, areas, nTargetIdx);

    if (nTargetIdx > -1)
    {
        BoostPolygon targetArea = areas[nTargetIdx];   // 注目範囲

        // 選別
        for (auto itData = roadData.begin(); itData != roadData.end(); itData++)
        {
            if (bg::within(itData->Polygon(), targetArea))
            {
                // 注目範囲に内包される場合
                dstData.push_back(*itData);
            }
            else
            {

                // 注目範囲に内包されない場合
                if (bg::disjoint(itData->Polygon(), targetArea))
                    continue;   // 注目範囲と衝突しない場合はskip

                // 面積占有率の確認
                int nIdx = checkAreaOccupancyRate(itData->Polygon(), areas);
                if (nIdx == nTargetIdx)
                {
                    // 注目範囲が最大面積占有率の場合
                    dstData.push_back(*itData);
                }
            }
        }
    }
}

/*!
 * @brief 注目エリアのポリゴンを取得する
 * @param[in]   polygons    道路ポリゴン群(車道交差部,道路構造変化領域を除いたポリゴン群)
 * @param[in]   nRow        入力エリアの分割行数
 * @param[in]   nColumn     入力エリアの分割列数
 * @param[in]   dProcWidth  注目エリアの幅(m)
 * @param[in]   dProcHeight 注目エリアの高さ(m)
 * @param[in]   dInputMinX  入力エリアの最小x座標
 * @param[in]   dInputMinY  入力エリアの最小y座標
 * @param[in]   nX          注目エリアの行数
 * @param[in]   nY          注目エリアの列数
 * @param[out]  dstData     注目エリアの道路データ群
*/
void CAnalyzeRoadEdge::selectPolygon(
    BoostMultiPolygon &polygons,
    const int nRow,
    const int nColumn,
    const double dProcWidth,
    const double dProcHeight,
    const double dInputMinX,
    const double dInputMinY,
    const int nX,
    const int nY,
    std::vector<CRoadData> &dstData)
{
    // 注目範囲と近傍範囲算出
    BoostMultiPolygon areas;
    int nTargetIdx;
    createAreas(nRow, nColumn, dProcWidth, dProcHeight,
        dInputMinX, dInputMinY, nX, nY, areas, nTargetIdx);

    if (nTargetIdx > -1)
    {
        BoostPolygon targetArea = areas[nTargetIdx];   // 注目範囲

        // 選別
        for (auto itPoly = polygons.begin(); itPoly != polygons.end(); itPoly++)
        {
            if (bg::within(*itPoly, targetArea))
            {
                // 注目範囲に内包される場合
                CRoadData data;
                data.Polygon(*itPoly);
                dstData.push_back(data);
            }
            else
            {
                // 注目範囲に内包されない場合
                if (bg::disjoint(*itPoly, targetArea))
                    continue;   // 注目範囲と衝突しない場合はskip

                if (itPoly->inners().size() == 0)
                {
                    // 分割出来ているポリゴンは面積占有率を確認する
                    int nIdx = checkAreaOccupancyRate(*itPoly, areas);
                    if (nIdx == nTargetIdx)
                    {
                        // 注目範囲が最大面積占有率の場合
                        CRoadData data;
                        data.Polygon(*itPoly);
                        dstData.push_back(data);
                    }
                }
                else
                {
                    // 道路分割の精度が良くないため
                    // 穴がついている未分割のポリゴンは注目領域でクリップして出力する
                    BoostMultiPolygon clipPolygons;
                    bg::intersection(*itPoly, targetArea, clipPolygons);
                    for (auto itClipPoly = clipPolygons.begin(); itClipPoly != clipPolygons.end(); itClipPoly++)
                    {
                        CRoadData data;
                        data.Polygon(*itClipPoly);
                        dstData.push_back(data);
                    }
                }
            }
        }
    }
}

/*!
 * @brief 道路縁解析処理
 * @param[out]  errMsg          エラーモデルメッセージ
 * @param[in]   targetRoadEdges 道路縁
 * @param[in]   targetBridges   道路橋
 * @param[in]   targetTunnels   トンネル
 * @param[in]   targetProcArea  注目領域
 * @param[in]   dInputMinX      道路縁の最小x座標
 * @param[in]   dInputMinY      道路縁の最小y座標
 * @param[in]   dProcWidth      注目領域幅(m)
 * @param[in]   dProcHeight     注目領域高さ(m)
 * @param[in]   nRow            領域行数
 * @param[in]   nColumn         領域列数
 * @param[in]   nX              注目領域行数
 * @param[in]   nY              注目領域列数
 * @return      道路ポリゴンデータ群
*/
std::vector<CRoadData> CAnalyzeRoadEdge::Process(
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
    const int nY)
{
    CRoadDivision roadDivision;
    std::vector<CRoadData> vecOutputRoadData; // 道路ポリゴンデータ

#ifdef _OUTPUT_MODEL_INTERMEDIATE_FILE
    CAnalyzeRoadEdgeDebugUtil debugUtil = CAnalyzeRoadEdgeDebugUtil();
    // 道路縁
    std::string strShpName = (boost::format("%03d_%03d_01_roadedge.shp") % nX % nY).str();
    std::string strShpPath = CFileUtil::Combine(m_strDebugFolderPath, strShpName);
    debugUtil.OutputPolylinesToShp(targetRoadEdges, strShpPath);

    // 高架橋
    strShpName = (boost::format("%03d_%03d_02_bridge.shp") % nX % nY).str();
    strShpPath = CFileUtil::Combine(m_strDebugFolderPath, strShpName);
    debugUtil.OutputPolylinesToShp(targetBridges, strShpPath);

#endif

    // 立体交差探索
    BoostMultiLines upperRoadEdges, middleRoadEdges;
    std::vector<BoostPairLine> lowerRoadEdges;
    searchMultiLevelCrossing(
        targetRoadEdges, targetBridges, targetTunnels,
        upperRoadEdges, middleRoadEdges, lowerRoadEdges);

#ifdef _OUTPUT_MODEL_INTERMEDIATE_FILE
    // 中層
    strShpName = (boost::format("%03d_%03d_03_middle.shp") % nX % nY).str();
    strShpPath = CFileUtil::Combine(m_strDebugFolderPath, strShpName);
    debugUtil.OutputPolylinesToShp(middleRoadEdges, strShpPath);

    // 上層
    strShpName = (boost::format("%03d_%03d_03_upper.shp") % nX % nY).str();
    strShpPath = CFileUtil::Combine(m_strDebugFolderPath, strShpName);
    debugUtil.OutputPolylinesToShp(upperRoadEdges, strShpPath);

    // 下層
    BoostMultiLines tmpLowerRoadEdges;
    for (auto pairEdge : lowerRoadEdges)
    {
        tmpLowerRoadEdges.push_back(pairEdge.first);
        tmpLowerRoadEdges.push_back(pairEdge.second);
    }
    strShpName = (boost::format("%03d_%03d_03_lower.shp") % nX % nY).str();
    strShpPath = CFileUtil::Combine(m_strDebugFolderPath, strShpName);
    debugUtil.OutputPolylinesToShp(tmpLowerRoadEdges, strShpPath);
#endif

    // 道路縁のループ化
    BoostMultiPolygon middlePolygons, errMiddlePolygons;
    looping(middleRoadEdges, middlePolygons, errMiddlePolygons);

#ifdef _OUTPUT_MODEL_INTERMEDIATE_FILE
    strShpName = (boost::format("%03d_%03d_04_loop_middle.shp") % nX % nY).str();
    strShpPath = CFileUtil::Combine(m_strDebugFolderPath, strShpName);
    debugUtil.OutputPolygonsToShp(middlePolygons, strShpPath);
    strShpName = (boost::format("%03d_%03d_04_loop_err_middle.shp") % nX % nY).str();
    strShpPath = CFileUtil::Combine(m_strDebugFolderPath, strShpName);
    debugUtil.OutputPolygonsToShp(errMiddlePolygons, strShpPath);

    BoostBox bbox;
    bg::envelope(middlePolygons, bbox);
    BoostPolygon areaPolygon;
    bg::convert(bbox, areaPolygon); // ポリゴン化
    BoostMultiPolygon boundingBox;
    boundingBox.push_back(areaPolygon);
    strShpName = (boost::format("%03d_%03d_04_bbox.shp") % nX % nY).str();
    strShpPath = CFileUtil::Combine(m_strDebugFolderPath, strShpName);
    debugUtil.OutputPolygonsToShp(boundingBox, strShpPath);
#endif

    // 道路ポリゴンの作成
    BoostMultiPolygon roadPolygons;
    BoostPolygon concaveHull;
    createRoadPolygon(middleRoadEdges, middlePolygons, roadPolygons, concaveHull);

#ifdef _OUTPUT_MODEL_INTERMEDIATE_FILE
    // 凹包結果
    BoostMultiPolygon polygons;
    polygons.push_back(concaveHull);
    strShpName = (boost::format("%03d_%03d_05_concavehull_middle.shp") % nX % nY).str();
    strShpPath = CFileUtil::Combine(m_strDebugFolderPath, strShpName);
    debugUtil.OutputPolygonsToShp(polygons, strShpPath);
    //道路ポリゴン
    strShpName = (boost::format("%03d_%03d_05_diff_middle.shp") % nX % nY).str();
    strShpPath = CFileUtil::Combine(m_strDebugFolderPath, strShpName);
    debugUtil.OutputPolygonsToShp(roadPolygons, strShpPath, true);
#endif

    // 道路中心線の作成
    CRoadCenterLine centerLineCreator;
    centerLineCreator.CreateCenterLine(roadPolygons, 0.5, -0.025);
    BoostMultiLines roadCenterLines = centerLineCreator.CenterLines();

#ifdef _OUTPUT_MODEL_INTERMEDIATE_FILE
    // 整形前道路中心線
    strShpName = (boost::format("%03d_%03d_06_orgRoadCenter.shp") % nX % nY).str();
    strShpPath = CFileUtil::Combine(m_strDebugFolderPath, strShpName);
    debugUtil.OutputPolylinesToShp(centerLineCreator.InternalCenterLines(), strShpPath);

    //道路ポリゴン
    strShpName = (boost::format("%03d_%03d_06_shrink_middle.shp") % nX % nY).str();
    strShpPath = CFileUtil::Combine(m_strDebugFolderPath, strShpName);
    debugUtil.OutputPolygonsToShp(centerLineCreator.ShrinkRoadPolygons(), strShpPath, true);

    // 道路中心線
    strShpName = (boost::format("%03d_%03d_06_dstRoadCenter.shp") % nX % nY).str();
    strShpPath = CFileUtil::Combine(m_strDebugFolderPath, strShpName);
    debugUtil.OutputPolylinesToShp(centerLineCreator.CenterLines(), strShpPath);

    // 交差点(近傍領域含む)
    BoostMultiPoints crossingPts = centerLineCreator.GetCrossPoints();
    strShpName = (boost::format("%03d_%03d_06_crossing.shp") % nX % nY).str();
    strShpPath = CFileUtil::Combine(m_strDebugFolderPath, strShpName);
    debugUtil.OutputMultiPointsToShp(crossingPts, strShpPath);

#endif
    // 車道交差部の分割
    std::vector<CCrossingData> crossing = centerLineCreator.GetCrossPointsWithBranchNum(targetProcArea);
    std::vector<CRoadData> roads;
    BoostMultiPolygon remainingPolygons;
    roadDivision.DivisionByCrossing(
        roadPolygons, middlePolygons, crossing, roads, remainingPolygons);

#ifdef _OUTPUT_MODEL_INTERMEDIATE_FILE
    BoostMultiPolygon voronoiPolygons, areas;
    BoostMultiPoints selectCrossing;
    for (auto it = crossing.begin(); it != crossing.end(); it++)
    {
        // ボロノイ領域
        if (!bg::is_empty(it->Cell()) && bg::is_valid(it->Cell()))
            voronoiPolygons.push_back(it->Cell());

        // 交差点領域
        if (!bg::is_empty(it->Area()) && bg::is_valid(it->Area()))
            areas.push_back(it->Area());

        // 分割対象の交差点
        selectCrossing.push_back(it->Point());
    }
    strShpName = (boost::format("%03d_%03d_07_voronoiCell.shp") % nX % nY).str();
    strShpPath = CFileUtil::Combine(m_strDebugFolderPath, strShpName);
    debugUtil.OutputPolygonsToShp(voronoiPolygons, strShpPath);
    strShpName = (boost::format("%03d_%03d_07_crossingAreas.shp") % nX % nY).str();
    strShpPath = CFileUtil::Combine(m_strDebugFolderPath, strShpName);
    debugUtil.OutputPolygonsToShp(areas, strShpPath);
    strShpName = (boost::format("%03d_%03d_07_crossing.shp") % nX % nY).str();
    strShpPath = CFileUtil::Combine(m_strDebugFolderPath, strShpName);
    debugUtil.OutputMultiPointsToShp(selectCrossing, strShpPath);

    // 車道交差部ポリゴン
    BoostMultiPolygon crossingPolygons;
    for (auto itRoad = roads.begin(); itRoad != roads.end(); itRoad++)
        crossingPolygons.push_back(itRoad->Polygon());
    strShpName = (boost::format("%03d_%03d_07_crossingPolygons.shp") % nX % nY).str();
    strShpPath = CFileUtil::Combine(m_strDebugFolderPath, strShpName);
    debugUtil.OutputPolygonsToShp(crossingPolygons, strShpPath);

    // 残存道路ポリゴン
    strShpName = (boost::format("%03d_%03d_07_remainingPolygons.shp") % nX % nY).str();
    strShpPath = CFileUtil::Combine(m_strDebugFolderPath, strShpName);
    debugUtil.OutputPolygonsToShp(remainingPolygons, strShpPath, true);
#endif

    // 道路構造変化による分割
    // 道路ポリゴンから道路縁を抽出(補正している関係で元データの道路縁と微妙に形状に差異がある)
    BoostMultiLines newRoadEdges = CAnalyzeRoadEdgeGeomUtil::GetEdges(roadPolygons);

    // 注目領域に内包される道路橋/トンネルと注目領域境界にまたがる道路/トンネル橋を抽出
    BoostMultiLines selectBridges = ExtractTargetData(targetProcArea, targetBridges);
    std::vector<BoostPairLine> selectTunnels = ExtractTargetData(targetProcArea, targetTunnels);

    // 道路縁と重畳する道路橋,トンネルの取得
    BoostMultiLines overlapBridgeds;
    std::vector<BoostPairLine> overlapTunnels;
    overlapBridgeds = getOverlapEdges(newRoadEdges, selectBridges, 0.2, 0.25, 1.0);
    overlapTunnels = getOverlapEdges(newRoadEdges, selectTunnels, 0.2, 0.25, 1.0);

#ifdef _OUTPUT_MODEL_INTERMEDIATE_FILE
    // 道路縁と重畳する高架橋
    strShpName = (boost::format("%03d_%03d_08_bridge.shp") % nX % nY).str();
    strShpPath = CFileUtil::Combine(m_strDebugFolderPath, strShpName);
    debugUtil.OutputPolylinesToShp(overlapBridgeds, strShpPath);

    // 道路縁と重畳するトンネル
    BoostMultiLines tmpOverlapTunnels;
    for (auto pairLine : overlapTunnels)
    {
        tmpOverlapTunnels.push_back(pairLine.first);
        tmpOverlapTunnels.push_back(pairLine.second);
    }
    strShpName = (boost::format("%03d_%03d_08_tunnel.shp") % nX % nY).str();
    strShpPath = CFileUtil::Combine(m_strDebugFolderPath, strShpName);
    debugUtil.OutputPolylinesToShp(tmpOverlapTunnels, strShpPath);
#endif

    // 橋梁による分割
    std::vector<CRoadData> bridgeData;
    BoostMultiPolygon bridgeRemainingPolygons;
    roadDivision.DivisionByStructualChange(
        remainingPolygons, overlapBridgeds,
        RoadSectionType::ROAD_SECTION_BRIDGE, bridgeData, bridgeRemainingPolygons);

    // トンネルによる分割
    std::vector<CRoadData> tunnelData;
    BoostMultiPolygon tunnelRemainingPolygons;
    roadDivision.DivisionByStructualChange(
        bridgeRemainingPolygons, overlapTunnels,
        RoadSectionType::ROAD_SECTION_TUNNEL, tunnelData, tunnelRemainingPolygons);

#ifdef _OUTPUT_MODEL_INTERMEDIATE_FILE
    // 橋梁ポリゴン
    BoostMultiPolygon bridgePolygons;
    for (auto itBridge = bridgeData.begin(); itBridge != bridgeData.end(); itBridge++)
        bridgePolygons.push_back(itBridge->Polygon());
    strShpName = (boost::format("%03d_%03d_08_bridgePolygons.shp") % nX % nY).str();
    strShpPath = CFileUtil::Combine(m_strDebugFolderPath, strShpName);
    debugUtil.OutputPolygonsToShp(bridgePolygons, strShpPath);

    // 残存道路ポリゴン
    strShpName = (boost::format("%03d_%03d_08_bridgeRemainingPolygons.shp") % nX % nY).str();
    strShpPath = CFileUtil::Combine(m_strDebugFolderPath, strShpName);
    debugUtil.OutputPolygonsToShp(bridgeRemainingPolygons, strShpPath, true);

    // トンネルポリゴン
    BoostMultiPolygon tunnelPolygons;
    for (auto itTunnel = tunnelData.begin(); itTunnel != tunnelData.end(); itTunnel++)
        tunnelPolygons.push_back(itTunnel->Polygon());
    strShpName = (boost::format("%03d_%03d_08_tunnelPolygons.shp") % nX % nY).str();
    strShpPath = CFileUtil::Combine(m_strDebugFolderPath, strShpName);
    debugUtil.OutputPolygonsToShp(tunnelPolygons, strShpPath);

    // 残存道路ポリゴン
    strShpName = (boost::format("%03d_%03d_08_tunnelRemainingPolygons.shp") % nX % nY).str();
    strShpPath = CFileUtil::Combine(m_strDebugFolderPath, strShpName);
    debugUtil.OutputPolygonsToShp(tunnelRemainingPolygons, strShpPath, true);
#endif

    if (roads.size() > 0)
    {
        // 注目範囲のデータを抽出
        selectPolygon(roads, nRow, nColumn, dProcWidth, dProcHeight,
            dInputMinX, dInputMinY, nX, nY, vecOutputRoadData);   // 車道交差部
        selectPolygon(bridgeData, nRow, nColumn, dProcWidth, dProcHeight,
            dInputMinX, dInputMinY, nX, nY, vecOutputRoadData);   // 橋梁
        selectPolygon(tunnelData, nRow, nColumn, dProcWidth, dProcHeight,
            dInputMinX, dInputMinY, nX, nY, vecOutputRoadData);   // トンネル
        selectPolygon(tunnelRemainingPolygons, nRow, nColumn, dProcWidth, dProcHeight,
            dInputMinX, dInputMinY, nX, nY, vecOutputRoadData);   // その他道路

#ifdef _OUTPUT_MODEL_INTERMEDIATE_FILE
        // 注目範囲ポリゴン
        BoostMultiPolygon results;
        for (auto itPoly = vecOutputRoadData.begin(); itPoly != vecOutputRoadData.end(); itPoly++)
            results.push_back(itPoly->Polygon());
        strShpName = (boost::format("%03d_%03d_08_results.shp") % nX % nY).str();
        strShpPath = CFileUtil::Combine(m_strDebugFolderPath, strShpName);
        debugUtil.OutputPolygonsToShp(results, strShpPath, true);
#endif

        // エラーチェック
        errMsg = errorCheck(
            vecOutputRoadData, roadPolygons, crossing, targetProcArea);
    }
    else
    {
        // 道路分割に失敗した場合、注目領域の道路ポリゴンを出力
        BoostMultiPolygon clipPolygons;
        bg::intersection(roadPolygons, targetProcArea, clipPolygons);
        for (auto itPolygon = clipPolygons.begin(); itPolygon != clipPolygons.end(); itPolygon++)
        {
            CRoadData data;
            data.Polygon(*itPolygon);
            vecOutputRoadData.push_back(data);
        }
    }

    return vecOutputRoadData;
}

/*!
 * @brief エラーチェック
 * @param[in] roadData      分割後の道路ポリゴンデータ
 * @param[in] roadPolygons  分割前の道路ポリゴン
 * @param[in] crosses       交差点座標群
 * @param[in] targetArea    注目エリア
 * @return    エラーメッセージ
*/
std::vector<std::vector<std::string>> CAnalyzeRoadEdge::errorCheck(
    std::vector<CRoadData> &roadData,
    BoostMultiPolygon &roadPolygons,
    std::vector<CCrossingData> &crosses,
    BoostBox &targetArea)
{
    // 分割後道路ポリゴンをエラーチェック用データに変換
    std::vector<CreatedRoadModelInfo> modelInfos;
    for (auto itData = roadData.begin(); itData != roadData.end(); itData++)
    {
        if (itData->Polygon().inners().size() == 0) // 穴なしのみ
        {
            CreatedRoadModelInfo info(*itData, true);
            modelInfos.push_back(info);
        }
    }

    // 分割前ポリゴンを注目領域でクリップ
    BoostMultiPolygon targetPolygons;
    bg::intersection(roadPolygons, targetArea, targetPolygons);

    // 注目領域内(境界含む)の交差点座標を取得
    BoostMultiPoints targetCrosses;
    for (auto itCross = crosses.begin(); itCross != crosses.end(); itCross++)
    {
        if (bg::covered_by(itCross->Point(), targetArea))
            targetCrosses.push_back(itCross->Point());
    }

    // エラーチェック
    RoadModelData modelData(modelInfos, targetPolygons, targetCrosses);
    std::vector<std::vector<std::string>> errMsg = RoadModelErrorChecker::Run(
        modelData, GetCreateParam()->GetMinArea(), GetCreateParam()->GetMaxDistance());
    return errMsg;
}


/*!
 * @brief 処理領域数の算出
 * @param[in] roadEdges     道路縁集合データ
 * @param[in] dProcWidth    処理範囲幅(m)
 * @param[in] dProcHeight   処理範囲高さ(m)
 * @param[out] nRow         処理範囲行数
 * @param[out] nColumn      処理範囲列数
 * @param[out] dMinX        処理範囲の最小x座標
 * @param[out] dMinY        処理範囲の最小y座標
*/
void CAnalyzeRoadEdgeManager::calcProcNum(
    const BoostMultiLines &roadEdges,
    const double dProcWidth,
    const double dProcHeight,
    int &nRow, int &nColumn,
    double &dMinX, double &dMinY)
{
    // 入力範囲の取得
    BoostBox inputArea;
    bg::envelope(roadEdges, inputArea);
    dMinX = inputArea.min_corner().x();
    dMinY = inputArea.min_corner().y();
    double dInputAreaWidth = abs(inputArea.max_corner().x() - inputArea.min_corner().x());
    double dInputAreaHeight = abs(inputArea.max_corner().y() - inputArea.min_corner().y());

    assert(dProcWidth > 0 && dProcHeight > 0);
    // 処理領域に分割した際に行列数
    nRow = static_cast<int>(ceil(dInputAreaHeight / dProcHeight));
    nColumn = static_cast<int>(ceil(dInputAreaWidth / dProcWidth));
}

/*!
 * @brief 道路ポリゴン出力
 * @param[in] strShpPath    出力shpファイルパス
 * @param[in] polygons      ポリゴンデータ群
 * @param[in] bHole         穴の付与フラグ
 * @return  処理結果
 * @retval  true            成功
 * @retval  false           失敗
*/
bool CAnalyzeRoadEdgeManager::outputRoadPolygons(
    const std::string &strShpPath,
    std::vector<CRoadData> polygons,
    const bool bHole)
{
    CShapeWriter writer;
    std::vector<CShapeAttribute::AttributeFieldData> vecFields;        // 属性定義
    std::vector<CShapeAttribute::AttributeDataRecord> vecAttrRecords;  // 属性
    BoostMultiPolygon multiPolygon; // ポリゴン群

    // 属性定義
    // 道路形状属性(uro:RoadStructureAttributeのuro:sectionTypeに該当)
    CShapeAttribute::AttributeFieldData field;
    field.fieldType = CShapeAttribute::AttributeFieldType::ATTR_FIELD_TYPE_INT;
    field.strName = "sectType";
    field.nWidth = 4;
    vecFields.push_back(field);

    // 出力用データ作成
    int nId = 0;
    for (auto it = polygons.begin(); it != polygons.end(); it++, nId++)
    {
        // 形状情報
        multiPolygon.push_back(it->Polygon());
        // 属性情報
        CShapeAttribute::AttributeDataRecord record;
        record.nShapeId = nId;
        if (it->Type() == RoadSectionType::ROAD_SECTION_UNKNOWN)
        {
            record.vecAttribute.push_back(CShapeAttribute::AttributeData());
        }
        else
        {
            // 車道交差部,橋梁,トンネルなど
            record.vecAttribute.push_back(CShapeAttribute::AttributeData(static_cast<int>(it->Type())));
        }
        vecAttrRecords.push_back(record);
    }

    // output shape file
    return writer.OutputPolygons(multiPolygon, strShpPath, vecFields, vecAttrRecords, bHole);
}

/*!
 * @brief マルチスレッド用のモデル生成処理
*/
void CAnalyzeRoadEdgeManager::analyze()
{
    const double dProcBuffer = 500.0;

#ifdef _OUTPUT_MODEL_INTERMEDIATE_FILE
    std::string strDebugFolderPath;
    CAnalyzeRoadEdgeDebugUtil debugUtil = CAnalyzeRoadEdgeDebugUtil();
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        strDebugFolderPath = m_strDebugFolderPath;
    }
#endif

    // パラメータ取得
    double dInputMinX, dInputMinY, dProcWidth, dProcHeight;
    int nRow, nColumn;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        dInputMinX = m_dInputMinX;
        dInputMinY = m_dInputMinY;
        dProcWidth = m_dProcWidth;
        dProcHeight = m_dProcHeight;
        nRow = m_nRow;
        nColumn = m_nColumn;
    }

    while (!m_regions.empty())
    {
        std::pair<int, int> region;
        if (!m_regions.pop(region))
        {
            // 小領域の取得に失敗
            break;
        }
        int procIdx = region.second * nColumn + region.first + 1;
        startAnalysis(procIdx, nRow * nColumn);
        int nX = region.first;
        int nY = region.second;

#ifdef _EIGHT_NEIGHBORS_REGION
        // 8近傍範囲を取得する(縦方向)
        int nPrevY = (nY > 0) ? nY - 1 : nY;
        int nNextY = (nY < nRow - 1) ? nY + 1 : nY;
        double dMinY = dProcHeight * static_cast<double>(nPrevY) + dInputMinY;
        double dMaxY = dProcHeight * static_cast<double>(nNextY) + dInputMinY + dProcHeight;
#else
        // 近傍込みの範囲(縦方向）
        double dMinY = dProcHeight * static_cast<double>(nY) + dInputMinY - dProcBuffer;
        double dMaxY = dProcHeight * static_cast<double>(nY) + dInputMinY + dProcHeight + dProcBuffer;
#endif
        // 注目範囲を取得する(縦方向)
        double dTargetMinY = dProcHeight * static_cast<double>(nY) + dInputMinY;
        double dTargetMaxY = dTargetMinY + dProcHeight;

#ifdef _EIGHT_NEIGHBORS_REGION
        // 8近傍範囲を取得する(横方向)
        int nPrevX = (nX > 0) ? nX - 1 : nX;
        int nNextX = (nX < nColumn - 1) ? nX + 1 : nX;
        double dMinX = dProcWidth * static_cast<double>(nPrevX) + dInputMinX;
        double dMaxX = dProcWidth * static_cast<double>(nNextX) + dInputMinX + dProcWidth;
#else
        // 近傍込みの範囲(横方向）
        double dMinX = dProcWidth * static_cast<double>(nX) + dInputMinX - dProcBuffer;
        double dMaxX = dProcWidth * static_cast<double>(nX) + dInputMinX + dProcWidth + dProcBuffer;
#endif
        // 近傍範囲
        BoostBox procArea = BoostBox(BoostPoint(dMinX, dMinY), BoostPoint(dMaxX, dMaxY));

        // 注目範囲を取得する(横方向)
        double dTargetMinX = dProcWidth * static_cast<double>(nX) + dInputMinX;
        double dTargetMaxX = dTargetMinX + dProcWidth;
        // 注目範囲
        BoostBox procTargetArea = BoostBox(BoostPoint(
            dTargetMinX, dTargetMinY), BoostPoint(dTargetMaxX, dTargetMaxY));

#ifdef _OUTPUT_MODEL_INTERMEDIATE_FILE

        CAnalyzeRoadEdge analyzer(strDebugFolderPath);
#else
        CAnalyzeRoadEdge analyzer;
#endif

        // 近傍範囲内のデータを取得
        BoostMultiLines targetRoadEdges, targetBridges;
        std::vector<BoostPairLine> targetTunnels;

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            targetRoadEdges = analyzer.ExtractTargetData(procArea, m_roadEdges, true);  // 道路縁
            targetBridges = analyzer.ExtractTargetData(procArea, m_bridges);            // 道路橋
            targetTunnels = analyzer.ExtractTargetData(procArea, m_tunnels);            // トンネル
        }

        BoostMultiLines tmpLines;
        bg::intersection(procTargetArea, targetRoadEdges, tmpLines);    // 注目範囲内の道路縁
        if (tmpLines.size() == 0)
        {
            // 道路縁が存在しない場合はskip
            stopAnalysis(procIdx, nRow * nColumn, false);
            continue;
        }

        try
        {
            // 道路縁解析
            std::vector<std::vector<std::string>> errMsg;
            std::vector<CRoadData> vecOutputRoadData = analyzer.Process(
                errMsg, targetRoadEdges, targetBridges, targetTunnels, procTargetArea,
                dInputMinX, dInputMinY, dProcWidth, dProcHeight,
                nRow, nColumn, nX, nY);

            // 結果の格納
            if (vecOutputRoadData.size() > 0)
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_vecOutputRoadData.insert(
                    m_vecOutputRoadData.end(), vecOutputRoadData.begin(), vecOutputRoadData.end());

            }
            if (errMsg.size() > 0)
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_errMsg.insert(
                    m_errMsg.end(), errMsg.begin(), errMsg.end());
            }
            stopAnalysis(procIdx, nRow *nColumn, false);
        }
        catch (...)
        {
            stopAnalysis(procIdx, nRow * nColumn, true);
        }

    }
}

/*!
 * @brief 道路縁解析の開始ログ
 * @param[in] nTarget 注目領域のインデックス番号
 * @param[in] nTotal  解析対象の総数
*/
void CAnalyzeRoadEdgeManager::startAnalysis(int nTarget, int nTotal)
{
    std::string strIdx;
    m_processingIdxs.insert(nTarget, strIdx);
    std::string strMsg = (boost::format(
        "Start Analysis... %d / %d, Queue Size = %d, Now Processing : %s") % nTarget % nTotal % m_regions.size() % strIdx).str();
    GetLogger()->ConsoleLog(strMsg);
}

/*!
 * @brief 道路縁解析の終了ログ
 * @param[in] nTarget 注目領域のインデックス番号
 * @param[in] nTotal  解析対象の総数
 * @param[in] bError  エラーフラグ
*/
void CAnalyzeRoadEdgeManager::stopAnalysis(int nTarget, int nTotal, bool bError)
{
    std::string strIdx;
    m_processingIdxs.erase(nTarget, strIdx);
    std::string strMsg;
    if (bError)
    {
        strMsg = (boost::format(
            "Stop Analysis(Error)... %d / %d, Queue Size = %d, Now Processing : %s") % nTarget % nTotal % m_regions.size() % strIdx).str();
    }
    else
    {
        strMsg = (boost::format(
            "Stop Analysis... %d / %d, Queue Size = %d, Now Processing : %s") % nTarget % nTotal % m_regions.size() % strIdx).str();
    }
    GetLogger()->ConsoleLog(strMsg);
}

/*!
 * @brief 道路縁解析
 * @param[in] vecRoadEdges  道路縁データ
 * @param[in] vecBridges    道路橋データ
 * @param[in] vecTunnels    トンネルデータ
 * @param[in] dProcWidth    解析処理範囲の幅(m)
 * @param[in] dProcHeight   解析処理範囲の高さ(m)
*/
void CAnalyzeRoadEdgeManager::Process(
    const std::vector<std::vector<CVector3D>> &vecRoadEdges,
    const std::vector<std::vector<CVector3D>> &vecBridges,
    const std::vector<BoostPairLine> &vecTunnels,
    const double dProcWidth, const double dProcHeight)
{
    const int nThread = GetCreateParam()->GetThreadNum();
    const double dProcBuffer = 500.0;
    CRoadDivision roadDivision;
    m_vecOutputRoadData.clear();

    // データ変換
    m_roadEdges = CAnalyzeRoadEdgeGeomUtil::ConvBoostMultiLines(vecRoadEdges);
    m_bridges = CAnalyzeRoadEdgeGeomUtil::ConvBoostMultiLines(vecBridges);
    m_tunnels = vecTunnels; // スレッド処理用にメンバ変数に設定


    // 処理範囲数の算出
    int nRow, nColumn;      // 行列数
    double dInputMinX, dInputMinY;    // 入力道路縁のバウンディングボックの最小座標
    calcProcNum(m_roadEdges, dProcWidth, dProcHeight, nRow, nColumn, dInputMinX, dInputMinY);

#ifdef _OUTPUT_MODEL_INTERMEDIATE_FILE
    CAnalyzeRoadEdgeDebugUtil debugUtil = CAnalyzeRoadEdgeDebugUtil();
    // 現在時刻
    std::string strTime = CTime::GetCurrentTime().Format("%Y%m%d_%H%M%S");
    // 作業ディレクトリパス
    std::string strCurrentPath = CAnalyzeRoadEdgeDebugUtil::GetCurrentPath();
    // debug folder
    std::string strDebugFolderPath = CFileUtil::Combine(strCurrentPath, strTime);
    CAnalyzeRoadEdgeDebugUtil::CreateFolder(strDebugFolderPath);

    // 処理範囲の可視化
    std::string strProcAreaShpPath = CFileUtil::Combine(strDebugFolderPath, "proc_area.shp");
    debugUtil.OutputProcArea(
        strProcAreaShpPath, dInputMinX, dInputMinY, nRow, nColumn, dProcWidth, dProcHeight);

    m_strDebugFolderPath = strDebugFolderPath;  // スレッド用の設定
#endif

    // スレッド用の設定
    m_dInputMinX = dInputMinX;
    m_dInputMinY = dInputMinY;
    m_dProcWidth = dProcWidth;
    m_dProcHeight = dProcHeight;
    m_nRow = nRow;
    m_nColumn = nColumn;

    // 探索対象をqueueに詰める
    for (int nY = 0; nY < nRow; nY++)
    {
        for (int nX = 0; nX < nColumn; nX++)
        {
            std::pair<int, int> region(nX, nY);
            m_regions.push(region);
        }
    }

    // thread作成
    std::vector<std::thread> threads;
    for (int n = 0; n < nThread; n++)
    {
        threads.push_back(std::thread(&CAnalyzeRoadEdgeManager::analyze, this));
    }

    // thread終了待ち
    for (auto &th : threads)
    {
        th.join();
    }
}

/*!
 * @brief 道路ポリゴン出力
 * @return 処理結果
 * @retval true     成功
 * @retval false    失敗
*/
bool CAnalyzeRoadEdgeManager::OutputResultFile()
{
    // エラーモデル確認結果
    RoadModelErrorChecker::SaveErr(m_errMsg, GetOutputSetting()->GetErrFilePath());

    // ポリゴン出力
    outputRoadPolygons(
        GetOutputSetting()->GetShpFilePathWithHoles(),
        m_vecOutputRoadData, true);     // 穴有り

    return outputRoadPolygons(
        GetOutputSetting()->GetShpFilePath(),
        m_vecOutputRoadData, false);    // 穴無し
}