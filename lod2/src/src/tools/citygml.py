#! /usr/bin/env python3

from pathlib import Path
from lxml import etree
import numpy as np
import shapefile
from shapely.geometry import Polygon
from uuid import uuid4


def parse_polygon_gml(polygon) -> Polygon:
    """
    gml:Polygon を読み、shapelyのPolygonを返す
    内部境界(gml:interior)は無視される
    """

    pos_list_path = '/'.join([
        'gml:exterior',
        'gml:LinearRing',
        'gml:posList',
    ])

    exterior = polygon.xpath(pos_list_path, namespaces=polygon.nsmap)

    raw_pos_list = map(lambda s: float(s), exterior[0].text.split())
    pos_list = np.array(list(raw_pos_list)).reshape((-1, 3))[:, [1, 0, 2]]

    return Polygon(pos_list)


def parse_road_gml(road) -> tuple[str, Polygon]:
    """
    tran:Road を読み、道路IDと道路ポリゴンの組を返す
    """
    polygon_path = '/'.join([
        'tran:lod1MultiSurface',
        'gml:MultiSurface',
        'gml:surfaceMember',
        'gml:Polygon'
    ])

    id = road.attrib[f'{{{road.nsmap["gml"]}}}id']
    polygon = road.xpath(polygon_path, namespaces=road.nsmap)

    #assert len(polygon) == 1

    #return id, parse_polygon_gml(polygon[0])

    if len(polygon)==0:
        return id, None
    else:
        return id, parse_polygon_gml(polygon[0])    


def parse_city_gml_file(input_file: Path) -> list[tuple[str, Polygon]]:
    """
    CityGMLを読み、道路のidとポリゴンの一覧を返す
    """
    
    tree = etree.parse(input_file, parser=None)
    root = tree.getroot()

    road_path = '/'.join([
        '',
        'core:CityModel',
        'core:cityObjectMember',
        'tran:Road',
    ])

    gml_roads = tree.xpath(road_path, namespaces=root.nsmap)
    

    #return [
    #    parse_road_gml(gml_road) for gml_road in gml_roads
    #]

    result = []
    for gml_road in gml_roads:
        id, poly = parse_road_gml(gml_road)
        if poly is not None:
            result.append((id, poly))
    return result

def parse_shape_file(input_file: Path) -> list[tuple[str, Polygon]]:
    """
    シェープファイルを読み、道路のidとポリゴンの一覧を返す
    """
    src = shapefile.Reader(input_file, encoding='cp932')
    shps = src.shapes()
    ret = list()

    for shp in shps:
        road_id = f'tran_{str(uuid4())}'   
        ply = road_id, Polygon(shp.points)
        ret.append(ply)

    return ret

def generate_random_lod2_polygon_id() -> str:
    """
    LOD2道路ポリゴンに割り当てるIDをランダム生成する
    """
    return f'poly_{str(uuid4())}'


def generate_random_lod2_traffic_area_id() -> str:
    """
    LOD2道路TrafficAreaに割り当てるIDをランダム生成する
    """
    return f'tra_{str(uuid4())}'


def get_area_name_and_function_code(road_class: int, is_intersection: bool):
    """
    地物名と種類コードを取得する
    """

    if road_class == 1:
        if is_intersection:
            return "TrafficArea", "1000"
        else:
            return "TrafficArea", "1020"
    elif road_class == 2:
        return "TrafficArea", "2000"
    elif road_class == 3:
        return "AuxiliaryTrafficArea", "3000"
    else:
        raise ValueError(f"road_class {road_class} is invalid")


def add_lod2_road(
    tree,
    polygon: Polygon,
    lod1_road_id: str,
    lod2_polygon_id: str,
    polygon_class: int,
    is_intersection: bool,
) -> None:
    area_name, function_code = get_area_name_and_function_code(
        polygon_class, is_intersection
    )

    road_path = '/'.join([
        '',
        'core:CityModel',
        'core:cityObjectMember',
        f'tran:Road[@gml:id="{lod1_road_id}"]',
    ])

    root = tree.getroot()
    nsmap = root.nsmap
    tran_ns = nsmap['tran']
    gml_ns = nsmap['gml']
    xlink_ns = nsmap['xlink']

    gml_roads = tree.xpath(road_path, namespaces=nsmap)

    assert len(gml_roads) >= 1, f"{lod1_road_id}をidにもつtran:Roadが見つかりませんでした"

    road = gml_roads[0]

    # tran:Road/tran:{area_name}/tran:lod2MultiSurface/gml:MultiSurface/gml:surfaceMember/gml:Polygon の追加
    area = etree.Element(
        f'{{{tran_ns}}}{area_name}',
        attrib={f"{{{gml_ns}}}id": generate_random_lod2_traffic_area_id()},
        nsmap=nsmap,
    )

    tranFunction = etree.SubElement(
        area,
        f"{{{tran_ns}}}function",
        attrib={
            "codeSpace": f"../../codelists/{area_name}_function.xml"
        },
        nsmap=None
    )
    tranFunction.text = function_code

    lod2_multi_surface = etree.SubElement(
        area, f"{{{tran_ns}}}lod2MultiSurface", attrib=None, nsmap=nsmap)
    multi_surface = etree.SubElement(
        lod2_multi_surface, f"{{{gml_ns}}}MultiSurface", attrib=None, nsmap=nsmap)
    surface_member = etree.SubElement(
        multi_surface, f"{{{gml_ns}}}surfaceMember", attrib=None, nsmap=nsmap)
    surface_member.append(create_gml_polygon(
        polygon, lod2_polygon_id, nsmap=nsmap))

    road.append(area)

    # tran:Road/tran:lod2MultiSurface/gml:MultiSurface/gml:surfaceMember/gml:CompositeSurface/gml:surfaceMember の作成
    composite_surface = road.xpath(
        '/'.join([
            'tran:lod2MultiSurface',
            'gml:MultiSurface',
            'gml:surfaceMember',
            'gml:CompositeSurface',
        ]),
        namespaces=nsmap
    )

    if len(composite_surface) >= 1:
        # 存在する場合はそれを参照
        assert len(composite_surface) == 1
        composite_surface = composite_surface[0]
    else:
        # 存在しない場合は作成
        a = etree.SubElement(
            road, f"{{{tran_ns}}}lod2MultiSurface", attrib=None, nsmap=nsmap)
        b = etree.SubElement(
            a, f"{{{gml_ns}}}MultiSurface", attrib=None, nsmap=nsmap)
        c = etree.SubElement(
            b, f"{{{gml_ns}}}surfaceMember", attrib=None, nsmap=nsmap)
        composite_surface = etree.SubElement(
            c, f"{{{gml_ns}}}CompositeSurface", attrib=None, nsmap=nsmap)

    etree.SubElement(
        composite_surface,
        f"{{{gml_ns}}}surfaceMember",
        attrib={f"{{{xlink_ns}}}href": f"#{lod2_polygon_id}"},
        nsmap=nsmap
    )


def create_gml_polygon(polygon: Polygon, polygon_id: str, nsmap: dict[str, str]):
    gml_polygon = etree.Element(
        f"{{{nsmap['gml']}}}Polygon",
        attrib={f"{{{nsmap['gml']}}}id": polygon_id},
        nsmap=nsmap
    )

    items = [
        (f'{{{nsmap["gml"]}}}exterior', polygon.exterior.coords),
    ] + [
        (f'{{{nsmap["gml"]}}}interior', interior.coords) for interior in polygon.interiors
    ]

    for name, coords in items:
        a = etree.Element(name, attrib=None, nsmap=None)
        b = etree.SubElement(
            a, f'{{{nsmap["gml"]}}}LinearRing', attrib=None, nsmap=None)
        c = etree.SubElement(
            b, f'{{{nsmap["gml"]}}}posList', attrib=None, nsmap=None)
        c.text = ' '.join([
            f'{y} {x} 0' for x, y in coords
        ])

        gml_polygon.append(a)

    return gml_polygon
