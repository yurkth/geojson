こちらは古いコードになっています。  
最新のコードは[OpenSiv3D](https://github.com/yurkth/OpenSiv3D) v0.6にマージされたので、そちらを利用してください。

---

# GeoJSON

[OpenSiv3D Challenge 2021](https://zenn.dev/reputeless/scraps/79865055750784)より、GeoJSONのパーサを作成しました。

いずれOpenSiv3D本体にも入るかと思いますが、とりあえずこちらから使えます。

![](https://user-images.githubusercontent.com/59264002/111031393-b8ee3f80-844a-11eb-96b6-88a8517cfcae.png)

## Usage

`src/GeoJSON.h`をincludeして使えます。詳しい使い方はコメントや`src/Main.cpp`のサンプル、[GeoJSONのドキュメント](https://tools.ietf.org/html/rfc7946)を参照してください。

本リポリトリをcloneしてOpenSiv3D v0.6のルートディレクトリに配置することで、サンプルプログラムを動作させられます。

このとき、OpenSiv3Dソリューションのプロパティから依存関係の設定を忘れないようにしてください。

## Requirement

GeoJSONは[Visual Studio 2019](https://visualstudio.microsoft.com/ja/vs/) v16.9以降と[OpenSiv3D v0.6](https://github.com/Siv3D/OpenSiv3D/tree/v6_master)を使ってビルドできます。

## Author

- [torin](https://github.com/yurkth)
- [Ryo Suzuki](https://github.com/Reputeless)

## License

GeoJSONのソースコードはMITライセンスです。詳しくは[LICENSE](https://github.com/yurkth/geojson/blob/master/LICENSE)を見てください。

### Dataset

- [geo-countries](https://datahub.io/core/geo-countries)(`App/countries.geojson`)は[PDDL v1.0](https://opendatacommons.org/licenses/pddl/1-0/)でライセンスされています。
  - `App/countries.geojson`を改変した`App/japan.geojson`もPDDL v1.0でライセンスされています。
- [dataofjapan/land](https://github.com/dataofjapan/land)はパブリックドメインです。
  - `App/prefectures.geojson`はlandの`japan.geojson`をリネームしたものです。
