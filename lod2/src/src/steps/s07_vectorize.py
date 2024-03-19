from pathlib import Path
from typing import Optional
import numpy as np
from tqdm import tqdm
from PIL import Image
import yaml
import shapely
from shapely.geometry import Polygon, LineString
import geopandas as gpd

from ..tools.geotiff import transform_img_coord_to_shp, transform_shp_to_img_coord
from ..tools.raster import extract_borders, get_mode_label_in_polygon
from ..tools.vector import smoothing, split_polygon
from ..tools.citygml import generate_random_lod2_polygon_id
from ..util.parammanager import ParamManager
from ..util.config import Config
from ..util.log import Log, ModuleType, LogLevel
from ..util.resulttype import ProcessResult, ResultType

class Vectorize:
    """ベクトル化する
    """
    def __init__(
        self, 
        param_manager: ParamManager, 
        input_dir: Path, 
        road_info_dir: Path,
        output_dir: Path,
        output_shape_dir: Path) -> None:
        """コンストラクタ

        Args:
            param_manager (ParamManager): パラメータ情報
        """
        self.param_manager = param_manager      # パラメータ情報
        self.input_dir = input_dir
        self.road_info_dir = road_info_dir
        self.output_dir = output_dir
        self.output_shape_dir = output_shape_dir

    def vectorize(
        self, 
        roadsInfo
    ) -> None:
        """
        セグメンテーションの結果をベクトル化する
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
                    ModuleType.VECTORIZE,
                    str(self.output_dir) + ' already exists')
            else:
                self.output_dir.mkdir(parents=True)

            if self.output_shape_dir.exists():
                Log.output_log_write(
                    LogLevel.WARN,
                    ModuleType.VECTORIZE,
                    str(self.output_shape_dir) + ' already exists')
            else:
                self.output_shape_dir.mkdir(parents=True)

            for input_file in tqdm(input_files, leave=None):
                # セグメンテーション結果の読み込み
                labels = np.array(Image.open(input_file), dtype=np.uint8)
                assert len(labels.shape) == 2

                # オルソと道路の情報を読み込み
                road_info_path = self.road_info_dir / (input_file.stem + ".yml")
                with road_info_path.open('rt') as f:
                    road_info = yaml.safe_load(f)
                tfw = road_info['tfw']
                ortho_size = road_info['size']

                # 境界線の抽出
                lines = extract_borders(labels, ignore_label=0)

                # 境界線のスムージング
                border_geoms: list[Polygon | LineString] = []
                for line, is_loop in lines:
                    if is_loop:
                        geom = Polygon(line)
                    else:
                        geom = LineString(line)

                    smooth_geom = smoothing(geom, dp_epsilon=3, smooth_iter=5)

                    border_geoms.append(smooth_geom)

                # 地理座標系に変換する
                border_geoms_geocoords = [
                    shapely.transform(
                        geometry, lambda p: transform_img_coord_to_shp(tfw, p[:, ::-1]))
                    for geometry in border_geoms
                ]

                lod2_data = []

                for road in tqdm(road_info['roads'], leave=None):
                    outline = Polygon(road['polygon'])
                    road_id: str = road['id']
                    citygml_name: str = road['citygml']
                    is_intersection: bool = len(road['neighbors']) >= 3

                    subparts = split_polygon(outline, border_geoms_geocoords)

                    for subpart in subparts:
                        subpart_label = get_mode_label_in_polygon(
                            labels,
                            subpart,
                            lambda p: transform_shp_to_img_coord(tfw, p),
                            ignore_label=0
                        )

                        lod2_data.append({
                            'lod1_id': road_id,
                            'lod2_id': generate_random_lod2_polygon_id(),
                            'citygml': citygml_name,
                            'class': subpart_label,
                            'geometry': subpart,
                            'is_in_intersection': is_intersection
                        })
                        # デバッグログ用
                        for info in roadsInfo:
                            if info.road_id in road['id']:
                                roadsInfo[roadsInfo.index(info)].vectorize = ResultType.SUCCESS
                                break

                output_shape_path = (
                    self.output_shape_dir / road_info['ortho']).with_suffix(".shp")
                output_yaml_path = (
                    self.output_dir / road_info['ortho']).with_suffix(".yml")

                gdf = gpd.GeoDataFrame(
                    data={
                        'lod1_id': [item['lod1_id'] for item in lod2_data],
                        'citygml': [item['citygml'] for item in lod2_data],
                        'class': [item['class'] for item in lod2_data],
                        'geometry': [item['geometry'] for item in lod2_data],
                        # シェープファイルの属性名は10文字制限のため、先頭10文字にする
                        'is_in_intersection'[:10]: [item['is_in_intersection'] for item in lod2_data],
                    },
                    crs=f'epsg:{self.param_manager.ortho_epsg}' if self.param_manager.ortho_epsg is not None else None
                )  # type: ignore
                gdf.to_file(output_shape_path, encoding='utf-8')

                with output_yaml_path.open('wt') as f:
                    yaml.safe_dump([
                        dict(item, geometry=item['geometry'].wkt)
                        for item in lod2_data
                    ], f)

            return restype

        except FileNotFoundError:
            Log.output_log_write(LogLevel.ERROR,
                                 ModuleType.VECTORIZE,
                                 'File not found')
            return ResultType.ERROR

        except Exception as e:
            Log.output_log_write(LogLevel.ERROR,
                                 ModuleType.VECTORIZE, e)
            return ResultType.ERROR