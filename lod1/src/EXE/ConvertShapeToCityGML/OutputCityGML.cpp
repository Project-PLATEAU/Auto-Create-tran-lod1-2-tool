#include "OutputCityGML.h"

#import "msxml6.dll" named_guids raw_interfaces_only
#include <atlbase.h>	// CComVariant, CComBSTR
#include <iostream>
#include <string>
#include <boost/geometry.hpp>

using namespace MSXML2;
OutputCityGML::OutputCityGML()
{
}

OutputCityGML::~OutputCityGML()
{
}

void AddNewline(MSXML2::IXMLDOMDocumentPtr node, MSXML2::IXMLDOMElementPtr element)
{
	MSXML2::IXMLDOMTextPtr pNewLine;
	std::string newLine = "\n";
	node->createTextNode(_bstr_t(newLine.c_str()), &pNewLine);
	element->appendChild(pNewLine, NULL);
}

void AddNewlineAndIndent(MSXML2::IXMLDOMDocumentPtr node, MSXML2::IXMLDOMElementPtr element, int count)
{
	if (count < 0)
	{
		return;
	}
	MSXML2::IXMLDOMTextPtr pNewLine;
	std::string newLine = "\n";
	std::string indent = "\t";
	for (int index = 0; index < count; index++)
	{
		indent += "\t";
	}
	std::string newLineIndent = newLine + indent;
	node->createTextNode(_bstr_t(newLineIndent.c_str()), &pNewLine);
	element->appendChild(pNewLine, NULL);
}

void appendChild(
	MSXML2::IXMLDOMDocumentPtr& node,
	MSXML2::IXMLDOMElementPtr& rootElement,
	std::list<MSXML2::IXMLDOMElementPtr> appendElement,
	int& indentCount,
	bool isEnd = false)
{
	for (auto var : appendElement)
	{
		AddNewlineAndIndent(node, rootElement, indentCount);
		rootElement->appendChild(var, NULL);
	}
	if (isEnd == false)
	{
		AddNewlineAndIndent(node, rootElement, --indentCount);
	}
	else
	{
		--indentCount;
	}
}

bool OutputCityGML::Run(
	std::vector<PolygonData> polygonData,
	std::set<CString> meshCodes,
	std::set<CString>& targetMeshCodes,
	std::vector<AttributeDataManager> attrDataVec,
	int JPZone,
	int meshLevel,
	std::string outputFilePath
)
{
	for (CString meshCode : meshCodes)
	{
		// GML�t�@�C���������ݏ���
		HRESULT hResult;
		hResult = CoInitialize(NULL); // ������
		std::string meshId;

		if (SUCCEEDED(hResult)) {
			try {
				MSXML2::IXMLDOMDocumentPtr pXMLDoc;
				hResult = pXMLDoc.CreateInstance(__uuidof(MSXML2::DOMDocument60));
				if (SUCCEEDED(hResult))
				{
					int indentCount = 0;
					int attrIndentCount = 0;
					pXMLDoc->put_async(VARIANT_FALSE);
					pXMLDoc->put_validateOnParse(VARIANT_FALSE);
					pXMLDoc->put_resolveExternals(VARIANT_FALSE);

					MSXML2::IXMLDOMProcessingInstructionPtr pi;
					pXMLDoc->createProcessingInstruction(SysAllocString(L"xml"), SysAllocString(L"version='1.0' encoding='UTF-8'"), &pi);
					pXMLDoc->appendChild(pi, NULL);

					// CityGML���[�g�v�f���쐬       
					MSXML2::IXMLDOMNodePtr pCityModel;
					pXMLDoc->createNode(_variant_t(MSXML2::NODE_ELEMENT), _bstr_t("core:CityModel"), _bstr_t("http://www.opengis.net/citygml/transportation/2.0"), &pCityModel);     //<CityModel>���쐬

					MSXML2::IXMLDOMElementPtr peCityModel;
					VARIANT varAttributeValue;
					VariantInit(&varAttributeValue);
					varAttributeValue.vt = VT_BSTR;
					varAttributeValue.bstrVal = SysAllocString(L"http://www.opengis.net/citygml/transportation/2.0");
					pXMLDoc->createElement(_bstr_t("core:CityModel"), &peCityModel);
					peCityModel->setAttribute(_bstr_t("xmlns:tran"), varAttributeValue);

					VariantInit(&varAttributeValue);
					varAttributeValue.vt = VT_BSTR;
					varAttributeValue.bstrVal = SysAllocString(L"http://www.opengis.net/citygml/2.0");
					peCityModel->setAttribute(SysAllocString(L"xmlns:core"), varAttributeValue);

					VariantInit(&varAttributeValue);
					varAttributeValue.vt = VT_BSTR;
					varAttributeValue.bstrVal = SysAllocString(L"http://www.opengis.net/gml");
					peCityModel->setAttribute(SysAllocString(L"xmlns:gml"), varAttributeValue);

					VariantInit(&varAttributeValue);
					varAttributeValue.vt = VT_BSTR;
					varAttributeValue.bstrVal = SysAllocString(L"https://www.geospatial.jp/iur/uro/2.0");
					peCityModel->setAttribute(SysAllocString(L"xmlns:uro"), varAttributeValue);

					// ���W�n�Ɣ͈͂̏��
					MSXML2::IXMLDOMElementPtr pBoundedBy;
					pXMLDoc->createElement(_bstr_t("gml:boundedBy"), &pBoundedBy);     //<gml:boundedBy>���쐬

					MSXML2::IXMLDOMElementPtr pEnvelope;
					pXMLDoc->createElement(_bstr_t("gml:Envelope"), &pEnvelope);     //<gml:Envelope>���쐬
					VariantInit(&varAttributeValue);
					varAttributeValue.vt = VT_BSTR;
					varAttributeValue.bstrVal = SysAllocString(L"http://www.opengis.net/def/crs/EPSG/0/6697");
					pEnvelope->setAttribute(SysAllocString(L"srsName"), varAttributeValue);
					pEnvelope->setAttribute(SysAllocString(L"srsDimension"), variant_t("3"));

					// ���b�V���R�[�h������W���擾
					MSXML2::IXMLDOMElementPtr pLowerCorner;
					pXMLDoc->createElement(_bstr_t("gml:lowerCorner"), &pLowerCorner);     //<gml:lowerCorner>���쐬
					MSXML2::IXMLDOMElementPtr pUpperCorner;
					pXMLDoc->createElement(_bstr_t("gml:upperCorner"), &pUpperCorner);     //<gml:upperCorner>���쐬
					typedef boost::geometry::model::d2::point_xy<double> PointType;
					typedef boost::geometry::model::polygon<PointType> PolygonType;
					typedef boost::geometry::model::multi_polygon<PolygonType> MultiPolygonType;
					MultiPolygonType multiPolygon;
					for (auto polygon : polygonData)
					{
						CString polygonMeshCode = polygon.meshCode;
						std::vector<Point> pointData = polygon.vertices;
						PolygonType polygon;

						// �������b�V���R�[�h�����m�F
						if (meshCode != polygonMeshCode)
						{
							// �قȂ郁�b�V���R�[�h�������ꍇ�X�L�b�v
							continue;
						}

						for (auto point : pointData)
						{
							// ���W����|���S�����쐬
							polygon.outer().push_back(PointType(point.x, point.y));
						}
						multiPolygon.push_back(polygon);
					}

					// �}���`�|���S���̃o�E���f�B���O�{�b�N�X���v�Z
					boost::geometry::model::box<PointType> boundingBox;
					boost::geometry::envelope(multiPolygon, boundingBox);

					// �ő�l�ƍŏ��l���擾
					PointType minPoint = boundingBox.min_corner();
					PointType maxPoint = boundingBox.max_corner();
					double minLat, maxLat;
					double minLon, maxLon;
					CGeoUtil::XYToLatLon(JPZone, minPoint.y(), minPoint.x(), minLat, minLon);
					CGeoUtil::XYToLatLon(JPZone, maxPoint.y(), maxPoint.x(), maxLat, maxLon);
					std::string lowerCorner = std::to_string(minLat) + " " + std::to_string(minLon) + " 0.0"; // z���W�ɂ�0.0
					std::string upperCorner = std::to_string(maxLat) + " " + std::to_string(maxLon) + " 0.0"; // z���W�ɂ�0.0

					// �ő�l�ƍŏ��l��GML�t�@�C���ɒǉ�

					pLowerCorner->put_text(_bstr_t(lowerCorner.c_str()));
					pUpperCorner->put_text(_bstr_t(upperCorner.c_str()));
					indentCount = 2;
					appendChild(pXMLDoc, pEnvelope, { pLowerCorner, pUpperCorner }, indentCount);
					appendChild(pXMLDoc, pBoundedBy, { pEnvelope }, indentCount);
					appendChild(pXMLDoc, peCityModel, { pBoundedBy }, indentCount);

					int currentId = 0;
					for (auto polygon : polygonData)
					{
						CString polygonMeshCode = polygon.meshCode;
						std::vector<Point> pointData = polygon.vertices;
						std::string posList = "";
						currentId = polygon.id;
						bool isFirst = true;
						// �������b�V���R�[�h�����m�F
						if (meshCode != polygonMeshCode)
						{
							// �قȂ�ꍇ�X�L�b�v
							continue;
						}

						for (auto point : pointData)
						{
							// ���W�ϊ�
							double dLat;
							double dLon;
							CGeoUtil::XYToLatLon(JPZone, point.y, point.x, dLat, dLon);

							// ���񏈗�
							if (isFirst)
							{
								posList = std::to_string(dLat) + " " + std::to_string(dLon) + " 0.0";
								isFirst = false;
							}
							else
							{
								posList = posList + " " + std::to_string(dLat) + " " + std::to_string(dLon) + " 0.0";
							}
						}

						// �e�v�f���쐬             
						MSXML2::IXMLDOMElementPtr pCityObjectMember;
						pXMLDoc->createElement(_bstr_t("core:cityObjectMember"), &pCityObjectMember);     //<core:cityObjectMember>���쐬

						// ���H�v�f���쐬             
						MSXML2::IXMLDOMElementPtr pRoad;
						pXMLDoc->createElement(_bstr_t("tran:Road"), &pRoad);

						// �����v�f���쐬
						std::map<std::string, MSXML2::IXMLDOMElementPtr> pAttributeDataMap;
						std::set<std::string> attrClassNames;
						std::vector<std::string> attrNames;
						for (auto data : attrDataVec[currentId].attrDataVec)
						{
							auto it = attrClassNames.find(data.className);
							// tran:Road�͂��łɍ쐬�ς݂̂��߁Atran:Road�ȊO����attrClassNames�ɒǉ�����Ă��Ȃ��ꍇ
							if ((it == attrClassNames.end()) && (data.className != "tran:Road"))
							{
								attrClassNames.insert(data.className);
								MSXML2::IXMLDOMElementPtr pAttributeClass;

								pXMLDoc->createElement(_bstr_t(data.className.c_str()), &pAttributeClass);
								pAttributeDataMap[data.className] = pAttributeClass;

								MSXML2::IXMLDOMElementPtr pAttributeName;
								pXMLDoc->createElement(_bstr_t(data.attrName.c_str()), &pAttributeName);

								// attrValue��int, double, string�Ŏ擾�̎d����ς���
								if (std::holds_alternative<int>(data.attrValue))
								{
									auto v = std::get<int>(data.attrValue);
									pAttributeName->put_text(_bstr_t(v));
								}
								else if (std::holds_alternative<double>(data.attrValue))
								{
									auto v = std::get<double>(data.attrValue);
									pAttributeName->put_text(_bstr_t(v));
								}
								else if (std::holds_alternative<std::string>(data.attrValue))
								{
									auto v = std::get<std::string>(data.attrValue);
									pAttributeName->put_text(_bstr_t(v.c_str()));
								}

								int indent = 3;
								if (data.className == "uro:TransportationDataQualityAttribute" ||
									data.className == "uro:RoadStructureAttribute" ||
									data.className == "uro:TrafficVolumeAttribute" ||
									data.className == "uro:DmAttribute"
									)
								{
									indent = 4;
								}
								appendChild(pXMLDoc, pAttributeDataMap[data.className], { pAttributeName }, indent, true);
							}
							else
							{
								MSXML2::IXMLDOMElementPtr pAttributeName;
								pXMLDoc->createElement(_bstr_t(data.attrName.c_str()), &pAttributeName);
								if (std::holds_alternative<int>(data.attrValue))
								{
									auto v = std::get<int>(data.attrValue);
									pAttributeName->put_text(_bstr_t(v));
								}
								else if (std::holds_alternative<double>(data.attrValue))
								{
									auto v = std::get<double>(data.attrValue);
									pAttributeName->put_text(_bstr_t(v));
								}
								else if (std::holds_alternative<std::string>(data.attrValue))
								{
									auto v = std::get<std::string>(data.attrValue);
									pAttributeName->put_text(_bstr_t(v.c_str()));
								}

								int indent = 2;
								if (data.className != "tran:Road")
								{
									indent = 4;
									appendChild(pXMLDoc, pAttributeDataMap[data.className], { pAttributeName }, indent, true);
								}
								else
								{
									appendChild(pXMLDoc, pRoad, { pAttributeName }, indent, true);
								}
							}
							attrNames.push_back(data.attrName);
						}
						// ���H�̍��W�|���S�����쐬              
						MSXML2::IXMLDOMElementPtr pLod1MultiSurface;
						pXMLDoc->createElement(_bstr_t("tran:lod1MultiSurface"), &pLod1MultiSurface);	//<tran:lod1MultiSurface>���쐬

						MSXML2::IXMLDOMElementPtr pMultiSurface;
						pXMLDoc->createElement(_bstr_t("gml:MultiSurface"), &pMultiSurface);			//<gml:MultiSurface>���쐬

						MSXML2::IXMLDOMElementPtr pSurfaceMember;
						pXMLDoc->createElement(_bstr_t("gml:surfaceMember"), &pSurfaceMember);			//<gml:surfaceMember>���쐬

						MSXML2::IXMLDOMElementPtr pPolygon;
						pXMLDoc->createElement(_bstr_t("gml:Polygon"), &pPolygon);						//<gml:Polygon>���쐬

						MSXML2::IXMLDOMElementPtr pExterior;
						pXMLDoc->createElement(_bstr_t("gml:exterior"), &pExterior);					//<gml:exterior>���쐬

						MSXML2::IXMLDOMElementPtr pLinearRing;
						pXMLDoc->createElement(_bstr_t("gml:LinearRing"), &pLinearRing);				//<gml:LinearRing>���쐬

						// ���W�|���S���̒l��ݒ�           
						MSXML2::IXMLDOMElementPtr pPosList;
						pXMLDoc->createElement(_bstr_t("gml:posList"), &pPosList);						//<gml:posList>���쐬

						// ���ꂼ��e�v�f�ɒǉ�
						pPosList->put_text(_bstr_t(posList.c_str()));
						indentCount = 8;
						appendChild(pXMLDoc, pLinearRing, { pPosList }, indentCount);
						appendChild(pXMLDoc, pExterior, { pLinearRing }, indentCount);
						appendChild(pXMLDoc, pPolygon, { pExterior }, indentCount);
						appendChild(pXMLDoc, pSurfaceMember, { pPolygon }, indentCount);
						appendChild(pXMLDoc, pMultiSurface, { pSurfaceMember }, indentCount);
						appendChild(pXMLDoc, pLod1MultiSurface, { pMultiSurface }, indentCount);
						appendChild(pXMLDoc, pRoad, { pLod1MultiSurface }, indentCount, true);
						for (const auto& p : pAttributeDataMap)
						{
							MSXML2::IXMLDOMElementPtr pParentAttributeClass;
							attrIndentCount = 3;

							if (p.first == "uro:TransportationDataQualityAttribute")
							{
								pXMLDoc->createElement(_bstr_t("uro:tranDataQualityAttribute"), &pParentAttributeClass);

								AddNewlineAndIndent(pXMLDoc, p.second, 3);
								appendChild(pXMLDoc, pParentAttributeClass, { p.second }, attrIndentCount);
								appendChild(pXMLDoc, pRoad, { pParentAttributeClass }, attrIndentCount);
							}
							else if (p.first == "uro:RoadStructureAttribute")
							{
								pXMLDoc->createElement(_bstr_t("uro:roadStructureAttribute"), &pParentAttributeClass);

								AddNewlineAndIndent(pXMLDoc, p.second, 3);
								appendChild(pXMLDoc, pParentAttributeClass, { p.second }, attrIndentCount);
								appendChild(pXMLDoc, pRoad, { pParentAttributeClass }, attrIndentCount, true);
							}
							else if (p.first == "uro:TrafficVolumeAttribute")
							{
								pXMLDoc->createElement(_bstr_t("uro:trafficVolumeAttribute"), &pParentAttributeClass);

								AddNewlineAndIndent(pXMLDoc, p.second, 3);
								appendChild(pXMLDoc, pParentAttributeClass, { p.second }, attrIndentCount);
								appendChild(pXMLDoc, pRoad, { pParentAttributeClass }, attrIndentCount, true);
							}
							else if (p.first == "uro:DmAttribute")
							{
								pXMLDoc->createElement(_bstr_t("uro:tranDmAttribute"), &pParentAttributeClass);

								AddNewlineAndIndent(pXMLDoc, p.second, 3);
								appendChild(pXMLDoc, pParentAttributeClass, { p.second }, attrIndentCount);
								appendChild(pXMLDoc, pRoad, { pParentAttributeClass }, attrIndentCount, true);
							}
							else
							{
								attrIndentCount = 2;
								appendChild(pXMLDoc, pRoad, { p.second }, attrIndentCount);
							}
						}
						appendChild(pXMLDoc, pCityObjectMember, { pRoad }, indentCount);
						appendChild(pXMLDoc, peCityModel, { pCityObjectMember }, indentCount, true);
					}
					AddNewline(pXMLDoc, peCityModel);
					pXMLDoc->appendChild(peCityModel, NULL);

					// CityGML�t�@�C����ۑ�
					auto fileName = outputFilePath.c_str() + bstr_t("/") + meshCode.Left(8) + L"_tran_" + CRS + ".gml";
					BSTR cityGMLFileName = SysAllocString(fileName);
					if (meshCode.GetLength() == 9)
					{
						if (_wremove(cityGMLFileName) == 0)
						{
							std::wcout << "1GB�𒴂���t�@�C�����폜���܂��� : " << cityGMLFileName << std::endl;
						}
						cityGMLFileName = SysAllocString((outputFilePath.c_str()) + bstr_t("/") + meshCode.Left(8) + L"_tran_" + CRS + "_" + meshCode.Mid(8, meshCode.GetLength() - 8) + ".gml");
					}
					if (meshCode.GetLength() >= 10)
					{
						cityGMLFileName = SysAllocString((outputFilePath.c_str()) + bstr_t("/") + meshCode.Left(8) + L"_tran_" + CRS + "_" + meshCode.Mid(8, meshCode.GetLength() - 9) + ".gml");
						if (_wremove(cityGMLFileName) == 0)
						{
							std::wcout << "1GB�𒴂���t�@�C�����폜���܂��� : " << cityGMLFileName << std::endl;
						}
						cityGMLFileName = SysAllocString((outputFilePath.c_str()) + bstr_t("/") + meshCode.Left(8) + L"_tran_" + CRS + "_" + meshCode.Mid(8, meshCode.GetLength() - 8) + ".gml");
					}

					if (SUCCEEDED(pXMLDoc->save(_variant_t(cityGMLFileName))))
					{
						std::cout << "CityGML�t�@�C������������܂����B" << std::endl;
						std::wcout << cityGMLFileName << std::endl;
					}
					else
					{
						std::cout << "CityGML�t�@�C���̐����Ɏ��s���܂����B" << std::endl;
						std::wcout << fileName << std::endl;
					}

					// �t�@�C���T�C�Y�m�F
					WIN32_FILE_ATTRIBUTE_DATA fileInfo;
					if (GetFileAttributesEx(cityGMLFileName, GetFileExInfoStandard, &fileInfo))
					{
						DWORD fileSize = fileInfo.nFileSizeLow;
						std::cout << "fileSize = " << fileSize << "(B)" << std::endl;

						// 1GB�𒴂��Ă��邩�ǂ���
						if (static_cast<double>(fileSize) > (1024L * 1024L * 1024L))
						{
							std::cout << "Over Size " << std::endl;
							if (targetMeshCodes.find(meshCode) == targetMeshCodes.end())
							{
								targetMeshCodes.insert(meshCode);
							}
						}
					}
				}
				else
				{
					std::wcerr << SysAllocString(L"MSXML�̏������Ɏ��s���܂����B") << std::endl;
				}
			}
			catch (_com_error& e)
			{
				std::wcerr << e.ErrorMessage() << std::endl;
			}
		}
		else
		{
			std::wcerr << SysAllocString(L"COM�̏������Ɏ��s���܂����B") << std::endl;
		}

		//COM�̉��
		CoUninitialize();

	}

	if (targetMeshCodes.size() == 0)
	{
		return true;
	}

	return false;

}
