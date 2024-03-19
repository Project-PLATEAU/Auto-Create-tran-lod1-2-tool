#include "pch.h"
#include "CRoadCenterLine.h"
#include "CAnalyzeRoadEdgeGeomUtil.h"
#include "CBoostGraphUtil.h"
#include "boost/graph/dijkstra_shortest_paths.hpp"

struct Point {
    int a;
    int b;
    Point(int x, int y) : a(x), b(y) {}
};

struct Segment {
    Point p0;
    Point p1;
    Segment(int x1, int y1, int x2, int y2) : p0(x1, y1), p1(x2, y2) {}
};

namespace boost {
    namespace polygon {

        template <>
        struct geometry_concept<Point> {
            typedef point_concept type;
        };

        template <>
        struct point_traits<Point> {
            typedef int coordinate_type;

            static inline coordinate_type get(
                const Point &point, orientation_2d orient) {
                return (orient == HORIZONTAL) ? point.a : point.b;
            }
        };

        template <>
        struct geometry_concept<Segment> {
            typedef segment_concept type;
        };

        template <>
        struct segment_traits<Segment> {
            typedef int coordinate_type;
            typedef Point point_type;

            static inline point_type get(const Segment &segment, direction_1d dir) {
                return dir.to_int() ? segment.p1 : segment.p0;
            }
        };
    }  // polygon
}  // boost


/*!
 * @brief コンストラクタ
*/
CRoadCenterLine::CRoadCenterLine()
{
    m_centerLines = BoostMultiLines();
    m_internalCenterLines = BoostMultiLines();
    m_shrinkRoadPolygons = BoostMultiPolygon();
    m_cycles = BoostMultiPolygon();
    m_crossings.clear();
}

/*!
 * @brief 交差点近傍の道路中心線を取得する
 * @param[in] crossPt   交差点座標
 * @param[in] area      交差点エリア
 * @return 道路中心線
*/
BoostMultiLines CRoadCenterLine::CenterLines(
    BoostPoint &crossPt,
    BoostPolygon &area)
{
    BoostMultiLines routes;
    if (m_rtree.size() > 0 && boost::num_edges(m_graph) > 0)
    {
        // 交差点探索
        std::vector<std::pair<BoostPoint, BoostVertexDesc>> vecValues;
        m_rtree.query(bg::index::nearest(crossPt, 2), std::back_inserter(vecValues));
        BoostVertexDesc crossVertexDesc = BoostUndirectedGraph::null_vertex();
        double dLength = DBL_MAX;
        for (auto itVal = vecValues.begin(); itVal != vecValues.end(); itVal++)
        {
            double dTmpLength = CAnalyzeRoadEdgeGeomUtil::Length(crossPt, itVal->first);
            if (dTmpLength < dLength)
            {
                dLength = dTmpLength;
                crossVertexDesc = itVal->second;
            }
        }

        // 交差点から延びる経路ごとに処理
        BoostEdgeRTree tmpRTree;
        BOOST_FOREACH(BoostEdgeDesc edgeDesc, boost::out_edges(crossVertexDesc, m_graph))
        {
            BoostVertexDesc srcDesc = boost::source(edgeDesc, m_graph);
            BoostVertexDesc trgDesc = boost::target(edgeDesc, m_graph);

            BoostPolyline route;
            route.push_back(m_graph[crossVertexDesc].pt);
            std::set<BoostVertexDesc> routeVertices;
            routeVertices.insert(crossVertexDesc);
            while (trgDesc != BoostUndirectedGraph::null_vertex())
            {
                BoostPolyline line;
                line.push_back(m_graph[srcDesc].pt);
                line.push_back(m_graph[trgDesc].pt);

                if (bg::disjoint(line, area))
                {
                    trgDesc = BoostUndirectedGraph::null_vertex();
                }
                else
                {
                    route.push_back(m_graph[trgDesc].pt);
                    routeVertices.insert(trgDesc);

                    if (boost::degree(trgDesc, m_graph) != 2)
                        break;  // 挿入した点が分岐や端点の場合は終了

                    // 更新
                    srcDesc = trgDesc;
                    trgDesc = BoostUndirectedGraph::null_vertex();
                    BOOST_FOREACH(BoostEdgeDesc tmpEdgeDesc, boost::out_edges(srcDesc, m_graph))
                    {
                        if (routeVertices.find(boost::target(tmpEdgeDesc, m_graph)) == routeVertices.end())
                        {
                            trgDesc = boost::target(tmpEdgeDesc, m_graph);
                            break;
                        }
                    }
                }
            }

            if (route.size() > 1)
                routes.push_back(route);
        }
    }
    return routes;
}

/*!
 * @brief 道路中心線作成
 * @param[in] roadPolygons  道路ポリゴン
 * @param[in] dReso         解像度(m/px)
 * @param[in] dShrink       収縮サイズ(m)
*/
void CRoadCenterLine::CreateCenterLine(
    const BoostMultiPolygon &roadPolygons,
    const double dReso,
    const double dShrink)
{
    assert(CEpsUtil::Greater(dReso, 0));
    assert(CEpsUtil::LessEqual(dShrink, 0));

    m_centerLines.clear();

    // ボロノイ分割線による道路中心線の作成
    getVoronoiEdges(roadPolygons, dReso, dShrink, m_internalCenterLines);

    // 無向グラフの作成
    BoostUndirectedGraph graph = CBoostGraphUtil::CreateGraph(m_internalCenterLines);

    //道路中心線のノイズ除去
    deleteNoise(graph);

    // 交差点のマージ
    mergeCrossing(graph);

    // 道路中心線のエッジ抽出
    // deleteNoiseCrossing()でm_centerLinesを参照するため記載順に注意
    BOOST_FOREACH(BoostEdgeDesc desc, boost::edges(graph))
    {
        BoostPolyline line;
        BoostEdgeProperty edge = graph[desc];
        line.push_back(graph[edge.vertexDesc1].pt);
        line.push_back(graph[edge.vertexDesc2].pt);
        m_centerLines.push_back(line);
    }

    // 交差点の抽出
    //m_crossings = deleteNoiseCrossing(graph, roadPolygons, 5.0);
    // 交差点の抽出
    m_crossings.clear();
    BOOST_FOREACH(BoostVertexDesc desc, boost::vertices(graph))
    {
        int nBranch = static_cast<int>(boost::degree(desc, graph));
        if (nBranch > 2)
        {
            // 交差点
            CCrossingData data(graph[desc].pt, nBranch);
            m_crossings.push_back(data);
        }
    }

    // 道路中心線のグラフを内部的に保持
    m_graph = graph;
    // rtree作成
    m_rtree.clear();
    BOOST_FOREACH(BoostVertexDesc vertexDesc, boost::vertices(graph))
    {
        // 分岐点をrtreeに登録
        if (boost::degree(vertexDesc, graph) > 2)
            m_rtree.insert(std::pair<BoostPoint, BoostVertexDesc>(graph[vertexDesc].pt, vertexDesc));
    }
}

/*!
 * @brief 交差点取得(座標情報のみ)
 * @return  交差点座標群
*/
BoostMultiPoints CRoadCenterLine::GetCrossPoints()
{
    BoostMultiPoints pts;
    for (auto it = m_crossings.begin(); it != m_crossings.end(); it++)
    {
        pts.push_back(it->Point());
    }
    return pts;
}

/*!
 * @brief 注目エリアの交差点取得(分岐数付き)
 * @param[in] area 注目エリア
 * @return 交差点情報群
*/
std::vector<CCrossingData> CRoadCenterLine::GetCrossPointsWithBranchNum(const BoostBox &area)
{
    std::vector<CCrossingData> crossings;
    std::set<BoostVertexDesc> selectedVerties;
    if (boost::num_edges(m_graph) > 0)
    {
        // 注目領域内(境界含む)の交差点取得
        BOOST_FOREACH(BoostVertexDesc vertexDesc, boost::vertices(m_graph))
        {
            int nBranch = static_cast<int>(boost::degree(vertexDesc, m_graph));
            if (nBranch > 2 && bg::covered_by(m_graph[vertexDesc].pt, area))
            {
                CCrossingData data(m_graph[vertexDesc].pt, nBranch);
                crossings.push_back(data);
                selectedVerties.insert(vertexDesc);   // 取得済み頂点を保持
            }
        }

        // 注目領域の境界を横断している道路が存在する場合は、
        // 道路の延長上に存在する注目領域外の交差点も取得する
        // 注目領域のポリゴン化
        BoostPolygon polygon;
        bg::convert(area, polygon);
        // 注目領域の境界線
        BoostPolyline line;
        BOOST_FOREACH(BoostPoint pt, polygon.outer())
        {
            line.push_back(pt);
        }
        // 境界線と衝突する道路中心線のエッジ探索
        BOOST_FOREACH(BoostEdgeDesc edgeDesc, boost::edges(m_graph))
        {
            BoostPolyline edge;
            edge.push_back(m_graph[m_graph[edgeDesc].vertexDesc1].pt);
            edge.push_back(m_graph[m_graph[edgeDesc].vertexDesc2].pt);
            if (!bg::disjoint(line, edge))
            {
                // 境界線と衝突するエッジを発見した場合は、エッジの両端から経路を辿って交差点を探索する
                std::vector<std::pair<BoostVertexDesc, BoostEdgeDesc>> vecSearch;
                std::pair<BoostVertexDesc, BoostEdgeDesc> pair1(m_graph[edgeDesc].vertexDesc1, edgeDesc);
                std::pair<BoostVertexDesc, BoostEdgeDesc> pair2(m_graph[edgeDesc].vertexDesc2, edgeDesc);
                vecSearch.push_back(pair1);
                vecSearch.push_back(pair2);
                for (auto itPair = vecSearch.begin(); itPair != vecSearch.end(); itPair++)
                {
                    BoostVertexDesc targetVertexDesc = itPair->first;
                    BoostEdgeDesc prevEdgeDesc = itPair->second;
                    std::set<BoostVertexDesc> route;
                    while (static_cast<int>(boost::degree(targetVertexDesc, m_graph)) == 2)
                    {
                        if (route.find(targetVertexDesc) != route.end())
                            break;  // 通過点に戻って来た場合

                        route.insert(targetVertexDesc); // 注目頂点の登録
                        // 次の移動頂点の探索
                        BOOST_FOREACH(BoostEdgeDesc nextEdgeDesc, boost::out_edges(targetVertexDesc, m_graph))
                        {
                            if (nextEdgeDesc != prevEdgeDesc)
                            {
                                prevEdgeDesc = nextEdgeDesc;

                                if (m_graph[nextEdgeDesc].vertexDesc1 != targetVertexDesc)
                                {
                                    targetVertexDesc = m_graph[nextEdgeDesc].vertexDesc1;
                                }
                                else
                                {
                                    targetVertexDesc = m_graph[nextEdgeDesc].vertexDesc2;
                                }
                                break;
                            }
                        }
                    }

                    int nBranch = static_cast<int>(boost::degree(targetVertexDesc, m_graph));
                    if (nBranch > 2 && !bg::covered_by(m_graph[targetVertexDesc].pt, area)
                        && selectedVerties.find(targetVertexDesc) == selectedVerties.end())
                    {
                        // 交差点かつ注目領域外かつ未登録の場合
                        CCrossingData data(m_graph[targetVertexDesc].pt, nBranch);
                        crossings.push_back(data);
                        selectedVerties.insert(targetVertexDesc);   // 取得済み頂点を保持
                    }
                }
            }
        }
    }

    return crossings;
}

/*!
 * @brief 道路中心線作成準備
 * @param[in]   roadPolygons    道路ポリゴン群
 * @param[in]   dSampling       サンプリング間隔
 * @param[out]  roadEdges       補助点追加後の道路縁
*/
void CRoadCenterLine::prepareRoadCenterLine(
    const BoostMultiPolygon &roadPolygons,
    const double dSampling,
    BoostMultiLines &roadEdges)
{
    roadEdges.clear();

    for (auto itPolygon = roadPolygons.begin();
        itPolygon != roadPolygons.end(); itPolygon++)
    {
        // 単純化
        BoostPolygon simplePolygon = CAnalyzeRoadEdgeGeomUtil::Simplify(*itPolygon);

        // 詰め替え
        std::vector<CEdge> edges;
        for (auto itPt = simplePolygon.outer().begin();
            itPt < simplePolygon.outer().end() - 1; itPt++)
        {
            CVector2D current(itPt->x(), itPt->y());
            CVector2D next((itPt + 1)->x(), (itPt + 1)->y());
            edges.push_back(CEdge(current, next));
        }
        for (auto itRing = simplePolygon.inners().begin();
            itRing != simplePolygon.inners().end(); itRing++)
        {
            for (auto itPt = itRing->begin(); itPt < itRing->end() - 1; itPt++)
            {
                CVector2D current(itPt->x(), itPt->y());
                CVector2D next((itPt + 1)->x(), (itPt + 1)->y());
                edges.push_back(CEdge(current, next));
            }
        }
        // 長いエッジから処理する
        std::sort(edges.begin(), edges.end(), std::greater<CEdge>());
        // 補助点追加
        for (auto itEdge = edges.begin();
            itEdge != edges.end(); itEdge++)
        {
            if (itEdge->m_middlePoints.size() > 0)
                continue;

            // 辺ベクトル
            CVector2D vec = itEdge->GetVector();
            double len = vec.Length();

            // 直交ベクトル
            CVector2D vecVertical;
            if (CAnalyzeRoadEdgeGeomUtil::GetVerticalVec(vec, vecVertical))
            {
                // 直交ベクトルが道路ポリゴン内に向いているか確認する
                CVector2D pos = vec * 0.5 + itEdge->m_start;
                CVector2D tmpPos = vecVertical * 0.01 + pos;
                if (!bg::within(BoostPoint(tmpPos.x, tmpPos.y), simplePolygon))
                {
                    // 直交ベクトルを反転する
                    vecVertical.Inverse();
                }

                // サンプリング
                std::vector<CVector2D> vecSampling;
                CAnalyzeRoadEdgeGeomUtil::Sampling(
                    itEdge->m_start, itEdge->m_end, vecSampling, dSampling);
                if (vecSampling.size() > 2)
                {
                    std::copy(vecSampling.begin() + 1, vecSampling.end() - 1,
                        std::back_inserter(itEdge->m_middlePoints));    // 始終点を除いてコピー
                }

                // 直交ベクトルと交差する対向位置にあるエッジに補助点を追加する
                std::vector<CVector2D> pts = itEdge->GetPoints();
                for (auto itPt = pts.begin(); itPt != pts.end(); itPt++)
                {
                    std::vector<CEdge>::iterator itCrossEdge = edges.end();
                    double dLength = 0;
                    double dS = 0;
                    CVector2D crossPos;
                    for (auto itTmpEdge = edges.begin();
                        itTmpEdge != edges.end(); itTmpEdge++)
                    {
                        if (itTmpEdge == itEdge)
                            continue;

                        CVector2D tmpCrossPos;
                        bool bOnline1, bOnline2;
                        double s, t;
                        if (CAnalyzeRoadEdgeGeomUtil::GetCrossPos(
                            vecVertical, *itPt, itTmpEdge->GetVector(), itTmpEdge->m_start,
                            tmpCrossPos, bOnline1, bOnline2, t, s))
                        {
                            CVector2D tmpVec = tmpCrossPos - *itPt;
                            if (bOnline2 && t > 0)
                            {
                                if (itCrossEdge == edges.end() || dLength > tmpVec.Length())
                                {
                                    // 初期値設定 or 注目エッジに道路領域をはさんで一番近いエッジで更新
                                    itCrossEdge = itTmpEdge;
                                    dLength = tmpVec.Length();
                                    dS = s;
                                    crossPos = tmpCrossPos;
                                }
                            }
                        }
                    }

                    if (itCrossEdge != edges.end() && !itCrossEdge->IsExitPt(crossPos))
                    {
                        // 補助点の追加作業
                        CVector2D baseVec = itCrossEdge->GetVector();
                        double dBaseLength = baseVec.Length();
                        std::vector<CVector2D>::iterator itTargetPt = itCrossEdge->m_middlePoints.begin();
                        for (; itTargetPt != itCrossEdge->m_middlePoints.end(); itTargetPt++)
                        {
                            // ベクトル係数算出
                            double dTmpLength = (*itTargetPt - itCrossEdge->m_start).Length();
                            double dRate = dTmpLength / dBaseLength;
                            if (dRate > dS)
                                break;
                        }
                        if (itTargetPt == itCrossEdge->m_middlePoints.end())
                        {
                            // 最後に追加
                            itCrossEdge->m_middlePoints.push_back(crossPos);
                        }
                        else
                        {
                            // 途中に追加
                            itCrossEdge->m_middlePoints.insert(itTargetPt, crossPos);
                        }
                    }
                }
            }
        }

        for (auto itEdge = edges.begin(); itEdge != edges.end(); itEdge++)
        {
            BoostPolyline line;
            std::vector<CVector2D> pts = itEdge->GetPoints();
            for (auto itPt = pts.begin(); itPt != pts.end(); itPt++)
                line.push_back(BoostPoint(itPt->x, itPt->y));
            roadEdges.push_back(line);
        }
    }
}

/*!
 * @brief ボロノイ分割線の取得
 * @param[in]   lines           ポリライン群
 * @param[in]   dReso           解像度(m/px)
 * @param[in]   dShrink         収縮サイズ(m)
 * @param[out]  voronoiEdges    ボロノイ分割線群
 * @note    boostのボロノイ分割の入力にdouble座標を入れると挙動がおかしくなったため,
            int座標に変換するため解像度を設定する
 */
void CRoadCenterLine::getVoronoiEdges(
    const BoostMultiPolygon &roadPolygons,
    const double dReso,
    const double dShrink,
    BoostMultiLines &voronoiEdges)
{
    voronoiEdges.clear();

    for (auto itPolygon = roadPolygons.begin();
        itPolygon != roadPolygons.end(); itPolygon++)
    {
        BoostMultiLines inputLines;

        // 単純化
        BoostPolygon simplePolygon = CAnalyzeRoadEdgeGeomUtil::Simplify(*itPolygon);

        // 詰め替え
        BoostPolyline outer;
        std::copy(simplePolygon.outer().begin(), simplePolygon.outer().end(), std::back_inserter(outer));
        inputLines.push_back(outer);
        for (auto itRing = simplePolygon.inners().begin();
            itRing != simplePolygon.inners().end(); itRing++)
        {
            BoostPolyline inner;
            std::copy(itRing->begin(), itRing->end(), std::back_inserter(inner));
            inputLines.push_back(inner);
        }

        // int座標にするためにworld→画像座標
        BoostBox box;
        bg::envelope(inputLines, box);
        double dMargin = dReso * 2.0;
        double dMinX = floor(box.min_corner().x() / dReso) * dReso - dMargin;
        double dMinY = floor(box.min_corner().y() / dReso) * dReso - dMargin;
        double dMaxX = ceil(box.max_corner().x() / dReso) * dReso + dMargin;
        double dMaxY = ceil(box.max_corner().y() / dReso) * dReso + dMargin;
        double dOffsetX = dMinX;
        double dOffsetY = dMaxY;
        std::vector<Segment> segments;
        for (auto itLine = inputLines.cbegin(); itLine != inputLines.cend(); itLine++)
        {
            for (auto itPt = itLine->cbegin(); itPt < itLine->cend() - 1; itPt++)
            {
                int nX1, nY1, nX2, nY2;
                CAnalyzeRoadEdgeGeomUtil::ConvertWorldToPx(
                    itPt->x(), itPt->y(), dOffsetX, dOffsetY, dReso, nX1, nY1);
                CAnalyzeRoadEdgeGeomUtil::ConvertWorldToPx(
                    (itPt + 1)->x(), (itPt + 1)->y(), dOffsetX, dOffsetY, dReso, nX2, nY2);
                segments.push_back(Segment(nX1, nY1, nX2, nY2));
            }
        }

        // ボロノイ分割
        BoostVoronoiDiagram diagram;
        bp::construct_voronoi(segments.begin(), segments.end(), &diagram);

        // 入力道路ポリゴンを収縮
        bg::strategy::buffer::distance_symmetric<double> distStrategy(dShrink);
        bg::strategy::buffer::join_miter joinStrategy;
        bg::strategy::buffer::end_flat endStrategy;
        bg::strategy::buffer::point_circle pointStrategy;
        bg::strategy::buffer::side_straight sideStrategy;
        BoostMultiPolygon shrinkRoadPolygons;
        bg::buffer(
            roadPolygons, shrinkRoadPolygons, distStrategy, sideStrategy,
            joinStrategy, endStrategy, pointStrategy);
        m_shrinkRoadPolygons = shrinkRoadPolygons;

        // polygons内部のボロノイエッジを取得
        std::vector<const bp::voronoi_edge<double> *> vecSearched;
        for (BoostVoronoiEdgeIt itEdge = diagram.edges().begin();
            itEdge != diagram.edges().end(); itEdge++)
        {
            if (itEdge->is_infinite())
                continue;

            // 有限長の場合
            // エッジが探索済みの場合はスキップする
            std::vector<const bp::voronoi_edge<double> *>::iterator targetIt = std::find(
                vecSearched.begin(), vecSearched.end(), &(*itEdge));
            if (targetIt != vecSearched.end())
                continue;

            // 画像→world座標
            double dX1, dY1, dX2, dY2;
            CAnalyzeRoadEdgeGeomUtil::ConvertPxToWorld(
                itEdge->vertex0()->x(), itEdge->vertex0()->y(), dOffsetX, dOffsetY, dReso, dX1, dY1);
            CAnalyzeRoadEdgeGeomUtil::ConvertPxToWorld(
                itEdge->vertex1()->x(), itEdge->vertex1()->y(), dOffsetX, dOffsetY, dReso, dX2, dY2);

            BoostPolyline line;
            line.push_back(BoostPoint(dX1, dY1));
            line.push_back(BoostPoint(dX2, dY2));
            if (bg::within(line, shrinkRoadPolygons))
            //if (bg::within(line, roadPolygons))
            {
                voronoiEdges.push_back(line);
            }

            // 自身と対となるエッジを探索済みに登録
            vecSearched.push_back(&(*itEdge));
            vecSearched.push_back(itEdge->twin());
        }
    }
}

// 無向グラフを利用した道路中心線のノイズ除去
void CRoadCenterLine::deleteNoise(
    BoostUndirectedGraph &graph,
    const double dAngleTh)
{
    bool bTriangle = true;
    bool bSpike = true;
    bool bCycle = true;

    while (bTriangle || bSpike || bCycle)
    {
        // 三角形ノイズの除去
        bTriangle = deleteTriangleNoise(graph, dAngleTh);

        // スパイクノイズの除去
        bSpike = deleteSpikeNoise(graph);

        // 閉路除去
        BoostDirectedGraph directGraph = createDirectedGraph(graph);
        bCycle = deleteCycleNoiseUsingSubGraph(directGraph);

        // 有向グラフを無向グラフに変換
        graph = createUndirectedGraph(directGraph);   // 差し替え

        // グラフの簡略化
        simplifyGraph(graph);
    }
}

/*!
 * @brief 三角形ノイズの除去
 * @param[in] graph     無向グラフ
 * @param[in] dAngleTh  角度閾値(deg)
 * @return  削除処理の実行結果
 * @retval  true    削除処理を実施
 * @retval  false   削除処理を未実施
 */
bool CRoadCenterLine::deleteTriangleNoise(
    BoostUndirectedGraph &graph,
    const double dAngleTh)
{
    std::vector<BoostEdgeDesc> vecDelete;

    BOOST_FOREACH(BoostVertexDesc desc, boost::vertices(graph))
    {
        auto outEdgesRange = boost::out_edges(desc, graph);
        if (boost::degree(desc, graph) == 2)
        {
            std::vector<bool> vecCheck;
            BOOST_FOREACH(BoostEdgeDesc edgeDesc, boost::out_edges(desc, graph))
            {
                if (boost::degree(graph[edgeDesc].vertexDesc1, graph) > 2
                    || boost::degree(graph[edgeDesc].vertexDesc2, graph) > 2)
                {
                    vecCheck.push_back(true);
                }
                else
                {
                    vecCheck.push_back(false);
                }
            }

            auto itTarget = std::find(vecCheck.begin(), vecCheck.end(), false);
            if (itTarget == vecCheck.end())
            {
                // 注目頂点の次数が2かつ
                // 注目頂点から延びるエッジの反対側の頂点の次数が3以上
                //  |
                //  *＼
                //  |   *(削除対象頂点)
                //  *／
                //  |
                // 2辺の角度確認
                BoostEdgeProperty edge1 = graph[*outEdgesRange.first];
                BoostEdgeProperty edge2 = graph[*(outEdgesRange.first + 1)];
                CVector2D vec1 = (edge1.vertexDesc1 == desc)
                    ? CVector2D(graph[edge1.vertexDesc2].pt.x() - graph[edge1.vertexDesc1].pt.x(),
                        graph[edge1.vertexDesc2].pt.y() - graph[edge1.vertexDesc1].pt.y())
                    : CVector2D(graph[edge1.vertexDesc1].pt.x() - graph[edge1.vertexDesc2].pt.x(),
                        graph[edge1.vertexDesc1].pt.y() - graph[edge1.vertexDesc2].pt.y());
                CVector2D vec2 = (edge2.vertexDesc1 == desc)
                    ? CVector2D(graph[edge2.vertexDesc2].pt.x() - graph[edge2.vertexDesc1].pt.x(),
                        graph[edge2.vertexDesc2].pt.y() - graph[edge2.vertexDesc1].pt.y())
                    : CVector2D(graph[edge2.vertexDesc1].pt.x() - graph[edge2.vertexDesc2].pt.x(),
                        graph[edge2.vertexDesc1].pt.y() - graph[edge2.vertexDesc2].pt.y());
                double dAngle = CGeoUtil::Angle(vec1, vec2);

                if (dAngleTh > dAngle)
                {
                    // 削除対象に追加
                    BOOST_FOREACH(BoostEdgeDesc edgeDesc, outEdgesRange)
                    {
                        if (std::find(vecDelete.begin(), vecDelete.end(), edgeDesc) == vecDelete.end())
                            vecDelete.push_back(edgeDesc);
                    }
                }
            }
        }
    }

    // 削除
    std::sort(vecDelete.begin(), vecDelete.end(), std::greater<BoostEdgeDesc>());
    for (auto it = vecDelete.begin(); it != vecDelete.end(); it++)
    {
        try
        {
            boost::remove_edge(graph[*it].vertexDesc1, graph[*it].vertexDesc2, graph);
        }
        catch (...)
        {
            // DO NOTHING
        }
    }

    return vecDelete.size() > 0;
}


/*!
 * @brief スパイクノイズの除去
 * @param[in] graph 無向グラフ
 * @param[in] dAngleDiffTh  削除対象のエッジを決定する角度差分閾値(deg)
 * @param[in] dLengthTh     削除対象のエッジを決定する長さ閾値(m)
 * @return  削除処理の実行結果
 * @retval  true    削除処理を実施
 * @retval  false   削除処理を未実施
 */
bool CRoadCenterLine::deleteSpikeNoise(
    BoostUndirectedGraph &graph,
    const double dAngleDiffTh,
    const double dLengthTh)
{
    std::vector<BoostEdgeDesc> vecDelete;
    BOOST_FOREACH(BoostVertexDesc desc, boost::vertices(graph))
    {
        if (boost::degree(desc, graph) == 1)
        {
            auto outEdgesRange = boost::out_edges(desc, graph);
            auto itEdge = outEdgesRange.first;

            // 注目頂点から延びるエッジの反対側の頂点
            BoostVertexDesc otherSideVertexDesc = graph[*itEdge].vertexDesc1;
            if (otherSideVertexDesc == desc)
                otherSideVertexDesc = graph[*itEdge].vertexDesc2;

            if (boost::degree(otherSideVertexDesc, graph) > 2)
            {
                // 注目頂点の次数が1かつ
                // 注目頂点から延びるエッジの反対側の頂点の次数が3以上
                //  |
                //  * ― *(削除対象)
                //  |

                // 分岐点から次数1となる頂点へのエッジが複数延びている場合は削除対象を選別する
                //  * (削除対象外)
                //  |
                //  * ― *(削除対象)
                //  |

                // 分岐点から延びているエッジを取得
                outEdgesRange = boost::out_edges(otherSideVertexDesc, graph);
                bool bDelete = true;
                BOOST_FOREACH(BoostEdgeDesc edgeDesc, outEdgesRange)
                {
                    if (edgeDesc == *itEdge)
                        continue;   // 削除候補のエッジはスキップ

                    // 分岐点から延びているエッジと削除候補のエッジのなす角を確認する
                    BoostVertexDesc vertexDesc = graph[edgeDesc].vertexDesc1;
                    if (vertexDesc == otherSideVertexDesc)
                        vertexDesc = graph[edgeDesc].vertexDesc2;

                    CVector2D pt1(graph[desc].pt.x(), graph[desc].pt.y());
                    CVector2D pt2(graph[otherSideVertexDesc].pt.x(), graph[otherSideVertexDesc].pt.y());
                    CVector2D pt3(graph[vertexDesc].pt.x(), graph[vertexDesc].pt.y());
                    CVector2D vec1 = pt1 - pt2;
                    CVector2D vec2 = pt3 - pt2;
                    double dAngle = CGeoUtil::Angle(vec1, vec2);
                    double dDiff = abs(180.0 - dAngle);
                    double dLength = vec1.Length();
                    if (dDiff < dAngleDiffTh || dLength > dLengthTh)
                    {
                        // 削除対象エッジに対して水平に延びるエッジがある場合、
                        // または、削除対象エッジの長さが長い場合は削除対象外
                        bDelete = false;
                        break;
                    }
                }
                if (bDelete && std::find(vecDelete.begin(), vecDelete.end(), *itEdge) == vecDelete.end())
                {
                    // 削除対象に追加
                    vecDelete.push_back(*itEdge);
                }
            }
        }
    }

    // 削除
    std::sort(vecDelete.begin(), vecDelete.end(), std::greater<BoostEdgeDesc>());
    for (auto it = vecDelete.begin(); it != vecDelete.end(); it++)
    {
        try
        {
            boost::remove_edge(graph[*it].vertexDesc1, graph[*it].vertexDesc2, graph);
        }
        catch (...)
        {
            // DO NOTHING
        }
    }

    return vecDelete.size() > 0;
}

/*!
 * @brief 無向グラフの簡略化
 * @param[in/out] graph 無向グラフ
 * @param[in] dAngleDiffTh 角度差分閾値(deg)
*/
void CRoadCenterLine::simplifyGraph(
    BoostUndirectedGraph &graph,
    const double dAngleDiffTh)
{
    bool bLoop = true;

    while (bLoop)
    {
        bLoop = false;

        // 次数2の頂点から延びるエッジが水平の場合は不要な頂点のため削除
        BOOST_FOREACH(BoostVertexDesc vertexDesc, boost::vertices(graph))
        {
            if (boost::degree(vertexDesc, graph) != 2)
                continue;

            // 頂点から延びるエッジを取得
            auto outEdgesRange = boost::out_edges(vertexDesc, graph);
            std::vector<CVector2D> vec;
            std::vector<BoostVertexDesc> vertices;
            BOOST_FOREACH(BoostEdgeDesc edgeDesc, outEdgesRange)
            {
                BoostVertexDesc dstVertexDesc = graph[edgeDesc].vertexDesc1;
                if (dstVertexDesc == vertexDesc)
                    dstVertexDesc = graph[edgeDesc].vertexDesc2;

                CVector2D srcPt(graph[vertexDesc].pt.x(), graph[vertexDesc].pt.y());
                CVector2D dstPt(graph[dstVertexDesc].pt.x(), graph[dstVertexDesc].pt.y());
                CVector2D v = dstPt - srcPt;
                v.Normalize();
                vec.push_back(v);
                vertices.push_back(dstVertexDesc);
            }

            // 2本のエッジがなす角度
            double dAngle = CGeoUtil::Angle(vec[0], vec[1]);
            double dDiff = abs(180.0 - dAngle);
            if (dDiff <= dAngleDiffTh)
            {
                // 次数2の頂点から延びるエッジが水平の場合
                // 次数2の頂点から延びるエッジを削除して短絡エッジを挿入する
                bLoop = true;
                BOOST_FOREACH(BoostEdgeDesc edgeDesc, outEdgesRange)
                {
                    boost::remove_edge(graph[edgeDesc].vertexDesc1, graph[edgeDesc].vertexDesc2, graph);
                }
                auto newEdgeDesc = boost::add_edge(vertices[0], vertices[1], graph);
                if (newEdgeDesc.second)
                {
                    // エッジのプロパティに頂点情報を紐づける
                    graph[newEdgeDesc.first].vertexDesc1 = vertices[0];
                    graph[newEdgeDesc.first].vertexDesc2 = vertices[1];
                }
            }
        }
    }
}

/*!
 * @brief 閉路探索用の部分グラフの作成
 * @param[in] graph             有効グラフ
 * @param[in] vertexDesc        注目頂点ディスクリプタ
 * @param[in] dBuffer           注目エリア作成用バッファサイズ(m)
 * @param[out] subGraph         部分グラフ
 * @param[out] mapGlobalToSub   全体グラフから部分グラフへの頂点ディスクリプタ対応マップ
 * @param[out] mapSubToGlobal   部分グラフから全体グラフへの頂点ディスクリプタ対応マップ
 * @return  作成結果
 * @retval  true        成功
 * @retval  false       失敗
 * @note    全体グラフから注目エリアと重畳するエッジを抜き出し部分グラフとする
 *          なお、閉路探索用のため端点と繋がるエッジは削除した部分グラフを作成する
*/
bool CRoadCenterLine::createSubGraph(
    BoostDirectedGraph &graph,
    BoostDVertexDesc vertexDesc,
    const double dBuffer,
    BoostDirectedGraph &subGraph,
    std::map<BoostDVertexDesc, BoostDVertexDesc> &mapGlobalToSub,
    std::map<BoostDVertexDesc, BoostDVertexDesc> &mapSubToGlobal)
{
    assert(CEpsUtil::Greater(dBuffer, 0));
    subGraph.clear();
    mapGlobalToSub.clear();
    mapSubToGlobal.clear();

    // 注目エリア
    BoostMultiPolygon areas = CAnalyzeRoadEdgeGeomUtil::Buffering(graph[vertexDesc].pt, dBuffer);

    // 部分グラフの作成
    std::queue<BoostDVertexDesc> searchVertices;    // 探索予定の頂点
    std::set<BoostDVertexDesc> searchedVertices;    // 探索済みの頂点
    searchVertices.push(vertexDesc);
    while (!searchVertices.empty())
    {
        // 探索頂点の取得
        BoostDVertexDesc targetDesc = searchVertices.front();
        searchVertices.pop();                   // queueから除去
        searchedVertices.insert(targetDesc);    // 探索済み頂点に追加

        // 注目頂点が部分グラフに存在するか確認
        BoostDVertexDesc srcLocalVertexDesc;
        auto itSrc = mapGlobalToSub.find(targetDesc);
        if (itSrc == mapGlobalToSub.end())
        {
            // 部分グラフに存在しない
            srcLocalVertexDesc = boost::add_vertex(graph[targetDesc], subGraph);
            mapGlobalToSub[targetDesc] = srcLocalVertexDesc;
            mapSubToGlobal[srcLocalVertexDesc] = targetDesc;
        }
        else
        {
            srcLocalVertexDesc = itSrc->second;
        }

        // 注目頂点から延びるエッジの確認
        BOOST_FOREACH(BoostDEdgeDesc edgeDesc, boost::out_edges(targetDesc, graph))
        {
            BoostPolyline line;
            line.push_back(graph[graph[edgeDesc].vertexDesc1].pt);
            line.push_back(graph[graph[edgeDesc].vertexDesc2].pt);
            if (bg::disjoint(line, areas))
                continue;   // エッジが注目エリアと衝突しない場合はスキップ

            // エッジが注目エリアと衝突する場合
            // target側の頂点が部分グラフに存在するか確認し、存在しない場合は頂点を追加する
            BoostDVertexDesc targetLocalVertexDesc;
            BoostDVertexDesc nextSearchVartexDesc = boost::target(edgeDesc, graph);
            auto itTarget = mapGlobalToSub.find(nextSearchVartexDesc);
            if (itTarget == mapGlobalToSub.end())
            {
                targetLocalVertexDesc = boost::add_vertex(graph[nextSearchVartexDesc], subGraph);
                mapGlobalToSub[nextSearchVartexDesc] = targetLocalVertexDesc;
                mapSubToGlobal[targetLocalVertexDesc] = nextSearchVartexDesc;
            }
            else
            {
                targetLocalVertexDesc = itTarget->second;
            }

            // 注目エッジを部分グラフに追加
            auto newEdge = boost::add_edge(srcLocalVertexDesc, targetLocalVertexDesc, subGraph);
            if (newEdge.second)
            {
                subGraph[newEdge.first].vertexDesc1 = srcLocalVertexDesc;
                subGraph[newEdge.first].vertexDesc2 = targetLocalVertexDesc;
            }

            // target側の頂点が既に探索対象として登録されているか確認する
            auto itSearch = std::find(
                searchVertices._Get_container().begin(), searchVertices._Get_container().end(),
                mapSubToGlobal[targetLocalVertexDesc]);
            // target側の頂点が既に探索済みか確認する
            auto itSearched = std::find(
                searchedVertices.begin(), searchedVertices.end(), mapSubToGlobal[targetLocalVertexDesc]);
            if (itSearch == searchVertices._Get_container().end() && itSearched == searchedVertices.end())
            {
                // 未探索の場合は探索対象の頂点とする
                searchVertices.push(mapSubToGlobal[targetLocalVertexDesc]);
            }
        }
    }

    // 端点と繋がるエッジの削除
    bool bLoop;
    do
    {
        bLoop = false;
        std::vector<BoostDEdgeDesc> vecDeleteEdge;      // 削除対象エッジ
        BOOST_FOREACH(BoostDVertexDesc currentVertexDesc, boost::vertices(subGraph))
        {
            if (boost::out_degree(currentVertexDesc, subGraph) == 1)
            {
                auto outEdgesRange = boost::out_edges(currentVertexDesc, subGraph);
                BOOST_FOREACH(BoostDEdgeDesc edgeDesc, outEdgesRange)
                {
                    if (std::find(vecDeleteEdge.begin(), vecDeleteEdge.end(), edgeDesc)
                        == vecDeleteEdge.end())
                        vecDeleteEdge.push_back(edgeDesc);

                    // 反対方向エッジの確認
                    BoostDVertexDesc srcVertexDesc = boost::source(edgeDesc, subGraph);
                    BoostDVertexDesc targetVertexDesc = boost::target(edgeDesc, subGraph);
                    auto reverseEdge = boost::edge(targetVertexDesc, srcVertexDesc, subGraph);
                    if (reverseEdge.second
                        && std::find(vecDeleteEdge.begin(), vecDeleteEdge.end(), reverseEdge.first) == vecDeleteEdge.end())
                        vecDeleteEdge.push_back(reverseEdge.first);
                }
            }
        }

        // エッジの削除
        std::sort(vecDeleteEdge.begin(), vecDeleteEdge.end(), std::greater<BoostDEdgeDesc>());
        for (auto it = vecDeleteEdge.begin(); it != vecDeleteEdge.end(); it++)
        {
            try
            {
                boost::remove_edge(subGraph[*it].vertexDesc1, subGraph[*it].vertexDesc2, subGraph);
            }
            catch (...)
            {
                // DO NOTHING
            }
        }

        if (vecDeleteEdge.size() > 0)
            bLoop = true;

    } while (boost::num_edges(subGraph) > 0 && bLoop);
    return boost::num_edges(subGraph) > 0;
}

/*!
 * @brief 閉路ノイズの除去
 * @param[in] graph     有向グラフ
 * @param[in] dAreaTh   除外する閉路(街区)の面積閾値(m^2)
 * @return  削除処理の実行結果
 * @retval  true    削除処理を実施
 * @retval  false   削除処理を未実施
 */
bool CRoadCenterLine::deleteCycleNoise(
    BoostDirectedGraph &graph,
    const double dAreaTh)
{
    assert(CEpsUtil::Greater(dAreaTh, 0));

    std::vector<BoostDEdgeDesc> vecDelete;  // 削除対象エッジ

    // 経路記憶用
    std::vector<BoostDVertexDesc> pred(boost::num_vertices(graph), BoostDirectedGraph::null_vertex());
    // 深さ優先探索
    boost::depth_first_search(
        graph,
        visitor(boost::make_dfs_visitor(boost::record_predecessors(pred.data(), boost::on_back_edge{}))));

    // 閉路確認
    BOOST_FOREACH(BoostDVertexDesc vertexDesc, boost::vertices(graph))
    {
        std::vector<BoostDVertexDesc> route;
        BoostDVertexDesc tmpDesc = vertexDesc;
        do
        {
            // ルート作成
            route.push_back(tmpDesc);
            tmpDesc = pred.at(tmpDesc);
        } while (tmpDesc != BoostDirectedGraph::null_vertex());

        bool bCycle = false;
        if (route.size() > 2)
        {
            // 3点以上の経路の場合
            // 終点から延びるエッジに閉路の始点があるか確認する
            auto outEdgesRange = boost::out_edges(route.back(), graph);
            BOOST_FOREACH(BoostDEdgeDesc edgeDesc, outEdgesRange)
            {
                BoostDVertexDesc target = boost::target(edgeDesc, graph);
                if (target == route.front())
                {
                    // 終点から延びるエッジが閉路の始点の場合
                    route.push_back(vertexDesc);

                    BoostPolygon poly;
                    BOOST_FOREACH(BoostDVertexDesc desc, route)
                    {
                        poly.outer().push_back(graph[desc].pt);
                    }
                    bg::correct(poly);

                    if (bg::area(poly) >= dAreaTh)  // todo 面積どうする? 穴ポリゴン確認にする?
                    {
                        continue;   // 街区による閉路の場合は面積が大きくなるため削除対象外とする
                    }

                    m_cycles.push_back(poly); //debug 発見した閉路
                    bCycle = true;  // 閉路フラグ
                    break;
                }
            }
        }

        if (bCycle)
        {
            // 閉路を発見した場合削除するエッジを決定する
            // 分岐点にはさまれたエッジごとに経路を分割する
            // 開始点(分岐点)の探索
            std::vector<BoostDVertexDesc>::iterator itStart = route.begin();
            for (; itStart != route.end(); itStart++)
                if (boost::out_degree(*itStart, graph) > 2)
                    break;

            std::vector<std::vector<BoostDVertexDesc>> segments;    // 分割経路(頂点desc)
            BoostMultiLines segmentLines;   // 分割経路(polyline)
            std::vector<BoostDVertexDesc>::iterator itEnd = itStart;
            do
            {
                if (boost::out_degree(*itStart, graph) > 2)
                {
                    // 分岐点の場合
                    if (segments.size() > 0)
                    {
                        // 既にセグメントが存在する場合は、セグメントの終点として追加
                        segments.back().push_back(*itStart);
                        segmentLines.back().push_back(graph[*itStart].pt);
                    }

                    // 新しいセグメントを作成
                    std::vector<BoostDVertexDesc> segment;
                    segments.push_back(segment);
                    BoostPolyline line;
                    segmentLines.push_back(line);
                }

                // セグメントに頂点を追加
                segments.back().push_back(*itStart);
                segmentLines.back().push_back(graph[*itStart].pt);

                if (itStart < route.end() - 2)  // routeの終点は始点と同一のためskipする
                {
                    itStart++;
                }
                else
                {
                    itStart = route.begin();
                }
            } while (itStart != itEnd);
            // 最終セグメントに終点を追加
            segments.back().push_back(*itEnd);
            segmentLines.back().push_back(graph[*itEnd].pt);

            // 最長セグメントを削除対象とする
            double dLength = bg::length(segmentLines.front());
            std::vector<std::vector<BoostDVertexDesc>>::iterator itTarget = segments.begin();
            std::vector<std::vector<BoostDVertexDesc>>::iterator itSegment = itTarget;
            for (auto it = segmentLines.begin(); it != segmentLines.end(); it++, itSegment++)
            {
                if (dLength < bg::length(*it))
                {
                    dLength = bg::length(*it);
                    itTarget = itSegment;
                }
            }

            for (std::vector<BoostDVertexDesc>::iterator it = itTarget->begin();
                it < itTarget->end() - 1; it++)
            {
                std:: pair<BoostDEdgeDesc, bool> edge = boost::edge(*it, *(it + 1), graph);
                if (edge.second && std::find(vecDelete.begin(), vecDelete.end(), edge.first) == vecDelete.end())
                    vecDelete.push_back(edge.first);    // エッジが存在する場合は削除対象に登録

                edge = boost::edge(*(it + 1), *it, graph);
                if (edge.second && std::find(vecDelete.begin(), vecDelete.end(), edge.first) == vecDelete.end())
                    vecDelete.push_back(edge.first);    // エッジが存在する場合は削除対象に登録
            }
        }
    }

    // 削除
    std::sort(vecDelete.begin(), vecDelete.end(), std::greater<BoostDEdgeDesc>());
    for (auto it = vecDelete.begin(); it != vecDelete.end(); it++)
    {
        try
        {
            boost::remove_edge(graph[*it].vertexDesc1, graph[*it].vertexDesc2, graph);
        }
        catch (...)
        {
            // DO NOTHING
        }
    }

    return vecDelete.size() > 0;
}

/*!
 * @brief 閉路ノイズの除去
 * @param[in] graph     有向グラフ
 * @param[in] dBuffer   注目エリア作成用バッファサイズ(m)
 * @param[in] dAreaTh   除外する閉路(街区)の面積閾値(m^2)
 * @return  削除処理の実行結果
 * @retval  true    削除処理を実施
 * @retval  false   削除処理を未実施
 */
bool CRoadCenterLine::deleteCycleNoiseUsingSubGraph(
    BoostDirectedGraph &graph,
    const double dBuffer,
    const double dAreaTh)
{
    assert(CEpsUtil::Greater(dBuffer, 0));
    assert(CEpsUtil::Greater(dAreaTh, 0));

    BoostMultiPolygon detectCircles;        // 重複確認用発見済み閉路

    std::vector<BoostDEdgeDesc> vecDelete;  // 削除対象
    BOOST_FOREACH(BoostDVertexDesc vertexDesc, boost::vertices(graph))
    {
        // 分岐点付近の部分グラフを作成
        if (boost::out_degree(vertexDesc, graph) > 2)
        {
            // 全体/部分グラフの頂点対応マップ
            std::map<BoostDVertexDesc, BoostDVertexDesc> mapGlobalToSub, mapSubToGlobal;
            BoostDirectedGraph subGraph;    // 部分グラフ

            // 部分グラフの作成
            if (!createSubGraph(graph, vertexDesc, dBuffer, subGraph, mapGlobalToSub, mapSubToGlobal))
                continue;   // 部分グラフなしの場合はskip

            // 深さ優先探索
            std::vector<BoostDVertexDesc> pred(boost::num_vertices(subGraph), BoostDirectedGraph::null_vertex());
            boost::depth_first_search(
                subGraph,
                visitor(boost::make_dfs_visitor(boost::record_predecessors(pred.data(), boost::on_back_edge{}))));

            // 閉路確認
            BOOST_FOREACH(BoostDVertexDesc subVertexDesc, boost::vertices(subGraph))
            {
                std::vector<BoostDVertexDesc> route;
                BoostDVertexDesc tmpDesc = subVertexDesc;
                do
                {
                    // ルート作成
                    route.push_back(mapSubToGlobal[tmpDesc]);
                    tmpDesc = pred.at(tmpDesc);
                } while (tmpDesc != BoostDirectedGraph::null_vertex());

                bool bCycle = false;
                if (route.size() > 2)
                {
                    // 3点以上の経路の場合
                    // 終点から延びるエッジに閉路の始点があるか確認する
                    auto outEdgesRange = boost::out_edges(route.back(), graph);
                    BOOST_FOREACH(BoostDEdgeDesc edgeDesc, outEdgesRange)
                    {
                        BoostDVertexDesc target = boost::target(edgeDesc, graph);
                        if (target == route.front())
                        {
                            // 終点から延びるエッジが閉路の始点の場合
                            route.push_back(mapSubToGlobal[subVertexDesc]);

                            BoostPolygon poly;
                            BOOST_FOREACH(BoostDVertexDesc desc, route)
                            {
                                poly.outer().push_back(graph[desc].pt);
                            }
                            bg::correct(poly);

                            if (bg::area(poly) >= dAreaTh)  // todo 面積どうする? 穴ポリゴン確認にする?
                            {
                                continue;   // 街区による閉路の場合は面積が大きくなるため削除対象外とする
                            }

                            // 発見済みの閉路と重複するか確認する
                            if (!detectCircles.empty())
                            {
                                BoostMultiPolygon andPolygons;
                                // 重畳領域が存在し個数が1である
                                bg::intersection(poly, detectCircles, andPolygons);
                                if (andPolygons.size() == 1)
                                {
                                    // 面積比率の算出
                                    double dRatio = bg::area(andPolygons[0]) / bg::area(poly);
                                    if (dRatio > 0.95)
                                        break;   // 重複するためskip
                                }
                            }

                            m_cycles.push_back(poly); //debug 発見した閉路
                            detectCircles.push_back(poly);  // 重複確認用発見済み閉路の登録
                            bCycle = true;  // 閉路フラグ
                            break;
                        }
                    }
                }

                if (bCycle)
                {
                    // 閉路を発見した場合削除するエッジを決定する
                    // 分岐点にはさまれたエッジごとに経路を分割する
                    // 開始点(分岐点)の探索
                    std::vector<BoostDVertexDesc>::iterator itStart = route.begin();
                    for (; itStart != route.end(); itStart++)
                        if (boost::out_degree(*itStart, graph) > 2)
                            break;

                    std::vector<std::vector<BoostDVertexDesc>> segments;    // 分割経路(頂点desc)
                    BoostMultiLines segmentLines;   // 分割経路(polyline)
                    std::vector<BoostDVertexDesc>::iterator itEnd = itStart;
                    do
                    {
                        if (boost::out_degree(*itStart, graph) > 2)
                        {
                            // 分岐点の場合
                            if (segments.size() > 0)
                            {
                                // 既にセグメントが存在する場合は、セグメントの終点として追加
                                segments.back().push_back(*itStart);
                                segmentLines.back().push_back(graph[*itStart].pt);
                            }

                            // 新しいセグメントを作成
                            std::vector<BoostDVertexDesc> segment;
                            segments.push_back(segment);
                            BoostPolyline line;
                            segmentLines.push_back(line);
                        }

                        // セグメントに頂点を追加
                        segments.back().push_back(*itStart);
                        segmentLines.back().push_back(graph[*itStart].pt);

                        if (itStart < route.end() - 2)  // routeの終点は始点と同一のためskipする
                        {
                            itStart++;
                        }
                        else
                        {
                            itStart = route.begin();
                        }
                    } while (itStart != itEnd);
                    // 最終セグメントに終点を追加
                    segments.back().push_back(*itEnd);
                    segmentLines.back().push_back(graph[*itEnd].pt);

                    // 最長セグメントを削除対象とする
                    double dLength = bg::length(segmentLines.front());
                    std::vector<std::vector<BoostDVertexDesc>>::iterator itTarget = segments.begin();
                    std::vector<std::vector<BoostDVertexDesc>>::iterator itSegment = itTarget;
                    for (auto it = segmentLines.begin(); it != segmentLines.end(); it++, itSegment++)
                    {
                        if (dLength < bg::length(*it))
                        {
                            dLength = bg::length(*it);
                            itTarget = itSegment;
                        }
                    }

                    for (std::vector<BoostDVertexDesc>::iterator it = itTarget->begin();
                        it < itTarget->end() - 1; it++)
                    {
                        std::pair<BoostDEdgeDesc, bool> edge = boost::edge(*it, *(it + 1), graph);
                        if (edge.second && std::find(vecDelete.begin(), vecDelete.end(), edge.first) == vecDelete.end())
                            vecDelete.push_back(edge.first);    // エッジが存在する場合は削除対象に登録

                        edge = boost::edge(*(it + 1), *it, graph);
                        if (edge.second && std::find(vecDelete.begin(), vecDelete.end(), edge.first) == vecDelete.end())
                            vecDelete.push_back(edge.first);    // エッジが存在する場合は削除対象に登録
                    }
                }
            }
        }
    }

    // 削除
    std::sort(vecDelete.begin(), vecDelete.end(), std::greater<BoostDEdgeDesc>());
    for (auto it = vecDelete.begin(); it != vecDelete.end(); it++)
    {
        try
        {
            boost::remove_edge(graph[*it].vertexDesc1, graph[*it].vertexDesc2, graph);
        }
        catch (...)
        {
            // DO NOTHING
        }
    }

    return vecDelete.size() > 0;
}

/*!
 * @brief 有向グラフの作成
 * @param[in] undirectedGraph 無向グラフ
 * @return 有向グラフ
*/
BoostDirectedGraph CRoadCenterLine::createDirectedGraph(
    const BoostUndirectedGraph &undirectedGraph)
{
    // 有向グラフの作成
    BoostDirectedGraph graph;
    std::map<BoostVertexDesc, BoostDVertexDesc> vertexMaps;
    BOOST_FOREACH(BoostEdgeDesc desc, boost::edges(undirectedGraph))
    {
        BoostDVertexDesc vertexDesc1, vertexDesc2;

        // 既存点の確認
        auto it1 = vertexMaps.find(undirectedGraph[desc].vertexDesc1);
        auto it2 = vertexMaps.find(undirectedGraph[desc].vertexDesc2);
        if (it1 == vertexMaps.end())
        {
            // 頂点追加
            BoostVertexProperty val(undirectedGraph[undirectedGraph[desc].vertexDesc1].pt);
            vertexDesc1 = boost::add_vertex(val, graph);
            vertexMaps[undirectedGraph[desc].vertexDesc1] = vertexDesc1;
        }
        else
        {
            vertexDesc1 = it1->second;
        }
        if (it2 == vertexMaps.end())
        {
            // 頂点追加
            BoostVertexProperty val(undirectedGraph[undirectedGraph[desc].vertexDesc2].pt);
            vertexDesc2 = boost::add_vertex(val, graph);
            vertexMaps[undirectedGraph[desc].vertexDesc2] = vertexDesc2;
        }
        else
        {
            vertexDesc2 = it2->second;
        }

        // エッジの追加
        auto edgeDesc = boost::add_edge(vertexDesc1, vertexDesc2, graph);
        // エッジのプロパティに頂点情報を紐づける
        graph[edgeDesc.first].vertexDesc1 = vertexDesc1;
        graph[edgeDesc.first].vertexDesc2 = vertexDesc2;

        // 反対エッジの追加
        edgeDesc = boost::add_edge(vertexDesc2, vertexDesc1, graph);
        // エッジのプロパティに頂点情報を紐づける
        graph[edgeDesc.first].vertexDesc1 = vertexDesc2;
        graph[edgeDesc.first].vertexDesc2 = vertexDesc1;
    }

    return graph;
}

/*!
 * @brief 無向グラフの作成
 * @param[in] directedGraph 有向グラフ
 * @return 無向グラフ
*/
BoostUndirectedGraph CRoadCenterLine::createUndirectedGraph(
    const BoostDirectedGraph &directedGraph)
{
    // 無向グラフの作成
    BoostUndirectedGraph graph;
    std::map<BoostDVertexDesc, BoostVertexDesc> vertexMaps;
    BOOST_FOREACH(BoostDEdgeDesc desc, boost::edges(directedGraph))
    {
        BoostVertexDesc vertexDesc1, vertexDesc2;

        // 既存点の確認
        auto it1 = vertexMaps.find(directedGraph[desc].vertexDesc1);
        auto it2 = vertexMaps.find(directedGraph[desc].vertexDesc2);
        if (it1 == vertexMaps.end())
        {
            // 頂点追加
            BoostVertexProperty val(directedGraph[directedGraph[desc].vertexDesc1].pt);
            vertexDesc1 = boost::add_vertex(val, graph);
            vertexMaps[directedGraph[desc].vertexDesc1] = vertexDesc1;
        }
        else
        {
            vertexDesc1 = it1->second;
        }
        if (it2 == vertexMaps.end())
        {
            // 頂点追加
            BoostVertexProperty val(directedGraph[directedGraph[desc].vertexDesc2].pt);
            vertexDesc2 = boost::add_vertex(val, graph);
            vertexMaps[directedGraph[desc].vertexDesc2] = vertexDesc2;
        }
        else
        {
            vertexDesc2 = it2->second;
        }

        std::pair<BoostEdgeDesc, bool> edge = boost::edge(vertexDesc1, vertexDesc2, graph);
        if (!edge.second)
        {
            // エッジの追加
            auto edgeDesc = boost::add_edge(vertexDesc1, vertexDesc2, graph);
            // エッジのプロパティに頂点情報を紐づける
            graph[edgeDesc.first].vertexDesc1 = vertexDesc1;
            graph[edgeDesc.first].vertexDesc2 = vertexDesc2;
        }
    }

    return graph;
}

/*!
 * @brief 交差点マージ
 * @param[in/out]   graph   道路中心線ネット―ワークの無向グラフ
 * @param[in]       dDistTh マージ距離閾値m
*/
void CRoadCenterLine::mergeCrossing(
    BoostUndirectedGraph &graph,
    const double dDistTh)
{
    // dijkstra用に無向グラフにエッジの長さを設定する
    BOOST_FOREACH(BoostEdgeDesc edgeDesc, boost::edges(graph))
    {
        graph[edgeDesc].dLength = CBoostGraphUtil::EdgeLength(graph, edgeDesc);
    }

    // rtree作成
    BoostVertexRTree rtree;
    BOOST_FOREACH(BoostVertexDesc vertexDesc, boost::vertices(graph))
    {
        // 分岐点をrtreeに登録
        if (boost::degree(vertexDesc, graph) > 2)
            rtree.insert(std::pair<BoostPoint, BoostVertexDesc>(graph[vertexDesc].pt, vertexDesc));
    }

    // kNN
    std::map<BoostVertexDesc, BoostVertexDesc> mapMerged;
    BOOST_FOREACH(BoostVertexDesc vertexDesc, boost::vertices(graph))
    {
        if (boost::degree(vertexDesc, graph) < 3)
            continue;   // 分岐点以外はskip

        // 経路探索
        std::vector<BoostVertexDesc> pred(boost::num_vertices(graph), BoostDirectedGraph::null_vertex());
        std::vector<double> vecDistance(boost::num_vertices(graph));
        boost::dijkstra_shortest_paths(
            graph, vertexDesc,
            boost::predecessor_map(pred.data()).
            distance_map(vecDistance.data()).
            weight_map(boost::get(&BoostEdgeProperty::dLength, graph)));

        // 近傍交差点探索
        std::vector<std::pair<BoostPoint, BoostVertexDesc>> vecValues;
        rtree.query(bg::index::nearest(graph[vertexDesc].pt, 2), std::back_inserter(vecValues));

        for (auto itVal = vecValues.begin(); itVal != vecValues.end(); itVal++)
        {
            if (itVal->second == vertexDesc)
                continue;   // 注目頂点はskip

            // 注目点から最近傍点までの距離
            CVector2D pt1(graph[vertexDesc].pt.x(), graph[vertexDesc].pt.y());
            CVector2D pt2(itVal->first.x(), itVal->first.y());
            CVector2D vec = pt1 - pt2;

            if (vec.Length() > dDistTh)
                continue; // マージ対象外のためskip

            // 注目点から近傍点までの経路を確認
            if (pred[itVal->second] == itVal->second)
                continue;  // 注目点から近傍点までの経路がない


            // 注目点から近傍点までの経路を削除
            BoostVertexDesc prevVertexDesc = BoostDirectedGraph::null_vertex();
            for (BoostVertexDesc tmpDesc = itVal->second;
                tmpDesc != vertexDesc; tmpDesc = pred[tmpDesc])
            {
                if (prevVertexDesc != BoostDirectedGraph::null_vertex())
                {
                    boost::remove_edge(prevVertexDesc, tmpDesc, graph);
                }
                prevVertexDesc = tmpDesc;
            }
            boost::remove_edge(prevVertexDesc, vertexDesc, graph);

            // マージ後の頂点座標
            CVector2D ptCenter = vec * 0.5 + pt2;
            graph[vertexDesc].pt = BoostPoint(ptCenter.x, ptCenter.y);

            // エッジの付け替え
            BOOST_FOREACH(BoostEdgeDesc edgeDesc, boost::out_edges(itVal->second, graph))
            {
                BoostVertexDesc targetDesc = boost::target(edgeDesc, graph);
                // エッジ追加
                auto edge = boost::add_edge(vertexDesc, targetDesc, graph);
                if (edge.second)
                {
                    // エッジのプロパティに頂点情報を紐づける
                    graph[edge.first].vertexDesc1 = vertexDesc;
                    graph[edge.first].vertexDesc2 = targetDesc;
                }
                // エッジ削除
                boost::remove_edge(itVal->second, targetDesc, graph);
            }

            // エッジの長さを更新
            BOOST_FOREACH(BoostEdgeDesc edgeDesc, boost::out_edges(vertexDesc, graph))
            {
                graph[edgeDesc].dLength = CBoostGraphUtil::EdgeLength(graph, edgeDesc);;
            }

            // マージ済み頂点の登録
            mapMerged[itVal->second] = vertexDesc;
        }
    }
}

/*!
 * @brief 交差点のノイズ点除去
 * @param[in] graph         道路中心線の無向グラフ
 * @param[in] roadPolygons  道路縁ポリゴン
 * @param[in] dBuffer       交差点確認用の範囲に対するバッファ距離(m)
 * @return 交差点群
*/
std::vector<CCrossingData> CRoadCenterLine::deleteNoiseCrossing(
    const BoostUndirectedGraph &graph,
    const BoostMultiPolygon &roadPolygons,
    const double dBuffer)
{
    std::vector<CCrossingData> crossings; // 交差点
    CVector2D vecX(1, 0);   // 道路縁の方向算出用のx軸方向のベクトル

    // 道路縁を取得
    BoostMultiLines roadEdges;
    for (auto itPoly = roadPolygons.begin();
        itPoly != roadPolygons.end(); itPoly++)
    {
        for (auto itPt = itPoly->outer().begin();
            itPt < itPoly->outer().end() - 1; itPt++)
        {
            BoostPolyline line;
            line.push_back(*itPt);
            line.push_back(*(itPt + 1));
            roadEdges.push_back(line);
        }

        for (auto itInter = itPoly->inners().begin();
            itInter != itPoly->inners().end(); itInter++)
        {
            for (auto itPt = itInter->begin();
                itPt < itInter->end() - 1; itPt++)
            {
                BoostPolyline line;
                line.push_back(*itPt);
                line.push_back(*(itPt + 1));
                roadEdges.push_back(line);
            }
        }
    }

    // 交差点の抽出
    BOOST_FOREACH(BoostVertexDesc desc, boost::vertices(graph))
    {
        int nBranch = static_cast<int>(boost::degree(desc, graph));
        if (nBranch > 2)
        {
            // 交差点候補
            // 候補点近傍の道路縁と道路中心線を取得
            // 近傍範囲を決定する
            double dDist = DBL_MAX;
            for (auto itLine = roadEdges.begin(); itLine != roadEdges.end(); itLine++)
            {
                double d = bg::distance(*itLine, graph[desc].pt);
                if (d < dDist)
                    dDist = d;
            }
            dDist += dBuffer;

            // 近傍エリア
            BoostMultiPolygon areas = CAnalyzeRoadEdgeGeomUtil::Buffering(graph[desc].pt, dDist);

            // 近傍エリア内の道路縁
            BoostMultiLines neighborRoadEdge;
            for (auto itLine = roadEdges.begin(); itLine != roadEdges.end(); itLine++)
            {
                // 道路縁が近傍エリアと衝突するか確認
                if (!bg::disjoint(*itLine, areas))
                    neighborRoadEdge.push_back(*itLine);
            }
            // 近傍エリア内の道路中心線
            BoostMultiLines neighborCenterLine;
            for (auto itLine = m_centerLines.begin(); itLine != m_centerLines.end(); itLine++)
            {
                // 道路中心線が近傍エリアと衝突するか確認
                if (!bg::disjoint(*itLine, areas))
                    neighborCenterLine.push_back(*itLine);
            }

            if (neighborRoadEdge.size() > 0 && neighborCenterLine.size() > 0)
            {
                // 近傍道路縁と道路中心線を取得した場合
                // 道路中心線と同一方向の道路縁が存在するか確認する
                // 道路中心線の方向を算出

                std::vector<std::pair<std::vector<CVector2D>, int>> vecDirection;
                for (auto itPolyline = neighborCenterLine.begin();
                    itPolyline != neighborCenterLine.end(); itPolyline++)
                {
                    for (auto itPt = itPolyline->begin();
                        itPt < itPolyline->end() - 1; itPt++)
                    {
                        CVector2D pt1(itPt->x(), itPt->y());
                        CVector2D pt2((itPt + 1)->x(), (itPt + 1)->y());
                        CVector2D vec = pt2 - pt1;
                        //if (vec.Length() < 1.0)
                        //    continue;   // 短いエッジは除外する

                        vec.Normalize();
                        // +0 - +180degの範囲に収める
                        if ((vec.x >= 0 && vec.y < 0) || (vec.x < 0 && vec.y < 0))
                        {
                            vec.Inverse();
                        }

                        auto itTarget = vecDirection.end();
                        double dDiffAngle = DBL_MAX;
                        for (auto it = vecDirection.begin(); it != vecDirection.end(); it++)
                        {
                            for (auto itVec = it->first.begin();
                                itVec != it->first.end(); itVec++)
                            {
                                double dAngle = CGeoUtil::Angle(vec, *itVec);
                                if (dAngle < dDiffAngle)
                                {
                                    dDiffAngle = dAngle;
                                    itTarget = it;
                                }
                            }
                        }

                        if (itTarget != vecDirection.end() && dDiffAngle <= 5.0)
                        {
                            // 類似ベクトルが存在する
                            itTarget->first.push_back(vec);
                        }
                        else
                        {
                            // 未登録 or 類似ベクトルとの角度が3degを超える場合
                            std::vector<CVector2D> vectors;
                            vectors.push_back(vec);
                            vecDirection.push_back(std::pair(vectors, 0));
                        }
                    }
                }

                // 道路縁の方向を確認
                // 交差点の場合は、2種類以上の方向の道路縁が存在する
                for (auto itPolyline = neighborRoadEdge.begin();
                    itPolyline != neighborRoadEdge.end(); itPolyline++)
                {
                    for (auto itPt = itPolyline->begin();
                        itPt < itPolyline->end() - 1; itPt++)
                    {
                        CVector2D pt1(itPt->x(), itPt->y());
                        CVector2D pt2((itPt + 1)->x(), (itPt + 1)->y());
                        CVector2D vec = pt2 - pt1;
                        if (vec.Length() < 1.0)
                            continue;   // 短いエッジは除外する

                        vec.Normalize();

                        // +0 - +180degの範囲の値に収める
                        if ((vec.x >= 0 && vec.y < 0) || (vec.x < 0 && vec.y < 0))
                        {
                            vec.Inverse();
                        }

                        auto itTarget = vecDirection.end();
                        double dDiffAngle = DBL_MAX;
                        for (auto it = vecDirection.begin(); it != vecDirection.end(); it++)
                        {
                            for (auto itVec = it->first.begin();
                                itVec != it->first.end(); itVec++)
                            {
                                double dAngle = CGeoUtil::Angle(vec, *itVec);
                                if (dAngle < dDiffAngle)
                                {
                                    dDiffAngle = dAngle;
                                    itTarget = it;
                                }
                            }
                        }
                        if (itTarget != vecDirection.end() && dDiffAngle <= 10.0)
                        {
                            // 類似ベクトルが存在する
                            itTarget->second += 1;
                        }
                    }
                }

                int nCount = 0;
                for (auto it = vecDirection.begin(); it != vecDirection.end(); it++)
                {
                    int nDeg = static_cast<int>(CGeoUtil::Angle(it->first[0], vecX));
                    // 道路縁は両端のエッジで1セットのため、同一方向のエッジが複数本(厳密には偶数本)存在する
                    // 分岐の場合は、複数方向のエッジが存在する
                    if (it->second > 1)
                        nCount++;
                }

                if (nCount > 1)
                {
                    CCrossingData data(graph[desc].pt, nBranch);
                    crossings.push_back(data);
                }
            }
        }
    }

    return crossings;
}