import itertools
import math
from typing import Callable, Literal, Optional, Union, cast
import cv2
import numpy as np
import numpy.typing as npt
from PIL import Image, ImageDraw
from tqdm import tqdm
import networkx as nx
import shapely
from shapely.geometry import Polygon
from ..util.log import Log, ModuleType, LogLevel


def remove_small_area(
    labels: npt.NDArray[np.uint8],
    connectivity: Literal[4, 8],
    threshold: int = 200,
    enable_tqdm: bool = True
) -> npt.NDArray[np.uint8]:
    """
    閾値以下のピクセル数の領域を消去し、周囲のラベルに置き変える
    """
    REMOVED_LABEL = 199

    original_labels = labels
    labels = labels.copy()

    assert (labels == REMOVED_LABEL).sum() == 0

    # 小さい領域のラベルを消去する
    for label in tqdm([1, 2, 3, 4], desc='ノイズの抽出', leave=None, disable=not enable_tqdm):
        flags = np.where(labels == label, np.uint8(255), np.uint8(0))
        _, group_labels, stats, _ = cv2.connectedComponentsWithStats(
            flags, connectivity=connectivity)

        for i, area in list(enumerate(stats[:, -1]))[1:]:
            if area < threshold:
                labels[group_labels == i] = REMOVED_LABEL

    # ラベルが消去されたピクセルの上下左右に隣接しているラベルを求める
    hole_edge_color = np.full(labels.shape, 0, dtype=np.uint8)

    for dy, dx in tqdm([(1, 0), (-1, 0), (0, 1), (0, -1)], desc="ノイズのラベルの算出", leave=None, disable=not enable_tqdm):
        h, w = labels.shape
        tmp = np.zeros((h, w))

        tmp[max(0, -dy):h-max(dy, 0), max(0, -dx):w-max(dx, 0)] =\
            labels[max(0, dy):h+dy, max(0, dx):w+dx]

        hole_edge_color = np.where(
            (labels == REMOVED_LABEL) & (tmp != REMOVED_LABEL) & (tmp != 0),
            tmp,
            hole_edge_color
        )

    # 複数の小さい領域が繋がった可能性があるため、ラベルが消去された領域をグループ分けする
    flags = np.where(labels == REMOVED_LABEL, np.uint8(255), np.uint8(0))
    num_of_group_labels, group_labels, stats, _ = cv2.connectedComponentsWithStats(
        flags, connectivity=connectivity)

    # ラベルが消去された領域毎に一番隣接しているラベルを適用する
    for i in tqdm(range(1, num_of_group_labels), desc="ノイズのラベルの適用", leave=None, disable=not enable_tqdm):
        left, right = stats[i][0], stats[i][0] + stats[i][2]
        top, bottom = stats[i][1], stats[i][1] + stats[i][3]

        bbox_hole_edge_color = hole_edge_color[top:bottom, left:right]
        bbox_group_labels = group_labels[top:bottom, left:right]

        colors, counts = np.unique(
            bbox_hole_edge_color[bbox_group_labels == i],
            return_counts=True
        )

        assert REMOVED_LABEL not in colors

        if (colors != 0).sum() > 0:
            colors = colors[np.argsort(-counts, kind='stable')]

            labels[group_labels == i] = colors[colors != 0][0]
        else:
            # 周囲が全て0だった場合は、元の色の最頻値を使用
            original_colors, original_counts = np.unique(
                original_labels[group_labels == i],
                return_counts=True
            )
            labels[group_labels == i] = \
                original_colors[np.argsort(-original_counts, kind='stable')[0]]

    return labels


def extract_borders(
    labels: npt.NDArray[np.uint8],
    ignore_label: Optional[int] = None,
) -> list[tuple[list[tuple[int, int]], bool]]:
    """
    境界線を抽出する
    ignore_labelとの境界は無視される
    """

    # 上下のピクセルの比較 (左右方向の境界線)
    border_ud = \
        (labels[:-1, :] != labels[1:, :]) & \
        (labels[:-1, :] != ignore_label) & \
        (labels[1:, :] != ignore_label)
    outline_ud = \
        (labels[:-1, :] != labels[1:, :]) & \
        ((labels[:-1, :] == ignore_label) | (labels[1:, :] == ignore_label))

    border_ud_pos = np.argwhere(border_ud)
    outline_ud_pos = np.argwhere(outline_ud)

    # 左右のピクセルの比較 (上下方向の境界線)
    border_lr = \
        (labels[:, :-1] != labels[:, 1:]) & \
        (labels[:, :-1] != ignore_label) & \
        (labels[:, 1:] != ignore_label)
    outline_lr = \
        (labels[:, :-1] != labels[:, 1:]) & \
        ((labels[:, :-1] == ignore_label) | (labels[:, 1:] == ignore_label))

    border_lr_pos = np.argwhere(border_lr)
    outline_lr_pos = np.argwhere(outline_lr)

    G = nx.Graph()

    _, width = labels.shape
    def to_node_index(y, x): return y * (width + 1) + x
    def to_node_pos(index): return index // (width + 1), index % (width + 1)

    # 隣接する境界線ピクセル同士を繋いだグラフを作る
    border_edge_ud = np.stack([  # (y+1, x) - (y+1, x+1)
        to_node_index(border_ud_pos[:, 0] + 1, border_ud_pos[:, 1]),
        to_node_index(border_ud_pos[:, 0] + 1, border_ud_pos[:, 1] + 1),
    ], axis=1)
    border_edge_lr = np.stack([  # (y, x+1)-(y+1, x+1)
        to_node_index(border_lr_pos[:, 0],     border_lr_pos[:, 1] + 1),
        to_node_index(border_lr_pos[:, 0] + 1, border_lr_pos[:, 1] + 1),
    ], axis=1)

    outline_nodes = set(np.concatenate([
        to_node_index(outline_ud_pos[:, 0] + 1, outline_ud_pos[:, 1]),
        to_node_index(outline_ud_pos[:, 0] + 1, outline_ud_pos[:, 1] + 1),
        to_node_index(outline_lr_pos[:, 0],     outline_lr_pos[:, 1] + 1),
        to_node_index(outline_lr_pos[:, 0] + 1, outline_lr_pos[:, 1] + 1),
    ]))

    G.add_edges_from(border_edge_ud)
    G.add_edges_from(border_edge_lr)

    deg = np.array(G.degree).reshape(-1, 2)
    # 分解時の端点を列挙する。(次数が1または3以上の頂点、次数が2で道路外との境界上の頂点)
    terminals: set[int] = set(deg[deg[:, 1] != 2][:, 0]) | set(
        filter(lambda n: n in outline_nodes, deg[deg[:, 1] == 2][:, 0]))
    used_nodes: set[int] = set()

    paths: list[tuple[list[int], bool]] = []  # (nodes, is_loop)

    for terminal in tqdm(terminals, desc="境界線のベクトル化", leave=None):
        for neighbor in nx.all_neighbors(G, terminal):
            # 2重の処理を防ぐ
            if neighbor in terminals:
                # 2つ目の頂点がterminalの場合は小さい番号から大きい番号の方向のみを採用
                if terminal >= neighbor:
                    continue
            else:
                # 2つ目の頂点がterminal以外の場合は既に通っていない場合に採用
                if neighbor in used_nodes:
                    continue

            prev = terminal
            cur = neighbor
            path = [prev, cur]

            while cur not in terminals:
                prev, cur = cur, list(
                    set(nx.all_neighbors(G, cur)) - set([prev]))[0]
                path.append(cur)

            used_nodes.update(path)
            paths.append((path, False))

    # 独立したloopへの対応
    for node in tqdm(deg[deg[:, 1] == 2][:, 0], desc="境界線のベクトル化(ループ)", leave=None):
        if node in used_nodes:
            continue

        prev = node
        cur = list(nx.all_neighbors(G, node))[0]
        path = [prev, cur]

        while cur != node:
            prev, cur = cur, list(
                set(nx.all_neighbors(G, cur)) - set([prev]))[0]
            path.append(cur)

        used_nodes.update(path)
        paths.append((path, True))

    return [
        (
            [to_node_pos(node_index) for node_index in path],
            is_loop
        )
        for path, is_loop in paths
    ]


def get_mode_label_in_polygon(
    labels: npt.NDArray[Union[np.int_, np.uint]],
    polygon: Polygon,
    transform: Callable[[npt.NDArray[np.float_]], npt.NDArray[np.float_]],
    ignore_label: Optional[int] = None
) -> int:
    """
    ポリゴンの領域で一番多いラベルを取得する
    一つも存在しない場合は付近から取得する
    """
    assert len(labels.shape) == 2

    if transform:
        polygon = cast(Polygon, shapely.transform(polygon, transform))

    minx, miny, maxx, maxy = polygon.bounds
    left = max(math.floor(minx - 1), 0)
    right = min(math.ceil(maxx + 1), labels.shape[1])
    top = max(math.floor(miny - 1), 0)
    bottom = min(math.ceil(maxy + 1), labels.shape[0])

    mask = Image.new('1', (right-left, bottom - top))
    draw = ImageDraw.Draw(mask)

    # ポリゴンをラスタにする
    draw.polygon([
        (p[0] - left, p[1] - top) for p in polygon.exterior.coords
    ], fill=1)
    for interior in polygon.interiors:
        draw.polygon([
            (p[0] - left, p[1] - top) for p in interior.coords
        ], fill=0)

    assert np.array(mask).sum() > 0

    # 各ラベルの個数を求める
    target_area_labels = labels[top:bottom, left:right]
    values, counts = np.unique(
        target_area_labels[
            (np.array(mask) == 1) & (target_area_labels != ignore_label)
        ],
        return_counts=True
    )

    if len(values) > 0:
        return int(values[np.argmax(counts)])

    # 領域内に 0以外のラベルが存在しない場合、最も近い0以外のラベルを使用する
    mask_all = np.full(labels.shape, 255, dtype=np.uint8)
    mask_all[top:bottom, left:right][mask] = 0
    distance = cv2.distanceTransform(mask_all, cv2.DIST_L2, 5)

    targets = (mask_all == 255) & (labels != 0)
    label = int(labels[targets][np.argmin(distance[targets])])

    Log.output_log_write(
        LogLevel.DEBUG,
        ModuleType.VECTORIZE,
        'ポリゴンの領域にラベルが存在しないため、最も近いラベルの' + str(label)+ 'を使用します')
    return label
