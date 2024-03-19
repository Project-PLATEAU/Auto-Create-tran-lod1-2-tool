#include "pch.h"
#include "CAnalyzeTunnel.h"

/*!
 * @brief トンネル探索
 * @param[in]   roadEdges       道路縁
 * @param[in]   roadFacilities  道路施設
 * @param[in]   dBuffDist       トンネル探索領域半径(m)
 * @param[in]   dAngleDiffTh    角度差分閾値(deg)
 * @return      トンネル部分に該当する道路縁
*/
std::vector<BoostPairLine> CAnalyzeTunnel::Process(
    const std::vector<CDMRoadDataManager::RoadEdgeData> roadEdges,
    const std::vector<CDMRoadDataManager::RoadFacilitiesData> roadFacilities,
    const double dBuffDist,
    const double dAngleDiffTh)
{
    BoostMultiPoints tunnelEntrancePoints;              // トンネルの入口のポイント
    BoostMultiLines tunnelEntrancePolylines;            // トンネルの入口のポリライン
    BoostMultiLines convertedTunnelEntrancePolylines;   // トンネル入口の情報を直線に変換したポリライン
    std::vector<BoostPairPoint> tunnelBothEndsPoint;    // トンネル入口の情報を道路縁両端点に変換したポイント

    for (std::vector<CDMRoadDataManager::RoadFacilitiesData>::const_iterator it = roadFacilities.cbegin(); it != roadFacilities.cend(); it++)
    {
        // 道路施設データの内、トンネル(非区分)を抽出
        if (it->nRoadFacilitiesCode == DMRoadFacilitiesCode::ROAD_TUNNELS && it->nGeometryType == DMGeometryType::UNCLASSIFIED)
        {
            // トンネルがポイントかポリラインか判別
            if (it->nRoadFacilitiesDataType == CDMRoadDataManager::RoadFacilitiesDataType::ROAD_FACILITIES_POLYGON_DATA
                || it->nRoadFacilitiesDataType == CDMRoadDataManager::RoadFacilitiesDataType::ROAD_FACILITIES_LINE_DATA)
            {
                // ポリラインの場合は中心点を求め、ポイントに統一（要修正）
                BoostPolyline polylineVertices;
                for (const CVector3D &point : it->vecPolyline)
                {
                    polylineVertices.emplace_back(BoostPoint(point.x, point.y));
                }

                if (bg::disjoint(tunnelEntrancePolylines, polylineVertices))
                {
                    // サンプルデータに面と線で重複しているトンネルを確認したため
                    // 重複を除外する条件を追加
                    tunnelEntrancePolylines.emplace_back(polylineVertices);
                }
            }
            else
            {
                tunnelEntrancePoints.emplace_back(BoostPoint(it->vecPolyline[0].x, it->vecPolyline[0].y));
            }
        }
    }

    ///////////////////////////////////////////////////////////
    // トンネルの入口のポイントの変換
    ///////////////////////////////////////////////////////////

    // 道路縁を BoostMultiLines 型に変換
    BoostMultiLines roadEdgePolylines;
    for (auto edge : roadEdges)
    {
        for (size_t i = 0; i < edge.vecPolyline.size() - 1; i++)
        {
            BoostPolyline line;
            line.emplace_back(BoostPoint(edge.vecPolyline[i].x, edge.vecPolyline[i].y));
            line.emplace_back(BoostPoint(edge.vecPolyline[i + 1].x, edge.vecPolyline[i + 1].y));
            roadEdgePolylines.emplace_back(line);
        }
    }

    // サンプリング
    BoostMultiLines samplifyPolyline = CAnalyzeRoadEdgeGeomUtil::Sampling(roadEdgePolylines, 2.0);

    // 近傍探索用のデータ作成
    BoostMultiLinesItRTree rtree;
    BoostMultiPoints samplingPoints;
    for (size_t i = 0; i < samplifyPolyline.size(); i++)
    {
        BoostMultiLines::iterator roadEdgePolylineIt = roadEdgePolylines.begin() + i;
        BoostMultiLines::iterator samplifyPolylineIt = samplifyPolyline.begin() + i;

        for (BoostPolyline::iterator polylineIt = samplifyPolylineIt->begin(); polylineIt != samplifyPolylineIt->end(); polylineIt++)
        {
            // データ追加
            rtree.insert(std::pair<BoostPoint, BoostMultiLines::iterator>(*polylineIt, roadEdgePolylineIt));

            // サンプリングのデータ確認用
            samplingPoints.emplace_back(*polylineIt);
        }
    }

    // トンネル点の近傍道路縁の探索
    std::vector<std::pair<BoostPoint, BoostMultiLines>> rtreeResultList;
    for (BoostMultiPoints::iterator pointIt = tunnelEntrancePoints.begin();
        pointIt != tunnelEntrancePoints.end(); pointIt++)
    {
        BoostMultiLines resultLine;
        std::set<BoostMultiLines::iterator> searchedLines;

        // 実行
        std::vector<std::pair<BoostPoint, BoostMultiLines::iterator>> vecValues;
        rtree.query(bg::index::nearest(*pointIt, 14), std::back_inserter(vecValues));

        // 結果データに保存
        for (const auto &value : vecValues)
        {
            // 同じ辺を保存している場合はスキップ
            if (searchedLines.find(value.second) != searchedLines.end())
            {
                continue;
            }
            searchedLines.insert(value.second);

            resultLine.emplace_back(*value.second);
        }

        rtreeResultList.emplace_back(std::pair<BoostPoint, BoostMultiLines>(*pointIt, resultLine));
    }

    // トンネルの入口にポリラインを引く
    for (auto item : rtreeResultList)
    {
        BoostPoint targetTunnelEntrancePoint = item.first;
        BoostMultiLines targetRTreeBoostMultiLines = item.second;

        for (BoostPolyline targetRTreePolyline : targetRTreeBoostMultiLines)
        {
            bool isTunnelEntrancePolyline = false;

            for (size_t i = 0; i < targetRTreePolyline.size() - 1; i++)
            {
                BoostPolyline targetLine;
                targetLine.emplace_back(targetRTreePolyline[i]);
                targetLine.emplace_back(targetRTreePolyline[i + 1]);

                // 対象の近傍辺のベクトルを求める
                CVector2D targetRTreePolylineVec = CVector2D(
                    targetLine[1].x() - targetLine[0].x(),
                    targetLine[1].y() - targetLine[0].y());

                // トンネル入口点から対象ポリラインの垂線
                CVector2D verticalVec;
                if (CAnalyzeRoadEdgeGeomUtil::GetVerticalVec(targetRTreePolylineVec, verticalVec) == false)
                {
                    continue;
                }

                bool targetRes = false;
                bool otherRes = false;
                CVector2D targetCrossPos;
                CVector2D otherCrossPos;
                bool targetBOnLine1 = false;
                bool targetBOnLine2 = false;
                bool otherBOnLine1 = false;
                bool otherBOnLine2 = false;
                double targetT = 0.0;
                double targetS = 0.0;
                double otherT = 0.0;
                double otherS = 0.0;

                targetRes = CAnalyzeRoadEdgeGeomUtil::GetCrossPos(targetRTreePolylineVec,
                    CVector2D(targetLine[0].x(), targetLine[0].y()),
                    verticalVec,
                    CVector2D(targetTunnelEntrancePoint.x(), targetTunnelEntrancePoint.y()),
                    targetCrossPos,
                    targetBOnLine1,
                    targetBOnLine2,
                    targetT,
                    targetS);

                if (targetRes && targetBOnLine1)
                {
                    if (targetS < 0)
                    {
                        verticalVec *= -1;
                        targetS *= -1;
                    }
                }
                else
                {
                    continue;
                }

                // 他の近傍線との接線を求める
                for (BoostPolyline otherTargetRTreePolyline : targetRTreeBoostMultiLines)
                {
                    for (size_t j = 0; j < otherTargetRTreePolyline.size() - 1; j++)
                    {
                        BoostPolyline otherLine;
                        otherLine.emplace_back(otherTargetRTreePolyline[j]);
                        otherLine.emplace_back(otherTargetRTreePolyline[j + 1]);

                        // 対象ポリラインと同じ近傍線ならスキップ
                        if (bg::equals(otherLine, targetLine))
                        {
                            continue;
                        }

                        // 他の近傍辺のベクトルを求める
                        CVector2D otherTargetRTreePolylineVec = CVector2D(
                            otherLine[1].x() - otherLine[0].x(),
                            otherLine[1].y() - otherLine[0].y());

                        otherRes = CAnalyzeRoadEdgeGeomUtil::GetCrossPos(otherTargetRTreePolylineVec,
                            CVector2D(otherLine[0].x(), otherLine[0].y()),
                            -1 * verticalVec,
                            CVector2D(targetTunnelEntrancePoint.x(), targetTunnelEntrancePoint.y()),
                            otherCrossPos,
                            otherBOnLine1,
                            otherBOnLine2,
                            otherT,
                            otherS);

                        if (otherRes && otherBOnLine1 && otherS > 0)
                        {
                            break;
                        }
                    }

                    if (otherRes && otherBOnLine1 && otherS > 0)
                    {
                        break;
                    }
                }

                if (targetRes && targetBOnLine1 && targetS > 0 && otherRes && otherBOnLine1 && otherS > 0)
                {
                    // 二つの交点を求める
                    BoostPoint targetCrossPosBoostPoint = BoostPoint(targetCrossPos.x, targetCrossPos.y);
                    BoostPoint otherCrossPosBoostPoint = BoostPoint(otherCrossPos.x, otherCrossPos.y);

                    BoostPolyline line;
                    line.emplace_back(targetCrossPosBoostPoint);
                    line.emplace_back(otherCrossPosBoostPoint);

                    // トンネル入口の直線情報に保存
                    convertedTunnelEntrancePolylines.emplace_back(line);

                    // トンネル入口の両端情報に保存
                    tunnelBothEndsPoint.emplace_back(BoostPairPoint(
                        targetCrossPosBoostPoint, otherCrossPosBoostPoint));

                    isTunnelEntrancePolyline = true;
                    break;
                }

            }

            if (isTunnelEntrancePolyline)
            {
                break;
            }
        }
    }

    ///////////////////////////////////////////////////////////
    // トンネルの入口のポリラインの変換
    ///////////////////////////////////////////////////////////

    for (BoostMultiLines::iterator entrancePolylineIt = tunnelEntrancePolylines.begin();
        entrancePolylineIt != tunnelEntrancePolylines.end(); entrancePolylineIt++)
    {
        // 水平線上の不要点の削除
        BoostPolyline entrancePolyline;// = CAnalyzeRoadEdgeGeomUtil::Simplify(*entrancePolylineIt);
        bg::simplify(*entrancePolylineIt, entrancePolyline, 0.1);

        // 最長線分の取得
        BoostPolyline maxLengthPolyline;
        maxLengthPolyline.emplace_back(entrancePolyline[0]);
        maxLengthPolyline.emplace_back(entrancePolyline[1]);

        for (size_t i = 1; i < entrancePolyline.size() - 1; i++)
        {
            BoostPolyline line;
            line.emplace_back(entrancePolyline[i]);
            line.emplace_back(entrancePolyline[i + 1]);

            if (bg::length(line) > bg::length(maxLengthPolyline))
            {
                maxLengthPolyline = line;
            }
        }

        convertedTunnelEntrancePolylines.emplace_back(maxLengthPolyline);

        // トンネル入口に二つの点を作成
        BoostMultiPoints crossPoint, uniqueCrossPoint;
        bg::intersection(maxLengthPolyline, roadEdgePolylines, crossPoint);
        // 同一頂点削除
        for (BoostPoint pt : crossPoint)
        {
            if (bg::disjoint(uniqueCrossPoint, pt))
                uniqueCrossPoint.emplace_back(pt);
        }

        if (uniqueCrossPoint.size() >= 2)
        {
            CVector2D pt1(maxLengthPolyline[0].x(), maxLengthPolyline[0].y());
            CVector2D pt2(maxLengthPolyline[1].x(), maxLengthPolyline[1].y());
            CVector2D vec = pt2 - pt1;
            CVector2D centerPt = 0.5 * vec + pt1;
            BoostPoint tmpPt(centerPt.x, centerPt.y);
            std::vector<std::pair<double, size_t>> vecDist;
            for (size_t idx = 0; idx < uniqueCrossPoint.size(); idx++)
            {
                double dLength = CAnalyzeRoadEdgeGeomUtil::Length(uniqueCrossPoint[idx], tmpPt);
                if (CEpsUtil::GreaterEqual(dLength, 1.0))
                {
                    std::pair<double, size_t> val(dLength, idx);
                    vecDist.push_back(val);
                }
            }
            if (vecDist.size() > 1)
            {
                std::sort(vecDist.begin(), vecDist.end());      // 昇順
                // トンネル入り口線の中点から距離が近い交点を2点取得
                CVector2D nearPt1(uniqueCrossPoint[vecDist[0].second].x(),
                    uniqueCrossPoint[vecDist[0].second].y());
                CVector2D nearPt2(uniqueCrossPoint[vecDist[1].second].x(),
                    uniqueCrossPoint[vecDist[1].second].y());
                CVector2D vec1 = nearPt1 - centerPt;
                CVector2D vec2 = nearPt2 - centerPt;

                double dAngleDiff = CAnalyzeRoadEdgeGeomUtil::RoundN(
                    abs(180.0 - CGeoUtil::Angle(vec1, vec2)), 2);
                if (CEpsUtil::Less(dAngleDiff, dAngleDiffTh))
                {
                    tunnelBothEndsPoint.emplace_back(BoostPairPoint(
                        uniqueCrossPoint[vecDist[0].second],
                        uniqueCrossPoint[vecDist[1].second]));
                }
            }
        }
    }

    ///////////////////////////////////////////////////////////
    // 道路縁と検出したトンネルの両端ポイントを統合
    ///////////////////////////////////////////////////////////
    BoostMultiLinesItRTree roadEdgeRTree;
    for (BoostMultiLines::iterator roadEdgeIt = roadEdgePolylines.begin(); roadEdgeIt != roadEdgePolylines.end(); roadEdgeIt++)
    {
        for (BoostPolyline::iterator edgeIt = roadEdgeIt->begin(); edgeIt != roadEdgeIt->end(); edgeIt++)
        {
            roadEdgeRTree.insert(std::pair<BoostPoint, BoostMultiLines::iterator>(*edgeIt, roadEdgeIt));
        }
    }

    // トンネルの両端ポイントごとに辺を検索
    BoostMultiLines pairingTargetPolylines;
    for (std::vector<BoostPairPoint>::iterator bothPointIt = tunnelBothEndsPoint.begin(); bothPointIt != tunnelBothEndsPoint.end(); bothPointIt++)
    {
        // トンネル入り口ベクトル
        CVector2D tunnnelVec(
            bothPointIt->second.x() - bothPointIt->first.x(),
            bothPointIt->second.y() - bothPointIt->first.y());

        BoostMultiPoints tmp;
        tmp.emplace_back(bothPointIt->first);
        tmp.emplace_back(bothPointIt->second);

        for (BoostMultiPoints::iterator pointIt = tmp.begin(); pointIt != tmp.end(); pointIt++)
        {
            // 実行
            std::vector<std::pair<BoostPoint, BoostMultiLines::iterator>> vecValues;
            roadEdgeRTree.query(bg::index::nearest(*pointIt, 4), std::back_inserter(vecValues));

            // 検索したポリラインごとにトンネルの両端ポイントを含むか確認
            for (const std::pair<BoostPoint, BoostMultiLines::iterator> &value : vecValues)
            {
                // 同じ点の場合はスキップ
                if (bg::equals(value.first, *pointIt))
                {
                    continue;
                }

                // トンネルベクトルと並行ではないことの確認
                CVector2D vec(
                    value.second->back().x() - value.second->front().x(),
                    value.second->back().y() - value.second->front().y());
                double dAngle = CGeoUtil::Angle(tunnnelVec, vec);
                if (CEpsUtil::Less(CAnalyzeRoadEdgeGeomUtil::RoundN(abs(dAngle), 2), dAngleDiffTh)
                    || CEpsUtil::Less(CAnalyzeRoadEdgeGeomUtil::RoundN(abs(180.0 - dAngle), 2), dAngleDiffTh))
                {
                    // トンネルベクトルと道路縁ベクトルのなす角が0degまたは180degの場合
                    continue;
                }

                BoostPoint point = *pointIt;
                BoostMultiLines::iterator multiLineIt = value.second;
                bool isInsert = false;

                // ポリライン内のすべての直線について確認
                for (size_t i = 0; i < multiLineIt->size() - 1; i++)
                {
                    BoostPoint boostPointU = (*multiLineIt)[i];
                    BoostPoint boostPointV = (*multiLineIt)[i + 1];

                    BoostPoint boostPointFirst = (*multiLineIt)[0];
                    BoostPoint boostPointLast = (*multiLineIt)[multiLineIt->size() - 1];

                    if (bg::equals(point, boostPointU)
                        || bg::equals(point, boostPointV)
                        || bg::equals(point, boostPointFirst)
                        || bg::equals(point, boostPointLast))
                    {
                        break;
                    }

                    CVector3D o = CVector3D(point.x(), point.y(), 0);
                    CVector3D u = CVector3D(boostPointU.x(), boostPointU.y(), 0);
                    CVector3D v = CVector3D(boostPointV.x(), boostPointV.y(), 0);
                    CVector3D O2U = u - o;
                    CVector3D O2V = v - o;

                    // 対象点を含む3つの点が直線か、
                    // 対象点・ポリラインの始点・ポリラインの終点の3点が直線かどうか確認
                    double dAngleDiff = CAnalyzeRoadEdgeGeomUtil::RoundN(
                        abs(180.0 - CGeoUtil::Angle(O2U, O2V)), 2);
                    if (CEpsUtil::Less(dAngleDiff, dAngleDiffTh))
                    {
                        // 辺内に頂点を追加する
                        multiLineIt->insert((multiLineIt->begin() + i + 1), *pointIt);

                        pairingTargetPolylines.emplace_back(*multiLineIt);

                        isInsert = true;
                        break;
                    }
                }

                if (isInsert)
                {
                    break;
                }
            }
        }
    }

    /////////////////////////////////////////////////////////////
    //// 入口同士を探索
    /////////////////////////////////////////////////////////////
    std::vector<BoostPairLine> shortestPathPolyline;
    BoostMultiPoints searchedEntranceVertex;
    for (auto itTargetPair = tunnelBothEndsPoint.begin();
        itTargetPair != tunnelBothEndsPoint.end(); itTargetPair++)
    {
        // 探索済みの確認
        if (!bg::disjoint(searchedEntranceVertex, itTargetPair->first)
            || !bg::disjoint(searchedEntranceVertex, itTargetPair->second))
        {
            continue;
        }

        // グラフ作成範囲の算出
        BoostPoint firstEntrancePoint = itTargetPair->first;
        BoostPoint lastEntrancePoint = itTargetPair->second;
        CVector2D vec(
            firstEntrancePoint.x() - lastEntrancePoint.x(),
            firstEntrancePoint.y() - lastEntrancePoint.y());
        CVector2D centerPos = 0.5 * vec + CVector2D(lastEntrancePoint.x(), lastEntrancePoint.y());
        BoostMultiPolygon area = CAnalyzeRoadEdgeGeomUtil::Buffering(
            BoostPoint(centerPos.x, centerPos.y), dBuffDist);

        // グラフ作成
        BoostMultiLines roadEdgePolylinesForGraph;
        bg::intersection(roadEdgePolylines, area, roadEdgePolylinesForGraph);
        BoostUndirectedGraph graph = CBoostGraphUtil::CreateGraph(roadEdgePolylinesForGraph);

        // tunnelEntrancePointDescList: トンネルポイントのデスクリプター
        // BoostPoint: トンネル端の座標
        // BoostPoint: もう一つのトンネル端の座標
        // BoostVertexDesc: トンネル端のデスクリプタ
        // BoostVertexDesc: もう一つのトンネル端のデスクリプタ)
        std::vector<EntranceData> tunnelEntrancePointDescList;

        // グラフからトンネルの入口ポイントのデスクリプターを登録
        int nTargetIdx = -1;
        for (auto itPair = tunnelBothEndsPoint.begin();
            itPair != tunnelBothEndsPoint.end(); itPair++)
        {
            if (bg::disjoint(area, itPair->first)
                || bg::disjoint(area, itPair->second)
                || !bg::disjoint(searchedEntranceVertex, itPair->first)
                || !bg::disjoint(searchedEntranceVertex, itPair->second))
                // 注目トンネル入り口が注目領域と衝突していない
                // または探索済みトンネルの場合
                continue;

            BoostVertexDesc firstEntranceVertexDesc = 0;
            BoostVertexDesc lastEntranceVertexDesc = 0;
            bool isFirstDesc = false;
            bool isLastDesc = false;

            BOOST_FOREACH(BoostVertexDesc vertexDesc, boost::vertices(graph))
            {
                if (CAnalyzeRoadEdgeGeomUtil::CheckPointEqual(itPair->first, graph[vertexDesc].pt))
                {
                    firstEntranceVertexDesc = graph[vertexDesc].desc;
                    isFirstDesc = true;
                }

                if (CAnalyzeRoadEdgeGeomUtil::CheckPointEqual(itPair->second, graph[vertexDesc].pt))
                {
                    lastEntranceVertexDesc = graph[vertexDesc].desc;
                    isLastDesc = true;
                }
                if (isFirstDesc && isLastDesc)
                    break;
            }
            if (isFirstDesc && isLastDesc)
            {
                tunnelEntrancePointDescList.emplace_back(EntranceData(
                    itPair->first, itPair->second,
                    firstEntranceVertexDesc, lastEntranceVertexDesc));

                if (CAnalyzeRoadEdgeGeomUtil::CheckPointEqual(firstEntrancePoint, itPair->first)
                    && CAnalyzeRoadEdgeGeomUtil::CheckPointEqual(lastEntrancePoint, itPair->second))
                {
                    nTargetIdx = static_cast<int>(tunnelEntrancePointDescList.size() - 1);
                }
            }
        }

        if (nTargetIdx < 0)
            continue;   // 注目トンネル入り口にグラフのディスクリプタ割りあてが出来なかった場合

        // dijkstra用に無向グラフにエッジの長さを設定する
        BOOST_FOREACH(BoostEdgeDesc edgeDesc, boost::edges(graph))
        {
            graph[edgeDesc].dLength = CBoostGraphUtil::EdgeLength(graph, edgeDesc);
        }

        // rtree作成
        BoostVertexRTree shortestPathsRtree;
        for (EntranceData tunnelEntrancePointDesc : tunnelEntrancePointDescList)
        {
            shortestPathsRtree.insert(std::pair<BoostPoint, BoostVertexDesc>(
                std::get<0>(tunnelEntrancePointDesc), std::get<2>(tunnelEntrancePointDesc)));
            shortestPathsRtree.insert(std::pair<BoostPoint, BoostVertexDesc>(
                std::get<1>(tunnelEntrancePointDesc), std::get<3>(tunnelEntrancePointDesc)));
        }

        // 注目トンネル入り口の経路探索
        BoostMultiLines pairPathPolyline;
        BoostVertexDesc firstPathStartVertexDesc = 0;
        BoostVertexDesc firstPathEndVertexDesc = 0;
        std::vector<std::tuple<BoostPoint, BoostVertexDesc, BoostVertexDesc>> tmp;
        tmp.emplace_back(std::tuple<BoostPoint, BoostVertexDesc, BoostVertexDesc>(
            std::get<0>(tunnelEntrancePointDescList[nTargetIdx]),
            std::get<2>(tunnelEntrancePointDescList[nTargetIdx]),
            std::get<3>(tunnelEntrancePointDescList[nTargetIdx])));
        tmp.emplace_back(std::tuple<BoostPoint, BoostVertexDesc, BoostVertexDesc>(
            std::get<1>(tunnelEntrancePointDescList[nTargetIdx]),
            std::get<3>(tunnelEntrancePointDescList[nTargetIdx]),
            std::get<2>(tunnelEntrancePointDescList[nTargetIdx])));

        for (std::vector<std::tuple<BoostPoint, BoostVertexDesc, BoostVertexDesc>>::iterator pointIt = tmp.begin(); pointIt != tmp.end(); pointIt++)
        {
            BoostPoint vertexPt = std::get<0>(*pointIt);
            BoostVertexDesc vertexDesc = std::get<1>(*pointIt);
            BoostVertexDesc pairVertexDesc = std::get<2>(*pointIt);

            // 近傍交差点探索
            std::vector<std::pair<BoostPoint, BoostVertexDesc>> vecValues;
            shortestPathsRtree.query(bg::index::nearest(vertexPt, 8), std::back_inserter(vecValues));

            // 経路探索
            std::vector<BoostVertexDesc> pred(boost::num_vertices(graph), BoostUndirectedGraph::null_vertex());
            std::vector<double> vecDistance(boost::num_vertices(graph));
            boost::dijkstra_shortest_paths(graph,
                vertexDesc,
                boost::predecessor_map(pred.data()).distance_map(vecDistance.data()).weight_map(boost::get(&BoostEdgeProperty::dLength, graph)));

            std::vector<std::tuple<BoostPolyline, BoostVertexDesc, double>> pathChoicesPolylines; // 近傍探索で検出した別のトンネルポイントへの最短ルート候補、検出ポイントのデスクリプタ
            for (auto itVal = vecValues.begin(); itVal != vecValues.end(); itVal++)
            {
                // 経路なしの場合
                // 同じ点の場合とあるトンネル入口に対するもう一つの点だった場合
                // 探索済み頂点の場合はスキップ
                if (pred[itVal->second] == itVal->second ||
                    itVal->second == vertexDesc || itVal->second == pairVertexDesc
                    || bg::overlaps(searchedEntranceVertex, itVal->first))
                {
                    continue;
                }

                // 注目点から近傍点までの経路を抽出
                BoostPolyline pathPolyline;
                for (BoostVertexDesc tmpDesc = itVal->second;
                    tmpDesc != vertexDesc; tmpDesc = pred[tmpDesc])
                {
                    pathPolyline.emplace_back(graph[tmpDesc].pt);
                }
                pathPolyline.emplace_back(graph[vertexDesc].pt);

                // 最短経路の選択肢(経路ポリラインと検出ポイントのデスクリプタ)を保存
                pathChoicesPolylines.emplace_back(std::tuple<BoostPolyline, BoostVertexDesc, double>(pathPolyline, itVal->second, vecDistance[itVal->second]));
            }

            // 抽出した経路の中で、最短のものを保存する
            // 既に保存した経路のゴールと一致していた場合は経路を確定する
            int shortestPolylineIndex = 0;
            if (pathChoicesPolylines.size() > 0)
            {
                if (pairPathPolyline.size() > 0)
                {
                    ///
                    /// 既に保存されている最短経路がある場合
                    ///

                    double shortestDistance = std::get<2>(pathChoicesPolylines[0]);
                    bool isPairing = false;

                    // 先に発見した経路の終点とペアになるトンネル入り口頂点の探索
                    BoostVertexDesc checkDesc = BoostUndirectedGraph::null_vertex();
                    for (EntranceData tunnelEntrancePointDesc : tunnelEntrancePointDescList)
                    {
                        if (std::get<2>(tunnelEntrancePointDesc) == firstPathEndVertexDesc)
                            checkDesc = std::get<3>(tunnelEntrancePointDesc);
                        else if (std::get<3>(tunnelEntrancePointDesc) == firstPathEndVertexDesc)
                            checkDesc = std::get<2>(tunnelEntrancePointDesc);
                    }
                    if (checkDesc != BoostUndirectedGraph::null_vertex())
                    {
                        for (int i = 0; i < pathChoicesPolylines.size(); i++)
                        {
                            if (checkDesc == std::get<1>(pathChoicesPolylines[i]))
                            {
                                // 先に発見した経路の終点とペアになるトンネル入り口の場合
                                if (isPairing == false || shortestDistance > std::get<2>(pathChoicesPolylines[i]))
                                {
                                    shortestPolylineIndex = i;
                                    shortestDistance = std::get<2>(pathChoicesPolylines[shortestPolylineIndex]);

                                    isPairing = true;
                                    break;

                                }
                            }
                        }
                    }

                    // ペアかつ最短経路が見つかった場合は経路同士の衝突確認をして保存する
                    if (isPairing
                        && bg::disjoint(pairPathPolyline[0], std::get<0>(pathChoicesPolylines[shortestPolylineIndex])))
                    {
                        // トンネル道路縁として登録
                        shortestPathPolyline.emplace_back(
                            BoostPairLine(pairPathPolyline[0], std::get<0>(pathChoicesPolylines[shortestPolylineIndex])));

                        // 最短経路の検索済み対象を、2本の始点終点の両方登録する
                        searchedEntranceVertex.emplace_back(std::get<0>(tunnelEntrancePointDescList[nTargetIdx]));
                        searchedEntranceVertex.emplace_back(std::get<1>(tunnelEntrancePointDescList[nTargetIdx]));
                        searchedEntranceVertex.emplace_back(graph[firstPathEndVertexDesc].pt);
                        searchedEntranceVertex.emplace_back(graph[checkDesc].pt);
                    }
                }
                else
                {
                    ///
                    /// まだ最短経路がない場合
                    ///
                    double shortestDistance = std::get<2>(pathChoicesPolylines[0]);

                    for (int i = 1; i < pathChoicesPolylines.size(); i++)
                    {
                        if (shortestDistance > std::get<2>(pathChoicesPolylines[i]))
                        {
                            // 最短なら保存
                            shortestPolylineIndex = i;
                            shortestDistance = std::get<2>(pathChoicesPolylines[shortestPolylineIndex]);
                        }
                    }
                    // トンネル片側の道路縁
                    pairPathPolyline.emplace_back(std::get<0>(pathChoicesPolylines[shortestPolylineIndex]));
                    firstPathStartVertexDesc = vertexDesc;
                    firstPathEndVertexDesc = std::get<1>(pathChoicesPolylines[shortestPolylineIndex]);
                }
            }
        }
    }
    return shortestPathPolyline;
}
