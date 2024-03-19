#pragma once

#include <utility>
#include <vector>
#include "boost/geometry.hpp"
#include "boost/graph/adjacency_list.hpp"
#include "boost/graph/depth_first_search.hpp"
#include "boost/polygon/voronoi.hpp"
#include "boost/foreach.hpp"

namespace bg = boost::geometry;
namespace bp = boost::polygon;

typedef bg::model::d2::point_xy<double> BoostPoint;                 //!< boostの2D座標型
typedef bg::model::linestring<BoostPoint> BoostPolyline;            //!< boostの2Dポリライン
typedef bg::model::multi_point<BoostPoint> BoostMultiPoints;        //!< 複数点型
typedef bg::model::multi_linestring<BoostPolyline> BoostMultiLines; //!< 複数ライン型
typedef bg::model::box<BoostPoint> BoostBox;                        //!< 矩形型
typedef bg::model::ring<BoostPoint, false, true> BoostRing;         //!< リング型
typedef bg::model::polygon<BoostPoint, false, true> BoostPolygon;   //!< boostの2Dポリゴン
typedef bg::model::multi_polygon<BoostPolygon> BoostMultiPolygon;   //!< boostの複数2Dポリゴン
// typedef bg::model::polygon<point, clock_wise, closed> polygon;
// clocke_wise = true(表:時計回り)
// clocke_wise = false(表:反時計回り)
// closed = true(終点に始点と同一な点を挿入してポリゴンを閉じる必要がある)
// closed = false(終点に始点と同一な点を挿入しなくても良い)

typedef std::pair<BoostPolyline, BoostPolyline> BoostPairLine; //!< ポリラインのペアデータ
typedef std::pair<BoostPoint, BoostPoint> BoostPairPoint;       //!< 頂点ペアデータ

// ボロノイ分割用
typedef bp::point_data<int> BoostVoronoiPoint;                  //!< ボロノイ分割用の座標型
typedef bp::voronoi_diagram<double> BoostVoronoiDiagram;        //!< ボロノイダイアグラム
typedef BoostVoronoiDiagram::const_edge_iterator BoostVoronoiEdgeIt;    //!< ボロノイダイアグラムのエッジイテレータ
typedef BoostVoronoiDiagram::const_cell_iterator BoostVoronoiCellIt;    //!< ボロノイダイアグラムのセルイテレータ

// グラフ用
struct BoostVertexProperty;
struct BoostEdgeProperty;

typedef boost::adjacency_list<
    boost::vecS, boost::vecS, boost::undirectedS,
    BoostVertexProperty,
    BoostEdgeProperty> BoostUndirectedGraph;  //!< 無向グラフ定義
typedef boost::graph_traits<BoostUndirectedGraph>::vertex_descriptor BoostVertexDesc;       //!< 頂点定義(無向グラフ)
typedef boost::graph_traits<BoostUndirectedGraph>::edge_descriptor BoostEdgeDesc;           //!< エッジ定義(無向グラフ)


// 有向グラフ
typedef boost::adjacency_list<
    boost::vecS, boost::vecS, boost::directedS,
    BoostVertexProperty,
    BoostEdgeProperty> BoostDirectedGraph;  //!< 有向グラフ定義
typedef boost::graph_traits<BoostDirectedGraph>::vertex_descriptor BoostDVertexDesc;       //!< 頂点定義(有向グラフ)
typedef boost::graph_traits<BoostDirectedGraph>::edge_descriptor BoostDEdgeDesc;           //!< エッジ定義(有向グラフ)

/*!
 * @brief グラフの頂点プロパティ定義
*/
struct BoostVertexProperty
{
    BoostVertexDesc desc;   //!< デスクリプター
    BoostPoint pt;          //!< 座標
    bool isSearched;        //!< 探索済みフラグ
    /*!
     * @brief コンストラクタ
    */
    BoostVertexProperty()
    {
        desc = 0;
        pt.x(0.0);
        pt.y(0.0);
        isSearched = false;
    }
    /*!
     * @brief コンストラクタ
    */
    BoostVertexProperty(const BoostPoint &p)
    {
        desc = 0;
        pt = p;
        isSearched = false;
    }

    /*!
     * @brief コピーコンストラクタ
    */
    BoostVertexProperty(const BoostVertexProperty &p) { *this = p; }

    /*!
     * @brief 代入演算子
    */
    BoostVertexProperty &operator =(const BoostVertexProperty &p)
    {
        if (&p != this)
        {
            desc = p.desc;
            pt = p.pt;
            isSearched = p.isSearched;
        }
        return *this;
    }
};

/*!
 * @brief グラフのエッジプロパティ定義
*/
struct BoostEdgeProperty
{
    BoostVertexDesc vertexDesc1;    //!< 頂点ディスクリプタ―1
    BoostVertexDesc vertexDesc2;    //!< 頂点ディスクリプタ―2
    double dLength;                 //!< エッジの長さ(要手動更新)
    /*!
     * @brief コンストラクタ
    */
    BoostEdgeProperty()
    {
        vertexDesc1 = 0;
        vertexDesc2 = 0;
        dLength = 0;
    }

    /*!
     * @brief コンストラクタ
     * @param[in] desc1     頂点デスクリプター
     * @param[in] desc2     頂点デスクリプター
     * @param[in] length    エッジの長さ
    */
    BoostEdgeProperty(BoostVertexDesc desc1, BoostVertexDesc desc2, double length = 0)
    {
        vertexDesc1 = desc1;
        vertexDesc2 = desc2;
        dLength = length;
    }

    /*!
     * @brief コピーコンストラクタ
    */
    BoostEdgeProperty(const BoostEdgeProperty &e) { *this = e; }

    /*!
     * @brief 代入演算子
    */
    BoostEdgeProperty &operator =(const BoostEdgeProperty &e)
    {
        if (&e != this)
        {
            vertexDesc1 = e.vertexDesc1;
            vertexDesc2 = e.vertexDesc2;
            dLength = e.dLength;
        }
        return *this;
    }
};

/*!
 * @brief 深さ優先探索時の動作
*/
class CBoostDFSVisitor : public boost::default_dfs_visitor
{
public:

    /*!
     * @brief 経路
    */
    std::vector<BoostVertexDesc> &vecRoute;

    /*!
     * @brief コンストラクタ
     * @param[in/out] vec 探索経路保存用
    */
    CBoostDFSVisitor(std::vector<BoostVertexDesc> &vec) : vecRoute(vec) {}

    /*!
     * @brief 頂点発見時の挙動
     * @tparam Vertex   頂点クラスのテンプレート
     * @tparam Graph    グラフクラスのテンプレート
     * @param[in] u     頂点
     * @param[in] g     グラフ
    */
    template <typename Vertex, typename Graph>
    void discover_vertex(Vertex u, const Graph &g)
    {
        vecRoute.push_back(u);
    }
};

/*!
 * @brief 交差点データクラス
 */
class CCrossingData
{
public:
    /*!
     * @brief コンストラクタ
     */
    CCrossingData()
    {
        m_pt = BoostPoint();
        m_nBranch = 0;
        m_voronoiCell = BoostPolygon();
        m_area = BoostPolygon();
    }

    /*!
     * @brief コンストラクタ
     */
    CCrossingData(BoostPoint pt, int nBranch)
    {
        m_pt = pt;
        m_nBranch = nBranch;
    }

    /*!
     * @brief デストラクタ
     */
    ~CCrossingData() {};

    /*!
     * @brief コピーコンストラクタ
     * @param[in] x コピー元データ
     */
    CCrossingData(const CCrossingData &x) { *this = x; }

    /*!
     * @brief 代入演算子
     */
    CCrossingData &operator = (const CCrossingData &x)
    {
        if (this != &x)
        {
            this->Point(x.m_pt);
            this->BranchNum(x.m_nBranch);
            this->Cell(x.m_voronoiCell);
            this->Area(x.m_area);
        }
        return *this;
    }

    /*!
     * @brief 交差点座標のゲッター
     * @return 交差点座標
    */
    BoostPoint Point() { return m_pt; }

    /*!
     * @brief 道路交差点座標のセッター
     * @param[in] pt 座標
    */
    void Point(BoostPoint pt) { m_pt = pt; }

    /*!
     * @brief 分岐数のゲッター
     * @return 分岐数
    */
    int BranchNum() { return m_nBranch; }

    /*!
     * @brief 分岐数のセッター
     * @param[in] nBranch 分岐数
    */
    void BranchNum(int nBranch) { m_nBranch = nBranch; }

    /*!
     * @brief ボロノイ分割時のセル領域のゲッター
     * @return ボロノイ分割時のセル領域
    */
    BoostPolygon Cell() { return m_voronoiCell; }

    /*!
     * @brief ボロノイ分割時のセル領域のセッター
     * @param[in] polygon ボロノイ分割時のセル領域
    */
    void Cell(BoostPolygon polygon) { m_voronoiCell = polygon; }

    /*!
     * @brief 交差点領域のゲッター
     * @return 交差点領域
    */
    BoostPolygon Area() { return m_area; }

    /*!
     * @brief 交差点領域のセッター
     * @param[in] polygon 交差点領域
    */
    void Area(BoostPolygon polygon) { m_area = polygon; }
protected:
private:
    BoostPoint m_pt;    //!< 座標
    int m_nBranch;      //!< 分岐数
    BoostPolygon m_voronoiCell; //!< ボロノイ分割時のセル領域
    BoostPolygon m_area;        //!< 交差点領域

};

// RTree用
typedef bg::index::rtree<std::pair<BoostPoint, BoostVertexDesc>,
    bg::index::quadratic<16>> BoostVertexRTree;    //!< RTree定義(無向グラフの頂点デスクリプター用)
typedef bg::index::rtree<std::pair<BoostPoint, BoostEdgeDesc>,
    bg::index::quadratic<16>> BoostEdgeRTree;        //!< RTree定義(無向グラフのエッジデスクリプター用)

typedef bg::index::rtree<std::pair<BoostPoint, BoostMultiLines::iterator>,
    bg::index::quadratic<16>> BoostMultiLinesItRTree;    //!< RTree定義(マルチラインのイテレータ用)

/*!
 * @brief ポリゴン内における近傍点探索用の情報
*/
struct NearestPointInfo
{
    BoostMultiPolygon::iterator m_itPolygon;    //!< ポリゴン
    bool m_bInner;                              //!< 近傍点が穴の点か否か
    int m_nInnerIdx;                            //!< 近傍点が穴の場合のインデックス
    BoostRing::iterator m_itPt;                 //!< 近傍点イテレータ

    /*!
     * @brief コンストラクタ
    */
    NearestPointInfo(
        BoostMultiPolygon::iterator itPolygon,
        BoostRing::iterator itPt,
        bool bInner,
        int nInnerIdx = 0)
    {
        this->m_itPolygon = itPolygon;
        this->m_itPt = itPt;
        this->m_bInner = bInner;
        this->m_nInnerIdx = nInnerIdx;
    }

    /*!
     * @brief コピーコンストラクタ
    */
    NearestPointInfo(const NearestPointInfo &info) { *this = info; }

    /*!
     * @brief 代入演算子
    */
    NearestPointInfo &operator =(const NearestPointInfo &info)
    {
        if (&info != this)
        {
            this->m_itPolygon = info.m_itPolygon;
            this->m_itPt = info.m_itPt;
            this->m_bInner = info.m_bInner;
            this->m_nInnerIdx = info.m_nInnerIdx;
        }
        return *this;
    }
};

typedef bg::index::rtree<std::pair<BoostPoint, NearestPointInfo>,
    bg::index::quadratic<16>> BoostPolygonRTree;    //!< RTree定義(ポリゴン用)

