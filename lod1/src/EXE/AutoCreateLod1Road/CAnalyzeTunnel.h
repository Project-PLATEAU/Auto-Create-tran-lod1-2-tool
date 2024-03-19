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
 * @brief トンネル探索クラス
*/
class CAnalyzeTunnel
{
public:
	/*!
	 * @brief コンストラクタ
	*/
	CAnalyzeTunnel()
	{

	}

    /*!
     * @brief デストラクタ
    */
	~CAnalyzeTunnel()
	{

	}

    // トンネル探索
    static std::vector<BoostPairLine> Process(
        const std::vector<CDMRoadDataManager::RoadEdgeData> roadEdges,
        const std::vector<CDMRoadDataManager::RoadFacilitiesData> roadFacilities,
        const double dBuffDist = 500.0,
        const double dAngleDiffTh = 1.0);

private:
    typedef std::tuple<BoostPoint, BoostPoint, BoostVertexDesc, BoostVertexDesc> EntranceData;  //!< トンネル入り口データ
};