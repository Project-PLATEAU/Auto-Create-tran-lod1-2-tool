#pragma once
#include <vector>
#include "AnalyzeRoadEdgeCommon.h"
#include "CRoadData.h"
#include "CRoadCenterLine.h"

/*!
 * @brief 道路分割クラス
*/
class CRoadDivision
{
public:
    CRoadDivision() {}  //!< コンストラクタ
    ~CRoadDivision() {} //!< デストラクタ

    // 車道交差部ポリゴンの分割
    void DivisionByCrossing(
        BoostMultiPolygon &roadPolygons,
        BoostMultiPolygon &blockPolyogns,
        std::vector<CCrossingData> &crossing,
        std::vector<CRoadData> &crossingPolygons,
        BoostMultiPolygon &remainingPolygons,
        const double dReso = 0.5);

    // 道路構造変化によるポリゴンの分割(道路橋用)
    void DivisionByStructualChange(
        BoostMultiPolygon &roadPolygons,
        BoostMultiLines &facilities,
        RoadSectionType roadSectionType,
        std::vector<CRoadData> &facilityPolygons,
        BoostMultiPolygon &remainingPolygons,
        const double dOverhangAreaTh = 1.0);

    // 道路構造変化によるポリゴンの分割(トンネル用)
    void DivisionByStructualChange(
        BoostMultiPolygon &roadPolygons,
        std::vector<BoostPairLine> &facilities,
        RoadSectionType roadSectionType,
        std::vector<CRoadData> &facilityPolygons,
        BoostMultiPolygon &remainingPolygons,
        const double dOverhangAreaTh = 1.0);

protected:

private:
    /*!
 * @brief 車道交差部の分割線を作成する際に頂点ペア情報
*/
    struct PairData
    {
        BoostMultiLines::iterator itTarget; // 注目道路縁イテレータ
        BoostMultiLines::iterator itTwin;   // 対向道路縁イテレータ
        bool bTarget;                       // 参照点が始点か否か(注目道路縁)
        bool bTwin;                         // 参照点が始点か否か(対向道路縁)
        int nTargetStep;                    // 注目点イテレータのループ用ステップ数
        int nTwinStep;                      // 対向点イテレータのループ用ステップ数
        BoostPoint prevTargetPt;            // 注目点の前点
        BoostPoint prevTwinPt;              // 対向点の前点

        /*!
         * @brief コンストラクタ
        */
        PairData(
            BoostMultiLines::iterator itTarget,
            bool bTarget,
            int nTargetStep,
            BoostPoint prevTargetPt,
            BoostMultiLines::iterator itTwin,
            bool bTwin,
            int nTwinStep,
            BoostPoint prevTwinPt)
        {
            this->itTarget = itTarget;
            this->bTarget = bTarget;
            this->nTargetStep = nTargetStep;
            this->prevTargetPt = prevTargetPt;
            this->itTwin = itTwin;
            this->bTwin = bTwin;
            this->nTwinStep = nTwinStep;
            this->prevTwinPt = prevTwinPt;
        }

        /*!
         * @brief コピーコンストラクタ
        */
        PairData(const PairData &p) { *this = p; }

        /*!
         * @brief 代入演算子
        */
        PairData &operator =(const PairData &p)
        {
            if (&p != this)
            {
                this->itTarget = p.itTarget;
                this->bTarget = p.bTarget;
                this->nTargetStep = p.nTargetStep;
                this->prevTargetPt = p.prevTargetPt;
                this->itTwin = p.itTwin;
                this->bTwin = p.bTwin;
                this->nTwinStep = p.nTwinStep;
                this->prevTwinPt = p.prevTwinPt;
            }
            return *this;
        }
    };

    /*!
     * @brief 追加頂点情報
    */
    struct AddPointInfo
    {
        BoostPoint pt;                  //!< 挿入点
        size_t offset;                  //!< 挿入位置
        double dLength;                 //!< 挿入順評価用の値
        int nInnerIdx;                  //!< 穴のインデックス(未設定は-1)

        /*!
         * @brief コンストラクタ
         * @param[in] pt            挿入点
         * @param[in] dLength       挿入順評価用の値
         * @param[in] nInnerIdx     穴のインデックス(未設定は-1)
        */
        AddPointInfo(
            BoostPoint &pt,
            size_t dist,
            double dLength,
            int nInnerIdx = -1)
        {
            this->pt = pt;
            this->offset = dist;
            this->dLength = dLength;
            this->nInnerIdx = nInnerIdx;
        }

        /*!
         * @brief コピーコンストラクタ
        */
        AddPointInfo(const AddPointInfo &p) { *this = p; }

        /*!
         * @brief 代入演算子
        */
        AddPointInfo &operator =(const AddPointInfo &p)
        {
            if (&p != this)
            {
                this->pt = p.pt;
                this->offset = p.offset;
                this->dLength = p.dLength;
                this->nInnerIdx = p.nInnerIdx;
            }
            return *this;
        }

        /*!
         * @brief 比較演算子
        */
        bool operator < (const AddPointInfo &p) const
        {
            if (this->nInnerIdx == p.nInnerIdx)
            {
                if (this->offset == p.offset)
                {
                    return this->dLength < p.dLength;
                }
                else
                {
                    return this->offset < p.offset;
                }
            }
            else
            {
                return this->nInnerIdx < p.nInnerIdx;
            }
        }

        /*!
         * @brief 比較演算子
        */
        bool operator > (const AddPointInfo &p) const
        {
            if (this->nInnerIdx == p.nInnerIdx)
            {
                if (this->offset == p.offset)
                {
                    return this->dLength > p.dLength;
                }
                else
                {
                    return this->offset > p.offset;
                }

            }
            else
            {
                return this->nInnerIdx > p.nInnerIdx;
            }
        }
    };

    // 交差点データにボロノイ領域を設定する
    void setVoronoiCellArea(
        BoostMultiPolygon &roadPolyogns,
        std::vector<CCrossingData> &crossing,
        const double dReso);

    // ボロノイセルポリゴンの取得
    void getVoronoiCell(
        const BoostVoronoiDiagram::cell_type &cell,
        std::vector<BoostVoronoiPoint> &pts,
        BoostBox &box,
        BoostPolygon &cellPolygon);

    // 交差点領域の設定
    void setCrossingArea(
        BoostMultiPolygon &roadPolygons,
        std::vector<CCrossingData> &crossing);

    // 交差点近傍道路縁の取得
    BoostMultiLines GetRoadEdges(
        BoostMultiPolygon &roadPolyogns,
        BoostPolygon &area,
        BoostPoint &crossPt);

    // 道路縁(線分)を結合して経路データを作成する
    BoostMultiLines GetRoutes(
        BoostMultiLines &roadEdges);

    // 交差点近傍道路縁の最遠点の探索
    bool SearchTarget(
        BoostMultiLines &routes,
        BoostPoint &crossPt,
        std::set<BoostPolyline::iterator> usedPtList,
        BoostMultiLines::iterator &itTarget,
        BoostPolyline::iterator &itTargetPt,
        CVector2D &targetVec,
        BoostPolyline::iterator &itTargetEnd,
        int &nTargetStep);

    // 交差点近傍道路縁の最遠点に対する対向点の探索
    bool SearchTwin(
        BoostMultiLines &routes,
        BoostMultiLines::iterator &itTarget,
        BoostPolyline::iterator &itTargetPt,
        CVector2D &targetVec,
        std::set<BoostPolyline::iterator> usedPtList,
        BoostMultiLines::iterator &itTwin,
        BoostPolyline::iterator &itTwinPt,
        CVector2D &twinVec,
        BoostPolyline::iterator &itTwinEnd,
        int &nTwinStep);

    // 道路縁短縮処理の前処理
    void roadEdgeShorteningPreProc(
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
        const double dEdgeLengthTh = 0.5);

    // 道路縁短縮処理
    bool roadEdgeShorteningProc(
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
        const double dVerticalAngleDiffTh = 10.0,
        const double dEdgeLengthTh = 0.5);

    // 道路縁短縮後の補正処理
    void correctionRoadEdgeProc(
        BoostMultiLines::iterator &itTarget,
        BoostPolyline::iterator &itTargetPt,
        int nTargetStep,
        BoostPoint &prevTargetPt,
        const double dCorrectionAngleTh = 10.0);

    // 交差点ポリゴンの作成
    BoostPolygon loopingCrossingArea(
        BoostMultiLines crossingOutlines);

    // 車道交差部切断
    void createCrossingPolygons(
        BoostMultiPolygon &roadPolyogns,
        BoostMultiPolygon &blockPolyogns,
        std::vector<CCrossingData> &crossing,
        std::vector<CRoadData> &crossingPolygons,
        BoostMultiPolygon &remainingPolygons);

    // ポリゴンの分割
    void divisionPolygon(
        BoostMultiPolygon &roadPolyogns,
        std::vector<CRoadData> &roadData,
        std::vector<CRoadData> &dstData,
        std::vector<CRoadData> &errData,
        BoostMultiPolygon &remainingPolygons);

    // ポリゴンの減算処理(スパイクノイズ/自己交差確認のみ)
    BoostMultiPolygon difference(
        BoostMultiPolygon &polygons,
        BoostPolygon &polygon,
        bool &bSpike,
        bool &bCross);
};
