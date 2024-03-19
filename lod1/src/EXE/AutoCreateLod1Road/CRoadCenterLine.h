#pragma once

#include "AnalyzeRoadEdgeCommon.h"
#include "CGeoUtil.h"
#include <algorithm>

class CRoadCenterLine
{
public:
    CRoadCenterLine();      //!< �R���X�g���N�^
    ~CRoadCenterLine() {}   //!< �R���X�g���N�^

    /*!
     * @brief ���H���S���̃Q�b�^�[
    */
    BoostMultiLines CenterLines() { return m_centerLines; }

    // �����_�ߖT�̓��H���S�����擾����
    BoostMultiLines CenterLines(
        BoostPoint &crossPt,
        BoostPolygon &area);

    /*!
     * @brief ���`�O���H���S���̃Q�b�^�[
    */
    BoostMultiLines InternalCenterLines() { return m_internalCenterLines; }

    /*!
     * @brief ���k���H�|���S���Q�̃Q�b�^�[(debug)
    */
    BoostMultiPolygon ShrinkRoadPolygons() { return m_shrinkRoadPolygons; }

    /*!
     * @brief �����ςݕH�|���S���Q�̃Q�b�^�[(debug)
    */
    BoostMultiPolygon Cycles() { return m_cycles; }

    // ���H���S���쐬
    void CreateCenterLine(
        const BoostMultiPolygon &roadPolygons,
        const double dReso = 0.5,
        const double dShrink = -0.025);

    // �����_�擾(���W���̂�)
    BoostMultiPoints GetCrossPoints();

    /*!
     * @brief  �����_�擾(���򐔕t��)
     * @return �����_���Q
    */
    std::vector<CCrossingData> GetCrossPointsWithBranchNum() { return m_crossings; };

    // ���ڃG���A�̌����_�擾(���򐔕t��)
    std::vector<CCrossingData> GetCrossPointsWithBranchNum(const BoostBox &area);

protected:

private:
    BoostMultiLines m_centerLines;          //!< ���H���S��
    BoostMultiLines m_internalCenterLines;  //!< ���`�O���H���S��
    BoostMultiPolygon m_shrinkRoadPolygons; //!< ���k���H
    BoostMultiPolygon m_cycles;             //!< ���������H
    std::vector<CCrossingData> m_crossings; //!< �����_�f�[�^
    BoostUndirectedGraph m_graph;           //!< ���H���S���̖����O���t
    BoostVertexRTree m_rtree; //!< �����_�T���pTree

    // ���H���S���쐬����
    void prepareRoadCenterLine(
        const BoostMultiPolygon &roadPolygons,
        const double dSampling,
        BoostMultiLines &roadEdges);

    // �{���m�C�������̎擾
    void getVoronoiEdges(
        const BoostMultiPolygon &roadPolygons,
        const double dReso,
        const double dShrink,
        BoostMultiLines &voronoiEdges);

    // �����O���t�𗘗p�������H���S���̃m�C�Y����
    void deleteNoise(
        BoostUndirectedGraph &graph,
        const double dAngleTh = 100.0);

    // �O�p�`�m�C�Y�̏���
    bool deleteTriangleNoise(
        BoostUndirectedGraph &graph,
        const double dAngleTh = 100.0);

    // �X�p�C�N�m�C�Y�̏���
    bool deleteSpikeNoise(
        BoostUndirectedGraph &graph,
        const double dAngleDiffTh = 5.0,
        const double dLengthTh = 10.0);

    // �O���t�̊ȗ���
    void simplifyGraph(
        BoostUndirectedGraph &graph,
        const double dAngleDiffTh = 1.0);

    // �H�T���p�̕����O���t�̍쐬
    bool createSubGraph(
        BoostDirectedGraph &graph,
        BoostDVertexDesc vertexDesc,
        const double dBuffer,
        BoostDirectedGraph &subGraph,
        std::map<BoostDVertexDesc, BoostDVertexDesc> &mapGlobalToSub,
        std::map<BoostDVertexDesc, BoostDVertexDesc> &mapSubToGlobal);

    // �H�m�C�Y����
    bool deleteCycleNoise(
        BoostDirectedGraph &graph,
        const double dAreaTh = 50.0);

    // �H�m�C�Y����
    bool deleteCycleNoiseUsingSubGraph(
        BoostDirectedGraph &graph,
        const double dBuffer = 20.0,
        const double dAreaTh = 50.0);

    // �L���O���t�̍쐬
    BoostDirectedGraph createDirectedGraph(const BoostUndirectedGraph &undirectedGraph);

    // �����O���t�̍쐬
    BoostUndirectedGraph createUndirectedGraph(const BoostDirectedGraph &directedGraph);


    // �����_�}�[�W
    void mergeCrossing(
        BoostUndirectedGraph &graph,
        const double dDistTh = 5.0);

    // �����_�̃m�C�Y�_����
    std::vector<CCrossingData> deleteNoiseCrossing(
        const BoostUndirectedGraph &graph,
        const BoostMultiPolygon &roadPolygons,
        const double dBuffer);

};

/*!
 * @brief ����
*/
struct CEdge
{
public:
    CVector2D m_start;    //!< �n�_
    CVector2D m_end;      //!< �I�_
    std::vector<CVector2D> m_middlePoints;  //!< �n�I�_�Ԃɑ}�����钆�_��

    /*!
     * @brief �R���X�g���N�^
    */
    CEdge()
    {
        m_start.x = 0;
        m_start.y = 0;
        m_end.x = 0;
        m_end.y = 0;
        m_middlePoints.clear();
    }

    /*!
     * @brief �R���X�g���N�^
     * @param[in] start �n�_
     * @param[in] end   �I�_
    */
    CEdge(const CVector2D &start, const CVector2D &end)
    {
        m_start = start;
        m_end = end;
    }

    /*!
     * @brief �R�s�[�R���X�g���N�^
     * @param[in] edge �R�s�[���G�b�W
    */
    CEdge(const CEdge &edge) { *this = edge; }

    /*!
     * @brief ������Z�q
    */
    CEdge &operator = (const CEdge &edge)
    {
        if (&edge != this)
        {
            m_start = edge.m_start;
            m_end = edge.m_end;
            std::copy(edge.m_middlePoints.begin(), edge.m_middlePoints.end(), std::back_inserter(m_middlePoints));
        }
        return *this;
    }

    /*!
     * @brief ��r���Z�q
    */
    bool operator < (const CEdge &edge) const
    {
        double dLen1 = (m_end - m_start).Length();
        double dLen2 = (edge.m_end - edge.m_start).Length();

        return dLen1 < dLen2;
    }

    /*!
     * @brief ��r���Z�q
    */
    bool operator > (const CEdge &edge) const
    {
        double dLen1 = (m_end - m_start).Length();
        double dLen2 = (edge.m_end - edge.m_start).Length();

        return dLen1 > dLen2;
    }

    CVector2D GetVector() { return m_end - m_start; }   //!< �x�N�g���擾
    std::vector<CVector2D> GetPoints()                  //!< �|�����C���̎擾
    {
        std::vector<CVector2D> pts;
        std::copy(m_middlePoints.begin(), m_middlePoints.end(), std::back_inserter(pts));
        pts.insert(pts.begin(), m_start);
        pts.push_back(m_end);
        return pts;
    }

    /*!
     * @brief ���_�̑��݊m�F
     * @param[in] pt ���_
     * @return ����
     * @retval true     ���꒸�_�����݂���
     * @retval false    ���꒸�_�����݂��Ȃ�
    */
    bool IsExitPt(const CVector2D &pt)
    {
        bool bRet = false;
        std::vector<CVector2D> pts = GetPoints();
        for (auto itPt = pts.begin(); itPt != pts.end(); itPt++)
        {
            if (CEpsUtil::Equal(itPt->x, pt.x) && CEpsUtil::Equal(itPt->y, pt.y))
            {
                bRet = true;
                break;
            }
        }
        return bRet;
    }
protected:
private:


};