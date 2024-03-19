#pragma once

#include <vector>
#include <mutex>
#include "CGeoUtil.h"
#include "AnalyzeRoadEdgeCommon.h"
#include "CDMRoadDataManager.h"
#include "CRoadData.h"
#include "CRoadCenterLine.h"
#include "CQueue.h"
#include "boost/format.hpp"

/*!
 * @brief �r������t���C���f�b�N�X���X�g
*/
class CProcessingSet
{
public:
    CProcessingSet() {};    //!< �R���X�g���N�^

    /*!
     * @brief �R���X�g���N�^
    */
    CProcessingSet(CProcessingSet const &other)
    {
        std::lock_guard<std::mutex> lock(other.m_mutex);
        m_set = other.m_set;
    }

    CProcessingSet &operator=(const CProcessingSet &) = delete;  // ������Z�ɂ��R�s�[�̋֎~
    ~CProcessingSet() {};   //!< �f�X�g���N�^

    /*!
     * @brief �ǉ�
     * @param[in] val ���͒l
    */
    void insert(int val)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_set.insert(val);
    }

    /*!
     * @brief �ǉ�
     * @param[in]   val ���͒l
     * @param[out]  strMsg  �i�[���f�[�^�̈ꗗ
    */
    void insert(int val, std::string &strMsg)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_set.insert(val);
        strMsg = _print();
    }

    /*!
     * @brief �폜
     * @param[in] key   �l
    */
    void erase(int &key)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_set.find(key);
        if (it != m_set.end())
            m_set.erase(it);
    }

    /*!
     * @brief �폜
     * @param[in]   key     �l
     * @param[out]  strMsg  �i�[���f�[�^�̈ꗗ
    */
    void erase(int &key, std::string &strMsg)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_set.find(key);
        if (it != m_set.end())
            m_set.erase(it);

        strMsg = _print();
    }

    /*!
     * @brief  �󔻒�
     * @return ���茋��
     * @retval true     �f�[�^�Ȃ�
     * @retval false    �f�[�^����
    */
    bool empty() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_set.empty();
    }

    /*!
     * @brief �f�[�^��
     * @return �f�[�^��
    */
    size_t size() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_set.size();
    }

    /*!
     * @brief �\���p������
     * @return �C���f�b�N�X�ꗗ(������)
    */
    std::string print() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return _print();
    }

protected:
private:
    mutable std::mutex m_mutex;     //!< �r������p
    std::set<int> m_set;            //!< �C���f�b�N�X�i�[�p

    std::string _print() const
    {
        std::string strMsg;
        for (auto it = m_set.cbegin(); it != m_set.cend(); it++)
        {
            if (it == m_set.cbegin())
            {
                strMsg += (boost::format("%d") % *it).str();
            }
            else
            {
                strMsg += (boost::format(", %d") % *it).str();
            }
        }

        if (strMsg.empty())
            strMsg = "None";

        return strMsg;
    }
};


class CAnalyzeRoadEdgeManager
{
public:
    /*!
     * @brief �R���X�g���N�^
    */
    CAnalyzeRoadEdgeManager()
    {
        m_vecOutputRoadData.clear();
        m_roadEdges.clear();
        m_bridges.clear();
        m_tunnels.clear();
        m_errMsg.clear();
        m_dInputMinX = 0;
        m_dInputMinY = 0;
        m_dProcWidth = 1500.0;
        m_dProcHeight = 2000.0;
        m_nRow = 0;
        m_nColumn = 0;

#ifdef _OUTPUT_MODEL_INTERMEDIATE_FILE
        m_strDebugFolderPath = "";
#endif
    }
    ~CAnalyzeRoadEdgeManager() {} //!< �f�X�g���N�^

    // ���H�����
    void Process(
        const std::vector<std::vector<CVector3D>> &vecRoadEdges,
        const std::vector<std::vector<CVector3D>> &vecBridges,
        const std::vector<BoostPairLine> &vecTunnels,
        const double dProcWidth = 1500.0, const double dProcHeight = 2000.0);

    // ���H�|���S���o��
    bool OutputResultFile();

protected:

private:
    std::vector<CRoadData> m_vecOutputRoadData;     //!< ���H�|���S���f�[�^(shp�o�͗p)
    std::vector<std::vector<std::string>> m_errMsg; //!< �G���[�`�F�b�N����

    BoostMultiLines m_roadEdges;                    //!< ���͓��H��
    BoostMultiLines m_bridges;                      //!< ���͓��H��
    std::vector<BoostPairLine> m_tunnels;           //!< ���̓g���l��
    std::mutex m_mutex;                             //!< �r������p
    CQueue<std::pair<int, int>> m_regions;          //!< �����Ώۏ��̈�
    CProcessingSet m_processingIdxs;                //!< �������C���f�b�N�X���X�g

    double m_dInputMinX;        //!< ���͓��H���̍ŏ�x���W
    double m_dInputMinY;        //!< ���͓��H���̍ŏ�y���W
    double m_dProcWidth;        //!< ���ڗ̈敝(m)
    double m_dProcHeight;       //!< ���ڗ̈捂��(m)
    int m_nRow;                 //!< ���ڗ̈�̍s��
    int m_nColumn;              //!< ���ڗ̈�̗�

#ifdef _OUTPUT_MODEL_INTERMEDIATE_FILE
    std::string m_strDebugFolderPath;               //!< �f�o�b�O�p�t�H���_�p�X
#endif

    // �����̈搔�̎Z�o
    void calcProcNum(
        const BoostMultiLines &roadEdges,
        const double dProcWidth,
        const double dProcHeight,
        int &nRow,
        int &nColumn,
        double &dMinX,
        double &dMinY);

    // ���H�|���S���o��
    bool outputRoadPolygons(
        const std::string &strShpPath,
        std::vector<CRoadData> polygons,
        const bool bHole = false);

    // �}���`�X���b�h�p���H����͏���
    void analyze();

    // ���H����͊J�n���O
    void startAnalysis(int nTarget, int total);

    // ���H����͏I�����O
    void stopAnalysis(int nTarget, int total, bool bError);
};

/*!
 * @brief ���H����̓N���X
*/
class CAnalyzeRoadEdge
{
public:
    /*!
     * @brief �R���X�g���N�^
    */
    CAnalyzeRoadEdge()
    {
#ifdef _OUTPUT_MODEL_INTERMEDIATE_FILE
        m_strDebugFolderPath = "";
#endif
    }

    /*!
     * @brief �R���X�g���N�^(debug�p)
    */
    CAnalyzeRoadEdge(std::string &strDebugFolderPath)
    {
#ifdef _OUTPUT_MODEL_INTERMEDIATE_FILE
        m_strDebugFolderPath = strDebugFolderPath;
#endif
    }
    ~CAnalyzeRoadEdge() {} //!< �f�X�g���N�^

    // ���H�����
    std::vector<CRoadData> Process(
        std::vector<std::vector<std::string>> &errMsg,
        BoostMultiLines &targetRoadEdges,
        BoostMultiLines &targetBridges,
        std::vector<BoostPairLine> &targetTunnels,
        BoostBox &targetProcArea,
        const double dInputMinX,
        const double dInputMinY,
        const double dProcWidth,
        const double dProcHeight,
        const int nRow,
        const int nColumn,
        const int nX,
        const int nY);

    // �����Ώۃf�[�^�̒��o
    static BoostMultiLines ExtractTargetData(
        const BoostBox &area,
        const BoostMultiLines &multiLines,
        const bool bClip = false);

    // �����Ώۃf�[�^�̒��o
    static std::vector<BoostPairLine> ExtractTargetData(
        const BoostBox &area,
        const std::vector<BoostPairLine> &pairLines,
        const bool bClip = false);

    // �����Ώۃf�[�^�̒��o
    static BoostMultiLines ExtractTargetData(
        const BoostMultiPolygon &area,
        const BoostMultiLines &multiLines,
        const bool bClip = false);

protected:

private:

#ifdef _OUTPUT_MODEL_INTERMEDIATE_FILE
    std::string m_strDebugFolderPath;           //!< �f�o�b�O�p�t�H���_�p�X
#endif
    // �|�����C�����`����
    void shapingRoadEdge(
        BoostPolyline &polyline,
        const double dLengthTh = 0.10,
        const double dAngleTh = 90.0);

    // ���H���Əd�􂷂�T���Ώې��̎擾
    BoostMultiLines getOverlapEdges(
        const BoostMultiLines &roadEdges,
        const BoostMultiLines &searchLines,
        const double dBufDist = 0.01,
        const double dLengthTh = 0.10,
        const double dLengthDiffTh = 0.10);

    // ���H���Əd�􂷂�T���Ώې��̎擾
    std::vector<BoostPairLine> getOverlapEdges(
        const BoostMultiLines &roadEdges,
        const std::vector<BoostPairLine> &searchLines,
        const double dBufDist = 0.01,
        const double dLengthTh = 0.10,
        const double dLengthDiffTh = 0.10);

    // ���̌����T��
    void searchMultiLevelCrossing(
        BoostMultiLines &roadEdges,
        const BoostMultiLines &bridges,
        const std::vector<BoostPairLine> &tunnels,
        BoostMultiLines &upperRoadEdge,
        BoostMultiLines &middleRoadEdge,
        std::vector<BoostPairLine> &lowerRoadEdge,
        const double dLengthTh = 0.10);

    // ���H���̃��[�v���������̊O�֊s�␳����
    void outerRingCorrection(
        std::vector<BoostPoint> &vecPolyline,
        const BoostPolygon &outlinePolygon);

    // ���H���̃��[�v��
    void looping(
        BoostMultiLines &roadedge,
        BoostMultiPolygon &blocks,
        BoostMultiPolygon &errBlocks,
        const double dAreaTh = 1.0,
        const double dOpeningBuffer = 0.1);

    // �I�[�v�j���O����
    BoostMultiPolygon opening(
        BoostPolygon &polygon,
        const double dBuffer);

    // �|���S���̌��Z����(�X�p�C�N�m�C�Y/���Ȍ��������@�\�t��)
    BoostMultiPolygon difference(
        BoostMultiPolygon &polygons,
        BoostPolygon &polygon,
        const double dOpeningBuffer = 0.1);

    // ���H�|���S���̍쐬
    void createRoadPolygon(
        const BoostMultiLines &roadedges,
        const BoostMultiPolygon &blocks,
        BoostMultiPolygon &roadPolygons,
        BoostPolygon &concaveHull,
        const double dSampling = 1.0,
        const double dAreaTh = 1.0,
        const double dOpeningBuffer = 0.1);

    // shp�o�͏����p�̒��ڃG���A�ƋߖT�G���A�̍쐬
    void createAreas(
        const int nRow,
        const int nColumn,
        const double dProcWidth,
        const double dProcHeight,
        const double dInputMinX,
        const double dInputMinY,
        const int nX,
        const int nY,
        BoostMultiPolygon &areas,
        int &nTargetIdx);

    // �ʐϐ�L���̊m�F
    int checkAreaOccupancyRate(
        const BoostPolygon &target,
        const BoostMultiPolygon &areas);

    // ���ڃG���A�̃|���S�����擾����
    void selectPolygon(
        std::vector<CRoadData> &roadData,
        const int nRow,
        const int nColumn,
        const double dProcWidth,
        const double dProcHeight,
        const double dInputMinX,
        const double dInputMinY,
        const int nX,
        const int nY,
        std::vector<CRoadData> &dstData);

    // ���ڃG���A�̃|���S�����擾����
    void selectPolygon(
        BoostMultiPolygon &polygons,
        const int nRow,
        const int nColumn,
        const double dProcWidth,
        const double dProcHeight,
        const double dInputMinX,
        const double dInputMinY,
        const int nX,
        const int nY,
        std::vector<CRoadData> &dstData);

    // �G���[�`�F�b�N
    std::vector<std::vector<std::string>> errorCheck(
        std::vector<CRoadData> &roadData,
        BoostMultiPolygon &roadPolygons,
        std::vector<CCrossingData> &crosses,
        BoostBox &targetArea);

};

