#include "pch.h"
#include "CRoadDivision.h"
#include "CAnalyzeRoadEdgeGeomUtil.h"
#include "CBoostGraphUtil.h"
#include "boost/graph/dijkstra_shortest_paths.hpp"

/*!
 * @brief 車道交差部ポリゴンの分割
 * @param[in] roadPolygons          道路ポリゴン群
 * @param[in] blockPolyogns         街区ポリゴン群
 * @param[in] crossing              交差点情報
 * @param[out] crossingPolygons     車道交差部ポリゴン
 * @param[out] remainingPolygons    残道路ポリゴン
 * @param[in] dReso                 座標変換の解像度(m)
*/
void CRoadDivision::DivisionByCrossing(
    BoostMultiPolygon &roadPolygons,
    BoostMultiPolygon &blockPolyogns,
    std::vector<CCrossingData> &crossing,
    std::vector<CRoadData> &crossingPolygons,
    BoostMultiPolygon &remainingPolygons,
    const double dReso)
{
    // 交差点データにボロノイ領域を設定する
    setVoronoiCellArea(roadPolygons, crossing, dReso);

    // 交差点領域の設定
    setCrossingArea(roadPolygons, crossing);

    // 車道交差部切断
    createCrossingPolygons(
        roadPolygons, blockPolyogns, crossing,
        crossingPolygons, remainingPolygons);
}

/*!
 * @brief 道路構造変化によるポリゴンの分割(道路橋用)
 * @param[in]   roadPolygons        道路ポリゴン群
 * @param[in]   facilities          道路橋
 * @param[in]   roadSectionType     道路形状タイプ
 * @param[out]  facilityPolygons    道路橋
 * @param[out]  remainingPolygons   残存道路ポリゴン
 * @param[in]   dOverhangAreaTh     道路橋ポリゴンと道路と道路橋の重畳領域の差分面積閾値(街区はみ出し確認用）
*/
void CRoadDivision::DivisionByStructualChange(
    BoostMultiPolygon &roadPolygons,
    BoostMultiLines &facilities,
    RoadSectionType roadSectionType,
    std::vector<CRoadData> &facilityPolygons,
    BoostMultiPolygon &remainingPolygons,
    const double dOverhangAreaTh)
{
    std::vector<CRoadData> tmpFacilityPolygons;

    // 近傍探索用のデータ作成
    BoostMultiLinesItRTree rtree;
    for (auto itLine = facilities.begin(); itLine != facilities.end(); itLine++)
    {
        rtree.insert(std::pair<BoostPoint, BoostMultiLines::iterator>(itLine->front(), itLine));
        rtree.insert(std::pair<BoostPoint, BoostMultiLines::iterator>(itLine->back(), itLine));
    }
    // 近傍探索
    std::set<BoostMultiLines::iterator> searchedLines;
    for (auto itLine = facilities.begin(); itLine != facilities.end(); itLine++)
    {
        if (searchedLines.find(itLine) != searchedLines.end())
            continue;   // 探索済みの場合はskip

        // KNN
        std::vector<std::pair<BoostPoint, BoostMultiLines::iterator>> vecValues;
        rtree.query(bg::index::nearest(itLine->front(), 4), std::back_inserter(vecValues));
        auto itTargetLine = facilities.end();   // 対向線
        double dLength = DBL_MAX;
        BoostPolygon targetPolygon;             // 橋梁ポリゴン
        for (std::vector<std::pair<BoostPoint, BoostMultiLines::iterator>>::iterator itVal = vecValues.begin();
            itVal != vecValues.end(); itVal++)
        {
            if (itVal->second == itLine
                || searchedLines.find(itVal->second) != searchedLines.end())
                continue;   // 対向線探索対象と同一 or 探索済みの場合はskip

            // 対向線確認
            // 対向線を繋いだポリゴンの作成
            BoostMultiPolygon candidates;
            BoostPolygon polygon1, polygon2;
            for (auto itPt = itLine->begin(); itPt != itLine->end(); itPt++)
            {
                polygon1.outer().push_back(*itPt);
                polygon2.outer().push_back(*itPt);
            }
            BoostPolyline tmpLine(*itVal->second);
            for (auto itPt = tmpLine.begin(); itPt != tmpLine.end(); itPt++)
                polygon1.outer().push_back(*itPt);
            for (auto itPt = tmpLine.rbegin(); itPt != tmpLine.rend(); itPt++)
                polygon2.outer().push_back(*itPt);
            bg::correct(polygon1);
            bg::correct(polygon2);
            candidates.push_back(polygon1);
            candidates.push_back(polygon2);

            for (auto itCandidate = candidates.begin(); itCandidate != candidates.end(); itCandidate++)
            {
                // 不正ポリゴンの確認
                if (!bg::is_valid(*itCandidate))
                    continue;

                // 道路ポリゴンとの重畳確認(街区にはみ出ていないか確認する)
                BoostMultiPolygon andPolygons;
                bg::intersection(roadPolygons, *itCandidate, andPolygons);
                if (!bg::is_empty(andPolygons))
                {
                    double dDiffArea = abs(bg::area(*itCandidate) - bg::area(andPolygons));
                    if (andPolygons.size() > 1 || CEpsUtil::GreaterEqual(dDiffArea, dOverhangAreaTh))
                    {
                        // 道路橋ポリゴンが街区にはみ出し、1ブロック隣の道路にも重畳している場合は道路との重畳領域が複数個になる
                        // 道路橋ポリゴンが街区にはみ出している場合は、道路との重畳領域と道路橋ポリゴンの面積に差が発生する
                        continue;
                    }
                }

                // 注目点と近傍点間の距離が最短となるポリゴンを取得する
                if (dLength > CAnalyzeRoadEdgeGeomUtil::Length(itLine->front(), itVal->first))
                {
                    itTargetLine = itVal->second;
                    dLength = CAnalyzeRoadEdgeGeomUtil::Length(itLine->front(), itVal->first);
                    targetPolygon = *itCandidate;
                }
            }
        }

        if (itTargetLine != facilities.end())
        {
            if (bg::is_valid(targetPolygon))
            {
                // 登録
                CRoadData roadData;
                roadData.Polygon(targetPolygon);
                roadData.Type(roadSectionType);
                roadData.Division(0);
                tmpFacilityPolygons.push_back(roadData);

                // 探索済みの登録
                searchedLines.insert(itLine);
                searchedLines.insert(itTargetLine);
            }
        }
    }

    // 道路ポリゴンから道路構造変化部分のポリゴンを引く
    std::vector<CRoadData> errBridgePolygons; // 分割時に不正ポリゴンが発生する道路橋ポリゴン情報群
    divisionPolygon(roadPolygons, tmpFacilityPolygons, facilityPolygons, errBridgePolygons, remainingPolygons);
}

/*!
 * @brief 道路構造変化によるポリゴンの分割(トンネル用）
 * @param[in]   roadPolygons        道路ポリゴン群
 * @param[in]   facilities          トンネル
 * @param[in]   roadSectionType     道路形状タイプ
 * @param[out]  facilityPolygons    トンネル
 * @param[out]  remainingPolygons   残存道路ポリゴン
 * @param[in]   dOverhangAreaTh     道路橋ポリゴンと道路と道路橋のAND領域の差分面積閾値(街区はみ出し確認用）
*/
void CRoadDivision::DivisionByStructualChange(
    BoostMultiPolygon &roadPolygons,
    std::vector<BoostPairLine> &facilities,
    RoadSectionType roadSectionType,
    std::vector<CRoadData> &facilityPolygons,
    BoostMultiPolygon &remainingPolygons,
    const double dOverhangAreaTh)
{
    std::vector<CRoadData> tmpFacilityPolygons;

    for (auto itPair = facilities.begin(); itPair != facilities.end(); itPair++)
    {
        BoostPolygon targetPolygon;     // トンネルポリゴン

        // 対向線を繋いだポリゴンの作成
        BoostMultiPolygon candidates;
        BoostPolygon polygon1, polygon2;
        for (auto itPt = itPair->first.begin(); itPt != itPair->first.end(); itPt++)
        {
            polygon1.outer().push_back(*itPt);
            polygon2.outer().push_back(*itPt);
        }
        for (auto itPt = itPair->second.begin(); itPt != itPair->second.end(); itPt++)
            polygon1.outer().push_back(*itPt);
        for (auto itPt = itPair->second.rbegin(); itPt != itPair->second.rend(); itPt++)
            polygon2.outer().push_back(*itPt);
        bg::correct(polygon1);
        bg::correct(polygon2);
        candidates.push_back(polygon1);
        candidates.push_back(polygon2);

        for (auto itCandidate = candidates.begin(); itCandidate != candidates.end(); itCandidate++)
        {
            // 不正ポリゴンの確認
            if (!bg::is_valid(*itCandidate))
                continue;

            // 道路ポリゴンとの重畳確認(街区にはみ出ていないか確認する)
            BoostMultiPolygon andPolygons;
            bg::intersection(roadPolygons, *itCandidate, andPolygons);
            if (!bg::is_empty(andPolygons))
            {
                double dDiffArea = abs(bg::area(*itCandidate) - bg::area(andPolygons));
                if (andPolygons.size() > 1 || CEpsUtil::GreaterEqual(dDiffArea, dOverhangAreaTh))
                {
                    // 道路橋ポリゴンが街区にはみ出し、1ブロック隣の道路にも重畳している場合は道路との重畳領域が複数個になる
                    // 道路橋ポリゴンが街区にはみ出している場合は、道路との重畳領域と道路橋ポリゴンの面積に差が発生する
                    continue;
                }

                targetPolygon = *itCandidate;
            }
        }

        if (!bg::is_empty(targetPolygon) && bg::is_valid(targetPolygon))
        {
            // 登録
            CRoadData roadData;
            roadData.Polygon(targetPolygon);
            roadData.Type(roadSectionType);
            roadData.Division(0);
            tmpFacilityPolygons.push_back(roadData);
        }
    }

    // 道路ポリゴンから道路構造変化部分のポリゴンを引く
    std::vector<CRoadData> errTunnelPolygons; // 分割時に不正ポリゴンが発生するトンネルポリゴン情報群
    divisionPolygon(roadPolygons, tmpFacilityPolygons, facilityPolygons, errTunnelPolygons, remainingPolygons);
}

/*!
 * @brief 交差点データにボロノイセル情報をセットする
 * @param[in]       roadPolyogns    道路ポリゴン群
 * @param[in/out]   crossing        交差点情報
 * @param[in]       dReso           座標変換の解像度(m)
*/
void CRoadDivision::setVoronoiCellArea(
    BoostMultiPolygon &roadPolyogns,
    std::vector<CCrossingData> &crossing,
    const double dReso)
{
    assert(CEpsUtil::Greater(dReso, 0));

    // 交差点でボロノイ分割
    // int座標にするためにworld→画像座標
    BoostBox box;
    bg::envelope(roadPolyogns, box);    // 入力道路ポリゴン範囲
    double dMargin = dReso * 2.0;
    double dMinX = floor(box.min_corner().x() / dReso) * dReso - dMargin;
    double dMinY = floor(box.min_corner().y() / dReso) * dReso - dMargin;
    double dMaxX = ceil(box.max_corner().x() / dReso) * dReso + dMargin;
    double dMaxY = ceil(box.max_corner().y() / dReso) * dReso + dMargin;
    double dOffsetX = dMinX;
    double dOffsetY = dMaxY;

    std::vector<BoostVoronoiPoint> pts;
    for (auto it = crossing.begin(); it != crossing.end(); it++)
    {
        int nX, nY;
        CAnalyzeRoadEdgeGeomUtil::ConvertWorldToPx(
            it->Point().x(), it->Point().y(), dOffsetX, dOffsetY, dReso, nX, nY);
        pts.push_back(BoostVoronoiPoint(nX, nY));
    }

    // ボロノイ分割
    BoostVoronoiDiagram diagram;
    bp::construct_voronoi(pts.begin(), pts.end(), &diagram);

    // 画像座標系のデータ範囲
    int nImgX1, nImgY1, nImgX2, nImgY2;
    CAnalyzeRoadEdgeGeomUtil::ConvertWorldToPx(
        dMinX, dMinY, dOffsetX, dOffsetY, dReso, nImgX1, nImgY1);
    CAnalyzeRoadEdgeGeomUtil::ConvertWorldToPx(
        dMaxX, dMaxY, dOffsetX, dOffsetY, dReso, nImgX2, nImgY2);
    BoostBox imgBox(
        BoostPoint(CEpsUtil::Min(static_cast<double>(nImgX1), static_cast<double>(nImgX2)),
            CEpsUtil::Min(static_cast<double>(nImgY1), static_cast<double>(nImgY2))),
        BoostPoint(CEpsUtil::Max(static_cast<double>(nImgX1), static_cast<double>(nImgX2)),
            CEpsUtil::Max(static_cast<double>(nImgY1), static_cast<double>(nImgY2))));

    // cellごとに作業
    for (BoostVoronoiCellIt it = diagram.cells().begin();
        it != diagram.cells().end(); it++)
    {
        // cell情報
        const BoostVoronoiDiagram::cell_type &cell = *it;
        if (cell.incident_edge() == NULL)
            continue;

        BoostPolygon cellPolygon;
        getVoronoiCell(cell, pts, imgBox, cellPolygon);

        if (bg::is_valid(cellPolygon))
        {
            // 画像座標->world座標に変換
            BoostPolygon worldCellPolygon;
            for (auto itPt = cellPolygon.outer().begin();
                itPt != cellPolygon.outer().end(); itPt++)
            {
                double dX, dY;
                CAnalyzeRoadEdgeGeomUtil::ConvertPxToWorld(
                    itPt->x(), itPt->y(), dOffsetX, dOffsetY, dReso, dX, dY);
                worldCellPolygon.outer().push_back((BoostPoint(dX, dY)));
            }
            bg::correct(worldCellPolygon);

            // cellに対応する交差点
            auto itCross = crossing.begin() + cell.source_index();
            itCross->Cell(worldCellPolygon);
        }
    }
}

/*!
 * @brief ボロノイセルポリゴンの取得
 * @param[in] cell          セル情報
 * @param[in] pts           ボロノイ分割時の入力頂点群
 * @param[in] box           入力範囲
 * @param[out] cellPolygon  セルポリゴン
*/
void CRoadDivision::getVoronoiCell(
    const BoostVoronoiDiagram::cell_type &cell,
    std::vector<BoostVoronoiPoint> &pts,
    BoostBox &box,
    BoostPolygon &cellPolygon)
{
    // 無限長edgeがある場合は、無限長edgeから探索を開始する
    const bp::voronoi_diagram<double>::edge_type *startEdge = cell.incident_edge();
    do
    {
        if (startEdge->is_infinite() && startEdge->vertex0() == nullptr)
        {
            break;
        }
        startEdge = startEdge->next();
    } while (startEdge != cell.incident_edge());

    const bp::voronoi_diagram<double>::edge_type *edge = startEdge;
    do
    {
        if (edge->is_primary())
        {
            if (edge->is_finite())
            {
                // edgeが有限の場合
                cellPolygon.outer().push_back(BoostPoint(edge->vertex0()->x(), edge->vertex0()->y()));
                cellPolygon.outer().push_back(BoostPoint(edge->vertex1()->x(), edge->vertex1()->y()));
            }
            else
            {
                // edgeが無限の場合

                // 対象セルの中心
                BoostVoronoiPoint p1 = pts[edge->cell()->source_index()];

                // 対象edgeと対になるedgeが形成するセルの中心
                BoostVoronoiPoint p2 = pts[edge->twin()->cell()->source_index()];

                BoostVoronoiPoint origin(
                    static_cast<int>((p1.x() + p2.x()) * 0.5),
                    static_cast<int>((p1.y() + p2.y()) * 0.5)); //基準点
                BoostVoronoiPoint direction(
                    static_cast<int>(p1.y() - p2.y()),
                    static_cast<int>(p2.x() - p1.x()));  //ベクトルの向き

                // 中点を結んだベクトルの向きを90deg回転させて、infinite edgeの向きを決める
                //vertex0,1の有無によって回転角度の正負を変更する
                //BoostPoint startPoint, endPoint;
                double side = box.max_corner().x() - box.min_corner().x();
                double koef = side / (std::max)(fabs(direction.x()), fabs(direction.y()));

                // 無限長edgeを有限長edgeにする
                if (edge->vertex0() == nullptr && edge->vertex1() != nullptr)
                {
                    BoostPoint pt(edge->vertex1()->x(), edge->vertex1()->y());
                    // 後段でセル領域を入力範囲でクリッピングする関係上
                    // 次エッジの始点が入力データ範囲外の場合は無限長エッジを有限長にしなくても良い
                    if (bg::within(pt, box))
                    {
                        cellPolygon.outer().push_back(
                            BoostPoint(origin.x() - direction.x() * koef,
                                origin.y() - direction.y() * koef));
                    }
                }

                if (edge->vertex1() == nullptr && edge->vertex0() != nullptr)
                {
                    // 後段でセル領域を入力範囲でクリッピングする関係上
                    // 前エッジの終点が入力データ範囲外の場合は無限長エッジを有限長にしなくても良い
                    BoostPoint pt(edge->vertex0()->x(), edge->vertex0()->y());
                    if (bg::within(pt, box))
                    {
                        cellPolygon.outer().push_back(
                            BoostPoint(origin.x() + direction.x() * koef,
                                origin.y() + direction.y() * koef));
                    }
                }
            }
        }
        edge = edge->next();    //反時計回り
    } while (edge != startEdge);

    bg::unique(cellPolygon);   //重複点の削除
    bg::correct(cellPolygon);

    if (bg::is_valid(cellPolygon))
    {
        // 入力範囲と重畳する範囲にセルの領域を収める
        BoostMultiPolygon polygons;
        bg::intersection(box, cellPolygon, polygons);
        double dArea = 0;
        for (auto itPoly = polygons.begin(); itPoly != polygons.end(); itPoly++)
        {
            if (dArea < bg::area(*itPoly))
            {
                cellPolygon = *itPoly;
                dArea = bg::area(*itPoly);
            }
        }
    }
}

/*!
 * @brief 交差点領域の設定
 * @param[in]       roadPolygons    道路ポリゴン群
 * @param[in/out]   crossing        交差点情報
*/
void CRoadDivision::setCrossingArea(
    BoostMultiPolygon &roadPolygons,
    std::vector<CCrossingData> &crossing)
{
    // 道路縁を取得
    BoostMultiLines roadEdges;
    for (auto itPoly = roadPolygons.begin();
        itPoly != roadPolygons.end(); itPoly++)
    {
        CAnalyzeRoadEdgeGeomUtil::GetEdges(*itPoly, roadEdges);
    }

    // 交差点領域の算出
    for (auto itCross = crossing.begin(); itCross != crossing.end(); itCross++)
    {
        // 交差点の最近傍の道路縁までの距離(概算道路幅の半分)を取得
        double dDist = DBL_MAX;
        for (auto itLine = roadEdges.begin(); itLine != roadEdges.end(); itLine++)
        {
            double d = bg::distance(*itLine, itCross->Point());
            if (d < dDist)
                dDist = d;
        }
        // 概算で道路幅の2倍
        dDist *= 4;

        // 近傍エリア
        BoostMultiPolygon circle = CAnalyzeRoadEdgeGeomUtil::Buffering(itCross->Point(), dDist);

        // 近傍エリアとボロノイ領域の重畳領域を取得する
        BoostMultiPolygon areas;
        bg::intersection(circle, itCross->Cell(), areas);

        if (areas.size() > 0)
            itCross->Area(areas[0]);
    }
}

/*!
 * @brief 交差点近傍道路縁の取得
 * @param[in] roadPolyogns  道路ポリゴンデータ
 * @param[in] area          交差点領域
 * @param[in] crossPt       交差点座標
 * @return  道路縁(線分単位)データ群
*/
BoostMultiLines CRoadDivision::GetRoadEdges(
    BoostMultiPolygon &roadPolyogns,
    BoostPolygon &area,
    BoostPoint &crossPt)
{
    // 交差点領域と重畳する道路縁を取得する
    BoostMultiLines lines;
    for (auto itPolygon = roadPolyogns.begin(); itPolygon != roadPolyogns.end(); itPolygon++)
    {
        CAnalyzeRoadEdgeGeomUtil::GetEdges(*itPolygon, area, lines);
    }

    // 取得した道路縁を絞り込む
    if (lines.size() > 0)
    {
        for (auto itPolyline = lines.end() - 1; itPolyline >= lines.begin(); itPolyline--)
        {
            // 交差点と取得した道路縁の頂点を繋いだ直線が道路縁全体と交差する回数を確認する
            // 複数回交差する場合は、注目道路縁が注目交差点の道路縁ではないとみなす
            for (auto itPt = itPolyline->begin(); itPt != itPolyline->end(); itPt++)
            {
                BoostPolyline tmpLine;
                tmpLine.push_back(crossPt);
                tmpLine.push_back(*itPt);
                BoostMultiPoints pts;
                bg::intersection(roadPolyogns, tmpLine, pts);
                if (pts.size() > 1)
                {
                    lines.erase(itPolyline);
                    break;
                }
            }
        }
    }
    return lines;
}

/*!
 * @brief 道路縁(線分)を結合して経路データを作成する
 * @param[in]   roadEdges 道路縁(線分)群
 * @return      ポリラインデータ形状の道路縁データ
 * @remarks     結合した際に閉路となる道路縁はない想定
*/
BoostMultiLines CRoadDivision::GetRoutes(
    BoostMultiLines &roadEdges)
{
    BoostMultiLines routes; // 経路データ

    // 道路縁で無向グラフを作成する
    BoostUndirectedGraph graph = CBoostGraphUtil::CreateGraph(roadEdges);

    // 端点を元に経路探索を行い、道路縁を結合する
    // 端点のリストアップ
    std::vector<BoostVertexDesc> vecVertexDesc;
    BOOST_FOREACH(BoostVertexDesc vertexDesc, boost::vertices(graph))
    {
        // 端点(次数1の頂点)の確認
        if (boost::degree(vertexDesc, graph) == 1)
            vecVertexDesc.push_back(vertexDesc);
    }
    // 経路探索
    std::set<BoostVertexDesc> searchedVertices; // 探索済み管理用
    for (auto itStart = vecVertexDesc.begin(); itStart != vecVertexDesc.end(); itStart++)
    {
        if (searchedVertices.find(*itStart) != searchedVertices.end())
            continue;   // 探索済みの場合はskip

        // 経路探索
        std::vector<BoostVertexDesc> pred(
            boost::num_vertices(graph), BoostDirectedGraph::null_vertex());
        std::vector<double> vecDistance(boost::num_vertices(graph));
        boost::dijkstra_shortest_paths(
            graph, *itStart,
            boost::predecessor_map(pred.data()).
            distance_map(vecDistance.data()).
            weight_map(boost::get(&BoostEdgeProperty::dLength, graph)));

        for (auto itEnd = vecVertexDesc.begin(); itEnd != vecVertexDesc.end(); itEnd++)
        {
            if (itStart == itEnd
                || searchedVertices.find(*itEnd) != searchedVertices.end())
                continue;   // 開始点と同一 or 探索済みの場合はskip

            // 始点から終点までの経路を確認
            if (pred[*itEnd] == *itEnd)
                continue;  // 始点から終点までの経路がない

            // 経路作成
            BoostPolyline route;
            for (BoostVertexDesc tmpDesc = *itEnd;
                tmpDesc != *itStart; tmpDesc = pred[tmpDesc])
            {
                route.push_back(graph[tmpDesc].pt);
            }
            route.push_back(graph[*itStart].pt);
            routes.push_back(route);
            // 探索済み
            searchedVertices.insert(*itStart);
            searchedVertices.insert(*itEnd);

            break;
        }
    }
    return routes;
}

/*!
 * @brief 交差点近傍道路縁の最遠点の探索
 * @param[in]   routes      道路縁(経路)
 * @param[in]   crossPt     交差点座標
 * @param[in]   usedPtList  道路縁短縮処理済みの道路縁頂点イテレータのリスト
 * @param[out]  itTarget    最遠点を含む道路縁のイテレータ
 * @param[out]  itTargetPt  最遠点のイテレータ
 * @param[out]  targetVec   最遠点を終点、前点を始点とするベクトル
 * @param[out]  itTargetEnd 道路縁の短縮処理で行うループ処理の終了イテレータ
 * @param[out]  nTargetStep 道路縁の短縮処理で行うループ処理のステップ値
 * @return      探索結果
 * @retval      true        発見
 * @retval      false       未発見
*/
bool CRoadDivision::SearchTarget(
    BoostMultiLines &routes,
    BoostPoint &crossPt,
    std::set<BoostPolyline::iterator> usedPtList,
    BoostMultiLines::iterator &itTarget,
    BoostPolyline::iterator &itTargetPt,
    CVector2D &targetVec,
    BoostPolyline::iterator &itTargetEnd,
    int &nTargetStep)
{
    itTarget = routes.end();

    // 最遠点の探索
    std::vector<std::pair<double, size_t>> vecDist;
    size_t idx = 0;
    for (auto itLine = routes.begin(); itLine != routes.end(); itLine++)
    {
        std::pair<double, int> val1(CAnalyzeRoadEdgeGeomUtil::Length(crossPt, itLine->front()), idx++);
        std::pair<double, int> val2(CAnalyzeRoadEdgeGeomUtil::Length(crossPt, itLine->back()), idx++);
        vecDist.push_back(val1);
        vecDist.push_back(val2);
    }

    std::sort(vecDist.begin(), vecDist.end());      // 昇順
    std::reverse(vecDist.begin(), vecDist.end());   // 反転して降順
    BoostPoint targetPt, prevPt;
    for (auto it = vecDist.begin(); it != vecDist.end(); it++)
    {
        size_t lineIdx = it->second / 2;
        BoostMultiLines::iterator itCandidateLine = routes.begin() + lineIdx;
        BoostPolyline::iterator itCandidatePt = itCandidateLine->end() - 1;
        BoostPoint candidatePrevPt = *(itCandidatePt - 1);

        if (it->second % 2 == 0)
        {
            itCandidatePt = itCandidateLine->begin();
            candidatePrevPt = *(itCandidatePt + 1);
        }
        if (usedPtList.find(itCandidatePt) == usedPtList.end())
        {
            itTarget = itCandidateLine;
            itTargetPt = itCandidatePt;
            targetPt = *itCandidatePt;
            prevPt = candidatePrevPt;
            break;
        }
    }

    if (itTarget != routes.end())
    {
        // 最遠点を終点、前点を始点とするベクトル
        targetVec.x = targetPt.x() - prevPt.x();
        targetVec.y = targetPt.y() - prevPt.y();

        // 道路縁の短縮処理で使用するパラメータの設定
        nTargetStep = 1;
        itTargetEnd = itTarget->end();
        if (itTargetPt != itTarget->begin())
        {
            itTargetEnd = itTarget->begin() - 1;
            nTargetStep = -1;
        }
    }

    return itTarget != routes.end();
}

/*!
 * @brief 交差点近傍道路縁の最遠点に対する対向点の探索
 * @param[in]   routes      道路縁(経路)
 * @param[in]   itTarget    最遠点を含む道路縁のイテレータ
 * @param[in]   itTargetPt  最遠点のイテレータ
 * @param[in]   targetVec   最遠点を終点、前点を始点とするベクトル
 * @param[in]   usedPtList  道路縁短縮処理済みの道路縁頂点イテレータのリスト
 * @param[out]  itTwin      対向点を含む道路縁のイテレータ
 * @param[out]  itTwinPt    対向点のイテレータ
 * @param[out]  twinVec     対向点を終点、前点を始点とするベクトル
 * @param[out]  itTwinEnd   道路縁の短縮処理で行うループ処理の終了イテレータ
 * @param[out]  nTwinStep   道路縁の短縮処理で行うループ処理のステップ値
 * @return      探索結果
 * @retval      true        発見
 * @retval      false       未発見
*/
bool CRoadDivision::SearchTwin(
    BoostMultiLines &routes,
    BoostMultiLines::iterator &itTarget,
    BoostPolyline::iterator &itTargetPt,
    CVector2D &targetVec,
    std::set<BoostPolyline::iterator> usedPtList,
    BoostMultiLines::iterator &itTwin,
    BoostPolyline::iterator &itTwinPt,
    CVector2D &twinVec,
    BoostPolyline::iterator &itTwinEnd,
    int &nTwinStep)
{

    CVector2D verticalVec;  // targetVecに垂直なベクトル
    CVector2D targetSrcPt(itTargetPt->x(), itTargetPt->y());

    itTwin = routes.end();

    // 対向エッジの探索
    if (CAnalyzeRoadEdgeGeomUtil::GetVerticalVec(targetVec, verticalVec))
    {
        // targetVecに垂直なベクトルを取得出来た場合
        double dDist = DBL_MAX;
        for (auto itLine = routes.begin(); itLine != routes.end(); itLine++)
        {
            if (itLine == itTarget)
                continue;   // 注目頂点を含む経路はskip

            // 各経路の始終点を延長した際に、最遠点を含むエッジに対して
            // 垂直なエッジとの交差状況を確認する

            // 探索対象経路の始点と次点、終点と終点の前点の座標を格納
            // 格納する際はfirstに始点/終点, secondに始点の次点/終点の前点とする
            // (ベクトル算出時にfirst - secondとし、交差点から遠ざかる方向にベクトルが向くようにする)
            std::vector<std::pair<BoostPolyline::iterator, BoostPolyline::iterator>> vecItCheckPt;
            if (usedPtList.find(itLine->begin()) == usedPtList.end())
                vecItCheckPt.push_back(
                    std::pair<BoostPolyline::iterator, BoostPolyline::iterator>(
                        itLine->begin(), itLine->begin() + 1)); // 経路の始点側の2頂点

            if (usedPtList.find(itLine->end() - 1) == usedPtList.end())
                vecItCheckPt.push_back(
                    std::pair<BoostPolyline::iterator, BoostPolyline::iterator>(
                        itLine->end() - 1, itLine->end() - 2)); // 経路の終点側の2頂点

            for (auto itCheckPt = vecItCheckPt.begin(); itCheckPt != vecItCheckPt.end(); itCheckPt++)
            {
                // 始点 - 始点の次点 or 終点 - 終点の前点のベクトルを作成
                CVector2D vec, tmpPos;
                vec.x = itCheckPt->first->x() - itCheckPt->second->x();
                vec.y = itCheckPt->first->y() - itCheckPt->second->y();
                CVector2D srcPt(itCheckPt->second->x(), itCheckPt->second->y());
                double dLength = vec.Length();
                vec.Normalize();    // 正規化

                // 作成したベクトルとtargetVecに垂直なベクトルとの交差状況確認
                bool bOnline1, bOnlilne2;
                double t, s;
                if (CAnalyzeRoadEdgeGeomUtil::GetCrossPos(
                    verticalVec, targetSrcPt, vec, srcPt, tmpPos, bOnline1, bOnlilne2, t, s))
                {
                    // 交差する場合
                    // TODO 角度確認も入れるべきか?(概ね直角で交差するはず)
                    double dAngle = CGeoUtil::Angle(verticalVec, vec);
                    // ベクトル係数と距離の確認
                    double dTmpDist = s - dLength;
                    if (CEpsUtil::GreaterEqual(s, 0) && dTmpDist < dDist)
                    {
                        // 正方向で交差し、ベクトル係数が小さい場合
                        dDist = dTmpDist;
                        itTwin = itLine;
                        itTwinPt = itCheckPt->first;
                        twinVec = vec;
                    }
                }
            }
        }
    }

    if (itTwin != routes.end())
    {
        // 道路縁の短縮処理で使用するパラメータの設定
        nTwinStep = 1;
        itTwinEnd = itTwin->end();
        if (itTwinPt != itTwin->begin())
        {
            itTwinEnd = itTwin->begin() - 1;
            nTwinStep = -1;
        }
    }

    return itTwin != routes.end();
}

/*!
 * @brief 道路縁短縮処理の前処理
 * @param[in]       itTarget            最遠点を含む道路縁のイテレータ
 * @param[in/out]   itTargetPt          最遠点のイテレータ
 * @param[in]       nTargetStep         最遠点を含む経路を走査する際にステップ値
 * @param[in/out]   targetVec           最遠点を終点、前点を始点とするベクトル(正規化済み)
 * @param[in/out]   dTargetVecLength    targetVecの正規化前の長さ
 * @param[in]       itTwin              対向点を含む道路縁のイテレータ
 * @param[in/out]   itTwinPt            対向点のイテレータ
 * @param[in]       nTwinStep           最遠点を含む経路を走査する際にステップ値
 * @param[in/out]   twinVec             対向点を終点、前点を始点とするベクトル(正規化済み)
 * @param[in/out]   dTwinVecLength      twinVecの正規化前の長さ
 * @param[in]       dEdgeLengthTh       最短エッジ長閾値
 * @remarks 最遠点を含む経路及び対向点を含む経路に必要があれば補助点を追加する処理のため、
            補助点追加後に各情報を最新情報に更新する
*/
void CRoadDivision::roadEdgeShorteningPreProc(
    BoostMultiLines::iterator &itTarget,
    BoostPolyline::iterator &itTargetPt,
    int nTargetStep,
    CVector2D &targetVec,
    double &dTargetVecLength,
    BoostMultiLines::iterator &itTwin,
    BoostPolyline::iterator &itTwinPt,
    int nTwinStep,
    CVector2D &twinVec,
    double &dTwinVecLength,
    const double dEdgeLengthTh)
{
    // 注目エッジと対向エッジの端点を結んだエッジのなす角が直角の場合に向けての前処理
    //          x
    //      x  │
    //     │  │
    //     │  │
    //  x─ x  │<-┐
    //         │  │
    //  x─ x  │<-┴ ここに頂点がないと注目エッジと対向エッジの端点を結んだ
    //     │  │     エッジのなす角が直角の場合の処理がうまくいかない
    //     │   x
    //      x

    CVector2D targetSrcPt(itTargetPt->x(), itTargetPt->y());

    // 注目エッジの始点と対向エッジの終点を繋ぐベクトル
    CVector2D vec((itTwinPt + nTwinStep)->x() - itTargetPt->x(),
        (itTwinPt + nTwinStep)->y() - itTargetPt->y());
    // 注目エッジに垂線を下した際の交点を取得する
    double dTargetInnerProduct = CGeoUtil::InnerProduct(targetVec, vec);
    CVector2D targetInsertPt = dTargetInnerProduct * targetVec + targetSrcPt;
    // 交点と注目エッジの終点を結んだエッジ
    CVector2D tmpPt((itTargetPt + nTargetStep)->x(), (itTargetPt + nTargetStep)->y());
    CVector2D nextTargetVec = tmpPt - targetInsertPt;

    // 同様に対向エッジの始点と注目エッジの終点を結んだエッジも確認する
    vec = CVector2D((itTargetPt + nTargetStep)->x() - itTwinPt->x(),
        (itTargetPt + nTargetStep)->y() - itTwinPt->y());
    // 対向エッジの始点と注目エッジの終点を結んだエッジのなす角が直角ではない場合
    // 対向エッジに垂線を下した際の交点を取得する
    double dTwinInnerProduct = CGeoUtil::InnerProduct(twinVec, vec);
    CVector2D twinSrcPt(itTwinPt->x(), itTwinPt->y());
    CVector2D twinInsertPt = dTwinInnerProduct * twinVec + twinSrcPt;
    // 交点と対向エッジの終点を結んだエッジ
    tmpPt = CVector2D((itTwinPt + nTwinStep)->x(), (itTwinPt + nTwinStep)->y());
    CVector2D nextTwinVec = tmpPt - twinInsertPt;

    // 更新処理は最後にまとめて行う
    if (CEpsUtil::Greater(dTargetInnerProduct, dEdgeLengthTh)
        && CEpsUtil::Less(dTargetInnerProduct, dTargetVecLength)
        && CEpsUtil::Greater(nextTargetVec.Length(), dEdgeLengthTh))
    {
        // 頂点追加
        BoostPolyline::iterator itInsert = itTargetPt;
        if (nTargetStep > 0)
            itInsert = itTargetPt + nTargetStep;
        BoostPolyline::iterator itTmp = itTarget->insert(
            itInsert, BoostPoint(targetInsertPt.x, targetInsertPt.y));

        // 追加頂点のイテレータを元に注目頂点のイテレータを更新
        itTargetPt = itTmp - nTargetStep;

        // 注目エッジを更新
        targetVec.x = (itTargetPt + nTargetStep)->x() - itTargetPt->x();
        targetVec.y = (itTargetPt + nTargetStep)->y() - itTargetPt->y();
        dTargetVecLength = targetVec.Length();
        targetVec.Normalize();  // 正規化
    }

    if (CEpsUtil::Greater(dTwinInnerProduct, dEdgeLengthTh)
        && CEpsUtil::Less(dTwinInnerProduct, dTwinVecLength)
        && CEpsUtil::Greater(nextTwinVec.Length(), dEdgeLengthTh))
    {
        // 頂点追加
        BoostPolyline::iterator itInsert = itTwinPt;
        if (nTwinStep > 0)
            itInsert = itTwinPt + nTwinStep;
        BoostPolyline::iterator itTmp = itTwin->insert(
            itInsert, BoostPoint(twinInsertPt.x, twinInsertPt.y));

        // 追加頂点のイテレータを元に対向エッジの頂点のイテレータを更新
        itTwinPt = itTmp - nTwinStep;

        // 対向エッジを更新
        twinVec.x = (itTwinPt + nTwinStep)->x() - itTwinPt->x();
        twinVec.y = (itTwinPt + nTwinStep)->y() - itTwinPt->y();
        dTwinVecLength = twinVec.Length();
        twinVec.Normalize();  // 正規化
    }
}

/*!
 * @brief 道路縁短縮処理
 * @param[in]       itTarget                最遠点を含む道路縁のイテレータ
 * @param[in/out]   itTargetPt              最遠点のイテレータ
 * @param[in]       nTargetStep             最遠点を含む経路を走査する際にステップ値
 * @param[in]       targetVec               最遠点を終点、前点を始点とするベクトル(正規化済み)
 * @param[in]       dTargetVecLength        targetVecの正規化前の長さ
 * @param[out]      prevTargetPt            短縮処理で削除した注目点(最遠点)座標(後段の補正処理で使用する)
 * @param[in]       itTwin                  対向点を含む道路縁のイテレータ
 * @param[in/out]   itTwinPt                対向点のイテレータ
 * @param[in]       nTwinStep               最遠点を含む経路を走査する際にステップ値
 * @param[out]      prevTwinPt              短縮処理で削除した対向点座標(後段の補正処理で使用する)
 * @param[in]       dVerticalAngleDiffTh    直交確認用角度差分閾値(90degからの±許容角度)
 * @param[in]       dEdgeLengthTh           最短エッジ長閾値
 * @return          処理結果
 * @retval          true    処理成功
 * @retval          false   処理失敗(エラー)
*/
bool CRoadDivision::roadEdgeShorteningProc(
    BoostMultiLines::iterator &itTarget,
    BoostPolyline::iterator &itTargetPt,
    int nTargetStep,
    CVector2D &targetVec,
    double &dTargetVecLength,
    BoostPoint &prevTargetPt,
    BoostMultiLines::iterator &itTwin,
    BoostPolyline::iterator &itTwinPt,
    int nTwinStep,
    BoostPoint &prevTwinPt,
    const double dVerticalAngleDiffTh,
    const double dEdgeLengthTh)
{
    // 短縮処理
    CVector2D targetSrcPt(itTargetPt->x(), itTargetPt->y());

    // 注目エッジの始点と対向エッジの始点を繋ぐベクトル
    CVector2D vec = CVector2D(itTwinPt->x() - itTargetPt->x(), itTwinPt->y() - itTargetPt->y());
    double dAngle = CGeoUtil::Angle(targetVec, vec);
    double dEpsilon = abs(90.0 - dAngle);
    if (!CEpsUtil::Less(dEpsilon, dVerticalAngleDiffTh))
    {
        // 注目エッジと対向エッジの端点を結んだエッジのなす角が直角ではない場合
        // 注目エッジに垂線を下した際の交点を取得する
        double dInnerProduct = CGeoUtil::InnerProduct(targetVec, vec);
        CVector2D pt = dInnerProduct * targetVec + targetSrcPt;
        // 交点と注目エッジの終点を結んだエッジ
        CVector2D tmpPt((itTargetPt + nTargetStep)->x(), (itTargetPt + nTargetStep)->y());
        CVector2D tmpVec = tmpPt - pt;
        if (CEpsUtil::Less(dInnerProduct, dTargetVecLength)
            && CEpsUtil::Greater(tmpVec.Length(), dEdgeLengthTh))
        {
            // 交点が注目エッジ上に位置し、短縮した際にエッジ長が閾値より大きい場合
            // 後段の補正処理で使用するため削除する頂点座標を保持する
            prevTargetPt = BoostPoint(*itTargetPt);

            // 座標値を更新
            itTargetPt->x(pt.x);
            itTargetPt->y(pt.y);
        }
        else
        {
            // 交点が注目エッジ上に位置しない
            // または、短縮した際にエッジ長が閾値以下の場合

            // 後段の補正処理で使用するため削除する頂点座標を保持する
            prevTargetPt = BoostPoint(*itTargetPt);

            // 注目点を削除して、次点に移動する
            itTargetPt = itTarget->erase(itTargetPt);
            if (nTargetStep < 0 && itTarget->size() > 0)
                itTargetPt = itTarget->end() - 1;
            else
                itTargetPt = itTarget->begin();
        }
    }
    else
    {
        // 注目エッジと対向エッジの端点を結んだエッジのなす角が直角の場合

        // 注目点の次点と対向点の次点を繋いだエッジの確認を行う
        vec.x = (itTwinPt + nTwinStep)->x() - (itTargetPt + nTargetStep)->x();
        vec.y = (itTwinPt + nTwinStep)->y() - (itTargetPt + nTargetStep)->y();
        dAngle = CGeoUtil::Angle(targetVec, vec);
        double dEpsilon = abs(90.0 - dAngle);

        if (CEpsUtil::Less(dEpsilon, dVerticalAngleDiffTh))
        {
            // 次点同士を結んだエッジが注目エッジに垂直な場合

            // 後段の補正処理で使用するため削除する頂点座標を保持する
            prevTargetPt = BoostPoint(*itTargetPt);
            prevTwinPt = BoostPoint(*itTwinPt);

            // 注目点を削除して、次点に移動する
            itTargetPt = itTarget->erase(itTargetPt);   // 削除した点の次イテレータが返却
            if (nTargetStep < 0 && itTarget->size() > 0)
                itTargetPt = itTarget->end() - 1;
            else
                itTargetPt = itTarget->begin();

            // 対向点を削除して、次点に移動する
            itTwinPt = itTwin->erase(itTwinPt); // 削除した点の次イテレータが返却
            if (nTwinStep < 0 && itTwin->size() > 0)
                itTwinPt = itTwin->end() - 1;
            else
                itTwinPt = itTwin->begin();
        }
        else
        {
            return false;  // TODO イレギュラー
        }
    }

    return true;    // 正常処理
}

/*!
 * @brief 道路縁短縮後の補正処理
 * @param[in]       itTarget            注目線イテレータ
 * @param[in/out]   itTargetPt          注目点イテレータ
 * @param[in]       nTargetStep         注目点イテレータのループ用ステップ数
 * @param[in]       prevTargetPt        注目点の前点座標
 * @param[in]       dCorrectionAngleTh  水平確認の角度閾値(deg)
*/
void CRoadDivision::correctionRoadEdgeProc(
    BoostMultiLines::iterator &itTarget,
    BoostPolyline::iterator &itTargetPt,
    int nTargetStep,
    BoostPoint &prevTargetPt,
    const double dCorrectionAngleTh)
{
    if (!bg::is_empty(prevTargetPt))
    {
        while (itTarget->size() > 2)
        {
            CVector2D prevPt(prevTargetPt.x(), prevTargetPt.y());
            CVector2D currentPt(itTargetPt->x(), itTargetPt->y());
            CVector2D nextPt((itTargetPt + nTargetStep)->x(), (itTargetPt + nTargetStep)->y());
            CVector2D vec1 = currentPt - prevPt;
            CVector2D vec2 = nextPt - currentPt;
            double dAngle = CGeoUtil::Angle(vec1, vec2);
            if (CEpsUtil::LessEqual(dAngle, dCorrectionAngleTh))
            {
                // 水平な場合は注目点を次点に移動する
                prevTargetPt = BoostPoint(*itTargetPt); // 更新

                // 注目点を削除して、次点に移動する
                itTargetPt = itTarget->erase(itTargetPt);
                if (nTargetStep < 0 && itTarget->size() > 0)
                    itTargetPt = itTarget->end() - 1;
                else
                    itTargetPt = itTarget->begin();
            }
            else
            {
                break;  // 補正処理終了
            }
        }
    }
}

/*!
 * @brief 交差点ポリゴンの作成
 * @param[in] crossingOutlines 交差点ポリゴンの輪郭線に該当する線分データ
 * @return 交差点ポリゴン
*/
BoostPolygon CRoadDivision::loopingCrossingArea(
    BoostMultiLines crossingOutlines)
{
    BoostPolygon polygon;

    // 車道交差部の輪郭線をループ化
    BoostUndirectedGraph graph = CBoostGraphUtil::CreateGraph(crossingOutlines);
    // 経路探索の開始頂点
    BoostVertexDesc searchVertexDesc = BoostUndirectedGraph::null_vertex();
    BOOST_FOREACH(BoostVertexDesc vertexDesc, boost::vertices(graph))
    {
        if (boost::degree(vertexDesc, graph) == 2)
        {
            searchVertexDesc = vertexDesc;
            break;
        }
    }
    if (searchVertexDesc != BoostUndirectedGraph::null_vertex())
    {
        // 深さ優先探索
        std::vector<boost::default_color_type> vecVertexColor(boost::num_vertices(graph));
        auto idmap = get(boost::vertex_index, graph);
        auto vcmap = boost::make_iterator_property_map(vecVertexColor.begin(), idmap);

        std::vector<BoostVertexDesc> vecRoute;  // 経路
        CBoostDFSVisitor vis(vecRoute);
        boost::depth_first_visit(graph, searchVertexDesc, vis, vcmap);

        // 車道交差部ポリゴン
        for (auto itDesc = vecRoute.begin(); itDesc != vecRoute.end(); itDesc++)
        {
            polygon.outer().push_back(graph[*itDesc].pt);
        }
        bg::correct(polygon);
    }

    return polygon;
}

/*!
 * @brief 車道交差部切断
 * @param[in]   roadPolyogns        道路ポリゴン群
 * @param[in]   blockPolyogns       街区ポリゴン群
 * @param[in]   crossing            交差点情報群
 * @param[out]  crossingPolygons    交差点領域群
 * @param[out]  remainingPolygons   残存道路ポリゴン群
*/
void CRoadDivision::createCrossingPolygons(
    BoostMultiPolygon &roadPolyogns,
    BoostMultiPolygon &blockPolyogns,
    std::vector<CCrossingData> &crossing,
    std::vector<CRoadData> &crossingPolygons,
    BoostMultiPolygon &remainingPolygons)
{
    const double dParallelAngleTh = 10.0;       // 平行確認用角度閾値
    const double dVerticalAngleDiffTh = 10.0;   // 直交確認用角度差分閾値(90degからの±許容角度)
    const double dEdgeLengthTh = 0.5;           // 最短エッジ長閾値
    const double dCorrectionAngleTh = 10.0;     // 補正対象の道路縁の角度閾値

    // 交差点ごとに作業
    std::vector<CRoadData> tmpCrossingPolygons;
    for (auto itCross = crossing.begin(); itCross != crossing.end(); itCross++)
    {
        if (!bg::is_empty(itCross->Area()))
        {
            // 交差点領域が取得出来ている場合
            BoostPolygon area = itCross->Area();
            BoostPoint crossPt = itCross->Point();  // 交差点座標

            // 交差点領域と重畳する道路縁を取得する
            BoostMultiLines lines = GetRoadEdges(roadPolyogns, area, crossPt);

            // 道路縁は線分状態のため結合して経路状態にする
            BoostMultiLines routes = GetRoutes(lines);

            if (routes.size() != itCross->BranchNum())
            {
                // todo
                // 分岐数と道路縁の本数が一致しない場合の対応
                continue;
            }

            // 分岐数分、切断位置特定処理を行う
            std::set<BoostPolyline::iterator> usedPtList;   // 使用済み頂点
            std::vector<PairData> segmentationLines; // 分割縁
            for (int nCount = 0; nCount < itCross->BranchNum(); nCount++)
            {
                // 最遠点、対向点の探索
                BoostMultiLines::iterator itTarget, itTwin;
                BoostPolyline::iterator itTargetPt, itTwinPt, itTargetEnd, itTwinEnd;
                CVector2D targetVec, twinVec;
                int nTargetStep, nTwinStep;
                if (!SearchTarget(routes, crossPt, usedPtList, itTarget, itTargetPt,
                    targetVec, itTargetEnd, nTargetStep))
                    continue;   // 未発見の場合

                if (!SearchTwin(routes, itTarget, itTargetPt, targetVec, usedPtList,
                    itTwin, itTwinPt, twinVec, itTwinEnd, nTwinStep))
                    continue;   // 未発見の場合

                CVector2D targetSrcPt(itTargetPt->x(), itTargetPt->y());

                if (itTwin != routes.end())
                {
                    // 対向エッジを発見した場合
                    BoostPoint prevTargetPt, prevTwinPt;
                    while (itTarget->size() > 1 && itTwin->size() > 1
                        && itTargetPt != itTargetEnd && itTwinPt != itTwinEnd)
                    {
                        // 注目道路縁と対向道路縁の点数が2点以上かつ
                        // 注目点と対向点が有効な場合

                        // 最新情報に更新
                        targetSrcPt.x = itTargetPt->x();
                        targetSrcPt.y = itTargetPt->y();

                        // 対向エッジが平行か確認する
                        targetVec.x = (itTargetPt + nTargetStep)->x() - itTargetPt->x();
                        targetVec.y = (itTargetPt + nTargetStep)->y() - itTargetPt->y();
                        twinVec.x = (itTwinPt + nTwinStep)->x() - itTwinPt->x();
                        twinVec.y = (itTwinPt + nTwinStep)->y() - itTwinPt->y();
                        double dAngle = CGeoUtil::Angle(targetVec, twinVec);
                        if (!CEpsUtil::Less(dAngle, dParallelAngleTh))
                        {
                            break;   // 平行でない場合は終了
                        }

                        // 注目エッジの長さを取得
                        double dTargetVecLength = targetVec.Length();
                        targetVec.Normalize();  // 正規化
                        // 対向エッジの長さを取得
                        double dTwinVecLength = twinVec.Length();
                        twinVec.Normalize();    // 正規化

                        // 注目エッジと対向エッジの端点を結んだエッジのなす角が直角の場合に向けての前処理
                        roadEdgeShorteningPreProc(
                            itTarget, itTargetPt, nTargetStep, targetVec, dTargetVecLength,
                            itTwin, itTwinPt, nTwinStep, twinVec, dTwinVecLength, dEdgeLengthTh);

                        // 短縮処理
                        if (!roadEdgeShorteningProc(
                            itTarget, itTargetPt, nTargetStep, targetVec, dTargetVecLength, prevTargetPt,
                            itTwin, itTwinPt, nTwinStep, prevTwinPt, dVerticalAngleDiffTh, dEdgeLengthTh))
                            break;  // 短縮処理にエラーが発生した場合
                    }

                    // 分割線を作成するための頂点ペアを保持
                    bool bTarget = true;
                    if (itTarget->begin() != itTargetPt)
                        bTarget = false;  // 終点の場合
                    bool bTwin = true;
                    if (itTwin->begin() != itTwinPt)
                        bTwin = false;  // 終点の場合
                    PairData pairData(
                        itTarget, bTarget, nTargetStep, prevTargetPt,
                        itTwin, bTwin, nTwinStep, prevTwinPt);
                    segmentationLines.push_back(pairData);

                    // 短縮処理済みリストに登録
                    usedPtList.insert(itTargetPt);
                    usedPtList.insert(itTwinPt);
                }
            }

#if 0
            // todo 要検討
            // 補正処理
            BoostMultiLines crossingOutlines;  // 交差点領域の外輪郭線
            for (std::vector<PairData>::iterator itPair = segmentationLines.begin();
                itPair != segmentationLines.end(); itPair++)
            {
                BoostPolyline::iterator itTargetPt = itPair->itTarget->begin();
                BoostPolyline::iterator itTwinPt = itPair->itTwin->begin();
                if (!itPair->bTarget)
                    itTargetPt = itPair->itTarget->end() - 1;
                if (!itPair->bTwin)
                    itTwinPt = itPair->itTwin->end() - 1;

                correctionRoadEdgeProc(
                    itPair->itTarget, itTargetPt, itPair->nTargetStep, itPair->prevTargetPt, dCorrectionAngleTh);
                correctionRoadEdgeProc(
                    itPair->itTwin, itTwinPt, itPair->nTwinStep, itPair->prevTwinPt, dCorrectionAngleTh);

                BoostPolyline line;
                line.push_back(*itTargetPt);
                line.push_back(*itTwinPt);
                crossingOutlines.push_back(line);
            }
#else
            BoostMultiLines crossingOutlines;  // 交差点領域の外輪郭線
            for (std::vector<PairData>::iterator itPair = segmentationLines.begin();
                itPair != segmentationLines.end(); itPair++)
            {
                BoostPolyline::iterator itTargetPt = itPair->itTarget->begin();
                BoostPolyline::iterator itTwinPt = itPair->itTwin->begin();
                if (!itPair->bTarget)
                    itTargetPt = itPair->itTarget->end() - 1;
                if (!itPair->bTwin)
                    itTwinPt = itPair->itTwin->end() - 1;

                BoostPolyline line;
                line.push_back(*itTargetPt);
                line.push_back(*itTwinPt);
                crossingOutlines.push_back(line);
            }
#endif
            // 近傍道路縁を短縮した際に残った線がある場合、車道交差部の輪郭線として記憶
            for (auto itLine = routes.begin(); itLine != routes.end(); itLine++)
            {
                if (bg::length(*itLine))
                {
                    crossingOutlines.push_back(*itLine);
                }
            }

            // 車道交差部の輪郭線をループ化
            BoostPolygon polygon = loopingCrossingArea(crossingOutlines);

            // 道路からはみ出ていないか確認
            BoostMultiPolygon overlapRegions;
            bg::intersection(roadPolyogns, polygon, overlapRegions);
            double dDiffArea = CAnalyzeRoadEdgeGeomUtil::RoundN(abs(bg::area(overlapRegions) - bg::area(polygon)), 3);

            if (!bg::is_empty(polygon) && bg::is_valid(polygon)
                && CEpsUtil::Zero(dDiffArea))
            {
                // 車道交差部ポリゴンの保存
                CRoadData roadData;
                roadData.Polygon(polygon);
                roadData.Type(RoadSectionType::ROAD_SECTION_CROSSING);
                roadData.Division(itCross->BranchNum());
                tmpCrossingPolygons.push_back(roadData);
            }
        }
    }

    // 道路ポリゴンから車道交差部ポリゴンを引く
    std::vector<CRoadData> errCrossingPolygons; // 分割時に不正ポリゴンが発生する交差点ポリゴン情報群
    divisionPolygon(roadPolyogns, tmpCrossingPolygons, crossingPolygons, errCrossingPolygons, remainingPolygons);
}

// ポリゴンの分割
/*!
 * @brief
 * @param[in]   roadPolyogns        道路ポリゴン
 * @param[in]   roadData            分割対象の道路ポリゴン群
 * @param[out]  dstData             分割が正常に行えた道路ポリゴン群
 * @param[out]  errData             分割時に不正ポリゴンが発生する道路ポリゴン群
 * @param[out]  remainingPolygons   残存道路
*/
void CRoadDivision::divisionPolygon(
    BoostMultiPolygon &roadPolyogns,
    std::vector<CRoadData> &roadData,
    std::vector<CRoadData> &dstData,
    std::vector<CRoadData> &errData,
    BoostMultiPolygon &remainingPolygons)
{
    const double dSampling = 0.1;
    // 道路ポリゴンから対象ポリゴンを引く
    remainingPolygons = BoostMultiPolygon(roadPolyogns);
    for (auto it = roadData.begin(); it != roadData.end(); it++)
    {
        BoostMultiPolygon tmpRemainingPolygons(remainingPolygons);
        for (auto itPoly = tmpRemainingPolygons.begin(); itPoly != tmpRemainingPolygons.end(); itPoly++)
        {
            if (bg::disjoint(*itPoly, it->Polygon()))
                continue;

            // 近傍探索用のデータ作成
            BoostPolygonRTree rtree;
            for (auto itPt = itPoly->outer().begin(); itPt < itPoly->outer().end() - 1; itPt++)
            {
                auto itNextPt = itPt + 1;
                BoostPolyline line;
                line.push_back(*itPt);
                line.push_back(*itNextPt);
                BoostPolyline samplingLine = CAnalyzeRoadEdgeGeomUtil::Sampling(line, dSampling);

                for (auto itSamplingPt = samplingLine.begin(); itSamplingPt != samplingLine.end() - 1; itSamplingPt++)
                {
                    NearestPointInfo info(itPoly, itPt, false);
                    rtree.insert(std::pair<BoostPoint, NearestPointInfo>(*itSamplingPt, info));
                }
            }
            int nInnerIdx = 0;
            for (auto itInner = itPoly->inners().begin(); itInner != itPoly->inners().end(); itInner++, nInnerIdx++)
            {
                for (auto itPt = itInner->begin(); itPt < itInner->end() - 1; itPt++)
                {
                    auto itNextPt = itPt + 1;
                    BoostPolyline line;
                    line.push_back(*itPt);
                    line.push_back(*itNextPt);
                    BoostPolyline samplingLine = CAnalyzeRoadEdgeGeomUtil::Sampling(line, dSampling);

                    for (auto itSamplingPt = samplingLine.begin(); itSamplingPt != samplingLine.end() - 1; itSamplingPt++)
                    {
                        NearestPointInfo info(itPoly, itPt, true, nInnerIdx);
                        rtree.insert(std::pair<BoostPoint, NearestPointInfo>(*itSamplingPt, info));
                    }
                }
            }

            // 補正の関係で被減算対象に減算対象の頂点を追加する
            std::vector<AddPointInfo> vecAddPoints;
            BoostPolygon polygon = it->Polygon();
            for (auto itCrossPt = polygon.outer().begin(); itCrossPt != polygon.outer().end() - 1; itCrossPt++)
            {
                // 最近傍探索
                std::vector<std::pair<BoostPoint, NearestPointInfo>> vecValues;
                rtree.query(bg::index::nearest(*itCrossPt, 1), std::back_inserter(vecValues));

                // 挿入地点の決定
                // 挿入予定辺の始終点
                BoostRing::iterator itStartPt = vecValues[0].second.m_itPt;
                BoostRing::iterator itNextPt = itStartPt + 1;
                CVector2D startPos(itStartPt->x(), itStartPt->y());
                CVector2D nextPos(itNextPt->x(), itNextPt->y());

                CVector2D targetPos(itCrossPt->x(), itCrossPt->y());
                CVector2D vec1 = nextPos - startPos;    // 挿入予定辺のベクトル
                double dLength = vec1.Length();
                vec1.Normalize();
                CVector2D vec2 = targetPos - startPos;  // 始点から挿入点までのベクトル
                double dInnerProduct = CGeoUtil::InnerProduct(vec1, vec2);  // 射影
                dInnerProduct = CAnalyzeRoadEdgeGeomUtil::RoundN(dInnerProduct, 3);
                double dAngle = CGeoUtil::Angle(vec1, vec2);

                int nInnerIdx = -1;
                BoostRing::iterator itPrevPt = itStartPt - 1;
                BoostRing::iterator itBegin = itPoly->outer().begin();
                if (vecValues[0].second.m_bInner)
                {
                    nInnerIdx = vecValues[0].second.m_nInnerIdx;
                    itBegin = itPoly->inners()[vecValues[0].second.m_nInnerIdx].begin();
                    if (itPoly->inners()[vecValues[0].second.m_nInnerIdx].begin() > itPrevPt)
                        itPrevPt = itPoly->inners()[vecValues[0].second.m_nInnerIdx].end() - 1;
                }
                else
                {
                    if (itPoly->outer().begin() > itPrevPt)
                        itPrevPt = itPoly->outer().end() - 1;
                }
                if (CEpsUtil::Less(dInnerProduct, 0))
                {
                    CVector2D p1(itPrevPt->x(), itPrevPt->y());
                    CVector2D p2(itStartPt->x(), itStartPt->y());
                    vec1 = p2 - p1;
                    dLength = vec1.Length();
                    vec1.Normalize();
                    vec2 = targetPos - p1;
                    dInnerProduct = CGeoUtil::InnerProduct(vec1, vec2);
                    dInnerProduct = CAnalyzeRoadEdgeGeomUtil::RoundN(dInnerProduct, 3);
                    dAngle = CGeoUtil::Angle(vec1, vec2);
                    if (CEpsUtil::Zero(dAngle)
                        && CEpsUtil::Greater(dInnerProduct, 0)
                        && CEpsUtil::Less(dInnerProduct, dLength))
                    {
                        size_t offset = static_cast<size_t>(std::distance(itBegin, itStartPt));
                        AddPointInfo info(*itCrossPt, offset, dInnerProduct, nInnerIdx);
                        vecAddPoints.push_back(info);
                    }
                }
                else if (CEpsUtil::Zero(dAngle)
                    && CEpsUtil::Greater(dInnerProduct, 0)
                    && CEpsUtil::Less(dInnerProduct, dLength))
                {
                    size_t offset = static_cast<size_t>(std::distance(itBegin, itNextPt));
                    AddPointInfo info(*itCrossPt, offset, dInnerProduct, nInnerIdx);
                    vecAddPoints.push_back(info);
                }
            }

            // 頂点追加
            std::sort(vecAddPoints.begin(), vecAddPoints.end(), std::greater<AddPointInfo>());
            for (auto itInfo = vecAddPoints.begin(); itInfo != vecAddPoints.end(); itInfo++)
            {
                if (itInfo->nInnerIdx < 0)
                {
                    itPoly->outer().insert(itPoly->outer().begin() + itInfo->offset, itInfo->pt);
                }
                else
                {
                    itPoly->inners()[itInfo->nInnerIdx].insert(
                        itPoly->inners()[itInfo->nInnerIdx].begin() + itInfo->offset, itInfo->pt);
                }
            }
            break;
        }

        bool bSpike, bCross;
        BoostPolygon crossingArea = it->Polygon();
        BoostMultiPolygon tmpDiff = difference(tmpRemainingPolygons, crossingArea, bSpike, bCross);

        double dDiffArea = abs(bg::area(tmpRemainingPolygons) - bg::area(tmpDiff));
        dDiffArea = CAnalyzeRoadEdgeGeomUtil::RoundN((dDiffArea - bg::area(crossingArea)), 3);

        if (!bSpike && !bCross && CEpsUtil::Zero(dDiffArea))
        {
            remainingPolygons = tmpDiff;
            dstData.push_back(*it);
        }
        else
        {
            errData.push_back(*it);
        }
    }
}

/*!
 * @brief ポリゴンの減算処理(スパイクノイズ/自己交差確認のみ)
 * @param[in]   polygons    被減算領域
 * @param[in]   polygon     減算領域
 * @param[out]  bSpike      減算結果のスパイクノイズの有無
 * @param[out]  bCross      減算結果の自己交差の有無
 * @return 減算結果
*/
BoostMultiPolygon CRoadDivision::difference(
    BoostMultiPolygon &polygons,
    BoostPolygon &polygon,
    bool &bSpike,
    bool &bCross)
{
    bSpike = false;
    bCross = false;
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
            bg::difference(*itPoly, polygon, tmpDiff);
            diffPolygons.insert(diffPolygons.end(), tmpDiff.begin(), tmpDiff.end());

            if (!bSpike || !bCross)
            {
                for (auto itDiff = tmpDiff.begin(); itDiff != tmpDiff.end(); itDiff++)
                {
                    // スパイクノイズ確認
                    BoostMultiPoints spikePts;
                    bSpike = CAnalyzeRoadEdgeGeomUtil::CheckSpike(*itDiff, spikePts, true);

                    // 自己交差確認
                    BoostMultiPoints crossPts;
                    bCross = CAnalyzeRoadEdgeGeomUtil::CheckSelfIntersection(*itDiff, crossPts);

                    if (bSpike || bCross)
                        break;
                }
            }
        }
    }

    return diffPolygons;
}
