from pathlib import Path
import numpy as np
from tqdm import tqdm
import yaml
from PIL import Image, ImageDraw
import rasterio as ras

from ..tools.geotiff import transform_shp_to_img_coord
from ..util.parammanager import ParamManager
from ..util.config import Config
from ..util.log import Log, ModuleType, LogLevel
from ..util.resulttype import ProcessResult, ResultType

class GenerateInferenceInput:
    """推論の入力として与えるラスタ画像を生成する
    """
    def __init__(self, param_manager: ParamManager, road_info_dir: Path, output_dir: Path) -> None:
        """コンストラクタ

        Args:
            param_manager (ParamManager): パラメータ情報
        """
        self.param_manager = param_manager      # パラメータ情報
        self.road_info_dir = road_info_dir
        self.output_dir = output_dir

    def generate_inference_input(self, roadsInfo) -> None:
        """
        推論の入力として与えるラスタ画像を生成する
        """
        try:
            restype = ResultType.SUCCESS

            if self.output_dir.exists():
                Log.output_log_write(
                    LogLevel.WARN,
                    ModuleType.GENERATE_INFERENCE_INPUT,
                    str(self.output_dir) + ' already exists')
            else:
                self.output_dir.mkdir(parents=True)

            road_info_files = list(filter(
                lambda file: file.suffix.lower() in ['.yaml', '.yml'],
                self.road_info_dir.iterdir()
            ))

            for road_info_file in tqdm(road_info_files, leave=None):
                with road_info_file.open('rt') as f:
                    road_info = yaml.safe_load(f)

                ortho_file_name: str = road_info['ortho']
                ortho_size: tuple[int, int] = tuple(road_info['size'])
                ortho_tfw: list[float] = road_info['tfw']

                # 道路の領域を1としたマスク画像を作成する
                mask = Image.new('1', ortho_size)
                draw = ImageDraw.Draw(mask)

                for road in road_info['roads']:
                    polygon_pixel = transform_shp_to_img_coord(
                        ortho_tfw,
                        np.array(road['polygon'])
                    )

                    draw.polygon(
                        np.ravel(np.round(polygon_pixel)).tolist(),
                        fill=1
                    )

                # オルソ画像
                #ortho = Image.open(self.param_manager.ortho_dir / ortho_file_name)
                ortho_ras = ras.open(self.param_manager.ortho_dir / ortho_file_name)

                # if blackout:
                # 道路外を黒塗りする
                #ortho_np = np.array(ortho)
                ortho_np_ras = ortho_ras.read()
                ortho_np_ras = np.stack((ortho_np_ras[0], ortho_np_ras[1], ortho_np_ras[2]), axis=-1)

                ortho_np_ras[np.array(mask) == 0] = 0
                ortho_ras = Image.fromarray((ortho_np_ras*1).astype(np.uint8)).convert('RGB')
                #ortho_ras = Image.fromarray(ortho_np_ras, mode=ortho_ras.mode)

                ortho_output_path = self.output_dir / ortho_file_name

                ortho_ras.save(ortho_output_path)
                mask.save(
                    ortho_output_path.with_stem(ortho_output_path.stem + "_mask")
                )

            return restype

        except FileNotFoundError:
            Log.output_log_write(LogLevel.ERROR,
                                 ModuleType.GENERATE_INFERENCE_INPUT,
                                 'File not found')
            return ResultType.ERROR

        except Exception as e:
            Log.output_log_write(LogLevel.ERROR,
                                 ModuleType.GENERATE_INFERENCE_INPUT, e)
            return ResultType.ERROR
