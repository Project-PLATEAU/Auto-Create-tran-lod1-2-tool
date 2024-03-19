#pragma once

/*!
 * @brief DMデータ取得分類コード(大項目)
*/
enum class DMCode
{
    UNCATEGORIZED = 0,                              //!< 未分類
    // 行政界
    ADMINISTRATIVE_CIRCLES_UNCATEGORIZED = 10,      //!< 未分類(行政界)
    ADMINISTRATIVE_CIRCLES_BOUNDARY,                //!< 境界・所属界
    // 交通施設
    TRANSPORTATION_FACILITIES_UNCATEGORIZED = 20,   //!< 未分類(交通施設)
    TRANSPORTATION_FACILITIES_ROAD,                 //!< 道路
    TRANSPORTATION_FACILITIES_ROAD_FACIRITIES,      //!< 道路施設
    TRANSPORTATION_FACILITIES_RAILWAY,              //!< 鉄道
    TRANSPORTATION_FACILITIES_RAILWAY_FACIRITIES,   //!< 鉄道施設
    TRANSPORTATION_FACILITIES_LINEAR_DIAGRAM,       //!< 線形図,杭打ち図
    // 建物
    BUILDING = 30,                                  //!< 建物
    BUILDING_ACCESSORIES = 34,                      //!< 建物の付属物
    BUILDING_SYMBOL,                                //!< 建物記号
    // 小物体
    SMALL_OBJECT_UNCATEGORIZED = 40,                //!< 未分類(商物体)
    SMALL_OBJECT_PUBLIC_FACILITIES,                 //!< 公共施設
    SMALL_OBJECT_OTHER,                             //!< その他の小物体
    // 水辺等
    WATERSIDE_UNCATEGORIZED = 50,                   //!< 未分類(水辺等)
    WATERSIDE_WATERSHED,                            //!< 水涯線
    WATERSIDE_STRUCTURES,                           //!< 水部に関する構造物
    // 土地利用等
    LAND_USE_UNCATEGORIZED = 60,                    //!< 未分類(土地利用等)
    LAND_USE_SLOPES,                                //!< 法面・構囲
    LAND_USE_VARIOUS_LANDS,                         //!< 諸地・場地
    LAND_USE_VEGETATION,                            //!< 植生
    LAND_USE_SITE = 65,                             //!< 用地
    // 地形
    TERRAIN_UNCATEGORIZED = 70,                     //!< 未分類(地形)
    TERRAIN_CONTOUR,                                //!< 等高線
    TERRAIN_DEFORMED_LAND,                          //!< 変形地
    TERRAIN_REFERENCE_POINT,                        //!< 基準点
    TERRAIN_NUMERICAL_TERRAIN_MODEL = 75,           //!< 数値地形モデル
    TERRAIN_REFERENCE_GRID_MAP,                     //!< 基準点網図
    TERRAIN_LEVEL_POINT_NETWORKDIAGRAM,             //!< 水準点網図
    TERRAIN_AERIAL_PHOTOGRAPHIC_DATA,               //!< 空中写真資料
    TERRAIN_APPLIED_SURVEYING_AND_DECORATION,       //!< 応用測量整飾
    // 注記
    NOTE_UNCATEGORIZED = 80,                        //!< 未分類(注記)
    NOTE_EXPLANATORY_NOTE,                          //!< 注記
    NOTE_SURVEY_RECORDS,                            //!< 測量記録等
};

/*!
 * @brief DMデータの取得分類コード(道路:21xx)
*/
enum class DMRoadCode
{
    UNCATEGORIZED_ROAD = 0, //!< 未分類
    ROAD_EDGE,              //!< 道路縁(街区線)
    LIGHT_ROADWAY,          //!< 軽車道
    FOOTPATH,               //!< 徒歩道
    GARDEN_PATH,            //!< 庭園路等
    TUNNEL,                 //!< トンネル内の道路
    CONSTRUCTION,           //!< 建設中の道路
};

enum class DMRoadFacilitiesCode
{
    UNCATEGORIZED_ROAD_FACILITIES = 0,      //!< 未分類
    ROAD_BRIDGE = 3,                        //!< 道路橋(高架部)
    WOODEN_BRIDGE,                          //!< 木橋
    FOOT_BRIDGE,                            //!< 徒橋
    PIER_BRIDGES,                           //!< 桟道橋
    PEDESTRIAN_CROSSING_BRIDGES = 11,       //!< 横断歩道
    UNDERGROUND_PEDESTRIAN_CROSSING,        //!< 地下横断歩道
    WALKWAY,                                //!< 歩道
    STONE_STEPS,                            //!< 石段
    ENTRANCES_TO_UNDERGROUND,               //!< 地下街・地下鉄等出入口
    ROAD_TUNNELS = 19,                      //!< 道路のトンネル
    BUS_STOPS = 21,                         //!< バス停
    SAFETY_ZONES,                           //!< 安全地帯
    SEPARATION_ZONES = 26,                  //!< 分離帯
    STOPS,                                  //!< 駒止
    ROAD_SNOW_COVER,                        //!< 道路の雪覆い等
    U_SHAPED_DITCH_WITHOUT_COVER = 31,      //!< 側溝 U字溝無蓋
    U_SHAPED_DITCH_WITH_COVER,              //!< 側溝 U字溝有蓋
    L_SHAPED_DITCH,                         //!< 側溝 L字溝
    UNDERGROUND_GUTTER,                     //!< 側溝地下部
    STORMWATER_INLET,                       //!< 雨水桝
    ROW_OF_TREES_BOX,                       //!< 並木桝
    ROW_OF_TREES = 38,                      //!< 並木
    TREE_PLANTING,                          //!< 植樹
    ROAD_INFORMATION_BOARD = 41,            //!< 道路情報版
    ROAD_SIGNS,                             //!< 道路標識 案内
    ROAD_WARNING_SIGNS,                     //!< 道路標識 警戒
    ROAD_CONTROL_SIGNS,                     //!< 道路標識 規制
    SIGNAL_LIGHTS = 46,                     //!< 信号灯
    SIGNAL_LIGHTS_WITHOUT_DEDICATED_POLE,   //!< 信号灯 専用ポールなし
    TRAFFIC_VOLUME_MONITORING_STATION = 51, //!< 交通量観測所
    SNOW_POLES,                             //!< スノーポール
    CURVE_MIRROR,                           //!< カーブミラー
    DISTANCE_MARKER_KM = 55,                //!< 距離標(km)
    DISTANCE_MARKER_M,                      //!< 距離標(m)
    TELEPHONE_BOX = 61,                     //!< 電話ボックス
    MAILBOX,                                //!< 郵便ポスト
    FIRE_ALARM,                             //!< 火災報知器
};

/*!
 * @brief DMデータのレコードタイプ
*/
enum class DMRecordType
{
    DM_RECORD_PLANE = 1,    //!< 面
    DM_RECORD_LINE,         //!< 線
    DM_RECORD_CIRCLE,       //!< 円
    DM_RECORD_ARC,          //!< 円弧
    DM_RECORD_POINT,        //!< 点
    DM_RECORD_DIRECTION,    //!< 方向
    DM_RECORD_NOTE,         //!< 注記
    DM_RECORD_ATTRIBUTE,    //!< 属性
};

/*!
 * @brief DMデータの精度区分(数値化区分, 上位桁)
*/
enum class DMNumericClassificationType
{
    REFERENCE_POINT_SURVEY = 1,                     //!< 基準面測量成果を用いる方法
    NUMERICAL_MEASUREMENT,                          //!< TS等を用いた数値実測
    NUMERICAL_GRAPHICAL_REPRESENTATION,             //!< 数値図化法・他の数値地形図データの利用
    EXISTING_DRAWINGS_QUANTIFICATION_NON_STRECH,    //!< 既成図数値化(無伸縮図面)
    EXISTING_DRAWINGS_QUANTIFICATION_STRECH,        //!< 既成図数値化(伸縮図面)
    AERIAL_LASER_SURVEY,                            //!< 航空レーザ測量成果を用いる方法
    OTHER_NUMERIC_CLASSIFICATION = 9,               //!< その他
};

/*!
 * @brief DMデータの精度区分(地図情報レベル区分, 下位桁)
*/
enum class DMMapInfoLevelType
{
    INFO_LEVEL_50 = 1,  //!< 1 ~ 50
    INFO_LEVEL_100,     //!< ~ 100
    INFO_LEVEL_250,     //!< ~ 250
    INFO_LEVEL_500,     //!< ~ 500
    INFO_LEVEL_1000,    //!< ~ 1000
    INFO_LEVEL_2500,    //!< ~ 2500
    INFO_LEVEL_5000,    //!< ~ 5000
    INFO_LEVEL_10000,   //!< ~ 10000
    OTHER_LEVEL,        //!< その他
};

/*!
 * @brief DMデータの図形区分
*/
enum class DMGeometryType
{
    UNCLASSIFIED = 0,                       //!< 非区分
    TOP_OF_PROJECTION_AREA = 11,            //!< 射影部の上端
    BOTTOM_OF_PROJECTION_AREA,              //!< 射影部の下端
    RAILING = 21,                           //!< 高欄
    BRIDGE_PIER,                            //!< 橋脚
    MAIN_PILLAR,                            //!< 親柱
    GUARDRAIL = 26,                         //!< ガードレール
    GURAD_PIPE,                             //!< ガードパイプ
    COURTYARD_LINE = 31,                    //!< 中庭線
    RIDGELINE,                              //!< 棟割線
    LAYERED_LINE,                           //!< 階層線
    EXTERNAL_STAIRCASE,                     //!< 外付階段
    PORCHES,                                //!< ポーチ・ひさし
    FENCES_ON_BOTH_SIDES_OF_THE_SITE = 46,  //!< 両側敷地のへい
    TRANSPORT_PIPE,                         //!< 輸送管
    SURFACE = 51,                           //!< 表層面
    SEA_LEVEL,                              //!< 海水面
    STRAIGHT_LINE = 61,                     //!< 直線
    ARC,                                    //!< 円弧
    CLOTHOID,                               //!< クロソイド
    OTHER_CURVE,                            //!< その他の緩和曲線
    SOTONE_PILE = 71,                       //!< 石杭
    CONCRETE,                               //!< コンクリート
    PLASTIC_PILE,                           //!< 合成樹脂杭
    STAINLESS_STEEL_PILE,                   //!< 不銹鋼杭
    OTHER_BOUNDARY_LINE,                    //!< その他の境界線
    BOUNDARY_CALC_POINT,                    //!< 境界計算点
    SUPPLEMENTAL_REPRESENTATION_DATA = 99,  //!< 表現補助データ
};
