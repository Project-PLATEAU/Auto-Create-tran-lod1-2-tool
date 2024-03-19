#include "pch.h"
#include "CBoostGraphUtil.h"
#include "CAnalyzeRoadEdgeGeomUtil.h"

/*!
 * @brief �����O���t�̍쐬
 * @param[in] lines �|�����C���Q
 * @return    �����O���t
*/
BoostUndirectedGraph CBoostGraphUtil::CreateGraph(
    BoostMultiLines &lines)
{
    // �����O���t�̍쐬
    BoostUndirectedGraph graph;
    std::vector<BoostVertexDesc> vecDesc;
    for (BoostMultiLines::const_iterator itLine = lines.cbegin();
        itLine != lines.cend(); itLine++)
    {
        BoostVertexDesc prevDesc = 0;
        for (BoostPolyline::const_iterator itPt = itLine->cbegin();
            itPt != itLine->cend(); itPt++)
        {
            // �O���t�����_�ɓ�����W�����݂��Ȃ����T��
            std::vector<BoostVertexDesc>::iterator itTargetDesc;
            for (itTargetDesc = vecDesc.begin(); itTargetDesc != vecDesc.end(); itTargetDesc++)
            {
                if (CAnalyzeRoadEdgeGeomUtil::CheckPointEqual(graph[*itTargetDesc].pt, *itPt))
                {
                    // ���W���������ꍇ
                    break;
                }
            }

            if (itTargetDesc == vecDesc.end())
            {
                // �O���t�����_�ɓ�����W�̒��_���Ȃ��ꍇ�͒ǉ�
                BoostVertexProperty val(*itPt);
                vecDesc.push_back(boost::add_vertex(val, graph));
                itTargetDesc = vecDesc.end() - 1;
                graph[*itTargetDesc].desc = *itTargetDesc;
            }

            if (itPt > itLine->cbegin())
            {
                // �G�b�W�ǉ�
                auto edge = boost::add_edge(prevDesc, *itTargetDesc, graph);
                // �G�b�W�̃v���p�e�B�ɒ��_����R�Â���
                graph[edge.first].vertexDesc1 = prevDesc;
                graph[edge.first].vertexDesc2 = *itTargetDesc;
                graph[edge.first].dLength = CBoostGraphUtil::EdgeLength(graph, edge.first); // �G�b�W��
            }
            prevDesc = *itTargetDesc;   // ���G�b�W�ǉ��p��1�O�̒��_�����X�V
        }
    }

    return graph;
}
