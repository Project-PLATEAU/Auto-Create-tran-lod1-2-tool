#pragma once

#include <vector>
#include <array>
#include "AnalyzeRoadEdgeCommon.h"

/*!
 * @brief 凹包ライブラリのラッパー
*/
class CConcaveHull
{
public:
    // 凹包作成
    static BoostPolygon ConcaveHull(
        const BoostMultiPoints &pts,
        const double concavity = 2.0,
        const double lengthTh = 0);

    // 凹包作成
    static BoostPolygon ConcaveHull(
        const BoostMultiLines &polylines,
        const double concavity = 2.0,
        const double lengthTh = 0);

private:
    // 入力データの作成
    static void createInputData(
        const BoostMultiPoints &pts,
        std::vector<std::array<double, 2>> &vecPts,
        std::vector<int> &vecConvexHullIdx);

    // 入力データの作成
    static void createInputData(
        const BoostMultiLines &polylines,
        std::vector<std::array<double, 2>> &vecPts,
        std::vector<int> &vecConvexHullIdx);

    // 頂点の存在確認
    static bool isExistPt(
        const std::vector<std::array<double, 2>> &vecPts,
        const BoostPoint &pt,
        int &nIdx);

};

