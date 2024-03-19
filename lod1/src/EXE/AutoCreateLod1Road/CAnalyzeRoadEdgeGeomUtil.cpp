#include "pch.h"
#include "CAnalyzeRoadEdgeGeomUtil.h"
#include "CEpsUtil.h"
#include <ppl.h>
#include <mutex>

/*!
 * @brief boost�p���_�̓�����r
 * @param[in] pt1 ���_1
 * @param[in] pt2 ���_2
 * @return ������r����
 * @retval true     ������
 * @retval false    �������Ȃ�
 */
bool CAnalyzeRoadEdgeGeomUtil::CheckPointEqual(const BoostPoint &pt1, const BoostPoint &pt2)
{
    bool bEqual = false;
    if (CEpsUtil::Equal(pt1.x(), pt2.x())
        && CEpsUtil::Equal(pt1.y(), pt2.y()))
    {
        bEqual = true;  // ������
    }
    return bEqual;
}

/*!
 * @brief       boost�p�|�����C���f�[�^�ϊ�
 * @param[im]   vecSrcPolyline ���̓|�����C���f�[�^
 * @return      boost�p�|�����C���f�[�^
*/
BoostPolyline CAnalyzeRoadEdgeGeomUtil::ConvBoostPolyline(const std::vector<CVector3D> &vecSrcPolyline)
{
    BoostPolyline dstPolyline;
    for (std::vector<CVector3D>::const_iterator it = vecSrcPolyline.cbegin();
        it != vecSrcPolyline.cend(); it++)
    {
        BoostPoint pt(it->x, it->y);
        bg::append(dstPolyline, pt);
    }
    return dstPolyline;
}

/*!
 * @brief       boost�p�|�����C���W���f�[�^�ϊ�
 * @param[in]   vecSrcPolylines ���̓|�����C���W��
 * @return      boost�p�}���`�|�����C��
*/
BoostMultiLines CAnalyzeRoadEdgeGeomUtil::ConvBoostMultiLines(const std::vector<std::vector<CVector3D>> &vecSrcPolylines)
{
    BoostMultiLines dstPolylines;
    for (std::vector<std::vector<CVector3D>>::const_iterator itLine = vecSrcPolylines.cbegin();
        itLine != vecSrcPolylines.cend(); itLine++)
    {
        BoostPolyline polyline = ConvBoostPolyline(*itLine);
        dstPolylines.push_back(polyline);
    }
    return dstPolylines;
}

/*!
 * @brief ���Ȍ����m�F
 * @param[in]   ring            �����O
 * @param[out]  crossPts        ���Ȍ������W
 * @param[out]  vecCrossEdgeIdx ���Ȍ������������Ă���ӂ��\������n�_�̒��_�C���f�b�N�X�Q
 * @param[in]   bShortCut       �����I���t���O(true���͌����n�_��1�ӏ�����������I������)
 * @return      ���茋��
 * @retval      true    �����L��
 * @retval      false   �����Ȃ�
*/
bool CAnalyzeRoadEdgeGeomUtil::CheckSelfIntersection(
    const BoostRing &ring,
    BoostMultiPoints &crossPts,
    std::vector<std::pair<size_t, size_t>> &vecCrossEdgeIdx)
{
    crossPts.clear();
    vecCrossEdgeIdx.clear();

    // �������o
    BoostMultiLines lines;
    for (size_t idx = 0; idx < ring.size() - 1; idx++)
    {
        BoostPolyline line;
        line.push_back(ring[idx]);
        line.push_back(ring[idx + 1]);
        if (CEpsUtil::Greater(bg::length(line), 0.0))
            lines.push_back(line);
    }
    std::mutex m;
    concurrency::parallel_for(0, static_cast<int>(lines.size() - 1),
        [&lines, &crossPts, &vecCrossEdgeIdx, &m](size_t idx1)
        {
            for (size_t idx2 = idx1 + 1; idx2 < lines.size(); idx2++)
            {
                if (bg::crosses(lines[idx1], lines[idx2]))  // �����m�F
                {
                    BoostMultiPoints pts;
                    bg::intersection(lines[idx1], lines[idx2], pts);
                    if (!bg::is_empty(pts))
                    {
                        std::lock_guard<std::mutex>  lock(m);
                        for (auto itPt = pts.begin(); itPt != pts.end(); itPt++)
                        {
                            // ��������_���Ȃ����m�F
                            auto itCrossPt = crossPts.begin();
                            for (; itCrossPt != crossPts.end(); itCrossPt++)
                            {
                                if (CheckPointEqual(*itPt, *itCrossPt))
                                    break;  // ������W�𔭌�
                            }
                            if (itCrossPt == crossPts.end())
                            {
                                crossPts.push_back(*itPt);  // �����_
                                vecCrossEdgeIdx.push_back(std::pair<size_t, size_t>(idx1, idx2));
                            }
                        }
                    }
                }
            }
        });
    return (crossPts.size() > 0);
}

/*!
 * @brief ���Ȍ����m�F(�O�֊s�ƌ��̗���)
 * @param[in]   polygon   �|���S��
 * @param[out]  crossPts  ���Ȍ������W
 * @return      ���茋��
 * @retval      true    �����L��
 * @retval      false   �����Ȃ�
*/
bool CAnalyzeRoadEdgeGeomUtil::CheckSelfIntersection(
    const BoostPolygon &polygon,
    BoostMultiPoints &crossPts)
{
    crossPts.clear();
    // �������o
    BoostMultiLines lines;
    for (size_t idx = 0; idx < polygon.outer().size() - 1; idx++)
    {
        BoostPolyline line;
        line.push_back(polygon.outer()[idx]);
        line.push_back(polygon.outer()[idx + 1]);
        if (CEpsUtil::Greater(bg::length(line), 0.0))
            lines.push_back(line);
    }
    for (auto itInner = polygon.inners().begin(); itInner != polygon.inners().end(); itInner++)
    {
        for (size_t idx = 0; idx < itInner->size() - 1; idx++)
        {
            BoostPolyline line;
            line.push_back((*itInner)[idx]);
            line.push_back((*itInner)[idx + 1]);
            if (CEpsUtil::Greater(bg::length(line), 0.0))
                lines.push_back(line);
        }
    }

    std::mutex m;
    concurrency::parallel_for(0, static_cast<int>(lines.size() - 1),
        [&lines, &crossPts, &m](size_t idx1)
        {
            for (size_t idx2 = idx1 + 1; idx2 < lines.size(); idx2++)
            {
                if (bg::crosses(lines[idx1], lines[idx2]))  // �����m�F
                {
                    BoostMultiPoints pts;
                    bg::intersection(lines[idx1], lines[idx2], pts);
                    if (!bg::is_empty(pts))
                    {
                        std::lock_guard<std::mutex>  lock(m);
                        for (auto itPt = pts.begin(); itPt != pts.end(); itPt++)
                        {
                            // ��������_���Ȃ����m�F
                            auto itCrossPt = crossPts.begin();
                            for (; itCrossPt != crossPts.end(); itCrossPt++)
                            {
                                if (CheckPointEqual(*itPt, *itCrossPt))
                                    break;  // ������W�𔭌�
                            }
                            if (itCrossPt == crossPts.end())
                            {
                                crossPts.push_back(*itPt);  // �����_
                            }
                        }
                    }
                }
            }
        });
    return (crossPts.size() > 0);
}

/*!
 * @brief ���Ȍ����̉���(������1�ӏ��̏ꍇ�̂�)
 * @param[in] ring          �������������Ă��郊���O�f�[�^
 * @param[in] crossPt       �������W
 * @param[in] crossEdgeIdx  �����O�f�[�^�ɂ�����������������Ă���ӂ̎n�_�C���f�b�N�X
 * @return    ���Ȍ��������ς݂̃����O�f�[�^(�����o���Ȃ������ꍇ�͋�vector)
*/
std::vector<BoostRing> CAnalyzeRoadEdgeGeomUtil::SelfIntersectionResolution(
    const BoostRing &ring,
    BoostPoint &crossPt,
    std::pair<size_t, size_t> &crossEdgeIdx)
{
    // ����
    std::vector<size_t> vecIdx;
    vecIdx.push_back(crossEdgeIdx.first);
    vecIdx.push_back(crossEdgeIdx.second);
    std::vector<BoostRing> vecParts;
    for (auto itTarget = vecIdx.begin(); itTarget != vecIdx.end(); itTarget++)
    {
        // �����_���玟�ɂȂ����_�̌���
        // �����ӂ̂ǂ�����g�p���邩���肷��
        size_t nextIdx;
        if ((itTarget + 1) == vecIdx.end())
            nextIdx = *vecIdx.begin();
        else
            nextIdx = *(itTarget + 1);

        // �����ӂ̏I�_�Ɉړ�
        nextIdx++;
        if (nextIdx >= ring.size() - 1)
            nextIdx = 0;    // �I�_�̏ꍇ�͎n�_�Ɉړ�(�n�I�_��������W�̂���)

        BoostRing partRing;
        partRing.push_back(crossPt);   // �����_�}��
        do
        {
            partRing.push_back(ring[nextIdx]);
            nextIdx++;
            if (nextIdx >= ring.size() - 1)
                nextIdx = 0;    // �I�_�̏ꍇ�͎n�_�Ɉړ�(�n�I�_��������W�̂���)
        } while (nextIdx != *itTarget);
        partRing.push_back(ring[*itTarget]);
        bg::unique(partRing);
        if (partRing.size() > 3 && !CEpsUtil::Equal(bg::area(partRing), 0))
            vecParts.push_back(partRing);
    }

    return vecParts;
}

/*!
 * @brief ���Ȍ����m�F�Ɖ�������
 * @param[in]   ring        �������������Ă��郊���O�f�[�^
 * @param[out]  vecRings    ����������̃����O�f�[�^(���_�̏���(�\��)�͎g�p���Ɋm�F���邱��)
*/
void CAnalyzeRoadEdgeGeomUtil::SelfIntersectionResolution(
    const BoostRing &ring,
    std::vector<BoostRing> &vecRings)
{
    vecRings.clear();

    std::stack<BoostRing> targets;
    targets.push(ring);
    while (!targets.empty())
    {
        BoostRing target = targets.top();
        targets.pop();

        // ���Ȍ����m�F
        BoostMultiPoints crossPts;
        std::vector<std::pair<size_t, size_t>> vecCrossEdgeIdx;
        if (CAnalyzeRoadEdgeGeomUtil::CheckSelfIntersection(
            target, crossPts, vecCrossEdgeIdx))
        {
            // ���Ȍ�����1�ӏ��������A�X�^�b�N�ɐς�
            std::vector<BoostRing> tmpVecRings = CAnalyzeRoadEdgeGeomUtil::SelfIntersectionResolution(
                target, crossPts[0], vecCrossEdgeIdx[0]);

            for (auto itRing = tmpVecRings.begin(); itRing != tmpVecRings.end(); itRing++)
            {
                bg::unique(*itRing);
                targets.push(*itRing);
            }
        }
        else
        {
            // ���Ȍ����Ȃ�
            bg::unique(target);
            vecRings.push_back(target);
        }
    }
}

// ���Ȍ����m�F�Ɖ�������
void CAnalyzeRoadEdgeGeomUtil::SelfIntersectionResolution(
    const BoostPolygon &polygon,
    BoostMultiPolygon &polygons)
{
    polygons.clear();

    // ���̎��Ȍ�������
    std::vector<BoostRing> vecHoles;
    for (auto itHole = polygon.inners().begin(); itHole != polygon.inners().end(); itHole++)
    {
        std::vector<BoostRing> vecTmpRings;
        CAnalyzeRoadEdgeGeomUtil::SelfIntersectionResolution(*itHole, vecTmpRings);

        // �ʐύő�̌����擾
        std::vector<BoostRing>::iterator itTargetHole = vecTmpRings.begin();
        for (auto itTmp = vecTmpRings.begin(); itTmp != vecTmpRings.end(); itTmp++)
        {
            if (CEpsUtil::Greater(abs(bg::area(*itTmp)), abs(bg::area(*itTargetHole))))
            {
                itTargetHole = itTmp;
            }
        }

        // �ʐς����ł͂Ȃ��ꍇ�͔��]
        if (!CEpsUtil::GreaterEqual(bg::area(*itTargetHole), 0))
            bg::reverse(*itTargetHole);
        vecHoles.push_back(*itTargetHole);

    }

    // �O�֊s�̎��Ȍ�������
    std::vector<BoostRing> vecOuterRings;
    CAnalyzeRoadEdgeGeomUtil::SelfIntersectionResolution(polygon.outer(), vecOuterRings);

    // ���̍Đݒ�
    for (auto itOuterRing = vecOuterRings.begin(); itOuterRing != vecOuterRings.end(); itOuterRing++)
    {
        // ���_��̏�������̏C��
        bg::correct(*itOuterRing);

        BoostPolygon newPolygon;
        std::copy(itOuterRing->begin(), itOuterRing->end(), std::back_inserter(newPolygon.outer()));
        BoostMultiPolygon newPolygons;
        newPolygons.push_back(newPolygon);
        for (auto itHole = vecHoles.begin();
            itHole != vecHoles.end(); itHole++)
        {
            if (!bg::disjoint(*itHole, newPolygons))
            {
                BoostMultiPolygon diffPolygons;
                bg::difference(newPolygons, *itHole, diffPolygons);
                newPolygons = diffPolygons;
            }
        }
        bg::unique(newPolygons);
        bg::correct(newPolygons);
        polygons.insert(polygons.end(), newPolygons.begin(), newPolygons.end());
    }
}

/*!
 * @brief �����_�ȉ�n���ł̎l�̌ܓ�
 * @param[in] dValue    �l�̌ܓ��Ώ�
 * @param[in] nDigit    �����_�ȉ��̗L������(nDisit+1�̌����l�̌ܓ�����)
 * @return  �l�̌ܓ�����
*/
double CAnalyzeRoadEdgeGeomUtil::RoundN(double dValue, int nDigit)
{
    double dPow = pow(10.0, static_cast<double>(nDigit));
    double dTmp = dValue * dPow;

    double dRet = dValue;
    if (CEpsUtil::Greater(dTmp, 0))
    {
        dRet = floor(dTmp + 0.5) / dPow;
    }
    else if (CEpsUtil::Less(dTmp, 0))
    {
        dRet = floor(abs(dTmp) + 0.5) / dPow * -1.0;
    }

    return dRet;
}

/*!
 * @brief �����̃T���v�����O
 * @param[in]   start �n�_
 * @param[in]   end   �I�_
 * @param[out]  pts   �T���v�����O����
 * @param[in]   dInterval �T���v�����O�Ԋu
 * @return  �T���v�����O����
*/
void CAnalyzeRoadEdgeGeomUtil::Sampling(
    const CVector2D &start,
    const CVector2D &end,
    std::vector<CVector2D> &pts,
    const double dInterval)
{
    assert(CEpsUtil::Greater(dInterval, 0));

    CVector2D vec = end - start;
    double dEdgeLength = vec.Length();

    // ���K��
    vec.Normalize();

    // �T���v�����O
    double dRate = dInterval;
    pts.clear();
    pts.push_back(start); // �n�_�ǉ�
    while (CEpsUtil::Less(dRate, dEdgeLength))  // �I�_���ǉ�����Ȃ������G�b�W�̎n�_�Ƃ��Ēǉ������
    {
        // �T���v�����O�_
        CVector2D pt = dRate * vec + start;
        pts.push_back(CVector2D(pt.x, pt.y));
        dRate += dInterval;
    }
    pts.push_back(end); // �I�_�ǉ�
}

/*!
 * @brief �T���v�����O
 * @param[in] polyline  �|�����C��
 * @param[in] dInterval �T���v�����O�Ԋu
 * @return �T���v�����O��̃|�����C��
*/
BoostPolyline CAnalyzeRoadEdgeGeomUtil::Sampling(
    const BoostPolyline &polyline,
    const double dInterval)
{
    // �T���v�����O�Ԋu�̊m�F
    if (CEpsUtil::Zero(dInterval) || dInterval < 0.0)
        return BoostPolyline(polyline);

    BoostPolyline dstPolyline;
    for (BoostPolyline::const_iterator it = polyline.cbegin();
        it != polyline.cend(); it++)
    {
        if (it == polyline.cend() - 1)
        {
            // �I�_�̏ꍇ
            dstPolyline.push_back(*it);
        }
        else
        {
            // �I�_�ȊO
            BoostPolyline::const_iterator itNext = it + 1;
            CVector2D pt1(it->x(), it->y());
            CVector2D pt2(itNext->x(), itNext->y());
            CVector2D vec = pt2 - pt1;
            double dEdgeLength = vec.Length();

            // ���K��
            vec.Normalize();

            // �T���v�����O
            double dRate = dInterval;
            dstPolyline.push_back(*it); // �n�_�ǉ�
            while (CEpsUtil::Less(dRate, dEdgeLength))  // �I�_���ǉ�����Ȃ������G�b�W�̎n�_�Ƃ��Ēǉ������
            {
                // �T���v�����O�_
                CVector2D pt = dRate * vec + pt1;
                dstPolyline.push_back(BoostPoint(pt.x, pt.y));
                dRate += dInterval;
            }
        }
    }
    return dstPolyline;
}

/*!
 * @brief �T���v�����O
 * @param[in] polylines  �|�����C���Q
 * @param[in] dInterval �T���v�����O�Ԋu
 * @return �T���v�����O��̃|�����C���Q
*/
BoostMultiLines CAnalyzeRoadEdgeGeomUtil::Sampling(
    const BoostMultiLines &polylines,
    const double dInterval)
{
    BoostMultiLines dstPolylines;
    for (BoostMultiLines::const_iterator it = polylines.cbegin();
        it != polylines.cend(); it++)
    {
        dstPolylines.push_back(Sampling(*it, dInterval));
    }
    return dstPolylines;
}

/*!
 * @brief  2D���W�̉�]
 * @param[in] pos       ��]�Ώۍ��W
 * @param[in] center    ��]���S���W
 * @param[in] dAngle    ��]�p(deg)
 * @return ��]��̍��W
*/
CVector2D CAnalyzeRoadEdgeGeomUtil::Rotate(
    const CVector2D &pos,
    const CVector2D &center,
    const double dAngle)
{
    CVector2D vec = pos - center;
    double dRad = dAngle * _COEF_DEG_TO_RAD;
    double dX = vec.x * cos(dRad) - vec.y * sin(dRad);
    double dY = vec.x * sin(dRad) + vec.y * cos(dRad);
    CVector2D rotatePos(dX + center.x, dY + center.y);
    return rotatePos;
}

/*!
 * @brief ���H�|���S���̊ȗ���
 * @param[in] polygon  ���H�|���S��
 * @return  �ȗ�����̓��H�|���S��
 * @note    ��������Ɉʒu���钸�_���폜����
*/
BoostPolygon CAnalyzeRoadEdgeGeomUtil::Simplify(const BoostPolygon &polygon)
{
    BoostPolygon newPolygon;
    BoostRing newOuter = CAnalyzeRoadEdgeGeomUtil::Simplify(polygon.outer());
    std::copy(newOuter.begin(), newOuter.end(), std::back_inserter(newPolygon.outer()));
    bg::unique(newOuter);
    bg::correct(newOuter);

    for (auto itRing = polygon.inners().begin();
        itRing != polygon.inners().end(); itRing++)
    {
        BoostRing newInner = CAnalyzeRoadEdgeGeomUtil::Simplify(*itRing);
        newPolygon.inners().push_back(BoostPolygon::ring_type());
        std::copy(newInner.begin(), newInner.end(), std::back_inserter(newPolygon.inners().back()));
        bg::unique(newPolygon);
        bg::correct(newPolygon);
    }

    return newPolygon;
}

/*!
 * @brief ���H�|���S���̊ȗ���
 * @param[in] polygons  ���H�|���S���Q
 * @return  �ȗ�����̓��H�|���S��
 * @note    ��������Ɉʒu���钸�_���폜����
*/
BoostMultiPolygon CAnalyzeRoadEdgeGeomUtil::Simplify(const BoostMultiPolygon &polygons)
{
    BoostMultiPolygon dstPolygons;
    for (auto itPolygon = polygons.begin(); itPolygon != polygons.end(); itPolygon++)
    {
        BoostPolygon newPolygon = CAnalyzeRoadEdgeGeomUtil::Simplify(*itPolygon);
        dstPolygons.push_back(newPolygon);
    }
    return dstPolygons;
}

/*!
 * @brief ���H���̊ȗ���
 * @param[in] line  ���H��
 * @return  �ȗ�����̓��H��
 * @note    ��������Ɉʒu���钸�_���폜����
*/
BoostPolyline CAnalyzeRoadEdgeGeomUtil::Simplify(const BoostPolyline &line)
{
    BoostPolyline newLine;
    newLine.push_back(line.front());    // �n�_
    for (auto itPt = line.cbegin() + 1; itPt != line.cend() - 1; itPt++)
    {
        CVector2D prev(newLine.back().x(), newLine.back().y());
        CVector2D current(itPt->x(), itPt->y());
        CVector2D next((itPt + 1)->x(), (itPt + 1)->y());
        CVector2D vec1 = prev - current;
        CVector2D vec2 = next - current;
        double dAngle = CGeoUtil::Angle(vec1, vec2);
        if (!CEpsUtil::GreaterEqual(dAngle, 179.0))
        {
            // �����ł͂Ȃ��ꍇ�͒ǉ�
            newLine.push_back(*itPt);
        }
    }
    newLine.push_back(line.back()); // �I�_
    return newLine;
}

BoostRing CAnalyzeRoadEdgeGeomUtil::Simplify(const BoostRing &ring)
{
    BoostRing newRing;

    // �J�n�_�͐����ł͂Ȃ����_�Ƃ���
    BoostRing::const_iterator itTarget = ring.cend();
    for (auto itPt = ring.cbegin(); itPt < ring.cend() - 1; itPt++)
    {
        BoostRing::const_iterator itPrev;
        if (itPt == ring.cbegin())
        {
            itPrev = ring.cend() - 2;
        }
        else
        {
            itPrev = itPt - 1;
        }
        CVector2D prev(itPrev->x(), itPrev->y());
        CVector2D current(itPt->x(), itPt->y());
        CVector2D next((itPt + 1)->x(), (itPt + 1)->y());
        CVector2D vec1 = prev - current;
        CVector2D vec2 = next - current;
        double dAngle = CGeoUtil::Angle(vec1, vec2);
        if (!CEpsUtil::GreaterEqual(dAngle, 179.0))
        {
            // �����ł͂Ȃ��ꍇ
            itTarget = itPt;
            break;
        }
    }

    newRing.push_back(*itTarget);
    BoostRing::const_iterator itEnd = itTarget;
    itTarget = (itTarget < ring.cend() - 2) ? itTarget + 1 : ring.cbegin();

    while (itTarget != itEnd)
    {
        BoostRing::const_iterator itNext;
        if (itTarget < ring.cend() - 2)
            itNext = itTarget + 1;
        else
            itNext = ring.cbegin();

        CVector2D prev(newRing.back().x(), newRing.back().y());
        CVector2D current(itTarget->x(), itTarget->y());
        CVector2D next(itNext->x(), itNext->y());
        CVector2D vec1 = prev - current;
        CVector2D vec2 = next - current;
        double dAngle = CGeoUtil::Angle(vec1, vec2);
        if (!CEpsUtil::GreaterEqual(dAngle, 179.0))
        {
            // �����ł͂Ȃ��ꍇ�͒ǉ�
            newRing.push_back(*itTarget);
        }

        itTarget = (itTarget < ring.cend() - 2) ? itTarget + 1 : ring.cbegin();
    }
    newRing.push_back(*itEnd);
    bg::unique(newRing);
    bg::correct(newRing);

    return newRing;
}

/*!
 * @brief 2�����̌�_�Z�o
 * @param[in]   vec1        �x�N�g��1
 * @param[in]   pos1        �x�N�g��1�̎n�_
 * @param[in]   vec2        �x�N�g��2
 * @param[in]   pos2        �x�N�g��2�̎n�_
 * @param[out]  pos         ��_
 * @param[out]  bOnLine1    �x�N�g��1��Ɍ�_�����݂��邩
 * @param[out]  bOnLine2    �x�N�g��2��Ɍ�_�����݂��邩
 * @param[out]  t           ��_�ʒu�܂ł̃x�N�g��1�̃x�N�g���W��
 * @param[out]  s           ��_�ʒu�܂ł̃x�N�g��2�̃x�N�g���W��
 * @return �Z�o����
 * @retval true     ����
 * @retval false    ���s
*/
bool CAnalyzeRoadEdgeGeomUtil::GetCrossPos(
    const CVector2D &vec1,
    const CVector2D &pos1,
    const CVector2D &vec2,
    const CVector2D &pos2,
    CVector2D &pos,
    bool &bOnLine1,
    bool &bOnLine2,
    double &t,
    double &s)
{
    if (CEpsUtil::Zero(vec1.Length()) || CEpsUtil::Zero(vec2.Length()))
        return false;

    // vec1�ɐ����ȃx�N�g��
    CVector2D vecVertical;
    if (!CAnalyzeRoadEdgeGeomUtil::GetVerticalVec(vec1, vecVertical))
        return false;

    // ��_�T��
    // l(s) = p + su (vec2)
    // m(t) = q + tv (vec1)
    // m�ɐ����ȒP�ʃx�N�g��n
    // s = n(q - p) / nu
    // ����nv = 0
    double dTmpVal = CGeoUtil::InnerProduct(vecVertical, vec2);
    if (CEpsUtil::Zero(dTmpVal))
        return false;

    s = CGeoUtil::InnerProduct(vecVertical, pos1 - pos2) / dTmpVal;
    // ��_
    pos = vec2 * s + pos2;

    // ��_������ɑ��݂��邩�m�F����
    CVector2D vec = pos - pos1;
    t = vec.Length() / vec1.Length();
    double dAngle = CGeoUtil::Angle(vec1, vec);
    if (CEpsUtil::Greater(dAngle, 90.0))
        t *= -1.0;
    bOnLine1 = (CEpsUtil::GreaterEqual(t, 0) && CEpsUtil::LessEqual(t, 1.0)) ? true : false;
    bOnLine2 = (CEpsUtil::GreaterEqual(s, 0) && CEpsUtil::LessEqual(s, 1.0)) ? true : false;
    return true;
}

/*!
 * @brief �_���璼���ɑ΂��Đ������������ۂ̌�_�Z�o
 * @param[in] targetPos ���ړ_
 * @param[in] vec       �����̃x�N�g��
 * @param[in] pos       �����̎n�_
 * @param[out] crossPos ��_
 * @param[out] bOnLine  ��_��������Ɉʒu���邩�ۂ�
 * @param[out] t        �n�_�����_�܂ł̃x�N�g���W��
 * @param[out] dist     ���ړ_�ƌ�_�܂ł̋���
 * @return �Z�o����
 * @retval true     ����
 * @retval false    ���s
*/
bool CAnalyzeRoadEdgeGeomUtil::GetCrossPos(
    const CVector2D &targetPos,
    const CVector2D &vec,
    const CVector2D &pos,
    CVector2D &crossPos,
    bool &bOnLine,
    double &t,
    double &dist)
{
    bool bRet = false;
    if (CEpsUtil::Greater(vec.Length(), 0))
    {
        double dLength = vec.Length();

        // �n�_�ƒ��ړ_���q���x�N�g��
        CVector2D vecTarget = targetPos - pos;

        // �����x�N�g���̒P�ʃx�N�g��
        CVector2D vecE(vec);
        vecE.Normalize();

        // �ˉe
        double dInnerProduct = CGeoUtil::InnerProduct(vecTarget, vecE);
        crossPos = vecE * dInnerProduct + pos;
        t = dInnerProduct / dLength;

        // ������Ɍ�_���ʒu���邩�ۂ�
        bOnLine = false;
        if (CEpsUtil::GreaterEqual(dInnerProduct, 0) && CEpsUtil::LessEqual(dInnerProduct, dLength))
        {
            bOnLine = true;
        }

        // ���ړ_�����_�܂ł̋���
        dist = (crossPos - targetPos).Length();

        bRet = true;
    }

    return bRet;
}

/*!
 * @brief ���̓x�N�g���ɐ����Ȑ��K���ς݃x�N�g���̎擾
 * @param[in] vec
 * @param[out] vecVertical
 * @return �쐬����
 * @retval true     ����
 * @retval false    ���s
*/
bool CAnalyzeRoadEdgeGeomUtil::GetVerticalVec(
    const CVector2D &vec,
    CVector2D &vecVertical)
{
    if (CEpsUtil::Zero(vec.Length()))
        return false;

    // vec1�ɐ����ȃx�N�g��
    double dK = sqrt(1.0 / (vec.x * vec.x + vec.y * vec.y));
    vecVertical.x = vec.y * dK;
    vecVertical.y = vec.x * -dK;
    vecVertical.Normalize();

    return true;
}

/*!
 * @brief ���W�ϊ�(wolrd -> px)
 * @param[in]   dX          world x���W
 * @param[in]   dY          world y���W
 * @param[in]   dOffsetX    �I�t�Z�b�gworld x���W
 * @param[in]   dOffsetY    �I�t�Z�b�gworld y���W
 * @param[in]   dReso       �𑜓x(m/px)
 * @param[out]  nX          �摜x���W
 * @param[out]  nY          �摜y���W
*/
void CAnalyzeRoadEdgeGeomUtil::ConvertWorldToPx(
    const double dX,
    const double dY,
    const double dOffsetX,
    const double dOffsetY,
    const double dReso,
    int &nX,
    int &nY)
{
    nX = static_cast<int>((dX - dOffsetX) / dReso);
    nY = static_cast<int>((dY - dOffsetY) / -dReso);
}

/*!
 * @brief ���W�ϊ�(px -> world)
 * @param[in]   nX          �摜x���W
 * @param[in]   nY          �摜y���W
 * @param[in]   dOffsetX    �I�t�Z�b�gworld x���W
 * @param[in]   dOffsetY    �I�t�Z�b�gworld y���W
 * @param[in]   dReso       �𑜓x(m/px)
 * @param[out]  dX          world x���W
 * @param[out]  dY          world y���W
*/
void CAnalyzeRoadEdgeGeomUtil::ConvertPxToWorld(
    const int nX,
    const int nY,
    const double dOffsetX,
    const double dOffsetY,
    const double dReso,
    double &dX,
    double &dY)
{
    dX = static_cast<double>(nX) * dReso + dOffsetX;
    dY = static_cast<double>(nY) * -dReso + dOffsetY;
}

/*!
 * @brief ���W�ϊ�(px -> world)
 * @param[in]   dImgX       �摜x���W
 * @param[in]   dImgY       �摜y���W
 * @param[in]   dOffsetX    �I�t�Z�b�gworld x���W
 * @param[in]   dOffsetY    �I�t�Z�b�gworld y���W
 * @param[in]   dReso       �𑜓x(m/px)
 * @param[out]  dWorldX     world x���W
 * @param[out]  dWorldY     world y���W
*/
void CAnalyzeRoadEdgeGeomUtil::ConvertPxToWorld(
    const double dImgX,
    const double dImgY,
    const double dOffsetX,
    const double dOffsetY,
    const double dReso,
    double &dWorldX,
    double &dWorldY)
{
    dWorldX = dImgX * dReso + dOffsetX;
    dWorldY = dImgY * -dReso + dOffsetY;
}

/*!
 * @brief ���͒��_���o�b�t�@�����O���|���S���Ƃ���
 * @param[in]   pt    �o�b�t�@�����O�Ώے��_
 * @param[in]   dDist �o�b�t�@����(m)
 * @return      �|���S��(�~�`)
*/
BoostMultiPolygon CAnalyzeRoadEdgeGeomUtil::Buffering(
    const BoostPoint &pt,
    const double dDist)
{
    assert(CEpsUtil::Greater(dDist, 0));

    // �o�b�t�@�����O�X�g���e�W�[
    bg::strategy::buffer::distance_symmetric<double> distStrategy(dDist);
    bg::strategy::buffer::join_miter joinStrategy;
    bg::strategy::buffer::end_flat endStrategy;
    bg::strategy::buffer::point_circle pointStrategy;
    bg::strategy::buffer::side_straight sideStrategy;

    // ���ڃG���A
    BoostMultiPolygon areas;
    bg::buffer(pt, areas, distStrategy, sideStrategy,
        joinStrategy, endStrategy, pointStrategy);

    return areas;
}

/*!
 * @brief ���̓|�����C���Q���o�b�t�@�����O���|���S���Ƃ���
 * @param[in]   lines �o�b�t�@�����O�Ώۃ|�����C���Q
 * @param[in]   dDist �o�b�t�@����(m)
 * @return      �|���S��
*/
BoostMultiPolygon CAnalyzeRoadEdgeGeomUtil::Buffering(
    const BoostMultiLines &lines,
    const double dDist)
{
    assert(CEpsUtil::Greater(dDist, 0));

    // �o�b�t�@�����O�X�g���e�W�[
    bg::strategy::buffer::distance_symmetric<double> distStrategy(dDist);
    bg::strategy::buffer::join_miter joinStrategy;
    bg::strategy::buffer::end_flat endStrategy;
    bg::strategy::buffer::point_circle pointStrategy;
    bg::strategy::buffer::side_straight sideStrategy;

    // ���ڃG���A
    BoostMultiPolygon areas;
    bg::buffer(lines, areas, distStrategy, sideStrategy,
        joinStrategy, endStrategy, pointStrategy);

    return areas;
}

/*!
 * @brief ���̓|�����C�����o�b�t�@�����O���|���S���Ƃ���
 * @param[in]   lines �o�b�t�@�����O�Ώۃ|�����C��
 * @param[in]   dDist �o�b�t�@����(m)
 * @return      �|���S��
*/
BoostMultiPolygon CAnalyzeRoadEdgeGeomUtil::Buffering(
    const BoostPolyline &line,
    const double dDist)
{
    assert(CEpsUtil::Greater(dDist, 0));

    // �o�b�t�@�����O�X�g���e�W�[
    bg::strategy::buffer::distance_symmetric<double> distStrategy(dDist);
    bg::strategy::buffer::join_miter joinStrategy;
    bg::strategy::buffer::end_flat endStrategy;
    bg::strategy::buffer::point_circle pointStrategy;
    bg::strategy::buffer::side_straight sideStrategy;

    // ���ڃG���A
    BoostMultiPolygon areas;
    bg::buffer(line, areas, distStrategy, sideStrategy,
        joinStrategy, endStrategy, pointStrategy);

    return areas;
}

/*!
 * @brief ���̓|���S���Ƀo�b�t�@��t�^����
 * @param[in]   polygon �o�b�t�@�����O�Ώ�
 * @param[in]   dDist   �o�b�t�@����(m)
 * @return      �|���S��
*/
BoostPolygon CAnalyzeRoadEdgeGeomUtil::Buffering(
    const BoostPolygon &polygon,
    const double dDist)
{
    // �o�b�t�@�����O�X�g���e�W�[
    bg::strategy::buffer::distance_symmetric<double> distStrategy(dDist);
    bg::strategy::buffer::join_miter joinStrategy;
    bg::strategy::buffer::end_flat endStrategy;
    bg::strategy::buffer::point_circle pointStrategy;
    bg::strategy::buffer::side_straight sideStrategy;

    BoostMultiPolygon dstPolygon;
    bg::buffer(polygon, dstPolygon, distStrategy, sideStrategy,
        joinStrategy, endStrategy, pointStrategy);

    return dstPolygon.front();
}

/*!
 * @brief ���̓|���S���ɑ΂��ă����t�H���W�[�����̃I�[�v�j���O�ɑ�������s��
 * @param[in]   polygons    �I�[�v�j���O�Ώۃ|���S���Q
 * @param[in]   dDist       �o�b�t�@����(m)
 * @return      �|���S���Q
*/
BoostMultiPolygon CAnalyzeRoadEdgeGeomUtil::Opening(
    const BoostMultiPolygon &polygons,
    const double dDist)
{
    assert(CEpsUtil::Greater(dDist, 0));

    // �o�b�t�@�����O�X�g���e�W�[
    bg::strategy::buffer::distance_symmetric<double> shrinkDistStrategy(-dDist);
    bg::strategy::buffer::distance_symmetric<double> expansionDistStrategy(dDist);
    bg::strategy::buffer::join_miter joinStrategy;
    bg::strategy::buffer::end_flat endStrategy;
    bg::strategy::buffer::point_circle pointStrategy;
    bg::strategy::buffer::side_straight sideStrategy;

    BoostMultiPolygon srcPolygons, tmpPolygons, correctPolygons, dstPolygons;

    // ��������ɑ��݂��钸�_�̍폜
    srcPolygons = CAnalyzeRoadEdgeGeomUtil::Simplify(polygons);

    // ���k
    bg::buffer(
        srcPolygons, tmpPolygons, shrinkDistStrategy, sideStrategy,
        joinStrategy, endStrategy, pointStrategy);
    bg::unique(tmpPolygons);
    bg::correct(tmpPolygons);
    correctPolygons = CAnalyzeRoadEdgeGeomUtil::Simplify(tmpPolygons);

    // �c��
    tmpPolygons.clear();
    bg::buffer(
        correctPolygons, tmpPolygons, expansionDistStrategy, sideStrategy,
        joinStrategy, endStrategy, pointStrategy);
    bg::unique(tmpPolygons);
    bg::correct(tmpPolygons);
    dstPolygons = CAnalyzeRoadEdgeGeomUtil::Simplify(tmpPolygons);

    return dstPolygons;
}

/*!
 * @brief   2�_�ԋ���
 * @param[in] pt1
 * @param[in] pt2
 * @return  ����
*/
double CAnalyzeRoadEdgeGeomUtil::Length(
    const BoostPoint &pt1,
    const BoostPoint &pt2)
{
    BoostPolyline line;
    line.push_back(pt1);
    line.push_back(pt2);
    return bg::length(line);
}

/*!
 * @brief �|���S���̃G�b�W�����o
 * @param[in] polygons �|���S���Q
 * @return �G�b�W���Q
*/
BoostMultiLines CAnalyzeRoadEdgeGeomUtil::GetEdges(
    const BoostMultiPolygon &polygons)
{
    BoostMultiLines dstLines;
    for (auto itPoly = polygons.cbegin(); itPoly != polygons.cend(); itPoly++)
    {
        // �O�֊s
        BoostPolyline outer;
        for (auto itPt = itPoly->outer().cbegin(); itPt != itPoly->outer().cend(); itPt++)
            outer.push_back(*itPt);
        dstLines.push_back(outer);

        // ��
        for (auto itHole = itPoly->inners().cbegin(); itHole != itPoly->inners().cend(); itHole++)
        {
            BoostPolyline inner;
            for (auto itPt = itHole->cbegin(); itPt != itHole->cend(); itPt++)
                inner.push_back(*itPt);
            dstLines.push_back(inner);
        }
    }
    return dstLines;
}

/*!
 * @brief �X�p�C�N�m�F
 * @param[in]   ring            �����O�f�[�^
 * @param[out]  vecSpike        �X�p�C�N�n�_�̍��W
 * @param[out]  spikePts        �X�p�C�N�n�_�̍��W�C���f�b�N�X�z��
 * @return      �X�p�C�N�̗L��
 * @retval      true    �X�p�C�N�L��
 * @retval      false    �X�p�C�N����
*/
bool CAnalyzeRoadEdgeGeomUtil::CheckSpike(
    const BoostRing &ring,
    BoostMultiPoints &spikePts,
    std::vector<size_t> &vecSpikePtIdx)
{
    spikePts.clear();
    vecSpikePtIdx.clear();
#if 0
    for (size_t idx = 0; idx < ring.size() - 1; idx++)
    {
        CVector2D current(ring[idx].x(), ring[idx].y());
        CVector2D next(ring[idx + 1].x(), ring[idx + 1].y());
        CVector2D prev;
        if (idx == 0)
        {
            if (ring.size() - 2 > 0)
            {
                prev.x = ring[ring.size() - 2].x();
                prev.y = ring[ring.size() - 2].y();
            }
        }
        else
        {
            prev.x = ring[idx - 1].x();
            prev.y = ring[idx - 1].y();
        }

        CVector2D vec1 = prev - current;
        CVector2D vec2 = next - current;
        double dAngle = CAnalyzeRoadEdgeGeomUtil::RoundN(CGeoUtil::Angle(vec1, vec2), 3);
        if (CEpsUtil::Less(dAngle, 1.0))
        {
            spikePts.push_back(ring[idx]);
            vecSpikePtIdx.push_back(idx);
        }
        if (bShortCut && spikePts.size() > 0)
            break;  // �����I������ꍇ
    }
#else
    std::mutex m;
    concurrency::parallel_for(0, static_cast<int>(ring.size() - 1),
        [&ring, &spikePts, &vecSpikePtIdx, &m](size_t idx)
        {
            CVector2D current(ring[idx].x(), ring[idx].y());
            CVector2D next(ring[idx + 1].x(), ring[idx + 1].y());
            CVector2D prev;
            if (idx == 0)
            {
                if (ring.size() - 2 > 0)
                {
                    prev.x = ring[ring.size() - 2].x();
                    prev.y = ring[ring.size() - 2].y();
                }
            }
            else
            {
                prev.x = ring[idx - 1].x();
                prev.y = ring[idx - 1].y();
            }

            CVector2D vec1 = prev - current;
            CVector2D vec2 = next - current;
            double dAngle = CAnalyzeRoadEdgeGeomUtil::RoundN(CGeoUtil::Angle(vec1, vec2), 3);
            if (CEpsUtil::Less(dAngle, 1.0))
            {
                std::lock_guard<std::mutex> lock(m);
                spikePts.push_back(ring[idx]);
                vecSpikePtIdx.push_back(idx);
            }
        });
#endif
    return (spikePts.size() > 0);
}

/*!
 * @brief �X�p�C�N�m�F
 * @param[in]   polygon     �|���S��
 * @param[out]  spikePts    �X�p�C�N�n�_�̍��W
 * @param[in]   bShortCut   �����I���t���O
 * @return      �X�p�C�N�̗L��
 * @retval      true    �X�p�C�N�L��
 * @retval      false    �X�p�C�N����
*/
bool CAnalyzeRoadEdgeGeomUtil::CheckSpike(
    const BoostPolygon &polygon,
    BoostMultiPoints &spikePts,
    const bool bShortCut)
{
    std::vector<size_t> vecSpikePtIdx;
    CAnalyzeRoadEdgeGeomUtil::CheckSpike(polygon.outer(), spikePts, vecSpikePtIdx);

    if (!bShortCut || (bShortCut && spikePts.size() == 0))
    {
        // �����I�����Ȃ��ꍇ�A�܂��͑����I�����邪�O�֊s�ɂ̓X�p�C�N�����݂��Ȃ��ꍇ
        for (auto itHole = polygon.inners().begin(); itHole != polygon.inners().end(); itHole++)
        {
            BoostMultiPoints tmpSpikePts;
            if (CAnalyzeRoadEdgeGeomUtil::CheckSpike(*itHole, tmpSpikePts, vecSpikePtIdx))
            {
                spikePts.insert(spikePts.end(), tmpSpikePts.begin(), tmpSpikePts.end());
                if (bShortCut)
                    break;  // �����I��
            }
        }
    }

    return (spikePts.size() > 0);
}

/*!
 * @brief �|���S������G�b�W(����)���擾����
 * @param[in]   polygon �|���S��
 * @param[out]  edges   �擾�G�b�W�Q
 * @remarks edges�͓�����clear���Ȃ�(�����f�[�^���}������Ă���ꍇ�͂��̂܂܃G�b�W��ǉ��}������)
*/
void CAnalyzeRoadEdgeGeomUtil::GetEdges(
    BoostPolygon &polygon,
    BoostMultiLines &edges)
{
    for (auto itPt = polygon.outer().begin();
        itPt < polygon.outer().end() - 1; itPt++)
    {
        BoostPolyline line;
        line.push_back(*itPt);
        line.push_back(*(itPt + 1));
        edges.push_back(line);
    }

    for (auto itInter = polygon.inners().begin();
        itInter != polygon.inners().end(); itInter++)
    {
        for (auto itPt = itInter->begin();
            itPt < itInter->end() - 1; itPt++)
        {
            BoostPolyline line;
            line.push_back(*itPt);
            line.push_back(*(itPt + 1));
            edges.push_back(line);
        }
    }
}

/*!
 * @brief �|���S�����璍�ڗ̈�Əd�􂷂�G�b�W(����)���擾����
 * @param[in]   polygon �|���S��
 * @param[in]   area    ���ڃG���A
 * @param[out]  edges   �擾�G�b�W�Q
 * @remarks edges�͓�����clear���Ȃ�(�����f�[�^���}������Ă���ꍇ�͂��̂܂܃G�b�W��ǉ��}������)
*/
void CAnalyzeRoadEdgeGeomUtil::GetEdges(
    BoostPolygon &polygon,
    BoostPolygon &area,
    BoostMultiLines &edges)
{
    for (auto itPt = polygon.outer().begin();
        itPt < polygon.outer().end() - 1; itPt++)
    {
        BoostPolyline line;
        line.push_back(*itPt);
        line.push_back(*(itPt + 1));
        if (!bg::disjoint(line, area))
            edges.push_back(line);  // �Փ˂���ꍇ
    }

    for (auto itInter = polygon.inners().begin();
        itInter != polygon.inners().end(); itInter++)
    {
        for (auto itPt = itInter->begin();
            itPt < itInter->end() - 1; itPt++)
        {
            BoostPolyline line;
            line.push_back(*itPt);
            line.push_back(*(itPt + 1));
            if (!bg::disjoint(line, area))
                edges.push_back(line);  // �Փ˂���ꍇ
        }
    }
}