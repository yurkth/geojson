#include <Siv3D.hpp> // OpenSiv3D v0.6
#include "GeoJSON.h"

struct Prefecture {
    String name;
    MultiPolygon polygons;
};

void Main() {
    MultiPolygon unitedKingdom;
    {
        // 世界全体のファイルから国名を指定して検索
        // https://datahub.io/core/geo-countries
        const Array<GeoJSONFeature> countries = GeoJSONFeatureCollection{ JSON::Load(U"countries.geojson") }.getFeatures();

        for (const auto& country : countries) {
            if (country.getProperties()[U"ADMIN"].getString() == U"United Kingdom") {
                unitedKingdom = country.getGeometry().getPolygons();
                break;
            }
        }
    }

    // countries.geojson を切り貼りして国単体のファイルを用意することで、以下のようにも書ける
    // const MultiPolygon japan = GeoJSONFeatureCollection{ JSON::Load(U"japan.geojson") }.getFeatures()
    //     .front().getGeometry().getPolygons();

    // 日本の都道府県
    // https://github.com/dotnsf/geojsonjapan/
    const Array<Prefecture> prefectures = GeoJSONFeatureCollection{ JSON::Load(U"prefectures.geojson") }.getFeatures()
        .map([](const GeoJSONFeature& feature) {
        return Prefecture{
            feature.getProperties()[U"nam_ja"].getString(),
            feature.getGeometry().getPolygons()
        };
    });

    Camera2D camera{ Vec2{ 139.69, -35.69 }, 128, Camera2DParameters{ .maxScale = 4096.0 } };

    while (System::Update()) {
        ClearPrint();
        Print << (camera.getCenter()) * Vec2 { 1, -1 };
        Print << camera.getScale() << U"x";

        camera.update();
        {
            const auto ct = camera.createTransformer();

            Rect{ Arg::center(0, 0), 360, 180 }.draw(Palette::Lightskyblue); // 海
            {
                Transformer2D mt{ Mat3x2{ 1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f }, true };

                unitedKingdom.draw(Palette::Forestgreen); // イギリス
                Circle(0.0, 51.48, 0.1).draw(Palette::Darkorange); // グリニッジ天文台

                for (const auto& prefecture : prefectures) { // 日本の各都道府県
                    if (prefecture.polygons.mouseOver()) {
                        prefecture.polygons.draw(Palette::Darkorange);
                        Print << prefecture.name;
                    }
                    else {
                        prefecture.polygons.draw(Palette::Forestgreen);
                    }
                }
            }
        }
        camera.draw();
    }
}
