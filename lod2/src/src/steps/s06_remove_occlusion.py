from pathlib import Path
from typing import cast
from PIL import Image
import cv2
from tqdm import tqdm
import numpy as np
import numpy.typing as npt
from sortedcontainers import SortedList

from ..tools.color import put_annotation_color_pallette
from ..util.parammanager import ParamManager
from ..util.config import Config
from ..util.log import Log, ModuleType, LogLevel
from ..util.resulttype import ProcessResult, ResultType

class RemoveOcclusion:
    """オクルージョンの再分類
    """
    def __init__(self, param_manager: ParamManager, input_dir: Path, output_dir: Path) -> None:
        """コンストラクタ

        Args:
            param_manager (ParamManager): パラメータ情報
        """
        self.param_manager = param_manager      # パラメータ情報
        self.input_dir = input_dir
        self.output_dir = output_dir

    def remove_occlusion(
        self, 
        roadsInfo,
        step_weight: float = 0.1,
    ):
        """
        オクルージョンを周囲のラベル情報を利用して、オクルージョン以外のラベルに再分類する
        """
        try:
            restype = ResultType.SUCCESS
            input_files = list(filter(
                lambda file: file.suffix.lower() in ['.tif', '.tiff'],
                self.input_dir.iterdir()
            ))

            if self.output_dir.exists():
                Log.output_log_write(
                    LogLevel.WARN,
                    ModuleType.REMOVE_OCCLUSION,
                    str(self.output_dir) + ' already exists')
            else:
                self.output_dir.mkdir(parents=True)

            for input_file in tqdm(input_files, leave=None):
                input = Image.open(input_file)
                palette = input.getpalette()
                labels = np.array(input)

                # 道路外からの距離を求める
                distance_from_outline: npt.NDArray[np.float_] = cv2.distanceTransform(
                    np.where(labels == 0, np.uint8(0), np.uint8(255)),
                    distanceType=cv2.DIST_L2,
                    maskSize=5
                )

                # 昇順に格納するリスト
                # データ：(スコア、元ピクセルの道路外からの距離、現在の道のりピクセル数、元ピクセルのy座標、元ピクセルのx座標、ラベル)
                # スコアは、道路外からの距離の差 と 道のりのピクセル数 から算出し、値が小さいほど優先的に採用される
                sl = SortedList()

                h, w = labels.shape

                def add_neighbors_if_need(y: int, x: int, original_dist: float, steps: int):
                    """
                    (y, x)の隣接4ピクセルの内必要なものをスコア等の情報と一緒にリストへ追加する
                    """
                    for dx, dy in [(0, 1), (0, -1), (1, 0), (-1, 0)]:
                        if 0 <= y + dy < h and 0 <= x + dx < w and labels[y+dy, x+dx] == 4:
                            score = abs(
                                original_dist - distance_from_outline[y+dy, x+dx]
                            ) + steps * step_weight

                            sl.add((
                                score,
                                original_dist,
                                steps,
                                y+dy,
                                x+dx,
                                labels[y][x]
                            ))

                # オクルージョン以外と隣接している場合、そのピクセルをリストへ追加する
                for y, x in cast(list[tuple[int, int]], zip(*np.where(labels == 4))):
                    for dx, dy in [(0, 1), (0, -1), (1, 0), (-1, 0)]:
                        if 0 <= y + dy < h and 0 <= x + dx < w and int(labels[y+dy, x+dx]) not in [0, 4]:
                            add_neighbors_if_need(
                                y+dy, x+dx, float(distance_from_outline[y+dy, x+dx]), 0)

                # スコアが小さいピクセルをオクルージョンから書き換え、その隣接ピクセルをリストへ追加する
                # 対象がリストから消えるまで繰り返す
                with tqdm(total=(labels == 4).sum(), leave=False) as pbar:
                    while len(sl) > 0:
                        _, original_dist, steps, y, x, label = sl.pop(index=0)

                        if labels[y, x] == 4:
                            labels[y, x] = label
                            add_neighbors_if_need(y, x, original_dist, steps + 1)
                            pbar.update(1)

                if np.any(labels == 4):
                    # 連結成分内がオクルージョンのみの場合は車道にする
                    Log.output_log_write(
                        LogLevel.DEBUG,
                        ModuleType.REMOVE_OCCLUSION,
                        'remain occlusion, rewrite roadway')
                    labels[labels == 4] = 1

                output = Image.fromarray(labels, 'P')
                put_annotation_color_pallette(output)

                output.save(self.output_dir / input_file.name)

            return restype

        except FileNotFoundError:
            Log.output_log_write(LogLevel.ERROR,
                                 ModuleType.REMOVE_OCCLUSION,
                                 'File not found')
            return ResultType.ERROR

        except Exception as e:
            Log.output_log_write(LogLevel.ERROR,
                                 ModuleType.REMOVE_OCCLUSION, e)
            return ResultType.ERROR
