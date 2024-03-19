#include "pch.h"
#include "RoadModelErrorChecker.h"
#include "CShapeManager.h"
#include <boost/assign.hpp>

//#define DEBUG

std::string strDebugFolerPath;

RoadModelErrorChecker::RoadModelErrorChecker()
{
}

RoadModelErrorChecker::~RoadModelErrorChecker()
{
}

void RoadModelErrorChecker::TestRun(double minArea, double maxDistance)
{
	std::cout << "RoadModelErrorChecker::TestRun()" << std::endl;

	BoostMultiPolygon dummySrcData;
	std::vector<CreatedRoadModelInfo> dummyDstData;

    CShapeWriter writer;
    CAnalyzeRoadEdgeDebugUtil debugUtil = CAnalyzeRoadEdgeDebugUtil();
	std::string strTime = CTime::GetCurrentTime().Format("%Y%m%d_%H%M%S");
	std::string strCurrentPath = CAnalyzeRoadEdgeDebugUtil::GetCurrentPath();
	strDebugFolerPath = CFileUtil::Combine(strCurrentPath, strTime);
	CAnalyzeRoadEdgeDebugUtil::CreateFolder(strDebugFolerPath);

	BoostPolygon srcPolygon;
	bg::append(srcPolygon.outer(), BoostPoint(10, 100));	bg::append(srcPolygon.outer(), BoostPoint(100, 100));	bg::append(srcPolygon.outer(), BoostPoint(100, 90));
	bg::append(srcPolygon.outer(), BoostPoint(110, 90));	bg::append(srcPolygon.outer(), BoostPoint(110, 100));	bg::append(srcPolygon.outer(), BoostPoint(200, 100));
	bg::append(srcPolygon.outer(), BoostPoint(210, 100));	bg::append(srcPolygon.outer(), BoostPoint(210, 90));	bg::append(srcPolygon.outer(), BoostPoint(220, 90));
	bg::append(srcPolygon.outer(), BoostPoint(220, 100));	bg::append(srcPolygon.outer(), BoostPoint(230, 100));	bg::append(srcPolygon.outer(), BoostPoint(230, 90));
	bg::append(srcPolygon.outer(), BoostPoint(240, 90));	bg::append(srcPolygon.outer(), BoostPoint(240, 100));	bg::append(srcPolygon.outer(), BoostPoint(250, 100));
	bg::append(srcPolygon.outer(), BoostPoint(330, 70));	bg::append(srcPolygon.outer(), BoostPoint(340, 60));	bg::append(srcPolygon.outer(), BoostPoint(330, 50));
	bg::append(srcPolygon.outer(), BoostPoint(340, 40));	bg::append(srcPolygon.outer(), BoostPoint(350, 50));	bg::append(srcPolygon.outer(), BoostPoint(390, 10));
	bg::append(srcPolygon.outer(), BoostPoint(400, 20));	bg::append(srcPolygon.outer(), BoostPoint(360, 60));	bg::append(srcPolygon.outer(), BoostPoint(370, 70));
	bg::append(srcPolygon.outer(), BoostPoint(360, 80));	bg::append(srcPolygon.outer(), BoostPoint(350, 70));	bg::append(srcPolygon.outer(), BoostPoint(340, 80));
	bg::append(srcPolygon.outer(), BoostPoint(350, 109));	bg::append(srcPolygon.outer(), BoostPoint(360, 109));	bg::append(srcPolygon.outer(), BoostPoint(361, 109));
	bg::append(srcPolygon.outer(), BoostPoint(361, 110));	bg::append(srcPolygon.outer(), BoostPoint(360, 110));	bg::append(srcPolygon.outer(), BoostPoint(340, 110));
	bg::append(srcPolygon.outer(), BoostPoint(250, 110));	bg::append(srcPolygon.outer(), BoostPoint(240, 110));	bg::append(srcPolygon.outer(), BoostPoint(240, 120));
	bg::append(srcPolygon.outer(), BoostPoint(230, 120));	bg::append(srcPolygon.outer(), BoostPoint(230, 110));	bg::append(srcPolygon.outer(), BoostPoint(220, 110));
	bg::append(srcPolygon.outer(), BoostPoint(210, 110));	bg::append(srcPolygon.outer(), BoostPoint(200, 110));	bg::append(srcPolygon.outer(), BoostPoint(110, 110));
	bg::append(srcPolygon.outer(), BoostPoint(110, 200));	bg::append(srcPolygon.outer(), BoostPoint(200, 200));	bg::append(srcPolygon.outer(), BoostPoint(230, 200));
	bg::append(srcPolygon.outer(), BoostPoint(240, 210));	bg::append(srcPolygon.outer(), BoostPoint(250, 200));	bg::append(srcPolygon.outer(), BoostPoint(300, 210));
	bg::append(srcPolygon.outer(), BoostPoint(350, 200));	bg::append(srcPolygon.outer(), BoostPoint(250, 300));	bg::append(srcPolygon.outer(), BoostPoint(250, 210));
	bg::append(srcPolygon.outer(), BoostPoint(240, 220));	bg::append(srcPolygon.outer(), BoostPoint(230, 210));	bg::append(srcPolygon.outer(), BoostPoint(200, 210));
	bg::append(srcPolygon.outer(), BoostPoint(110, 210));	bg::append(srcPolygon.outer(), BoostPoint(110, 300));	bg::append(srcPolygon.outer(), BoostPoint(200, 300));
	bg::append(srcPolygon.outer(), BoostPoint(220, 310));	bg::append(srcPolygon.outer(), BoostPoint(230, 310));	bg::append(srcPolygon.outer(), BoostPoint(230, 320));
	bg::append(srcPolygon.outer(), BoostPoint(200, 310));	bg::append(srcPolygon.outer(), BoostPoint(110, 310));	bg::append(srcPolygon.outer(), BoostPoint(100, 310));
	bg::append(srcPolygon.outer(), BoostPoint(10, 310));	bg::append(srcPolygon.outer(), BoostPoint(10, 300));	bg::append(srcPolygon.outer(), BoostPoint(100, 300));
	bg::append(srcPolygon.outer(), BoostPoint(100, 210));	bg::append(srcPolygon.outer(), BoostPoint(10, 210));	bg::append(srcPolygon.outer(), BoostPoint(10, 200));
	bg::append(srcPolygon.outer(), BoostPoint(100, 200));	bg::append(srcPolygon.outer(), BoostPoint(100, 110));	bg::append(srcPolygon.outer(), BoostPoint(10, 110));
	bg::append(srcPolygon.outer(), BoostPoint(10, 100));	bg::append(srcPolygon.outer(), BoostPoint(10, 100));

	dummySrcData.emplace_back(srcPolygon);

	BoostPolygon dstPolygon;
	CRoadData dstRoadData;
	//CreatedRoadModelInfo dstInfo;
	dummyDstData = std::vector<CreatedRoadModelInfo>();

	bg::append(dstPolygon.outer(), BoostPoint(10, 100));	bg::append(dstPolygon.outer(), BoostPoint(100, 100));	bg::append(dstPolygon.outer(), BoostPoint(100, 110));
	bg::append(dstPolygon.outer(), BoostPoint(10, 110));	bg::append(dstPolygon.outer(), BoostPoint(10, 100));
	dstRoadData.Polygon(dstPolygon);
	dstRoadData.Type(RoadSectionType::ROAD_SECTION_UNKNOWN);
	CreatedRoadModelInfo dstInfo = CreatedRoadModelInfo(dstRoadData);
	dummyDstData.emplace_back(dstInfo);
	dstPolygon.clear();

	dstRoadData = CRoadData();
	bg::append(dstPolygon.outer(), BoostPoint(100, 90));	bg::append(dstPolygon.outer(), BoostPoint(110, 90));	bg::append(dstPolygon.outer(), BoostPoint(110, 100));
	bg::append(dstPolygon.outer(), BoostPoint(100, 100));	bg::append(dstPolygon.outer(), BoostPoint(100, 90));
	dstRoadData.Polygon(dstPolygon);
	dstRoadData.Type(RoadSectionType::ROAD_SECTION_UNKNOWN);
	dstInfo = CreatedRoadModelInfo(dstRoadData);
	dummyDstData.emplace_back(dstRoadData);
	dstPolygon.clear();

	dstRoadData = CRoadData();
	bg::append(dstPolygon.outer(), BoostPoint(100, 100));	bg::append(dstPolygon.outer(), BoostPoint(110, 100));	bg::append(dstPolygon.outer(), BoostPoint(110, 110));
	bg::append(dstPolygon.outer(), BoostPoint(100, 110));	bg::append(dstPolygon.outer(), BoostPoint(100, 100));
	dstRoadData.Polygon(dstPolygon);
	dstRoadData.Type(RoadSectionType::ROAD_SECTION_CROSSING);
	dstRoadData.Division(4);
	dstInfo = CreatedRoadModelInfo(dstRoadData);
	dummyDstData.emplace_back(dstRoadData);
	dstPolygon.clear();

	dstRoadData = CRoadData();
	bg::append(dstPolygon.outer(), BoostPoint(110, 100));	bg::append(dstPolygon.outer(), BoostPoint(200, 100));	bg::append(dstPolygon.outer(), BoostPoint(200, 110));
	bg::append(dstPolygon.outer(), BoostPoint(110, 110));	bg::append(dstPolygon.outer(), BoostPoint(110, 100));
	dstRoadData.Polygon(dstPolygon);
	dstRoadData.Type(RoadSectionType::ROAD_SECTION_UNKNOWN);
	dstInfo = CreatedRoadModelInfo(dstRoadData);
	dummyDstData.emplace_back(dstRoadData);
	dstPolygon.clear();

	dstRoadData = CRoadData();
	bg::append(dstPolygon.outer(), BoostPoint(200, 100));	bg::append(dstPolygon.outer(), BoostPoint(210, 90));	bg::append(dstPolygon.outer(), BoostPoint(220, 90));
	bg::append(dstPolygon.outer(), BoostPoint(220, 110));	bg::append(dstPolygon.outer(), BoostPoint(200, 110));	bg::append(dstPolygon.outer(), BoostPoint(200, 100));
	dstRoadData.Polygon(dstPolygon);
	dstRoadData.Type(RoadSectionType::ROAD_SECTION_CROSSING);
	dstRoadData.Division(2);
	dstInfo = CreatedRoadModelInfo(dstRoadData);
	dummyDstData.emplace_back(dstRoadData);
	dstPolygon.clear();

	dstRoadData = CRoadData();
	bg::append(dstPolygon.outer(), BoostPoint(220, 100));	bg::append(dstPolygon.outer(), BoostPoint(230, 100));	bg::append(dstPolygon.outer(), BoostPoint(230, 110));
	bg::append(dstPolygon.outer(), BoostPoint(220, 110));	bg::append(dstPolygon.outer(), BoostPoint(220, 100));
	dstRoadData.Polygon(dstPolygon);
	dstRoadData.Type(RoadSectionType::ROAD_SECTION_UNKNOWN);
	dstInfo = CreatedRoadModelInfo(dstRoadData);
	dummyDstData.emplace_back(dstRoadData);
	dstPolygon.clear();

	dstRoadData = CRoadData();
	bg::append(dstPolygon.outer(), BoostPoint(230, 100));	bg::append(dstPolygon.outer(), BoostPoint(240, 100));	bg::append(dstPolygon.outer(), BoostPoint(240, 110));
	bg::append(dstPolygon.outer(), BoostPoint(230, 110));	bg::append(dstPolygon.outer(), BoostPoint(230, 100));
	dstRoadData.Polygon(dstPolygon);
	dstRoadData.Type(RoadSectionType::ROAD_SECTION_CROSSING);
	dstRoadData.Division(4);
	dstInfo = CreatedRoadModelInfo(dstRoadData);
	dummyDstData.emplace_back(dstRoadData);
	dstPolygon.clear();

	dstRoadData = CRoadData();
	bg::append(dstPolygon.outer(), BoostPoint(230, 100));	bg::append(dstPolygon.outer(), BoostPoint(240, 100));	bg::append(dstPolygon.outer(), BoostPoint(240, 110));
	bg::append(dstPolygon.outer(), BoostPoint(230, 110));	bg::append(dstPolygon.outer(), BoostPoint(230, 100));
	dstRoadData.Polygon(dstPolygon);
	dstRoadData.Type(RoadSectionType::ROAD_SECTION_CROSSING);
	dstRoadData.Division(4);
	dstInfo = CreatedRoadModelInfo(dstRoadData);
	dummyDstData.emplace_back(dstRoadData);
	dstPolygon.clear();

	dstRoadData = CRoadData();
	bg::append(dstPolygon.outer(), BoostPoint(230, 90));	bg::append(dstPolygon.outer(), BoostPoint(240, 90));	bg::append(dstPolygon.outer(), BoostPoint(240, 100));
	bg::append(dstPolygon.outer(), BoostPoint(230, 100));	bg::append(dstPolygon.outer(), BoostPoint(230, 90));
	dstRoadData.Polygon(dstPolygon);
	dstRoadData.Type(RoadSectionType::ROAD_SECTION_UNKNOWN);
	dstInfo = CreatedRoadModelInfo(dstRoadData);
	dummyDstData.emplace_back(dstRoadData);
	dstPolygon.clear();

	dstRoadData = CRoadData();
	bg::append(dstPolygon.outer(), BoostPoint(230, 110));	bg::append(dstPolygon.outer(), BoostPoint(240, 110));	bg::append(dstPolygon.outer(), BoostPoint(240, 120));
	bg::append(dstPolygon.outer(), BoostPoint(230, 120));	bg::append(dstPolygon.outer(), BoostPoint(230, 110));
	dstRoadData.Polygon(dstPolygon);
	dstRoadData.Type(RoadSectionType::ROAD_SECTION_UNKNOWN);
	dstInfo = CreatedRoadModelInfo(dstRoadData);
	dummyDstData.emplace_back(dstRoadData);
	dstPolygon.clear();

	dstRoadData = CRoadData();
	bg::append(dstPolygon.outer(), BoostPoint(240, 100));	bg::append(dstPolygon.outer(), BoostPoint(250, 100));	bg::append(dstPolygon.outer(), BoostPoint(250, 110));
	bg::append(dstPolygon.outer(), BoostPoint(240, 110));	bg::append(dstPolygon.outer(), BoostPoint(240, 100));
	dstRoadData.Polygon(dstPolygon);
	dstRoadData.Type(RoadSectionType::ROAD_SECTION_UNKNOWN);
	dstInfo = CreatedRoadModelInfo(dstRoadData);
	dummyDstData.emplace_back(dstRoadData);
	dstPolygon.clear();

	dstRoadData = CRoadData();
	bg::append(dstPolygon.outer(), BoostPoint(250, 100));	bg::append(dstPolygon.outer(), BoostPoint(330, 70));	bg::append(dstPolygon.outer(), BoostPoint(340, 110));
	bg::append(dstPolygon.outer(), BoostPoint(250, 110));	bg::append(dstPolygon.outer(), BoostPoint(250, 100));
	dstRoadData.Polygon(dstPolygon);
	dstRoadData.Type(RoadSectionType::ROAD_SECTION_CROSSING);
	dstRoadData.Division(3);
	dstInfo = CreatedRoadModelInfo(dstRoadData);
	dummyDstData.emplace_back(dstRoadData);
	dstPolygon.clear();

	dstRoadData = CRoadData();
	bg::append(dstPolygon.outer(), BoostPoint(330, 70));	bg::append(dstPolygon.outer(), BoostPoint(340, 60));	bg::append(dstPolygon.outer(), BoostPoint(350, 70));
	bg::append(dstPolygon.outer(), BoostPoint(340, 80));	bg::append(dstPolygon.outer(), BoostPoint(350, 109));	bg::append(dstPolygon.outer(), BoostPoint(360, 109));
	bg::append(dstPolygon.outer(), BoostPoint(360, 110));	bg::append(dstPolygon.outer(), BoostPoint(340, 110));	bg::append(dstPolygon.outer(), BoostPoint(330, 70));
	dstRoadData.Polygon(dstPolygon);
	dstRoadData.Type(RoadSectionType::ROAD_SECTION_UNKNOWN);
	dstInfo = CreatedRoadModelInfo(dstRoadData);
	dummyDstData.emplace_back(dstRoadData);
	dstPolygon.clear();

	dstRoadData = CRoadData();
	bg::append(dstPolygon.outer(), BoostPoint(360, 109));	bg::append(dstPolygon.outer(), BoostPoint(361, 109));	bg::append(dstPolygon.outer(), BoostPoint(361, 110));
	bg::append(dstPolygon.outer(), BoostPoint(360, 110));	bg::append(dstPolygon.outer(), BoostPoint(360, 109));
	dstRoadData.Polygon(dstPolygon);
	dstRoadData.Type(RoadSectionType::ROAD_SECTION_UNKNOWN);
	dstInfo = CreatedRoadModelInfo(dstRoadData);
	dummyDstData.emplace_back(dstRoadData);
	dstPolygon.clear();

	dstRoadData = CRoadData();
	bg::append(dstPolygon.outer(), BoostPoint(330, 50));	bg::append(dstPolygon.outer(), BoostPoint(340, 40));	bg::append(dstPolygon.outer(), BoostPoint(350, 50));
	bg::append(dstPolygon.outer(), BoostPoint(340, 60));	bg::append(dstPolygon.outer(), BoostPoint(330, 50));
	dstRoadData.Polygon(dstPolygon);
	dstRoadData.Type(RoadSectionType::ROAD_SECTION_UNKNOWN);
	dstInfo = CreatedRoadModelInfo(dstRoadData);
	dummyDstData.emplace_back(dstRoadData);
	dstPolygon.clear();

	dstRoadData = CRoadData();
	bg::append(dstPolygon.outer(), BoostPoint(340, 60));	bg::append(dstPolygon.outer(), BoostPoint(350, 50));	bg::append(dstPolygon.outer(), BoostPoint(360, 60));
	bg::append(dstPolygon.outer(), BoostPoint(350, 70));	bg::append(dstPolygon.outer(), BoostPoint(340, 60));
	dstRoadData.Polygon(dstPolygon);
	dstRoadData.Type(RoadSectionType::ROAD_SECTION_CROSSING);
	dstRoadData.Division(4);
	dstInfo = CreatedRoadModelInfo(dstRoadData);
	dummyDstData.emplace_back(dstRoadData);
	dstPolygon.clear();

	dstRoadData = CRoadData();
	bg::append(dstPolygon.outer(), BoostPoint(350, 70));	bg::append(dstPolygon.outer(), BoostPoint(360, 60));	bg::append(dstPolygon.outer(), BoostPoint(370, 70));
	bg::append(dstPolygon.outer(), BoostPoint(360, 80));	bg::append(dstPolygon.outer(), BoostPoint(350, 70));
	dstRoadData.Polygon(dstPolygon);
	dstRoadData.Type(RoadSectionType::ROAD_SECTION_UNKNOWN);
	dstInfo = CreatedRoadModelInfo(dstRoadData);
	dummyDstData.emplace_back(dstRoadData);
	dstPolygon.clear();

	dstRoadData = CRoadData();
	bg::append(dstPolygon.outer(), BoostPoint(350, 50));	bg::append(dstPolygon.outer(), BoostPoint(390, 10));	bg::append(dstPolygon.outer(), BoostPoint(400, 20));
	bg::append(dstPolygon.outer(), BoostPoint(360, 60));	bg::append(dstPolygon.outer(), BoostPoint(350, 50));
	dstRoadData.Polygon(dstPolygon);
	dstRoadData.Type(RoadSectionType::ROAD_SECTION_UNKNOWN);
	dstInfo = CreatedRoadModelInfo(dstRoadData);
	dummyDstData.emplace_back(dstRoadData);
	dstPolygon.clear();

	dstRoadData = CRoadData();
	bg::append(dstPolygon.outer(), BoostPoint(100, 110));	bg::append(dstPolygon.outer(), BoostPoint(110, 110));	bg::append(dstPolygon.outer(), BoostPoint(110, 200));
	bg::append(dstPolygon.outer(), BoostPoint(100, 200));	bg::append(dstPolygon.outer(), BoostPoint(100, 110));
	dstRoadData.Polygon(dstPolygon);
	dstRoadData.Type(RoadSectionType::ROAD_SECTION_UNKNOWN);
	dummyDstData.emplace_back(dstRoadData);
	dstPolygon.clear();

	dstRoadData = CRoadData();
	bg::append(dstPolygon.outer(), BoostPoint(10, 200));	bg::append(dstPolygon.outer(), BoostPoint(100, 200));	bg::append(dstPolygon.outer(), BoostPoint(100, 210));
	bg::append(dstPolygon.outer(), BoostPoint(10, 210));	bg::append(dstPolygon.outer(), BoostPoint(10, 200));
	dstRoadData.Polygon(dstPolygon);
	dstRoadData.Type(RoadSectionType::ROAD_SECTION_UNKNOWN);
	dstInfo = CreatedRoadModelInfo(dstRoadData);
	dummyDstData.emplace_back(dstRoadData);
	dstPolygon.clear();

	dstRoadData = CRoadData();
	bg::append(dstPolygon.outer(), BoostPoint(100, 200));	bg::append(dstPolygon.outer(), BoostPoint(110, 200));	bg::append(dstPolygon.outer(), BoostPoint(110, 210));
	bg::append(dstPolygon.outer(), BoostPoint(100, 210));	bg::append(dstPolygon.outer(), BoostPoint(100, 200));
	dstRoadData.Polygon(dstPolygon);
	dstRoadData.Type(RoadSectionType::ROAD_SECTION_CROSSING);
	dstRoadData.Division(4);
	dstInfo = CreatedRoadModelInfo(dstRoadData);
	dummyDstData.emplace_back(dstRoadData);
	dstPolygon.clear();

	dstRoadData = CRoadData();
	bg::append(dstPolygon.outer(), BoostPoint(110, 200));	bg::append(dstPolygon.outer(), BoostPoint(200, 200));	bg::append(dstPolygon.outer(), BoostPoint(200, 210));
	bg::append(dstPolygon.outer(), BoostPoint(110, 210));	bg::append(dstPolygon.outer(), BoostPoint(110, 200));
	dstRoadData.Polygon(dstPolygon);
	dstRoadData.Type(RoadSectionType::ROAD_SECTION_UNKNOWN);
	dstInfo = CreatedRoadModelInfo(dstRoadData);
	dummyDstData.emplace_back(dstRoadData);
	dstPolygon.clear();

	dstRoadData = CRoadData();
	bg::append(dstPolygon.outer(), BoostPoint(200, 200));	bg::append(dstPolygon.outer(), BoostPoint(230, 210));	bg::append(dstPolygon.outer(), BoostPoint(230, 200));
	bg::append(dstPolygon.outer(), BoostPoint(200, 210));	bg::append(dstPolygon.outer(), BoostPoint(200, 200));
	dstRoadData.Polygon(dstPolygon);
	dstRoadData.Type(RoadSectionType::ROAD_SECTION_UNKNOWN);
	dstInfo = CreatedRoadModelInfo(dstRoadData);
	dummyDstData.emplace_back(dstRoadData);
	dstPolygon.clear();

	dstRoadData = CRoadData();
	bg::append(dstPolygon.outer(), BoostPoint(230, 200));	bg::append(dstPolygon.outer(), BoostPoint(240, 210));	bg::append(dstPolygon.outer(), BoostPoint(250, 200));
	bg::append(dstPolygon.outer(), BoostPoint(250, 210));	bg::append(dstPolygon.outer(), BoostPoint(230, 210));	bg::append(dstPolygon.outer(), BoostPoint(230, 200));
	dstRoadData.Polygon(dstPolygon);
	dstRoadData.Type(RoadSectionType::ROAD_SECTION_UNKNOWN);
	dstInfo = CreatedRoadModelInfo(dstRoadData);
	dummyDstData.emplace_back(dstRoadData);
	dstPolygon.clear();

	dstRoadData = CRoadData();
	bg::append(dstPolygon.outer(), BoostPoint(250, 200));	bg::append(dstPolygon.outer(), BoostPoint(300, 210));	bg::append(dstPolygon.outer(), BoostPoint(350, 200));
	bg::append(dstPolygon.outer(), BoostPoint(250, 300));	bg::append(dstPolygon.outer(), BoostPoint(250, 200));
	dstRoadData.Polygon(dstPolygon);
	dstRoadData.Type(RoadSectionType::ROAD_SECTION_UNKNOWN);
	dstInfo = CreatedRoadModelInfo(dstRoadData);
	dummyDstData.emplace_back(dstRoadData);
	dstPolygon.clear();

	dstRoadData = CRoadData();
	bg::append(dstPolygon.outer(), BoostPoint(10, 300));	bg::append(dstPolygon.outer(), BoostPoint(100, 300));	bg::append(dstPolygon.outer(), BoostPoint(100, 310));
	bg::append(dstPolygon.outer(), BoostPoint(10, 310));	bg::append(dstPolygon.outer(), BoostPoint(10, 300));
	dstRoadData.Polygon(dstPolygon);
	dstRoadData.Type(RoadSectionType::ROAD_SECTION_UNKNOWN);
	dstInfo = CreatedRoadModelInfo(dstRoadData);
	dummyDstData.emplace_back(dstRoadData);
	dstPolygon.clear();

	dstRoadData = CRoadData();
	bg::append(dstPolygon.outer(), BoostPoint(100, 300));	bg::append(dstPolygon.outer(), BoostPoint(110, 300));	bg::append(dstPolygon.outer(), BoostPoint(110, 310));
	bg::append(dstPolygon.outer(), BoostPoint(100, 310));	bg::append(dstPolygon.outer(), BoostPoint(100, 300));
	dstRoadData.Polygon(dstPolygon);
	dstRoadData.Type(RoadSectionType::ROAD_SECTION_CROSSING);
	dstRoadData.Division(3);
	dstInfo = CreatedRoadModelInfo(dstRoadData);
	dummyDstData.emplace_back(dstRoadData);
	dstPolygon.clear();

	dstRoadData = CRoadData();
	bg::append(dstPolygon.outer(), BoostPoint(110, 300));	bg::append(dstPolygon.outer(), BoostPoint(200, 300));	bg::append(dstPolygon.outer(), BoostPoint(200, 310));
	bg::append(dstPolygon.outer(), BoostPoint(110, 310));	bg::append(dstPolygon.outer(), BoostPoint(110, 300));
	dstRoadData.Polygon(dstPolygon);
	dstRoadData.Type(RoadSectionType::ROAD_SECTION_UNKNOWN);
	dstInfo = CreatedRoadModelInfo(dstRoadData);
	dummyDstData.emplace_back(dstRoadData);
	dstPolygon.clear();

	dstRoadData = CRoadData();
	bg::append(dstPolygon.outer(), BoostPoint(200, 300));	bg::append(dstPolygon.outer(), BoostPoint(220, 310));	bg::append(dstPolygon.outer(), BoostPoint(230, 310));
	bg::append(dstPolygon.outer(), BoostPoint(200, 310));	bg::append(dstPolygon.outer(), BoostPoint(200, 300));
	dstRoadData.Polygon(dstPolygon);
	dstRoadData.Type(RoadSectionType::ROAD_SECTION_UNKNOWN);
	dstInfo = CreatedRoadModelInfo(dstRoadData);
	dummyDstData.emplace_back(dstRoadData);
	dstPolygon.clear();

	BoostMultiPoints intersectionList;
	intersectionList.emplace_back(BoostPoint(105, 105));
	intersectionList.emplace_back(BoostPoint(210, 100));
	intersectionList.emplace_back(BoostPoint(235, 105));
	intersectionList.emplace_back(BoostPoint(295, 90));
	intersectionList.emplace_back(BoostPoint(400, 10));
	intersectionList.emplace_back(BoostPoint(105, 205));
	intersectionList.emplace_back(BoostPoint(105, 305));

	RoadModelData roadModelData = RoadModelData(dummyDstData, dummySrcData, intersectionList);

	// 分割ポリゴン用属性データの作成
	std::vector<CShapeAttribute::AttributeFieldData> vecFields;
	std::vector<CShapeAttribute::AttributeDataRecord> vecAttrRecords;

    CShapeAttribute::AttributeFieldData field;

	field.fieldType = CShapeAttribute::AttributeFieldType::ATTR_FIELD_TYPE_STRING;
	field.strName = "roadType";
	field.nWidth = 4;
	vecFields.push_back(field);

	field.fieldType = CShapeAttribute::AttributeFieldType::ATTR_FIELD_TYPE_STRING;
	field.strName = "dividedRoad";
	field.nWidth = 4;
	vecFields.push_back(field);

	for(int i = 0; i < dummyDstData.size(); i++)
	{
        CShapeAttribute::AttributeDataRecord record;
		auto roadData = dummyDstData[i].GetRoadData();

		// nShapeId
		record.nShapeId = i;

		// 道路タイプ
        CShapeAttribute::AttributeData roadType;
		roadType.dataType = CShapeAttribute::AttributeDataType::ATTR_DATA_TYPE_INT;
		roadType.nValue = (int)roadData.Type();
		record.vecAttribute.emplace_back(roadType);

		// 道路分割数
        CShapeAttribute::AttributeData dividedroad;
		dividedroad.dataType = CShapeAttribute::AttributeDataType::ATTR_DATA_TYPE_INT;
		dividedroad.nValue = (int)roadData.Division();
		record.vecAttribute.emplace_back(dividedroad);

		vecAttrRecords.emplace_back(record);
	}

	std::string strBeforePolygonDivisionPath = CFileUtil::Combine(strDebugFolerPath, "BeforePolygonDivision.shp");
	std::string strAfterPolygonDivisionPath = CFileUtil::Combine(strDebugFolerPath, "AfterPolygonDivision.shp");
	std::string strIntersectionPointListPath = CFileUtil::Combine(strDebugFolerPath, "IntersectionPointList.shp");
	debugUtil.OutputPolygonsToShp(dummySrcData, strBeforePolygonDivisionPath);
	//debugUtil.OutputPolygonsToShp(roadModelData.GetRoadBoostMultiPolygon(), strAfterPolygonDivisionPath);
	writer.OutputPolygons(roadModelData.GetRoadBoostMultiPolygon(), strAfterPolygonDivisionPath, vecFields, vecAttrRecords);
	debugUtil.OutputMultiPointsToShp(intersectionList, strIntersectionPointListPath);

	std::string strOutputErrPath = CFileUtil::Combine(strDebugFolerPath, "ErrCheckLog.csv");
	Run(roadModelData, strOutputErrPath, minArea, maxDistance);
}

void RoadModelErrorChecker::RunFromSHP(std::string inputRoadBeforeDivisionShpFilePath, std::string inputDividedShpFilePath, std::string inputIntersectionShpFilePath, std::string outputErrFilePath, double minArea, double maxDistance)
{
	std::cout << "RoadModelErrorChecker::RunFromSHP()" << std::endl;

	// SHPファイル読み込み
    CShapeReader reader;

	// 分割前道路モデルの読み込み
	BoostMultiPolygon roadModel;
    reader.ReadPolygons(inputRoadBeforeDivisionShpFilePath, roadModel);

	// 分割後の道路モデル(属性情報付き)の読み込み
	std::vector<CreatedRoadModelInfo> dividedRoadModelInfo;
	BoostMultiPolygon dividedRoadPolygons;
	std::vector<CShapeAttribute::AttributeFieldData> vecFields;
	std::vector<CShapeAttribute::AttributeDataRecord> vecAttrRecords;
    reader.ReadPolygons(inputDividedShpFilePath, dividedRoadPolygons, vecFields, vecAttrRecords);
	for (int i = 0; i < dividedRoadPolygons.size(); i++)
	{

		CRoadData dstRoadData = CRoadData();
		dstRoadData.Polygon(dividedRoadPolygons[i]);

		switch (vecAttrRecords[i].vecAttribute[0].dataType)
		{
		case CShapeAttribute::AttributeDataType::ATTR_DATA_TYPE_INT:
			dstRoadData.Type((RoadSectionType)vecAttrRecords[i].vecAttribute[0].nValue);
			break;
		case CShapeAttribute::AttributeDataType::ATTR_DATA_TYPE_DOUBLE:
		case CShapeAttribute::AttributeDataType::ATTR_DATA_TYPE_STRING:
		case CShapeAttribute::AttributeDataType::ATTR_DATA_TYPE_NULL:
		default:
			break;
		}

		switch (vecAttrRecords[i].vecAttribute[1].dataType)
		{
		case CShapeAttribute::AttributeDataType::ATTR_DATA_TYPE_INT:
			dstRoadData.Division(vecAttrRecords[i].vecAttribute[1].nValue);
			break;
		case CShapeAttribute::AttributeDataType::ATTR_DATA_TYPE_DOUBLE:
		case CShapeAttribute::AttributeDataType::ATTR_DATA_TYPE_STRING:
		case CShapeAttribute::AttributeDataType::ATTR_DATA_TYPE_NULL:
		default:
			break;
		}

		CreatedRoadModelInfo info = CreatedRoadModelInfo(dstRoadData);
		dividedRoadModelInfo.emplace_back(info);
	}

	// 交差点情報の読み込み
	BoostMultiPoints intersectionList;
    reader.ReadPoints(inputIntersectionShpFilePath, intersectionList);

#ifdef DEBUG
	std::string strTime = CTime::GetCurrentTime().Format("%Y%m%d_%H%M%S");
	std::string strCurrentPath = CAnalyzeRoadEdgeDebugUtil::GetCurrentPath();
	strDebugFolerPath = CFileUtil::Combine(strCurrentPath, strTime);
	CAnalyzeRoadEdgeDebugUtil::CreateFolder(strDebugFolerPath);

	std::string strBeforePolygonDivisionPath = CFileUtil::Combine(strDebugFolerPath, "BeforePolygonDivision.shp");
	std::string strAfterPolygonDivisionPath = CFileUtil::Combine(strDebugFolerPath, "AfterPolygonDivision.shp");
	std::string strIntersectionPointListPath = CFileUtil::Combine(strDebugFolerPath, "IntersectionPointList.shp");
	debugUtil.OutputPolygonsToShp(roadModel, strBeforePolygonDivisionPath);
	debugUtil.OutputPolygonsToShp(dividedRoadPolygons, strAfterPolygonDivisionPath, vecFields, vecAttrRecords);
	debugUtil.OutputMultiPointsToShp(intersectionList, strIntersectionPointListPath);
#endif // DEBUG

	// エラーチェックに必要なデータクラス作成
	RoadModelData roadModelData = RoadModelData(dividedRoadModelInfo, roadModel, intersectionList);

	// エラーチェック実行
	Run(roadModelData, outputErrFilePath, minArea, maxDistance);
}

void RoadModelErrorChecker::Run(RoadModelData& data, std::string outputErrFilePath, bool isNewFile, double minArea, double maxDistance)
{
	CheckMissingModelErr(data);
	CheckTopologicalErr(data);
	CheckAngleErr(data);
	CheckIntersectionErr(data);
	CheckExcessErr(data);
	CheckSuperimposeErr(data);
	CheckRoadDivisionErr(data);
	CheckMinusculePolygonErr(data, minArea);
	CheckIntersectionDistanceErr(data, maxDistance);
	SaveErr(data, outputErrFilePath, isNewFile);
}

std::vector<std::vector<std::string>> RoadModelErrorChecker::Run(RoadModelData &data, double minArea, double maxDistance)
{
    CheckMissingModelErr(data);
    CheckTopologicalErr(data);
    CheckAngleErr(data);
    CheckIntersectionErr(data);
    CheckExcessErr(data);
    CheckSuperimposeErr(data);
    CheckRoadDivisionErr(data);
    CheckMinusculePolygonErr(data, minArea);
    CheckIntersectionDistanceErr(data, maxDistance);
    std::vector<std::vector<std::string>> errStrings = CreateErrMsg(data);
    return errStrings;
}

void RoadModelErrorChecker::CheckMissingModelErr(RoadModelData& data)
{
#ifdef DEBUG
	std::cout << "************************************* CheckMissingModelErr *************************************" << std::endl;
#endif // DEBUG

	BoostMultiPolygon out = data.GetPreRoadPolygonList();

	for (BoostPolygon devidedPolygon : data.GetRoadBoostMultiPolygon())
	{
		// 頂点数が足りずポリゴンの形が生成できない場合はスキップ
		if (devidedPolygon.outer().size() < 3)
		{
			continue;
		}

		BoostMultiPolygon newOut;
		bg::difference(out, devidedPolygon, newOut);
		out = newOut;
	}

	for (auto item : out)
	{
		data.AddErrWithEmptyPolygon(RoadErrInfo(RoadModelErr::MISSING_MODEL_ERR, GetPolygonPoint(item)));
	}
}

void RoadModelErrorChecker::CheckTopologicalErr(RoadModelData& data)
{
#ifdef DEBUG
	std::cout << "************************************* CheckTopologicalErr *************************************" << std::endl;
#endif // DEBUG

	std::vector<std::tuple<int, CreatedRoadModelInfo>> updateCreatedRoadModelInfoList;

	for (CreatedRoadModelInfo& createdRoadModelInfo : data.GetRoadPolygonList())
	{
		if (IsInspectionInfo(createdRoadModelInfo) == false)
		{
			continue;
		}

		auto pol = createdRoadModelInfo.GetRoadData().Polygon();
		size_t vertexSize = pol.outer().size();

		///
		/// ポリゴンの単純確認
		///
		if (bg::is_valid(pol) == false // ポリゴンが不正でないか
			|| bg::is_simple(pol) == false // ポリゴンが単純かどうか
			|| bg::area(pol) <= 0 // 面積が0以下かどうか
			)
		{
			createdRoadModelInfo.AddErr(RoadErrInfo(RoadModelErr::TOPOLOGICAL_INVAILD_ERR, GetPolygonPoint(pol)));

			// 次のポリゴンへ
			continue;
		}

		///
		/// 内角確認
		/// (180度以上の角がない場合はトポロジー不正がないため、次のポリゴンの検証に移る)
		///
		std::vector<double> angles = Angles(pol);
		bool isOverAngel = false;

		for (double angle : angles)
		{
			if (CEpsUtil::GreaterEqual(angle, boost::math::constants::pi<double>()))
			{
				isOverAngel = true;
				break;
			}

		}

		if (isOverAngel == false)
		{
			// 次のポリゴンへ
			continue;
		}

		///
		/// ポリゴンの閉区間確認
		///
		if (pol.outer()[0].x() == pol.outer()[vertexSize - 1].x()
			&& pol.outer()[0].y() == pol.outer()[vertexSize - 1].y())
		{
			//std::cout << "first point and last point is equal. continue process..." << std::endl;
		}
		else
		{
			std::cout << "first point and last point is NOT equal. save err info." << std::endl;
			createdRoadModelInfo.AddErr(RoadErrInfo(RoadModelErr::TOPOLOGICAL_SHORTAGE_POINT_ERR, GetPolygonPoint(pol)));

			// 次のポリゴンへ
			continue;
		}

		///
		/// ポリゴンの頂点重複確認
		///
		bool isDuplicatePoint = false;
		for (int i = 0; i < vertexSize - 3; i++)
		{
			if (isDuplicatePoint == true)
			{
				// 頂点確認終わり
				break;
			}

			for (int j = i + 1; j < vertexSize - 2; j++)
			{
				if (bg::equals(pol.outer()[i], pol.outer()[j]))
				{
					isDuplicatePoint = true;
					break;
				}
			}
		}

		if (isDuplicatePoint == true)
		{
			createdRoadModelInfo.AddErr(RoadErrInfo(RoadModelErr::TOPOLOGICAL_DUPLICATION_POINT_ERR, GetPolygonPoint(pol)));

			// 次のポリゴンへ
			continue;
		}

		///
		/// ポリゴンの各辺同士の交点を求める
		///
		std::vector<bg::model::segment<BoostPoint>> sides;
		bool isIntersection = false;
		for (int i = 0; i < vertexSize - 3; i++)
		{
			for (int j = i + 1; j < vertexSize - 2; j++)
			{
				sides.emplace_back(bg::model::segment<BoostPoint>(pol.outer()[i], pol.outer()[j]));
			}
		}

		for (int i = 0; i < sides.size() - 2; i++)
		{
			if (isIntersection == true)
			{
				break;
			}

			for (int j = i + 1; j < sides.size() - 1; j++)
			{
				BoostMultiPoints intersections;

				if (bg::intersection(sides[i], sides[j], intersections))
				{
					// 辺同士の交点がない場合は次の辺へ
					continue;
				}
				else if (
					(bg::equals(sides[i].first, sides[j].first) && !bg::equals(sides[i].first, sides[j].second) && !bg::equals(sides[i].second, sides[j].first) && !bg::equals(sides[i].second, sides[j].second))
					|| (!bg::equals(sides[i].first, sides[j].first) && bg::equals(sides[i].first, sides[j].second) && !bg::equals(sides[i].second, sides[j].first) && !bg::equals(sides[i].second, sides[j].second))
					|| (!bg::equals(sides[i].first, sides[j].first) && !bg::equals(sides[i].first, sides[j].second) && bg::equals(sides[i].second, sides[j].first) && !bg::equals(sides[i].second, sides[j].second))
					|| (!bg::equals(sides[i].first, sides[j].first) && !bg::equals(sides[i].first, sides[j].second) && !bg::equals(sides[i].second, sides[j].first) && bg::equals(sides[i].second, sides[j].second))
					)
				{
					// 辺同士が端同士の1点で交わる場合は正常なので次の辺へ
					continue;
				}
				else
				{
					// それ以外の場合は自己交差または自己接線のため
					// エラー出力する
					isIntersection = true;
					break;
				}

			}
		}

		if (isIntersection == true)
		{
			createdRoadModelInfo.AddErr(RoadErrInfo(RoadModelErr::TOPOLOGICAL_INVAILD_ERR, GetPolygonPoint(pol)));

			// 次のポリゴンへ
			//continue;
		}
	}
}

void RoadModelErrorChecker::CheckAngleErr(RoadModelData& data)
{
#ifdef DEBUG
	std::cout << "************************************* CheckAngleErr *************************************" << std::endl;
#endif // DEBUG

	// ポリゴンごとの処理
	for (auto& createdRoadModelInfo : data.GetRoadPolygonList())
	{
		if (RoadModelErrorChecker::IsInspectionInfo(createdRoadModelInfo) == false)
		{
			continue;
		}

		auto pol = createdRoadModelInfo.GetRoadData().Polygon();

		bool isAcuteAngle = false;
		std::vector<double> angles = Angles(pol);
		for (double angle : angles)
		{
			if (CEpsUtil::LessEqual(angle, (boost::math::constants::pi<double>() / 4)))
			{
				isAcuteAngle = true;
				break;
			}
		}

		if (isAcuteAngle == true)
		{
			createdRoadModelInfo.AddErr(RoadErrInfo(RoadModelErr::ANGLE_ERR, GetPolygonPoint(pol)));
		}
	}
}

void RoadModelErrorChecker::CheckIntersectionErr(RoadModelData& data)
{
#ifdef DEBUG
	std::cout << "************************************* CheckIntersectionErr *************************************" << std::endl;
#endif // DEBUG

	///
	/// すべてのポリゴンが交差点ポイントを内包しているか確認
	///
	for (auto& createdRoadModelInfo : data.GetRoadPolygonList())
	{
		// ポリゴンのタイプが交差点でなければスキップ
		if (createdRoadModelInfo.GetRoadData().Type() != RoadSectionType::ROAD_SECTION_CROSSING)
		{
			continue;
		}

		if (IsInspectionInfo(createdRoadModelInfo) == false)
		{
			continue;
		}

		auto pol = createdRoadModelInfo.GetRoadData().Polygon();

		bool isWithin = false;
		for (auto intersection : data.GetIntersectionPointList())
		{
			if (bg::within(intersection, pol))
			{
				// 交差点ポリゴンと交差点ポイントをリンク付けする
				createdRoadModelInfo.SetIntersectionPoint(intersection);

				isWithin = true;
				break;
			}
		}

		// 交差点ポイントの内包がなければエラー出力
		if (isWithin == false)
		{
			createdRoadModelInfo.AddErr(RoadErrInfo(RoadModelErr::INTERSECTION_MISMATCH_ERR, GetPolygonPoint(pol)));
		}
	}

	///
	/// すべての交差点ポイントがポリゴンを内包しているか確認
	///
	for (auto intersection : data.GetIntersectionPointList())
	{
		bool isWithin = false;

		for (auto createdRoadModelInfo : data.GetRoadPolygonList())
		{
			// ポリゴンのタイプが交差点でなければスキップ
			if (createdRoadModelInfo.GetRoadData().Type() != RoadSectionType::ROAD_SECTION_CROSSING)
			{
				continue;
			}

			if (IsInspectionInfo(createdRoadModelInfo) == false)
			{
				continue;
			}

			auto pol = createdRoadModelInfo.GetRoadData().Polygon();

			// 対象の交差点ポイントと保存されている交差点座標が同じ場合
			if (createdRoadModelInfo.GetIntersectionPoint().x() == intersection.x()
				&& createdRoadModelInfo.GetIntersectionPoint().y() == intersection.y())
			{
				if (isWithin == false)
				{
					// intersectionに対応するpolがまだなければ
					// スキップする
					isWithin = true;
					continue;
				}
				else
				{
					// intersectionに対応するpolが既にある場合は、
					// pol内に同じ座標の交差点ポイントが複数あるため
					// エラーを出力する
					createdRoadModelInfo.AddErr(RoadErrInfo(RoadModelErr::INTERSECTION_SAME_POINT_ERR, GetPolygonPoint(pol)));
					continue;
				}
			}
			else if(bg::within(intersection, pol))
			{
				if (isWithin == false)
				{
					// intersectionに対応するpolがまだなければ
					// スキップする
					isWithin = true;
					continue;
				}
				else
				{
					// intersectionに対応するpolが既にある場合は、
					// pol内に異なる座標の交差点ポイントがあるため
					// エラーを出力する
					createdRoadModelInfo.AddErr(RoadErrInfo(RoadModelErr::INTERSECTION_DIFFERENT_POINT_ERR, GetPolygonPoint(pol)));
					continue;
				}
			}
		}

		// intersectionに対応する交差点ポリゴンがなかった場合は
		// エラーを出力する
		if (isWithin == false)
		{
			data.AddErrWithEmptyPolygon(RoadErrInfo(RoadModelErr::INTERSECTION_MISMATCH_ERR, intersection));
		}
	}
}

void RoadModelErrorChecker::CheckExcessErr(RoadModelData& data)
{
#ifdef DEBUG
	std::cout << "************************************* CheckExcessErr *************************************" << std::endl;
#endif // DEBUG

	BoostMultiPolygon out = data.GetRoadBoostMultiPolygon();

	for (BoostPolygon prePolygon : data.GetPreRoadPolygonList())
	{
		// 頂点数が足りずポリゴンの形が生成できない場合はスキップ
		if (prePolygon.outer().size() < 3)
		{
			continue;
		}

		BoostMultiPolygon newOut;
		bg::difference(out, prePolygon, newOut);
		out = newOut;
	}

	for (auto item : out)
	{
		data.AddErrWithEmptyPolygon(RoadErrInfo(RoadModelErr::EXCESS_ERR, GetPolygonPoint(item)));
	}
}

void RoadModelErrorChecker::CheckSuperimposeErr(RoadModelData& data)
{
#ifdef DEBUG
	std::cout << "************************************* CheckSuperimposeErr *************************************" << std::endl;
#endif // DEBUG

	auto& polygonList = data.GetRoadPolygonList();

	for (int i = 0; i < polygonList.size(); i++)
	{
		for (int j = 0; j < polygonList.size(); j++)
		{
			if (i == j)
			{
				// 同じ組み合わせはチェックしないように
				// インデックスが同じタイミングで次の対象ポリゴンに移る
				break;
			}

			if (IsInspectionInfo(polygonList[i]) == false || IsInspectionInfo(polygonList[j]) == false)
			{
				continue;
			}

			BoostPolygon srcPolygon = polygonList[i].GetRoadData().Polygon();
			BoostPolygon dstPolygon = polygonList[j].GetRoadData().Polygon();

			///
			/// 完全一致
			///
			if (bg::equals(srcPolygon, dstPolygon))
			{
				polygonList[i].AddErr(RoadErrInfo(RoadModelErr::SUPERIMPOSE_ERR, GetPolygonPoint(srcPolygon)));
				continue;
			}

			///
			/// 内包
			///
			if (bg::within(srcPolygon, dstPolygon))
			{
				polygonList[i].AddErr(RoadErrInfo(RoadModelErr::WITHIN_ERR, GetPolygonPoint(srcPolygon)));
				continue;
			}

			///
			/// 内包
			///
			if (bg::within(dstPolygon, srcPolygon))
			{
				polygonList[i].AddErr(RoadErrInfo(RoadModelErr::WITHIN_ERR, GetPolygonPoint(dstPolygon)));
				continue;
			}
		}
	}
}

void RoadModelErrorChecker::CheckRoadDivisionErr(RoadModelData& data)
{
#ifdef DEBUG
	std::cout << "************************************* CheckRoadDivisionErr *************************************" << std::endl;
#endif // DEBUG

	for (auto& createdRoadModelInfo : data.GetRoadPolygonList())
	{
		if (createdRoadModelInfo.GetRoadData().Type() != RoadSectionType::ROAD_SECTION_CROSSING)
		{
			// 交差点ではないポリゴンはエラーチェック対象外
			continue;
		}

		if (IsInspectionInfo(createdRoadModelInfo) == false)
		{
			continue;
		}

		auto pol = createdRoadModelInfo.GetRoadData().Polygon();

		///
		/// ポリゴンの辺を求める
		///
		BoostMultiLines roadPolygonSides;
		for (size_t i = 0; i < pol.outer().size() - 1; i++)
		{
			BoostPolyline a;
			a.emplace_back(pol.outer()[i]);
			a.emplace_back(pol.outer()[i + 1]);
			roadPolygonSides.emplace_back(a);
		}

		BoostMultiLines prePolygonSides;
		for (auto preRoadPolygon : data.GetPreRoadPolygonList())
		{
			for (size_t i = 0; i < preRoadPolygon.outer().size() - 1; i++)
			{
				BoostPolyline a;
				a.emplace_back(preRoadPolygon.outer()[i]);
				a.emplace_back(preRoadPolygon.outer()[i + 1]);
				prePolygonSides.emplace_back(a);
			}
		}

		BoostMultiLines out;
		bg::difference(roadPolygonSides, prePolygonSides, out);

		///
		/// 出力された道路分割数と分岐数を比較する
		///
		if (out.size() != createdRoadModelInfo.GetRoadData().Division())
		{
			createdRoadModelInfo.AddErr(RoadErrInfo(RoadModelErr::ROAD_DIVISION_ERR, GetPolygonPoint(pol)));
		}
	}
}

void RoadModelErrorChecker::CheckMinusculePolygonErr(RoadModelData& data, double minArea)
{
#ifdef DEBUG
	std::cout << "************************************* CheckMinusculePolygonErr (Min Area: " << minArea << ") *************************************" << std::endl;
#endif // DEBUG

	for (auto& createdRoadModelInfo : data.GetRoadPolygonList())
	{
		if (IsInspectionInfo(createdRoadModelInfo) == false)
		{
			continue;
		}

		auto pol = createdRoadModelInfo.GetRoadData().Polygon();

		if (bg::area(pol) < minArea)
		{
			createdRoadModelInfo.AddErr(RoadErrInfo(RoadModelErr::MINUSCULE_POLYGON_ERR, GetPolygonPoint(pol)));
		}
	}
}

void RoadModelErrorChecker::CheckIntersectionDistanceErr(RoadModelData& data, double maxDistance)
{
#ifdef DEBUG
	std::cout << "************************************* CheckIntersectionDistanceErr (Max Distance: " << maxDistance << ") *************************************" << std::endl;
#endif // DEBUG

	for (auto& createdRoadModelInfo : data.GetRoadPolygonList())
	{
		// ポリゴンが交差点でないならスキップ
		if (createdRoadModelInfo.GetRoadData().Type() != RoadSectionType::ROAD_SECTION_CROSSING)
		{
			continue;
		}

		if (IsInspectionInfo(createdRoadModelInfo) == false)
		{
			continue;
		}

		// ポリゴンが交差点エラーがあった場合はスキップ
		bool isIntersectionErr = false;
		for (auto err : createdRoadModelInfo.GetRoadModelErrList())
		{
			if (err.GetErr() == RoadModelErr::INTERSECTION_MISMATCH_ERR
				|| err.GetErr() == RoadModelErr::INTERSECTION_SAME_POINT_ERR
				|| err.GetErr() == RoadModelErr::INTERSECTION_DIFFERENT_POINT_ERR)
			{
				isIntersectionErr = true;
			}
		}

		if (isIntersectionErr == true)
		{
			continue;
		}

		BoostPoint polygonPoint = GetPolygonPoint(createdRoadModelInfo.GetRoadData().Polygon());
		BoostPoint intersectionPoint = createdRoadModelInfo.GetIntersectionPoint();

		if (bg::distance(polygonPoint, intersectionPoint) > maxDistance)
		{
			createdRoadModelInfo.AddErr(RoadErrInfo(RoadModelErr::INTERSECTION_DISTANCE_ERR, polygonPoint));
		}

	}
}

void RoadModelErrorChecker::SaveErr(RoadModelData& data, std::string outputErrFilePath, bool isNewFile)
{

	///
	/// csvに出力する文字列を作成
	///
	std::vector<std::vector<std::string>> errMsg = CreateErrMsg(data);
    std::vector<std::vector<std::string>> errStrings;
    if (isNewFile)
    {
        // 項目名
        std::vector<std::vector<std::string>> errStrings;
        std::vector<std::string> column;
        column.emplace_back("ERROR");
        column.emplace_back("X");
        column.emplace_back("Y");
        errStrings.emplace_back(column);
        errStrings.insert(errStrings.end(), errMsg.begin(), errMsg.end());
    }
#ifdef DEBUG
	CCsvFileIO::WriteA(errStrings, outputErrFilePath, levels::err, true, isNewFile);
#else
	CCsvFileIO::WriteA(errStrings, outputErrFilePath, levels::off, false, isNewFile);
#endif //DEBUG
}

bool RoadModelErrorChecker::IsInspectionInfo(CreatedRoadModelInfo createdRoadModelInfo)
{
	// 既にトポロジー不正があった場合はfalse
	for (RoadErrInfo errInfo : createdRoadModelInfo.GetRoadModelErrList())
	{
		RoadModelErr err = errInfo.GetErr();

		if (err == RoadModelErr::TOPOLOGICAL_INVAILD_ERR
			|| err == RoadModelErr::TOPOLOGICAL_SHORTAGE_POINT_ERR
			|| err == RoadModelErr::TOPOLOGICAL_DUPLICATION_POINT_ERR)
		{
			return false;
		}
	}

	// 頂点数が足りずポリゴンの形が生成できない場合はfalse
	if (createdRoadModelInfo.GetRoadData().Polygon().outer().size() < 3)
	{
		return false;
	}

	// それ以外はtrue
	return true;
}

double RoadModelErrorChecker::Angle(BoostPoint o, BoostPoint u, BoostPoint v)
{
	BoostPolygon targetPolygon;
	double angle;
	bool isOverAngel = false;

	bg::append(targetPolygon.outer(), v);
	bg::append(targetPolygon.outer(), o);
	bg::append(targetPolygon.outer(), u);
	bg::append(targetPolygon.outer(), v);

	BoostPoint relativeU = BoostPoint(u.x() - o.x(), u.y() - o.y());
	BoostPoint relativeV = BoostPoint(v.x() - o.x(), v.y() - o.y());

	double length = std::sqrt(std::pow(relativeU.x(), 2) + std::pow(relativeU.y(), 2))
		* std::sqrt(std::pow(relativeV.x(), 2) + std::pow(relativeV.y(), 2));
	if (bg::math::equals(length, static_cast<double>(0.0)))
	{
		angle = boost::math::constants::half_pi<double>();
	}
	else
	{
		double x = (relativeU.x() * relativeV.x() + relativeU.y() * relativeV.y()) / length;
		double rounded = boost::algorithm::clamp(x, static_cast<double>(-1.0), static_cast<double>(1.0));
		angle = std::acos(rounded);
	}

	if (bg::area(targetPolygon) < 0)
	{
		angle = boost::math::constants::pi<double>() * 2 - angle;
	}

	return angle;
}

std::vector<double> RoadModelErrorChecker::Angles(BoostPolygon polygon)
{
	std::vector<double> angles;

	//// ポリゴンの各頂点ごとの処理
	// 各頂点の角度を求める
	for (size_t i = 0; i < polygon.outer().size() - 1; i++)
	{
		BoostPolygon targetPolygon;
		bool isOverAngel = false;

		if (i == 0)
		{
			angles.emplace_back(Angle(polygon.outer()[0], polygon.outer()[1], polygon.outer()[polygon.outer().size() - 2]));
		}
		else
		{
			angles.emplace_back(Angle(polygon.outer()[i], polygon.outer()[i + 1], polygon.outer()[i - 1]));
		}
	}

	return angles;
}

BoostPoint RoadModelErrorChecker::GetPolygonPoint(BoostPolygon polygon)
{
	if (polygon.outer().size() == 0)
	{
		return BoostPoint(0, 0);
	}

	double x = 0;
	double y = 0;
	size_t size = polygon.outer().size() - 1;

	for (int i = 0; i < size; i++)
	{
		x += polygon.outer()[i].x();
		y += polygon.outer()[i].y();
	}

	BoostPoint errPoint = BoostPoint(x / size, y / size);
	return errPoint;
}

std::vector<std::vector<std::string>> RoadModelErrorChecker::CreateErrMsg(RoadModelData &data)
{
#ifdef DEBUG
    ///
    /// BoostMultiPoint出力
    ///
    BoostMultiPoints output;
    for (auto createdRoadModelInfo : data.GetRoadPolygonList())
    {
        for (auto roadErrInfo : createdRoadModelInfo.GetRoadModelErrList())
        {
            output.emplace_back(roadErrInfo.GetErrPoint());
        }
    }

    CAnalyzeRoadEdgeDebugUtil debugUtil = CAnalyzeRoadEdgeDebugUtil();

    // ポリゴン出力
    std::string strPolygonShpPath = CFileUtil::Combine(strDebugFolerPath, "RoadModelErrList.shp");
    debugUtil.OutputMultiPointsToShp(output, strPolygonShpPath);
#endif // DEBUG

    ///
    /// csvに出力する文字列を作成
    ///
    std::vector<std::vector<std::string>> errStrings;

    // エラー本文
    for (auto createdRoadModelInfo : data.GetRoadPolygonList())
    {
        for (auto err : createdRoadModelInfo.GetRoadModelErrList())
        {
            std::vector<std::string> errString;

            switch (err.GetErr())
            {
            case RoadModelErr::SUCCESS:
                errString.emplace_back("SUCCESS");
                break;
            case RoadModelErr::MISSING_MODEL_ERR:
                errString.emplace_back("MISSING_MODEL_ERR");
                break;
            case RoadModelErr::TOPOLOGICAL_INVAILD_ERR:
                errString.emplace_back("TOPOLOGICAL_INVAILD_ERR");
                break;
            case RoadModelErr::TOPOLOGICAL_SHORTAGE_POINT_ERR:
                errString.emplace_back("TOPOLOGICAL_SHORTAGE_POINT_ERR");
                break;
            case RoadModelErr::TOPOLOGICAL_DUPLICATION_POINT_ERR:
                errString.emplace_back("TOPOLOGICAL_DUPLICATION_POINT_ERR");
                break;
            case RoadModelErr::ANGLE_ERR:
                errString.emplace_back("ANGLE_ERR");
                break;
            case RoadModelErr::INTERSECTION_MISMATCH_ERR:
                errString.emplace_back("INTERSECTION_MISMATCH_ERR");
                break;
            case RoadModelErr::INTERSECTION_SAME_POINT_ERR:
                errString.emplace_back("INTERSECTION_SAME_POINT_ERR");
                break;
            case RoadModelErr::INTERSECTION_DIFFERENT_POINT_ERR:
                errString.emplace_back("INTERSECTION_DIFFERENT_POINT_ERR");
                break;
            case RoadModelErr::EXCESS_ERR:
                errString.emplace_back("EXCESS_ERR");
                break;
            case RoadModelErr::SUPERIMPOSE_ERR:
                errString.emplace_back("SUPERIMPOSE_ERR");
                break;
            case RoadModelErr::WITHIN_ERR:
                errString.emplace_back("WITHIN_ERR");
                break;
            case RoadModelErr::ROAD_DIVISION_ERR:
                errString.emplace_back("ROAD_DIVISION_ERR");
                break;
            case RoadModelErr::MINUSCULE_POLYGON_ERR:
                errString.emplace_back("MINUSCULE_POLYGON_ERR");
                break;
            case RoadModelErr::INTERSECTION_DISTANCE_ERR:
                errString.emplace_back("INTERSECTION_DISTANCE_ERR");
                break;
            default:
                errString.emplace_back("");
                break;
            }
            errString.emplace_back(std::to_string(err.GetErrPoint().x()));
            errString.emplace_back(std::to_string(err.GetErrPoint().y()));

            errStrings.emplace_back(errString);
        }
    }

    return errStrings;
}

void RoadModelErrorChecker::SaveErr(std::vector<std::vector<std::string>> &errMsg, std::string outputErrFilePath)
{
    // 項目名
    std::vector<std::vector<std::string>> errStrings;
    std::vector<std::string> column;
    column.emplace_back("ERROR");
    column.emplace_back("X");
    column.emplace_back("Y");
    errStrings.emplace_back(column);
    errStrings.insert(errStrings.end(), errMsg.begin(), errMsg.end());
#ifdef DEBUG
    CCsvFileIO::WriteA(errStrings, outputErrFilePath, levels::err, true, true);
#else
    CCsvFileIO::WriteA(errStrings, outputErrFilePath, levels::off, false, true);
#endif //DEBUG
}

