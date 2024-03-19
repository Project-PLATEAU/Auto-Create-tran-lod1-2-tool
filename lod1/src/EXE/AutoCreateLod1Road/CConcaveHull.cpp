#include "pch.h"
#include "CConcaveHull.h"
#include "CEpsUtil.h"
#include "concaveman.h"

/*!
 * @brief ���̓f�[�^�̍쐬
 * @param[in]   pts     �_�Q
 * @param[out]  vecPts  ���_�W��
 * @param[out]  vecConvexHullIdx �ʕ�̒��_�C���f�b�N�X�W��
*/
void CConcaveHull::createInputData(
    const BoostMultiPoints &pts,
    std::vector<std::array<double, 2>> &vecPts,
    std::vector<int> &vecConvexHullIdx)
{
    BoostMultiPoints tmpPts;

    // ���_�̋l�ߑւ�
    vecPts.clear();
    for (BoostMultiPoints::const_iterator itPt = pts.cbegin();
        itPt != pts.cend(); itPt++)
    {
        int nIdx;
        if (!isExistPt(vecPts, *itPt, nIdx))
        {
            std::array<double, 2> dstPt = { itPt->x(), itPt->y() };
            vecPts.push_back(dstPt);
            tmpPts.push_back(*itPt);
        }
    }

    // ����쐬���Ɏg�p����ʕ�쐬
    BoostPolygon polygon;
    bg::convex_hull(tmpPts, polygon);

    // ���͗p�ɓʕ�f�[�^�����H
    vecConvexHullIdx.clear();
    for (auto itPt = polygon.outer().cbegin(); itPt != polygon.outer().cend(); itPt++)
    {
        int nIdx;
        if (isExistPt(vecPts, *itPt, nIdx))
        {
            vecConvexHullIdx.push_back(nIdx);
        }
    }
}

/*!
 * @brief ���̓f�[�^�̍쐬
 * @param[in]   polylines �}���`�|�����C��
 * @param[out]  vecPts    ���_�W��
 * @param[out]  vecConvexHullIdx �ʕ�̒��_�C���f�b�N�X�W��
*/
void CConcaveHull::createInputData(
    const BoostMultiLines &polylines,
    std::vector<std::array<double, 2>> &vecPts,
    std::vector<int> &vecConvexHullIdx)
{
    BoostMultiPoints tmpPts;

    // ���_�̋l�ߑւ�
    vecPts.clear();
    for (BoostMultiLines::const_iterator itLine = polylines.cbegin();
        itLine != polylines.cend(); itLine++)
    {
        for (BoostPolyline::const_iterator itPt = itLine->cbegin();
            itPt != itLine->cend(); itPt++)
        {
            int nIdx;
            if (!isExistPt(vecPts, *itPt, nIdx))
            {
                std::array<double, 2> dstPt = { itPt->x(), itPt->y() };
                vecPts.push_back(dstPt);
                tmpPts.push_back(*itPt);
            }
        }
    }

    // ����쐬���Ɏg�p����ʕ�쐬
    BoostPolygon polygon;
    bg::convex_hull(tmpPts, polygon);

    // ���͗p�ɓʕ�f�[�^�����H
    vecConvexHullIdx.clear();
    for (auto itPt = polygon.outer().cbegin(); itPt != polygon.outer().cend(); itPt++)
    {
        int nIdx;
        if (isExistPt(vecPts, *itPt, nIdx))
        {
            vecConvexHullIdx.push_back(nIdx);
        }
    }
}

/*!
 * @brief ���_�̑��݊m�F
 * @param[in]   vecPts  �T���Ώ�
 * @param[in]   pt      �T���_
 * @param[out]  nIdx    �_�����݂���ꍇ�̃C���f�b�N�X�ԍ�
 * @return ��������
 * @retval true     ���݂���
 * @retval false    ���݂��Ȃ�
*/
bool CConcaveHull::isExistPt(
    const std::vector<std::array<double, 2>> &vecPts,
    const BoostPoint &pt,
    int &nIdx)
{
    nIdx = 0;
    for (std::vector<std::array<double, 2>>::const_iterator it = vecPts.cbegin();
        it != vecPts.cend(); it++, nIdx++)
    {
        double dx = abs((*it)[0] - pt.x());
        double dy = abs((*it)[1] - pt.y());
        if (CEpsUtil::Zero(dx) && CEpsUtil::Zero(dy))
        {
            // ���꒸�_�𔭌�
            return true;
        }
    }

    return false;
}

/*!
 * @brief ����쐬
 * @param[in] pts       �_�Q
 * @param[in] concavity ���ʂ̑��ΓI�Ȏړx(�l���傫���قǒP��������)
 * @param[in] lengthTh  �Z�O�����g�̒���臒l(臒l�������Ƃ���ȏ�@�艺���Ȃ�)
 * @return �ʕ�|���S��
*/
BoostPolygon CConcaveHull::ConcaveHull(
    const BoostMultiPoints &pts,
    const double concavity,
    const double lengthTh)
{
    // ���̓f�[�^�̍쐬
    std::vector<std::array<double, 2>> vecPts;
    std::vector<int> vecConvexHullIdx;
    createInputData(pts, vecPts, vecConvexHullIdx);

    // ����쐬
    std::vector<std::array<double, 2>> concave = concaveman<double, 16>(
        vecPts, vecConvexHullIdx, concavity, lengthTh);

    // �o�̓f�[�^�̍쐬
    BoostPolygon polygon;
    for (std::vector<std::array<double, 2>>::const_iterator it = concave.cbegin();
        it != concave.cend(); it++)
    {
        polygon.outer().push_back(BoostPoint((*it)[0], (*it)[1]));
    }
    return polygon;
}

/*!
 * @brief ����쐬
 * @param[in] polylines �}���`�|�����C��
 * @param[in] concavity ���ʂ̑��ΓI�Ȏړx(�l���傫���قǒP��������)
 * @param[in] lengthTh  �Z�O�����g�̒���臒l(臒l�������Ƃ���ȏ�@�艺���Ȃ�)
 * @return �ʕ�|���S��
*/
BoostPolygon CConcaveHull::ConcaveHull(
    const BoostMultiLines &polylines,
    const double concavity,
    const double lengthTh)
{
    // ���̓f�[�^�̍쐬
    std::vector<std::array<double, 2>> vecPts;
    std::vector<int> vecConvexHullIdx;
    createInputData(polylines, vecPts, vecConvexHullIdx);

    // ����쐬
    std::vector<std::array<double, 2>> concave = concaveman<double, 16>(
        vecPts, vecConvexHullIdx, concavity, lengthTh);

    // �o�̓f�[�^�̍쐬
    BoostPolygon polygon;
    for (std::vector<std::array<double, 2>>::const_iterator it = concave.cbegin();
        it != concave.cend(); it++)
    {
        polygon.outer().push_back(BoostPoint((*it)[0], (*it)[1]));
    }
    bg::unique(polygon);
    bg::correct(polygon);
    return polygon;
}

