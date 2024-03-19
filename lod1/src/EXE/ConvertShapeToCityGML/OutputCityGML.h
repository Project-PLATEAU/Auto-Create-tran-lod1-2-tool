#pragma once
#define _AMD64_
#define WIN32_LEAN_AND_MEAN 
#include <windows.h>

#include <vector>
#include <set>
#include <variant>
#include "MeshSplitter.h"
#include "CDMDataManager.h"

using VariantType = std::variant<std::pair<std::string, int>, std::pair<std::string, double>, std::pair<std::string, std::string>>;

#define NAME_SPACE _T("xmlns:core='http://www.opengis.net/citygml/2.0' xmlns:bldg='http://www.opengis.net/citygml/building/2.0' xmlns:gen='http://www.opengis.net/citygml/generics/2.0' xmlns:gml='http://www.opengis.net/gml' xmlns:app='http://www.opengis.net/citygml/appearance/2.0' xmlns:uro='https://www.geospatial.jp/iur/uro/2.0'")
#define DEM_NAME_SPACE _T("xmlns:core='http://www.opengis.net/citygml/2.0' xmlns:dem='http://www.opengis.net/citygml/relief/2.0' xmlns:gml='http://www.opengis.net/gml'")
#define CRS "6668"

class OutputCityGML
{
public:
	OutputCityGML();
	~OutputCityGML();
	bool Run(
		std::vector<PolygonData> polygonData,
		std::set<CString> meshCodes,
		std::set<CString>& targetMeshCodes,
		std::vector<AttributeDataManager> attrDataVec,
		int JPZone,
		int meshLevel,
		std::string outputFilePath
	);
private:

};

