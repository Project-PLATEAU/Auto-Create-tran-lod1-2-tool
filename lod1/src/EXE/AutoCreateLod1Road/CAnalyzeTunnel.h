#pragma once
#include <iostream>
#include <vector>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>

#include <CGeoUtil.h>
#include "CDMRoadDataManager.h"
#include "AnalyzeRoadEdgeCommon.h"
#include "CAnalyzeRoadEdgeGeomUtil.h"
#include "CBoostGraphUtil.h"
#include "CTime.h"

/*!
 * @brief �g���l���T���N���X
*/
class CAnalyzeTunnel
{
public:
	/*!
	 * @brief �R���X�g���N�^
	*/
	CAnalyzeTunnel()
	{

	}

    /*!
     * @brief �f�X�g���N�^
    */
	~CAnalyzeTunnel()
	{

	}

    // �g���l���T��
    static std::vector<BoostPairLine> Process(
        const std::vector<CDMRoadDataManager::RoadEdgeData> roadEdges,
        const std::vector<CDMRoadDataManager::RoadFacilitiesData> roadFacilities,
        const double dBuffDist = 500.0,
        const double dAngleDiffTh = 1.0);

private:
    typedef std::tuple<BoostPoint, BoostPoint, BoostVertexDesc, BoostVertexDesc> EntranceData;  //!< �g���l��������f�[�^
};