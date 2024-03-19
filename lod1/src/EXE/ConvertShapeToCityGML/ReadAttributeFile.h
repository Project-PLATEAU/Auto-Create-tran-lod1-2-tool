#pragma once

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>

#define GetAttributeInstance() (ReadAttributeFile::GetInstance())

class ShpAttr
{
public:
	ShpAttr()
	{
		cityGMLAttrName = "";
		shpAttrName = "";
		IsSetting = true;
		IsContainMappingFile = false;
	}
	ShpAttr(std::string name1, std::string name2, bool isSetting)
	{
		cityGMLAttrName = name1;
		shpAttrName = name2;
		IsSetting = isSetting;
		IsContainMappingFile = true;
	}
	~ShpAttr() {}

	std::pair<std::string, std::string> GetAttrName() { return std::make_pair(cityGMLAttrName, shpAttrName); }
	bool GetIsSetting() { return IsSetting; }
	bool GetIsContainMappingFile() { return IsContainMappingFile; }

private:
	std::string cityGMLAttrName;
	std::string shpAttrName;
	bool IsSetting;
	bool IsContainMappingFile;

};

class TranRoad
{
public:
	TranRoad();
	~TranRoad();

	void Initialize(std::string filePath);

	bool GetDescription(std::pair<std::string, std::string>& str);
	bool GetName(std::pair<std::string, std::string>& str);
	bool GetCreationDate(std::pair<std::string, std::string>& str);
	bool GetTerminationDate(std::pair<std::string, std::string>& str);
	bool GetTransportationComplex(std::pair<std::string, std::string>& str);
	bool GetFunction(std::pair<std::string, std::string>& str);
	bool GetUsage(std::pair<std::string, std::string>& str);
	bool GetTranDmAttribute(std::pair<std::string, std::string>& str);
	bool GetTranDataQualityAttribute(std::pair<std::string, std::string>& str);
	bool GetRoadStructureAttribute(std::pair<std::string, std::string>& str);
	bool GetTrafficVolumeAttribute(std::pair<std::string, std::string>& str);
	void GetTranRoadStringAttributes(std::vector<std::pair<std::string, std::string>>& strVec);
	void GetTranRoadIntegerAttributes(std::vector<std::pair<std::string, std::string>>& strVec);


	void Print()
	{
		std::pair<std::string, std::string> buf;

		std::cout << "m_description: " << (GetDescription(buf) ? buf.second : "false") << std::endl
			<< "m_name: " << (GetName(buf) ? buf.second : "false") << std::endl
			<< "m_creationDate: " << (GetCreationDate(buf) ? buf.second : "false") << std::endl
			<< "m_terminationDate: " << (GetTerminationDate(buf) ? buf.second : "false") << std::endl
			<< "m_transportationComplex: " << (GetTransportationComplex(buf) ? buf.second : "false") << std::endl
			<< "m_function: " << (GetFunction(buf) ? buf.second : "false") << std::endl
			<< "m_usage: " << (GetUsage(buf) ? buf.second : "false") << std::endl
			<< "m_tranDmAttribute: " << (GetTranDmAttribute(buf) ? buf.second : "false") << std::endl
			<< "m_tranDataQualityAttribute: " << (GetTranDataQualityAttribute(buf) ? buf.second : "false") << std::endl
			<< "m_roadStructureAttribute: " << (GetRoadStructureAttribute(buf) ? buf.second : "false") << std::endl
			<< "m_trafficVolumeAttribute: " << (GetTrafficVolumeAttribute(buf) ? buf.second : "false") << std::endl;
	}

private:
	ShpAttr m_description;
	ShpAttr m_name;
	ShpAttr	m_creationDate;
	ShpAttr	m_terminationDate;
	ShpAttr	m_transportationComplex;
	ShpAttr	m_function;
	ShpAttr	m_usage;
	ShpAttr	m_tranDmAttribute;
	ShpAttr	m_tranDataQualityAttribute;
	ShpAttr	m_roadStructureAttribute;
	ShpAttr	m_trafficVolumeAttribute;
	ShpAttr	m_dmAttribute;
};

class UroTransportationDataQualityAttribute
{
public:
	UroTransportationDataQualityAttribute();
	~UroTransportationDataQualityAttribute();

	void Initialize(std::string filePath);

	bool GetSrcScale(std::pair<std::string, std::string>& str);
	bool GetGeometrySrcDesc(std::pair<std::string, std::string>& str);
	bool GetThematicSrcDesc(std::pair<std::string, std::string>& str);
	void GetUroTransportationDataQualityAttributes(std::vector<std::pair<std::string, std::string>>& strVec);

	void Print()
	{
		std::pair<std::string, std::string> buf;

		std::cout << "m_srcScale: " << (GetSrcScale(buf) ? buf.second : "false") << std::endl;
		std::cout << "m_geometrySrcDesc: " << (GetGeometrySrcDesc(buf) ? buf.second : "false") << std::endl;
		std::cout << "m_thematicSrcDesc: " << (GetThematicSrcDesc(buf) ? buf.second : "false") << std::endl;
	}

private:
	ShpAttr m_srcScale;
	ShpAttr m_geometrySrcDesc;
	ShpAttr	m_thematicSrcDesc;
};

class UroRoadStructureAttribute
{
public:
	UroRoadStructureAttribute();
	~UroRoadStructureAttribute();

	void Initialize(std::string filePath);

	bool GetWidthType(std::pair<std::string, std::string>& str);
	bool GetWidth(std::pair<std::string, std::string>& str);
	bool GetNumberOfLanes(std::pair<std::string, std::string>& str);
	bool GetSectionType(std::pair<std::string, std::string>& str);
	void GetUroRoadStructureAttributes(std::vector<std::pair<std::string, std::string>>& strVec);

	void Print()
	{
		std::pair<std::string, std::string> buf;

		std::cout << "m_widthType: " << (GetWidthType(buf) ? buf.second : "false") << std::endl
			<< "m_width: " << (GetWidth(buf) ? buf.second : "false") << std::endl
			<< "m_numberOfLanes: " << (GetNumberOfLanes(buf) ? buf.second : "false") << std::endl
			<< "m_sectionType: " << (GetSectionType(buf) ? buf.second : "false") << std::endl;
	}

private:
	ShpAttr m_widthType;
	ShpAttr m_width;
	ShpAttr m_numberOfLanes;
	ShpAttr m_sectionType;
};

class UroTrafficVolumeAttribute
{
public:
	UroTrafficVolumeAttribute();
	~UroTrafficVolumeAttribute();

	void Initialize(std::string filePath);

	bool GetSectionID(std::pair<std::string, std::string>& str);
	bool GetRouteName(std::pair<std::string, std::string>& str);
	bool GetWeekday12hourTrafficVolume(std::pair<std::string, std::string>& str);
	bool GetWeekday24hourTrafficVolume(std::pair<std::string, std::string>& str);
	bool GetLargeVehicleRate(std::pair<std::string, std::string>& str);
	bool GetCongestionRate(std::pair<std::string, std::string>& str);
	bool GetAverageTravelSpeedInCongestion(std::pair<std::string, std::string>& str);
	bool GetAverageInboundTravelSpeedCongestion(std::pair<std::string, std::string>& str);
	bool GetAverageOutboundTraelSpeedInCongestion(std::pair<std::string, std::string>& str);
	bool GetAverageInboundTravelSpeedNotCongestion(std::pair<std::string, std::string>& str);
	bool GetAverageOutboundTravelSpeedNotCongestion(std::pair<std::string, std::string>& str);
	bool GetObservationPointName(std::pair<std::string, std::string>& str);
	bool GetReference(std::pair<std::string, std::string>& str);
	bool GetSurveyYear(std::pair<std::string, std::string>& str);
	void GetUroTrafficIntegerVolumeAttributes(std::vector<std::pair<std::string, std::string>>& strVec);
	void GetUroTrafficDoubleVolumeAttributes(std::vector<std::pair<std::string, std::string>>& strVec);
	void GetUroTrafficStringVolumeAttributes(std::vector<std::pair<std::string, std::string>>& strVec);

	void Print()
	{
		std::pair<std::string, std::string> buf;

		std::cout << "m_sectionID: " << (GetSectionID(buf) ? buf.second : "false") << std::endl
			<< "m_routeName: " << (GetRouteName(buf) ? buf.second : "false") << std::endl
			<< "m_weekday12hourTrafficVolume: " << (GetWeekday12hourTrafficVolume(buf) ? buf.second : "false") << std::endl
			<< "m_weekday24hourTrafficVolume: " << (GetWeekday24hourTrafficVolume(buf) ? buf.second : "false") << std::endl
			<< "m_largeVehicleRate: " << (GetLargeVehicleRate(buf) ? buf.second : "false") << std::endl
			<< "m_congestionRate: " << (GetCongestionRate(buf) ? buf.second : "false") << std::endl
			<< "m_averageTravelSpeedInCongestion: " << (GetAverageTravelSpeedInCongestion(buf) ? buf.second : "false") << std::endl
			<< "m_averageInboundTravelSpeedCongestion: " << (GetAverageInboundTravelSpeedCongestion(buf) ? buf.second : "false") << std::endl
			<< "m_averageOutboundTravelSpeedCongestion: " << (GetAverageOutboundTraelSpeedInCongestion(buf) ? buf.second : "false") << std::endl
			<< "m_averageInboundTravelSpeedNotCongestion: " << (GetAverageInboundTravelSpeedNotCongestion(buf) ? buf.second : "false") << std::endl
			<< "m_averageOutboundTravelSpeedNotCongestion: " << (GetAverageOutboundTravelSpeedNotCongestion(buf) ? buf.second : "false") << std::endl
			<< "m_observationPointName: " << (GetObservationPointName(buf) ? buf.second : "false") << std::endl
			<< "m_reference: " << (GetReference(buf) ? buf.second : "false") << std::endl
			<< "m_surveyYear: " << (GetSurveyYear(buf) ? buf.second : "false") << std::endl;
	}

private:
	ShpAttr m_sectionID;
	ShpAttr m_routeName;
	ShpAttr m_weekday12hourTrafficVolume;
	ShpAttr m_weekday24hourTrafficVolume;
	ShpAttr m_largeVehicleRate;
	ShpAttr m_congestionRate;
	ShpAttr m_averageTravelSpeedInCongestion;
	ShpAttr m_averageInboundTravelSpeedCongestion;
	ShpAttr m_averageOutboundTravelSpeedCongestion;
	ShpAttr m_averageInboundTravelSpeedNotCongestion;
	ShpAttr m_averageOutboundTravelSpeedNotCongestion;
	ShpAttr m_observationPointName;
	ShpAttr m_reference;
	ShpAttr m_surveyYear;
};

class UroDmAttribute
{
public:
	UroDmAttribute();
	~UroDmAttribute();

	void Initialize(std::string filePath);

	bool GetDmCode(std::pair<std::string, std::string>& str);
	bool GetMeshCode(std::pair<std::string, std::string>& str);
	void GetUroDmAttributes(std::vector<std::pair<std::string, std::string>>& strVec);

	void Print()
	{
		std::pair<std::string, std::string> buf;

		std::cout << "m_widthType: " << (GetDmCode(buf) ? buf.second : "false") << std::endl
			<< "m_width: " << (GetMeshCode(buf) ? buf.second : "false") << std::endl;
	}

private:
	ShpAttr m_dmCode;
	ShpAttr m_meshCode;
};

class ReadAttributeFile
{
public:
	ReadAttributeFile();
	~ReadAttributeFile();

	void Initialize(std::string filePath);
	static ReadAttributeFile* GetInstance() { return &m_instance; }

	TranRoad* GetTranRoad();
	UroTransportationDataQualityAttribute* GetUroTransportationDataQualityAttribute();
	UroRoadStructureAttribute* GetUroRoadStructureAttribute();
	UroTrafficVolumeAttribute* GetUroTrafficVolumeAttribute();
	UroDmAttribute* GetUroDmAttribute();

	void Print()
	{
		std::cout << "[TranRoad]" << std::endl;
		m_TranRoad.Print();
		std::cout << std::endl;

		std::cout << "[UroTransportationDataQualityAttribute]" << std::endl;
		m_UroTransportationDataQualityAttribute.Print();
		std::cout << std::endl;

		std::cout << "[UroRoadStructureAttribute]" << std::endl;
		m_UroRoadStructureAttribute.Print();
		std::cout << std::endl;

		std::cout << "[UroTrafficVolumeAttribute]" << std::endl;
		m_UroTrafficVolumeAttribute.Print();
		std::cout << std::endl;

		std::cout << "[UroDmAttribute]" << std::endl;
		m_UroDmAttribute.Print();
		std::cout << std::endl;
	}

private:
	static ReadAttributeFile m_instance;

	// attribute
	TranRoad m_TranRoad;
	UroTransportationDataQualityAttribute m_UroTransportationDataQualityAttribute;
	UroRoadStructureAttribute m_UroRoadStructureAttribute;
	UroTrafficVolumeAttribute m_UroTrafficVolumeAttribute;
	UroDmAttribute m_UroDmAttribute;
};