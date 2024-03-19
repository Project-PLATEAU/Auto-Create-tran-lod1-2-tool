#pragma once
#include "AnalyzeRoadEdgeCommon.h"


/*!
 * @brief �O���t�̃��[�e�B���e�B�N���X
*/
class CBoostGraphUtil
{
public:
    /*!
     * @brief �G�b�W�̒���
     * @param[in] graph     �����O���t
     * @param[in] edgeDesc  �G�b�W�̃f�X�N���v�^�[
     * @return    �G�b�W�̒���
    */
    static double EdgeLength(const BoostUndirectedGraph &graph, BoostEdgeDesc &edgeDesc)
    {
        BoostPolyline line;
        line.push_back(graph[graph[edgeDesc].vertexDesc1].pt);
        line.push_back(graph[graph[edgeDesc].vertexDesc2].pt);
        return bg::length(line);
    }

    // �����O���t�̍쐬
    static BoostUndirectedGraph CreateGraph(
        BoostMultiLines &lines);
};
