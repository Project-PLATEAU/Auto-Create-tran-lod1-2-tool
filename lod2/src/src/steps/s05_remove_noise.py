from pathlib import Path
from tqdm import tqdm
from PIL import Image
import numpy as np

from ..tools.raster import remove_small_area
from ..util.parammanager import ParamManager
from ..util.config import Config
from ..util.log import Log, ModuleType, LogLevel
from ..util.resulttype import ProcessResult, ResultType

class RemoveNoise:
    """ノイズ除去
    """
    def __init__(self, param_manager: ParamManager, input_dir: Path, output_dir: Path) -> None:
        """コンストラクタ

        Args:
            param_manager (ParamManager): パラメータ情報
        """
        self.param_manager = param_manager      # パラメータ情報
        self.input_dir = input_dir
        self.output_dir = output_dir

    def remove_noise(self, roadsInfo) -> None:
        try:
            restype = ResultType.SUCCESS

            input_files = list(filter(
                lambda file: file.suffix.lower() in ['.tif', '.tiff'],
                self.input_dir.iterdir()
            ))

            if self.output_dir.exists():
                Log.output_log_write(
                    LogLevel.WARN,
                    ModuleType.REMOVE_NOISE,
                    str(self.output_dir) + ' already exists')
            else:
                self.output_dir.mkdir(parents=True)

            for input_file in tqdm(input_files, leave=None):
                image = Image.open(input_file)

                image_np = np.array(image)

                noiseless_image_np = remove_small_area(
                    image_np,
                    connectivity=4,
                    threshold=self.param_manager.noise_threshold,
                    enable_tqdm=True,
                )

                output = Image.fromarray(noiseless_image_np, mode=image.mode)
                if output.mode == 'P':
                    output.putpalette(image.palette)

                output.save(self.output_dir / input_file.name)

            return restype

        except FileNotFoundError:
            Log.output_log_write(LogLevel.ERROR,
                                 ModuleType.REMOVE_NOISE,
                                 'File not found')
            return ResultType.ERROR

        except Exception as e:
            Log.output_log_write(LogLevel.ERROR,
                                 ModuleType.REMOVE_NOISE, e)
            return ResultType.ERROR
