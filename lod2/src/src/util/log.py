import inspect
import os
import shutil
import datetime
import json
import logging

from .parammanager import ParamManager
from .config import Config
from .resulttype import ResultType, ProcessResult
from enum import IntEnum
from logging import getLogger, config


class ModuleType(IntEnum):
    """モジュール情報
    """
    LOAD_CITYGML = 0                # CityGML入力
    CHECK_ROAD_CONNECTIVITY = 1     # 道路の隣接関係の判定
    GENERATE_INFERENCE_INPUT = 2    # 推論用データの生成
    INFER = 3                       # セグメンテーション
    REMOVE_NOISE = 4                # ノイズ除去
    REMOVE_OCCLUSION = 5            # オクルージョンの再分類
    VECTORIZE = 6                   # ベクトル化
    EXPORT_CITYGML = 7              # CityGML出力
    NONE = 8                        # モジュール不明


class LogLevel(IntEnum):
    """ログレベル
    """
    ERROR = 50              # エラー
    MODEL_ERROR = 40        # モデルエラー
    WARN = 30               # 警告
    INFO = 20               # お知らせ
    DEBUG = 10              # デバッグ


class Singleton(object):
    def __new__(cls, *args, **kargs):
        if not hasattr(cls, "_instance"):
            cls._instance = super(Singleton, cls).__new__(cls)
        return cls._instance


class Log(Singleton):
    """ログクラス
        シングルトン
    """
    log_conf = []
    _MAIN_LOG_FILE_PATH = 'main_log.txt'
    _main_log_file = []           # 実行ログ
    _module_log_file = ['', '', '', '', '', '', '', '']     # モジュールログ
    _standard_log = []              # 標準出力
    FORMAT_ON = 0                   # フォーマット設定あり
    FORMAT_OFF = 1                  # フォーマット設定なし
    # モジュール情報
    MODULE_LIST = {ModuleType.LOAD_CITYGML:
                   ["LoadCitygml", "load_citygml_log.txt"],
                   ModuleType.CHECK_ROAD_CONNECTIVITY :
                   ["CheckRoadConnectivity",
                    "check_road_connectivity_log.txt"],
                   ModuleType.GENERATE_INFERENCE_INPUT :
                   ["GenerateInferenceInput",
                    "generate_inference_input_log.txt"],
                   ModuleType.INFER:
                   ["Infer", "infer_log.txt"],
                   ModuleType.REMOVE_NOISE:
                   ["RemoveNoise", "remove_noise_log.txt"],
                   ModuleType.REMOVE_OCCLUSION:
                   ["RemoveOcclusion", "remove_occlusion_log.txt"],
                   ModuleType.VECTORIZE:
                   ["Vectorize", "vectorize_log.txt"],
                   ModuleType.EXPORT_CITYGML:
                   ["ExportCitygml", "export_citygml_log.txt"]
                   }
    RESULT_MESSAGE = ['SUCCESS', 'WARNING', 'ERROR']    # モジュール実行結果メッセージ
    debug_flag = False          # DEBUGログを出力するかのフラグ

    def __init__(self, param: ParamManager, param_file):
        """ログクラスコンストラクタ
            ロガー作成、ヘッダ部分出力
            モジュールごとのログファイルに開始ログ出力

        Args:
            param (ParamManager): パラメータ情報
            param_file: パラメータファイルパス
        """
        # パラメータ情報からの読み込み情報
        log_folder_path = param.output_log_dir    # ログ出力先
        Log.debug_flag = param.debug_log_output     # デバッグログ出力フラグ

        # 出力フォルダ等に記載する時刻を揃えるためParamManagerで取得した時刻を使用する
        create_time = param.time.strftime("%Y%m%d_%H%M%S")
        time_log_folder = f'outputlog_{create_time}'
        
        # 環境設定ファイルパス取得
        config_file = os.path.join(
            os.path.dirname(__file__), 'log_config.json')
        # 環境設定用の辞書作成
        with open(config_file, 'r') as f:
            Log.log_conf = json.load(f)
        
        # ログレベル設定
        logging.addLevelName(LogLevel.MODEL_ERROR, 'MODEL_ERROR')
        logging.addLevelName(LogLevel.ERROR, 'ERROR')

        path_err = False
        try:
            if not os.path.isdir(log_folder_path):
                os.makedirs(log_folder_path)
            
            output_log_folder_path = os.path.join(
                log_folder_path, time_log_folder)

        except Exception:
            path_err = True
            output_log_folder_path = os.path.join(
                'output_log', time_log_folder)

        Log.output_log_folder_path = output_log_folder_path

        # フォルダが存在する場合は削除
        if os.path.isdir(Log.output_log_folder_path):
            shutil.rmtree(Log.output_log_folder_path)

        os.makedirs(Log.output_log_folder_path)

        # 実行ログファイルパス作成
        main_log_path = os.path.join(
            Log.output_log_folder_path, Log._MAIN_LOG_FILE_PATH)

        # 既に出力ファイルがあったら削除
        if os.path.isfile(main_log_path):
            os.remove(main_log_path)

        # 実行ログファイル出力先設定
        handlers = Log.log_conf['handlers']
        handlers['MainLogFile']['filename'] = main_log_path
        handlers['MainLogFileNoForm']['filename'] = main_log_path

        # logger環境設定
        config.dictConfig(Log.log_conf)

        # 実行ログと標準出力のロガー作成
        Log._standard_log.append(getLogger('Console'))
        Log._standard_log.append(getLogger('ConsoleNoForm'))
        Log._main_log_file.append(getLogger('MainLogFile'))
        Log._main_log_file.append(getLogger('MainLogFileNoForm'))

        # ヘッダ出力
        self.__log_header(Log._main_log_file[Log.FORMAT_OFF], param_file)
        self.__log_header(Log._standard_log[Log.FORMAT_OFF], param_file)

        if path_err:
            message = f'OutputLogFolderPath Value change "{log_folder_path}"'
            message += ' to "output_log"'
            self.output_log_write(LogLevel.WARN, ModuleType.NONE, message)
 
    def __log_header(self, logger, param_file):
        """実行ログファイルヘッダ出力

        Args:
            logger: ログ出力先
            param_file: パラメータファイルパス
        """
        self._start_time = datetime.datetime.now()      # 実行開始時間

        # 実行ログファイルへのヘッダ出力
        logger.info('AutoCreateRoadLod2')
        logger.info(f'Version : {Config.SYSYTEM_VERSION}')
        logger.info(f'Start Time : {self._start_time}\n')
        logger.info('Module Information List')

        for module_type in Log.MODULE_LIST:
            # モジュール情報出力
            module = Log.MODULE_LIST[module_type]
            logger.info(f'{module[0]} Module')
            logger.info(f'LogFileName : {module[1]}')

        logger.info(f'\nInput Parameter File Path : {param_file}')
        logger.info(f'DebugFlag : {Log.debug_flag}')

    def log_footer(self):
        """実行ログファイルフッタ出力
        """
        # 終了日時
        end_time = datetime.datetime.now()
        # 実行時間
        process_time = end_time - self._start_time

        # 実行ログファイル出力
        Log._main_log_file[Log.FORMAT_OFF].info(
            f'\nEnd Time : {end_time}')
        Log._main_log_file[Log.FORMAT_OFF].info(
            f'Process Time: {process_time}')

        # 標準出力
        Log._standard_log[Log.FORMAT_OFF].info(
            f'\nEnd Time : {end_time}')
        Log._standard_log[Log.FORMAT_OFF].info(
            f'Process Time: {process_time}')

        # ログファイル操作終了
        logging.shutdown()

        # モジュールロガーリストのリセット
        Log._module_log_file = ['', '', '', '', '', '', '', '']

    @classmethod
    def module_result_log(self, module: ModuleType, result: ResultType):
        """モジュールの実行結果ログ出力
        
        Args:
            module (ModuleType): モジュール情報
            result (ResultType): モジュール実行結果
        """
        # モジュールの実行結果メッセージ取得
        message = Log.RESULT_MESSAGE[result]

        # 実行結果出力メッセージ作成
        module_name = f'{Log.MODULE_LIST[module][0]} Module'
        message = f'{module_name} : Result : {message}'
        Log._main_log_file[Log.FORMAT_ON].info(message)

        # モジュール実行終了ログ出力
        Log._main_log_file[Log.FORMAT_ON].info(f'{module_name} End\n')
        Log._module_log_file[module].info(f'{module_name} End')

        # 標準出力にログ出力
        Log._standard_log[Log.FORMAT_ON].info(message)
        Log._standard_log[Log.FORMAT_ON].info(f'{module_name} End\n')
        
    def __create_logger(module: ModuleType):
        """ロガー作成

        Args:
            module (ModuleType): モジュール情報
        """
        # モジュールタイプがNONE以外はログファイル作成
        if module != ModuleType.NONE and Log._module_log_file[module] == "":
            # モジュールログ出力用のロガー作成
            Log._module_log_file[module] = getLogger(
                f'{Log.MODULE_LIST[module][0]}Log')

            # モジュールログファイルパス作成
            module_log_file_path = os.path.join(
                Log.output_log_folder_path, Log.MODULE_LIST[module][1])

            # 既に出力ファイルが存在していたら削除
            if os.path.isfile(module_log_file_path):
                os.remove(module_log_file_path)

            # ロガーの出力先設定
            fh = logging.FileHandler(module_log_file_path, encoding='utf-8')

            if not Log.debug_flag:
                # 出力ログレベル設定
                fh.setLevel(logging.INFO)

            # ロガーフォーマット設定
            fmt = logging.Formatter(Log.log_conf
                                    ['formatters']['Versatility']['format'])
            fh.setFormatter(fmt)
            Log._module_log_file[module].addHandler(fh)

    @classmethod
    def output_log_write(self, level: LogLevel, module: ModuleType,
                         message=None, standard_flag=False):
        """ログ出力
            モジュールごとのログファイルに出力

        Args:
            module (ModuleType): モジュール情報
            level : ログレベル情報
            message: ログメッセージ
            standard_flag: 標準出力するかのフラグ情報
        """
        if module is not ModuleType.NONE:
            # 出力ログメッセージ作成
            module_name = f'{Log.MODULE_LIST[module][0]} Module'
            message = f'{module_name} : {message}'
            if Log.debug_flag and level >= LogLevel.WARN:
                caller = '\n     [DEBUG] : Caller : relative path = '
                caller += f'{os.path.relpath(inspect.stack()[1].filename)}, '
                caller += f'function = {inspect.stack()[1].function}, '
                caller += f'line = {inspect.stack()[1].lineno}'
                message += caller

            # 標準出力にログ出力
            if standard_flag:
                Log._standard_log[Log.FORMAT_ON].log(level, message)

            Log._module_log_file[module].log(level, message)
        else:
            # 実行ログファイルと標準出力にログ出力
            Log._main_log_file[Log.FORMAT_ON].log(level, message)
            Log._standard_log[Log.FORMAT_ON].log(level, message)

    @classmethod
    def module_start_log(self, module: ModuleType, citygml_filename: str = ''):
        """実行ログ、標準出力、モジュールログへのモジュール実行開始のログ出力
            モジュールログ出力用のロガーを作成
        
        Args:
            module (ModuleType): モジュール情報
            citygml_filename (str, optional): 処理対象ファイル名. Defaults to ''.
        """
        # モジュール名取得
        module_name = f'{Log.MODULE_LIST[module][0]} Module'

        # 実行ログファイルログ出力
        Log._main_log_file[Log.FORMAT_ON].info(f'{module_name} Run')
    
        # 標準出力にログ出力
        Log._standard_log[Log.FORMAT_ON].info(f'{module_name} Run')

        # ロガー作成
        self.__create_logger(module)

        # モジュールログファイルに開始ログ出力
        Log._module_log_file[module].info(
            '--------------------------------------')
        Log._module_log_file[module].info(
            f'start processing {citygml_filename}')
        Log._module_log_file[module].info(
            f'{Log.MODULE_LIST[module][0]} Module Run')

    @classmethod
    def process_start_log(self, citygml_filename: str = ''):
        """実行ログ、標準出力に処理対象のCityGMLファイル名のログを出力する

        Args:
            citygml_filename (str, optional): 処理対象ファイル名. Defaults to ''.
        """
        # 実行ログファイルログ出力
        Log._main_log_file[Log.FORMAT_OFF].info(
            '--------------------------------------')
        Log._main_log_file[Log.FORMAT_ON].info(
            f'{citygml_filename} processing')

        # 標準出力にログ出力
        print('--------------------------------------')
        Log._standard_log[Log.FORMAT_ON].info(
            f'start processing {citygml_filename}')

    def output_summary(self, roadsInfo):
        """モデル作成結果csv出力

        Args:
            buildings (list[CityGmlManager.RoadInfo]): 道路情報リスト
        """
        now_time = datetime.datetime.now()
        create_result = getLogger('Summary')

        # モジュールログファイルパス作成
        file_path = os.path.join(
            Log.output_log_folder_path, 'road_create_result.csv')

        # 既に出力ファイルが存在していたら削除
        if os.path.isfile(file_path):
            os.remove(file_path)

        # ロガーの出力先設定
        fh = logging.FileHandler(file_path, encoding='utf_8_sig')

        if not Log.debug_flag:
            # 出力ログレベル設定
            fh.setLevel(logging.INFO)

        # ロガーフォーマット設定
        fmt = logging.Formatter('%(message)s')
        fh.setFormatter(fmt)
        create_result.addHandler(fh)
        
        # ヘッダ部分出力
        time = f'{now_time.year}/{now_time.month}/{now_time.day} '
        time += f'{now_time.hour}:{now_time.minute}:{now_time.second}'

        create_result.info(f'{time}')
        create_result.info('\n[最終結果]')
        create_result.info('SUCCESS: 処理済')
        create_result.info('WARNING: 未処理')
        create_result.info('ERROR: エラー')

        create_result.info('\n[詳細項目]')
        # 項目説明用の出力フォーマット
        RESULT_ITEMS = [['道路LOD1データ入力', '〇: 入力に成功',
                         '×: 入力に失敗',
                         '-: 未処理'],
                        ['\n道路の隣接関係の判定', '〇: 処理に成功',
                         '×:処理に失敗',
                         '-: 未処理'],
                        ['\nベクトル化', '〇: 処理に成功',
                         '×:処理に失敗',
                         '-: 未処理'],
        ]


        # 項目説明出力
        for i in range(len(RESULT_ITEMS)):
            for j in RESULT_ITEMS[i]:
                create_result.info(j)
        
        # 項目結果のメッセージ
        PROCESS_RESULT_MESSAGE = [',〇', ',×', ',-']
        
        # モデル化結果項目出力
        create_result.info('\nNo,ファイル名,道路ID,最終結果,'
                           '道路LOD1データ入力結果,道路の隣接関係の判定結果,'
                           'ベクトル化結果')

        row_count = 0       # 行番号

        # モデル化結果サマリー出力
        for road in roadsInfo:
            # モデル化結果の最終結果判定
            if road.vectorize == ProcessResult.SUCCESS:
                road.create_result = ResultType.SUCCESS
            else:
                road.create_result = ResultType.WARN
            row_count += 1
            message = f'{row_count},{road.citygml_filename},{road.road_id}'
            message += f',{Log.RESULT_MESSAGE[road.create_result]}'
            message += PROCESS_RESULT_MESSAGE[road.load_citygml]
            message += PROCESS_RESULT_MESSAGE[road.check_road_connectivity]
            # message += PROCESS_RESULT_MESSAGE[road.generate_inference_input]
            # message += PROCESS_RESULT_MESSAGE[road.infer]
            # message += PROCESS_RESULT_MESSAGE[road.remove_noise]
            # message += PROCESS_RESULT_MESSAGE[road.remove_occlusion]
            message += PROCESS_RESULT_MESSAGE[road.vectorize]
            create_result.info(message)
