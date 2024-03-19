#include "ReadAttributeFile.h"

ReadAttributeFile ReadAttributeFile::m_instance;

ReadAttributeFile::ReadAttributeFile() :
	m_TranRoad(),
	m_UroTransportationDataQualityAttribute(),
	m_UroRoadStructureAttribute(),
	m_UroTrafficVolumeAttribute(),
	m_UroDmAttribute()
{
}

ReadAttributeFile::~ReadAttributeFile()
{
}

void ReadAttributeFile::Initialize(std::string filePath)
{
	m_TranRoad.Initialize(filePath);
	m_UroTransportationDataQualityAttribute.Initialize(filePath);
	m_UroRoadStructureAttribute.Initialize(filePath);
	m_UroTrafficVolumeAttribute.Initialize(filePath);
	m_UroDmAttribute.Initialize(filePath);
}

TranRoad* ReadAttributeFile::GetTranRoad()
{
	return &m_TranRoad;
}

UroTransportationDataQualityAttribute* ReadAttributeFile::GetUroTransportationDataQualityAttribute()
{
	return &m_UroTransportationDataQualityAttribute;
}

UroRoadStructureAttribute* ReadAttributeFile::GetUroRoadStructureAttribute()
{
	return &m_UroRoadStructureAttribute;
}

UroTrafficVolumeAttribute* ReadAttributeFile::GetUroTrafficVolumeAttribute()
{
	return &m_UroTrafficVolumeAttribute;
}

UroDmAttribute* ReadAttributeFile::GetUroDmAttribute()
{
	return &m_UroDmAttribute;
}

TranRoad::TranRoad() :
	m_description(),
	m_name(),
	m_creationDate(),
	m_terminationDate(),
	m_transportationComplex(),
	m_function(),
	m_usage(),
	m_tranDmAttribute(),
	m_tranDataQualityAttribute(),
	m_roadStructureAttribute(),
	m_trafficVolumeAttribute()
{

}

TranRoad::~TranRoad()
{

}

void TranRoad::Initialize(std::string filePath)
{
	std::ifstream ifs(filePath);
	std::string line;
	bool isFirst = true;

	while (std::getline(ifs, line))
	{
		std::stringstream ss(line);
		std::string isAttr;
		std::string cityGMLAttrName;
		std::string shpAttrName;
		bool isSetting = true;

		std::getline(ss, isAttr, ',');
		std::getline(ss, cityGMLAttrName, ',');
		std::getline(ss, shpAttrName, ',');

		if (isFirst == true)
		{
			isFirst = false;
			continue;
		}

		size_t stringLength = shpAttrName.length();
		isAttr == "1" ? isSetting = true : isSetting = false;

		// Check null string
		if (shpAttrName == "" || stringLength == 0)
		{
			shpAttrName = "";
		}

		// Check too length string
		if (stringLength > 10)
		{
			shpAttrName = "";
		}

		if (cityGMLAttrName == "gml:description")
		{
			m_description = ShpAttr(cityGMLAttrName, shpAttrName, isSetting);
		}
		else if (cityGMLAttrName == "gml:name")
		{
			m_name = ShpAttr(cityGMLAttrName, shpAttrName, isSetting);
		}
		else if (cityGMLAttrName == "core:creationDate")
		{
			m_creationDate = ShpAttr(cityGMLAttrName, shpAttrName, isSetting);
		}
		else if (cityGMLAttrName == "core:terminationDate")
		{
			m_terminationDate = ShpAttr(cityGMLAttrName, shpAttrName, isSetting);
		}
		else if (cityGMLAttrName == "tran:class")
		{
			m_transportationComplex = ShpAttr(cityGMLAttrName, shpAttrName, isSetting);
		}
		else if (cityGMLAttrName == "tran:function")
		{
			m_function = ShpAttr(cityGMLAttrName, shpAttrName, isSetting);
		}
		else if (cityGMLAttrName == "tran:usage")
		{
			m_usage = ShpAttr(cityGMLAttrName, shpAttrName, isSetting);
		}
		else if (cityGMLAttrName == "uro:tranDmAttribute")
		{
			m_tranDmAttribute = ShpAttr(cityGMLAttrName, shpAttrName, isSetting);
		}
		else if (cityGMLAttrName == "uro:tranDataQualityAttribute")
		{
			m_tranDataQualityAttribute = ShpAttr(cityGMLAttrName, shpAttrName, isSetting);
		}
		else if (cityGMLAttrName == "uro:roadStructureAttribute")
		{
			m_roadStructureAttribute = ShpAttr(cityGMLAttrName, shpAttrName, isSetting);
		}
		else if (cityGMLAttrName == "uro:trafficVolumeAttribute")
		{
			m_trafficVolumeAttribute = ShpAttr(cityGMLAttrName, shpAttrName, isSetting);
		}
		else
		{
			// 他クラスのステータスを読み込む場合もあるため
			// エラーはスキップする
		}
	}
}


bool TranRoad::GetDescription(std::pair<std::string, std::string>& str)
{
	str = m_description.GetAttrName();
	return str.second != "" && m_description.GetIsSetting();
}

bool TranRoad::GetName(std::pair<std::string, std::string>& str)
{
	str = m_name.GetAttrName();
	return str.second != "" && m_name.GetIsSetting();
}

bool TranRoad::GetCreationDate(std::pair<std::string, std::string>& str)
{
	str = m_creationDate.GetAttrName();
	return str.second != "" && m_creationDate.GetIsSetting();
}

bool TranRoad::GetTerminationDate(std::pair<std::string, std::string>& str)
{
	str = m_terminationDate.GetAttrName();
	return str.second != "" && m_terminationDate.GetIsSetting();
}

bool TranRoad::GetTransportationComplex(std::pair<std::string, std::string>& str)
{
	str = m_transportationComplex.GetAttrName();
	return str.second != "" && m_transportationComplex.GetIsSetting();
}

bool TranRoad::GetFunction(std::pair<std::string, std::string>& str)
{
	str = m_function.GetAttrName();
	return str.second != "" && m_function.GetIsSetting();
}

bool TranRoad::GetUsage(std::pair<std::string, std::string>& str)
{
	str = m_usage.GetAttrName();
	return str.second != "" && m_usage.GetIsSetting();
}

bool TranRoad::GetTranDmAttribute(std::pair<std::string, std::string>& str)
{
	str = m_tranDmAttribute.GetAttrName();
	return str.second != "" && m_tranDmAttribute.GetIsSetting();
}

bool TranRoad::GetTranDataQualityAttribute(std::pair<std::string, std::string>& str)
{
	str = m_tranDataQualityAttribute.GetAttrName();
	return str.second != "" && m_tranDataQualityAttribute.GetIsSetting();
}

bool TranRoad::GetRoadStructureAttribute(std::pair<std::string, std::string>& str)
{
	str = m_roadStructureAttribute.GetAttrName();
	return str.second != "" && m_roadStructureAttribute.GetIsSetting();
}

bool TranRoad::GetTrafficVolumeAttribute(std::pair<std::string, std::string>& str)
{
	str = m_trafficVolumeAttribute.GetAttrName();
	return str.second != "" && m_trafficVolumeAttribute.GetIsSetting();
}

void TranRoad::GetTranRoadStringAttributes(std::vector<std::pair<std::string, std::string>>& strVec)
{
	std::pair<std::string, std::string> str;
	if (GetDescription(str))
	{
		strVec.push_back(str);
	}
	if (GetName(str))
	{
		strVec.push_back(str);
	}
	if (GetCreationDate(str))
	{
		strVec.push_back(str);
	}
	if (GetTerminationDate(str))
	{
		strVec.push_back(str);
	}
	if (GetTranDmAttribute(str))
	{
		strVec.push_back(str);
	}
	if (GetTranDataQualityAttribute(str))
	{
		strVec.push_back(str);
	}
	if (GetRoadStructureAttribute(str))
	{
		strVec.push_back(str);
	}
	if (GetTrafficVolumeAttribute(str))
	{
		strVec.push_back(str);
	}
}

void TranRoad::GetTranRoadIntegerAttributes(std::vector<std::pair<std::string, std::string>>& strVec)
{
	std::pair<std::string, std::string> str;

	if (GetTransportationComplex(str))
	{
		strVec.push_back(str);
		return;
	}
	if (GetFunction(str))
	{
		strVec.push_back(str);
		return;
	}
	if (GetUsage(str))
	{
		strVec.push_back(str);
		return;
	}
}

UroTransportationDataQualityAttribute::UroTransportationDataQualityAttribute() :
	m_srcScale(),
	m_geometrySrcDesc(),
	m_thematicSrcDesc()
{

}

UroTransportationDataQualityAttribute::~UroTransportationDataQualityAttribute()
{

}

void UroTransportationDataQualityAttribute::Initialize(std::string filePath)
{
	std::ifstream ifs(filePath);
	std::string line;
	bool isFirst = true;

	while (std::getline(ifs, line))
	{
		std::stringstream ss(line);
		std::string isAttr;
		std::string cityGMLAttrName;
		std::string shpAttrName;
		bool isSetting = true;

		std::getline(ss, isAttr, ',');
		std::getline(ss, cityGMLAttrName, ',');
		std::getline(ss, shpAttrName, ',');

		if (isFirst == true)
		{
			isFirst = false;
			continue;
		}

		size_t stringLength = shpAttrName.length();
		isSetting = (isAttr == "1") ? true : false;

		// Check null string
		if (shpAttrName == "" || stringLength == 0)
		{
			shpAttrName = "";
		}

		// Check too length string
		if (stringLength > 10)
		{
			shpAttrName = "";
		}

		if (cityGMLAttrName == "uro:srcScale")
		{
			m_srcScale = ShpAttr(cityGMLAttrName, shpAttrName, isSetting);
		}
		else if (cityGMLAttrName == "uro:geometrySrcDesc")
		{
			m_geometrySrcDesc = ShpAttr(cityGMLAttrName, shpAttrName, isSetting);
		}
		else if (cityGMLAttrName == "uro:thematicSrcDesc")
		{
			m_thematicSrcDesc = ShpAttr(cityGMLAttrName, shpAttrName, isSetting);
		}
		else
		{
			// 他クラスのステータスを読み込む場合もあるため
			// エラーはスキップする
		}

	}

}

bool UroTransportationDataQualityAttribute::GetSrcScale(std::pair<std::string, std::string>& str)
{
	str = m_srcScale.GetAttrName();
	return str.second != "" && m_srcScale.GetIsSetting();
}

bool UroTransportationDataQualityAttribute::GetGeometrySrcDesc(std::pair<std::string, std::string>& str)
{
	str = m_geometrySrcDesc.GetAttrName();
	return str.second != "" && m_geometrySrcDesc.GetIsSetting();
}

bool UroTransportationDataQualityAttribute::GetThematicSrcDesc(std::pair<std::string, std::string>& str)
{
	str = m_thematicSrcDesc.GetAttrName();
	return str.second != "" && m_thematicSrcDesc.GetIsSetting();
}

void UroTransportationDataQualityAttribute::GetUroTransportationDataQualityAttributes(std::vector<std::pair<std::string, std::string>>& strVec)
{
	std::pair<std::string, std::string> str;
	if (GetSrcScale(str))
	{
		strVec.push_back(str);
	}
	if (GetGeometrySrcDesc(str))
	{
		strVec.push_back(str);
	}
	if (GetThematicSrcDesc(str))
	{
		strVec.push_back(str);
	}
}


UroRoadStructureAttribute::UroRoadStructureAttribute() :
	m_widthType(),
	m_width(),
	m_numberOfLanes(),
	m_sectionType()
{

}

UroRoadStructureAttribute::~UroRoadStructureAttribute()
{

}

void UroRoadStructureAttribute::Initialize(std::string filePath)
{
	std::ifstream ifs(filePath);
	std::string line;
	bool isFirst = true;

	while (std::getline(ifs, line))
	{
		std::stringstream ss(line);
		std::string isAttr;
		std::string cityGMLAttrName;
		std::string shpAttrName;
		bool isSetting = true;

		std::getline(ss, isAttr, ',');
		std::getline(ss, cityGMLAttrName, ',');
		std::getline(ss, shpAttrName, ',');

		if (isFirst == true)
		{
			isFirst = false;
			continue;
		}

		size_t stringLength = shpAttrName.length();
		isSetting = (isAttr == "1") ? true : false;

		// Check null string
		if (shpAttrName == "" || stringLength == 0)
		{
			shpAttrName = "";
		}

		// Check too length string
		if (stringLength > 10)
		{
			shpAttrName = "";
		}

		if (cityGMLAttrName == "uro:widthType")
		{
			m_widthType = ShpAttr(cityGMLAttrName, shpAttrName, isSetting);
		}
		else if (cityGMLAttrName == "uro:width")
		{
			m_width = ShpAttr(cityGMLAttrName, shpAttrName, isSetting);
		}
		else if (cityGMLAttrName == "uro:numberOfLanes")
		{
			m_numberOfLanes = ShpAttr(cityGMLAttrName, shpAttrName, isSetting);
		}
		else if (cityGMLAttrName == "uro:sectionType")
		{
			m_sectionType = ShpAttr(cityGMLAttrName, shpAttrName, isSetting);
		}
		else
		{
			// 他クラスのステータスを読み込む場合もあるため
			// エラーはスキップする
		}

		// Output error
	}

}

bool UroRoadStructureAttribute::GetWidthType(std::pair<std::string, std::string>& str)
{
	str = m_widthType.GetAttrName();
	return str.second != "" && m_widthType.GetIsSetting();
}

bool UroRoadStructureAttribute::GetWidth(std::pair<std::string, std::string>& str)
{
	str = m_width.GetAttrName();
	return str.second != "" && m_width.GetIsSetting();
}

bool UroRoadStructureAttribute::GetNumberOfLanes(std::pair<std::string, std::string>& str)
{
	str = m_numberOfLanes.GetAttrName();
	return str.second != "" && m_numberOfLanes.GetIsSetting();
}

bool UroRoadStructureAttribute::GetSectionType(std::pair<std::string, std::string>& str)
{
	str = m_sectionType.GetAttrName();
	return str.second != "" && m_sectionType.GetIsSetting();
}

void UroRoadStructureAttribute::GetUroRoadStructureAttributes(std::vector<std::pair<std::string, std::string>>& strVec)
{
	std::pair<std::string, std::string> str;
	if (GetWidthType(str))
	{
		strVec.push_back(str);
	}
	if (GetWidth(str))
	{
		strVec.push_back(str);
	}
	if (GetNumberOfLanes(str))
	{
		strVec.push_back(str);
	}
	if (GetSectionType(str))
	{
		strVec.push_back(str);
	}
}


UroTrafficVolumeAttribute::UroTrafficVolumeAttribute() :
	m_sectionID(),
	m_routeName(),
	m_weekday12hourTrafficVolume(),
	m_weekday24hourTrafficVolume(),
	m_largeVehicleRate(),
	m_congestionRate(),
	m_averageTravelSpeedInCongestion(),
	m_averageInboundTravelSpeedCongestion(),
	m_averageOutboundTravelSpeedCongestion(),
	m_averageInboundTravelSpeedNotCongestion(),
	m_averageOutboundTravelSpeedNotCongestion(),
	m_observationPointName(),
	m_reference(),
	m_surveyYear()
{

}

UroTrafficVolumeAttribute::~UroTrafficVolumeAttribute()
{

}

void UroTrafficVolumeAttribute::Initialize(std::string filePath)
{
	std::ifstream ifs(filePath);
	std::string line;
	bool isFirst = true;

	while (std::getline(ifs, line))
	{
		std::stringstream ss(line);
		std::string isAttr;
		std::string cityGMLAttrName;
		std::string shpAttrName;
		bool isSetting = true;

		std::getline(ss, isAttr, ',');
		std::getline(ss, cityGMLAttrName, ',');
		std::getline(ss, shpAttrName, ',');

		if (isFirst == true)
		{
			isFirst = false;
			continue;
		}

		size_t stringLength = shpAttrName.length();
		isSetting = (isAttr == "1") ? true : false;

		// Check null string
		if (shpAttrName == "" || stringLength == 0)
		{
			shpAttrName = "";
		}

		// Check too length string
		if (stringLength > 10)
		{
			shpAttrName = "";
		}

		if (cityGMLAttrName == "uro:sectionID")
		{
			m_sectionID = ShpAttr(cityGMLAttrName, shpAttrName, isSetting);
		}
		else if (cityGMLAttrName == "uro:routeName")
		{
			m_routeName = ShpAttr(cityGMLAttrName, shpAttrName, isSetting);
		}
		else if (cityGMLAttrName == "uro:weekday12hourTrafficVolume")
		{
			m_weekday12hourTrafficVolume = ShpAttr(cityGMLAttrName, shpAttrName, isSetting);
		}
		else if (cityGMLAttrName == "uro:weekday24hourTrafficVolume")
		{
			m_weekday24hourTrafficVolume = ShpAttr(cityGMLAttrName, shpAttrName, isSetting);
		}
		else if (cityGMLAttrName == "uro:largeVehicleRate")
		{
			m_largeVehicleRate = ShpAttr(cityGMLAttrName, shpAttrName, isSetting);
		}
		else if (cityGMLAttrName == "uro:congestionRate")
		{
			m_congestionRate = ShpAttr(cityGMLAttrName, shpAttrName, isSetting);
		}
		else if (cityGMLAttrName == "uro:averageTravelSpeedInCongestion")
		{
			m_averageTravelSpeedInCongestion = ShpAttr(cityGMLAttrName, shpAttrName, isSetting);
		}
		else if (cityGMLAttrName == "uro:averageInboundTravelSpeedCongestion")
		{
			m_averageInboundTravelSpeedCongestion = ShpAttr(cityGMLAttrName, shpAttrName, isSetting);
		}
		else if (cityGMLAttrName == "uro:averageOutboundTraelSpeedInCongestion")
		{
			m_averageOutboundTravelSpeedCongestion = ShpAttr(cityGMLAttrName, shpAttrName, isSetting);
		}
		else if (cityGMLAttrName == "uro:averageInboundTravelSpeedNotCongestion")
		{
			m_averageInboundTravelSpeedNotCongestion = ShpAttr(cityGMLAttrName, shpAttrName, isSetting);
		}
		else if (cityGMLAttrName == "uro:averageOutboundTravelSpeedNotCongestion")
		{
			m_averageOutboundTravelSpeedNotCongestion = ShpAttr(cityGMLAttrName, shpAttrName, isSetting);
		}
		else if (cityGMLAttrName == "uro:observationPointName")
		{
			m_observationPointName = ShpAttr(cityGMLAttrName, shpAttrName, isSetting);
		}
		else if (cityGMLAttrName == "uro:reference")
		{
			m_reference = ShpAttr(cityGMLAttrName, shpAttrName, isSetting);
		}
		else if (cityGMLAttrName == "uro:surveyYear")
		{
			m_surveyYear = ShpAttr(cityGMLAttrName, shpAttrName, isSetting);
		}
		else
		{
			// 他クラスのステータスを読み込む場合もあるため
			// エラーはスキップする
		}

	}

}

bool UroTrafficVolumeAttribute::GetSectionID(std::pair<std::string, std::string>& str)
{
	str = m_sectionID.GetAttrName();
	return str.second != "" && m_sectionID.GetIsSetting();
}

bool UroTrafficVolumeAttribute::GetRouteName(std::pair<std::string, std::string>& str)
{
	str = m_routeName.GetAttrName();
	return str.second != "" && m_routeName.GetIsSetting();
}

bool UroTrafficVolumeAttribute::GetWeekday12hourTrafficVolume(std::pair<std::string, std::string>& str)
{
	str = m_weekday12hourTrafficVolume.GetAttrName();
	return str.second != "" && m_weekday12hourTrafficVolume.GetIsSetting();
}

bool UroTrafficVolumeAttribute::GetWeekday24hourTrafficVolume(std::pair<std::string, std::string>& str)
{
	str = m_weekday24hourTrafficVolume.GetAttrName();
	return str.second != "" && m_weekday24hourTrafficVolume.GetIsSetting();
}

bool UroTrafficVolumeAttribute::GetLargeVehicleRate(std::pair<std::string, std::string>& str)
{
	str = m_largeVehicleRate.GetAttrName();
	return str.second != "" && m_largeVehicleRate.GetIsSetting();
}

bool UroTrafficVolumeAttribute::GetCongestionRate(std::pair<std::string, std::string>& str)
{
	str = m_congestionRate.GetAttrName();
	return str.second != "" && m_congestionRate.GetIsSetting();
}

bool UroTrafficVolumeAttribute::GetAverageTravelSpeedInCongestion(std::pair<std::string, std::string>& str)
{
	str = m_averageTravelSpeedInCongestion.GetAttrName();
	return str.second != "" && m_averageTravelSpeedInCongestion.GetIsSetting();
}

bool UroTrafficVolumeAttribute::GetAverageInboundTravelSpeedCongestion(std::pair<std::string, std::string>& str)
{
	str = m_averageInboundTravelSpeedCongestion.GetAttrName();
	return str.second != "" && m_averageInboundTravelSpeedCongestion.GetIsSetting();
}

bool UroTrafficVolumeAttribute::GetAverageOutboundTraelSpeedInCongestion(std::pair<std::string, std::string>& str)
{
	str = m_averageOutboundTravelSpeedCongestion.GetAttrName();
	return str.second != "" && m_averageOutboundTravelSpeedCongestion.GetIsSetting();
}

bool UroTrafficVolumeAttribute::GetAverageInboundTravelSpeedNotCongestion(std::pair<std::string, std::string>& str)
{
	str = m_averageInboundTravelSpeedNotCongestion.GetAttrName();
	return str.second != "" && m_averageInboundTravelSpeedNotCongestion.GetIsSetting();
}

bool UroTrafficVolumeAttribute::GetAverageOutboundTravelSpeedNotCongestion(std::pair<std::string, std::string>& str)
{
	str = m_averageOutboundTravelSpeedNotCongestion.GetAttrName();
	return str.second != "" && m_averageOutboundTravelSpeedNotCongestion.GetIsSetting();
}

bool UroTrafficVolumeAttribute::GetObservationPointName(std::pair<std::string, std::string>& str)
{
	str = m_observationPointName.GetAttrName();
	return str.second != "" && m_observationPointName.GetIsSetting();
}

bool UroTrafficVolumeAttribute::GetReference(std::pair<std::string, std::string>& str)
{
	str = m_reference.GetAttrName();
	return str.second != "" && m_reference.GetIsSetting();
}

bool UroTrafficVolumeAttribute::GetSurveyYear(std::pair<std::string, std::string>& str)
{
	str = m_surveyYear.GetAttrName();
	return str.second != "" && m_surveyYear.GetIsSetting();
}

void UroTrafficVolumeAttribute::GetUroTrafficIntegerVolumeAttributes(std::vector<std::pair<std::string, std::string>>& strVec)
{
	std::pair<std::string, std::string> str;
	if (GetWeekday12hourTrafficVolume(str))
	{
		strVec.push_back(str);
		return;
	}
	if (GetWeekday24hourTrafficVolume(str))
	{
		strVec.push_back(str);
		return;
	}
}
void UroTrafficVolumeAttribute::GetUroTrafficDoubleVolumeAttributes(std::vector<std::pair<std::string, std::string>>& strVec)
{
	std::pair<std::string, std::string> str;
	if (GetLargeVehicleRate(str))
	{
		strVec.push_back(str);
		return;
	}
	if (GetCongestionRate(str))
	{
		strVec.push_back(str);
		return;
	}
	if (GetAverageTravelSpeedInCongestion(str))
	{
		strVec.push_back(str);
		return;
	}
	if (GetAverageInboundTravelSpeedCongestion(str))
	{
		strVec.push_back(str);
		return;
	}
	if (GetAverageOutboundTraelSpeedInCongestion(str))
	{
		strVec.push_back(str);
		return;
	}
	if (GetAverageInboundTravelSpeedNotCongestion(str))
	{
		strVec.push_back(str);
		return;
	}
	if (GetAverageOutboundTravelSpeedNotCongestion(str))
	{
		strVec.push_back(str);
		return;
	}
}
void UroTrafficVolumeAttribute::GetUroTrafficStringVolumeAttributes(std::vector<std::pair<std::string, std::string>>& strVec)
{
	std::pair<std::string, std::string> str;
	if (GetSectionID(str))
	{
		strVec.push_back(str);
		return;
	}
	if (GetRouteName(str))
	{
		strVec.push_back(str);
		return;
	}
	if (GetObservationPointName(str))
	{
		strVec.push_back(str);
		return;
	}
	if (GetReference(str))
	{
		strVec.push_back(str);
		return;
	}
	if (GetSurveyYear(str))
	{
		strVec.push_back(str);
		return;
	}
}

UroDmAttribute::UroDmAttribute() :
	m_dmCode(),
	m_meshCode()
{

}

UroDmAttribute::~UroDmAttribute()
{

}

void UroDmAttribute::Initialize(std::string filePath)
{
	std::ifstream ifs(filePath);
	std::string line;
	bool isFirst = true;

	while (std::getline(ifs, line))
	{
		std::stringstream ss(line);
		std::string isAttr;
		std::string cityGMLAttrName;
		std::string shpAttrName;
		bool isSetting = true;

		std::getline(ss, isAttr, ',');
		std::getline(ss, cityGMLAttrName, ',');
		std::getline(ss, shpAttrName, ',');

		if (isFirst == true)
		{
			isFirst = false;
			continue;
		}

		size_t stringLength = shpAttrName.length();
		isSetting = (isAttr == "1") ? true : false;

		// Check null string
		if (shpAttrName == "" || stringLength == 0)
		{
			shpAttrName = "";
		}

		// Check too length string
		if (stringLength > 10)
		{
			shpAttrName = "";
		}

		if (cityGMLAttrName == "uro:dmCode")
		{
			m_dmCode = ShpAttr(cityGMLAttrName, shpAttrName, isSetting);
		}
		else if (cityGMLAttrName == "uro:meshCode")
		{
			m_meshCode = ShpAttr(cityGMLAttrName, shpAttrName, isSetting);
		}
		else
		{
			// 他クラスのステータスを読み込む場合もあるため
			// エラーはスキップする
		}

		// Output error
	}

}

bool UroDmAttribute::GetDmCode(std::pair<std::string, std::string>& str)
{
	str = m_dmCode.GetAttrName();
	return str.second != "" && m_dmCode.GetIsSetting();
}

bool UroDmAttribute::GetMeshCode(std::pair<std::string, std::string>& str)
{
	str = m_meshCode.GetAttrName();
	return str.second != "" && m_meshCode.GetIsSetting();
}


void UroDmAttribute::GetUroDmAttributes(std::vector<std::pair<std::string, std::string>>& strVec)
{
	std::pair<std::string, std::string> str;
	if (GetDmCode(str))
	{
		strVec.push_back(str);
	}
	if (GetMeshCode(str))
	{
		strVec.push_back(str);
	}
}
