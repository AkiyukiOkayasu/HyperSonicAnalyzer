# HyperSonic Analyzer

超高サンプルレート対応スペクトラムアナライザー VST3/Standalone プラグイン

768kHz などの超高サンプルレートに対応し、アナログオシレーターの高周波成分を可視化するために設計されたスペクトラムアナライザーです。

## 特徴

- **超高サンプルレート対応**: 768kHz までのサンプルレートに対応
- **大規模FFT**: 65536サンプルのFFTサイズで高い周波数分解能を実現
- **Blackman-Harris窓**: 優れたサイドローブ特性を持つ窓関数を使用
- **対数周波数スケール**: 10Hz からナイキスト周波数まで対数スケールで表示
- **広いダイナミックレンジ**: -192dB から +20dB までの振幅範囲をサポート
- **リアルタイムカーソル表示**: マウス位置の周波数と振幅をリアルタイムで表示

## スクリーンショット

(準備中)

## 動作環境

- macOS 10.15 以降
- VST3 対応 DAW または スタンドアロン実行

## ビルド方法

### 必要なツール

- CMake 3.22 以降
- C++20 対応コンパイラ (Xcode 14 以降推奨)
- clang-format (コードフォーマット用)

### ビルド手順

```bash
# リポジトリをクローン
git clone https://github.com/your-username/HyperSonicAnalyzer.git
cd HyperSonicAnalyzer

# ビルドディレクトリを作成
mkdir build && cd build

# CMake を実行
cmake .. -DCMAKE_BUILD_TYPE=Release

# ビルド
cmake --build . --config Release
```

### ビルド成果物

- **VST3**: `build/HyperSonicAnalyzer_artefacts/Release/VST3/HyperSonic Analyzer.vst3`
- **Standalone**: `build/HyperSonicAnalyzer_artefacts/Release/Standalone/HyperSonic Analyzer.app`

## 開発

### コードスタイル

このプロジェクトでは clang-format を使用してコードスタイルを統一しています。

```bash
# フォーマットとビルドチェックを実行
./scripts/lint.sh
```

### プロジェクト構造

```
HyperSonicAnalyzer/
├── CMakeLists.txt          # CMake ビルド設定
├── Source/
│   ├── PluginProcessor.h/cpp   # オーディオ処理・FFT
│   ├── PluginEditor.h/cpp      # UI レイアウト
│   └── SpectralAnalyzer.h/cpp  # スペクトラム描画
├── scripts/
│   └── lint.sh             # フォーマット・リントスクリプト
├── .clang-format           # clang-format 設定
└── .clang-tidy             # clang-tidy 設定
```

## ライセンス

このプロジェクトは [GNU General Public License v3.0](LICENSE) の下で公開されています。

JUCE フレームワークは [JUCE End User License Agreement](https://juce.com/legal/juce-8-licence/) に基づいて使用しています。

## 作者

Groundless Electronics  
https://groundlesselectronics.com
