from typing import Any, Optional
import torch
import lightning.pytorch as pl
from mmseg.registry import MODELS
from mmengine import Config
from mmengine.registry import init_default_scope


class SegmentorPLModule(pl.LightningModule):
    def __init__(
        self,
        default_scope: Optional[str] = None,
        backbone_cfg: Optional[dict[str, Any]] = None,
        decode_head_cfg: Optional[dict[str, Any]] = None,
    ) -> None:
        super(SegmentorPLModule, self).__init__()
        self.save_hyperparameters()

        backbone_cfg = Config(backbone_cfg)
        decode_head_cfg = Config(decode_head_cfg)

        init_default_scope(default_scope)
        self.backbone = MODELS.build(backbone_cfg)
        self.decode_head = MODELS.build(decode_head_cfg)

    def forward(self, batch) -> torch.Tensor:
        feature_map = self.backbone(batch["inputs"])
        batch_img_metas = [ds.metainfo for ds in batch["data_samples"]]
        return self.decode_head.predict(feature_map, batch_img_metas, None)
