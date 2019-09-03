# MIDItoScore
MIDIデータから譜面データに変換するクラスです．

### 概要
ノーツ配置が書かれたMIDIファイルを読み込み，音楽ゲームの譜面データを作成します．

SMF(Standard MIDI File)のうち，フォーマット0と1に対応しています．フォーマット2には対応していません．
また，デルタタイムを実時間で表す形式(ほとんどないと思いますが)には対応していません．

### 特徴
ノートイベントのベロシティ値を元に，譜面上のノーツが単押しか長押しかを区別します．
指定した音程ごとに譜面データを分けることができ，複数レーンでノーツが流れる音楽ゲームに対応しています．


### 使い方

1. 譜面データにするMIDIファイルを用意します．DAWでノートを配置し，MIDIファイルに書き出すと便利です．
書き出すときに必ずTime Signature(拍子情報)を埋め込んでください．
2. MIDItoScoreクラスを使って譜面ファイルに書き出します．

サンプルコードはこんなかんじ
```cpp
#include "MIDItoScore.hpp"

int main() {
	using namespace midireader;
	MIDIReader midi;

	// MIDIファイル読み込み
	midi.openAndRead("sample.mid");

	miditoscore::NoteFormat format;

	// 4分音符以上の長さのノーツをを長押しノーツに
	format.holdMinLength = math::Fraction(1, 4);

	// ノーツが流れるレーンに音程を割り当てる
	format.laneAllocation = { 
		toNoteNum("C3", PitchNotation::A3_440Hz),
		toNoteNum("D3", PitchNotation::A3_440Hz)
	};

	// 許容できる譜面データ1行の長さ
	format.allowedLineLength = 128;

	miditoscore::MIDItoScore midiToScore;

	// 譜面データ書き出し
	for (const auto& track : midi.getTracks()) {
		midiToScore.writeScore("score.txt", format, midi.getNoteEvent(track.trackNum));
	}

	return 0;
}

```
上の例では，"sample.mid"というMIDIファイルを"score.txt"という譜面ファイルに書き出しています．
また，同時に譜面データに変換するときのフォーマットを指定しています．

上の例では，4分音符以上の長さのノーツを長押しノーツに，
C3，D3の音程のノートをそれぞれ2つのレーンに対応させています．


### フォーマット
譜面のフォーマットは次の通りです．
```
s:nnn:kk...
```
s   ノーツが流れるレーンの番号 </br>
nnn 小節番号 </br>
kk... ノーツの位置とノーツの種類 (ノーツの位置に合わせて文字数は可変) </br>


また，ノーツの種類は次の3種類です． </br>
0 ... ノーツなし </br>
1 ... 単押し </br>
2 ... 長押しの始点 </br>
3 ... 長押しの終点 </br>


実際の譜面データを例にすると
```
0:020:0000010000000000
```
0番のレーンに，20小節と16分音符x5つ分ずれた位置に単押しのノーツが存在することを表します．
ノーツの位置が4分音符で表せる場合には，kk..の部分は4文字になります．


### 備考
三連符配置のあるMIDIファイルを譜面データに書き出すと，1行のデータがとても長くなる場合があります．
そのような場合には，以下の処理をMIDIを読み込む前に追加して下さい．
```
// Setting of Timing Adjustment
midireader.setAdjustmentAmplitude(1);
```
`MIDIReader::setAdjustmentAmplitude()`は，指定範囲内でノーツのタイミング補正を行う関数です．
引数で補正範囲を指定できますが，通常は1で大丈夫です．



### ライセンス (about License)
(This software is released under the MIT License, see LICENSE)

このリポジトリにあるソースコードはすべてMIT Licenseのもとで公開されています。
MIT LicenseについてはLICENSEを参照して下さい。
ざっくり説明すると以下のようになっています。

<dl>
	<dt>条件</dt>
	<dd>著作権とライセンスの表示</dd>
	<dt>許可</dt>
	<dd>商用利用</dd>
	<dd>修正</dd>
	<dd>配布</dd>
	<dd>個人利用</dd>
	<dt>禁止</dt>
	<dd>責任免除</dd>
</dl>

  
  
-------------

#### コミットメッセージの書式
1行目:コミットの種類

2行目:改行

3行目:コミットの要約


#### コミットの種類
add     新規追加

fix     バグ修正

update  バグではない変更

disable コメントアウトなど、機能の無効化

remove  ファイルやコードの削除

clean   ファイルやコードの整理

    
#### バージョンナンバーについて
タグに用いられているバージョンナンバーは，[Semantic Versioning 2.0.0](https://semver.org/)に従ってつけています．

