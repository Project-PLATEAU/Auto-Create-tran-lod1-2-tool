#include "pch.h"
#include "CRoadDivision.h"
#include "CAnalyzeRoadEdgeGeomUtil.h"
#include "CBoostGraphUtil.h"
#include "boost/graph/dijkstra_shortest_paths.hpp"

/*!
 * @brief �ԓ��������|���S���̕���
 * @param[in] roadPolygons          ���H�|���S���Q
 * @param[in] blockPolyogns         �X��|���S���Q
 * @param[in] crossing              �����_���
 * @param[out] crossingPolygons     �ԓ��������|���S��
 * @param[out] remainingPolygons    �c���H�|���S��
 * @param[in] dReso                 ���W�ϊ��̉𑜓x(m)
*/
void CRoadDivision::DivisionByCrossing(
    BoostMultiPolygon &roadPolygons,
    BoostMultiPolygon &blockPolyogns,
    std::vector<CCrossingData> &crossing,
    std::vector<CRoadData> &crossingPolygons,
    BoostMultiPolygon &remainingPolygons,
    const double dReso)
{
    // �����_�f�[�^�Ƀ{���m�C�̈��ݒ肷��
    setVoronoiCellArea(roadPolygons, crossing, dReso);

    // �����_�̈�̐ݒ�
    setCrossingArea(roadPolygons, crossing);

    // �ԓ��������ؒf
    createCrossingPolygons(
        roadPolygons, blockPolyogns, crossing,
        crossingPolygons, remainingPolygons);
}

/*!
 * @brief ���H�\���ω��ɂ��|���S���̕���(���H���p)
 * @param[in]   roadPolygons        ���H�|���S���Q
 * @param[in]   facilities          ���H��
 * @param[in]   roadSectionType     ���H�`��^�C�v
 * @param[out]  facilityPolygons    ���H��
 * @param[out]  remainingPolygons   �c�����H�|���S��
 * @param[in]   dOverhangAreaTh     ���H���|���S���Ɠ��H�Ɠ��H���̏d���̈�̍����ʐ�臒l(�X��͂ݏo���m�F�p�j
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

    // �ߖT�T���p�̃f�[�^�쐬
    BoostMultiLinesItRTree rtree;
    for (auto itLine = facilities.begin(); itLine != facilities.end(); itLine++)
    {
        rtree.insert(std::pair<BoostPoint, BoostMultiLines::iterator>(itLine->front(), itLine));
        rtree.insert(std::pair<BoostPoint, BoostMultiLines::iterator>(itLine->back(), itLine));
    }
    // �ߖT�T��
    std::set<BoostMultiLines::iterator> searchedLines;
    for (auto itLine = facilities.begin(); itLine != facilities.end(); itLine++)
    {
        if (searchedLines.find(itLine) != searchedLines.end())
            continue;   // �T���ς݂̏ꍇ��skip

        // KNN
        std::vector<std::pair<BoostPoint, BoostMultiLines::iterator>> vecValues;
        rtree.query(bg::index::nearest(itLine->front(), 4), std::back_inserter(vecValues));
        auto itTargetLine = facilities.end();   // �Ό���
        double dLength = DBL_MAX;
        BoostPolygon targetPolygon;             // �����|���S��
        for (std::vector<std::pair<BoostPoint, BoostMultiLines::iterator>>::iterator itVal = vecValues.begin();
            itVal != vecValues.end(); itVal++)
        {
            if (itVal->second == itLine
                || searchedLines.find(itVal->second) != searchedLines.end())
                continue;   // �Ό����T���ΏۂƓ��� or �T���ς݂̏ꍇ��skip

            // �Ό����m�F
            // �Ό������q�����|���S���̍쐬
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
                // �s���|���S���̊m�F
                if (!bg::is_valid(*itCandidate))
                    continue;

                // ���H�|���S���Ƃ̏d��m�F(�X��ɂ͂ݏo�Ă��Ȃ����m�F����)
                BoostMultiPolygon andPolygons;
                bg::intersection(roadPolygons, *itCandidate, andPolygons);
                if (!bg::is_empty(andPolygons))
                {
                    double dDiffArea = abs(bg::area(*itCandidate) - bg::area(andPolygons));
                    if (andPolygons.size() > 1 || CEpsUtil::GreaterEqual(dDiffArea, dOverhangAreaTh))
                    {
                        // ���H���|���S�����X��ɂ͂ݏo���A1�u���b�N�ׂ̓��H�ɂ��d�􂵂Ă���ꍇ�͓��H�Ƃ̏d���̈悪�����ɂȂ�
                        // ���H���|���S�����X��ɂ͂ݏo���Ă���ꍇ�́A���H�Ƃ̏d���̈�Ɠ��H���|���S���̖ʐςɍ�����������
                        continue;
                    }
                }

                // ���ړ_�ƋߖT�_�Ԃ̋������ŒZ�ƂȂ�|���S�����擾����
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
                // �o�^
                CRoadData roadData;
                roadData.Polygon(targetPolygon);
                roadData.Type(roadSectionType);
                roadData.Division(0);
                tmpFacilityPolygons.push_back(roadData);

                // �T���ς݂̓o�^
                searchedLines.insert(itLine);
                searchedLines.insert(itTargetLine);
            }
        }
    }

    // ���H�|���S�����瓹�H�\���ω������̃|���S��������
    std::vector<CRoadData> errBridgePolygons; // �������ɕs���|���S�����������铹�H���|���S�����Q
    divisionPolygon(roadPolygons, tmpFacilityPolygons, facilityPolygons, errBridgePolygons, remainingPolygons);
}

/*!
 * @brief ���H�\���ω��ɂ��|���S���̕���(�g���l���p�j
 * @param[in]   roadPolygons        ���H�|���S���Q
 * @param[in]   facilities          �g���l��
 * @param[in]   roadSectionType     ���H�`��^�C�v
 * @param[out]  facilityPolygons    �g���l��
 * @param[out]  remainingPolygons   �c�����H�|���S��
 * @param[in]   dOverhangAreaTh     ���H���|���S���Ɠ��H�Ɠ��H����AND�̈�̍����ʐ�臒l(�X��͂ݏo���m�F�p�j
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
        BoostPolygon targetPolygon;     // �g���l���|���S��

        // �Ό������q�����|���S���̍쐬
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
            // �s���|���S���̊m�F
            if (!bg::is_valid(*itCandidate))
                continue;

            // ���H�|���S���Ƃ̏d��m�F(�X��ɂ͂ݏo�Ă��Ȃ����m�F����)
            BoostMultiPolygon andPolygons;
            bg::intersection(roadPolygons, *itCandidate, andPolygons);
            if (!bg::is_empty(andPolygons))
            {
                double dDiffArea = abs(bg::area(*itCandidate) - bg::area(andPolygons));
                if (andPolygons.size() > 1 || CEpsUtil::GreaterEqual(dDiffArea, dOverhangAreaTh))
                {
                    // ���H���|���S�����X��ɂ͂ݏo���A1�u���b�N�ׂ̓��H�ɂ��d�􂵂Ă���ꍇ�͓��H�Ƃ̏d���̈悪�����ɂȂ�
                    // ���H���|���S�����X��ɂ͂ݏo���Ă���ꍇ�́A���H�Ƃ̏d���̈�Ɠ��H���|���S���̖ʐςɍ�����������
                    continue;
                }

                targetPolygon = *itCandidate;
            }
        }

        if (!bg::is_empty(targetPolygon) && bg::is_valid(targetPolygon))
        {
            // �o�^
            CRoadData roadData;
            roadData.Polygon(targetPolygon);
            roadData.Type(roadSectionType);
            roadData.Division(0);
            tmpFacilityPolygons.push_back(roadData);
        }
    }

    // ���H�|���S�����瓹�H�\���ω������̃|���S��������
    std::vector<CRoadData> errTunnelPolygons; // �������ɕs���|���S������������g���l���|���S�����Q
    divisionPolygon(roadPolygons, tmpFacilityPolygons, facilityPolygons, errTunnelPolygons, remainingPolygons);
}

/*!
 * @brief �����_�f�[�^�Ƀ{���m�C�Z�������Z�b�g����
 * @param[in]       roadPolyogns    ���H�|���S���Q
 * @param[in/out]   crossing        �����_���
 * @param[in]       dReso           ���W�ϊ��̉𑜓x(m)
*/
void CRoadDivision::setVoronoiCellArea(
    BoostMultiPolygon &roadPolyogns,
    std::vector<CCrossingData> &crossing,
    const double dReso)
{
    assert(CEpsUtil::Greater(dReso, 0));

    // �����_�Ń{���m�C����
    // int���W�ɂ��邽�߂�world���摜���W
    BoostBox box;
    bg::envelope(roadPolyogns, box);    // ���͓��H�|���S���͈�
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

    // �{���m�C����
    BoostVoronoiDiagram diagram;
    bp::construct_voronoi(pts.begin(), pts.end(), &diagram);

    // �摜���W�n�̃f�[�^�͈�
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

    // cell���Ƃɍ��
    for (BoostVoronoiCellIt it = diagram.cells().begin();
        it != diagram.cells().end(); it++)
    {
        // cell���
        const BoostVoronoiDiagram::cell_type &cell = *it;
        if (cell.incident_edge() == NULL)
            continue;

        BoostPolygon cellPolygon;
        getVoronoiCell(cell, pts, imgBox, cellPolygon);

        if (bg::is_valid(cellPolygon))
        {
            // �摜���W->world���W�ɕϊ�
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

            // cell�ɑΉ���������_
            auto itCross = crossing.begin() + cell.source_index();
            itCross->Cell(worldCellPolygon);
        }
    }
}

/*!
 * @brief �{���m�C�Z���|���S���̎擾
 * @param[in] cell          �Z�����
 * @param[in] pts           �{���m�C�������̓��͒��_�Q
 * @param[in] box           ���͔͈�
 * @param[out] cellPolygon  �Z���|���S��
*/
void CRoadDivision::getVoronoiCell(
    const BoostVoronoiDiagram::cell_type &cell,
    std::vector<BoostVoronoiPoint> &pts,
    BoostBox &box,
    BoostPolygon &cellPolygon)
{
    // ������edge������ꍇ�́A������edge����T�����J�n����
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
                // edge���L���̏ꍇ
                cellPolygon.outer().push_back(BoostPoint(edge->vertex0()->x(), edge->vertex0()->y()));
                cellPolygon.outer().push_back(BoostPoint(edge->vertex1()->x(), edge->vertex1()->y()));
            }
            else
            {
                // edge�������̏ꍇ

                // �ΏۃZ���̒��S
                BoostVoronoiPoint p1 = pts[edge->cell()->source_index()];

                // �Ώ�edge�Ƒ΂ɂȂ�edge���`������Z���̒��S
                BoostVoronoiPoint p2 = pts[edge->twin()->cell()->source_index()];

                BoostVoronoiPoint origin(
                    static_cast<int>((p1.x() + p2.x()) * 0.5),
                    static_cast<int>((p1.y() + p2.y()) * 0.5)); //��_
                BoostVoronoiPoint direction(
                    static_cast<int>(p1.y() - p2.y()),
                    static_cast<int>(p2.x() - p1.x()));  //�x�N�g���̌���

                // ���_�����񂾃x�N�g���̌�����90deg��]�����āAinfinite edge�̌��������߂�
                //vertex0,1�̗L���ɂ���ĉ�]�p�x�̐�����ύX����
                //BoostPoint startPoint, endPoint;
                double side = box.max_corner().x() - box.min_corner().x();
                double koef = side / (std::max)(fabs(direction.x()), fabs(direction.y()));

                // ������edge��L����edge�ɂ���
                if (edge->vertex0() == nullptr && edge->vertex1() != nullptr)
                {
                    BoostPoint pt(edge->vertex1()->x(), edge->vertex1()->y());
                    // ��i�ŃZ���̈����͔͈͂ŃN���b�s���O����֌W��
                    // ���G�b�W�̎n�_�����̓f�[�^�͈͊O�̏ꍇ�͖������G�b�W��L�����ɂ��Ȃ��Ă��ǂ�
                    if (bg::within(pt, box))
                    {
                        cellPolygon.outer().push_back(
                            BoostPoint(origin.x() - direction.x() * koef,
                                origin.y() - direction.y() * koef));
                    }
                }

                if (edge->vertex1() == nullptr && edge->vertex0() != nullptr)
                {
                    // ��i�ŃZ���̈����͔͈͂ŃN���b�s���O����֌W��
                    // �O�G�b�W�̏I�_�����̓f�[�^�͈͊O�̏ꍇ�͖������G�b�W��L�����ɂ��Ȃ��Ă��ǂ�
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
        edge = edge->next();    //�����v���
    } while (edge != startEdge);

    bg::unique(cellPolygon);   //�d���_�̍폜
    bg::correct(cellPolygon);

    if (bg::is_valid(cellPolygon))
    {
        // ���͔͈͂Əd�􂷂�͈͂ɃZ���̗̈�����߂�
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
 * @brief �����_�̈�̐ݒ�
 * @param[in]       roadPolygons    ���H�|���S���Q
 * @param[in/out]   crossing        �����_���
*/
void CRoadDivision::setCrossingArea(
    BoostMultiPolygon &roadPolygons,
    std::vector<CCrossingData> &crossing)
{
    // ���H�����擾
    BoostMultiLines roadEdges;
    for (auto itPoly = roadPolygons.begin();
        itPoly != roadPolygons.end(); itPoly++)
    {
        CAnalyzeRoadEdgeGeomUtil::GetEdges(*itPoly, roadEdges);
    }

    // �����_�̈�̎Z�o
    for (auto itCross = crossing.begin(); itCross != crossing.end(); itCross++)
    {
        // �����_�̍ŋߖT�̓��H���܂ł̋���(�T�Z���H���̔���)���擾
        double dDist = DBL_MAX;
        for (auto itLine = roadEdges.begin(); itLine != roadEdges.end(); itLine++)
        {
            double d = bg::distance(*itLine, itCross->Point());
            if (d < dDist)
                dDist = d;
        }
        // �T�Z�œ��H����2�{
        dDist *= 4;

        // �ߖT�G���A
        BoostMultiPolygon circle = CAnalyzeRoadEdgeGeomUtil::Buffering(itCross->Point(), dDist);

        // �ߖT�G���A�ƃ{���m�C�̈�̏d���̈���擾����
        BoostMultiPolygon areas;
        bg::intersection(circle, itCross->Cell(), areas);

        if (areas.size() > 0)
            itCross->Area(areas[0]);
    }
}

/*!
 * @brief �����_�ߖT���H���̎擾
 * @param[in] roadPolyogns  ���H�|���S���f�[�^
 * @param[in] area          �����_�̈�
 * @param[in] crossPt       �����_���W
 * @return  ���H��(�����P��)�f�[�^�Q
*/
BoostMultiLines CRoadDivision::GetRoadEdges(
    BoostMultiPolygon &roadPolyogns,
    BoostPolygon &area,
    BoostPoint &crossPt)
{
    // �����_�̈�Əd�􂷂铹�H�����擾����
    BoostMultiLines lines;
    for (auto itPolygon = roadPolyogns.begin(); itPolygon != roadPolyogns.end(); itPolygon++)
    {
        CAnalyzeRoadEdgeGeomUtil::GetEdges(*itPolygon, area, lines);
    }

    // �擾�������H�����i�荞��
    if (lines.size() > 0)
    {
        for (auto itPolyline = lines.end() - 1; itPolyline >= lines.begin(); itPolyline--)
        {
            // �����_�Ǝ擾�������H���̒��_���q�������������H���S�̂ƌ�������񐔂��m�F����
            // �������������ꍇ�́A���ړ��H�������ڌ����_�̓��H���ł͂Ȃ��Ƃ݂Ȃ�
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
 * @brief ���H��(����)���������Čo�H�f�[�^���쐬����
 * @param[in]   roadEdges ���H��(����)�Q
 * @return      �|�����C���f�[�^�`��̓��H���f�[�^
 * @remarks     ���������ۂɕH�ƂȂ铹�H���͂Ȃ��z��
*/
BoostMultiLines CRoadDivision::GetRoutes(
    BoostMultiLines &roadEdges)
{
    BoostMultiLines routes; // �o�H�f�[�^

    // ���H���Ŗ����O���t���쐬����
    BoostUndirectedGraph graph = CBoostGraphUtil::CreateGraph(roadEdges);

    // �[�_�����Ɍo�H�T�����s���A���H������������
    // �[�_�̃��X�g�A�b�v
    std::vector<BoostVertexDesc> vecVertexDesc;
    BOOST_FOREACH(BoostVertexDesc vertexDesc, boost::vertices(graph))
    {
        // �[�_(����1�̒��_)�̊m�F
        if (boost::degree(vertexDesc, graph) == 1)
            vecVertexDesc.push_back(vertexDesc);
    }
    // �o�H�T��
    std::set<BoostVertexDesc> searchedVertices; // �T���ς݊Ǘ��p
    for (auto itStart = vecVertexDesc.begin(); itStart != vecVertexDesc.end(); itStart++)
    {
        if (searchedVertices.find(*itStart) != searchedVertices.end())
            continue;   // �T���ς݂̏ꍇ��skip

        // �o�H�T��
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
                continue;   // �J�n�_�Ɠ��� or �T���ς݂̏ꍇ��skip

            // �n�_����I�_�܂ł̌o�H���m�F
            if (pred[*itEnd] == *itEnd)
                continue;  // �n�_����I�_�܂ł̌o�H���Ȃ�

            // �o�H�쐬
            BoostPolyline route;
            for (BoostVertexDesc tmpDesc = *itEnd;
                tmpDesc != *itStart; tmpDesc = pred[tmpDesc])
            {
                route.push_back(graph[tmpDesc].pt);
            }
            route.push_back(graph[*itStart].pt);
            routes.push_back(route);
            // �T���ς�
            searchedVertices.insert(*itStart);
            searchedVertices.insert(*itEnd);

            break;
        }
    }
    return routes;
}

/*!
 * @brief �����_�ߖT���H���̍ŉ��_�̒T��
 * @param[in]   routes      ���H��(�o�H)
 * @param[in]   crossPt     �����_���W
 * @param[in]   usedPtList  ���H���Z�k�����ς݂̓��H�����_�C�e���[�^�̃��X�g
 * @param[out]  itTarget    �ŉ��_���܂ޓ��H���̃C�e���[�^
 * @param[out]  itTargetPt  �ŉ��_�̃C�e���[�^
 * @param[out]  targetVec   �ŉ��_���I�_�A�O�_���n�_�Ƃ���x�N�g��
 * @param[out]  itTargetEnd ���H���̒Z�k�����ōs�����[�v�����̏I���C�e���[�^
 * @param[out]  nTargetStep ���H���̒Z�k�����ōs�����[�v�����̃X�e�b�v�l
 * @return      �T������
 * @retval      true        ����
 * @retval      false       ������
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

    // �ŉ��_�̒T��
    std::vector<std::pair<double, size_t>> vecDist;
    size_t idx = 0;
    for (auto itLine = routes.begin(); itLine != routes.end(); itLine++)
    {
        std::pair<double, int> val1(CAnalyzeRoadEdgeGeomUtil::Length(crossPt, itLine->front()), idx++);
        std::pair<double, int> val2(CAnalyzeRoadEdgeGeomUtil::Length(crossPt, itLine->back()), idx++);
        vecDist.push_back(val1);
        vecDist.push_back(val2);
    }

    std::sort(vecDist.begin(), vecDist.end());      // ����
    std::reverse(vecDist.begin(), vecDist.end());   // ���]���č~��
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
        // �ŉ��_���I�_�A�O�_���n�_�Ƃ���x�N�g��
        targetVec.x = targetPt.x() - prevPt.x();
        targetVec.y = targetPt.y() - prevPt.y();

        // ���H���̒Z�k�����Ŏg�p����p�����[�^�̐ݒ�
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
 * @brief �����_�ߖT���H���̍ŉ��_�ɑ΂���Ό��_�̒T��
 * @param[in]   routes      ���H��(�o�H)
 * @param[in]   itTarget    �ŉ��_���܂ޓ��H���̃C�e���[�^
 * @param[in]   itTargetPt  �ŉ��_�̃C�e���[�^
 * @param[in]   targetVec   �ŉ��_���I�_�A�O�_���n�_�Ƃ���x�N�g��
 * @param[in]   usedPtList  ���H���Z�k�����ς݂̓��H�����_�C�e���[�^�̃��X�g
 * @param[out]  itTwin      �Ό��_���܂ޓ��H���̃C�e���[�^
 * @param[out]  itTwinPt    �Ό��_�̃C�e���[�^
 * @param[out]  twinVec     �Ό��_���I�_�A�O�_���n�_�Ƃ���x�N�g��
 * @param[out]  itTwinEnd   ���H���̒Z�k�����ōs�����[�v�����̏I���C�e���[�^
 * @param[out]  nTwinStep   ���H���̒Z�k�����ōs�����[�v�����̃X�e�b�v�l
 * @return      �T������
 * @retval      true        ����
 * @retval      false       ������
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

    CVector2D verticalVec;  // targetVec�ɐ����ȃx�N�g��
    CVector2D targetSrcPt(itTargetPt->x(), itTargetPt->y());

    itTwin = routes.end();

    // �Ό��G�b�W�̒T��
    if (CAnalyzeRoadEdgeGeomUtil::GetVerticalVec(targetVec, verticalVec))
    {
        // targetVec�ɐ����ȃx�N�g�����擾�o�����ꍇ
        double dDist = DBL_MAX;
        for (auto itLine = routes.begin(); itLine != routes.end(); itLine++)
        {
            if (itLine == itTarget)
                continue;   // ���ڒ��_���܂ތo�H��skip

            // �e�o�H�̎n�I�_�����������ۂɁA�ŉ��_���܂ރG�b�W�ɑ΂���
            // �����ȃG�b�W�Ƃ̌����󋵂��m�F����

            // �T���Ώیo�H�̎n�_�Ǝ��_�A�I�_�ƏI�_�̑O�_�̍��W���i�[
            // �i�[����ۂ�first�Ɏn�_/�I�_, second�Ɏn�_�̎��_/�I�_�̑O�_�Ƃ���
            // (�x�N�g���Z�o����first - second�Ƃ��A�����_���牓����������Ƀx�N�g���������悤�ɂ���)
            std::vector<std::pair<BoostPolyline::iterator, BoostPolyline::iterator>> vecItCheckPt;
            if (usedPtList.find(itLine->begin()) == usedPtList.end())
                vecItCheckPt.push_back(
                    std::pair<BoostPolyline::iterator, BoostPolyline::iterator>(
                        itLine->begin(), itLine->begin() + 1)); // �o�H�̎n�_����2���_

            if (usedPtList.find(itLine->end() - 1) == usedPtList.end())
                vecItCheckPt.push_back(
                    std::pair<BoostPolyline::iterator, BoostPolyline::iterator>(
                        itLine->end() - 1, itLine->end() - 2)); // �o�H�̏I�_����2���_

            for (auto itCheckPt = vecItCheckPt.begin(); itCheckPt != vecItCheckPt.end(); itCheckPt++)
            {
                // �n�_ - �n�_�̎��_ or �I�_ - �I�_�̑O�_�̃x�N�g�����쐬
                CVector2D vec, tmpPos;
                vec.x = itCheckPt->first->x() - itCheckPt->second->x();
                vec.y = itCheckPt->first->y() - itCheckPt->second->y();
                CVector2D srcPt(itCheckPt->second->x(), itCheckPt->second->y());
                double dLength = vec.Length();
                vec.Normalize();    // ���K��

                // �쐬�����x�N�g����targetVec�ɐ����ȃx�N�g���Ƃ̌����󋵊m�F
                bool bOnline1, bOnlilne2;
                double t, s;
                if (CAnalyzeRoadEdgeGeomUtil::GetCrossPos(
                    verticalVec, targetSrcPt, vec, srcPt, tmpPos, bOnline1, bOnlilne2, t, s))
                {
                    // ��������ꍇ
                    // TODO �p�x�m�F�������ׂ���?(�T�˒��p�Ō�������͂�)
                    double dAngle = CGeoUtil::Angle(verticalVec, vec);
                    // �x�N�g���W���Ƌ����̊m�F
                    double dTmpDist = s - dLength;
                    if (CEpsUtil::GreaterEqual(s, 0) && dTmpDist < dDist)
                    {
                        // �������Ō������A�x�N�g���W�����������ꍇ
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
        // ���H���̒Z�k�����Ŏg�p����p�����[�^�̐ݒ�
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
 * @brief ���H���Z�k�����̑O����
 * @param[in]       itTarget            �ŉ��_���܂ޓ��H���̃C�e���[�^
 * @param[in/out]   itTargetPt          �ŉ��_�̃C�e���[�^
 * @param[in]       nTargetStep         �ŉ��_���܂ތo�H�𑖍�����ۂɃX�e�b�v�l
 * @param[in/out]   targetVec           �ŉ��_���I�_�A�O�_���n�_�Ƃ���x�N�g��(���K���ς�)
 * @param[in/out]   dTargetVecLength    targetVec�̐��K���O�̒���
 * @param[in]       itTwin              �Ό��_���܂ޓ��H���̃C�e���[�^
 * @param[in/out]   itTwinPt            �Ό��_�̃C�e���[�^
 * @param[in]       nTwinStep           �ŉ��_���܂ތo�H�𑖍�����ۂɃX�e�b�v�l
 * @param[in/out]   twinVec             �Ό��_���I�_�A�O�_���n�_�Ƃ���x�N�g��(���K���ς�)
 * @param[in/out]   dTwinVecLength      twinVec�̐��K���O�̒���
 * @param[in]       dEdgeLengthTh       �ŒZ�G�b�W��臒l
 * @remarks �ŉ��_���܂ތo�H�y�ёΌ��_���܂ތo�H�ɕK�v������Ε⏕�_��ǉ����鏈���̂��߁A
            �⏕�_�ǉ���Ɋe�����ŐV���ɍX�V����
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
    // ���ڃG�b�W�ƑΌ��G�b�W�̒[�_�����񂾃G�b�W�̂Ȃ��p�����p�̏ꍇ�Ɍ����Ă̑O����
    //          x
    //      x  ��
    //     ��  ��
    //     ��  ��
    //  x�� x  ��<-��
    //         ��  ��
    //  x�� x  ��<-�� �����ɒ��_���Ȃ��ƒ��ڃG�b�W�ƑΌ��G�b�W�̒[�_������
    //     ��  ��     �G�b�W�̂Ȃ��p�����p�̏ꍇ�̏��������܂������Ȃ�
    //     ��   x
    //      x

    CVector2D targetSrcPt(itTargetPt->x(), itTargetPt->y());

    // ���ڃG�b�W�̎n�_�ƑΌ��G�b�W�̏I�_���q���x�N�g��
    CVector2D vec((itTwinPt + nTwinStep)->x() - itTargetPt->x(),
        (itTwinPt + nTwinStep)->y() - itTargetPt->y());
    // ���ڃG�b�W�ɐ������������ۂ̌�_���擾����
    double dTargetInnerProduct = CGeoUtil::InnerProduct(targetVec, vec);
    CVector2D targetInsertPt = dTargetInnerProduct * targetVec + targetSrcPt;
    // ��_�ƒ��ڃG�b�W�̏I�_�����񂾃G�b�W
    CVector2D tmpPt((itTargetPt + nTargetStep)->x(), (itTargetPt + nTargetStep)->y());
    CVector2D nextTargetVec = tmpPt - targetInsertPt;

    // ���l�ɑΌ��G�b�W�̎n�_�ƒ��ڃG�b�W�̏I�_�����񂾃G�b�W���m�F����
    vec = CVector2D((itTargetPt + nTargetStep)->x() - itTwinPt->x(),
        (itTargetPt + nTargetStep)->y() - itTwinPt->y());
    // �Ό��G�b�W�̎n�_�ƒ��ڃG�b�W�̏I�_�����񂾃G�b�W�̂Ȃ��p�����p�ł͂Ȃ��ꍇ
    // �Ό��G�b�W�ɐ������������ۂ̌�_���擾����
    double dTwinInnerProduct = CGeoUtil::InnerProduct(twinVec, vec);
    CVector2D twinSrcPt(itTwinPt->x(), itTwinPt->y());
    CVector2D twinInsertPt = dTwinInnerProduct * twinVec + twinSrcPt;
    // ��_�ƑΌ��G�b�W�̏I�_�����񂾃G�b�W
    tmpPt = CVector2D((itTwinPt + nTwinStep)->x(), (itTwinPt + nTwinStep)->y());
    CVector2D nextTwinVec = tmpPt - twinInsertPt;

    // �X�V�����͍Ō�ɂ܂Ƃ߂čs��
    if (CEpsUtil::Greater(dTargetInnerProduct, dEdgeLengthTh)
        && CEpsUtil::Less(dTargetInnerProduct, dTargetVecLength)
        && CEpsUtil::Greater(nextTargetVec.Length(), dEdgeLengthTh))
    {
        // ���_�ǉ�
        BoostPolyline::iterator itInsert = itTargetPt;
        if (nTargetStep > 0)
            itInsert = itTargetPt + nTargetStep;
        BoostPolyline::iterator itTmp = itTarget->insert(
            itInsert, BoostPoint(targetInsertPt.x, targetInsertPt.y));

        // �ǉ����_�̃C�e���[�^�����ɒ��ڒ��_�̃C�e���[�^���X�V
        itTargetPt = itTmp - nTargetStep;

        // ���ڃG�b�W���X�V
        targetVec.x = (itTargetPt + nTargetStep)->x() - itTargetPt->x();
        targetVec.y = (itTargetPt + nTargetStep)->y() - itTargetPt->y();
        dTargetVecLength = targetVec.Length();
        targetVec.Normalize();  // ���K��
    }

    if (CEpsUtil::Greater(dTwinInnerProduct, dEdgeLengthTh)
        && CEpsUtil::Less(dTwinInnerProduct, dTwinVecLength)
        && CEpsUtil::Greater(nextTwinVec.Length(), dEdgeLengthTh))
    {
        // ���_�ǉ�
        BoostPolyline::iterator itInsert = itTwinPt;
        if (nTwinStep > 0)
            itInsert = itTwinPt + nTwinStep;
        BoostPolyline::iterator itTmp = itTwin->insert(
            itInsert, BoostPoint(twinInsertPt.x, twinInsertPt.y));

        // �ǉ����_�̃C�e���[�^�����ɑΌ��G�b�W�̒��_�̃C�e���[�^���X�V
        itTwinPt = itTmp - nTwinStep;

        // �Ό��G�b�W���X�V
        twinVec.x = (itTwinPt + nTwinStep)->x() - itTwinPt->x();
        twinVec.y = (itTwinPt + nTwinStep)->y() - itTwinPt->y();
        dTwinVecLength = twinVec.Length();
        twinVec.Normalize();  // ���K��
    }
}

/*!
 * @brief ���H���Z�k����
 * @param[in]       itTarget                �ŉ��_���܂ޓ��H���̃C�e���[�^
 * @param[in/out]   itTargetPt              �ŉ��_�̃C�e���[�^
 * @param[in]       nTargetStep             �ŉ��_���܂ތo�H�𑖍�����ۂɃX�e�b�v�l
 * @param[in]       targetVec               �ŉ��_���I�_�A�O�_���n�_�Ƃ���x�N�g��(���K���ς�)
 * @param[in]       dTargetVecLength        targetVec�̐��K���O�̒���
 * @param[out]      prevTargetPt            �Z�k�����ō폜�������ړ_(�ŉ��_)���W(��i�̕␳�����Ŏg�p����)
 * @param[in]       itTwin                  �Ό��_���܂ޓ��H���̃C�e���[�^
 * @param[in/out]   itTwinPt                �Ό��_�̃C�e���[�^
 * @param[in]       nTwinStep               �ŉ��_���܂ތo�H�𑖍�����ۂɃX�e�b�v�l
 * @param[out]      prevTwinPt              �Z�k�����ō폜�����Ό��_���W(��i�̕␳�����Ŏg�p����)
 * @param[in]       dVerticalAngleDiffTh    �����m�F�p�p�x����臒l(90deg����́}���e�p�x)
 * @param[in]       dEdgeLengthTh           �ŒZ�G�b�W��臒l
 * @return          ��������
 * @retval          true    ��������
 * @retval          false   �������s(�G���[)
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
    // �Z�k����
    CVector2D targetSrcPt(itTargetPt->x(), itTargetPt->y());

    // ���ڃG�b�W�̎n�_�ƑΌ��G�b�W�̎n�_���q���x�N�g��
    CVector2D vec = CVector2D(itTwinPt->x() - itTargetPt->x(), itTwinPt->y() - itTargetPt->y());
    double dAngle = CGeoUtil::Angle(targetVec, vec);
    double dEpsilon = abs(90.0 - dAngle);
    if (!CEpsUtil::Less(dEpsilon, dVerticalAngleDiffTh))
    {
        // ���ڃG�b�W�ƑΌ��G�b�W�̒[�_�����񂾃G�b�W�̂Ȃ��p�����p�ł͂Ȃ��ꍇ
        // ���ڃG�b�W�ɐ������������ۂ̌�_���擾����
        double dInnerProduct = CGeoUtil::InnerProduct(targetVec, vec);
        CVector2D pt = dInnerProduct * targetVec + targetSrcPt;
        // ��_�ƒ��ڃG�b�W�̏I�_�����񂾃G�b�W
        CVector2D tmpPt((itTargetPt + nTargetStep)->x(), (itTargetPt + nTargetStep)->y());
        CVector2D tmpVec = tmpPt - pt;
        if (CEpsUtil::Less(dInnerProduct, dTargetVecLength)
            && CEpsUtil::Greater(tmpVec.Length(), dEdgeLengthTh))
        {
            // ��_�����ڃG�b�W��Ɉʒu���A�Z�k�����ۂɃG�b�W����臒l���傫���ꍇ
            // ��i�̕␳�����Ŏg�p���邽�ߍ폜���钸�_���W��ێ�����
            prevTargetPt = BoostPoint(*itTargetPt);

            // ���W�l���X�V
            itTargetPt->x(pt.x);
            itTargetPt->y(pt.y);
        }
        else
        {
            // ��_�����ڃG�b�W��Ɉʒu���Ȃ�
            // �܂��́A�Z�k�����ۂɃG�b�W����臒l�ȉ��̏ꍇ

            // ��i�̕␳�����Ŏg�p���邽�ߍ폜���钸�_���W��ێ�����
            prevTargetPt = BoostPoint(*itTargetPt);

            // ���ړ_���폜���āA���_�Ɉړ�����
            itTargetPt = itTarget->erase(itTargetPt);
            if (nTargetStep < 0 && itTarget->size() > 0)
                itTargetPt = itTarget->end() - 1;
            else
                itTargetPt = itTarget->begin();
        }
    }
    else
    {
        // ���ڃG�b�W�ƑΌ��G�b�W�̒[�_�����񂾃G�b�W�̂Ȃ��p�����p�̏ꍇ

        // ���ړ_�̎��_�ƑΌ��_�̎��_���q�����G�b�W�̊m�F���s��
        vec.x = (itTwinPt + nTwinStep)->x() - (itTargetPt + nTargetStep)->x();
        vec.y = (itTwinPt + nTwinStep)->y() - (itTargetPt + nTargetStep)->y();
        dAngle = CGeoUtil::Angle(targetVec, vec);
        double dEpsilon = abs(90.0 - dAngle);

        if (CEpsUtil::Less(dEpsilon, dVerticalAngleDiffTh))
        {
            // ���_���m�����񂾃G�b�W�����ڃG�b�W�ɐ����ȏꍇ

            // ��i�̕␳�����Ŏg�p���邽�ߍ폜���钸�_���W��ێ�����
            prevTargetPt = BoostPoint(*itTargetPt);
            prevTwinPt = BoostPoint(*itTwinPt);

            // ���ړ_���폜���āA���_�Ɉړ�����
            itTargetPt = itTarget->erase(itTargetPt);   // �폜�����_�̎��C�e���[�^���ԋp
            if (nTargetStep < 0 && itTarget->size() > 0)
                itTargetPt = itTarget->end() - 1;
            else
                itTargetPt = itTarget->begin();

            // �Ό��_���폜���āA���_�Ɉړ�����
            itTwinPt = itTwin->erase(itTwinPt); // �폜�����_�̎��C�e���[�^���ԋp
            if (nTwinStep < 0 && itTwin->size() > 0)
                itTwinPt = itTwin->end() - 1;
            else
                itTwinPt = itTwin->begin();
        }
        else
        {
            return false;  // TODO �C���M�����[
        }
    }

    return true;    // ���폈��
}

/*!
 * @brief ���H���Z�k��̕␳����
 * @param[in]       itTarget            ���ڐ��C�e���[�^
 * @param[in/out]   itTargetPt          ���ړ_�C�e���[�^
 * @param[in]       nTargetStep         ���ړ_�C�e���[�^�̃��[�v�p�X�e�b�v��
 * @param[in]       prevTargetPt        ���ړ_�̑O�_���W
 * @param[in]       dCorrectionAngleTh  �����m�F�̊p�x臒l(deg)
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
                // �����ȏꍇ�͒��ړ_�����_�Ɉړ�����
                prevTargetPt = BoostPoint(*itTargetPt); // �X�V

                // ���ړ_���폜���āA���_�Ɉړ�����
                itTargetPt = itTarget->erase(itTargetPt);
                if (nTargetStep < 0 && itTarget->size() > 0)
                    itTargetPt = itTarget->end() - 1;
                else
                    itTargetPt = itTarget->begin();
            }
            else
            {
                break;  // �␳�����I��
            }
        }
    }
}

/*!
 * @brief �����_�|���S���̍쐬
 * @param[in] crossingOutlines �����_�|���S���̗֊s���ɊY����������f�[�^
 * @return �����_�|���S��
*/
BoostPolygon CRoadDivision::loopingCrossingArea(
    BoostMultiLines crossingOutlines)
{
    BoostPolygon polygon;

    // �ԓ��������̗֊s�������[�v��
    BoostUndirectedGraph graph = CBoostGraphUtil::CreateGraph(crossingOutlines);
    // �o�H�T���̊J�n���_
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
        // �[���D��T��
        std::vector<boost::default_color_type> vecVertexColor(boost::num_vertices(graph));
        auto idmap = get(boost::vertex_index, graph);
        auto vcmap = boost::make_iterator_property_map(vecVertexColor.begin(), idmap);

        std::vector<BoostVertexDesc> vecRoute;  // �o�H
        CBoostDFSVisitor vis(vecRoute);
        boost::depth_first_visit(graph, searchVertexDesc, vis, vcmap);

        // �ԓ��������|���S��
        for (auto itDesc = vecRoute.begin(); itDesc != vecRoute.end(); itDesc++)
        {
            polygon.outer().push_back(graph[*itDesc].pt);
        }
        bg::correct(polygon);
    }

    return polygon;
}

/*!
 * @brief �ԓ��������ؒf
 * @param[in]   roadPolyogns        ���H�|���S���Q
 * @param[in]   blockPolyogns       �X��|���S���Q
 * @param[in]   crossing            �����_���Q
 * @param[out]  crossingPolygons    �����_�̈�Q
 * @param[out]  remainingPolygons   �c�����H�|���S���Q
*/
void CRoadDivision::createCrossingPolygons(
    BoostMultiPolygon &roadPolyogns,
    BoostMultiPolygon &blockPolyogns,
    std::vector<CCrossingData> &crossing,
    std::vector<CRoadData> &crossingPolygons,
    BoostMultiPolygon &remainingPolygons)
{
    const double dParallelAngleTh = 10.0;       // ���s�m�F�p�p�x臒l
    const double dVerticalAngleDiffTh = 10.0;   // �����m�F�p�p�x����臒l(90deg����́}���e�p�x)
    const double dEdgeLengthTh = 0.5;           // �ŒZ�G�b�W��臒l
    const double dCorrectionAngleTh = 10.0;     // �␳�Ώۂ̓��H���̊p�x臒l

    // �����_���Ƃɍ��
    std::vector<CRoadData> tmpCrossingPolygons;
    for (auto itCross = crossing.begin(); itCross != crossing.end(); itCross++)
    {
        if (!bg::is_empty(itCross->Area()))
        {
            // �����_�̈悪�擾�o���Ă���ꍇ
            BoostPolygon area = itCross->Area();
            BoostPoint crossPt = itCross->Point();  // �����_���W

            // �����_�̈�Əd�􂷂铹�H�����擾����
            BoostMultiLines lines = GetRoadEdges(roadPolyogns, area, crossPt);

            // ���H���͐�����Ԃ̂��ߌ������Čo�H��Ԃɂ���
            BoostMultiLines routes = GetRoutes(lines);

            if (routes.size() != itCross->BranchNum())
            {
                // todo
                // ���򐔂Ɠ��H���̖{������v���Ȃ��ꍇ�̑Ή�
                continue;
            }

            // ���򐔕��A�ؒf�ʒu���菈�����s��
            std::set<BoostPolyline::iterator> usedPtList;   // �g�p�ςݒ��_
            std::vector<PairData> segmentationLines; // ������
            for (int nCount = 0; nCount < itCross->BranchNum(); nCount++)
            {
                // �ŉ��_�A�Ό��_�̒T��
                BoostMultiLines::iterator itTarget, itTwin;
                BoostPolyline::iterator itTargetPt, itTwinPt, itTargetEnd, itTwinEnd;
                CVector2D targetVec, twinVec;
                int nTargetStep, nTwinStep;
                if (!SearchTarget(routes, crossPt, usedPtList, itTarget, itTargetPt,
                    targetVec, itTargetEnd, nTargetStep))
                    continue;   // �������̏ꍇ

                if (!SearchTwin(routes, itTarget, itTargetPt, targetVec, usedPtList,
                    itTwin, itTwinPt, twinVec, itTwinEnd, nTwinStep))
                    continue;   // �������̏ꍇ

                CVector2D targetSrcPt(itTargetPt->x(), itTargetPt->y());

                if (itTwin != routes.end())
                {
                    // �Ό��G�b�W�𔭌������ꍇ
                    BoostPoint prevTargetPt, prevTwinPt;
                    while (itTarget->size() > 1 && itTwin->size() > 1
                        && itTargetPt != itTargetEnd && itTwinPt != itTwinEnd)
                    {
                        // ���ړ��H���ƑΌ����H���̓_����2�_�ȏォ��
                        // ���ړ_�ƑΌ��_���L���ȏꍇ

                        // �ŐV���ɍX�V
                        targetSrcPt.x = itTargetPt->x();
                        targetSrcPt.y = itTargetPt->y();

                        // �Ό��G�b�W�����s���m�F����
                        targetVec.x = (itTargetPt + nTargetStep)->x() - itTargetPt->x();
                        targetVec.y = (itTargetPt + nTargetStep)->y() - itTargetPt->y();
                        twinVec.x = (itTwinPt + nTwinStep)->x() - itTwinPt->x();
                        twinVec.y = (itTwinPt + nTwinStep)->y() - itTwinPt->y();
                        double dAngle = CGeoUtil::Angle(targetVec, twinVec);
                        if (!CEpsUtil::Less(dAngle, dParallelAngleTh))
                        {
                            break;   // ���s�łȂ��ꍇ�͏I��
                        }

                        // ���ڃG�b�W�̒������擾
                        double dTargetVecLength = targetVec.Length();
                        targetVec.Normalize();  // ���K��
                        // �Ό��G�b�W�̒������擾
                        double dTwinVecLength = twinVec.Length();
                        twinVec.Normalize();    // ���K��

                        // ���ڃG�b�W�ƑΌ��G�b�W�̒[�_�����񂾃G�b�W�̂Ȃ��p�����p�̏ꍇ�Ɍ����Ă̑O����
                        roadEdgeShorteningPreProc(
                            itTarget, itTargetPt, nTargetStep, targetVec, dTargetVecLength,
                            itTwin, itTwinPt, nTwinStep, twinVec, dTwinVecLength, dEdgeLengthTh);

                        // �Z�k����
                        if (!roadEdgeShorteningProc(
                            itTarget, itTargetPt, nTargetStep, targetVec, dTargetVecLength, prevTargetPt,
                            itTwin, itTwinPt, nTwinStep, prevTwinPt, dVerticalAngleDiffTh, dEdgeLengthTh))
                            break;  // �Z�k�����ɃG���[�����������ꍇ
                    }

                    // ���������쐬���邽�߂̒��_�y�A��ێ�
                    bool bTarget = true;
                    if (itTarget->begin() != itTargetPt)
                        bTarget = false;  // �I�_�̏ꍇ
                    bool bTwin = true;
                    if (itTwin->begin() != itTwinPt)
                        bTwin = false;  // �I�_�̏ꍇ
                    PairData pairData(
                        itTarget, bTarget, nTargetStep, prevTargetPt,
                        itTwin, bTwin, nTwinStep, prevTwinPt);
                    segmentationLines.push_back(pairData);

                    // �Z�k�����ς݃��X�g�ɓo�^
                    usedPtList.insert(itTargetPt);
                    usedPtList.insert(itTwinPt);
                }
            }

#if 0
            // todo �v����
            // �␳����
            BoostMultiLines crossingOutlines;  // �����_�̈�̊O�֊s��
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
            BoostMultiLines crossingOutlines;  // �����_�̈�̊O�֊s��
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
            // �ߖT���H����Z�k�����ۂɎc������������ꍇ�A�ԓ��������̗֊s���Ƃ��ċL��
            for (auto itLine = routes.begin(); itLine != routes.end(); itLine++)
            {
                if (bg::length(*itLine))
                {
                    crossingOutlines.push_back(*itLine);
                }
            }

            // �ԓ��������̗֊s�������[�v��
            BoostPolygon polygon = loopingCrossingArea(crossingOutlines);

            // ���H����͂ݏo�Ă��Ȃ����m�F
            BoostMultiPolygon overlapRegions;
            bg::intersection(roadPolyogns, polygon, overlapRegions);
            double dDiffArea = CAnalyzeRoadEdgeGeomUtil::RoundN(abs(bg::area(overlapRegions) - bg::area(polygon)), 3);

            if (!bg::is_empty(polygon) && bg::is_valid(polygon)
                && CEpsUtil::Zero(dDiffArea))
            {
                // �ԓ��������|���S���̕ۑ�
                CRoadData roadData;
                roadData.Polygon(polygon);
                roadData.Type(RoadSectionType::ROAD_SECTION_CROSSING);
                roadData.Division(itCross->BranchNum());
                tmpCrossingPolygons.push_back(roadData);
            }
        }
    }

    // ���H�|���S������ԓ��������|���S��������
    std::vector<CRoadData> errCrossingPolygons; // �������ɕs���|���S����������������_�|���S�����Q
    divisionPolygon(roadPolyogns, tmpCrossingPolygons, crossingPolygons, errCrossingPolygons, remainingPolygons);
}

// �|���S���̕���
/*!
 * @brief
 * @param[in]   roadPolyogns        ���H�|���S��
 * @param[in]   roadData            �����Ώۂ̓��H�|���S���Q
 * @param[out]  dstData             ����������ɍs�������H�|���S���Q
 * @param[out]  errData             �������ɕs���|���S�����������铹�H�|���S���Q
 * @param[out]  remainingPolygons   �c�����H
*/
void CRoadDivision::divisionPolygon(
    BoostMultiPolygon &roadPolyogns,
    std::vector<CRoadData> &roadData,
    std::vector<CRoadData> &dstData,
    std::vector<CRoadData> &errData,
    BoostMultiPolygon &remainingPolygons)
{
    const double dSampling = 0.1;
    // ���H�|���S������Ώۃ|���S��������
    remainingPolygons = BoostMultiPolygon(roadPolyogns);
    for (auto it = roadData.begin(); it != roadData.end(); it++)
    {
        BoostMultiPolygon tmpRemainingPolygons(remainingPolygons);
        for (auto itPoly = tmpRemainingPolygons.begin(); itPoly != tmpRemainingPolygons.end(); itPoly++)
        {
            if (bg::disjoint(*itPoly, it->Polygon()))
                continue;

            // �ߖT�T���p�̃f�[�^�쐬
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

            // �␳�̊֌W�Ŕ팸�Z�ΏۂɌ��Z�Ώۂ̒��_��ǉ�����
            std::vector<AddPointInfo> vecAddPoints;
            BoostPolygon polygon = it->Polygon();
            for (auto itCrossPt = polygon.outer().begin(); itCrossPt != polygon.outer().end() - 1; itCrossPt++)
            {
                // �ŋߖT�T��
                std::vector<std::pair<BoostPoint, NearestPointInfo>> vecValues;
                rtree.query(bg::index::nearest(*itCrossPt, 1), std::back_inserter(vecValues));

                // �}���n�_�̌���
                // �}���\��ӂ̎n�I�_
                BoostRing::iterator itStartPt = vecValues[0].second.m_itPt;
                BoostRing::iterator itNextPt = itStartPt + 1;
                CVector2D startPos(itStartPt->x(), itStartPt->y());
                CVector2D nextPos(itNextPt->x(), itNextPt->y());

                CVector2D targetPos(itCrossPt->x(), itCrossPt->y());
                CVector2D vec1 = nextPos - startPos;    // �}���\��ӂ̃x�N�g��
                double dLength = vec1.Length();
                vec1.Normalize();
                CVector2D vec2 = targetPos - startPos;  // �n�_����}���_�܂ł̃x�N�g��
                double dInnerProduct = CGeoUtil::InnerProduct(vec1, vec2);  // �ˉe
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

            // ���_�ǉ�
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
 * @brief �|���S���̌��Z����(�X�p�C�N�m�C�Y/���Ȍ����m�F�̂�)
 * @param[in]   polygons    �팸�Z�̈�
 * @param[in]   polygon     ���Z�̈�
 * @param[out]  bSpike      ���Z���ʂ̃X�p�C�N�m�C�Y�̗L��
 * @param[out]  bCross      ���Z���ʂ̎��Ȍ����̗L��
 * @return ���Z����
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
            // �Փ˂��Ȃ��ꍇ
            diffPolygons.push_back(*itPoly);
        }
        else
        {
            // �̈�����Z
            BoostMultiPolygon tmpDiff;
            bg::difference(*itPoly, polygon, tmpDiff);
            diffPolygons.insert(diffPolygons.end(), tmpDiff.begin(), tmpDiff.end());

            if (!bSpike || !bCross)
            {
                for (auto itDiff = tmpDiff.begin(); itDiff != tmpDiff.end(); itDiff++)
                {
                    // �X�p�C�N�m�C�Y�m�F
                    BoostMultiPoints spikePts;
                    bSpike = CAnalyzeRoadEdgeGeomUtil::CheckSpike(*itDiff, spikePts, true);

                    // ���Ȍ����m�F
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
