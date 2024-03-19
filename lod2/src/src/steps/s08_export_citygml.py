from pathlib import Path
from lxml import etree
import numpy as np
from pyproj import Transformer
from tqdm import tqdm
import yaml
from shapely import from_wkt, transform

from ..tools.citygml import add_lod2_road
from ..util.parammanager import ParamManager
from ..util.config import Config
from ..util.log import Log, ModuleType, LogLevel
from ..util.resulttype import ProcessResult, ResultType

class ExportCityGmlManager:
    """CityGML出力
    """
    def __init__(
        self, 
        param_manager: ParamManager, 
        vectorized_dir: Path,
        output_dir: Path) -> None:
        """コンストラクタ

        Args:
            param_manager (ParamManager): パラメータ情報
        """
        self.param_manager = param_manager      # パラメータ情報
        self.vectorized_dir = vectorized_dir
        self.output_dir =output_dir

    def export_citygml(
        self, 
        roadsInfo
    ):
        """
        LOD1 CityGMLファイルにLOD2の情報を追記する
        """
        try:
            restype = ResultType.SUCCESS

            vectorized_files = list(filter(
                lambda file: file.suffix.lower() in ['.yml'],
                self.vectorized_dir.iterdir()
            ))
            citygml_files = list(filter(
                lambda file: file.suffix.lower() in ['.gml'],
                self.param_manager.citygml_dir.iterdir()
            ))

            if not self.output_dir.exists():
                self.output_dir.mkdir(parents=True)

            # CityGMLファイル毎にLOD2道路を分ける
            lod2_roads: dict[str, list] = {}

            for vectorized_file in tqdm(vectorized_files, leave=None):
                with vectorized_file.open('rt') as f:
                    roads = yaml.safe_load(f)

                for road in roads:
                    citygml_name = road['citygml']
                    if citygml_name not in lod2_roads:
                        lod2_roads[citygml_name] = []
                    lod2_roads[citygml_name].append(road)

            # LOD2道路をCityGMLファイルに追記する
            tr = Transformer.from_proj(self.param_manager.ortho_epsg, self.param_manager.citygml_epsg, always_xy=True)
            for citygml_file in tqdm(citygml_files, leave=None):
                tree = etree.parse(citygml_file, parser=None)

                if citygml_file.name not in lod2_roads:
                    Log.output_log_write(
                        LogLevel.WARN,
                        ModuleType.EXPORT_CITYGML,
                        citygml_file.name + ' lod2 road data not found, write skip')
                    continue

                for road in lod2_roads[citygml_file.name]:
                    polygon = transform(
                        from_wkt(road['geometry']),
                        lambda p: np.stack(tr.transform(p[:, 0], p[:, 1]), axis=1)
                    )

                    add_lod2_road(
                        tree,
                        polygon=polygon,
                        lod1_road_id=road['lod1_id'],
                        lod2_polygon_id=road['lod2_id'],
                        polygon_class=road['class'],
                        is_intersection=road['is_in_intersection'],
                    )

                etree.indent(tree, space='\t')
                tree.write(
                    self.output_dir / citygml_file.name,
                    pretty_print=True,
                    xml_declaration=True,
                    encoding="utf-8"
                )

            return restype

        except FileNotFoundError:
            Log.output_log_write(LogLevel.ERROR,
                                 ModuleType.EXPORT_CITYGML,
                                 'File not found')
            return ResultType.ERROR

        except Exception as e:
            Log.output_log_write(LogLevel.ERROR,
                                 ModuleType.EXPORT_CITYGML, e)
            return ResultType.ERROR
