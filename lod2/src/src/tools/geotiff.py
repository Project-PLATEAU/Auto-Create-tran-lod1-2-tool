from pathlib import Path
from PIL import Image
import rasterio as ras
import numpy as np
import numpy.typing as npt

Image.MAX_IMAGE_PIXELS = 1000000000

def transform_shp_to_img_coord(tfw: list[float], coord: npt.NDArray[np.float_]):
    return np.stack([
        (coord[:, 0] - tfw[4]) / tfw[0],  # x
        (coord[:, 1] - tfw[5]) / tfw[3],  # y
    ], axis=1)


def transform_img_coord_to_shp(tfw: list[float], coord: npt.NDArray[np.float_]):
    return np.stack([
        coord[:, 0] * tfw[0] + tfw[4],  # x
        coord[:, 1] * tfw[3] + tfw[5],  # y
    ], axis=1)


def read_geotiff(image_path: Path):
    img = Image.open(image_path)
    tfw = read_tfw(image_path.with_suffix('.tfw'))

    return img, tfw


def read_geotiff_ras(image_path: Path):
    img = ras.open(image_path)
    tfw = read_tfw(image_path.with_suffix('.tfw'))

    return img, tfw

def read_tfw(tfw_path: Path) -> list[float]:
    with tfw_path.open('r') as f:
        tfw = list(map(float, f.readlines()))

    return tfw


def get_bounds(image: Image.Image, tfw: list[float]) -> tuple[float, float, float, float]:
    """
    imageの範囲を求める
    (minx, miny, maxx, maxy) のタプルが返る
    """

    assert tfw[1] == 0 and tfw[2] == 0, "回転角度が0°以外のオルソ画像は非対応です"

    return (
        tfw[4],
        tfw[5] + tfw[3] * image.size[1],
        tfw[4] + tfw[0] * image.size[0],
        tfw[5],
    )

def get_bounds_ras(image: ras.open, tfw: list[float]) -> tuple[float, float, float, float]:
    """
    imageの範囲を求める
    (minx, miny, maxx, maxy) のタプルが返る
    """

    assert tfw[1] == 0 and tfw[2] == 0, "回転角度が0°以外のオルソ画像は非対応です"

    return (
        tfw[4],
        tfw[5] + tfw[3] * image.width,
        tfw[4] + tfw[0] * image.height,
        tfw[5],
    )