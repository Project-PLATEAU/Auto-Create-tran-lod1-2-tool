import itertools
from PIL import Image, ImagePalette


def put_annotation_color_pallette(image: Image.Image):
    """
    アノテーション用のカラーパレットを適用する
    imageはmode=Pの必要がある
    """

    assert image.mode == 'P'

    color_palette = ImagePalette.ImagePalette(
        palette=list(itertools.chain(
            [0,   0,   0],    # none
            [128, 0,   0],    # 車道
            [0,   128, 0],    # 歩道
            [0,   0,   128],  # 島
            [128, 0,   128],  # オクルージョン
        ))
    )

    image.putpalette(color_palette, "RGB")
