#pragma once

#include <vector>
#include "CGeoUtil.h"
#include "AnalyzeRoadEdgeCommon.h"

/*!
 * @brief ���H����͂Ɋւ���W�I���g�������̃��[�e�B���e�B
*/
class CAnalyzeRoadEdgeGeomUtil
{
public:
    // boost�p���_�̓�����r
    static bool CheckPointEqual(const BoostPoint &pt1, const BoostPoint &pt2);

    // boost�p�|�����C���f�[�^�ϊ�
    static BoostPolyline ConvBoostPolyline(
        const std::vector<CVector3D> &vecSrcPolyline);

    // boost�p�|�����C���W���f�[�^�ϊ�
    static BoostMultiLines ConvBoostMultiLines(
        const std::vector<std::vector<CVector3D>> &vecSrcPolylines);

    // ���Ȍ����m�F
    static bool CheckSelfIntersection(
        const BoostRing &ring,
        BoostMultiPoints &crossPts,
        std::vector<std::pair<size_t, size_t>> &vecCrossEdgeIdx);

    // ���Ȍ����m�F(�O�֊s�ƌ��̗���)
    static bool CheckSelfIntersection(
        const BoostPolygon &polygon,
        BoostMultiPoints &crossPts);

    // ���Ȍ�������
    static std::vector<BoostRing> SelfIntersectionResolution(
        const BoostRing &ring,
        BoostPoint &crossPt,
        std::pair<size_t, size_t> &crossEdgeIdx);

    // ���Ȍ����m�F�Ɖ�������
    static void SelfIntersectionResolution(
        const BoostRing &ring,
        std::vector<BoostRing> &vecRings);

    // ���Ȍ����m�F�Ɖ�������
    static void SelfIntersectionResolution(
        const BoostPolygon &polygon,
        BoostMultiPolygon &polygons);

    // �l�̌ܓ�
    static double RoundN(double dValue, int nDigit);

    // �T���v�����O
    static void Sampling(
        const CVector2D &start,
        const CVector2D &end,
        std::vector<CVector2D> &pts,
        const double dInterval = 1.0);

    // �T���v�����O
    static BoostPolyline Sampling(
        const BoostPolyline &polyline, const double dInterval = 1.0);

    // �T���v�����O
    static BoostMultiLines Sampling(
        const BoostMultiLines &polylines, const double dInterval = 1.0);

    // 2D���W�̉�]
    static CVector2D Rotate(
        const CVector2D &pos,
        const CVector2D &center,
        const double dAngle);

    // �}�`�̊ȗ���(��������ɑ��݂��钸�_���폜����)
    static BoostPolygon Simplify(const BoostPolygon &polygon);

    // �}�`�̊ȗ���(��������ɑ��݂��钸�_���폜����)
    static BoostMultiPolygon Simplify(const BoostMultiPolygon &polygons);

    // �}�`�̊ȗ���(��������ɑ��݂��钸�_���폜����)
    static BoostPolyline Simplify(const BoostPolyline &line);

    // �}�`�̊ȗ���(��������ɑ��݂��钸�_���폜����)
    static BoostRing Simplify(const BoostRing &ring);

    // 2�����̌�_�Z�o
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

    // �_���璼���ɑ΂��Đ������������ۂ̌�_�Z�o
    static bool GetCrossPos(
        const CVector2D &targetPos,
        const CVector2D &vec,
        const CVector2D &pos,
        CVector2D &crossPos,
        bool &bOnLine,
        double &t,
        double &dist);

    // ���̓x�N�g���ɐ����ȃx�N�g���̎擾
    static bool GetVerticalVec(
        const CVector2D &vec,
        CVector2D &vecVertical);

    // ���W�ϊ�(wolrd -> px)
    static void ConvertWorldToPx(
        const double dX,
        const double dY,
        const double dOffsetX,
        const double dOffsetY,
        const double dReso,
        int &nX,
        int &nY);

    // ���W�ϊ�(px -> world)
    static void ConvertPxToWorld(
        const int nX,
        const int nY,
        const double dOffsetX,
        const double dOffsetY,
        const double dReso,
        double &dX,
        double &dY);

    // ���W�ϊ�(px -> world)
    static void ConvertPxToWorld(
        const double dImgX,
        const double dImgY,
        const double dOffsetX,
        const double dOffsetY,
        const double dReso,
        double &dWorldX,
        double &dWorldY);

    // ���͒��_���o�b�t�@�����O���|���S���Ƃ���
    static BoostMultiPolygon Buffering(
        const BoostPoint &pt,
        const double dDist);

    // ���̓|�����C���Q���o�b�t�@�����O���|���S���Ƃ���
    static BoostMultiPolygon Buffering(
        const BoostMultiLines &lines,
        const double dDist);

    // ���̓|�����C�����o�b�t�@�����O���|���S���Ƃ���
    static BoostMultiPolygon Buffering(
        const BoostPolyline &line,
        const double dDist);

    // ���̓|���S���Ƀo�b�t�@��t�^����
    static BoostPolygon Buffering(
        const BoostPolygon &polygon,
        const double dDist);

    // ���̓|���S���ɑ΂��ă����t�H���W�[�����̃I�[�v�j���O�ɑ�������s��
    static BoostMultiPolygon Opening(
        const BoostMultiPolygon &polygons,
        const double dDist);

    // 2�_�ԋ���
    static double Length(
        const BoostPoint &pt1,
        const BoostPoint &pt2);

    // �|���S���̃G�b�W�����o
    static BoostMultiLines GetEdges(
        const BoostMultiPolygon &polygons);


    // �X�p�C�N�m�F
    static bool CheckSpike(
        const BoostRing &ring,
        BoostMultiPoints &spikePts,
        std::vector<size_t> &vecSpikePtIdx);

    // �X�p�C�N�m�F
    static bool CheckSpike(
        const BoostPolygon &polygon,
        BoostMultiPoints &spikePts,
        const bool bShortCut = false);

    // �|���S������G�b�W(����)���擾����
    static void GetEdges(
        BoostPolygon &polygon,
        BoostMultiLines &edges);

    // �|���S�����璍�ڗ̈�Əd�􂷂�G�b�W(����)���擾����
    static void GetEdges(
        BoostPolygon &polygon,
        BoostPolygon &area,
        BoostMultiLines &edges);
};
