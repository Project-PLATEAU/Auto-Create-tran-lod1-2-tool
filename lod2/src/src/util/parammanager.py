import json
import os
import datetime
import pathlib
from pathlib import Path
from typing import Optional

class ParamManager:
    """ パラメータファイル管理クラス
    """

    class ChangeParam:
        """エラー値が入力された場合にデフォルト値に変更したパラメータ通知用クラス

        Returns:
            _type_: _description_
        """
        @property
        def name(self):
            """パラメータ名

            Returns:
                string: パラメータ名
            """
            return self._name

        @property
        def value(self):
            """デフォルト設定値

            Returns:
                Any: デフォルト設定値(パラメータによって値が異なる)
            """
            return self._value

        def __init__(self, name: str, value) -> None:
            """コンストラクタ

            Args:
                name (string): パラメータ名
                value (Any): 設定値
            """
            self._name = name
            self._value = value

    # クラス変数
    # jsonファイルのキー
    KEY_ORTHO_DIR = 'OrthoDir'
    KEY_INPUT_TYPE = 'InputType'
    KEY_CITHGML_DIR = 'CityGMLDir'
    KEY_SHAPE_DIR = 'ShapeDir'
    KEY_OUTPUT_DIR = 'OutputDir'
    KEY_ORTHO_EPSG = 'OrthoEpsg'
    KEY_CITYGML_EPSG = 'CityGmlEpsg'
    KEY_DEVICE = 'Device'
    KEY_INFERENCE_SIZE = 'InferenceSize'
    KEY_INFERENCE_PADDING = 'InferencePadding'
    KEY_NOISE_THRESHOLD = 'NoiseThreshold'
    KEY_STARTWITH = 'StartWith'
    KEY_ENDWITH = 'EndWith'
    KEY_OUTPUT_LOG_DIR = 'OutputLogDir'
    KEY_DEBUG_LOG_OUTPUT = 'DebugLogOutput'

    # jsonファイルキーリスト
    KEYS = [
        KEY_ORTHO_DIR,
        KEY_INPUT_TYPE,
        KEY_CITHGML_DIR,
        KEY_SHAPE_DIR,
        KEY_OUTPUT_DIR,
        KEY_ORTHO_EPSG,
        KEY_CITYGML_EPSG,
        KEY_DEVICE,
        KEY_INFERENCE_SIZE,
        KEY_INFERENCE_PADDING,
        KEY_NOISE_THRESHOLD,
        KEY_STARTWITH,
        KEY_ENDWITH,
        KEY_OUTPUT_LOG_DIR,
        KEY_DEBUG_LOG_OUTPUT]

    # デバッグログ設定のデフォルト値
    DEFALT_DEBUG_LOG_OUTPUT = False

    # 任意項目のデフォルト値
    # 推論を実行するデバイスのデフォルト値
    DEFALT_DEVICE = "cpu"
    # 推論を行う際のタイルの1辺のサイズのデフォルト値
    DEFALT_INFERENCE_SIZE = 256
    # 推論結果を使用しない外側の幅のデフォルト値
    DEFALT_INFERENCE_PADDING = 64
    # 除去する領域のピクセル数の閾値のデフォルト値
    DEFALT_NOISE_THRESHOLD = 200
    # 途中から実行したい場合の開始ステップ名のデフォルト値
    DEFALT_STARTWITH = None
    # 途中で実行を止めたい場合の最後のステップ名のデフォルト値
    DEFALT_ENDWITH = None

    def __init__(self) -> None:
        """ コンストラクタ
        """
        self.ortho_dir = ''             # オルソ画像ディレクトリパス
        self.input_type = 0             # 入力形式(1:CityGMLファイル 2:シェープファイル)
        self.citygml_dir = ''           # LOD1 CityGMLディレクトリパス
        self.shape_dir = ''             # LOD1 シェープファイルディレクトリパス
        self.output_dir = ''            # 途中結果と最終結果を出力するディレクトリパス
        self.ortho_epsg = 0             # オルソ画像のワールドファイルに記載された座標のEPSGコード
        self.citygml_epsg = 0           # CityGMLに記載された座標のEPSGコード
        # 推論を実行するデバイス【任意】
        self.device \
            = ParamManager.DEFALT_DEVICE
        # 推論を行う際のタイルの1辺のサイズ【任意】
        self.inference_size \
            = ParamManager.DEFALT_INFERENCE_SIZE
        # 推論結果を使用しない外側の幅【任意】
        self.inference_padding \
            = ParamManager.DEFALT_INFERENCE_PADDING
        # 除去する領域のピクセル数の閾値【任意】
        self.noise_threshold \
            = ParamManager.DEFALT_NOISE_THRESHOLD
        # 途中から実行したい場合の開始ステップ名【任意】
        self.startwith: Optional[str]
        self.startwith \
            = ParamManager.DEFALT_STARTWITH
        # 途中で実行を止めたい場合の最後のステップ名【任意】
        self.endwith: Optional[str]
        self.endwith \
            = ParamManager.DEFALT_ENDWITH
        # ログのフォルダパス
        self.output_log_dir = ''
        # デバッグログ出力フラグ
        self.debug_log_output \
            = ParamManager.DEFALT_DEBUG_LOG_OUTPUT

        # 作業用パラメータ
        self.time = datetime.datetime.now()     # 処理開始時刻

    def read(self, file_path) -> list[ChangeParam]:
        """jsonファイル読み込み関数

        Args:
            file_path (string): jsonファイルパス

        Raises:
            FileNotFoundError: filePathで指定されたファイルが存在しない
            Exception: ファイル/フォルダパスが文字列ではない場合,または空文字の場合

        Returns:
            list[ChangeParam]: 入力エラーによりデフォルト値を採用したパラメータリスト
        """

        change_params = []  # デフォルト値に変更したパラメータリスト

        if not os.path.isfile(file_path):
            # ファイルが存在しない場合
            raise FileNotFoundError('parameter file does not exist.')

        # ファイルが存在する場合
        with open(file_path, encoding='utf-8', mode='r') as jsonOpen:
            try:
                jsonLoad = json.load(jsonOpen)
            except json.decoder.JSONDecodeError as e:
                # 未記入項目がある場合にデコードエラーが発生する
                r = e.lineno
                c = e.colno
                raise (Exception(
                    f'json file decoding error: {e.msg} line {r} column {c}.'))

            # キーの確認
            for key in ParamManager.KEYS:
                if key not in jsonLoad:
                    # キーがない場合エラーとする
                    raise ValueError(f'{key} key does not exist in json file.')

            # 値の取得
            self.ortho_dir = Path(jsonLoad[self.KEY_ORTHO_DIR])
            self.input_type = jsonLoad[self.KEY_INPUT_TYPE]
            self.citygml_dir = Path(jsonLoad[self.KEY_CITHGML_DIR])
            self.shape_dir = Path(jsonLoad[self.KEY_SHAPE_DIR])
            self.output_dir = Path(jsonLoad[self.KEY_OUTPUT_DIR])
            self.ortho_epsg = jsonLoad[self.KEY_ORTHO_EPSG]
            self.citygml_epsg = jsonLoad[self.KEY_CITYGML_EPSG]
            self.device = jsonLoad[self.KEY_DEVICE]
            self.inference_size = jsonLoad[self.KEY_INFERENCE_SIZE]
            self.inference_padding = jsonLoad[self.KEY_INFERENCE_PADDING]
            self.noise_threshold = jsonLoad[self.KEY_NOISE_THRESHOLD]
            self.startwith = jsonLoad[self.KEY_STARTWITH]
            self.endwith = jsonLoad[self.KEY_ENDWITH]
            self.output_log_dir = Path(jsonLoad[self.KEY_OUTPUT_LOG_DIR])
            self.debug_log_output = jsonLoad[self.KEY_DEBUG_LOG_OUTPUT]

        # オルソ画像ディレクトリパス
        if self.ortho_dir == Path(""):
            # 文字列ではない or 空文字の場合
            raise Exception(ParamManager.KEY_ORTHO_DIR + ' is invalid.')

        if not os.path.exists(self.ortho_dir):
            # オルソ画像ディレクトリが存在しない場合
            raise Exception(
                ParamManager.KEY_ORTHO_DIR + ' not found.')

        # 入力ファイルタイプ
        if (type(self.input_type) is not int
                and type(self.input_type) is not float):
            raise Exception(ParamManager.KEY_INPUT_TYPE + ' is invalid.')
        
        if (self.input_type != 1) and (self.input_type != 2):
            raise Exception(ParamManager.KEY_INPUT_TYPE + ' not support.')       

        # 入力CityGMLフォルダとシェープファイル
        if self.input_type == 1:
            if self.citygml_dir == Path(""):
                # 入力CityGMLフォルダとシェープファイルどちらも文字列ではない or 空文字の場合
                raise Exception(
                    ParamManager.KEY_CITHGML_DIR + ' is invalid.')

            if (not os.path.isdir(self.citygml_dir)):
                # 入力CityGMLフォルダとシェープファイルどちらも存在しない場合
                raise Exception(
                    ParamManager.KEY_CITHGML_DIR + ' not found.')
            
        # 入力シェープファイル
        if self.input_type == 2:
            if self.shape_dir == Path(""):
                # 入力CityGMLフォルダとシェープファイルどちらも文字列ではない or 空文字の場合
                raise Exception(
                    ParamManager.KEY_SHAPE_DIR + ' is invalid.')

            if (not os.path.isdir(self.shape_dir)):
                # 入力CityGMLフォルダとシェープファイルどちらも存在しない場合
                raise Exception(
                    ParamManager.KEY_SHAPE_DIR + ' not found.')

        # 途中結果と最終結果とログを出力するディレクトリパス
        if self.output_dir == Path(""):
            # 文字列ではない or 空文字の場合
            raise Exception(
                ParamManager.KEY_OUTPUT_DIR + ' is invalid.')

        if self.input_type == 1:
            input_folder_name = "CityGMLFolder"
        elif self.input_type == 2:
            input_folder_name = "ShapeFolder"
        time_str = self.time.strftime('%Y%m%d_%H%M')
        output_dir = os.path.join(
            self.output_dir, f'{input_folder_name}_{time_str}')
        if not os.path.exists(output_dir):
            # CityGML出力ファイルの格納フォルダがない場合
            try:
                os.makedirs(output_dir)
                self.output_dir = Path(output_dir)    # 更新
            except Exception:
                raise Exception(
                    output_dir + ' cannot make.')

        if not self.output_log_dir:
            # 文字列ではない or 空文字の場合
            self.output_log_dir = None
            # エラー対応(途中終了)
            raise Exception(
                ParamManager.KEY_OUTPUT_LOG_DIR + ' is invalid.')
        else:
            try:
                # 作成不能なパスであるかの確認
                os.makedirs(self.output_log_dir, exist_ok=True)
            except Exception:
                self.output_log_dir = None
                # エラー対応(途中終了)
                raise Exception(
                    ParamManager.KEY_OUTPUT_LOG_DIR + ' is invalid.')

        # オルソ画像のワールドファイルに記載された座標のEPSGコード
        if type(self.ortho_epsg) is not int:
            raise Exception(
                ParamManager.KEY_ORTHO_EPSG + ' is invalid.')

        # CityGMLに記載された座標のEPSGコード
        if type(self.citygml_epsg) is not int:
            raise Exception(
                ParamManager.KEY_CITYGML_EPSG + ' is invalid.')

        # 推論を実行するデバイス【任意】
        if (type(self.device) is not str
                or not self.device):
            # デフォルトに変更
            print("SetDefault Device")
            self.device = ParamManager.DEFALT_DEVICE

        # 推論を行う際のタイルの1辺のサイズ【任意】
        if (type(self.inference_size) is not int
                and type(self.inference_size) is not float):
            # デフォルトに変更
            print("SetDefault InferenceSize")
            self.inference_size = ParamManager.DEFALT_INFERENCE_SIZE

        # 推論結果を使用しない外側の幅【任意】
        if (type(self.inference_padding) is not int
                and type(self.inference_padding) is not float):
            # デフォルトに変更
            print("SetDefault InferencePadding")
            self.inference_padding = ParamManager.DEFALT_INFERENCE_PADDING

        # 除去する領域のピクセル数の閾値【任意】
        if (type(self.noise_threshold) is not int
                and type(self.noise_threshold) is not float):
            # デフォルトに変更
            print("SetDefault NoiseThreshold")
            self.noise_threshold = ParamManager.DEFALT_NOISE_THRESHOLD

        # 途中から実行したい場合の開始ステップ名【任意】
        if (type(self.startwith) is not str
                or not self.startwith):
            # デフォルトに変更
            print("SetDefault Startwith")
            self.startwith = ParamManager.DEFALT_STARTWITH

        # 途中で実行を止めたい場合の最後のステップ名【任意】
        if (type(self.endwith) is not str
                or not self.endwith):
            # デフォルトに変更
            self.endwith = ParamManager.DEFALT_ENDWITH

        # デバッグログ出力フラグ
        if (type(self.debug_log_output) is not bool):
            # エラー対応(途中終了)
            raise Exception(
                ParamManager.KEY_DEBUG_LOG_OUTPUT + ' is invalid.')

        return change_params
