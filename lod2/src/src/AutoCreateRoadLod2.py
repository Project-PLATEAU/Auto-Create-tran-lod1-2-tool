import sys
import shutil
import os
import glob
import datetime

# import before shapely (https://github.com/shapely/shapely/issues/1435)
import torch
from argparse import ArgumentParser
from pathlib import Path
from typing import Optional
from tqdm import tqdm
from PIL import Image

from .steps.s01_load_citygml import InputCityGmlManager
from .steps.s02_check_road_connectivity import CheckRoadConnectivity
from .steps.s03_generate_inference_input import GenerateInferenceInput
from .steps.s04_infer import Infer
from .steps.s05_remove_noise import RemoveNoise
from .steps.s06_remove_occlusion import RemoveOcclusion
from .steps.s07_vectorize import Vectorize
from .steps.s08_export_citygml import ExportCityGmlManager
from .util.parammanager import ParamManager
from .util.log import Log, ModuleType, LogLevel
from .util.config import Config
from .util.resulttype import ResultType

"""
def _delete_module_tmp_folder() -> None:
    #中間フォルダの削除(モジュールごと)
    if os.path.isdir(Config.OUTPUT_MODEL_OBJDIR):
        shutil.rmtree(Config.OUTPUT_MODEL_OBJDIR)
    if os.path.isdir(Config.OUTPUT_PHASE_OBJDIR):
        shutil.rmtree(Config.OUTPUT_PHASE_OBJDIR)
    if os.path.isdir(Config.OUTPUT_PHASE_OBJDIR):
        shutil.rmtree(Config.OUTPUT_PHASE_OBJDIR)
"""
STEPS = [
    'load_citygml',
    'check_road_connectivity',
    'generate_inference_input',
    'infer',
    'remove_noise',
    'remove_occlusion',
    'vectorize',
    'export_citygml',
]
def main():
    """メイン関数
    """
    #args = sys.argv
    #args = ["AutoCreateLod2.py", os.path.join(".", "param_kawasaki_pss_test.json")]
    #args = ["AutoCreateLod2.py", os.path.join(".", "param_kawasaki_region_0.json")]
    args = ["AutoCreateLod2.py", os.path.join(".", "param.json")]
    #args = ["AutoCreateLod2.py", os.path.join(".", "param_gifu.json")]

    if len(args) != 2:
        print('usage: python AutoCreateLod2.py param.json')
        sys.exit()

    # 中間フォルダがある場合は削除
    #if os.path.isdir(Config.OUTPUT_OBJDIR):
    #    shutil.rmtree(Config.OUTPUT_OBJDIR)

    # 中間フォルダの作成
    #os.makedirs(Config.OUTPUT_OBJDIR)

    try:
        param_manager = ParamManager()
        change_params = param_manager.read(args[1])

    except Exception as e:
        param_manager.debug_log_output = False
        log = Log(param_manager, args[1])
        log.output_log_write(LogLevel.ERROR, ModuleType.NONE, e)
        log.log_footer()
        sys.exit()

    try:
        def should_run(step_name: str) -> bool:
            """
            与えられたステップ名のステップを実行する必要がある場合はTrueを返す
            """
            start_index = STEPS.index(param_manager.startwith) if param_manager.startwith is not None else 0
            end_index = STEPS.index(param_manager.endwith) \
                if param_manager.endwith is not None else len(STEPS) - 1
            return start_index <= STEPS.index(step_name) <= end_index

        # ログクラスインスタンス化
        log = Log(param_manager, args[1])

        # パラメータがデフォルトに変更された場合
        for change_param in change_params:
            message = f'{change_param.name} Value change to '
            message += f'{change_param.value}'
            log.output_log_write(LogLevel.WARN, ModuleType.NONE,
                                 message)
        
        roads_for_summary = []

        # 処理結果初期化
        ret_citygml_read = ResultType.ERROR     
        ret_check_road_connectivity = ResultType.ERROR
        ret_generateInferenceInput = ResultType.ERROR
        ret_infer = ResultType.ERROR
        ret_remove_noise = ResultType.ERROR
        ret_remove_occlusion = ResultType.ERROR
        ret_vectorize = ResultType.ERROR

        # 入力ファイル名
        file_name = datetime.datetime.now().strftime('%Y%m%d_%H%M')

        # 中間出力フォルダ
        road_info_dir = param_manager.output_dir / '1_road_info'
        road_info_with_neighbors_dir = param_manager.output_dir / '2_road_info_with_neighbors'
        inference_input_dir = param_manager.output_dir / '3_inference_input'
        inference_result_dir = param_manager.output_dir / '4_inference_result'
        inference_result_noiseless_dir = param_manager.output_dir / '5_inference_result_noiseless'
        inference_result_without_occlusion_dir = param_manager.output_dir / '6_inference_result_without_occlusion'
        vectorized_dir = param_manager.output_dir  / '7_vectorized'
        result_dir = param_manager.output_dir / '8_lod2_output'

        """
        road_info_dir = Path('C:/Users/802700/Documents/LOD2_road/data/test/road_info')
        road_info_with_neighbors_dir = Path('C:/Users/802700/Documents/LOD2_road/data/test/road_info_with_neighbors')
        inference_input_dir = Path('C:/Users/802700/Documents/LOD2_road/data/test/inference_input')
        inference_result_dir = Path('C:/Users/802700/Documents/LOD2_road/data/test/inference_result')
        inference_result_noiseless_dir = Path('C:/Users/802700/Documents/LOD2_road/data/test/inference_result_noiseless')
        inference_result_without_occlusion_dir = Path('C:/Users/802700/Documents/LOD2_road/data/test/inference_result_without_occlusion')
        vectorized_dir = Path('C:/Users/802700/Documents/LOD2_road/data/test//vectorized')
        result_dir = Path(str(param_manager.output_dir) + '/lod2_output')
        """
        
        # 処理対象のファイル名のログを出力
        log.process_start_log(file_name)

        # 1.CityGML入力
        #if should_run('load_citygml'):
        log.module_start_log(ModuleType.LOAD_CITYGML, file_name)

        citygml = InputCityGmlManager(
            param_manager, 
            output_dir=road_info_dir
        )

        # CityGML読み込み
        ret_citygml_read, roads = citygml.load_citygml()

        log.module_result_log(
            ModuleType.LOAD_CITYGML, ret_citygml_read)
        #else:
        #    ret_citygml_read = ResultType.SUCCESS

        # 2.道路の隣接関係の判定
        if ret_citygml_read is not ResultType.ERROR:
            if should_run('check_road_connectivity'):
                log.module_start_log(
                    ModuleType.CHECK_ROAD_CONNECTIVITY, file_name)

                checkRoadConnectivity = CheckRoadConnectivity(
                    param_manager,
                    road_info_dir=road_info_dir,
                    output_dir=road_info_with_neighbors_dir,
                )
                ret_check_road_connectivity = checkRoadConnectivity.check_road_connectivity(
                    roads)

                log.module_result_log(ModuleType.CHECK_ROAD_CONNECTIVITY,
                                    ret_check_road_connectivity)
            else:
                ret_check_road_connectivity = ResultType.SUCCESS
        
        # 3.推論用データの生成
        if ret_check_road_connectivity is not ResultType.ERROR:
            if should_run('generate_inference_input'):
                log.module_start_log(
                    ModuleType.GENERATE_INFERENCE_INPUT, file_name)

                generateInferenceInput = GenerateInferenceInput(
                    param_manager,
                    road_info_dir=road_info_with_neighbors_dir,
                    output_dir=inference_input_dir
                )
                ret_generateInferenceInput = generateInferenceInput.generate_inference_input(
                    roads)

                log.module_result_log(ModuleType.GENERATE_INFERENCE_INPUT,
                                    ret_generateInferenceInput)
            else:
                ret_generateInferenceInput = ResultType.SUCCESS 

        # 4.セグメンテーション
        if ret_generateInferenceInput is not ResultType.ERROR:
            if should_run('infer'):
                log.module_start_log(
                    ModuleType.INFER, file_name)

                infer = Infer(
                    param_manager,
                    input_dir=inference_input_dir,
                    output_dir=inference_result_dir
                )
                ret_infer = infer.infer(roads)

                log.module_result_log(ModuleType.INFER,
                                    ret_infer)
            else:
                ret_infer = ResultType.SUCCESS

        # 5.ノイズ除去
        if ret_infer is not ResultType.ERROR:
            if should_run('remove_noise'):
                log.module_start_log(
                    ModuleType.REMOVE_NOISE, file_name)

                infer = RemoveNoise(
                    param_manager,
                    input_dir=inference_result_dir,
                    output_dir=inference_result_noiseless_dir,
                )
                ret_remove_noise = infer.remove_noise(roads)

                log.module_result_log(ModuleType.REMOVE_NOISE,
                                    ret_remove_noise)
            else:
                ret_remove_noise = ResultType.SUCCESS

        # 6.オクルージョンの再分類
        if ret_remove_noise is not ResultType.ERROR:
            if should_run('remove_occlusion'):
                log.module_start_log(
                    ModuleType.REMOVE_OCCLUSION, file_name)

                removeOcclusion = RemoveOcclusion(
                    param_manager,
                    input_dir=inference_result_noiseless_dir,
                    output_dir=inference_result_without_occlusion_dir
                )
                ret_remove_occlusion = removeOcclusion.remove_occlusion(roads)

                log.module_result_log(ModuleType.REMOVE_OCCLUSION,
                                    ret_remove_occlusion)
            else:
                ret_remove_occlusion = ResultType.SUCCESS

        # 7.ベクトル化
        if ret_remove_occlusion is not ResultType.ERROR:
            if should_run('vectorize'):
                log.module_start_log(
                    ModuleType.VECTORIZE, file_name)

                vectorize = Vectorize(
                    param_manager,
                    input_dir=inference_result_without_occlusion_dir,
                    road_info_dir=road_info_with_neighbors_dir,
                    output_dir=vectorized_dir,
                    output_shape_dir=result_dir
                )
                ret_vectorize = vectorize.vectorize(roads)

                log.module_result_log(ModuleType.VECTORIZE,
                                    ret_vectorize)
            else:
                ret_vectorize = ResultType.SUCCESS

            # summary用にモデル化結果を保存
            roads_for_summary.extend(roads)

        # 8.CityGML出力
        if ret_vectorize is not ResultType.ERROR and param_manager.input_type == 1:
            if should_run('export_citygml'):
                log.module_start_log(
                    ModuleType.EXPORT_CITYGML, file_name)

                exportCityGml = ExportCityGmlManager(
                    param_manager,
                    vectorized_dir=vectorized_dir,
                    output_dir=result_dir
                )
                ret_export_citygml = exportCityGml.export_citygml(roads)

                log.module_result_log(ModuleType.EXPORT_CITYGML,
                                    ret_export_citygml)
            else:
                ret_export_citygml = ResultType.SUCCESS

            # 中間フォルダの削除(モジュールごと)
            #_delete_module_tmp_folder()

        # 中間フォルダの削除(temp)
        #if os.path.isdir(Config.OUTPUT_OBJDIR):
        #    shutil.rmtree(Config.OUTPUT_OBJDIR)

    except Exception as e:
        log.output_log_write(LogLevel.ERROR, ModuleType.NONE, e)
        roads_for_summary.extend(roads)

    finally:
        if ret_citygml_read is not ResultType.ERROR:
            # モデル化結果サマリー出力
            log.output_summary(roads_for_summary)

        # 実行ログファイルと標準出力にフッタ出力
        log.log_footer()
