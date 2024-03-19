#pragma once
#include <string>
#include <algorithm>
#include <iterator>
#include "../../LIB/CommonUtil/CGeoUtil.h"
#include "DMType.h"
#include "shapefil.h"


class CDMRoadDataManager
{
public:
    /*!
     * @brief 道路施設のデータ形状タイプ
    */
    enum class RoadFacilitiesDataType
    {
        ROAD_FACILITIES_UNKNOWN = 0,    //!< 未設定
        ROAD_FACILITIES_POINT_DATA,     //!< 点データ
        ROAD_FACILITIES_LINE_DATA,      //!< 線データ
        ROAD_FACILITIES_POLYGON_DATA,   //!< 面データ

    };

    /*!
     * @brief 道路縁データ
    */
    struct RoadEdgeData
    {
        //int                         nId;            //!< id
        DMCode                      nDMCode;        //!< DMコード(大分類)
        DMRoadCode                  nRoadCode;      //!< DMコード(小分類)
        //DMRecordType                nRecordType;    //!< レコードタイプ
        //DMNumericClassificationType nNumClassType;  //!< 精度区分(数値化区分)
        //DMMapInfoLevelType          nMapInfoLevel;  //!< 精度区分(地図情報レベル)
        //int                         nInterrupt;     //!< 間断区分 0:間断しない 1-9:間断する(数値は優先順位)
        std::vector<CVector3D>      vecPolyline;    //!< ポリライン頂点列

        /*!
         * コンストラクタ
         */
        RoadEdgeData()
        {
            //nId = -1;
            nDMCode = DMCode::UNCATEGORIZED;
            nRoadCode = DMRoadCode::UNCATEGORIZED_ROAD;
            //nRecordType = DMRecordType::DM_RECORD_PLANE;
            //nNumClassType = DMNumericClassificationType::OTHER_NUMERIC_CLASSIFICATION;
            //nMapInfoLevel = DMMapInfoLevelType::OTHER_LEVEL;
            //nInterrupt = 0;
        }

        /*!
         * デストラクタ
         */
        virtual ~RoadEdgeData() {}

        /*!
         * コピーコンストラクタ
         */
        RoadEdgeData(const RoadEdgeData& x) { *this = x; }

        /*!
         * 代入演算子
         */
        RoadEdgeData& operator=(const RoadEdgeData& x)
        {
            if (this != &x)
            {
                //nId = x.nId;
                nDMCode = x.nDMCode;
                nRoadCode = x.nRoadCode;
                //nRecordType = x.nRecordType;
                //nNumClassType = x.nNumClassType;
                //nMapInfoLevel = x.nMapInfoLevel;
                std::copy(x.vecPolyline.begin(), x.vecPolyline.end(), std::back_inserter(vecPolyline));
            }
            return *this;
        }
    };

    /*!
     * @brief 道路施設データ
    */
    struct RoadFacilitiesData
    {
        //int nId;    //!< id
        std::vector<CVector3D>  vecPolyline;                //!< ポリライン頂点列
        DMCode                  nDMCode;                    //!< DMコード(大分類)
        DMRoadFacilitiesCode    nRoadFacilitiesCode;        //!< DMコード(小分類)
        DMGeometryType          nGeometryType;              //!< 図形区分
        RoadFacilitiesDataType  nRoadFacilitiesDataType;    //!< 道路施設の形状データタイプ

        /*!
         * コンストラクタ
         */
        RoadFacilitiesData()
        {
            //nId = -1;
            nDMCode = DMCode::UNCATEGORIZED;
            nRoadFacilitiesCode = DMRoadFacilitiesCode::UNCATEGORIZED_ROAD_FACILITIES;
            nGeometryType = DMGeometryType::UNCLASSIFIED;
            nRoadFacilitiesDataType = RoadFacilitiesDataType::ROAD_FACILITIES_UNKNOWN;
        }

        /*!
         * デストラクタ
         */
        virtual ~RoadFacilitiesData() {}

        /*!
         * コピーコンストラクタ
         */
        RoadFacilitiesData(const RoadFacilitiesData& x) { *this = x; }

        /*!
         * 代入演算子
         */
        RoadFacilitiesData& operator=(const RoadFacilitiesData& x)
        {
            if (this != &x)
            {
                //nId = x.nId;
                nDMCode = x.nDMCode;
                nRoadFacilitiesCode = x.nRoadFacilitiesCode;
                nGeometryType = x.nGeometryType;
                nRoadFacilitiesDataType = x.nRoadFacilitiesDataType;
                std::copy(x.vecPolyline.begin(), x.vecPolyline.end(), std::back_inserter(vecPolyline));
            }
            return *this;
        }
    };

    CDMRoadDataManager(void) {}     //!< コンストラクタ
    ~CDMRoadDataManager(void) {}    //!< デストラクタ
    bool ReadRoadEdgeShapeFile(void);   // 道路縁shape読み込み
    void ReadRoadFacilitiesShapeFile(void);   // 道路施設shape読み込み

    /*!
     * @brief  道路縁データのゲッター
     * @return 道路縁データ
    */
    std::vector<RoadEdgeData> GetRoadEdges() { return m_vecRoadEdgeData; }

    /*!
     * @brief  道路施設データのゲッター
     * @return 道路施設データ
    */
    std::vector<RoadFacilitiesData> GetRoadFacilities() { return m_vecRoadFacilitiesData; }

protected:

private:
    std::vector<RoadEdgeData>       m_vecRoadEdgeData;              //!< 道路縁データ配列
    std::vector<RoadFacilitiesData> m_vecRoadFacilitiesData;        //!< 道路施設データ配列

    void separateDMCode(int nCode, int& nUpper, int& nLower);       //!< DMコード分割
    int combineDMCode(int nUpper, int nLower);                      //!< DMコード結合
    void separateAccuracyCode(int nCode, int& nUpper, int& nLower); //!< 精度区分コード分割
    int separateParts(SHPObject* psElem, std::vector<std::vector<CVector3D>>& vecParts);  //!< パート分割

    //!< 道路施設shapeファイル読み込み(内部関数)
    bool readRoadFacilitiesShapeFile(const std::string &strShpPath, RoadFacilitiesDataType type);
};
