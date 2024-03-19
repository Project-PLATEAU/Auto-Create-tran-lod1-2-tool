#! /usr/bin/env python3

import os
from pathlib import Path
from pyproj import Transformer
from tqdm import tqdm
import shapely
from shapely.geometry import Polygon
import numpy as np
import yaml
from ..util.parammanager import ParamManager
from ..util.config import Config
from ..util.log import Log, ModuleType, LogLevel
from ..util.resulttype import ProcessResult, ResultType
from ..tools.citygml import parse_city_gml_file
from ..tools.citygml import parse_shape_file
from ..tools.geotiff import read_geotiff, get_bounds, read_geotiff_ras, get_bounds_ras

class InputCityGmlManager:
    """CityGML建物情報クラス
    """
    def __init__(self, param_manager: ParamManager, output_dir: Path) -> None:
        """コンストラクタ

        Args:
            param_manager (ParamManager): パラメータ情報
        """
        self.param_manager = param_manager      # パラメータ情報
        self.output_dir = output_dir
        self.citygml_info = []
        #self.lod1_file_path = ''
        #self._obj_info = ObjInfo()
        #self._nsmap = None

    #def load_citygml(
    #        ortho_dir: Path,
    #        citygml_dir: Path,
    #        output_dir: Path,
    #        ortho_epsg: int,
    #        citygml_epsg: int,
    def load_citygml(self
    ) -> None:
        """
        CityGMLから道路情報を読み込み、オルソ画像の領域毎に保存する
        """
        try:
            restype = ResultType.ERROR

            if os.path.exists(self.output_dir):
                Log.output_log_write(
                    LogLevel.WARN,
                    ModuleType.LOAD_CITYGML,
                    str(self.output_dir) + ' already exists')
            else:
                self.output_dir.mkdir(parents=True)

            if self.param_manager.input_type == 1:
                # CityGMLファイル一覧の取得
                input_files = list(filter(
                    lambda file: file.suffix.lower() in ['.gml'],
                    self.param_manager.citygml_dir.iterdir()
                ))

                if len(input_files) == 0:
                    # 入力CityGMLファイルがない場合
                    raise FileNotFoundError
                
            elif self.param_manager.input_type == 2:
                # シェープファイル一覧の取得
                input_files = list(filter(
                    lambda file: file.suffix.lower() in ['.shp'],
                    self.param_manager.shape_dir.iterdir()
                ))

                if len(input_files) == 0:
                    # 入力シェープファイルがない場合
                    raise FileNotFoundError 

            ortho_files = list(filter(
                lambda file: file.suffix.lower() in ['.tif', '.tiff'],
                self.param_manager.ortho_dir.iterdir()
            ))

            Log.output_log_write(
                LogLevel.DEBUG,
                ModuleType.LOAD_CITYGML,
                'found ' + str(len(ortho_files)) + ' ortho files')

            # (citygmlName, roadId, polygon)
            polygons: list[tuple[str, str, Polygon]] = []

            tr = Transformer.from_proj(self.param_manager.citygml_epsg, self.param_manager.ortho_epsg, always_xy=True)

            for input_file in tqdm(input_files, leave=None):

                # 座標系を変換してpolygonsに追加する
                if self.param_manager.input_type == 1:
                    # CityGMLファイルの場合
                    city_gml_polygons = parse_city_gml_file(input_file)

                    polygons.extend([
                        (
                            input_file.name,
                            city_gml_polygon[0],
                            shapely.transform(
                                city_gml_polygon[1],
                                lambda pos: np.stack(
                                    tr.transform(pos[:, 0], pos[:, 1]), axis=1)
                            )
                        )
                        for city_gml_polygon in city_gml_polygons
                    ])
                elif self.param_manager.input_type == 2:
                    # Shapeファイルの場合
                    city_gml_polygons = parse_shape_file(input_file)

                    polygons.extend([
                        (
                            input_file.name,
                            city_gml_polygon[0],
                            city_gml_polygon[1]
                        )
                        for city_gml_polygon in city_gml_polygons
                    ])

                # 道路情報デバッグログ用
                for city_gml_polygon in city_gml_polygons:
                    rinfo = self.RoadInfo()
                    # 建物ID
                    rinfo.citygml_filename = input_file.name
                    rinfo.road_id = city_gml_polygon[0]
                    # rinfo.load_citygml = ResultType.SUCCESS
                    self.citygml_info.append(rinfo)

            for ortho_file in tqdm(ortho_files, leave=None):
                ortho, tfw = read_geotiff_ras(ortho_file)

                assert tfw[1] == 0 and tfw[2] == 0, "回転角度が0°以外のオルソ画像は非対応です"
                bounds = get_bounds_ras(ortho, tfw)

                # 領域内に完全に収まっているpolygonを抽出
                eps = 1e-1  # 許容誤差
                inner_polygons: list[tuple[str, str, Polygon]] = list(filter(
                    lambda polygon:
                    bounds[0] - eps <= polygon[2].bounds[0] and
                    polygon[2].bounds[2] <= bounds[2] + eps and
                    bounds[1] - eps <= polygon[2].bounds[1] and
                    polygon[2].bounds[3] <= bounds[3] + eps,
                    polygons
                ))

                info = [
                    {
                        "citygml": citygmlName,
                        "id": roadId,
                        "polygon": [(x, y) for x, y in polygon.exterior.coords],
                    }
                    for citygmlName, roadId, polygon in inner_polygons
                ]

                with (self.output_dir / f'{ortho_file.stem}.yml').open('wt') as f:
                    yaml.safe_dump({
                        'ortho': ortho_file.name,
                        #'size': ortho.size,  # [width, height],
                        'size': [ortho.width, ortho.height],
                        'tfw': tfw,
                        'roads': info,
                    }, f)

                if len(info) != 0:
                    restype = ResultType.SUCCESS

                    for i in info:
                        id = i["id"]
                        cinfo = [c for c in self.citygml_info if id == c.road_id]
                        if len(cinfo) != 0:
                            cinfo[0].load_citygml = ResultType.SUCCESS
                else:
                    Log.output_log_write(
                        LogLevel.ERROR,
                        ModuleType.LOAD_CITYGML,
                        'support ortho files not found')
            return restype, self.citygml_info

        except FileNotFoundError:
            Log.output_log_write(LogLevel.ERROR,
                                 ModuleType.LOAD_CITYGML,
                                 'File not found')
            return ResultType.ERROR, None

        except Exception as e:
            Log.output_log_write(LogLevel.ERROR,
                                 ModuleType.LOAD_CITYGML, e)
            return ResultType.ERROR, None
        
    class RoadInfo:
        """CityGML出力道路情報クラス(道路id毎)
        """
        def __init__(self):
            """コンストラクタ
            """
            self.citygml_filename = ''
            self.road_id = ''     # 道路ID
            self.create_result = ResultType.SUCCESS             # 最終結果
            self.load_citygml = ProcessResult.SKIP              # CityGML入力結果
            self.check_road_connectivity = ProcessResult.SKIP   # 道路の隣接関係の判定結果
            # self.generate_inference_input = ProcessResult.SKIP  # 推論用データの生成結果
            # self.infer = ProcessResult.SKIP                     # セグメンテーション結果
            # self.remove_noise = ProcessResult.SKIP              # ノイズ除去結果
            # self.remove_occlusion = ProcessResult.SKIP          # オクルージョンの再分類結果
            self.vectorize = ProcessResult.SKIP                 # ベクトル化結果
