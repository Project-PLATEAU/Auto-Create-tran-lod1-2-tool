import itertools
from pathlib import Path
import numpy as np
from tqdm import tqdm
import yaml
from ..util.parammanager import ParamManager
from ..util.config import Config
from ..util.log import Log, ModuleType, LogLevel
from ..util.resulttype import ProcessResult, ResultType

class CheckRoadConnectivity:
    """道路同士の隣接関係を調べる
    """
    def __init__(self, param_manager: ParamManager, road_info_dir: Path, output_dir: Path) -> None:
        """コンストラクタ

        Args:
            param_manager (ParamManager): パラメータ情報
        """
        self.param_manager = param_manager      # パラメータ情報
        self.road_info_dir = road_info_dir
        self.output_dir = output_dir

    def check_road_connectivity(self, roadsInfo) -> None:
        """
        道路同士の隣接関係を調べる
        """
        try:
            restype = ResultType.SUCCESS

            road_info_files = list(filter(
                lambda file: file.suffix.lower() in ['.yaml', '.yml'],
                self.road_info_dir.iterdir()
            ))

            if self.output_dir.exists():
                Log.output_log_write(
                    LogLevel.WARN,
                    ModuleType.CHECK_ROAD_CONNECTIVITY,
                    str(self.output_dir) + ' already exists')
            else:
                self.output_dir.mkdir(parents=True)

            for road_info_file in tqdm(road_info_files, leave=None):
                with road_info_file.open('rt') as f:
                    road_info = yaml.safe_load(f)
                roads = road_info['roads']

                # 辺の一覧と各辺を持つ道路のid一覧
                edges_coords = np.zeros((0, 4), dtype=np.float_)  # (x1,y1,x2,y2)
                edges_roads: list[list[str]] = []

                for road in tqdm(roads, leave=None):
                    road_id = road['id']
                    polygon: list[list[float]] = road['polygon']

                    for p1, p2 in zip(polygon[:-1], polygon[1:]):  # itertools.pairwise
                        # 辺の向きは両方向考慮する
                        edge = np.array([*p1, *p2])
                        edge_r = np.array([*p2, *p1])

                        # すでに調べた辺とのマンハッタン距離を求める
                        distance = np.abs(np.concatenate([
                            (edges_coords - edge),
                            (edges_coords - edge_r),
                        ])).sum(axis=1)

                        # マンハッタン距離が最小の辺を求める
                        if len(distance) >= 1:
                            min_idx = np.argmin(distance)
                        else:
                            min_idx = None

                        if min_idx is not None and distance[min_idx] < 1e-4 * 4:
                            # 距離が一定値より小さい場合は同じ辺と判断し、該当の辺の道路一覧にidを追加する
                            edges_roads[min_idx % len(edges_roads)].append(road_id)
                        else:
                            # 距離が一定値より大きい場合は新たな辺として追加する
                            edges_roads.append([road_id])
                            edges_coords = np.append(edges_coords, [edge], axis=0)

                # 各道路の隣接先の集合
                neighbors: dict[str, set[str]] = {}
                for road in roads:
                    neighbors[road['id']] = set()

                    # デバッグログ用
                    for info in roadsInfo:
                        if info.road_id in road['id']:
                            roadsInfo[roadsInfo.index(info)].check_road_connectivity = ResultType.SUCCESS
                            break
                        
                for edge_roads in edges_roads:
                    for road1, road2 in itertools.combinations(edge_roads, 2):
                        neighbors[road1].add(road2)
                        neighbors[road2].add(road1)

                road_info_with_neighbors = dict(
                    road_info,
                    roads=[
                        dict(road, neighbors=list(neighbors[road['id']]))
                        for road in roads
                    ],
                )

                with (self.output_dir / road_info_file.name).open('wt') as f:
                    yaml.safe_dump(road_info_with_neighbors, f)

            return restype

        except FileNotFoundError:
            Log.output_log_write(LogLevel.ERROR,
                                 ModuleType.CHECK_ROAD_CONNECTIVITY,
                                 'File not found')
            return ResultType.ERROR

        except Exception as e:
            Log.output_log_write(LogLevel.ERROR,
                                 ModuleType.CHECK_ROAD_CONNECTIVITY, e)
            return ResultType.ERROR
