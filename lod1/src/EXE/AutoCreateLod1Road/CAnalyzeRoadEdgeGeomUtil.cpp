#include "pch.h"
#include "CAnalyzeRoadEdgeGeomUtil.h"
#include "CEpsUtil.h"
#include <ppl.h>
#include <mutex>

/*!
 * @brief boost用頂点の等号比較
 * @param[in] pt1 頂点1
 * @param[in] pt2 頂点2
 * @return 等号比較結果
 * @retval true     等しい
 * @retval false    等しくない
 */
bool CAnalyzeRoadEdgeGeomUtil::CheckPointEqual(const BoostPoint &pt1, const BoostPoint &pt2)
{
    bool bEqual = false;
    if (CEpsUtil::Equal(pt1.x(), pt2.x())
        && CEpsUtil::Equal(pt1.y(), pt2.y()))
    {
        bEqual = true;  // 等しい
    }
    return bEqual;
}

/*!
 * @brief       boost用ポリラインデータ変換
 * @param[im]   vecSrcPolyline 入力ポリラインデータ
 * @return      boost用ポリラインデータ
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
 * @brief       boost用ポリライン集合データ変換
 * @param[in]   vecSrcPolylines 入力ポリライン集合
 * @return      boost用マルチポリライン
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
 * @brief 自己交差確認
 * @param[in]   ring            リング
 * @param[out]  crossPts        自己交差座標
 * @param[out]  vecCrossEdgeIdx 自己交差が発生している辺を構成する始点の頂点インデックス群
 * @param[in]   bShortCut       早期終了フラグ(true時は交差地点を1箇所発見したら終了する)
 * @return      判定結果
 * @retval      true    交差有り
 * @retval      false   交差なし
*/
bool CAnalyzeRoadEdgeGeomUtil::CheckSelfIntersection(
    const BoostRing &ring,
    BoostMultiPoints &crossPts,
    std::vector<std::pair<size_t, size_t>> &vecCrossEdgeIdx)
{
    crossPts.clear();
    vecCrossEdgeIdx.clear();

    // 線分抽出
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
                if (bg::crosses(lines[idx1], lines[idx2]))  // 交差確認
                {
                    BoostMultiPoints pts;
                    bg::intersection(lines[idx1], lines[idx2], pts);
                    if (!bg::is_empty(pts))
                    {
                        std::lock_guard<std::mutex>  lock(m);
                        for (auto itPt = pts.begin(); itPt != pts.end(); itPt++)
                        {
                            // 同一交差点がないか確認
                            auto itCrossPt = crossPts.begin();
                            for (; itCrossPt != crossPts.end(); itCrossPt++)
                            {
                                if (CheckPointEqual(*itPt, *itCrossPt))
                                    break;  // 同一座標を発見
                            }
                            if (itCrossPt == crossPts.end())
                            {
                                crossPts.push_back(*itPt);  // 交差点
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
 * @brief 自己交差確認(外輪郭と穴の両方)
 * @param[in]   polygon   ポリゴン
 * @param[out]  crossPts  自己交差座標
 * @return      判定結果
 * @retval      true    交差有り
 * @retval      false   交差なし
*/
bool CAnalyzeRoadEdgeGeomUtil::CheckSelfIntersection(
    const BoostPolygon &polygon,
    BoostMultiPoints &crossPts)
{
    crossPts.clear();
    // 線分抽出
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
                if (bg::crosses(lines[idx1], lines[idx2]))  // 交差確認
                {
                    BoostMultiPoints pts;
                    bg::intersection(lines[idx1], lines[idx2], pts);
                    if (!bg::is_empty(pts))
                    {
                        std::lock_guard<std::mutex>  lock(m);
                        for (auto itPt = pts.begin(); itPt != pts.end(); itPt++)
                        {
                            // 同一交差点がないか確認
                            auto itCrossPt = crossPts.begin();
                            for (; itCrossPt != crossPts.end(); itCrossPt++)
                            {
                                if (CheckPointEqual(*itPt, *itCrossPt))
                                    break;  // 同一座標を発見
                            }
                            if (itCrossPt == crossPts.end())
                            {
                                crossPts.push_back(*itPt);  // 交差点
                            }
                        }
                    }
                }
            }
        });
    return (crossPts.size() > 0);
}

/*!
 * @brief 自己交差の解消(交差が1箇所の場合のみ)
 * @param[in] ring          交差が発生しているリングデータ
 * @param[in] crossPt       交差座標
 * @param[in] crossEdgeIdx  リングデータにおける交差が発生している辺の始点インデックス
 * @return    自己交差解消済みのリングデータ(解消出来なかった場合は空vector)
*/
std::vector<BoostRing> CAnalyzeRoadEdgeGeomUtil::SelfIntersectionResolution(
    const BoostRing &ring,
    BoostPoint &crossPt,
    std::pair<size_t, size_t> &crossEdgeIdx)
{
    // 分割
    std::vector<size_t> vecIdx;
    vecIdx.push_back(crossEdgeIdx.first);
    vecIdx.push_back(crossEdgeIdx.second);
    std::vector<BoostRing> vecParts;
    for (auto itTarget = vecIdx.begin(); itTarget != vecIdx.end(); itTarget++)
    {
        // 交差点から次につなぐ頂点の決定
        // 交差辺のどちらを使用するか決定する
        size_t nextIdx;
        if ((itTarget + 1) == vecIdx.end())
            nextIdx = *vecIdx.begin();
        else
            nextIdx = *(itTarget + 1);

        // 交差辺の終点に移動
        nextIdx++;
        if (nextIdx >= ring.size() - 1)
            nextIdx = 0;    // 終点の場合は始点に移動(始終点が同一座標のため)

        BoostRing partRing;
        partRing.push_back(crossPt);   // 交差点挿入
        do
        {
            partRing.push_back(ring[nextIdx]);
            nextIdx++;
            if (nextIdx >= ring.size() - 1)
                nextIdx = 0;    // 終点の場合は始点に移動(始終点が同一座標のため)
        } while (nextIdx != *itTarget);
        partRing.push_back(ring[*itTarget]);
        bg::unique(partRing);
        if (partRing.size() > 3 && !CEpsUtil::Equal(bg::area(partRing), 0))
            vecParts.push_back(partRing);
    }

    return vecParts;
}

/*!
 * @brief 自己交差確認と解消処理
 * @param[in]   ring        交差が発生しているリングデータ
 * @param[out]  vecRings    交差解消後のリングデータ(頂点の順列(表裏)は使用時に確認すること)
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

        // 自己交差確認
        BoostMultiPoints crossPts;
        std::vector<std::pair<size_t, size_t>> vecCrossEdgeIdx;
        if (CAnalyzeRoadEdgeGeomUtil::CheckSelfIntersection(
            target, crossPts, vecCrossEdgeIdx))
        {
            // 自己交差を1箇所解消し、スタックに積む
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
            // 自己交差なし
            bg::unique(target);
            vecRings.push_back(target);
        }
    }
}

// 自己交差確認と解消処理
void CAnalyzeRoadEdgeGeomUtil::SelfIntersectionResolution(
    const BoostPolygon &polygon,
    BoostMultiPolygon &polygons)
{
    polygons.clear();

    // 穴の自己交差解消
    std::vector<BoostRing> vecHoles;
    for (auto itHole = polygon.inners().begin(); itHole != polygon.inners().end(); itHole++)
    {
        std::vector<BoostRing> vecTmpRings;
        CAnalyzeRoadEdgeGeomUtil::SelfIntersectionResolution(*itHole, vecTmpRings);

        // 面積最大の穴を取得
        std::vector<BoostRing>::iterator itTargetHole = vecTmpRings.begin();
        for (auto itTmp = vecTmpRings.begin(); itTmp != vecTmpRings.end(); itTmp++)
        {
            if (CEpsUtil::Greater(abs(bg::area(*itTmp)), abs(bg::area(*itTargetHole))))
            {
                itTargetHole = itTmp;
            }
        }

        // 面積が正ではない場合は反転
        if (!CEpsUtil::GreaterEqual(bg::area(*itTargetHole), 0))
            bg::reverse(*itTargetHole);
        vecHoles.push_back(*itTargetHole);

    }

    // 外輪郭の自己交差解消
    std::vector<BoostRing> vecOuterRings;
    CAnalyzeRoadEdgeGeomUtil::SelfIntersectionResolution(polygon.outer(), vecOuterRings);

    // 穴の再設定
    for (auto itOuterRing = vecOuterRings.begin(); itOuterRing != vecOuterRings.end(); itOuterRing++)
    {
        // 頂点列の順列方向の修正
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
 * @brief 小数点以下n桁での四捨五入
 * @param[in] dValue    四捨五入対象
 * @param[in] nDigit    小数点以下の有効桁数(nDisit+1の桁を四捨五入する)
 * @return  四捨五入結果
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
 * @brief 線分のサンプリング
 * @param[in]   start 始点
 * @param[in]   end   終点
 * @param[out]  pts   サンプリング結果
 * @param[in]   dInterval サンプリング間隔
 * @return  サンプリング結果
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

    // 正規化
    vec.Normalize();

    // サンプリング
    double dRate = dInterval;
    pts.clear();
    pts.push_back(start); // 始点追加
    while (CEpsUtil::Less(dRate, dEdgeLength))  // 終点が追加されないが次エッジの始点として追加される
    {
        // サンプリング点
        CVector2D pt = dRate * vec + start;
        pts.push_back(CVector2D(pt.x, pt.y));
        dRate += dInterval;
    }
    pts.push_back(end); // 終点追加
}

/*!
 * @brief サンプリング
 * @param[in] polyline  ポリライン
 * @param[in] dInterval サンプリング間隔
 * @return サンプリング後のポリライン
*/
BoostPolyline CAnalyzeRoadEdgeGeomUtil::Sampling(
    const BoostPolyline &polyline,
    const double dInterval)
{
    // サンプリング間隔の確認
    if (CEpsUtil::Zero(dInterval) || dInterval < 0.0)
        return BoostPolyline(polyline);

    BoostPolyline dstPolyline;
    for (BoostPolyline::const_iterator it = polyline.cbegin();
        it != polyline.cend(); it++)
    {
        if (it == polyline.cend() - 1)
        {
            // 終点の場合
            dstPolyline.push_back(*it);
        }
        else
        {
            // 終点以外
            BoostPolyline::const_iterator itNext = it + 1;
            CVector2D pt1(it->x(), it->y());
            CVector2D pt2(itNext->x(), itNext->y());
            CVector2D vec = pt2 - pt1;
            double dEdgeLength = vec.Length();

            // 正規化
            vec.Normalize();

            // サンプリング
            double dRate = dInterval;
            dstPolyline.push_back(*it); // 始点追加
            while (CEpsUtil::Less(dRate, dEdgeLength))  // 終点が追加されないが次エッジの始点として追加される
            {
                // サンプリング点
                CVector2D pt = dRate * vec + pt1;
                dstPolyline.push_back(BoostPoint(pt.x, pt.y));
                dRate += dInterval;
            }
        }
    }
    return dstPolyline;
}

/*!
 * @brief サンプリング
 * @param[in] polylines  ポリライン群
 * @param[in] dInterval サンプリング間隔
 * @return サンプリング後のポリライン群
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
 * @brief  2D座標の回転
 * @param[in] pos       回転対象座標
 * @param[in] center    回転中心座標
 * @param[in] dAngle    回転角(deg)
 * @return 回転後の座標
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
 * @brief 道路ポリゴンの簡略化
 * @param[in] polygon  道路ポリゴン
 * @return  簡略化後の道路ポリゴン
 * @note    水平線上に位置する頂点を削除する
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
 * @brief 道路ポリゴンの簡略化
 * @param[in] polygons  道路ポリゴン群
 * @return  簡略化後の道路ポリゴン
 * @note    水平線上に位置する頂点を削除する
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
 * @brief 道路縁の簡略化
 * @param[in] line  道路縁
 * @return  簡略化後の道路縁
 * @note    水平線上に位置する頂点を削除する
*/
BoostPolyline CAnalyzeRoadEdgeGeomUtil::Simplify(const BoostPolyline &line)
{
    BoostPolyline newLine;
    newLine.push_back(line.front());    // 始点
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
            // 水平ではない場合は追加
            newLine.push_back(*itPt);
        }
    }
    newLine.push_back(line.back()); // 終点
    return newLine;
}

BoostRing CAnalyzeRoadEdgeGeomUtil::Simplify(const BoostRing &ring)
{
    BoostRing newRing;

    // 開始点は水平ではない頂点とする
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
            // 水平ではない場合
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
            // 水平ではない場合は追加
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
 * @brief 2直線の交点算出
 * @param[in]   vec1        ベクトル1
 * @param[in]   pos1        ベクトル1の始点
 * @param[in]   vec2        ベクトル2
 * @param[in]   pos2        ベクトル2の始点
 * @param[out]  pos         交点
 * @param[out]  bOnLine1    ベクトル1上に交点が存在するか
 * @param[out]  bOnLine2    ベクトル2上に交点が存在するか
 * @param[out]  t           交点位置までのベクトル1のベクトル係数
 * @param[out]  s           交点位置までのベクトル2のベクトル係数
 * @return 算出結果
 * @retval true     成功
 * @retval false    失敗
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

    // vec1に垂直なベクトル
    CVector2D vecVertical;
    if (!CAnalyzeRoadEdgeGeomUtil::GetVerticalVec(vec1, vecVertical))
        return false;

    // 交点探索
    // l(s) = p + su (vec2)
    // m(t) = q + tv (vec1)
    // mに垂直な単位ベクトルn
    // s = n(q - p) / nu
    // 内積nv = 0
    double dTmpVal = CGeoUtil::InnerProduct(vecVertical, vec2);
    if (CEpsUtil::Zero(dTmpVal))
        return false;

    s = CGeoUtil::InnerProduct(vecVertical, pos1 - pos2) / dTmpVal;
    // 交点
    pos = vec2 * s + pos2;

    // 交点が線上に存在するか確認する
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
 * @brief 点から直線に対して垂線を下した際の交点算出
 * @param[in] targetPos 注目点
 * @param[in] vec       直線のベクトル
 * @param[in] pos       直線の始点
 * @param[out] crossPos 交点
 * @param[out] bOnLine  交点が直線上に位置するか否か
 * @param[out] t        始点から交点までのベクトル係数
 * @param[out] dist     注目点と交点までの距離
 * @return 算出結果
 * @retval true     成功
 * @retval false    失敗
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

        // 始点と注目点を繋ぐベクトル
        CVector2D vecTarget = targetPos - pos;

        // 直線ベクトルの単位ベクトル
        CVector2D vecE(vec);
        vecE.Normalize();

        // 射影
        double dInnerProduct = CGeoUtil::InnerProduct(vecTarget, vecE);
        crossPos = vecE * dInnerProduct + pos;
        t = dInnerProduct / dLength;

        // 直線上に交点が位置するか否か
        bOnLine = false;
        if (CEpsUtil::GreaterEqual(dInnerProduct, 0) && CEpsUtil::LessEqual(dInnerProduct, dLength))
        {
            bOnLine = true;
        }

        // 注目点から交点までの距離
        dist = (crossPos - targetPos).Length();

        bRet = true;
    }

    return bRet;
}

/*!
 * @brief 入力ベクトルに垂直な正規化済みベクトルの取得
 * @param[in] vec
 * @param[out] vecVertical
 * @return 作成結果
 * @retval true     成功
 * @retval false    失敗
*/
bool CAnalyzeRoadEdgeGeomUtil::GetVerticalVec(
    const CVector2D &vec,
    CVector2D &vecVertical)
{
    if (CEpsUtil::Zero(vec.Length()))
        return false;

    // vec1に垂直なベクトル
    double dK = sqrt(1.0 / (vec.x * vec.x + vec.y * vec.y));
    vecVertical.x = vec.y * dK;
    vecVertical.y = vec.x * -dK;
    vecVertical.Normalize();

    return true;
}

/*!
 * @brief 座標変換(wolrd -> px)
 * @param[in]   dX          world x座標
 * @param[in]   dY          world y座標
 * @param[in]   dOffsetX    オフセットworld x座標
 * @param[in]   dOffsetY    オフセットworld y座標
 * @param[in]   dReso       解像度(m/px)
 * @param[out]  nX          画像x座標
 * @param[out]  nY          画像y座標
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
 * @brief 座標変換(px -> world)
 * @param[in]   nX          画像x座標
 * @param[in]   nY          画像y座標
 * @param[in]   dOffsetX    オフセットworld x座標
 * @param[in]   dOffsetY    オフセットworld y座標
 * @param[in]   dReso       解像度(m/px)
 * @param[out]  dX          world x座標
 * @param[out]  dY          world y座標
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
 * @brief 座標変換(px -> world)
 * @param[in]   dImgX       画像x座標
 * @param[in]   dImgY       画像y座標
 * @param[in]   dOffsetX    オフセットworld x座標
 * @param[in]   dOffsetY    オフセットworld y座標
 * @param[in]   dReso       解像度(m/px)
 * @param[out]  dWorldX     world x座標
 * @param[out]  dWorldY     world y座標
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
 * @brief 入力頂点をバッファリングしポリゴンとする
 * @param[in]   pt    バッファリング対象頂点
 * @param[in]   dDist バッファ距離(m)
 * @return      ポリゴン(円形)
*/
BoostMultiPolygon CAnalyzeRoadEdgeGeomUtil::Buffering(
    const BoostPoint &pt,
    const double dDist)
{
    assert(CEpsUtil::Greater(dDist, 0));

    // バッファリングストラテジー
    bg::strategy::buffer::distance_symmetric<double> distStrategy(dDist);
    bg::strategy::buffer::join_miter joinStrategy;
    bg::strategy::buffer::end_flat endStrategy;
    bg::strategy::buffer::point_circle pointStrategy;
    bg::strategy::buffer::side_straight sideStrategy;

    // 注目エリア
    BoostMultiPolygon areas;
    bg::buffer(pt, areas, distStrategy, sideStrategy,
        joinStrategy, endStrategy, pointStrategy);

    return areas;
}

/*!
 * @brief 入力ポリライン群をバッファリングしポリゴンとする
 * @param[in]   lines バッファリング対象ポリライン群
 * @param[in]   dDist バッファ距離(m)
 * @return      ポリゴン
*/
BoostMultiPolygon CAnalyzeRoadEdgeGeomUtil::Buffering(
    const BoostMultiLines &lines,
    const double dDist)
{
    assert(CEpsUtil::Greater(dDist, 0));

    // バッファリングストラテジー
    bg::strategy::buffer::distance_symmetric<double> distStrategy(dDist);
    bg::strategy::buffer::join_miter joinStrategy;
    bg::strategy::buffer::end_flat endStrategy;
    bg::strategy::buffer::point_circle pointStrategy;
    bg::strategy::buffer::side_straight sideStrategy;

    // 注目エリア
    BoostMultiPolygon areas;
    bg::buffer(lines, areas, distStrategy, sideStrategy,
        joinStrategy, endStrategy, pointStrategy);

    return areas;
}

/*!
 * @brief 入力ポリラインをバッファリングしポリゴンとする
 * @param[in]   lines バッファリング対象ポリライン
 * @param[in]   dDist バッファ距離(m)
 * @return      ポリゴン
*/
BoostMultiPolygon CAnalyzeRoadEdgeGeomUtil::Buffering(
    const BoostPolyline &line,
    const double dDist)
{
    assert(CEpsUtil::Greater(dDist, 0));

    // バッファリングストラテジー
    bg::strategy::buffer::distance_symmetric<double> distStrategy(dDist);
    bg::strategy::buffer::join_miter joinStrategy;
    bg::strategy::buffer::end_flat endStrategy;
    bg::strategy::buffer::point_circle pointStrategy;
    bg::strategy::buffer::side_straight sideStrategy;

    // 注目エリア
    BoostMultiPolygon areas;
    bg::buffer(line, areas, distStrategy, sideStrategy,
        joinStrategy, endStrategy, pointStrategy);

    return areas;
}

/*!
 * @brief 入力ポリゴンにバッファを付与する
 * @param[in]   polygon バッファリング対象
 * @param[in]   dDist   バッファ距離(m)
 * @return      ポリゴン
*/
BoostPolygon CAnalyzeRoadEdgeGeomUtil::Buffering(
    const BoostPolygon &polygon,
    const double dDist)
{
    // バッファリングストラテジー
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
 * @brief 入力ポリゴンに対してモルフォロジー処理のオープニングに相当する行う
 * @param[in]   polygons    オープニング対象ポリゴン群
 * @param[in]   dDist       バッファ距離(m)
 * @return      ポリゴン群
*/
BoostMultiPolygon CAnalyzeRoadEdgeGeomUtil::Opening(
    const BoostMultiPolygon &polygons,
    const double dDist)
{
    assert(CEpsUtil::Greater(dDist, 0));

    // バッファリングストラテジー
    bg::strategy::buffer::distance_symmetric<double> shrinkDistStrategy(-dDist);
    bg::strategy::buffer::distance_symmetric<double> expansionDistStrategy(dDist);
    bg::strategy::buffer::join_miter joinStrategy;
    bg::strategy::buffer::end_flat endStrategy;
    bg::strategy::buffer::point_circle pointStrategy;
    bg::strategy::buffer::side_straight sideStrategy;

    BoostMultiPolygon srcPolygons, tmpPolygons, correctPolygons, dstPolygons;

    // 水平線上に存在する頂点の削除
    srcPolygons = CAnalyzeRoadEdgeGeomUtil::Simplify(polygons);

    // 収縮
    bg::buffer(
        srcPolygons, tmpPolygons, shrinkDistStrategy, sideStrategy,
        joinStrategy, endStrategy, pointStrategy);
    bg::unique(tmpPolygons);
    bg::correct(tmpPolygons);
    correctPolygons = CAnalyzeRoadEdgeGeomUtil::Simplify(tmpPolygons);

    // 膨張
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
 * @brief   2点間距離
 * @param[in] pt1
 * @param[in] pt2
 * @return  距離
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
 * @brief ポリゴンのエッジ線抽出
 * @param[in] polygons ポリゴン群
 * @return エッジ線群
*/
BoostMultiLines CAnalyzeRoadEdgeGeomUtil::GetEdges(
    const BoostMultiPolygon &polygons)
{
    BoostMultiLines dstLines;
    for (auto itPoly = polygons.cbegin(); itPoly != polygons.cend(); itPoly++)
    {
        // 外輪郭
        BoostPolyline outer;
        for (auto itPt = itPoly->outer().cbegin(); itPt != itPoly->outer().cend(); itPt++)
            outer.push_back(*itPt);
        dstLines.push_back(outer);

        // 穴
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
 * @brief スパイク確認
 * @param[in]   ring            リングデータ
 * @param[out]  vecSpike        スパイク地点の座標
 * @param[out]  spikePts        スパイク地点の座標インデックス配列
 * @return      スパイクの有無
 * @retval      true    スパイク有り
 * @retval      false    スパイク無し
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
            break;  // 早期終了する場合
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
 * @brief スパイク確認
 * @param[in]   polygon     ポリゴン
 * @param[out]  spikePts    スパイク地点の座標
 * @param[in]   bShortCut   早期終了フラグ
 * @return      スパイクの有無
 * @retval      true    スパイク有り
 * @retval      false    スパイク無し
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
        // 早期終了しない場合、または早期終了するが外輪郭にはスパイクが存在しない場合
        for (auto itHole = polygon.inners().begin(); itHole != polygon.inners().end(); itHole++)
        {
            BoostMultiPoints tmpSpikePts;
            if (CAnalyzeRoadEdgeGeomUtil::CheckSpike(*itHole, tmpSpikePts, vecSpikePtIdx))
            {
                spikePts.insert(spikePts.end(), tmpSpikePts.begin(), tmpSpikePts.end());
                if (bShortCut)
                    break;  // 早期終了
            }
        }
    }

    return (spikePts.size() > 0);
}

/*!
 * @brief ポリゴンからエッジ(線分)を取得する
 * @param[in]   polygon ポリゴン
 * @param[out]  edges   取得エッジ群
 * @remarks edgesは内部でclearしない(既存データが挿入されている場合はそのままエッジを追加挿入する)
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
 * @brief ポリゴンから注目領域と重畳するエッジ(線分)を取得する
 * @param[in]   polygon ポリゴン
 * @param[in]   area    注目エリア
 * @param[out]  edges   取得エッジ群
 * @remarks edgesは内部でclearしない(既存データが挿入されている場合はそのままエッジを追加挿入する)
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
            edges.push_back(line);  // 衝突する場合
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
                edges.push_back(line);  // 衝突する場合
        }
    }
}