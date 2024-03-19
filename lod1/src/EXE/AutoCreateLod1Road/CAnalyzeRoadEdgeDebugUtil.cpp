#include "pch.h"
#include "CAnalyzeRoadEdgeDebugUtil.h"
#include "shapefil.h"
#include "boost/format.hpp"

/*!
 * @brief 点のshape file出力
 * @param[in] points        点群
 * @param[in] strShpPath    shpファイルパス
 * @return  処理結果
 * @retval  true    成功
 * @retval  false   失敗
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
 * @brief ポリラインのshape file 出力
 * @param[in] polylines         ポリライン集合
 * @param[in] strShpPath        shpファイルパス
 * @return  処理結果
 * @retval  true    成功
 * @retval  false   失敗
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
 * @brief ポリゴンのshape file出力
 * @param[in] polygons          ポリゴン群
 * @param[in] strShpPath        出力 shape file パス
 * @param[in] bHole             穴の書き出しフラグ
 * @return  処理結果
 * @retval  true        成功
 * @retval  false       失敗
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
 * @brief 注目範囲の可視化
 * @param[in] strShpPath    出力shapeパス
 * @param[in] dInputMinX    入力範囲の最小x座標
 * @param[in] dInputMinY    入力範囲の最小y座標
 * @param[in] nRow          処理範囲の行数
 * @param[in] nColumn       処理範囲の列数
 * @param[in] dProcWidth    処理範囲幅
 * @param[in] dProcHeight   処理範囲高さ
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
    // 属性定義
    std::vector<CShapeAttribute::AttributeFieldData> vecAttrFields;
    CShapeAttribute::AttributeFieldData field;
    field.fieldType = CShapeAttribute::AttributeFieldType::ATTR_FIELD_TYPE_STRING;
    field.strName = "name";
    field.nWidth = 8;
    vecAttrFields.push_back(field);


    // 注目範囲と属性情報作成
    BoostMultiLines procAreas;
    std::vector<CShapeAttribute::AttributeDataRecord> vecRecords;
    for (int nY = 0; nY < nRow; nY++)
    {
        // 注目範囲(縦方向）
        double dMinY = dProcHeight * static_cast<double>(nY) + dInputMinY;
        double dMaxY = dMinY + dProcHeight;
        for (int nX = 0; nX < nColumn; nX++)
        {
            // 注目範囲(横方向）
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
