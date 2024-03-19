#pragma once

#include <vector>
#include "CGeoUtil.h"
#include "AnalyzeRoadEdgeCommon.h"

/*!
 * @brief 道路縁解析に関するジオメトリ処理のユーティリティ
*/
class CAnalyzeRoadEdgeGeomUtil
{
public:
    // boost用頂点の等号比較
    static bool CheckPointEqual(const BoostPoint &pt1, const BoostPoint &pt2);

    // boost用ポリラインデータ変換
    static BoostPolyline ConvBoostPolyline(
        const std::vector<CVector3D> &vecSrcPolyline);

    // boost用ポリライン集合データ変換
    static BoostMultiLines ConvBoostMultiLines(
        const std::vector<std::vector<CVector3D>> &vecSrcPolylines);

    // 自己交差確認
    static bool CheckSelfIntersection(
        const BoostRing &ring,
        BoostMultiPoints &crossPts,
        std::vector<std::pair<size_t, size_t>> &vecCrossEdgeIdx);

    // 自己交差確認(外輪郭と穴の両方)
    static bool CheckSelfIntersection(
        const BoostPolygon &polygon,
        BoostMultiPoints &crossPts);

    // 自己交差解消
    static std::vector<BoostRing> SelfIntersectionResolution(
        const BoostRing &ring,
        BoostPoint &crossPt,
        std::pair<size_t, size_t> &crossEdgeIdx);

    // 自己交差確認と解消処理
    static void SelfIntersectionResolution(
        const BoostRing &ring,
        std::vector<BoostRing> &vecRings);

    // 自己交差確認と解消処理
    static void SelfIntersectionResolution(
        const BoostPolygon &polygon,
        BoostMultiPolygon &polygons);

    // 四捨五入
    static double RoundN(double dValue, int nDigit);

    // サンプリング
    static void Sampling(
        const CVector2D &start,
        const CVector2D &end,
        std::vector<CVector2D> &pts,
        const double dInterval = 1.0);

    // サンプリング
    static BoostPolyline Sampling(
        const BoostPolyline &polyline, const double dInterval = 1.0);

    // サンプリング
    static BoostMultiLines Sampling(
        const BoostMultiLines &polylines, const double dInterval = 1.0);

    // 2D座標の回転
    static CVector2D Rotate(
        const CVector2D &pos,
        const CVector2D &center,
        const double dAngle);

    // 図形の簡略化(水平線上に存在する頂点を削除する)
    static BoostPolygon Simplify(const BoostPolygon &polygon);

    // 図形の簡略化(水平線上に存在する頂点を削除する)
    static BoostMultiPolygon Simplify(const BoostMultiPolygon &polygons);

    // 図形の簡略化(水平線上に存在する頂点を削除する)
    static BoostPolyline Simplify(const BoostPolyline &line);

    // 図形の簡略化(水平線上に存在する頂点を削除する)
    static BoostRing Simplify(const BoostRing &ring);

    // 2直線の交点算出
    static bool GetCrossPos(
        const CVector2D &vec1,
        const CVector2D &pos1,
        const CVector2D &vec2,
        const CVector2D &pos2,
        CVector2D &pos,
        bool &bOnLine1,
        bool &bOnLine2,
        double &t,
        double &s);

    // 点から直線に対して垂線を下した際の交点算出
    static bool GetCrossPos(
        const CVector2D &targetPos,
        const CVector2D &vec,
        const CVector2D &pos,
        CVector2D &crossPos,
        bool &bOnLine,
        double &t,
        double &dist);

    // 入力ベクトルに垂直なベクトルの取得
    static bool GetVerticalVec(
        const CVector2D &vec,
        CVector2D &vecVertical);

    // 座標変換(wolrd -> px)
    static void ConvertWorldToPx(
        const double dX,
        const double dY,
        const double dOffsetX,
        const double dOffsetY,
        const double dReso,
        int &nX,
        int &nY);

    // 座標変換(px -> world)
    static void ConvertPxToWorld(
        const int nX,
        const int nY,
        const double dOffsetX,
        const double dOffsetY,
        const double dReso,
        double &dX,
        double &dY);

    // 座標変換(px -> world)
    static void ConvertPxToWorld(
        const double dImgX,
        const double dImgY,
        const double dOffsetX,
        const double dOffsetY,
        const double dReso,
        double &dWorldX,
        double &dWorldY);

    // 入力頂点をバッファリングしポリゴンとする
    static BoostMultiPolygon Buffering(
        const BoostPoint &pt,
        const double dDist);

    // 入力ポリライン群をバッファリングしポリゴンとする
    static BoostMultiPolygon Buffering(
        const BoostMultiLines &lines,
        const double dDist);

    // 入力ポリラインをバッファリングしポリゴンとする
    static BoostMultiPolygon Buffering(
        const BoostPolyline &line,
        const double dDist);

    // 入力ポリゴンにバッファを付与する
    static BoostPolygon Buffering(
        const BoostPolygon &polygon,
        const double dDist);

    // 入力ポリゴンに対してモルフォロジー処理のオープニングに相当する行う
    static BoostMultiPolygon Opening(
        const BoostMultiPolygon &polygons,
        const double dDist);

    // 2点間距離
    static double Length(
        const BoostPoint &pt1,
        const BoostPoint &pt2);

    // ポリゴンのエッジ線抽出
    static BoostMultiLines GetEdges(
        const BoostMultiPolygon &polygons);


    // スパイク確認
    static bool CheckSpike(
        const BoostRing &ring,
        BoostMultiPoints &spikePts,
        std::vector<size_t> &vecSpikePtIdx);

    // スパイク確認
    static bool CheckSpike(
        const BoostPolygon &polygon,
        BoostMultiPoints &spikePts,
        const bool bShortCut = false);

    // ポリゴンからエッジ(線分)を取得する
    static void GetEdges(
        BoostPolygon &polygon,
        BoostMultiLines &edges);

    // ポリゴンから注目領域と重畳するエッジ(線分)を取得する
    static void GetEdges(
        BoostPolygon &polygon,
        BoostPolygon &area,
        BoostMultiLines &edges);
};
