#pragma once
#include <vector>
#include "AnalyzeRoadEdgeCommon.h"
#include "CRoadData.h"
#include "CRoadCenterLine.h"

/*!
 * @brief ���H�����N���X
*/
class CRoadDivision
{
public:
    CRoadDivision() {}  //!< �R���X�g���N�^
    ~CRoadDivision() {} //!< �f�X�g���N�^

    // �ԓ��������|���S���̕���
    void DivisionByCrossing(
        BoostMultiPolygon &roadPolygons,
        BoostMultiPolygon &blockPolyogns,
        std::vector<CCrossingData> &crossing,
        std::vector<CRoadData> &crossingPolygons,
        BoostMultiPolygon &remainingPolygons,
        const double dReso = 0.5);

    // ���H�\���ω��ɂ��|���S���̕���(���H���p)
    void DivisionByStructualChange(
        BoostMultiPolygon &roadPolygons,
        BoostMultiLines &facilities,
        RoadSectionType roadSectionType,
        std::vector<CRoadData> &facilityPolygons,
        BoostMultiPolygon &remainingPolygons,
        const double dOverhangAreaTh = 1.0);

    // ���H�\���ω��ɂ��|���S���̕���(�g���l���p)
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
 * @brief �ԓ��������̕��������쐬����ۂɒ��_�y�A���
*/
    struct PairData
    {
        BoostMultiLines::iterator itTarget; // ���ړ��H���C�e���[�^
        BoostMultiLines::iterator itTwin;   // �Ό����H���C�e���[�^
        bool bTarget;                       // �Q�Ɠ_���n�_���ۂ�(���ړ��H��)
        bool bTwin;                         // �Q�Ɠ_���n�_���ۂ�(�Ό����H��)
        int nTargetStep;                    // ���ړ_�C�e���[�^�̃��[�v�p�X�e�b�v��
        int nTwinStep;                      // �Ό��_�C�e���[�^�̃��[�v�p�X�e�b�v��
        BoostPoint prevTargetPt;            // ���ړ_�̑O�_
        BoostPoint prevTwinPt;              // �Ό��_�̑O�_

        /*!
         * @brief �R���X�g���N�^
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
         * @brief �R�s�[�R���X�g���N�^
        */
        PairData(const PairData &p) { *this = p; }

        /*!
         * @brief ������Z�q
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
     * @brief �ǉ����_���
    */
    struct AddPointInfo
    {
        BoostPoint pt;                  //!< �}���_
        size_t offset;                  //!< �}���ʒu
        double dLength;                 //!< �}�����]���p�̒l
        int nInnerIdx;                  //!< ���̃C���f�b�N�X(���ݒ��-1)

        /*!
         * @brief �R���X�g���N�^
         * @param[in] pt            �}���_
         * @param[in] dLength       �}�����]���p�̒l
         * @param[in] nInnerIdx     ���̃C���f�b�N�X(���ݒ��-1)
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
         * @brief �R�s�[�R���X�g���N�^
        */
        AddPointInfo(const AddPointInfo &p) { *this = p; }

        /*!
         * @brief ������Z�q
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
         * @brief ��r���Z�q
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
         * @brief ��r���Z�q
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

    // �����_�f�[�^�Ƀ{���m�C�̈��ݒ肷��
    void setVoronoiCellArea(
        BoostMultiPolygon &roadPolyogns,
        std::vector<CCrossingData> &crossing,
        const double dReso);

    // �{���m�C�Z���|���S���̎擾
    void getVoronoiCell(
        const BoostVoronoiDiagram::cell_type &cell,
        std::vector<BoostVoronoiPoint> &pts,
        BoostBox &box,
        BoostPolygon &cellPolygon);

    // �����_�̈�̐ݒ�
    void setCrossingArea(
        BoostMultiPolygon &roadPolygons,
        std::vector<CCrossingData> &crossing);

    // �����_�ߖT���H���̎擾
    BoostMultiLines GetRoadEdges(
        BoostMultiPolygon &roadPolyogns,
        BoostPolygon &area,
        BoostPoint &crossPt);

    // ���H��(����)���������Čo�H�f�[�^���쐬����
    BoostMultiLines GetRoutes(
        BoostMultiLines &roadEdges);

    // �����_�ߖT���H���̍ŉ��_�̒T��
    bool SearchTarget(
        BoostMultiLines &routes,
        BoostPoint &crossPt,
        std::set<BoostPolyline::iterator> usedPtList,
        BoostMultiLines::iterator &itTarget,
        BoostPolyline::iterator &itTargetPt,
        CVector2D &targetVec,
        BoostPolyline::iterator &itTargetEnd,
        int &nTargetStep);

    // �����_�ߖT���H���̍ŉ��_�ɑ΂���Ό��_�̒T��
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

    // ���H���Z�k�����̑O����
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

    // ���H���Z�k����
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

    // ���H���Z�k��̕␳����
    void correctionRoadEdgeProc(
        BoostMultiLines::iterator &itTarget,
        BoostPolyline::iterator &itTargetPt,
        int nTargetStep,
        BoostPoint &prevTargetPt,
        const double dCorrectionAngleTh = 10.0);

    // �����_�|���S���̍쐬
    BoostPolygon loopingCrossingArea(
        BoostMultiLines crossingOutlines);

    // �ԓ��������ؒf
    void createCrossingPolygons(
        BoostMultiPolygon &roadPolyogns,
        BoostMultiPolygon &blockPolyogns,
        std::vector<CCrossingData> &crossing,
        std::vector<CRoadData> &crossingPolygons,
        BoostMultiPolygon &remainingPolygons);

    // �|���S���̕���
    void divisionPolygon(
        BoostMultiPolygon &roadPolyogns,
        std::vector<CRoadData> &roadData,
        std::vector<CRoadData> &dstData,
        std::vector<CRoadData> &errData,
        BoostMultiPolygon &remainingPolygons);

    // �|���S���̌��Z����(�X�p�C�N�m�C�Y/���Ȍ����m�F�̂�)
    BoostMultiPolygon difference(
        BoostMultiPolygon &polygons,
        BoostPolygon &polygon,
        bool &bSpike,
        bool &bCross);
};
