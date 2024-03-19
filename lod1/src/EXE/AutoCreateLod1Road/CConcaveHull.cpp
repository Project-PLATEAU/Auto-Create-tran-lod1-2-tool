#include "pch.h"
#include "CConcaveHull.h"
#include "CEpsUtil.h"
#include "concaveman.h"

/*!
 * @brief 入力データの作成
 * @param[in]   pts     点群
 * @param[out]  vecPts  頂点集合
 * @param[out]  vecConvexHullIdx 凸包の頂点インデックス集合
*/
void CConcaveHull::createInputData(
    const BoostMultiPoints &pts,
    std::vector<std::array<double, 2>> &vecPts,
    std::vector<int> &vecConvexHullIdx)
{
    BoostMultiPoints tmpPts;

    // 頂点の詰め替え
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

    // 凹包作成時に使用する凸包作成
    BoostPolygon polygon;
    bg::convex_hull(tmpPts, polygon);

    // 入力用に凸包データを加工
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
 * @brief 入力データの作成
 * @param[in]   polylines マルチポリライン
 * @param[out]  vecPts    頂点集合
 * @param[out]  vecConvexHullIdx 凸包の頂点インデックス集合
*/
void CConcaveHull::createInputData(
    const BoostMultiLines &polylines,
    std::vector<std::array<double, 2>> &vecPts,
    std::vector<int> &vecConvexHullIdx)
{
    BoostMultiPoints tmpPts;

    // 頂点の詰め替え
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

    // 凹包作成時に使用する凸包作成
    BoostPolygon polygon;
    bg::convex_hull(tmpPts, polygon);

    // 入力用に凸包データを加工
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
 * @brief 頂点の存在確認
 * @param[in]   vecPts  探索対象
 * @param[in]   pt      探索点
 * @param[out]  nIdx    点が存在する場合のインデックス番号
 * @return 処理結果
 * @retval true     存在する
 * @retval false    存在しない
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
            // 同一頂点を発見
            return true;
        }
    }

    return false;
}

/*!
 * @brief 凹包作成
 * @param[in] pts       点群
 * @param[in] concavity 凹凸の相対的な尺度(値が大きいほど単純化する)
 * @param[in] lengthTh  セグメントの長さ閾値(閾値を下回るとそれ以上掘り下げない)
 * @return 凸包ポリゴン
*/
BoostPolygon CConcaveHull::ConcaveHull(
    const BoostMultiPoints &pts,
    const double concavity,
    const double lengthTh)
{
    // 入力データの作成
    std::vector<std::array<double, 2>> vecPts;
    std::vector<int> vecConvexHullIdx;
    createInputData(pts, vecPts, vecConvexHullIdx);

    // 凹包作成
    std::vector<std::array<double, 2>> concave = concaveman<double, 16>(
        vecPts, vecConvexHullIdx, concavity, lengthTh);

    // 出力データの作成
    BoostPolygon polygon;
    for (std::vector<std::array<double, 2>>::const_iterator it = concave.cbegin();
        it != concave.cend(); it++)
    {
        polygon.outer().push_back(BoostPoint((*it)[0], (*it)[1]));
    }
    return polygon;
}

/*!
 * @brief 凹包作成
 * @param[in] polylines マルチポリライン
 * @param[in] concavity 凹凸の相対的な尺度(値が大きいほど単純化する)
 * @param[in] lengthTh  セグメントの長さ閾値(閾値を下回るとそれ以上掘り下げない)
 * @return 凸包ポリゴン
*/
BoostPolygon CConcaveHull::ConcaveHull(
    const BoostMultiLines &polylines,
    const double concavity,
    const double lengthTh)
{
    // 入力データの作成
    std::vector<std::array<double, 2>> vecPts;
    std::vector<int> vecConvexHullIdx;
    createInputData(polylines, vecPts, vecConvexHullIdx);

    // 凹包作成
    std::vector<std::array<double, 2>> concave = concaveman<double, 16>(
        vecPts, vecConvexHullIdx, concavity, lengthTh);

    // 出力データの作成
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

