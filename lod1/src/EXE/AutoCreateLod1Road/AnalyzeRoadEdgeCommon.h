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

typedef bg::model::d2::point_xy<double> BoostPoint;                 //!< boost��2D���W�^
typedef bg::model::linestring<BoostPoint> BoostPolyline;            //!< boost��2D�|�����C��
typedef bg::model::multi_point<BoostPoint> BoostMultiPoints;        //!< �����_�^
typedef bg::model::multi_linestring<BoostPolyline> BoostMultiLines; //!< �������C���^
typedef bg::model::box<BoostPoint> BoostBox;                        //!< ��`�^
typedef bg::model::ring<BoostPoint, false, true> BoostRing;         //!< �����O�^
typedef bg::model::polygon<BoostPoint, false, true> BoostPolygon;   //!< boost��2D�|���S��
typedef bg::model::multi_polygon<BoostPolygon> BoostMultiPolygon;   //!< boost�̕���2D�|���S��
// typedef bg::model::polygon<point, clock_wise, closed> polygon;
// clocke_wise = true(�\:���v���)
// clocke_wise = false(�\:�����v���)
// closed = true(�I�_�Ɏn�_�Ɠ���ȓ_��}�����ă|���S�������K�v������)
// closed = false(�I�_�Ɏn�_�Ɠ���ȓ_��}�����Ȃ��Ă��ǂ�)

typedef std::pair<BoostPolyline, BoostPolyline> BoostPairLine; //!< �|�����C���̃y�A�f�[�^
typedef std::pair<BoostPoint, BoostPoint> BoostPairPoint;       //!< ���_�y�A�f�[�^

// �{���m�C�����p
typedef bp::point_data<int> BoostVoronoiPoint;                  //!< �{���m�C�����p�̍��W�^
typedef bp::voronoi_diagram<double> BoostVoronoiDiagram;        //!< �{���m�C�_�C�A�O����
typedef BoostVoronoiDiagram::const_edge_iterator BoostVoronoiEdgeIt;    //!< �{���m�C�_�C�A�O�����̃G�b�W�C�e���[�^
typedef BoostVoronoiDiagram::const_cell_iterator BoostVoronoiCellIt;    //!< �{���m�C�_�C�A�O�����̃Z���C�e���[�^

// �O���t�p
struct BoostVertexProperty;
struct BoostEdgeProperty;

typedef boost::adjacency_list<
    boost::vecS, boost::vecS, boost::undirectedS,
    BoostVertexProperty,
    BoostEdgeProperty> BoostUndirectedGraph;  //!< �����O���t��`
typedef boost::graph_traits<BoostUndirectedGraph>::vertex_descriptor BoostVertexDesc;       //!< ���_��`(�����O���t)
typedef boost::graph_traits<BoostUndirectedGraph>::edge_descriptor BoostEdgeDesc;           //!< �G�b�W��`(�����O���t)


// �L���O���t
typedef boost::adjacency_list<
    boost::vecS, boost::vecS, boost::directedS,
    BoostVertexProperty,
    BoostEdgeProperty> BoostDirectedGraph;  //!< �L���O���t��`
typedef boost::graph_traits<BoostDirectedGraph>::vertex_descriptor BoostDVertexDesc;       //!< ���_��`(�L���O���t)
typedef boost::graph_traits<BoostDirectedGraph>::edge_descriptor BoostDEdgeDesc;           //!< �G�b�W��`(�L���O���t)

/*!
 * @brief �O���t�̒��_�v���p�e�B��`
*/
struct BoostVertexProperty
{
    BoostVertexDesc desc;   //!< �f�X�N���v�^�[
    BoostPoint pt;          //!< ���W
    bool isSearched;        //!< �T���ς݃t���O
    /*!
     * @brief �R���X�g���N�^
    */
    BoostVertexProperty()
    {
        desc = 0;
        pt.x(0.0);
        pt.y(0.0);
        isSearched = false;
    }
    /*!
     * @brief �R���X�g���N�^
    */
    BoostVertexProperty(const BoostPoint &p)
    {
        desc = 0;
        pt = p;
        isSearched = false;
    }

    /*!
     * @brief �R�s�[�R���X�g���N�^
    */
    BoostVertexProperty(const BoostVertexProperty &p) { *this = p; }

    /*!
     * @brief ������Z�q
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
 * @brief �O���t�̃G�b�W�v���p�e�B��`
*/
struct BoostEdgeProperty
{
    BoostVertexDesc vertexDesc1;    //!< ���_�f�B�X�N���v�^�\1
    BoostVertexDesc vertexDesc2;    //!< ���_�f�B�X�N���v�^�\2
    double dLength;                 //!< �G�b�W�̒���(�v�蓮�X�V)
    /*!
     * @brief �R���X�g���N�^
    */
    BoostEdgeProperty()
    {
        vertexDesc1 = 0;
        vertexDesc2 = 0;
        dLength = 0;
    }

    /*!
     * @brief �R���X�g���N�^
     * @param[in] desc1     ���_�f�X�N���v�^�[
     * @param[in] desc2     ���_�f�X�N���v�^�[
     * @param[in] length    �G�b�W�̒���
    */
    BoostEdgeProperty(BoostVertexDesc desc1, BoostVertexDesc desc2, double length = 0)
    {
        vertexDesc1 = desc1;
        vertexDesc2 = desc2;
        dLength = length;
    }

    /*!
     * @brief �R�s�[�R���X�g���N�^
    */
    BoostEdgeProperty(const BoostEdgeProperty &e) { *this = e; }

    /*!
     * @brief ������Z�q
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
 * @brief �[���D��T�����̓���
*/
class CBoostDFSVisitor : public boost::default_dfs_visitor
{
public:

    /*!
     * @brief �o�H
    */
    std::vector<BoostVertexDesc> &vecRoute;

    /*!
     * @brief �R���X�g���N�^
     * @param[in/out] vec �T���o�H�ۑ��p
    */
    CBoostDFSVisitor(std::vector<BoostVertexDesc> &vec) : vecRoute(vec) {}

    /*!
     * @brief ���_�������̋���
     * @tparam Vertex   ���_�N���X�̃e���v���[�g
     * @tparam Graph    �O���t�N���X�̃e���v���[�g
     * @param[in] u     ���_
     * @param[in] g     �O���t
    */
    template <typename Vertex, typename Graph>
    void discover_vertex(Vertex u, const Graph &g)
    {
        vecRoute.push_back(u);
    }
};

/*!
 * @brief �����_�f�[�^�N���X
 */
class CCrossingData
{
public:
    /*!
     * @brief �R���X�g���N�^
     */
    CCrossingData()
    {
        m_pt = BoostPoint();
        m_nBranch = 0;
        m_voronoiCell = BoostPolygon();
        m_area = BoostPolygon();
    }

    /*!
     * @brief �R���X�g���N�^
     */
    CCrossingData(BoostPoint pt, int nBranch)
    {
        m_pt = pt;
        m_nBranch = nBranch;
    }

    /*!
     * @brief �f�X�g���N�^
     */
    ~CCrossingData() {};

    /*!
     * @brief �R�s�[�R���X�g���N�^
     * @param[in] x �R�s�[���f�[�^
     */
    CCrossingData(const CCrossingData &x) { *this = x; }

    /*!
     * @brief ������Z�q
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
     * @brief �����_���W�̃Q�b�^�[
     * @return �����_���W
    */
    BoostPoint Point() { return m_pt; }

    /*!
     * @brief ���H�����_���W�̃Z�b�^�[
     * @param[in] pt ���W
    */
    void Point(BoostPoint pt) { m_pt = pt; }

    /*!
     * @brief ���򐔂̃Q�b�^�[
     * @return ����
    */
    int BranchNum() { return m_nBranch; }

    /*!
     * @brief ���򐔂̃Z�b�^�[
     * @param[in] nBranch ����
    */
    void BranchNum(int nBranch) { m_nBranch = nBranch; }

    /*!
     * @brief �{���m�C�������̃Z���̈�̃Q�b�^�[
     * @return �{���m�C�������̃Z���̈�
    */
    BoostPolygon Cell() { return m_voronoiCell; }

    /*!
     * @brief �{���m�C�������̃Z���̈�̃Z�b�^�[
     * @param[in] polygon �{���m�C�������̃Z���̈�
    */
    void Cell(BoostPolygon polygon) { m_voronoiCell = polygon; }

    /*!
     * @brief �����_�̈�̃Q�b�^�[
     * @return �����_�̈�
    */
    BoostPolygon Area() { return m_area; }

    /*!
     * @brief �����_�̈�̃Z�b�^�[
     * @param[in] polygon �����_�̈�
    */
    void Area(BoostPolygon polygon) { m_area = polygon; }
protected:
private:
    BoostPoint m_pt;    //!< ���W
    int m_nBranch;      //!< ����
    BoostPolygon m_voronoiCell; //!< �{���m�C�������̃Z���̈�
    BoostPolygon m_area;        //!< �����_�̈�

};

// RTree�p
typedef bg::index::rtree<std::pair<BoostPoint, BoostVertexDesc>,
    bg::index::quadratic<16>> BoostVertexRTree;    //!< RTree��`(�����O���t�̒��_�f�X�N���v�^�[�p)
typedef bg::index::rtree<std::pair<BoostPoint, BoostEdgeDesc>,
    bg::index::quadratic<16>> BoostEdgeRTree;        //!< RTree��`(�����O���t�̃G�b�W�f�X�N���v�^�[�p)

typedef bg::index::rtree<std::pair<BoostPoint, BoostMultiLines::iterator>,
    bg::index::quadratic<16>> BoostMultiLinesItRTree;    //!< RTree��`(�}���`���C���̃C�e���[�^�p)

/*!
 * @brief �|���S�����ɂ�����ߖT�_�T���p�̏��
*/
struct NearestPointInfo
{
    BoostMultiPolygon::iterator m_itPolygon;    //!< �|���S��
    bool m_bInner;                              //!< �ߖT�_�����̓_���ۂ�
    int m_nInnerIdx;                            //!< �ߖT�_�����̏ꍇ�̃C���f�b�N�X
    BoostRing::iterator m_itPt;                 //!< �ߖT�_�C�e���[�^

    /*!
     * @brief �R���X�g���N�^
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
     * @brief �R�s�[�R���X�g���N�^
    */
    NearestPointInfo(const NearestPointInfo &info) { *this = info; }

    /*!
     * @brief ������Z�q
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
    bg::index::quadratic<16>> BoostPolygonRTree;    //!< RTree��`(�|���S���p)

