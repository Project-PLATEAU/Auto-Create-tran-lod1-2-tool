// AutoCreateLod1Road.cpp : このファイルには 'main' 関数が含まれています。プログラム実行の開始と終了がそこで行われます。
//

#include <iostream>
#include "pch.h"
#include "StringEx.h"
#include "CDMRoadDataManager.h"
#include "CReadParamFile.h"
#include "CAnalyzeRoadEdge.h"
#include "CStopWatch.h"
#include "COutputSetting.h"
#include "CAnalyzeTunnel.h"

int main(int argc, char *argv[])
{
    const std::string strErrFileName = "ErrorInfo";

    if (argc != 2)
    {
        // パラメータエラー
        std::cout << "Usage : AutoCreateLod1Road.exe create_param.txt" << std::endl;
        return -1;
    }

    CStopWatch sw;
    sw.Start();

    // パラメータファイル読み込み
    std::cout << "Read parameter file." << std::endl;
    std::string strParamPath(argv[1]);
    if (!GetCreateParam()->Initialize(strParamPath))
    {
        // 失敗
        std::cout << "Error : Read parameter file." << std::endl;
        return -1;
    }

    // 出力ファイル設定
    std::cout << "Setting output file path." << std::endl;
    std::string strFileName;
    CFileUtil::SplitPath(GetCreateParam()->GetRoadSHPPath(), NULL, NULL, &strFileName, NULL);
    if (!GetOutputSetting()->Initialize(
        GetCreateParam()->GetOutputFolderPath(),
        strFileName, strErrFileName))
    {
        // 失敗
        std::cout << "Error : Setting output file path." << std::endl;
        return -1;
    }

    // shapeファイル読み込み
    std::cout << "Read road edge shape file." << std::endl;
    CDMRoadDataManager roadEdgeShpMng = CDMRoadDataManager();
    if (!roadEdgeShpMng.ReadRoadEdgeShapeFile())
    {
        // 失敗
        std::cout << "Error : Read road edge shape file." << std::endl;
        return -1;
    }
    std::cout << "Read road facilities shape file." << std::endl;
    roadEdgeShpMng.ReadRoadFacilitiesShapeFile();

    // 道路縁形状の抽出
    std::cout << "Filtering road edge." << std::endl;
    std::vector<std::vector<CVector3D>> roadEdgePolylines;
    std::vector<CDMRoadDataManager::RoadEdgeData> roadEdges = roadEdgeShpMng.GetRoadEdges();
    for (std::vector<CDMRoadDataManager::RoadEdgeData>::iterator it = roadEdges.begin();
        it != roadEdges.end(); it++)
    {
        if (it->nRoadCode == DMRoadCode::ROAD_EDGE)
        {
            // 街区線を取得
            roadEdgePolylines.push_back(it->vecPolyline);
        }
    }
    // 高架橋, トンネル形状の抽出
    std::cout << "Filtering road facilities." << std::endl;
    std::vector<std::vector<CVector3D>> bridgePolylines;
    std::vector<CDMRoadDataManager::RoadFacilitiesData> tunnelFacilities;
    std::vector<CDMRoadDataManager::RoadFacilitiesData> roadFacilities = roadEdgeShpMng.GetRoadFacilities();
    for (std::vector<CDMRoadDataManager::RoadFacilitiesData>::iterator it = roadFacilities.begin();
        it != roadFacilities.end(); it++)
    {
        if (it->nRoadFacilitiesCode == DMRoadFacilitiesCode::ROAD_BRIDGE)
        {
            // 道路橋(高架部)
            if (it->nGeometryType == DMGeometryType::UNCLASSIFIED)
            {
                // 非区分を取得
                bridgePolylines.push_back(it->vecPolyline);
            }
        }
        else if (it->nRoadFacilitiesCode == DMRoadFacilitiesCode::ROAD_TUNNELS)
        {
            // トンネル(点, 線, 面混在状態)
            if (it->nGeometryType == DMGeometryType::UNCLASSIFIED)
            {
                // 非区分を取得
                tunnelFacilities.push_back(*it);
            }
        }
    }
    // トンネル坑口からトンネル範囲の道路縁データを作成する
    std::vector<BoostPairLine> tunnelPolylines = CAnalyzeTunnel::Process(roadEdges, tunnelFacilities);

    // 道路縁解析
    std::cout << "Analyze road edge." << std::endl;
    CAnalyzeRoadEdgeManager analyzeRoadEdgeMng = CAnalyzeRoadEdgeManager();
    analyzeRoadEdgeMng.Process(roadEdgePolylines, bridgePolylines, tunnelPolylines,
        static_cast<double>(GetCreateParam()->GetRegionWidth()),
        static_cast<double>(GetCreateParam()->GetRegionHeight()));

    // shpファイル出力
    std::cout << "Output shape file." << std::endl;
    if (!analyzeRoadEdgeMng.OutputResultFile())
    {
        // 失敗
        std::cout << "Error : Output shape file." << std::endl;
        return -1;
    }

    sw.Stop();
    int nDay, nHour, nMin, nSec;
    sw.GetTime(nDay, nHour, nMin, nSec);
    std::cout << "Process Time : ";
    if (nDay > 0)
        std::cout << nDay << " days ";
    std::cout << nHour << ":" << nMin << ":" << nSec << std::endl;
}
