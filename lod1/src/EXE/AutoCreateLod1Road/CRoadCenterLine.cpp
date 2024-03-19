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
 * @brief �R���X�g���N�^
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
 * @brief �����_�ߖT�̓��H���S�����擾����
 * @param[in] crossPt   �����_���W
 * @param[in] area      �����_�G���A
 * @return ���H���S��
*/
BoostMultiLines CRoadCenterLine::CenterLines(
    BoostPoint &crossPt,
    BoostPolygon &area)
{
    BoostMultiLines routes;
    if (m_rtree.size() > 0 && boost::num_edges(m_graph) > 0)
    {
        // �����_�T��
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

        // �����_���牄�т�o�H���Ƃɏ���
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
                        break;  // �}�������_�������[�_�̏ꍇ�͏I��

                    // �X�V
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
 * @brief ���H���S���쐬
 * @param[in] roadPolygons  ���H�|���S��
 * @param[in] dReso         �𑜓x(m/px)
 * @param[in] dShrink       ���k�T�C�Y(m)
*/
void CRoadCenterLine::CreateCenterLine(
    const BoostMultiPolygon &roadPolygons,
    const double dReso,
    const double dShrink)
{
    assert(CEpsUtil::Greater(dReso, 0));
    assert(CEpsUtil::LessEqual(dShrink, 0));

    m_centerLines.clear();

    // �{���m�C�������ɂ�铹�H���S���̍쐬
    getVoronoiEdges(roadPolygons, dReso, dShrink, m_internalCenterLines);

    // �����O���t�̍쐬
    BoostUndirectedGraph graph = CBoostGraphUtil::CreateGraph(m_internalCenterLines);

    //���H���S���̃m�C�Y����
    deleteNoise(graph);

    // �����_�̃}�[�W
    mergeCrossing(graph);

    // ���H���S���̃G�b�W���o
    // deleteNoiseCrossing()��m_centerLines���Q�Ƃ��邽�ߋL�ڏ��ɒ���
    BOOST_FOREACH(BoostEdgeDesc desc, boost::edges(graph))
    {
        BoostPolyline line;
        BoostEdgeProperty edge = graph[desc];
        line.push_back(graph[edge.vertexDesc1].pt);
        line.push_back(graph[edge.vertexDesc2].pt);
        m_centerLines.push_back(line);
    }

    // �����_�̒��o
    //m_crossings = deleteNoiseCrossing(graph, roadPolygons, 5.0);
    // �����_�̒��o
    m_crossings.clear();
    BOOST_FOREACH(BoostVertexDesc desc, boost::vertices(graph))
    {
        int nBranch = static_cast<int>(boost::degree(desc, graph));
        if (nBranch > 2)
        {
            // �����_
            CCrossingData data(graph[desc].pt, nBranch);
            m_crossings.push_back(data);
        }
    }

    // ���H���S���̃O���t������I�ɕێ�
    m_graph = graph;
    // rtree�쐬
    m_rtree.clear();
    BOOST_FOREACH(BoostVertexDesc vertexDesc, boost::vertices(graph))
    {
        // ����_��rtree�ɓo�^
        if (boost::degree(vertexDesc, graph) > 2)
            m_rtree.insert(std::pair<BoostPoint, BoostVertexDesc>(graph[vertexDesc].pt, vertexDesc));
    }
}

/*!
 * @brief �����_�擾(���W���̂�)
 * @return  �����_���W�Q
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
 * @brief ���ڃG���A�̌����_�擾(���򐔕t��)
 * @param[in] area ���ڃG���A
 * @return �����_���Q
*/
std::vector<CCrossingData> CRoadCenterLine::GetCrossPointsWithBranchNum(const BoostBox &area)
{
    std::vector<CCrossingData> crossings;
    std::set<BoostVertexDesc> selectedVerties;
    if (boost::num_edges(m_graph) > 0)
    {
        // ���ڗ̈��(���E�܂�)�̌����_�擾
        BOOST_FOREACH(BoostVertexDesc vertexDesc, boost::vertices(m_graph))
        {
            int nBranch = static_cast<int>(boost::degree(vertexDesc, m_graph));
            if (nBranch > 2 && bg::covered_by(m_graph[vertexDesc].pt, area))
            {
                CCrossingData data(m_graph[vertexDesc].pt, nBranch);
                crossings.push_back(data);
                selectedVerties.insert(vertexDesc);   // �擾�ςݒ��_��ێ�
            }
        }

        // ���ڗ̈�̋��E�����f���Ă��铹�H�����݂���ꍇ�́A
        // ���H�̉�����ɑ��݂��钍�ڗ̈�O�̌����_���擾����
        // ���ڗ̈�̃|���S����
        BoostPolygon polygon;
        bg::convert(area, polygon);
        // ���ڗ̈�̋��E��
        BoostPolyline line;
        BOOST_FOREACH(BoostPoint pt, polygon.outer())
        {
            line.push_back(pt);
        }
        // ���E���ƏՓ˂��铹�H���S���̃G�b�W�T��
        BOOST_FOREACH(BoostEdgeDesc edgeDesc, boost::edges(m_graph))
        {
            BoostPolyline edge;
            edge.push_back(m_graph[m_graph[edgeDesc].vertexDesc1].pt);
            edge.push_back(m_graph[m_graph[edgeDesc].vertexDesc2].pt);
            if (!bg::disjoint(line, edge))
            {
                // ���E���ƏՓ˂���G�b�W�𔭌������ꍇ�́A�G�b�W�̗��[����o�H��H���Č����_��T������
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
                            break;  // �ʉߓ_�ɖ߂��ė����ꍇ

                        route.insert(targetVertexDesc); // ���ڒ��_�̓o�^
                        // ���̈ړ����_�̒T��
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
                        // �����_�����ڗ̈�O�����o�^�̏ꍇ
                        CCrossingData data(m_graph[targetVertexDesc].pt, nBranch);
                        crossings.push_back(data);
                        selectedVerties.insert(targetVertexDesc);   // �擾�ςݒ��_��ێ�
                    }
                }
            }
        }
    }

    return crossings;
}

/*!
 * @brief ���H���S���쐬����
 * @param[in]   roadPolygons    ���H�|���S���Q
 * @param[in]   dSampling       �T���v�����O�Ԋu
 * @param[out]  roadEdges       �⏕�_�ǉ���̓��H��
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
        // �P����
        BoostPolygon simplePolygon = CAnalyzeRoadEdgeGeomUtil::Simplify(*itPolygon);

        // �l�ߑւ�
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
        // �����G�b�W���珈������
        std::sort(edges.begin(), edges.end(), std::greater<CEdge>());
        // �⏕�_�ǉ�
        for (auto itEdge = edges.begin();
            itEdge != edges.end(); itEdge++)
        {
            if (itEdge->m_middlePoints.size() > 0)
                continue;

            // �Ӄx�N�g��
            CVector2D vec = itEdge->GetVector();
            double len = vec.Length();

            // �����x�N�g��
            CVector2D vecVertical;
            if (CAnalyzeRoadEdgeGeomUtil::GetVerticalVec(vec, vecVertical))
            {
                // �����x�N�g�������H�|���S�����Ɍ����Ă��邩�m�F����
                CVector2D pos = vec * 0.5 + itEdge->m_start;
                CVector2D tmpPos = vecVertical * 0.01 + pos;
                if (!bg::within(BoostPoint(tmpPos.x, tmpPos.y), simplePolygon))
                {
                    // �����x�N�g���𔽓]����
                    vecVertical.Inverse();
                }

                // �T���v�����O
                std::vector<CVector2D> vecSampling;
                CAnalyzeRoadEdgeGeomUtil::Sampling(
                    itEdge->m_start, itEdge->m_end, vecSampling, dSampling);
                if (vecSampling.size() > 2)
                {
                    std::copy(vecSampling.begin() + 1, vecSampling.end() - 1,
                        std::back_inserter(itEdge->m_middlePoints));    // �n�I�_�������ăR�s�[
                }

                // �����x�N�g���ƌ�������Ό��ʒu�ɂ���G�b�W�ɕ⏕�_��ǉ�����
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
                                    // �����l�ݒ� or ���ڃG�b�W�ɓ��H�̈���͂���ň�ԋ߂��G�b�W�ōX�V
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
                        // �⏕�_�̒ǉ����
                        CVector2D baseVec = itCrossEdge->GetVector();
                        double dBaseLength = baseVec.Length();
                        std::vector<CVector2D>::iterator itTargetPt = itCrossEdge->m_middlePoints.begin();
                        for (; itTargetPt != itCrossEdge->m_middlePoints.end(); itTargetPt++)
                        {
                            // �x�N�g���W���Z�o
                            double dTmpLength = (*itTargetPt - itCrossEdge->m_start).Length();
                            double dRate = dTmpLength / dBaseLength;
                            if (dRate > dS)
                                break;
                        }
                        if (itTargetPt == itCrossEdge->m_middlePoints.end())
                        {
                            // �Ō�ɒǉ�
                            itCrossEdge->m_middlePoints.push_back(crossPos);
                        }
                        else
                        {
                            // �r���ɒǉ�
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
 * @brief �{���m�C�������̎擾
 * @param[in]   lines           �|�����C���Q
 * @param[in]   dReso           �𑜓x(m/px)
 * @param[in]   dShrink         ���k�T�C�Y(m)
 * @param[out]  voronoiEdges    �{���m�C�������Q
 * @note    boost�̃{���m�C�����̓��͂�double���W������Ƌ��������������Ȃ�������,
            int���W�ɕϊ����邽�߉𑜓x��ݒ肷��
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

        // �P����
        BoostPolygon simplePolygon = CAnalyzeRoadEdgeGeomUtil::Simplify(*itPolygon);

        // �l�ߑւ�
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

        // int���W�ɂ��邽�߂�world���摜���W
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

        // �{���m�C����
        BoostVoronoiDiagram diagram;
        bp::construct_voronoi(segments.begin(), segments.end(), &diagram);

        // ���͓��H�|���S�������k
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

        // polygons�����̃{���m�C�G�b�W���擾
        std::vector<const bp::voronoi_edge<double> *> vecSearched;
        for (BoostVoronoiEdgeIt itEdge = diagram.edges().begin();
            itEdge != diagram.edges().end(); itEdge++)
        {
            if (itEdge->is_infinite())
                continue;

            // �L�����̏ꍇ
            // �G�b�W���T���ς݂̏ꍇ�̓X�L�b�v����
            std::vector<const bp::voronoi_edge<double> *>::iterator targetIt = std::find(
                vecSearched.begin(), vecSearched.end(), &(*itEdge));
            if (targetIt != vecSearched.end())
                continue;

            // �摜��world���W
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

            // ���g�Ƒ΂ƂȂ�G�b�W��T���ς݂ɓo�^
            vecSearched.push_back(&(*itEdge));
            vecSearched.push_back(itEdge->twin());
        }
    }
}

// �����O���t�𗘗p�������H���S���̃m�C�Y����
void CRoadCenterLine::deleteNoise(
    BoostUndirectedGraph &graph,
    const double dAngleTh)
{
    bool bTriangle = true;
    bool bSpike = true;
    bool bCycle = true;

    while (bTriangle || bSpike || bCycle)
    {
        // �O�p�`�m�C�Y�̏���
        bTriangle = deleteTriangleNoise(graph, dAngleTh);

        // �X�p�C�N�m�C�Y�̏���
        bSpike = deleteSpikeNoise(graph);

        // �H����
        BoostDirectedGraph directGraph = createDirectedGraph(graph);
        bCycle = deleteCycleNoiseUsingSubGraph(directGraph);

        // �L���O���t�𖳌��O���t�ɕϊ�
        graph = createUndirectedGraph(directGraph);   // �����ւ�

        // �O���t�̊ȗ���
        simplifyGraph(graph);
    }
}

/*!
 * @brief �O�p�`�m�C�Y�̏���
 * @param[in] graph     �����O���t
 * @param[in] dAngleTh  �p�x臒l(deg)
 * @return  �폜�����̎��s����
 * @retval  true    �폜���������{
 * @retval  false   �폜�����𖢎��{
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
                // ���ڒ��_�̎�����2����
                // ���ڒ��_���牄�т�G�b�W�̔��Α��̒��_�̎�����3�ȏ�
                //  |
                //  *�_
                //  |   *(�폜�Ώے��_)
                //  *�^
                //  |
                // 2�ӂ̊p�x�m�F
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
                    // �폜�Ώۂɒǉ�
                    BOOST_FOREACH(BoostEdgeDesc edgeDesc, outEdgesRange)
                    {
                        if (std::find(vecDelete.begin(), vecDelete.end(), edgeDesc) == vecDelete.end())
                            vecDelete.push_back(edgeDesc);
                    }
                }
            }
        }
    }

    // �폜
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
 * @brief �X�p�C�N�m�C�Y�̏���
 * @param[in] graph �����O���t
 * @param[in] dAngleDiffTh  �폜�Ώۂ̃G�b�W�����肷��p�x����臒l(deg)
 * @param[in] dLengthTh     �폜�Ώۂ̃G�b�W�����肷�钷��臒l(m)
 * @return  �폜�����̎��s����
 * @retval  true    �폜���������{
 * @retval  false   �폜�����𖢎��{
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

            // ���ڒ��_���牄�т�G�b�W�̔��Α��̒��_
            BoostVertexDesc otherSideVertexDesc = graph[*itEdge].vertexDesc1;
            if (otherSideVertexDesc == desc)
                otherSideVertexDesc = graph[*itEdge].vertexDesc2;

            if (boost::degree(otherSideVertexDesc, graph) > 2)
            {
                // ���ڒ��_�̎�����1����
                // ���ڒ��_���牄�т�G�b�W�̔��Α��̒��_�̎�����3�ȏ�
                //  |
                //  * �\ *(�폜�Ώ�)
                //  |

                // ����_���玟��1�ƂȂ钸�_�ւ̃G�b�W���������тĂ���ꍇ�͍폜�Ώۂ�I�ʂ���
                //  * (�폜�ΏۊO)
                //  |
                //  * �\ *(�폜�Ώ�)
                //  |

                // ����_���牄�тĂ���G�b�W���擾
                outEdgesRange = boost::out_edges(otherSideVertexDesc, graph);
                bool bDelete = true;
                BOOST_FOREACH(BoostEdgeDesc edgeDesc, outEdgesRange)
                {
                    if (edgeDesc == *itEdge)
                        continue;   // �폜���̃G�b�W�̓X�L�b�v

                    // ����_���牄�тĂ���G�b�W�ƍ폜���̃G�b�W�̂Ȃ��p���m�F����
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
                        // �폜�ΏۃG�b�W�ɑ΂��Đ����ɉ��т�G�b�W������ꍇ�A
                        // �܂��́A�폜�ΏۃG�b�W�̒����������ꍇ�͍폜�ΏۊO
                        bDelete = false;
                        break;
                    }
                }
                if (bDelete && std::find(vecDelete.begin(), vecDelete.end(), *itEdge) == vecDelete.end())
                {
                    // �폜�Ώۂɒǉ�
                    vecDelete.push_back(*itEdge);
                }
            }
        }
    }

    // �폜
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
 * @brief �����O���t�̊ȗ���
 * @param[in/out] graph �����O���t
 * @param[in] dAngleDiffTh �p�x����臒l(deg)
*/
void CRoadCenterLine::simplifyGraph(
    BoostUndirectedGraph &graph,
    const double dAngleDiffTh)
{
    bool bLoop = true;

    while (bLoop)
    {
        bLoop = false;

        // ����2�̒��_���牄�т�G�b�W�������̏ꍇ�͕s�v�Ȓ��_�̂��ߍ폜
        BOOST_FOREACH(BoostVertexDesc vertexDesc, boost::vertices(graph))
        {
            if (boost::degree(vertexDesc, graph) != 2)
                continue;

            // ���_���牄�т�G�b�W���擾
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

            // 2�{�̃G�b�W���Ȃ��p�x
            double dAngle = CGeoUtil::Angle(vec[0], vec[1]);
            double dDiff = abs(180.0 - dAngle);
            if (dDiff <= dAngleDiffTh)
            {
                // ����2�̒��_���牄�т�G�b�W�������̏ꍇ
                // ����2�̒��_���牄�т�G�b�W���폜���ĒZ���G�b�W��}������
                bLoop = true;
                BOOST_FOREACH(BoostEdgeDesc edgeDesc, outEdgesRange)
                {
                    boost::remove_edge(graph[edgeDesc].vertexDesc1, graph[edgeDesc].vertexDesc2, graph);
                }
                auto newEdgeDesc = boost::add_edge(vertices[0], vertices[1], graph);
                if (newEdgeDesc.second)
                {
                    // �G�b�W�̃v���p�e�B�ɒ��_����R�Â���
                    graph[newEdgeDesc.first].vertexDesc1 = vertices[0];
                    graph[newEdgeDesc.first].vertexDesc2 = vertices[1];
                }
            }
        }
    }
}

/*!
 * @brief �H�T���p�̕����O���t�̍쐬
 * @param[in] graph             �L���O���t
 * @param[in] vertexDesc        ���ڒ��_�f�B�X�N���v�^
 * @param[in] dBuffer           ���ڃG���A�쐬�p�o�b�t�@�T�C�Y(m)
 * @param[out] subGraph         �����O���t
 * @param[out] mapGlobalToSub   �S�̃O���t���畔���O���t�ւ̒��_�f�B�X�N���v�^�Ή��}�b�v
 * @param[out] mapSubToGlobal   �����O���t����S�̃O���t�ւ̒��_�f�B�X�N���v�^�Ή��}�b�v
 * @return  �쐬����
 * @retval  true        ����
 * @retval  false       ���s
 * @note    �S�̃O���t���璍�ڃG���A�Əd�􂷂�G�b�W�𔲂��o�������O���t�Ƃ���
 *          �Ȃ��A�H�T���p�̂��ߒ[�_�ƌq����G�b�W�͍폜���������O���t���쐬����
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

    // ���ڃG���A
    BoostMultiPolygon areas = CAnalyzeRoadEdgeGeomUtil::Buffering(graph[vertexDesc].pt, dBuffer);

    // �����O���t�̍쐬
    std::queue<BoostDVertexDesc> searchVertices;    // �T���\��̒��_
    std::set<BoostDVertexDesc> searchedVertices;    // �T���ς݂̒��_
    searchVertices.push(vertexDesc);
    while (!searchVertices.empty())
    {
        // �T�����_�̎擾
        BoostDVertexDesc targetDesc = searchVertices.front();
        searchVertices.pop();                   // queue���珜��
        searchedVertices.insert(targetDesc);    // �T���ςݒ��_�ɒǉ�

        // ���ڒ��_�������O���t�ɑ��݂��邩�m�F
        BoostDVertexDesc srcLocalVertexDesc;
        auto itSrc = mapGlobalToSub.find(targetDesc);
        if (itSrc == mapGlobalToSub.end())
        {
            // �����O���t�ɑ��݂��Ȃ�
            srcLocalVertexDesc = boost::add_vertex(graph[targetDesc], subGraph);
            mapGlobalToSub[targetDesc] = srcLocalVertexDesc;
            mapSubToGlobal[srcLocalVertexDesc] = targetDesc;
        }
        else
        {
            srcLocalVertexDesc = itSrc->second;
        }

        // ���ڒ��_���牄�т�G�b�W�̊m�F
        BOOST_FOREACH(BoostDEdgeDesc edgeDesc, boost::out_edges(targetDesc, graph))
        {
            BoostPolyline line;
            line.push_back(graph[graph[edgeDesc].vertexDesc1].pt);
            line.push_back(graph[graph[edgeDesc].vertexDesc2].pt);
            if (bg::disjoint(line, areas))
                continue;   // �G�b�W�����ڃG���A�ƏՓ˂��Ȃ��ꍇ�̓X�L�b�v

            // �G�b�W�����ڃG���A�ƏՓ˂���ꍇ
            // target���̒��_�������O���t�ɑ��݂��邩�m�F���A���݂��Ȃ��ꍇ�͒��_��ǉ�����
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

            // ���ڃG�b�W�𕔕��O���t�ɒǉ�
            auto newEdge = boost::add_edge(srcLocalVertexDesc, targetLocalVertexDesc, subGraph);
            if (newEdge.second)
            {
                subGraph[newEdge.first].vertexDesc1 = srcLocalVertexDesc;
                subGraph[newEdge.first].vertexDesc2 = targetLocalVertexDesc;
            }

            // target���̒��_�����ɒT���ΏۂƂ��ēo�^����Ă��邩�m�F����
            auto itSearch = std::find(
                searchVertices._Get_container().begin(), searchVertices._Get_container().end(),
                mapSubToGlobal[targetLocalVertexDesc]);
            // target���̒��_�����ɒT���ς݂��m�F����
            auto itSearched = std::find(
                searchedVertices.begin(), searchedVertices.end(), mapSubToGlobal[targetLocalVertexDesc]);
            if (itSearch == searchVertices._Get_container().end() && itSearched == searchedVertices.end())
            {
                // ���T���̏ꍇ�͒T���Ώۂ̒��_�Ƃ���
                searchVertices.push(mapSubToGlobal[targetLocalVertexDesc]);
            }
        }
    }

    // �[�_�ƌq����G�b�W�̍폜
    bool bLoop;
    do
    {
        bLoop = false;
        std::vector<BoostDEdgeDesc> vecDeleteEdge;      // �폜�ΏۃG�b�W
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

                    // ���Ε����G�b�W�̊m�F
                    BoostDVertexDesc srcVertexDesc = boost::source(edgeDesc, subGraph);
                    BoostDVertexDesc targetVertexDesc = boost::target(edgeDesc, subGraph);
                    auto reverseEdge = boost::edge(targetVertexDesc, srcVertexDesc, subGraph);
                    if (reverseEdge.second
                        && std::find(vecDeleteEdge.begin(), vecDeleteEdge.end(), reverseEdge.first) == vecDeleteEdge.end())
                        vecDeleteEdge.push_back(reverseEdge.first);
                }
            }
        }

        // �G�b�W�̍폜
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
 * @brief �H�m�C�Y�̏���
 * @param[in] graph     �L���O���t
 * @param[in] dAreaTh   ���O����H(�X��)�̖ʐ�臒l(m^2)
 * @return  �폜�����̎��s����
 * @retval  true    �폜���������{
 * @retval  false   �폜�����𖢎��{
 */
bool CRoadCenterLine::deleteCycleNoise(
    BoostDirectedGraph &graph,
    const double dAreaTh)
{
    assert(CEpsUtil::Greater(dAreaTh, 0));

    std::vector<BoostDEdgeDesc> vecDelete;  // �폜�ΏۃG�b�W

    // �o�H�L���p
    std::vector<BoostDVertexDesc> pred(boost::num_vertices(graph), BoostDirectedGraph::null_vertex());
    // �[���D��T��
    boost::depth_first_search(
        graph,
        visitor(boost::make_dfs_visitor(boost::record_predecessors(pred.data(), boost::on_back_edge{}))));

    // �H�m�F
    BOOST_FOREACH(BoostDVertexDesc vertexDesc, boost::vertices(graph))
    {
        std::vector<BoostDVertexDesc> route;
        BoostDVertexDesc tmpDesc = vertexDesc;
        do
        {
            // ���[�g�쐬
            route.push_back(tmpDesc);
            tmpDesc = pred.at(tmpDesc);
        } while (tmpDesc != BoostDirectedGraph::null_vertex());

        bool bCycle = false;
        if (route.size() > 2)
        {
            // 3�_�ȏ�̌o�H�̏ꍇ
            // �I�_���牄�т�G�b�W�ɕH�̎n�_�����邩�m�F����
            auto outEdgesRange = boost::out_edges(route.back(), graph);
            BOOST_FOREACH(BoostDEdgeDesc edgeDesc, outEdgesRange)
            {
                BoostDVertexDesc target = boost::target(edgeDesc, graph);
                if (target == route.front())
                {
                    // �I�_���牄�т�G�b�W���H�̎n�_�̏ꍇ
                    route.push_back(vertexDesc);

                    BoostPolygon poly;
                    BOOST_FOREACH(BoostDVertexDesc desc, route)
                    {
                        poly.outer().push_back(graph[desc].pt);
                    }
                    bg::correct(poly);

                    if (bg::area(poly) >= dAreaTh)  // todo �ʐςǂ�����? ���|���S���m�F�ɂ���?
                    {
                        continue;   // �X��ɂ��H�̏ꍇ�͖ʐς��傫���Ȃ邽�ߍ폜�ΏۊO�Ƃ���
                    }

                    m_cycles.push_back(poly); //debug ���������H
                    bCycle = true;  // �H�t���O
                    break;
                }
            }
        }

        if (bCycle)
        {
            // �H�𔭌������ꍇ�폜����G�b�W�����肷��
            // ����_�ɂ͂��܂ꂽ�G�b�W���ƂɌo�H�𕪊�����
            // �J�n�_(����_)�̒T��
            std::vector<BoostDVertexDesc>::iterator itStart = route.begin();
            for (; itStart != route.end(); itStart++)
                if (boost::out_degree(*itStart, graph) > 2)
                    break;

            std::vector<std::vector<BoostDVertexDesc>> segments;    // �����o�H(���_desc)
            BoostMultiLines segmentLines;   // �����o�H(polyline)
            std::vector<BoostDVertexDesc>::iterator itEnd = itStart;
            do
            {
                if (boost::out_degree(*itStart, graph) > 2)
                {
                    // ����_�̏ꍇ
                    if (segments.size() > 0)
                    {
                        // ���ɃZ�O�����g�����݂���ꍇ�́A�Z�O�����g�̏I�_�Ƃ��Ēǉ�
                        segments.back().push_back(*itStart);
                        segmentLines.back().push_back(graph[*itStart].pt);
                    }

                    // �V�����Z�O�����g���쐬
                    std::vector<BoostDVertexDesc> segment;
                    segments.push_back(segment);
                    BoostPolyline line;
                    segmentLines.push_back(line);
                }

                // �Z�O�����g�ɒ��_��ǉ�
                segments.back().push_back(*itStart);
                segmentLines.back().push_back(graph[*itStart].pt);

                if (itStart < route.end() - 2)  // route�̏I�_�͎n�_�Ɠ���̂���skip����
                {
                    itStart++;
                }
                else
                {
                    itStart = route.begin();
                }
            } while (itStart != itEnd);
            // �ŏI�Z�O�����g�ɏI�_��ǉ�
            segments.back().push_back(*itEnd);
            segmentLines.back().push_back(graph[*itEnd].pt);

            // �Œ��Z�O�����g���폜�ΏۂƂ���
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
                    vecDelete.push_back(edge.first);    // �G�b�W�����݂���ꍇ�͍폜�Ώۂɓo�^

                edge = boost::edge(*(it + 1), *it, graph);
                if (edge.second && std::find(vecDelete.begin(), vecDelete.end(), edge.first) == vecDelete.end())
                    vecDelete.push_back(edge.first);    // �G�b�W�����݂���ꍇ�͍폜�Ώۂɓo�^
            }
        }
    }

    // �폜
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
 * @brief �H�m�C�Y�̏���
 * @param[in] graph     �L���O���t
 * @param[in] dBuffer   ���ڃG���A�쐬�p�o�b�t�@�T�C�Y(m)
 * @param[in] dAreaTh   ���O����H(�X��)�̖ʐ�臒l(m^2)
 * @return  �폜�����̎��s����
 * @retval  true    �폜���������{
 * @retval  false   �폜�����𖢎��{
 */
bool CRoadCenterLine::deleteCycleNoiseUsingSubGraph(
    BoostDirectedGraph &graph,
    const double dBuffer,
    const double dAreaTh)
{
    assert(CEpsUtil::Greater(dBuffer, 0));
    assert(CEpsUtil::Greater(dAreaTh, 0));

    BoostMultiPolygon detectCircles;        // �d���m�F�p�����ςݕH

    std::vector<BoostDEdgeDesc> vecDelete;  // �폜�Ώ�
    BOOST_FOREACH(BoostDVertexDesc vertexDesc, boost::vertices(graph))
    {
        // ����_�t�߂̕����O���t���쐬
        if (boost::out_degree(vertexDesc, graph) > 2)
        {
            // �S��/�����O���t�̒��_�Ή��}�b�v
            std::map<BoostDVertexDesc, BoostDVertexDesc> mapGlobalToSub, mapSubToGlobal;
            BoostDirectedGraph subGraph;    // �����O���t

            // �����O���t�̍쐬
            if (!createSubGraph(graph, vertexDesc, dBuffer, subGraph, mapGlobalToSub, mapSubToGlobal))
                continue;   // �����O���t�Ȃ��̏ꍇ��skip

            // �[���D��T��
            std::vector<BoostDVertexDesc> pred(boost::num_vertices(subGraph), BoostDirectedGraph::null_vertex());
            boost::depth_first_search(
                subGraph,
                visitor(boost::make_dfs_visitor(boost::record_predecessors(pred.data(), boost::on_back_edge{}))));

            // �H�m�F
            BOOST_FOREACH(BoostDVertexDesc subVertexDesc, boost::vertices(subGraph))
            {
                std::vector<BoostDVertexDesc> route;
                BoostDVertexDesc tmpDesc = subVertexDesc;
                do
                {
                    // ���[�g�쐬
                    route.push_back(mapSubToGlobal[tmpDesc]);
                    tmpDesc = pred.at(tmpDesc);
                } while (tmpDesc != BoostDirectedGraph::null_vertex());

                bool bCycle = false;
                if (route.size() > 2)
                {
                    // 3�_�ȏ�̌o�H�̏ꍇ
                    // �I�_���牄�т�G�b�W�ɕH�̎n�_�����邩�m�F����
                    auto outEdgesRange = boost::out_edges(route.back(), graph);
                    BOOST_FOREACH(BoostDEdgeDesc edgeDesc, outEdgesRange)
                    {
                        BoostDVertexDesc target = boost::target(edgeDesc, graph);
                        if (target == route.front())
                        {
                            // �I�_���牄�т�G�b�W���H�̎n�_�̏ꍇ
                            route.push_back(mapSubToGlobal[subVertexDesc]);

                            BoostPolygon poly;
                            BOOST_FOREACH(BoostDVertexDesc desc, route)
                            {
                                poly.outer().push_back(graph[desc].pt);
                            }
                            bg::correct(poly);

                            if (bg::area(poly) >= dAreaTh)  // todo �ʐςǂ�����? ���|���S���m�F�ɂ���?
                            {
                                continue;   // �X��ɂ��H�̏ꍇ�͖ʐς��傫���Ȃ邽�ߍ폜�ΏۊO�Ƃ���
                            }

                            // �����ς݂̕H�Əd�����邩�m�F����
                            if (!detectCircles.empty())
                            {
                                BoostMultiPolygon andPolygons;
                                // �d���̈悪���݂�����1�ł���
                                bg::intersection(poly, detectCircles, andPolygons);
                                if (andPolygons.size() == 1)
                                {
                                    // �ʐϔ䗦�̎Z�o
                                    double dRatio = bg::area(andPolygons[0]) / bg::area(poly);
                                    if (dRatio > 0.95)
                                        break;   // �d�����邽��skip
                                }
                            }

                            m_cycles.push_back(poly); //debug ���������H
                            detectCircles.push_back(poly);  // �d���m�F�p�����ςݕH�̓o�^
                            bCycle = true;  // �H�t���O
                            break;
                        }
                    }
                }

                if (bCycle)
                {
                    // �H�𔭌������ꍇ�폜����G�b�W�����肷��
                    // ����_�ɂ͂��܂ꂽ�G�b�W���ƂɌo�H�𕪊�����
                    // �J�n�_(����_)�̒T��
                    std::vector<BoostDVertexDesc>::iterator itStart = route.begin();
                    for (; itStart != route.end(); itStart++)
                        if (boost::out_degree(*itStart, graph) > 2)
                            break;

                    std::vector<std::vector<BoostDVertexDesc>> segments;    // �����o�H(���_desc)
                    BoostMultiLines segmentLines;   // �����o�H(polyline)
                    std::vector<BoostDVertexDesc>::iterator itEnd = itStart;
                    do
                    {
                        if (boost::out_degree(*itStart, graph) > 2)
                        {
                            // ����_�̏ꍇ
                            if (segments.size() > 0)
                            {
                                // ���ɃZ�O�����g�����݂���ꍇ�́A�Z�O�����g�̏I�_�Ƃ��Ēǉ�
                                segments.back().push_back(*itStart);
                                segmentLines.back().push_back(graph[*itStart].pt);
                            }

                            // �V�����Z�O�����g���쐬
                            std::vector<BoostDVertexDesc> segment;
                            segments.push_back(segment);
                            BoostPolyline line;
                            segmentLines.push_back(line);
                        }

                        // �Z�O�����g�ɒ��_��ǉ�
                        segments.back().push_back(*itStart);
                        segmentLines.back().push_back(graph[*itStart].pt);

                        if (itStart < route.end() - 2)  // route�̏I�_�͎n�_�Ɠ���̂���skip����
                        {
                            itStart++;
                        }
                        else
                        {
                            itStart = route.begin();
                        }
                    } while (itStart != itEnd);
                    // �ŏI�Z�O�����g�ɏI�_��ǉ�
                    segments.back().push_back(*itEnd);
                    segmentLines.back().push_back(graph[*itEnd].pt);

                    // �Œ��Z�O�����g���폜�ΏۂƂ���
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
                            vecDelete.push_back(edge.first);    // �G�b�W�����݂���ꍇ�͍폜�Ώۂɓo�^

                        edge = boost::edge(*(it + 1), *it, graph);
                        if (edge.second && std::find(vecDelete.begin(), vecDelete.end(), edge.first) == vecDelete.end())
                            vecDelete.push_back(edge.first);    // �G�b�W�����݂���ꍇ�͍폜�Ώۂɓo�^
                    }
                }
            }
        }
    }

    // �폜
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
 * @brief �L���O���t�̍쐬
 * @param[in] undirectedGraph �����O���t
 * @return �L���O���t
*/
BoostDirectedGraph CRoadCenterLine::createDirectedGraph(
    const BoostUndirectedGraph &undirectedGraph)
{
    // �L���O���t�̍쐬
    BoostDirectedGraph graph;
    std::map<BoostVertexDesc, BoostDVertexDesc> vertexMaps;
    BOOST_FOREACH(BoostEdgeDesc desc, boost::edges(undirectedGraph))
    {
        BoostDVertexDesc vertexDesc1, vertexDesc2;

        // �����_�̊m�F
        auto it1 = vertexMaps.find(undirectedGraph[desc].vertexDesc1);
        auto it2 = vertexMaps.find(undirectedGraph[desc].vertexDesc2);
        if (it1 == vertexMaps.end())
        {
            // ���_�ǉ�
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
            // ���_�ǉ�
            BoostVertexProperty val(undirectedGraph[undirectedGraph[desc].vertexDesc2].pt);
            vertexDesc2 = boost::add_vertex(val, graph);
            vertexMaps[undirectedGraph[desc].vertexDesc2] = vertexDesc2;
        }
        else
        {
            vertexDesc2 = it2->second;
        }

        // �G�b�W�̒ǉ�
        auto edgeDesc = boost::add_edge(vertexDesc1, vertexDesc2, graph);
        // �G�b�W�̃v���p�e�B�ɒ��_����R�Â���
        graph[edgeDesc.first].vertexDesc1 = vertexDesc1;
        graph[edgeDesc.first].vertexDesc2 = vertexDesc2;

        // ���΃G�b�W�̒ǉ�
        edgeDesc = boost::add_edge(vertexDesc2, vertexDesc1, graph);
        // �G�b�W�̃v���p�e�B�ɒ��_����R�Â���
        graph[edgeDesc.first].vertexDesc1 = vertexDesc2;
        graph[edgeDesc.first].vertexDesc2 = vertexDesc1;
    }

    return graph;
}

/*!
 * @brief �����O���t�̍쐬
 * @param[in] directedGraph �L���O���t
 * @return �����O���t
*/
BoostUndirectedGraph CRoadCenterLine::createUndirectedGraph(
    const BoostDirectedGraph &directedGraph)
{
    // �����O���t�̍쐬
    BoostUndirectedGraph graph;
    std::map<BoostDVertexDesc, BoostVertexDesc> vertexMaps;
    BOOST_FOREACH(BoostDEdgeDesc desc, boost::edges(directedGraph))
    {
        BoostVertexDesc vertexDesc1, vertexDesc2;

        // �����_�̊m�F
        auto it1 = vertexMaps.find(directedGraph[desc].vertexDesc1);
        auto it2 = vertexMaps.find(directedGraph[desc].vertexDesc2);
        if (it1 == vertexMaps.end())
        {
            // ���_�ǉ�
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
            // ���_�ǉ�
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
            // �G�b�W�̒ǉ�
            auto edgeDesc = boost::add_edge(vertexDesc1, vertexDesc2, graph);
            // �G�b�W�̃v���p�e�B�ɒ��_����R�Â���
            graph[edgeDesc.first].vertexDesc1 = vertexDesc1;
            graph[edgeDesc.first].vertexDesc2 = vertexDesc2;
        }
    }

    return graph;
}

/*!
 * @brief �����_�}�[�W
 * @param[in/out]   graph   ���H���S���l�b�g�\���[�N�̖����O���t
 * @param[in]       dDistTh �}�[�W����臒lm
*/
void CRoadCenterLine::mergeCrossing(
    BoostUndirectedGraph &graph,
    const double dDistTh)
{
    // dijkstra�p�ɖ����O���t�ɃG�b�W�̒�����ݒ肷��
    BOOST_FOREACH(BoostEdgeDesc edgeDesc, boost::edges(graph))
    {
        graph[edgeDesc].dLength = CBoostGraphUtil::EdgeLength(graph, edgeDesc);
    }

    // rtree�쐬
    BoostVertexRTree rtree;
    BOOST_FOREACH(BoostVertexDesc vertexDesc, boost::vertices(graph))
    {
        // ����_��rtree�ɓo�^
        if (boost::degree(vertexDesc, graph) > 2)
            rtree.insert(std::pair<BoostPoint, BoostVertexDesc>(graph[vertexDesc].pt, vertexDesc));
    }

    // kNN
    std::map<BoostVertexDesc, BoostVertexDesc> mapMerged;
    BOOST_FOREACH(BoostVertexDesc vertexDesc, boost::vertices(graph))
    {
        if (boost::degree(vertexDesc, graph) < 3)
            continue;   // ����_�ȊO��skip

        // �o�H�T��
        std::vector<BoostVertexDesc> pred(boost::num_vertices(graph), BoostDirectedGraph::null_vertex());
        std::vector<double> vecDistance(boost::num_vertices(graph));
        boost::dijkstra_shortest_paths(
            graph, vertexDesc,
            boost::predecessor_map(pred.data()).
            distance_map(vecDistance.data()).
            weight_map(boost::get(&BoostEdgeProperty::dLength, graph)));

        // �ߖT�����_�T��
        std::vector<std::pair<BoostPoint, BoostVertexDesc>> vecValues;
        rtree.query(bg::index::nearest(graph[vertexDesc].pt, 2), std::back_inserter(vecValues));

        for (auto itVal = vecValues.begin(); itVal != vecValues.end(); itVal++)
        {
            if (itVal->second == vertexDesc)
                continue;   // ���ڒ��_��skip

            // ���ړ_����ŋߖT�_�܂ł̋���
            CVector2D pt1(graph[vertexDesc].pt.x(), graph[vertexDesc].pt.y());
            CVector2D pt2(itVal->first.x(), itVal->first.y());
            CVector2D vec = pt1 - pt2;

            if (vec.Length() > dDistTh)
                continue; // �}�[�W�ΏۊO�̂���skip

            // ���ړ_����ߖT�_�܂ł̌o�H���m�F
            if (pred[itVal->second] == itVal->second)
                continue;  // ���ړ_����ߖT�_�܂ł̌o�H���Ȃ�


            // ���ړ_����ߖT�_�܂ł̌o�H���폜
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

            // �}�[�W��̒��_���W
            CVector2D ptCenter = vec * 0.5 + pt2;
            graph[vertexDesc].pt = BoostPoint(ptCenter.x, ptCenter.y);

            // �G�b�W�̕t���ւ�
            BOOST_FOREACH(BoostEdgeDesc edgeDesc, boost::out_edges(itVal->second, graph))
            {
                BoostVertexDesc targetDesc = boost::target(edgeDesc, graph);
                // �G�b�W�ǉ�
                auto edge = boost::add_edge(vertexDesc, targetDesc, graph);
                if (edge.second)
                {
                    // �G�b�W�̃v���p�e�B�ɒ��_����R�Â���
                    graph[edge.first].vertexDesc1 = vertexDesc;
                    graph[edge.first].vertexDesc2 = targetDesc;
                }
                // �G�b�W�폜
                boost::remove_edge(itVal->second, targetDesc, graph);
            }

            // �G�b�W�̒������X�V
            BOOST_FOREACH(BoostEdgeDesc edgeDesc, boost::out_edges(vertexDesc, graph))
            {
                graph[edgeDesc].dLength = CBoostGraphUtil::EdgeLength(graph, edgeDesc);;
            }

            // �}�[�W�ςݒ��_�̓o�^
            mapMerged[itVal->second] = vertexDesc;
        }
    }
}

/*!
 * @brief �����_�̃m�C�Y�_����
 * @param[in] graph         ���H���S���̖����O���t
 * @param[in] roadPolygons  ���H���|���S��
 * @param[in] dBuffer       �����_�m�F�p�͈̔͂ɑ΂���o�b�t�@����(m)
 * @return �����_�Q
*/
std::vector<CCrossingData> CRoadCenterLine::deleteNoiseCrossing(
    const BoostUndirectedGraph &graph,
    const BoostMultiPolygon &roadPolygons,
    const double dBuffer)
{
    std::vector<CCrossingData> crossings; // �����_
    CVector2D vecX(1, 0);   // ���H���̕����Z�o�p��x�������̃x�N�g��

    // ���H�����擾
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

    // �����_�̒��o
    BOOST_FOREACH(BoostVertexDesc desc, boost::vertices(graph))
    {
        int nBranch = static_cast<int>(boost::degree(desc, graph));
        if (nBranch > 2)
        {
            // �����_���
            // ���_�ߖT�̓��H���Ɠ��H���S�����擾
            // �ߖT�͈͂����肷��
            double dDist = DBL_MAX;
            for (auto itLine = roadEdges.begin(); itLine != roadEdges.end(); itLine++)
            {
                double d = bg::distance(*itLine, graph[desc].pt);
                if (d < dDist)
                    dDist = d;
            }
            dDist += dBuffer;

            // �ߖT�G���A
            BoostMultiPolygon areas = CAnalyzeRoadEdgeGeomUtil::Buffering(graph[desc].pt, dDist);

            // �ߖT�G���A���̓��H��
            BoostMultiLines neighborRoadEdge;
            for (auto itLine = roadEdges.begin(); itLine != roadEdges.end(); itLine++)
            {
                // ���H�����ߖT�G���A�ƏՓ˂��邩�m�F
                if (!bg::disjoint(*itLine, areas))
                    neighborRoadEdge.push_back(*itLine);
            }
            // �ߖT�G���A���̓��H���S��
            BoostMultiLines neighborCenterLine;
            for (auto itLine = m_centerLines.begin(); itLine != m_centerLines.end(); itLine++)
            {
                // ���H���S�����ߖT�G���A�ƏՓ˂��邩�m�F
                if (!bg::disjoint(*itLine, areas))
                    neighborCenterLine.push_back(*itLine);
            }

            if (neighborRoadEdge.size() > 0 && neighborCenterLine.size() > 0)
            {
                // �ߖT���H���Ɠ��H���S�����擾�����ꍇ
                // ���H���S���Ɠ�������̓��H�������݂��邩�m�F����
                // ���H���S���̕������Z�o

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
                        //    continue;   // �Z���G�b�W�͏��O����

                        vec.Normalize();
                        // +0 - +180deg�͈̔͂Ɏ��߂�
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
                            // �ގ��x�N�g�������݂���
                            itTarget->first.push_back(vec);
                        }
                        else
                        {
                            // ���o�^ or �ގ��x�N�g���Ƃ̊p�x��3deg�𒴂���ꍇ
                            std::vector<CVector2D> vectors;
                            vectors.push_back(vec);
                            vecDirection.push_back(std::pair(vectors, 0));
                        }
                    }
                }

                // ���H���̕������m�F
                // �����_�̏ꍇ�́A2��ވȏ�̕����̓��H�������݂���
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
                            continue;   // �Z���G�b�W�͏��O����

                        vec.Normalize();

                        // +0 - +180deg�͈̔͂̒l�Ɏ��߂�
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
                            // �ގ��x�N�g�������݂���
                            itTarget->second += 1;
                        }
                    }
                }

                int nCount = 0;
                for (auto it = vecDirection.begin(); it != vecDirection.end(); it++)
                {
                    int nDeg = static_cast<int>(CGeoUtil::Angle(it->first[0], vecX));
                    // ���H���͗��[�̃G�b�W��1�Z�b�g�̂��߁A��������̃G�b�W�������{(�����ɂ͋����{)���݂���
                    // ����̏ꍇ�́A���������̃G�b�W�����݂���
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