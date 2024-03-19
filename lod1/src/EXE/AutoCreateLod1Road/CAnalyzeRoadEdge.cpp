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
 * @brief �����Ώۃf�[�^�̒��o
 * @param[in] area          �Ώ۔͈�
 * @param[in] multiLines    �|�����C���W��
 * @param bClip             �Ώ۔͈͂ŃN���b�v���邩�ۂ�
 * @return �����Ώۂ̃|�����C���W��
*/
BoostMultiLines CAnalyzeRoadEdge::ExtractTargetData(
    const BoostBox &area,
    const BoostMultiLines &multiLines,
    const bool bClip)
{
    // �͈͓��̃|�����C�����擾
    BoostPolygon areaPolygon;
    bg::convert(area, areaPolygon);
    BoostMultiPolygon polygons;
    polygons.push_back(areaPolygon);
    return ExtractTargetData(polygons, multiLines, bClip);
}

/*!
 * @brief �����Ώۃf�[�^�̒��o
 * @param[in] area          �Ώ۔͈�
 * @param[in] pairLines     �y�A�|�����C���W��
 * @param bClip             �Ώ۔͈͂ŃN���b�v���邩�ۂ�
 * @return �����Ώۂ̃|�����C���W��
*/
std::vector<BoostPairLine> CAnalyzeRoadEdge::ExtractTargetData(
    const BoostBox &area,
    const std::vector<BoostPairLine> &pairLines,
    const bool bClip)
{
    // �͈͓��̃|�����C�����擾
    BoostPolygon areaPolygon;
    bg::convert(area, areaPolygon);

    // �͈͓��̃|�����C�����擾
    std::vector<BoostPairLine>  targetLines;
    for (auto pairLine : pairLines)
    {
        if (bClip)
        {
            // �Ώ۔͈͂ŃN���b�v����ꍇ
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
            // �Ώ۔͈͂ŃN���b�v���Ȃ��ꍇ�́A�d�􂵂Ă���|�����C�����擾����
            // �ǂ��炩�̃|�����C�����Ώ۔͈͂ƏՓ˂��Ă��邩�m�F����
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
 * @brief �����Ώۃf�[�^�̒��o
 * @param[in] area          �Ώ۔͈�
 * @param[in] multiLines    �|�����C���W��
 * @param[in] bClip         �Ώ۔͈͂ŃN���b�v���邩�ۂ�
 * @return �����Ώۂ̃|�����C���W��
*/
BoostMultiLines CAnalyzeRoadEdge::ExtractTargetData(
    const BoostMultiPolygon &area,
    const BoostMultiLines &multiLines,
    const bool bClip)
{
    // �͈͓��̃|�����C�����擾
    BoostMultiLines targetLines;
    if (bClip)
    {
        // �Ώ۔͈͂ŃN���b�v����ꍇ
        bg::intersection(multiLines, area, targetLines);
    }
    else
    {
        // �Ώ۔͈͂ŃN���b�v���Ȃ��ꍇ�́A�d�􂵂Ă���|�����C�����擾����
        for (auto line : multiLines)
        {
            // �a�ł��邩�m�F
            if (!bg::disjoint(area, line))
            {
                targetLines.push_back(line);
            }
        }
    }
    return targetLines;
}

/*!
 * @brief �|�����C�����`����
 * @param[in/out] polyline  ���`�Ώۂ̃|�����C��
 * @param[in]     dLengthTh ���`�m�F�p�̐����̒���臒l(m)
 * @param[in]     dAngleTh  ���`�m�F�p��2�ӂ̊p�x臒l(deg)
*/
void CAnalyzeRoadEdge::shapingRoadEdge(
    BoostPolyline &polyline,
    const double dLengthTh,
    const double dAngleTh)
{
    if (polyline.size() > 2)
    {
        // �I�_���̊m�F
        CVector2D pt1((polyline.end() - 1)->x(), (polyline.end() - 1)->y());
        CVector2D pt2((polyline.end() - 2)->x(), (polyline.end() - 2)->y());
        CVector2D pt3((polyline.end() - 3)->x(), (polyline.end() - 3)->y());
        CVector2D vec1 = pt1 - pt2;
        CVector2D vec2 = pt3 - pt2;
        double dAngle = CGeoUtil::Angle(vec1, vec2);

        if (vec1.Length() < dLengthTh && dAngle <= dAngleTh)
        {
            polyline.erase(polyline.end() - 1);   // �I�_�̍폜
        }
    }

    if (polyline.size() > 2)
    {
        // �n�_���̊m�F
        CVector2D pt1(polyline.begin()->x(), polyline.begin()->y());
        CVector2D pt2((polyline.begin() + 1)->x(), (polyline.begin() + 1)->y());
        CVector2D pt3((polyline.begin() + 2)->x(), (polyline.begin() + 2)->y());
        CVector2D vec1 = pt1 - pt2;
        CVector2D vec2 = pt3 - pt2;
        double dAngle = CGeoUtil::Angle(vec1, vec2);

        if (vec1.Length() < dLengthTh && dAngle <= dAngleTh)
        {
            polyline.erase(polyline.begin());   // �n�_�̍폜
        }
    }
}

/*!
 * @brief ���H���Əd�􂷂�T���Ώې��̎擾
 * @param[in] roadEdges     ���H��
 * @param[in] searchLines   �T���Ώې�
 * @param[in] dBufDist      �o�b�t�@�T�C�Y(m)
 * @param[in] dLengthTh     �s�v���������p�̒���臒l(m)
 * @param[in] dLengthDiffTh �T���Ώۂƒ��o�|�����C���̒�������臒l(m)
 * @return �d�����Q
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
        // ���H���Ɠ��H���ɔ��ʂ̈ʒu������m�F���Ă���
        // ���H���Ƀo�b�t�@��t���ďd���̈���擾�ł���悤�ɂ���
        // �T���Ώۂ��o�b�t�@�����O
        BoostMultiPolygon searchPolygons = CAnalyzeRoadEdgeGeomUtil::Buffering(*itCurrentLine, dBufDist);

        // �T���Ώۂɏd�􂷂铹�H�����擾����
        BoostMultiLines geoms;
        bg::intersection(roadEdges, searchPolygons, geoms);
        if (!bg::is_empty(geoms))
        {
            for (BoostMultiLines::iterator itLine = geoms.begin();
                itLine != geoms.end(); itLine++)
            {
                // �T���Ώې��Ƀo�b�t�@��t�����֌W�ŗ]���ȒZ���������擾���邽�ߏ���
                // �n�I�_�ɐڑ����Ă���G�b�W���]���Ȑ����̏ꍇ�����邽�ߊm�F
                shapingRoadEdge(*itLine, dLengthTh);

                // �T���Ώۂ̒����Ƙ���������|�����C���͏��O����
                double dDiff = abs(bg::length(*itCurrentLine) - bg::length(*itLine));
                if (dDiff > dLengthDiffTh)
                    continue;

                dstLines.push_back(*itLine);  // ���H���Əd�􂷂邷�铹�H��
            }
        }
    }

    return dstLines;
}

/*!
 * @brief ���H���Əd�􂷂�T���Ώې��̎擾
 * @param[in] roadEdges     ���H��
 * @param[in] searchLines   �T���Ώې�
 * @param[in] dBufDist      �o�b�t�@�T�C�Y(m)
 * @param[in] dLengthTh     �s�v���������p�̒���臒l(m)
 * @param[in] dLengthDiffTh �T���Ώۂƒ��o�|�����C���̒�������臒l(m)
 * @return �d�����Q
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

            // �T���Ώۂɏd�􂷂铹�H�����擾����
            BoostMultiLines geoms;
            bg::intersection(roadEdges, searchPolygons, geoms);
            if (!bg::is_empty(geoms))
            {
                for (BoostMultiLines::iterator itLine = geoms.begin();
                    itLine != geoms.end(); itLine++)
                {
                    // �T���Ώې��Ƀo�b�t�@��t�����֌W�ŗ]���ȒZ���������擾���邽�ߏ���
                    // �n�I�_�ɐڑ����Ă���G�b�W���]���Ȑ����̏ꍇ�����邽�ߊm�F
                    shapingRoadEdge(*itLine, dLengthTh);

                    // �T���Ώۂ̒����Ƙ���������|�����C���͏��O����
                    double dDiff = abs(bg::length(*itCurrentLine) - bg::length(*itLine));
                    if (dDiff > dLengthDiffTh)
                        continue;

                    tmpDstLines.push_back(*itLine);  // ���H���Əd�􂷂邷��g���l��
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
 * @brief ���̌����T��
 * @param[in/out]   roadEdges       ���H��
 * @param[in]       bridges         ���H��(���˕�)
 * @param[in]       tunnels         �g���l��
 * @param[out]      upperRoadEdge   ��w�̓��H��
 * @param[out]      middleRoadEdge  ���w�̓��H��
 * @param[out]      lowerRoadEdge   ���w�̓��H��
 * @param[in]       dLengthTh       ���H�����o���̒���臒l(m)
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
    const double dDiffLengthTh = 0.001; // ���H���ƒ��サ�Ă��铹�H��or�g���l������p�̒�������臒l(m)
    const double dBufDist = 0.01;       // �o�b�t�@�T�C�Y(m)
    bg::strategy::buffer::distance_symmetric<double> distStrategy(dBufDist);
    bg::strategy::buffer::join_miter joinStrategy;
    bg::strategy::buffer::end_flat endStrategy;
    bg::strategy::buffer::point_circle pointStrategy;
    bg::strategy::buffer::side_straight sideStrategy;

    middleRoadEdge = BoostMultiLines(roadEdges);

    if (bridges.size() > 0)
    {
        // ���H���Ɠ��H���ɔ��ʂ̈ʒu������m�F
        // ���H���Ƀo�b�t�@��t���ďd���̈���擾�ł���悤�ɂ���
        BoostMultiPolygon bridgePolygons;
        bg::buffer(
            bridges, bridgePolygons, distStrategy, sideStrategy,
            joinStrategy, endStrategy, pointStrategy);

        // ���H���ɏd�􂷂铹�H�����擾����
        BoostMultiLines geoms;
        bg::intersection(roadEdges, bridgePolygons, geoms);
        if (!bg::is_empty(geoms))
        {
            for (BoostMultiLines::iterator itLine = geoms.begin();
                itLine != geoms.end(); itLine++)
            {
                // ���H���Ƀo�b�t�@��t�����֌W�ŗ]���ȒZ���������擾���邽�ߏ���
                // �n�I�_�ɐڑ����Ă���G�b�W���]���Ȑ����̏ꍇ�����邽�ߊm�F
                shapingRoadEdge(*itLine, dLengthTh);

                // �Z���|�����C���͏��O����
                if (bg::length(*itLine) < dLengthTh)
                    continue;

                upperRoadEdge.push_back(*itLine);
            }
        }

        //���H���̑I��
        std::vector<BoostMultiLines::const_iterator> vecDelete;
        for (BoostMultiLines::const_iterator itLine = upperRoadEdge.cbegin();
            itLine != upperRoadEdge.cend(); itLine++)
        {
            // ������̎擾
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
                        // ���H���Əd�􂵂Ă��铹�H���̂��߃X�L�b�v
                        continue;
                    }

                    // ������Ƃ̌�_���擾
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
                                // ���H���̎n�I�_�ȊO�Ō�������������ꍇ
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

    // �g���l���𗘗p�����K�w����
    if (tunnels.size() > 0)
    {
        //�g���l���̑I��
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

                // ������̎擾
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
                            // �g���l���Əd�􂵂Ă��铹�H���̂��߃X�L�b�v
                            continue;
                        }

                        // ������Ƃ̌�_���擾
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
                                    // �g���l���̎n�I�_�ȊO�Ō�������������ꍇ
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
                // ���̌������Ă���g���l���̏ꍇ
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
 * @brief ���H���̃��[�v���������̊O�֊s�␳����
 * @param[in/out]   vecPolyline     �␳�Ώۂ̊J�H���_��
 * @param[in]       outlinePolygon  �␳�Ɏg�p����O�֊s�|���S��
*/
void CAnalyzeRoadEdge::outerRingCorrection(
    std::vector<BoostPoint> &vecPolyline,
    const BoostPolygon &outlinePolygon)
{
    // �T���o�H���H�ł��邩�ۂ�
    bool bClose = CAnalyzeRoadEdgeGeomUtil::CheckPointEqual(
        *vecPolyline.begin(), *(vecPolyline.end() - 1));
    if (!bClose)
    {
        // �J�H�̏ꍇ
        // �f�[�^���E�ɂ��ؒf�m�F
        std::vector<BoostPoint> vecSrc;
        vecSrc.push_back(*vecPolyline.begin());      // �J�H�̎n�_
        vecSrc.push_back(*(vecPolyline.end() - 1));  // �J�H�̏I�_
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
                    // �ڐG����ꍇ
                    targetIdx = static_cast<int>(t);
                }
            }
            vecTargetIdx.push_back(targetIdx);
        }

        if (vecTargetIdx[0] != vecTargetIdx[1])
        {
            // �f�[�^���E�̊O�֊s����̓���ӂŕ��f����Ă��Ȃ�
            // �ׂ荇��2�{�̊O�֊s���Őؒf����Ă��邩�m�F����
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
                    // �|�����C���ƐڐG����O�֊s�̕ӓ��m����������
                    BoostMultiPoints crossPts;
                    bg::intersection(line1, line2, crossPts);
                    if (!bg::is_empty(crossPts))
                        vecPolyline.push_back(crossPts[0]);
                }
            }

            // TODO ����Ă���O�֊s���Őؒf����Ă���ꍇ�����l��
        }
        //else
        //{
        //    // �f�[�^���E�̊O�֊s����̓���ӂŕ��f����Ă��邽�ߎn�I�_���Ȃ��ŕH�ɂ��Ă悢
        //    // ��i������bg::correct�Ŏn�I�_�𓯈�ɂ���␳�����邽�߉������Ȃ�
        //}
    }
}

/*!
 * @brief ���H���̃��[�v��
 * @param[in]  roadEdge         ���H��
 * @param[out] blocks           �X��|���S���Q
 * @param[out] errBlocks        �G���[�X��|���S���Q
 * @param[in]  dAreaTh          �X��̖ʐ�臒l
 * @param[in]  dOpeningBuffer   �I�[�v�j���O�����̃o�b�t�@����
*/
void CAnalyzeRoadEdge::looping(
    BoostMultiLines &roadEdge,
    BoostMultiPolygon &blocks,
    BoostMultiPolygon &errBlocks,
    const double dAreaTh,
    const double dOpeningBuffer)
{
    // ���͓��H���̃o�E���f�B���O�{�b�N�X
    BoostBox bbox;
    bg::envelope(roadEdge, bbox);
    BoostPolygon areaPolygon;
    bg::convert(bbox, areaPolygon); // �|���S����

    // �����O���t�̍쐬
    BoostUndirectedGraph graph = CBoostGraphUtil::CreateGraph(roadEdge);

    // �[���D��T�����g�p���Čo�H�T��
    // �J�H����T��
    std::vector<BoostVertexDesc> vecSearchDesc, vecCloseDesc;
    BOOST_FOREACH (BoostVertexDesc desc, boost::vertices(graph))
    {
        // ���_�̎���
        if (boost::degree(desc, graph) == 1)
        {
            // �J�H�̒[�_�̏ꍇ
            vecSearchDesc.push_back(desc);
        }
        else if (boost::degree(desc, graph) == 2)
        {
            // �H��̓_(����3�ȏ�͗��̌������Ă���ƍl����U���O)
            vecCloseDesc.push_back(desc);
        }
    }

    // �J�H�A�H�̓_�̏��ɐ[���D��T���s��
    vecSearchDesc.insert(vecSearchDesc.end(), vecCloseDesc.begin(), vecCloseDesc.end());
    std::vector<boost::default_color_type> vecVertexColor(boost::num_vertices(graph));
    auto idmap = get(boost::vertex_index, graph);
    auto vcmap = boost::make_iterator_property_map(vecVertexColor.begin(), idmap);
    for (auto it = vecSearchDesc.begin(); it != vecSearchDesc.end(); it++)
    {
        if (graph[*it].isSearched)
            continue;   // �T���ς�
        std::vector<BoostVertexDesc> vecRoute;  // �o�H
        CBoostDFSVisitor vis(vecRoute);
        boost::depth_first_visit(graph, *it, vis, vcmap);

        std::vector<BoostPoint> vecPts;
        for (auto itDesc = vecRoute.begin(); itDesc != vecRoute.end(); itDesc++)
        {
            graph[*itDesc].isSearched = true;   //�T���ς݂ɍX�V
            vecPts.push_back(graph[*itDesc].pt);   // �|���S���쐬�̂��ߒ��_�ǉ�
        }

        // �X��|���S����
        BoostPolygon polygon;
        std::copy(vecPts.begin(), vecPts.end(), std::back_inserter(polygon.outer()));
        bg::unique(polygon);
        bg::correct(polygon);
        // ���Ȍ����̊m�F�Ɖ���
        BoostMultiPolygon newPolygons;
        CAnalyzeRoadEdgeGeomUtil::SelfIntersectionResolution(polygon, newPolygons);
        if (newPolygons.size() > 0)
        {
            // �ʐύő�̃|���S�����擾
            BoostMultiPolygon::iterator itTarget = newPolygons.begin();
            for (auto itTmp = newPolygons.begin(); itTmp != newPolygons.end(); itTmp++)
            {
                if (CEpsUtil::Greater(bg::area(*itTmp), bg::area(*itTarget)))
                {
                    itTarget = itTmp;
                }
            }

            // �X�p�C�N�m�C�Y�΍�
            BoostMultiPolygon openingPolygons = opening(*itTarget, dOpeningBuffer);

            if (openingPolygons.size() > 0
                && bg::is_valid(openingPolygons.front()) &&
                CEpsUtil::GreaterEqual(CAnalyzeRoadEdgeGeomUtil::RoundN(bg::area(openingPolygons.front()), 1), dAreaTh))
                blocks.push_back(openingPolygons.front());
        }
    }
}

/*!
 * @brief �I�[�v�j���O����
 * @param[in] polygon �|���S��
 * @param[in] dBuffer �I�[�v�j���O�����̃o�b�t�@�T�C�Y
 * @return �I�[�v�j���O��̃|���S��
 * @note    ���k�c�����Ɏ��Ȍ�������������ꍇ�����邽�ߎ��Ȍ�����������������
*/
BoostMultiPolygon CAnalyzeRoadEdge::opening(
    BoostPolygon &polygon,
    const double dBuffer)
{
    // �o�b�t�@�����O�X�g���e�W�[
    bg::strategy::buffer::distance_symmetric<double> shrinkDistStrategy(-dBuffer);
    bg::strategy::buffer::distance_symmetric<double> expansionDistStrategy(dBuffer);
    bg::strategy::buffer::join_miter joinStrategy;
    bg::strategy::buffer::end_flat endStrategy;
    bg::strategy::buffer::point_circle pointStrategy;
    bg::strategy::buffer::side_straight sideStrategy;

    //���k���Ɍ����O�֊s���𒴂���ꍇ��buffer�̋��������������Ȃ�(�O�֊s�������������O�֊s�̃|���S���ɂȂ�)���߂̑΍�
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

    // �O�֊s�͎��k�A���͖c��
    BoostMultiPolygon tmpOuter, tmpInners, shrinkPolygons, tmpShrinkPolygons;
    bg::buffer(
        outerPolygon, tmpOuter, shrinkDistStrategy, sideStrategy,
        joinStrategy, endStrategy, pointStrategy);
    bg::unique(tmpOuter);
    bg::correct(tmpOuter);
    // ���k�����ۂɎ��Ȍ��������|���S������������ꍇ�̑Ή�
    double dDiffArea = abs(bg::area(outerPolygon) - bg::area(tmpOuter));
    double dDiffAreaRatio = CAnalyzeRoadEdgeGeomUtil::RoundN(
        dDiffArea / bg::area(outerPolygon), 3);
    if (CEpsUtil::GreaterEqual(dDiffAreaRatio, 0.8))
    {
        // �|���S���������������ߓ��̓|���S���𕪊�����
        // �c���|���S����c��
        BoostMultiPolygon tmpRemaining;
        bg::buffer(
            tmpOuter, tmpRemaining, expansionDistStrategy, sideStrategy,
            joinStrategy, endStrategy, pointStrategy);
        bg::unique(tmpRemaining);
        bg::correct(tmpRemaining);
        // ����
        BoostMultiPolygon outerPolygons;
        bg::difference(outerPolygon, tmpRemaining, outerPolygons);
        outerPolygons.insert(outerPolygons.end(), tmpRemaining.begin(), tmpRemaining.end());
        // �ďk��
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
    // ��������ɑ��݂��钸�_�̍폜(�s�v�_�����݂���Ɩc�����k���Ɏ��Ȍ������������邱�Ƃ��m�F)
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
            // �����|���S���̏ꍇ�͎��Ȍ����̊m�F/����
            BoostMultiPolygon polygons;
            CAnalyzeRoadEdgeGeomUtil::SelfIntersectionResolution(*itShrink, polygons);
            srcErosionPolygons.insert(srcErosionPolygons.end(), polygons.begin(), polygons.end());
        }
    }

    // �c��
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

            // �����|���S���̏ꍇ�͎��Ȍ����̊m�F/����
            BoostMultiPolygon polygons;
            CAnalyzeRoadEdgeGeomUtil::SelfIntersectionResolution(*itErosion, polygons);
            dstPolygons.insert(dstPolygons.end(), polygons.begin(), polygons.end());
        }
    }
    return dstPolygons;
}

/*!
 * @brief �|���S���̌��Z����(�X�p�C�N�m�C�Y/���Ȍ��������@�\�t��)
 * @param[in] polygons          �팸�Z�̈�
 * @param[in] polygon           ���Z�̈�
 * @param[in] dOpeningBuffer    �I�[�v�j���O�����̃o�b�t�@����
 * @return ���Z����
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
            // �Փ˂��Ȃ��ꍇ
            diffPolygons.push_back(*itPoly);
        }
        else
        {
            // �̈�����Z
            BoostMultiPolygon tmpDiff;
            if (bg::within(polygon, *itPoly))
            {
                BoostPolygon diffPolygon(*itPoly);
                diffPolygon.inners().push_back(polygon.outer());
                bg::correct(diffPolygon);
                bg::unique(diffPolygon);

                // �X�p�C�N�m�C�Y�΍�
                BoostMultiPolygon openingPolygons = opening(diffPolygon, dOpeningBuffer);
                diffPolygons.insert(diffPolygons.end(), openingPolygons.begin(), openingPolygons.end());
            }
            else
            {
                bg::difference(*itPoly, polygon, tmpDiff);
                for (auto itDiff = tmpDiff.begin(); itDiff != tmpDiff.end(); itDiff++)
                {
                    // �X�p�C�N�m�C�Y�΍�
                    BoostMultiPolygon openingPolygons = opening(*itDiff, dOpeningBuffer);
                    diffPolygons.insert(diffPolygons.end(), openingPolygons.begin(), openingPolygons.end());
                }
            }
        }
    }

    return diffPolygons;
}

/*!
 * @brief ���H�|���S���쐬
 * @param[in]   roadedges           ���H��
 * @param[in]   blocks              �X��|���S���Q
 * @param[out]  roadPolygons        ���H�|���S���Q
 * @param[out]  concaveHull         �����
 * @param[in]   dSampling           ���H���̃T���v�����O�Ԋu
 * @param[in]   dAreaTh             ���H�|���S���̖ʐ�臒l
 * @param[in]   dOpeningBuffer      �I�[�v�j���O�����̃o�b�t�@�T�C�Y
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
    const double dAreaEpsilon = 0.01;   // �ʐό덷���e�l(��)
    const double dConcavity = 2.25;     // ����̕��G�x

    // ����p�ɃT���v�����O
    BoostMultiLines samplingEdges = CAnalyzeRoadEdgeGeomUtil::Sampling(roadedges, dSampling);

    // ����
    concaveHull = CConcaveHull::ConcaveHull(samplingEdges, dConcavity);

    // ���H�|���S���̍쐬
    BoostPolygon roadPolygon(concaveHull);
    BoostMultiPolygon tmpSrcRoadPolygons = opening(roadPolygon, dOpeningBuffer);

    // ���Z�Ώۗ̈�̎擾
    BoostMultiPolygon subtrahend;
    for (BoostMultiPolygon::const_iterator it = blocks.cbegin();
        it != blocks.cend(); it++)
    {
        BoostMultiPolygon andRegion;
        bg::intersection(tmpSrcRoadPolygons, *it, andRegion);

        for (auto itAndRegion = andRegion.begin(); itAndRegion != andRegion.end(); itAndRegion++)
        {
            // �X�p�C�N�m�C�Y�΍�
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
    // �O�֊s�����牓���X�悩�猸�Z����悤�Ƀ\�[�g
    BoostPolyline outer;
    std::copy(concaveHull.outer().begin(), concaveHull.outer().end(), std::back_inserter(outer));
    std::sort(subtrahend.begin(), subtrahend.end(), [outer](auto const &a, auto const &b) {
            return bg::distance(outer, a) > bg::distance(outer, b);
        });

    // ���Z����
    for (auto it = subtrahend.begin(); it != subtrahend.end(); it++)
    {
        BoostMultiPolygon diffPolygons = difference(tmpSrcRoadPolygons, *it, dOpeningBuffer);

        // ���Z���ꂽ�ʐ�
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
 * @brief shp�o�͏����p�̒��ڃG���A�ƋߖT�G���A�̍쐬
 * @param[in]   nRow        ���̓G���A�̕����s��
 * @param[in]   nColumn     ���̓G���A�̕�����
 * @param[in]   dProcWidth  ���ڃG���A�̕�(m)
 * @param[in]   dProcHeight ���ڃG���A�̍���(m)
 * @param[in]   dInputMinX  ���̓G���A�̍ŏ�x���W
 * @param[in]   dInputMinY  ���̓G���A�̍ŏ�y���W
 * @param[in]   nX          ���ڃG���A�̍s��
 * @param[in]   nY          ���ڃG���A�̗�
 * @param[[out] areas       ���ڃG���A�ƋߖT�G���A�̃|���S���Q
 * @param[[out] nTargetIdx  ���ڃG���A�|���S���̃C���f�b�N�X
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
            bg::convert(area, areaPolygon); // �|���S����

            if (nX == nTempX && nY == nTempY)
                nTargetIdx = static_cast<int>(areas.size());    // ���ڔ͈͂̃C���f�b�N�X��ێ�
            areas.push_back(areaPolygon);
        }
    }
}

/*!
 * @brief �ʐϐ�L���̊m�F
 * @param[in] target    ���ڃ|���S��
 * @param[in] areas     �͈̓|���S���Q
 * @return    �ʐϐ�L�����ő�͈̔̓|���S���̃C���f�b�N�X
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

        // �d���̈�̎擾
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
 * @brief ���ڃG���A�̃|���S�����擾����
 * @param[in]   roadData    ���H���Q
 * @param[in]   nRow        ���̓G���A�̕����s��
 * @param[in]   nColumn     ���̓G���A�̕�����
 * @param[in]   dProcWidth  ���ڃG���A�̕�(m)
 * @param[in]   dProcHeight ���ڃG���A�̍���(m)
 * @param[in]   dInputMinX  ���̓G���A�̍ŏ�x���W
 * @param[in]   dInputMinY  ���̓G���A�̍ŏ�y���W
 * @param[in]   nX          ���ڃG���A�̍s��
 * @param[in]   nY          ���ڃG���A�̗�
 * @param[out]  dstData     ���ڃG���A�̓��H�f�[�^�Q
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
    // ���ڔ͈͂ƋߖT�͈͎Z�o
    BoostMultiPolygon areas;
    int nTargetIdx;
    createAreas(nRow, nColumn, dProcWidth, dProcHeight,
        dInputMinX, dInputMinY, nX, nY, areas, nTargetIdx);

    if (nTargetIdx > -1)
    {
        BoostPolygon targetArea = areas[nTargetIdx];   // ���ڔ͈�

        // �I��
        for (auto itData = roadData.begin(); itData != roadData.end(); itData++)
        {
            if (bg::within(itData->Polygon(), targetArea))
            {
                // ���ڔ͈͂ɓ�����ꍇ
                dstData.push_back(*itData);
            }
            else
            {

                // ���ڔ͈͂ɓ����Ȃ��ꍇ
                if (bg::disjoint(itData->Polygon(), targetArea))
                    continue;   // ���ڔ͈͂ƏՓ˂��Ȃ��ꍇ��skip

                // �ʐϐ�L���̊m�F
                int nIdx = checkAreaOccupancyRate(itData->Polygon(), areas);
                if (nIdx == nTargetIdx)
                {
                    // ���ڔ͈͂��ő�ʐϐ�L���̏ꍇ
                    dstData.push_back(*itData);
                }
            }
        }
    }
}

/*!
 * @brief ���ڃG���A�̃|���S�����擾����
 * @param[in]   polygons    ���H�|���S���Q(�ԓ�������,���H�\���ω��̈���������|���S���Q)
 * @param[in]   nRow        ���̓G���A�̕����s��
 * @param[in]   nColumn     ���̓G���A�̕�����
 * @param[in]   dProcWidth  ���ڃG���A�̕�(m)
 * @param[in]   dProcHeight ���ڃG���A�̍���(m)
 * @param[in]   dInputMinX  ���̓G���A�̍ŏ�x���W
 * @param[in]   dInputMinY  ���̓G���A�̍ŏ�y���W
 * @param[in]   nX          ���ڃG���A�̍s��
 * @param[in]   nY          ���ڃG���A�̗�
 * @param[out]  dstData     ���ڃG���A�̓��H�f�[�^�Q
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
    // ���ڔ͈͂ƋߖT�͈͎Z�o
    BoostMultiPolygon areas;
    int nTargetIdx;
    createAreas(nRow, nColumn, dProcWidth, dProcHeight,
        dInputMinX, dInputMinY, nX, nY, areas, nTargetIdx);

    if (nTargetIdx > -1)
    {
        BoostPolygon targetArea = areas[nTargetIdx];   // ���ڔ͈�

        // �I��
        for (auto itPoly = polygons.begin(); itPoly != polygons.end(); itPoly++)
        {
            if (bg::within(*itPoly, targetArea))
            {
                // ���ڔ͈͂ɓ�����ꍇ
                CRoadData data;
                data.Polygon(*itPoly);
                dstData.push_back(data);
            }
            else
            {
                // ���ڔ͈͂ɓ����Ȃ��ꍇ
                if (bg::disjoint(*itPoly, targetArea))
                    continue;   // ���ڔ͈͂ƏՓ˂��Ȃ��ꍇ��skip

                if (itPoly->inners().size() == 0)
                {
                    // �����o���Ă���|���S���͖ʐϐ�L�����m�F����
                    int nIdx = checkAreaOccupancyRate(*itPoly, areas);
                    if (nIdx == nTargetIdx)
                    {
                        // ���ڔ͈͂��ő�ʐϐ�L���̏ꍇ
                        CRoadData data;
                        data.Polygon(*itPoly);
                        dstData.push_back(data);
                    }
                }
                else
                {
                    // ���H�����̐��x���ǂ��Ȃ�����
                    // �������Ă��関�����̃|���S���͒��ڗ̈�ŃN���b�v���ďo�͂���
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
 * @brief ���H����͏���
 * @param[out]  errMsg          �G���[���f�����b�Z�[�W
 * @param[in]   targetRoadEdges ���H��
 * @param[in]   targetBridges   ���H��
 * @param[in]   targetTunnels   �g���l��
 * @param[in]   targetProcArea  ���ڗ̈�
 * @param[in]   dInputMinX      ���H���̍ŏ�x���W
 * @param[in]   dInputMinY      ���H���̍ŏ�y���W
 * @param[in]   dProcWidth      ���ڗ̈敝(m)
 * @param[in]   dProcHeight     ���ڗ̈捂��(m)
 * @param[in]   nRow            �̈�s��
 * @param[in]   nColumn         �̈��
 * @param[in]   nX              ���ڗ̈�s��
 * @param[in]   nY              ���ڗ̈��
 * @return      ���H�|���S���f�[�^�Q
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
    std::vector<CRoadData> vecOutputRoadData; // ���H�|���S���f�[�^

#ifdef _OUTPUT_MODEL_INTERMEDIATE_FILE
    CAnalyzeRoadEdgeDebugUtil debugUtil = CAnalyzeRoadEdgeDebugUtil();
    // ���H��
    std::string strShpName = (boost::format("%03d_%03d_01_roadedge.shp") % nX % nY).str();
    std::string strShpPath = CFileUtil::Combine(m_strDebugFolderPath, strShpName);
    debugUtil.OutputPolylinesToShp(targetRoadEdges, strShpPath);

    // ���ˋ�
    strShpName = (boost::format("%03d_%03d_02_bridge.shp") % nX % nY).str();
    strShpPath = CFileUtil::Combine(m_strDebugFolderPath, strShpName);
    debugUtil.OutputPolylinesToShp(targetBridges, strShpPath);

#endif

    // ���̌����T��
    BoostMultiLines upperRoadEdges, middleRoadEdges;
    std::vector<BoostPairLine> lowerRoadEdges;
    searchMultiLevelCrossing(
        targetRoadEdges, targetBridges, targetTunnels,
        upperRoadEdges, middleRoadEdges, lowerRoadEdges);

#ifdef _OUTPUT_MODEL_INTERMEDIATE_FILE
    // ���w
    strShpName = (boost::format("%03d_%03d_03_middle.shp") % nX % nY).str();
    strShpPath = CFileUtil::Combine(m_strDebugFolderPath, strShpName);
    debugUtil.OutputPolylinesToShp(middleRoadEdges, strShpPath);

    // ��w
    strShpName = (boost::format("%03d_%03d_03_upper.shp") % nX % nY).str();
    strShpPath = CFileUtil::Combine(m_strDebugFolderPath, strShpName);
    debugUtil.OutputPolylinesToShp(upperRoadEdges, strShpPath);

    // ���w
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

    // ���H���̃��[�v��
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
    bg::convert(bbox, areaPolygon); // �|���S����
    BoostMultiPolygon boundingBox;
    boundingBox.push_back(areaPolygon);
    strShpName = (boost::format("%03d_%03d_04_bbox.shp") % nX % nY).str();
    strShpPath = CFileUtil::Combine(m_strDebugFolderPath, strShpName);
    debugUtil.OutputPolygonsToShp(boundingBox, strShpPath);
#endif

    // ���H�|���S���̍쐬
    BoostMultiPolygon roadPolygons;
    BoostPolygon concaveHull;
    createRoadPolygon(middleRoadEdges, middlePolygons, roadPolygons, concaveHull);

#ifdef _OUTPUT_MODEL_INTERMEDIATE_FILE
    // �����
    BoostMultiPolygon polygons;
    polygons.push_back(concaveHull);
    strShpName = (boost::format("%03d_%03d_05_concavehull_middle.shp") % nX % nY).str();
    strShpPath = CFileUtil::Combine(m_strDebugFolderPath, strShpName);
    debugUtil.OutputPolygonsToShp(polygons, strShpPath);
    //���H�|���S��
    strShpName = (boost::format("%03d_%03d_05_diff_middle.shp") % nX % nY).str();
    strShpPath = CFileUtil::Combine(m_strDebugFolderPath, strShpName);
    debugUtil.OutputPolygonsToShp(roadPolygons, strShpPath, true);
#endif

    // ���H���S���̍쐬
    CRoadCenterLine centerLineCreator;
    centerLineCreator.CreateCenterLine(roadPolygons, 0.5, -0.025);
    BoostMultiLines roadCenterLines = centerLineCreator.CenterLines();

#ifdef _OUTPUT_MODEL_INTERMEDIATE_FILE
    // ���`�O���H���S��
    strShpName = (boost::format("%03d_%03d_06_orgRoadCenter.shp") % nX % nY).str();
    strShpPath = CFileUtil::Combine(m_strDebugFolderPath, strShpName);
    debugUtil.OutputPolylinesToShp(centerLineCreator.InternalCenterLines(), strShpPath);

    //���H�|���S��
    strShpName = (boost::format("%03d_%03d_06_shrink_middle.shp") % nX % nY).str();
    strShpPath = CFileUtil::Combine(m_strDebugFolderPath, strShpName);
    debugUtil.OutputPolygonsToShp(centerLineCreator.ShrinkRoadPolygons(), strShpPath, true);

    // ���H���S��
    strShpName = (boost::format("%03d_%03d_06_dstRoadCenter.shp") % nX % nY).str();
    strShpPath = CFileUtil::Combine(m_strDebugFolderPath, strShpName);
    debugUtil.OutputPolylinesToShp(centerLineCreator.CenterLines(), strShpPath);

    // �����_(�ߖT�̈�܂�)
    BoostMultiPoints crossingPts = centerLineCreator.GetCrossPoints();
    strShpName = (boost::format("%03d_%03d_06_crossing.shp") % nX % nY).str();
    strShpPath = CFileUtil::Combine(m_strDebugFolderPath, strShpName);
    debugUtil.OutputMultiPointsToShp(crossingPts, strShpPath);

#endif
    // �ԓ��������̕���
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
        // �{���m�C�̈�
        if (!bg::is_empty(it->Cell()) && bg::is_valid(it->Cell()))
            voronoiPolygons.push_back(it->Cell());

        // �����_�̈�
        if (!bg::is_empty(it->Area()) && bg::is_valid(it->Area()))
            areas.push_back(it->Area());

        // �����Ώۂ̌����_
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

    // �ԓ��������|���S��
    BoostMultiPolygon crossingPolygons;
    for (auto itRoad = roads.begin(); itRoad != roads.end(); itRoad++)
        crossingPolygons.push_back(itRoad->Polygon());
    strShpName = (boost::format("%03d_%03d_07_crossingPolygons.shp") % nX % nY).str();
    strShpPath = CFileUtil::Combine(m_strDebugFolderPath, strShpName);
    debugUtil.OutputPolygonsToShp(crossingPolygons, strShpPath);

    // �c�����H�|���S��
    strShpName = (boost::format("%03d_%03d_07_remainingPolygons.shp") % nX % nY).str();
    strShpPath = CFileUtil::Combine(m_strDebugFolderPath, strShpName);
    debugUtil.OutputPolygonsToShp(remainingPolygons, strShpPath, true);
#endif

    // ���H�\���ω��ɂ�镪��
    // ���H�|���S�����瓹�H���𒊏o(�␳���Ă���֌W�Ō��f�[�^�̓��H���Ɣ����Ɍ`��ɍ��ق�����)
    BoostMultiLines newRoadEdges = CAnalyzeRoadEdgeGeomUtil::GetEdges(roadPolygons);

    // ���ڗ̈�ɓ����铹�H��/�g���l���ƒ��ڗ̈拫�E�ɂ܂����铹�H/�g���l�����𒊏o
    BoostMultiLines selectBridges = ExtractTargetData(targetProcArea, targetBridges);
    std::vector<BoostPairLine> selectTunnels = ExtractTargetData(targetProcArea, targetTunnels);

    // ���H���Əd�􂷂铹�H��,�g���l���̎擾
    BoostMultiLines overlapBridgeds;
    std::vector<BoostPairLine> overlapTunnels;
    overlapBridgeds = getOverlapEdges(newRoadEdges, selectBridges, 0.2, 0.25, 1.0);
    overlapTunnels = getOverlapEdges(newRoadEdges, selectTunnels, 0.2, 0.25, 1.0);

#ifdef _OUTPUT_MODEL_INTERMEDIATE_FILE
    // ���H���Əd�􂷂鍂�ˋ�
    strShpName = (boost::format("%03d_%03d_08_bridge.shp") % nX % nY).str();
    strShpPath = CFileUtil::Combine(m_strDebugFolderPath, strShpName);
    debugUtil.OutputPolylinesToShp(overlapBridgeds, strShpPath);

    // ���H���Əd�􂷂�g���l��
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

    // �����ɂ�镪��
    std::vector<CRoadData> bridgeData;
    BoostMultiPolygon bridgeRemainingPolygons;
    roadDivision.DivisionByStructualChange(
        remainingPolygons, overlapBridgeds,
        RoadSectionType::ROAD_SECTION_BRIDGE, bridgeData, bridgeRemainingPolygons);

    // �g���l���ɂ�镪��
    std::vector<CRoadData> tunnelData;
    BoostMultiPolygon tunnelRemainingPolygons;
    roadDivision.DivisionByStructualChange(
        bridgeRemainingPolygons, overlapTunnels,
        RoadSectionType::ROAD_SECTION_TUNNEL, tunnelData, tunnelRemainingPolygons);

#ifdef _OUTPUT_MODEL_INTERMEDIATE_FILE
    // �����|���S��
    BoostMultiPolygon bridgePolygons;
    for (auto itBridge = bridgeData.begin(); itBridge != bridgeData.end(); itBridge++)
        bridgePolygons.push_back(itBridge->Polygon());
    strShpName = (boost::format("%03d_%03d_08_bridgePolygons.shp") % nX % nY).str();
    strShpPath = CFileUtil::Combine(m_strDebugFolderPath, strShpName);
    debugUtil.OutputPolygonsToShp(bridgePolygons, strShpPath);

    // �c�����H�|���S��
    strShpName = (boost::format("%03d_%03d_08_bridgeRemainingPolygons.shp") % nX % nY).str();
    strShpPath = CFileUtil::Combine(m_strDebugFolderPath, strShpName);
    debugUtil.OutputPolygonsToShp(bridgeRemainingPolygons, strShpPath, true);

    // �g���l���|���S��
    BoostMultiPolygon tunnelPolygons;
    for (auto itTunnel = tunnelData.begin(); itTunnel != tunnelData.end(); itTunnel++)
        tunnelPolygons.push_back(itTunnel->Polygon());
    strShpName = (boost::format("%03d_%03d_08_tunnelPolygons.shp") % nX % nY).str();
    strShpPath = CFileUtil::Combine(m_strDebugFolderPath, strShpName);
    debugUtil.OutputPolygonsToShp(tunnelPolygons, strShpPath);

    // �c�����H�|���S��
    strShpName = (boost::format("%03d_%03d_08_tunnelRemainingPolygons.shp") % nX % nY).str();
    strShpPath = CFileUtil::Combine(m_strDebugFolderPath, strShpName);
    debugUtil.OutputPolygonsToShp(tunnelRemainingPolygons, strShpPath, true);
#endif

    if (roads.size() > 0)
    {
        // ���ڔ͈͂̃f�[�^�𒊏o
        selectPolygon(roads, nRow, nColumn, dProcWidth, dProcHeight,
            dInputMinX, dInputMinY, nX, nY, vecOutputRoadData);   // �ԓ�������
        selectPolygon(bridgeData, nRow, nColumn, dProcWidth, dProcHeight,
            dInputMinX, dInputMinY, nX, nY, vecOutputRoadData);   // ����
        selectPolygon(tunnelData, nRow, nColumn, dProcWidth, dProcHeight,
            dInputMinX, dInputMinY, nX, nY, vecOutputRoadData);   // �g���l��
        selectPolygon(tunnelRemainingPolygons, nRow, nColumn, dProcWidth, dProcHeight,
            dInputMinX, dInputMinY, nX, nY, vecOutputRoadData);   // ���̑����H

#ifdef _OUTPUT_MODEL_INTERMEDIATE_FILE
        // ���ڔ͈̓|���S��
        BoostMultiPolygon results;
        for (auto itPoly = vecOutputRoadData.begin(); itPoly != vecOutputRoadData.end(); itPoly++)
            results.push_back(itPoly->Polygon());
        strShpName = (boost::format("%03d_%03d_08_results.shp") % nX % nY).str();
        strShpPath = CFileUtil::Combine(m_strDebugFolderPath, strShpName);
        debugUtil.OutputPolygonsToShp(results, strShpPath, true);
#endif

        // �G���[�`�F�b�N
        errMsg = errorCheck(
            vecOutputRoadData, roadPolygons, crossing, targetProcArea);
    }
    else
    {
        // ���H�����Ɏ��s�����ꍇ�A���ڗ̈�̓��H�|���S�����o��
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
 * @brief �G���[�`�F�b�N
 * @param[in] roadData      ������̓��H�|���S���f�[�^
 * @param[in] roadPolygons  �����O�̓��H�|���S��
 * @param[in] crosses       �����_���W�Q
 * @param[in] targetArea    ���ڃG���A
 * @return    �G���[���b�Z�[�W
*/
std::vector<std::vector<std::string>> CAnalyzeRoadEdge::errorCheck(
    std::vector<CRoadData> &roadData,
    BoostMultiPolygon &roadPolygons,
    std::vector<CCrossingData> &crosses,
    BoostBox &targetArea)
{
    // �����㓹�H�|���S�����G���[�`�F�b�N�p�f�[�^�ɕϊ�
    std::vector<CreatedRoadModelInfo> modelInfos;
    for (auto itData = roadData.begin(); itData != roadData.end(); itData++)
    {
        if (itData->Polygon().inners().size() == 0) // ���Ȃ��̂�
        {
            CreatedRoadModelInfo info(*itData, true);
            modelInfos.push_back(info);
        }
    }

    // �����O�|���S���𒍖ڗ̈�ŃN���b�v
    BoostMultiPolygon targetPolygons;
    bg::intersection(roadPolygons, targetArea, targetPolygons);

    // ���ڗ̈��(���E�܂�)�̌����_���W���擾
    BoostMultiPoints targetCrosses;
    for (auto itCross = crosses.begin(); itCross != crosses.end(); itCross++)
    {
        if (bg::covered_by(itCross->Point(), targetArea))
            targetCrosses.push_back(itCross->Point());
    }

    // �G���[�`�F�b�N
    RoadModelData modelData(modelInfos, targetPolygons, targetCrosses);
    std::vector<std::vector<std::string>> errMsg = RoadModelErrorChecker::Run(
        modelData, GetCreateParam()->GetMinArea(), GetCreateParam()->GetMaxDistance());
    return errMsg;
}


/*!
 * @brief �����̈搔�̎Z�o
 * @param[in] roadEdges     ���H���W���f�[�^
 * @param[in] dProcWidth    �����͈͕�(m)
 * @param[in] dProcHeight   �����͈͍���(m)
 * @param[out] nRow         �����͈͍s��
 * @param[out] nColumn      �����͈͗�
 * @param[out] dMinX        �����͈͂̍ŏ�x���W
 * @param[out] dMinY        �����͈͂̍ŏ�y���W
*/
void CAnalyzeRoadEdgeManager::calcProcNum(
    const BoostMultiLines &roadEdges,
    const double dProcWidth,
    const double dProcHeight,
    int &nRow, int &nColumn,
    double &dMinX, double &dMinY)
{
    // ���͔͈͂̎擾
    BoostBox inputArea;
    bg::envelope(roadEdges, inputArea);
    dMinX = inputArea.min_corner().x();
    dMinY = inputArea.min_corner().y();
    double dInputAreaWidth = abs(inputArea.max_corner().x() - inputArea.min_corner().x());
    double dInputAreaHeight = abs(inputArea.max_corner().y() - inputArea.min_corner().y());

    assert(dProcWidth > 0 && dProcHeight > 0);
    // �����̈�ɕ��������ۂɍs��
    nRow = static_cast<int>(ceil(dInputAreaHeight / dProcHeight));
    nColumn = static_cast<int>(ceil(dInputAreaWidth / dProcWidth));
}

/*!
 * @brief ���H�|���S���o��
 * @param[in] strShpPath    �o��shp�t�@�C���p�X
 * @param[in] polygons      �|���S���f�[�^�Q
 * @param[in] bHole         ���̕t�^�t���O
 * @return  ��������
 * @retval  true            ����
 * @retval  false           ���s
*/
bool CAnalyzeRoadEdgeManager::outputRoadPolygons(
    const std::string &strShpPath,
    std::vector<CRoadData> polygons,
    const bool bHole)
{
    CShapeWriter writer;
    std::vector<CShapeAttribute::AttributeFieldData> vecFields;        // ������`
    std::vector<CShapeAttribute::AttributeDataRecord> vecAttrRecords;  // ����
    BoostMultiPolygon multiPolygon; // �|���S���Q

    // ������`
    // ���H�`�󑮐�(uro:RoadStructureAttribute��uro:sectionType�ɊY��)
    CShapeAttribute::AttributeFieldData field;
    field.fieldType = CShapeAttribute::AttributeFieldType::ATTR_FIELD_TYPE_INT;
    field.strName = "sectType";
    field.nWidth = 4;
    vecFields.push_back(field);

    // �o�͗p�f�[�^�쐬
    int nId = 0;
    for (auto it = polygons.begin(); it != polygons.end(); it++, nId++)
    {
        // �`����
        multiPolygon.push_back(it->Polygon());
        // �������
        CShapeAttribute::AttributeDataRecord record;
        record.nShapeId = nId;
        if (it->Type() == RoadSectionType::ROAD_SECTION_UNKNOWN)
        {
            record.vecAttribute.push_back(CShapeAttribute::AttributeData());
        }
        else
        {
            // �ԓ�������,����,�g���l���Ȃ�
            record.vecAttribute.push_back(CShapeAttribute::AttributeData(static_cast<int>(it->Type())));
        }
        vecAttrRecords.push_back(record);
    }

    // output shape file
    return writer.OutputPolygons(multiPolygon, strShpPath, vecFields, vecAttrRecords, bHole);
}

/*!
 * @brief �}���`�X���b�h�p�̃��f����������
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

    // �p�����[�^�擾
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
            // ���̈�̎擾�Ɏ��s
            break;
        }
        int procIdx = region.second * nColumn + region.first + 1;
        startAnalysis(procIdx, nRow * nColumn);
        int nX = region.first;
        int nY = region.second;

#ifdef _EIGHT_NEIGHBORS_REGION
        // 8�ߖT�͈͂��擾����(�c����)
        int nPrevY = (nY > 0) ? nY - 1 : nY;
        int nNextY = (nY < nRow - 1) ? nY + 1 : nY;
        double dMinY = dProcHeight * static_cast<double>(nPrevY) + dInputMinY;
        double dMaxY = dProcHeight * static_cast<double>(nNextY) + dInputMinY + dProcHeight;
#else
        // �ߖT���݂͈̔�(�c�����j
        double dMinY = dProcHeight * static_cast<double>(nY) + dInputMinY - dProcBuffer;
        double dMaxY = dProcHeight * static_cast<double>(nY) + dInputMinY + dProcHeight + dProcBuffer;
#endif
        // ���ڔ͈͂��擾����(�c����)
        double dTargetMinY = dProcHeight * static_cast<double>(nY) + dInputMinY;
        double dTargetMaxY = dTargetMinY + dProcHeight;

#ifdef _EIGHT_NEIGHBORS_REGION
        // 8�ߖT�͈͂��擾����(������)
        int nPrevX = (nX > 0) ? nX - 1 : nX;
        int nNextX = (nX < nColumn - 1) ? nX + 1 : nX;
        double dMinX = dProcWidth * static_cast<double>(nPrevX) + dInputMinX;
        double dMaxX = dProcWidth * static_cast<double>(nNextX) + dInputMinX + dProcWidth;
#else
        // �ߖT���݂͈̔�(�������j
        double dMinX = dProcWidth * static_cast<double>(nX) + dInputMinX - dProcBuffer;
        double dMaxX = dProcWidth * static_cast<double>(nX) + dInputMinX + dProcWidth + dProcBuffer;
#endif
        // �ߖT�͈�
        BoostBox procArea = BoostBox(BoostPoint(dMinX, dMinY), BoostPoint(dMaxX, dMaxY));

        // ���ڔ͈͂��擾����(������)
        double dTargetMinX = dProcWidth * static_cast<double>(nX) + dInputMinX;
        double dTargetMaxX = dTargetMinX + dProcWidth;
        // ���ڔ͈�
        BoostBox procTargetArea = BoostBox(BoostPoint(
            dTargetMinX, dTargetMinY), BoostPoint(dTargetMaxX, dTargetMaxY));

#ifdef _OUTPUT_MODEL_INTERMEDIATE_FILE

        CAnalyzeRoadEdge analyzer(strDebugFolderPath);
#else
        CAnalyzeRoadEdge analyzer;
#endif

        // �ߖT�͈͓��̃f�[�^���擾
        BoostMultiLines targetRoadEdges, targetBridges;
        std::vector<BoostPairLine> targetTunnels;

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            targetRoadEdges = analyzer.ExtractTargetData(procArea, m_roadEdges, true);  // ���H��
            targetBridges = analyzer.ExtractTargetData(procArea, m_bridges);            // ���H��
            targetTunnels = analyzer.ExtractTargetData(procArea, m_tunnels);            // �g���l��
        }

        BoostMultiLines tmpLines;
        bg::intersection(procTargetArea, targetRoadEdges, tmpLines);    // ���ڔ͈͓��̓��H��
        if (tmpLines.size() == 0)
        {
            // ���H�������݂��Ȃ��ꍇ��skip
            stopAnalysis(procIdx, nRow * nColumn, false);
            continue;
        }

        try
        {
            // ���H�����
            std::vector<std::vector<std::string>> errMsg;
            std::vector<CRoadData> vecOutputRoadData = analyzer.Process(
                errMsg, targetRoadEdges, targetBridges, targetTunnels, procTargetArea,
                dInputMinX, dInputMinY, dProcWidth, dProcHeight,
                nRow, nColumn, nX, nY);

            // ���ʂ̊i�[
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
 * @brief ���H����͂̊J�n���O
 * @param[in] nTarget ���ڗ̈�̃C���f�b�N�X�ԍ�
 * @param[in] nTotal  ��͑Ώۂ̑���
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
 * @brief ���H����͂̏I�����O
 * @param[in] nTarget ���ڗ̈�̃C���f�b�N�X�ԍ�
 * @param[in] nTotal  ��͑Ώۂ̑���
 * @param[in] bError  �G���[�t���O
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
 * @brief ���H�����
 * @param[in] vecRoadEdges  ���H���f�[�^
 * @param[in] vecBridges    ���H���f�[�^
 * @param[in] vecTunnels    �g���l���f�[�^
 * @param[in] dProcWidth    ��͏����͈͂̕�(m)
 * @param[in] dProcHeight   ��͏����͈͂̍���(m)
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

    // �f�[�^�ϊ�
    m_roadEdges = CAnalyzeRoadEdgeGeomUtil::ConvBoostMultiLines(vecRoadEdges);
    m_bridges = CAnalyzeRoadEdgeGeomUtil::ConvBoostMultiLines(vecBridges);
    m_tunnels = vecTunnels; // �X���b�h�����p�Ƀ����o�ϐ��ɐݒ�


    // �����͈͐��̎Z�o
    int nRow, nColumn;      // �s��
    double dInputMinX, dInputMinY;    // ���͓��H���̃o�E���f�B���O�{�b�N�̍ŏ����W
    calcProcNum(m_roadEdges, dProcWidth, dProcHeight, nRow, nColumn, dInputMinX, dInputMinY);

#ifdef _OUTPUT_MODEL_INTERMEDIATE_FILE
    CAnalyzeRoadEdgeDebugUtil debugUtil = CAnalyzeRoadEdgeDebugUtil();
    // ���ݎ���
    std::string strTime = CTime::GetCurrentTime().Format("%Y%m%d_%H%M%S");
    // ��ƃf�B���N�g���p�X
    std::string strCurrentPath = CAnalyzeRoadEdgeDebugUtil::GetCurrentPath();
    // debug folder
    std::string strDebugFolderPath = CFileUtil::Combine(strCurrentPath, strTime);
    CAnalyzeRoadEdgeDebugUtil::CreateFolder(strDebugFolderPath);

    // �����͈͂̉���
    std::string strProcAreaShpPath = CFileUtil::Combine(strDebugFolderPath, "proc_area.shp");
    debugUtil.OutputProcArea(
        strProcAreaShpPath, dInputMinX, dInputMinY, nRow, nColumn, dProcWidth, dProcHeight);

    m_strDebugFolderPath = strDebugFolderPath;  // �X���b�h�p�̐ݒ�
#endif

    // �X���b�h�p�̐ݒ�
    m_dInputMinX = dInputMinX;
    m_dInputMinY = dInputMinY;
    m_dProcWidth = dProcWidth;
    m_dProcHeight = dProcHeight;
    m_nRow = nRow;
    m_nColumn = nColumn;

    // �T���Ώۂ�queue�ɋl�߂�
    for (int nY = 0; nY < nRow; nY++)
    {
        for (int nX = 0; nX < nColumn; nX++)
        {
            std::pair<int, int> region(nX, nY);
            m_regions.push(region);
        }
    }

    // thread�쐬
    std::vector<std::thread> threads;
    for (int n = 0; n < nThread; n++)
    {
        threads.push_back(std::thread(&CAnalyzeRoadEdgeManager::analyze, this));
    }

    // thread�I���҂�
    for (auto &th : threads)
    {
        th.join();
    }
}

/*!
 * @brief ���H�|���S���o��
 * @return ��������
 * @retval true     ����
 * @retval false    ���s
*/
bool CAnalyzeRoadEdgeManager::OutputResultFile()
{
    // �G���[���f���m�F����
    RoadModelErrorChecker::SaveErr(m_errMsg, GetOutputSetting()->GetErrFilePath());

    // �|���S���o��
    outputRoadPolygons(
        GetOutputSetting()->GetShpFilePathWithHoles(),
        m_vecOutputRoadData, true);     // ���L��

    return outputRoadPolygons(
        GetOutputSetting()->GetShpFilePath(),
        m_vecOutputRoadData, false);    // ������
}