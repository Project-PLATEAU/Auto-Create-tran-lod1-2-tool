![image](https://github.com/Project-PLATEAU/Auto-Create-tran-lod1-2-tool/assets/79615787/3f0bd8c9-319a-46b9-89d1-a12d1c4c3005)



## 1. 概要 <!-- 本リポジトリでOSS化しているソフトウェア・ライブラリについて1文で説明を記載ください -->

本リポジトリでは、FY2023のProject PLATEAUにおいて開発した「LOD1-2道路モデル自動作成ツール」のソースコードを公開しています。

「LOD1-2道路モデル自動生成ツール」は、道路縁シェープファイルからLOD1道路モデルの作成を行うためのツールと、LOD1道路CityGMLファイルと航空写真からLOD2道路モデルの作成を行うためのツールを含みます。

## 2. 「LOD1-2道路モデル自動作成ツール」の開発について

「LOD1-2道路モデル自動生成ツール」は、[「LOD2建築物モデル自動作成ツール」](https://github.com/Project-PLATEAU/Auto-Create-bldg-lod2-tool)とともに、3D都市モデルのデータ整備を自動化し、整備コストを低減することを目的に国土交通省都市局が開発している一連のツールです。  
本ツールの詳細については[技術検証レポート](https://www.mlit.go.jp/plateau/file/libraries/doc/XXX)を参照してください。

![image](https://github.com/Project-PLATEAU/Auto-Create-tran-lod1-2-tool/assets/79615787/5186dc68-6365-4fc7-a9cd-aa8b07c9e3c3)


## 3. 利用手順 <!-- 下記の通り、GitHub Pagesへリンクを記載ください。URLはアクセンチュアにて設定しますので、サンプルそのままでOKです。 -->

本ツールの構築手順及び利用手順については[利用チュートリアル](https://project-plateau.github.io/Auto-Create-tran-lod1-2-tool/)を参照してください。

![image](https://github.com/Project-PLATEAU/Auto-Create-tran-lod1-2-tool/assets/79615787/44a048e6-a237-4f55-8141-ad09c6e99e02)
<LOD2道路モデル自動作成ツール│ツール構成図>

## 4. システム概要 <!-- OSS化対象のシステムが有する機能を記載ください。 -->

### 【LOD1道路モデル自動作成ツール】

#### ①LOD1道路モデル作成機能

- 道路縁と道路施設シェープファイルを使用して、LOD1道路モデルのシェープファイルを作成します。
  - [公共測量成果検査支援ツール](https://psgsv2.gsi.go.jp/koukyou/public/sien/pindex.html#psea03)を使用して、地図情報レベル2500のDM（Digital Mapping）データから、入力データとなる道路と道路施設のシェープファイル（\*.shp、\*.shx、\*.dbf、\*.prj）を作成することを想定しています。
- 作成したLOD1道路モデルに幾何形状エラーが発生していないかを確認し、エラーが発生している場合はCSVファイルにエラー情報を出力します。

#### ②CityGML変換機能

- LOD1道路モデルのシェープファイルを、CityGMLファイルに変換します。

### 【LOD2道路モデル自動生成ツール】

#### ①LOD2道路モデル作成機能

- 航空写真（オルソ）や道路ポリゴンデータ等からCityGML形式およびシェープファイル形式のLOD2道路モデルを作成します。
  - 航空写真（オルソ）は航空写真（原画像）から作成することを想定しています。\
    AIモデルの学習では地上画素寸法12cm～16cmの航空写真（オルソ）を使用しています。そのため、範囲外の航空写真を使用すると認識精度が低下する可能性があります。オルソ画像の最大ピクセル数は178956970以下とします。
  - 道路ポリゴンデータは、[G空間情報センター 3D都市モデル（Project PLATEAU）ポータルサイト](https://www.geospatial.jp/ckan/dataset/plateau)で公開している道路のCityGMLデータを利用することを想定しています。

## 5. 利用技術

### 【LOD1道路モデル自動作成ツールの利用技術】

| 種別       | 名称   | バージョン | 内容 |
| --------- | --------|-------------|-----------------------------|
| ライブラリ | [Boost C++ Libraries](https://www.boost.org/) | 1.83.0 | 幾何計算ライブラリ |
|           | [concaveman-cpp](https://github.com/sadaszewski/concaveman-cpp) | 97d47b0 | 凹包ライブラリ |
|           | [shapelib](http://shapelib.maptools.org/) | 1.5.0 | シェープファイルライブラリ |

### 【LOD2道路モデル自動作成ツールの利用技術】

| 種別       | 名称                   | バージョン             | 説明                      |
| ---------- | ------------------    | ---------------------- | ------------------------- |
| ライブラリ  | tqdm                  | 4.65.0 | プログレスバーの表示 |
|            | shapely               | 2.0.1 | 幾何計算 |
|            | numpy                 | 1.25.0 | 数値計算 |
|            | Pillow                | 10.0.0 | 画像の読み書き |
|            | PyYAML                | 6.0.1 | yamlファイルの読み書き |
|            | pyproj                | 3.6.0 | 座標系の変換 |
|            | pyshp                 | 2.3.1 | シェープファイルの読み書き |
|            | lxml                  | 4.9.3 | XMLファイルの読み書き |
|            | torch                 | 2.0.1 | 機械学習 |
|            | torchvision           | 0.15.2 | 機械学習 |
|            | lightning             | 2.0.6 | 機械学習 |
|            | opencv-python         | 4.7.0.72 | 画像の読み書き、画像処理 |
|            | opencv-contrib-python | 4.8.0.74 | 画像処理 |
|            | networkx              | 3.1 | グラフ構造の管理 |
|            | shapelysmooth         | 0.1.1 | 幾何のスムージング処理 |
|            | geopandas             | 0.13.2 | シェープファイルの書き出し |
|            | sortedcontainers      | 2.4.0 | ソート済みコレクションの管理 |
|            | openmim               | 0.3.9 | ライブラリの管理 |
|            | mmcv                  | 2.0.1 | 機械学習 |
|            | mmsegmentation        | 1.1.1 | 機械学習 |
|            | mmdet                 | 3.1.0 | 機械学習 |

※ライブラリ名称は、pipコマンドでインストールで使用可能な名称を記載しています

- torch, torchvision以外のライブラリに関しては、pipコマンドによるインストールで利用可能なrequirements.txtをリポジトリに用意してあります
- torch, torchvisionのインストールに関しては、[LOD2道路モデル自動生成ツールの環境構築手順書](https://project-plateau.github.io/Auto-Create-tran-lod1-2-tool/manual/devManLod2.html)の「6章 GPU環境の構築」を参照してください

## 6. 動作環境 <!-- 動作環境についての仕様を記載ください。 -->

### 【LOD1道路モデル自動作成ツールの動作環境】

| 項目               | 推奨動作環境                   |
| ------------------ | ------------------------------ |
| OS                 | Microsoft Windows 10 または 11 |
| CPU                | Intel Core i7以上 |
| メモリ             | 32GB以上 |
| GPU                | 不要 |
| GPU メモリ         | 不要 |
| ネットワーク       | 不要 |

### 【LOD2道路モデル自動作成ツールの動作環境】

| 項目               | 推奨動作環境               |
| ------------------ | ------------------------- |
| OS                 | Microsoft Windows 10 または 11 |
| CPU                | Intel Core i7以上 |
| メモリ             | 16GB以上 |
| GPU                | NVIDIA RTX 5000以上 |
| GPU メモリ         | 16GB以上 |
| ネットワーク       | 不要 |

## 7. 本リポジトリのフォルダ構成 <!-- 本GitHub上のソースファイルの構成を記載ください。 -->

### 【LOD1道路モデル自動作成ツールのフォルダ構成】

| フォルダ名 |　詳細 |
|-|-|
| lod1/src/EXE/AutoCreateLod1Road | 道路LOD1モデル生成処理 |
| lod1/src/EXE/ConvertShapeToCityGML | CityGML変換処理 |
| lod1/src/LIB/CommonUtil | 道路LOD1モデル生成処理とCityGML変換処理の共有ライブラリ |
| lod1/src/LIB/concaveman-cpp | 凹包ライブラリ |
| lod1/src/PARAM | サンプルパラメータファイル |

### 【LOD2道路モデル自動作成ツールのフォルダ構成】

| フォルダ名 |　詳細 |
|-|-|
| lod2/src | 道路LOD2モデル生成処理 |
| lod2/src/src/steps | 道路LOD2モデル生成実処理 |
| lod2/src/src/tools | 処理ライブラリ |
| lod2/src/src/util | ログ機能処理 |

## 8. ライセンス <!-- 変更せず、そのまま使うこと。 -->

- ソースコード及び関連ドキュメントの著作権は国土交通省に帰属します。
- 本ドキュメントは[Project PLATEAUのサイトポリシー](https://www.mlit.go.jp/plateau/site-policy/)（CCBY4.0及び政府標準利用規約2.0）に従い提供されています。

## 9. 注意事項 <!-- 変更せず、そのまま使うこと。 -->

- 本リポジトリは参考資料として提供しているものです。動作保証は行っていません。
- 本リポジトリについては予告なく変更又は削除をする可能性があります。
- 本リポジトリの利用により生じた損失及び損害等について、国土交通省はいかなる責任も負わないものとします。

## 10. 参考資料 <!-- 技術検証レポートのURLはアクセンチュアにて記載します。 -->

- 技術検証レポート: <https://www.mlit.go.jp/plateau/file/libraries/doc/XXX>
