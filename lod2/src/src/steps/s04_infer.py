from pathlib import Path
import numpy as np
import numpy.typing as npt
from tqdm import tqdm
from PIL import Image
import torch
from mmseg.structures import SegDataSample
from os.path import join, dirname

from ..tools.model import SegmentorPLModule
from ..tools.color import put_annotation_color_pallette
from ..util.parammanager import ParamManager
from ..util.config import Config
from ..util.log import Log, ModuleType, LogLevel
from ..util.resulttype import ProcessResult, ResultType

class Infer:
    """推論を実行する
    """
    def __init__(self, param_manager: ParamManager, input_dir: Path, output_dir: Path) -> None:
        """コンストラクタ

        Args:
            param_manager (ParamManager): パラメータ情報
        """
        self.param_manager = param_manager      # パラメータ情報
        self.input_dir = input_dir
        self.output_dir = output_dir

    def infer_tile(
        self,
        model: torch.nn.Module,
        image: npt.NDArray[np.uint8],
        device: str = "cuda"
    ) -> npt.NDArray[np.uint8]:
        """
        推論を実行する
        imageのshapeは(3, H, W)
        outputのshapeは(H, W)
        """
        with torch.no_grad():
            image = torch.from_numpy(image).float().unsqueeze(0).to(device)
            data_sample = SegDataSample()
            data_sample.set_metainfo({"img_shape": image.shape[-2:]})
            inputs = {
                "inputs": image,
                "data_samples": [data_sample],
            }
            logits = model(inputs)
        return (logits.cpu().squeeze(0).argmax(dim=0) + 1).numpy().astype(np.uint8)


    def infer(self, roadsInfo) -> None:

        try:
            restype = ResultType.SUCCESS
            input_ortho_files = list(filter(
                lambda file:
                file.suffix.lower() in ['.tif', '.tiff'] and '_mask' not in file.name,
                self.input_dir.iterdir()
            ))

            dir_name = dirname(__file__)
            checkpoint_path = join(
                dir_name, 'data', 'epoch136-step67130-miou0.705.ckpt')

            model = SegmentorPLModule.load_from_checkpoint(
                checkpoint_path,
                map_location=self.param_manager.device
            )
            model.eval()

            if self.output_dir.exists():
                Log.output_log_write(
                    LogLevel.WARN,
                    ModuleType.INFER,
                    str(self.output_dir) + ' already exists')
            else:
                self.output_dir.mkdir(parents=True)

            for ortho_file in tqdm(input_ortho_files, leave=None):
                mask_file = ortho_file.with_stem(ortho_file.stem + '_mask')
                assert mask_file.is_file()

                ortho = np.array(Image.open(ortho_file))
                mask = np.array(Image.open(mask_file))
                result = np.zeros(ortho.shape[:2], dtype=np.uint8)

                h, w = ortho.shape[:2]
                step = self.param_manager.inference_size - self.param_manager.inference_padding * 2

                for i in tqdm(range((h + step - 1) // step), leave=None):
                    for j in range((w + step - 1) // step):
                        upper = min(i * step, h - self.param_manager.inference_size)
                        bottom = upper + self.param_manager.inference_size
                        left = min(j * step, w - self.param_manager.inference_size)
                        right = left + self.param_manager.inference_size

                        ortho_tile = ortho[upper:bottom, left:right]

                        result_tile = self.infer_tile(
                            model,
                            np.moveaxis(ortho_tile, -1, 0),
                            self.param_manager.device
                        )

                        # 端の画像の場合のみpadding分も結果として含める
                        result_upper = upper + self.param_manager.inference_padding if upper != 0 else 0
                        result_bottom = bottom - self.param_manager.inference_padding if bottom != h else h
                        result_left = left + self.param_manager.inference_padding if left != 0 else 0
                        result_right = right - self.param_manager.inference_padding if right != w else w

                        result[
                            result_upper:result_bottom,
                            result_left:result_right
                        ] = result_tile[
                            result_upper-upper:result_bottom-upper,
                            result_left-left:result_right-left,
                        ]

                result[mask == 0] = 0

                output = Image.fromarray(result, 'P')
                put_annotation_color_pallette(output)

                output.save(self.output_dir / ortho_file.name)

            return restype

        except FileNotFoundError:
            Log.output_log_write(LogLevel.ERROR,
                                 ModuleType.INFER,
                                 'File not found')
            return ResultType.ERROR

        except Exception as e:
            Log.output_log_write(LogLevel.ERROR,
                                 ModuleType.INFER, e)
            return ResultType.ERROR

