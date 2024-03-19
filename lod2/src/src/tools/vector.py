import itertools
import logging
from typing import Union
import cv2
import numpy as np
from shapelysmooth import chaikin_smooth
from shapely.geometry import LineString, Polygon, Point, MultiLineString, MultiPolygon, MultiPoint, GeometryCollection
from shapely.ops import nearest_points, linemerge, unary_union, polygonize


def smoothing(
    geometry: Union[Polygon, LineString],
    dp_epsilon: float = 3,
    smooth_iter: int = 5,
) -> Union[Polygon, LineString]:
    """
    LineString、Polygonの辺からジャギーを除去し、なめらかにする
    """
    is_polygon = isinstance(geometry, Polygon)

    coords = np.array(
        geometry.exterior.coords if is_polygon else geometry.coords,
        dtype=np.int_
    )

    # 線の単純化
    if isinstance(geometry, Polygon) or np.all(coords[0] != coords[-1]):
        # ループまたは、始点と終点が一致していない線の場合
        fewer_points_path = cv2.approxPolyDP(
            coords,
            dp_epsilon,
            closed=is_polygon
        )[:, 0, :]
    else:
        # 開始地点と終端地点が同じ線の場合、閉路として処理されるため、最後の線を除いて単純化する
        fewer_points_path = np.concatenate([
            cv2.approxPolyDP(coords[:-1], dp_epsilon, False)[:, 0, :],
            coords[-1, None, :]
        ])

    assert len(fewer_points_path) >= 2

    if is_polygon and len(fewer_points_path) >= 3:  # 2頂点になった場合、ループとはみなさない
        fewer_points_geometry = Polygon(fewer_points_path)
    else:
        fewer_points_geometry = LineString(fewer_points_path)

    # 線をなめらかにする
    smooth_geometry = chaikin_smooth(
        fewer_points_geometry,
        iters=smooth_iter
    )

    return smooth_geometry


def to_line_strings(
        geometry: Union[MultiLineString, MultiPolygon,
                        MultiPoint, Point, Polygon, LineString]
) -> list[LineString]:
    """
    Geometryを分解してLineStringの配列にする
    Pointは除去される
    """
    if isinstance(geometry, (MultiLineString, MultiPolygon, GeometryCollection)):
        return list(itertools.chain.from_iterable([
            to_line_strings(geom) for geom in geometry.geoms
        ]))
    elif isinstance(geometry, Polygon):
        boundary = geometry.boundary
        return [boundary] if len(boundary.coords) >= 2 else []
    elif isinstance(geometry, LineString):
        return [geometry] if len(geometry.coords) >= 2 else []
    elif isinstance(geometry, (MultiPoint, Point)):
        return []

    raise TypeError(f'Unsupported polygon type ({geometry.geom_type})')


def split_polygon(
        polygon: Polygon,
        lines: list[Union[LineString, Polygon]]
) -> list[Polygon]:
    """
    ポリゴンを線で分割する
    """

    multi_lines = MultiLineString(
        list(itertools.chain.from_iterable([
            to_line_strings(line) for line in lines
        ]))
    )

    inner_lines = to_line_strings(
        polygon.intersection(multi_lines)
    ) if polygon.intersects(multi_lines) else []  # RuntimeWarning: invalid value encountered in intersection への対処

    terminals = list(itertools.chain.from_iterable([
        [
            geom.coords[0],
            geom.coords[-1]
        ] for geom in inner_lines
    ]))

    for i, inner_line in enumerate(inner_lines):
        coords = list(inner_line.coords)

        # 外と交差させるために延長する
        if terminals.count(coords[0]) == 1:
            point = nearest_points(
                polygon.boundary, Point(coords[0]))[0].coords[0]
            v = np.array(point) - np.array(coords[1])
            coords[0] = tuple(np.array(point) + v /
                              np.linalg.norm(v) * 0.01)

        if terminals.count(coords[-1]) == 1:
            point = nearest_points(
                polygon.boundary, Point(coords[-1]))[0].coords[0]
            v = np.array(point) - np.array(coords[-2])
            coords[-1] = tuple(np.array(point) + v /
                               np.linalg.norm(v) * 0.01)

        inner_lines[i] = LineString(coords)

    borders = linemerge([geom.boundary if isinstance(geom, Polygon)
                         else geom for geom in (inner_lines + [polygon])])
    borders = unary_union(borders)

    # ポリゴン化して、道路外のポリゴンは除く
    polygons = list(filter(
        lambda g: polygon.intersection(g).area > g.area * 0.9,
        polygonize(borders)
    ))

    return polygons
