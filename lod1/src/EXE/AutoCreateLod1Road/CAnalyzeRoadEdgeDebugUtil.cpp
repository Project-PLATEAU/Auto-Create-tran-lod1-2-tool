#include "pch.h"
#include "CAnalyzeRoadEdgeDebugUtil.h"
#include "shapefil.h"
#include "boost/format.hpp"

/*!
 * @brief �_��shape file�o��
 * @param[in] points        �_�Q
 * @param[in] strShpPath    shp�t�@�C���p�X
 * @return  ��������
 * @retval  true    ����
 * @retval  false   ���s
*/
bool CAnalyzeRoadEdgeDebugUtil::OutputMultiPointsToShp(
    const BoostMultiPoints &points,
    std::string strShpPath)
{
    std::vector<CShapeAttribute::AttributeFieldData> vecFields;
    std::vector<CShapeAttribute::AttributeDataRecord> vecAttrRecords;

    CShapeAttribute::AttributeFieldData field1, field2, field3;
    field1.fieldType = CShapeAttribute::AttributeFieldType::ATTR_FIELD_TYPE_INT;
    field1.strName = "id";
    field1.nWidth = 8;
    vecFields.push_back(field1);
    field2.fieldType = CShapeAttribute::AttributeFieldType::ATTR_FIELD_TYPE_DOUBLE;
    field2.strName = "x";
    field2.nWidth = 10;
    field2.nDecimals = 3;
    vecFields.push_back(field2);
    field3.fieldType = CShapeAttribute::AttributeFieldType::ATTR_FIELD_TYPE_DOUBLE;
    field3.strName = "y";
    field3.nWidth = 10;
    field3.nDecimals = 3;
    vecFields.push_back(field3);
    int nId = 0;
    BOOST_FOREACH(BoostPoint pt, points)
    {
        CShapeAttribute::AttributeDataRecord record;
        record.nShapeId = nId;
        record.vecAttribute.push_back(CShapeAttribute::AttributeData(nId));
        record.vecAttribute.push_back(CShapeAttribute::AttributeData(pt.x()));
        record.vecAttribute.push_back(CShapeAttribute::AttributeData(pt.y()));
        vecAttrRecords.push_back(record);
        nId++;
    }

    CShapeWriter writer;
    return writer.OutputMultiPoints(points, strShpPath, vecFields, vecAttrRecords);
}

/*!
 * @brief �|�����C����shape file �o��
 * @param[in] polylines         �|�����C���W��
 * @param[in] strShpPath        shp�t�@�C���p�X
 * @return  ��������
 * @retval  true    ����
 * @retval  false   ���s
 */
bool CAnalyzeRoadEdgeDebugUtil::OutputPolylinesToShp(
    const BoostMultiLines &polylines,
    std::string strShpPath)
{
    std::vector<CShapeAttribute::AttributeFieldData> vecFields;
    std::vector<CShapeAttribute::AttributeDataRecord> vecAttrRecords;

    CShapeAttribute::AttributeFieldData field1, field2;
    field1.fieldType = CShapeAttribute::AttributeFieldType::ATTR_FIELD_TYPE_INT;
    field1.strName = "id";
    field1.nWidth = 8;
    vecFields.push_back(field1);
    field2.fieldType = CShapeAttribute::AttributeFieldType::ATTR_FIELD_TYPE_DOUBLE;
    field2.strName = "length";
    field2.nWidth = 6;
    field2.nDecimals = 3;
    vecFields.push_back(field2);

    int nId = 0;
    BOOST_FOREACH (BoostPolyline line, polylines)
    {
        CShapeAttribute::AttributeDataRecord record;
        record.nShapeId = nId;
        record.vecAttribute.push_back(CShapeAttribute::AttributeData(nId));
        record.vecAttribute.push_back(CShapeAttribute::AttributeData(bg::length(line)));
        vecAttrRecords.push_back(record);
        nId++;
    }

    CShapeWriter writer;
    return writer.OutputPolylines(polylines, strShpPath, vecFields, vecAttrRecords);
}

/*!
 * @brief �|���S����shape file�o��
 * @param[in] polygons          �|���S���Q
 * @param[in] strShpPath        �o�� shape file �p�X
 * @param[in] bHole             ���̏����o���t���O
 * @return  ��������
 * @retval  true        ����
 * @retval  false       ���s
*/
bool CAnalyzeRoadEdgeDebugUtil::OutputPolygonsToShp(
    const BoostMultiPolygon &polygons,
    std::string strShpPath,
    const bool bHole)
{
    std::vector<CShapeAttribute::AttributeFieldData> vecFields;
    std::vector<CShapeAttribute::AttributeDataRecord> vecAttrRecords;

    CShapeAttribute::AttributeFieldData field1, field2;
    field1.fieldType = CShapeAttribute::AttributeFieldType::ATTR_FIELD_TYPE_INT;
    field1.strName = "id";
    field1.nWidth = 8;
    vecFields.push_back(field1);
    field2.fieldType = CShapeAttribute::AttributeFieldType::ATTR_FIELD_TYPE_DOUBLE;
    field2.strName = "area";
    field2.nWidth = 7;
    field2.nDecimals = 3;
    vecFields.push_back(field2);

    int nId = 0;
    BOOST_FOREACH(BoostPolygon polygon, polygons)
    {
        CShapeAttribute::AttributeDataRecord record;
        record.nShapeId = nId;
        record.vecAttribute.push_back(CShapeAttribute::AttributeData(nId));
        record.vecAttribute.push_back(CShapeAttribute::AttributeData(bg::area(polygon)));
        vecAttrRecords.push_back(record);
        nId++;
    }

    CShapeWriter writer;
    return writer.OutputPolygons(polygons, strShpPath, vecFields, vecAttrRecords, bHole);
}

/*!
 * @brief ���ڔ͈͂̉���
 * @param[in] strShpPath    �o��shape�p�X
 * @param[in] dInputMinX    ���͔͈͂̍ŏ�x���W
 * @param[in] dInputMinY    ���͔͈͂̍ŏ�y���W
 * @param[in] nRow          �����͈͂̍s��
 * @param[in] nColumn       �����͈̗͂�
 * @param[in] dProcWidth    �����͈͕�
 * @param[in] dProcHeight   �����͈͍���
 */
void CAnalyzeRoadEdgeDebugUtil::OutputProcArea(
    std::string strShpPath,
    const double dInputMinX,
    const double dInputMinY,
    const int nRow,
    const int nColumn,
    const double dProcWidth,
    const double dProcHeight)
{
    // ������`
    std::vector<CShapeAttribute::AttributeFieldData> vecAttrFields;
    CShapeAttribute::AttributeFieldData field;
    field.fieldType = CShapeAttribute::AttributeFieldType::ATTR_FIELD_TYPE_STRING;
    field.strName = "name";
    field.nWidth = 8;
    vecAttrFields.push_back(field);


    // ���ڔ͈͂Ƒ������쐬
    BoostMultiLines procAreas;
    std::vector<CShapeAttribute::AttributeDataRecord> vecRecords;
    for (int nY = 0; nY < nRow; nY++)
    {
        // ���ڔ͈�(�c�����j
        double dMinY = dProcHeight * static_cast<double>(nY) + dInputMinY;
        double dMaxY = dMinY + dProcHeight;
        for (int nX = 0; nX < nColumn; nX++)
        {
            // ���ڔ͈�(�������j
            double dMinX = dProcWidth * static_cast<double>(nX) + dInputMinX;
            double dMaxX = dMinX + dProcWidth;
            BoostPolyline polyline;
            polyline.push_back(BoostPoint(dMinX, dMinY));
            polyline.push_back(BoostPoint(dMinX, dMaxY));
            polyline.push_back(BoostPoint(dMaxX, dMaxY));
            polyline.push_back(BoostPoint(dMaxX, dMinY));
            polyline.push_back(BoostPoint(dMinX, dMinY));
            procAreas.push_back(polyline);

            CShapeAttribute::AttributeDataRecord record;
            record.nShapeId = nColumn * nY + nX;
            std::string strName = (boost::format("%03d_%03d") % nX % nY).str();
            record.vecAttribute.push_back(CShapeAttribute::AttributeData(strName));
            vecRecords.push_back(record);
        }
    }
    CShapeWriter writer;
    writer.OutputPolylines(procAreas, strShpPath, vecAttrFields, vecRecords);
}
