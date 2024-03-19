#pragma once

#include <vector>
#include <array>
#include "AnalyzeRoadEdgeCommon.h"

/*!
 * @brief ����C�u�����̃��b�p�[
*/
class CConcaveHull
{
public:
    // ����쐬
    static BoostPolygon ConcaveHull(
        const BoostMultiPoints &pts,
        const double concavity = 2.0,
        const double lengthTh = 0);

    // ����쐬
    static BoostPolygon ConcaveHull(
        const BoostMultiLines &polylines,
        const double concavity = 2.0,
        const double lengthTh = 0);

private:
    // ���̓f�[�^�̍쐬
    static void createInputData(
        const BoostMultiPoints &pts,
        std::vector<std::array<double, 2>> &vecPts,
        std::vector<int> &vecConvexHullIdx);

    // ���̓f�[�^�̍쐬
    static void createInputData(
        const BoostMultiLines &polylines,
        std::vector<std::array<double, 2>> &vecPts,
        std::vector<int> &vecConvexHullIdx);

    // ���_�̑��݊m�F
    static bool isExistPt(
        const std::vector<std::array<double, 2>> &vecPts,
        const BoostPoint &pt,
        int &nIdx);

};

