#include "pch.h"
#include "CAnalyzeTunnel.h"

/*!
 * @brief �g���l���T��
 * @param[in]   roadEdges       ���H��
 * @param[in]   roadFacilities  ���H�{��
 * @param[in]   dBuffDist       �g���l���T���̈攼�a(m)
 * @param[in]   dAngleDiffTh    �p�x����臒l(deg)
 * @return      �g���l�������ɊY�����铹�H��
*/
std::vector<BoostPairLine> CAnalyzeTunnel::Process(
    const std::vector<CDMRoadDataManager::RoadEdgeData> roadEdges,
    const std::vector<CDMRoadDataManager::RoadFacilitiesData> roadFacilities,
    const double dBuffDist,
    const double dAngleDiffTh)
{
    BoostMultiPoints tunnelEntrancePoints;              // �g���l���̓����̃|�C���g
    BoostMultiLines tunnelEntrancePolylines;            // �g���l���̓����̃|�����C��
    BoostMultiLines convertedTunnelEntrancePolylines;   // �g���l�������̏��𒼐��ɕϊ������|�����C��
    std::vector<BoostPairPoint> tunnelBothEndsPoint;    // �g���l�������̏��𓹘H�����[�_�ɕϊ������|�C���g

    for (std::vector<CDMRoadDataManager::RoadFacilitiesData>::const_iterator it = roadFacilities.cbegin(); it != roadFacilities.cend(); it++)
    {
        // ���H�{�݃f�[�^�̓��A�g���l��(��敪)�𒊏o
        if (it->nRoadFacilitiesCode == DMRoadFacilitiesCode::ROAD_TUNNELS && it->nGeometryType == DMGeometryType::UNCLASSIFIED)
        {
            // �g���l�����|�C���g���|�����C��������
            if (it->nRoadFacilitiesDataType == CDMRoadDataManager::RoadFacilitiesDataType::ROAD_FACILITIES_POLYGON_DATA
                || it->nRoadFacilitiesDataType == CDMRoadDataManager::RoadFacilitiesDataType::ROAD_FACILITIES_LINE_DATA)
            {
                // �|�����C���̏ꍇ�͒��S�_�����߁A�|�C���g�ɓ���i�v�C���j
                BoostPolyline polylineVertices;
                for (const CVector3D &point : it->vecPolyline)
                {
                    polylineVertices.emplace_back(BoostPoint(point.x, point.y));
                }

                if (bg::disjoint(tunnelEntrancePolylines, polylineVertices))
                {
                    // �T���v���f�[�^�ɖʂƐ��ŏd�����Ă���g���l�����m�F��������
                    // �d�������O���������ǉ�
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
    // �g���l���̓����̃|�C���g�̕ϊ�
    ///////////////////////////////////////////////////////////

    // ���H���� BoostMultiLines �^�ɕϊ�
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

    // �T���v�����O
    BoostMultiLines samplifyPolyline = CAnalyzeRoadEdgeGeomUtil::Sampling(roadEdgePolylines, 2.0);

    // �ߖT�T���p�̃f�[�^�쐬
    BoostMultiLinesItRTree rtree;
    BoostMultiPoints samplingPoints;
    for (size_t i = 0; i < samplifyPolyline.size(); i++)
    {
        BoostMultiLines::iterator roadEdgePolylineIt = roadEdgePolylines.begin() + i;
        BoostMultiLines::iterator samplifyPolylineIt = samplifyPolyline.begin() + i;

        for (BoostPolyline::iterator polylineIt = samplifyPolylineIt->begin(); polylineIt != samplifyPolylineIt->end(); polylineIt++)
        {
            // �f�[�^�ǉ�
            rtree.insert(std::pair<BoostPoint, BoostMultiLines::iterator>(*polylineIt, roadEdgePolylineIt));

            // �T���v�����O�̃f�[�^�m�F�p
            samplingPoints.emplace_back(*polylineIt);
        }
    }

    // �g���l���_�̋ߖT���H���̒T��
    std::vector<std::pair<BoostPoint, BoostMultiLines>> rtreeResultList;
    for (BoostMultiPoints::iterator pointIt = tunnelEntrancePoints.begin();
        pointIt != tunnelEntrancePoints.end(); pointIt++)
    {
        BoostMultiLines resultLine;
        std::set<BoostMultiLines::iterator> searchedLines;

        // ���s
        std::vector<std::pair<BoostPoint, BoostMultiLines::iterator>> vecValues;
        rtree.query(bg::index::nearest(*pointIt, 14), std::back_inserter(vecValues));

        // ���ʃf�[�^�ɕۑ�
        for (const auto &value : vecValues)
        {
            // �����ӂ�ۑ����Ă���ꍇ�̓X�L�b�v
            if (searchedLines.find(value.second) != searchedLines.end())
            {
                continue;
            }
            searchedLines.insert(value.second);

            resultLine.emplace_back(*value.second);
        }

        rtreeResultList.emplace_back(std::pair<BoostPoint, BoostMultiLines>(*pointIt, resultLine));
    }

    // �g���l���̓����Ƀ|�����C��������
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

                // �Ώۂ̋ߖT�ӂ̃x�N�g�������߂�
                CVector2D targetRTreePolylineVec = CVector2D(
                    targetLine[1].x() - targetLine[0].x(),
                    targetLine[1].y() - targetLine[0].y());

                // �g���l�������_����Ώۃ|�����C���̐���
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

                // ���̋ߖT���Ƃ̐ڐ������߂�
                for (BoostPolyline otherTargetRTreePolyline : targetRTreeBoostMultiLines)
                {
                    for (size_t j = 0; j < otherTargetRTreePolyline.size() - 1; j++)
                    {
                        BoostPolyline otherLine;
                        otherLine.emplace_back(otherTargetRTreePolyline[j]);
                        otherLine.emplace_back(otherTargetRTreePolyline[j + 1]);

                        // �Ώۃ|�����C���Ɠ����ߖT���Ȃ�X�L�b�v
                        if (bg::equals(otherLine, targetLine))
                        {
                            continue;
                        }

                        // ���̋ߖT�ӂ̃x�N�g�������߂�
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
                    // ��̌�_�����߂�
                    BoostPoint targetCrossPosBoostPoint = BoostPoint(targetCrossPos.x, targetCrossPos.y);
                    BoostPoint otherCrossPosBoostPoint = BoostPoint(otherCrossPos.x, otherCrossPos.y);

                    BoostPolyline line;
                    line.emplace_back(targetCrossPosBoostPoint);
                    line.emplace_back(otherCrossPosBoostPoint);

                    // �g���l�������̒������ɕۑ�
                    convertedTunnelEntrancePolylines.emplace_back(line);

                    // �g���l�������̗��[���ɕۑ�
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
    // �g���l���̓����̃|�����C���̕ϊ�
    ///////////////////////////////////////////////////////////

    for (BoostMultiLines::iterator entrancePolylineIt = tunnelEntrancePolylines.begin();
        entrancePolylineIt != tunnelEntrancePolylines.end(); entrancePolylineIt++)
    {
        // ��������̕s�v�_�̍폜
        BoostPolyline entrancePolyline;// = CAnalyzeRoadEdgeGeomUtil::Simplify(*entrancePolylineIt);
        bg::simplify(*entrancePolylineIt, entrancePolyline, 0.1);

        // �Œ������̎擾
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

        // �g���l�������ɓ�̓_���쐬
        BoostMultiPoints crossPoint, uniqueCrossPoint;
        bg::intersection(maxLengthPolyline, roadEdgePolylines, crossPoint);
        // ���꒸�_�폜
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
                std::sort(vecDist.begin(), vecDist.end());      // ����
                // �g���l����������̒��_���狗�����߂���_��2�_�擾
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
    // ���H���ƌ��o�����g���l���̗��[�|�C���g�𓝍�
    ///////////////////////////////////////////////////////////
    BoostMultiLinesItRTree roadEdgeRTree;
    for (BoostMultiLines::iterator roadEdgeIt = roadEdgePolylines.begin(); roadEdgeIt != roadEdgePolylines.end(); roadEdgeIt++)
    {
        for (BoostPolyline::iterator edgeIt = roadEdgeIt->begin(); edgeIt != roadEdgeIt->end(); edgeIt++)
        {
            roadEdgeRTree.insert(std::pair<BoostPoint, BoostMultiLines::iterator>(*edgeIt, roadEdgeIt));
        }
    }

    // �g���l���̗��[�|�C���g���Ƃɕӂ�����
    BoostMultiLines pairingTargetPolylines;
    for (std::vector<BoostPairPoint>::iterator bothPointIt = tunnelBothEndsPoint.begin(); bothPointIt != tunnelBothEndsPoint.end(); bothPointIt++)
    {
        // �g���l��������x�N�g��
        CVector2D tunnnelVec(
            bothPointIt->second.x() - bothPointIt->first.x(),
            bothPointIt->second.y() - bothPointIt->first.y());

        BoostMultiPoints tmp;
        tmp.emplace_back(bothPointIt->first);
        tmp.emplace_back(bothPointIt->second);

        for (BoostMultiPoints::iterator pointIt = tmp.begin(); pointIt != tmp.end(); pointIt++)
        {
            // ���s
            std::vector<std::pair<BoostPoint, BoostMultiLines::iterator>> vecValues;
            roadEdgeRTree.query(bg::index::nearest(*pointIt, 4), std::back_inserter(vecValues));

            // ���������|�����C�����ƂɃg���l���̗��[�|�C���g���܂ނ��m�F
            for (const std::pair<BoostPoint, BoostMultiLines::iterator> &value : vecValues)
            {
                // �����_�̏ꍇ�̓X�L�b�v
                if (bg::equals(value.first, *pointIt))
                {
                    continue;
                }

                // �g���l���x�N�g���ƕ��s�ł͂Ȃ����Ƃ̊m�F
                CVector2D vec(
                    value.second->back().x() - value.second->front().x(),
                    value.second->back().y() - value.second->front().y());
                double dAngle = CGeoUtil::Angle(tunnnelVec, vec);
                if (CEpsUtil::Less(CAnalyzeRoadEdgeGeomUtil::RoundN(abs(dAngle), 2), dAngleDiffTh)
                    || CEpsUtil::Less(CAnalyzeRoadEdgeGeomUtil::RoundN(abs(180.0 - dAngle), 2), dAngleDiffTh))
                {
                    // �g���l���x�N�g���Ɠ��H���x�N�g���̂Ȃ��p��0deg�܂���180deg�̏ꍇ
                    continue;
                }

                BoostPoint point = *pointIt;
                BoostMultiLines::iterator multiLineIt = value.second;
                bool isInsert = false;

                // �|�����C�����̂��ׂĂ̒����ɂ��Ċm�F
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

                    // �Ώۓ_���܂�3�̓_���������A
                    // �Ώۓ_�E�|�����C���̎n�_�E�|�����C���̏I�_��3�_���������ǂ����m�F
                    double dAngleDiff = CAnalyzeRoadEdgeGeomUtil::RoundN(
                        abs(180.0 - CGeoUtil::Angle(O2U, O2V)), 2);
                    if (CEpsUtil::Less(dAngleDiff, dAngleDiffTh))
                    {
                        // �ӓ��ɒ��_��ǉ�����
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
    //// �������m��T��
    /////////////////////////////////////////////////////////////
    std::vector<BoostPairLine> shortestPathPolyline;
    BoostMultiPoints searchedEntranceVertex;
    for (auto itTargetPair = tunnelBothEndsPoint.begin();
        itTargetPair != tunnelBothEndsPoint.end(); itTargetPair++)
    {
        // �T���ς݂̊m�F
        if (!bg::disjoint(searchedEntranceVertex, itTargetPair->first)
            || !bg::disjoint(searchedEntranceVertex, itTargetPair->second))
        {
            continue;
        }

        // �O���t�쐬�͈͂̎Z�o
        BoostPoint firstEntrancePoint = itTargetPair->first;
        BoostPoint lastEntrancePoint = itTargetPair->second;
        CVector2D vec(
            firstEntrancePoint.x() - lastEntrancePoint.x(),
            firstEntrancePoint.y() - lastEntrancePoint.y());
        CVector2D centerPos = 0.5 * vec + CVector2D(lastEntrancePoint.x(), lastEntrancePoint.y());
        BoostMultiPolygon area = CAnalyzeRoadEdgeGeomUtil::Buffering(
            BoostPoint(centerPos.x, centerPos.y), dBuffDist);

        // �O���t�쐬
        BoostMultiLines roadEdgePolylinesForGraph;
        bg::intersection(roadEdgePolylines, area, roadEdgePolylinesForGraph);
        BoostUndirectedGraph graph = CBoostGraphUtil::CreateGraph(roadEdgePolylinesForGraph);

        // tunnelEntrancePointDescList: �g���l���|�C���g�̃f�X�N���v�^�[
        // BoostPoint: �g���l���[�̍��W
        // BoostPoint: ������̃g���l���[�̍��W
        // BoostVertexDesc: �g���l���[�̃f�X�N���v�^
        // BoostVertexDesc: ������̃g���l���[�̃f�X�N���v�^)
        std::vector<EntranceData> tunnelEntrancePointDescList;

        // �O���t����g���l���̓����|�C���g�̃f�X�N���v�^�[��o�^
        int nTargetIdx = -1;
        for (auto itPair = tunnelBothEndsPoint.begin();
            itPair != tunnelBothEndsPoint.end(); itPair++)
        {
            if (bg::disjoint(area, itPair->first)
                || bg::disjoint(area, itPair->second)
                || !bg::disjoint(searchedEntranceVertex, itPair->first)
                || !bg::disjoint(searchedEntranceVertex, itPair->second))
                // ���ڃg���l������������ڗ̈�ƏՓ˂��Ă��Ȃ�
                // �܂��͒T���ς݃g���l���̏ꍇ
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
            continue;   // ���ڃg���l��������ɃO���t�̃f�B�X�N���v�^���肠�Ă��o���Ȃ������ꍇ

        // dijkstra�p�ɖ����O���t�ɃG�b�W�̒�����ݒ肷��
        BOOST_FOREACH(BoostEdgeDesc edgeDesc, boost::edges(graph))
        {
            graph[edgeDesc].dLength = CBoostGraphUtil::EdgeLength(graph, edgeDesc);
        }

        // rtree�쐬
        BoostVertexRTree shortestPathsRtree;
        for (EntranceData tunnelEntrancePointDesc : tunnelEntrancePointDescList)
        {
            shortestPathsRtree.insert(std::pair<BoostPoint, BoostVertexDesc>(
                std::get<0>(tunnelEntrancePointDesc), std::get<2>(tunnelEntrancePointDesc)));
            shortestPathsRtree.insert(std::pair<BoostPoint, BoostVertexDesc>(
                std::get<1>(tunnelEntrancePointDesc), std::get<3>(tunnelEntrancePointDesc)));
        }

        // ���ڃg���l��������̌o�H�T��
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

            // �ߖT�����_�T��
            std::vector<std::pair<BoostPoint, BoostVertexDesc>> vecValues;
            shortestPathsRtree.query(bg::index::nearest(vertexPt, 8), std::back_inserter(vecValues));

            // �o�H�T��
            std::vector<BoostVertexDesc> pred(boost::num_vertices(graph), BoostUndirectedGraph::null_vertex());
            std::vector<double> vecDistance(boost::num_vertices(graph));
            boost::dijkstra_shortest_paths(graph,
                vertexDesc,
                boost::predecessor_map(pred.data()).distance_map(vecDistance.data()).weight_map(boost::get(&BoostEdgeProperty::dLength, graph)));

            std::vector<std::tuple<BoostPolyline, BoostVertexDesc, double>> pathChoicesPolylines; // �ߖT�T���Ō��o�����ʂ̃g���l���|�C���g�ւ̍ŒZ���[�g���A���o�|�C���g�̃f�X�N���v�^
            for (auto itVal = vecValues.begin(); itVal != vecValues.end(); itVal++)
            {
                // �o�H�Ȃ��̏ꍇ
                // �����_�̏ꍇ�Ƃ���g���l�������ɑ΂��������̓_�������ꍇ
                // �T���ςݒ��_�̏ꍇ�̓X�L�b�v
                if (pred[itVal->second] == itVal->second ||
                    itVal->second == vertexDesc || itVal->second == pairVertexDesc
                    || bg::overlaps(searchedEntranceVertex, itVal->first))
                {
                    continue;
                }

                // ���ړ_����ߖT�_�܂ł̌o�H�𒊏o
                BoostPolyline pathPolyline;
                for (BoostVertexDesc tmpDesc = itVal->second;
                    tmpDesc != vertexDesc; tmpDesc = pred[tmpDesc])
                {
                    pathPolyline.emplace_back(graph[tmpDesc].pt);
                }
                pathPolyline.emplace_back(graph[vertexDesc].pt);

                // �ŒZ�o�H�̑I����(�o�H�|�����C���ƌ��o�|�C���g�̃f�X�N���v�^)��ۑ�
                pathChoicesPolylines.emplace_back(std::tuple<BoostPolyline, BoostVertexDesc, double>(pathPolyline, itVal->second, vecDistance[itVal->second]));
            }

            // ���o�����o�H�̒��ŁA�ŒZ�̂��̂�ۑ�����
            // ���ɕۑ������o�H�̃S�[���ƈ�v���Ă����ꍇ�͌o�H���m�肷��
            int shortestPolylineIndex = 0;
            if (pathChoicesPolylines.size() > 0)
            {
                if (pairPathPolyline.size() > 0)
                {
                    ///
                    /// ���ɕۑ�����Ă���ŒZ�o�H������ꍇ
                    ///

                    double shortestDistance = std::get<2>(pathChoicesPolylines[0]);
                    bool isPairing = false;

                    // ��ɔ��������o�H�̏I�_�ƃy�A�ɂȂ�g���l����������_�̒T��
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
                                // ��ɔ��������o�H�̏I�_�ƃy�A�ɂȂ�g���l��������̏ꍇ
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

                    // �y�A���ŒZ�o�H�����������ꍇ�͌o�H���m�̏Փˊm�F�����ĕۑ�����
                    if (isPairing
                        && bg::disjoint(pairPathPolyline[0], std::get<0>(pathChoicesPolylines[shortestPolylineIndex])))
                    {
                        // �g���l�����H���Ƃ��ēo�^
                        shortestPathPolyline.emplace_back(
                            BoostPairLine(pairPathPolyline[0], std::get<0>(pathChoicesPolylines[shortestPolylineIndex])));

                        // �ŒZ�o�H�̌����ςݑΏۂ��A2�{�̎n�_�I�_�̗����o�^����
                        searchedEntranceVertex.emplace_back(std::get<0>(tunnelEntrancePointDescList[nTargetIdx]));
                        searchedEntranceVertex.emplace_back(std::get<1>(tunnelEntrancePointDescList[nTargetIdx]));
                        searchedEntranceVertex.emplace_back(graph[firstPathEndVertexDesc].pt);
                        searchedEntranceVertex.emplace_back(graph[checkDesc].pt);
                    }
                }
                else
                {
                    ///
                    /// �܂��ŒZ�o�H���Ȃ��ꍇ
                    ///
                    double shortestDistance = std::get<2>(pathChoicesPolylines[0]);

                    for (int i = 1; i < pathChoicesPolylines.size(); i++)
                    {
                        if (shortestDistance > std::get<2>(pathChoicesPolylines[i]))
                        {
                            // �ŒZ�Ȃ�ۑ�
                            shortestPolylineIndex = i;
                            shortestDistance = std::get<2>(pathChoicesPolylines[shortestPolylineIndex]);
                        }
                    }
                    // �g���l���Б��̓��H��
                    pairPathPolyline.emplace_back(std::get<0>(pathChoicesPolylines[shortestPolylineIndex]));
                    firstPathStartVertexDesc = vertexDesc;
                    firstPathEndVertexDesc = std::get<1>(pathChoicesPolylines[shortestPolylineIndex]);
                }
            }
        }
    }
    return shortestPathPolyline;
}
