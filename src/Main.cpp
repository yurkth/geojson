#include <Siv3D.hpp> // OpenSiv3D v0.6
#include "GeoJSON.h"

SIV3D_SET(EngineOption::Renderer::Direct3D11)

void Main() {
    const auto& countries = geojson::FeatureCollection{ JSON::Load(U"countries.geojson") }.features;
    const auto& japan = std::find_if(countries.begin(), countries.end(), [](const auto& feature) {
        return (feature.properties[U"ADMIN"].getString() == U"Japan");
    })->geometry.getData<Array<Polygon>>(); // 他の国で試す場合、geometry.typeに応じてgetDataの型を変えること

    // 他の国のデータがいらないなら、ファイルを分けたほうが読み込み時間が減る
    // const auto& japan = geojson::FeatureCollection{ JSON::Load(U"japan.geojson") }.features[0].geometry.getData<Array<Polygon>>();

    // https://github.com/niiyz/JapanCityGeoJson を使いたい場合
    HashTable<String, Color> colors;
    const auto& tokyo = geojson::FeatureCollection{ JSON::Load(U"JapanCityGeoJson/prefectures/13.json") }.features
        .map([&colors](const geojson::Feature& feature) {
        const auto& code = std::get<String>(feature.id.value());
        if (!colors.contains(code)) {
            colors.emplace(code, RandomColor()); // 市区町村コードごとに色を決定
        }
        return std::make_pair(feature.geometry.getData<Polygon>(), colors[code]);
    });

    auto setting = Camera2DParameters::Default();
    setting.maxScale = 256;
    Camera2D camera{ { 0, 0 }, 2.0, setting };

    while (System::Update()) {
        camera.update();
        {
            const auto ct = camera.createTransformer();
            ClearPrint();
            Print << (camera.getCenter()) * Vec2 { 1, -1 };
            Print << camera.getScale() << U"x";

            Rect(Arg::center(0, 0), 360, 180).draw(Palette::Lightskyblue); // 海

            {
                Transformer2D mt{ Mat3x2{ 1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f } };
                for (const auto& polygon : japan) {
                    polygon.draw(Palette::Forestgreen);
                }
                for (const auto& city : tokyo) {
                    city.first.draw(city.second);
                }
            }
        }
        camera.draw();
    }
}
