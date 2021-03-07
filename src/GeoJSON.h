#pragma once
#include <Siv3D.hpp>
#include <variant>

namespace geojson {
    /// @brief GeoJSONで定義されているオブジェクトの型
    /// @detail https://tools.ietf.org/html/rfc7946#section-1.4
    enum class GeoJSONType {
        Point,
        MultiPoint,
        LineString,
        MultiLineString,
        Polygon,
        MultiPolygon,
        GeometryCollection,
        Feature,
        FeatureCollection
    };

    inline void Formatter(FormatData& formatData, const GeoJSONType& type) {
        static const String typeString[] = {
            U"Point",
            U"MultiPoint",
            U"LineString",
            U"MultiLineString",
            U"Polygon",
            U"MultiPolygon",
            U"GeometryCollection",
            U"Feature",
            U"FeatureCollection"
        };
        formatData.string.append(typeString[static_cast<size_t>(type)]);
    }

    namespace detail {
        /// @brief GeoJSONオブジェクトの基底クラス
        /// @detail https://tools.ietf.org/html/rfc7946#section-3
        class GeoJSONBase {
        private:
            /// @brief GeoJSONオブジェクトの型を読み取る
            /// @param object JSON内のオブジェクト
            /// @return オブジェクトの型
            static GeoJSONType readType(const JSON& object) {
                if (object.isEmpty()) {
                    throw Error(U"JSON object is empty");
                }

                const String typeString = object.getString();
                if (typeString == U"Point") {
                    return GeoJSONType::Point;
                }
                else if (typeString == U"MultiPoint") {
                    return GeoJSONType::MultiPoint;
                }
                else if (typeString == U"LineString") {
                    return GeoJSONType::LineString;
                }
                else if (typeString == U"MultiLineString") {
                    return GeoJSONType::MultiLineString;
                }
                else if (typeString == U"Polygon") {
                    return GeoJSONType::Polygon;
                }
                else if (typeString == U"MultiPolygon") {
                    return GeoJSONType::MultiPolygon;
                }
                else if (typeString == U"GeometryCollection") {
                    return GeoJSONType::GeometryCollection;
                }
                else if (typeString == U"Feature") {
                    return GeoJSONType::Feature;
                }
                else if (typeString == U"FeatureCollection") {
                    return GeoJSONType::FeatureCollection;
                }
                else {
                    throw Error(U"\"type\" value is wrong");
                }
            }

        protected:
            /// @brief オブジェクトから配列を読み取る
            /// @tparam T 配列のvalue_type
            /// @param arr JSON内の配列
            /// @param function 配列の要素に適用する関数
            /// @param exclusion 末尾から読み飛ばす要素数
            /// @return 読み取った配列
            template <class T>
            static Array<T> readArray(const JSON& arr, const std::function<T(const JSON&)>& function, size_t exclusion = 0) {
                assert(arr.isArray());
                Array<T> result;
                for (size_t i = 0; i < arr.size() - exclusion; i++) {
                    result << function(arr[i]);
                }
                return result;
            }

        public:
            /// @brief GeoJSONオブジェクトの型
            GeoJSONType type;

            /// @brief バウンディングボックスの配列
            /// @detail https://tools.ietf.org/html/rfc7946#section-5
            Optional<Array<double>> bbox = none; // bboxを使った処理については未実装

            /// @brief GeoJSONオブジェクトを作る
            /// @param object JSON内のオブジェクト
            GeoJSONBase(const JSON& object)
                : type(readType(object[U"type"])) {
                if (object.hasElement(U"bbox")) {
                    bbox = readArray<double>(object[U"bbox"], [](const JSON& element) { return element.get<double>(); });
                }
            }
        };
    }

    /// @brief Geometryオブジェクト
    /// @detail https://tools.ietf.org/html/rfc7946#section-3.1
    class Geometry : public detail::GeoJSONBase {
    private:
        /// @brief Geometryオブジェクトの持つ図形
        std::variant<
            Vec2,
            Array<Vec2>,
            LineString,
            Array<LineString>,
            Polygon,
            Array<Polygon>, // MultiPolygonにする？
            Array<Geometry>
        > m_data;

        /// @brief 座標を読み取る
        /// @param arr JSON内の配列
        /// @return 読み取った座標
        static Vec2 readVec2(const JSON& arr) {
            return { arr[0].get<double>(), arr[1].get<double>() };
        }

        /// @brief ポリゴンを読み取る
        /// @param arr JSON内の配列
        /// @return 読み取ったポリゴン
        static Polygon readPolygon(const JSON& arr) {
            const auto& outer = readArray<Vec2>(arr[0], readVec2, 1);
            Polygon polygon{ Geometry2D::IsClockwise(outer) ? outer : outer.reversed() };
            for (size_t i = 1; i < arr.size(); i++) {
                const auto& inner = readArray<Vec2>(arr[i], readVec2, 1);
                polygon.addHole(Geometry2D::IsClockwise(inner) ? inner.reversed() : inner);
            }
            if (polygon.outer() == Array<Vec2>{}) {
                throw Error(U"Incorrect Polygon");
            }
            return polygon;
        }

    public:
        /// @brief Geometryオブジェクトを作る
        /// @param object JSON内のオブジェクト
        Geometry(const JSON& object)
            : detail::GeoJSONBase({ object }) {
            switch (type) {
            case GeoJSONType::Point:
                m_data = readVec2(object[U"coordinates"]);
                break;
            case GeoJSONType::MultiPoint:
                m_data = readArray<Vec2>(object[U"coordinates"], readVec2);
                break;
            case GeoJSONType::LineString:
                m_data = LineString{ readArray<Vec2>(object[U"coordinates"], readVec2) };
                break;
            case GeoJSONType::MultiLineString:
                m_data = readArray<LineString>(
                    object[U"coordinates"],
                    [this](const JSON& element) { return LineString{ readArray<Vec2>(element, readVec2) }; }
                );
                break;
            case GeoJSONType::Polygon:
                m_data = readPolygon(object[U"coordinates"]);
                break;
            case GeoJSONType::MultiPolygon:
                m_data = readArray<Polygon>(object[U"coordinates"], readPolygon);
                break;
            case GeoJSONType::GeometryCollection:
                m_data = readArray<Geometry>(
                    object[U"geometries"],
                    [](const JSON& element) { return Geometry{ element }; }
                );
                break;
            default:
                throw Error(U"\"{}\" is not \"geometry type\""_fmt(object[U"type"].getString()));
            }
        }

        /// @brief 図形のデータを取得する
        /// @tparam T 図形の型
        /// @return 図形
        template <class T>
        T getData() const {
            return std::get<T>(m_data);
        }

        /// @brief 図形を引数にして関数を呼び出す
        /// @tparam Visitor Visitorの戻り値の型
        /// @param visitor Visitorオブジェクト
        template <class Visitor>
        void visit(Visitor&& visitor) const {
            return std::visit(visitor, m_data);
        }
    };

    inline void Formatter(FormatData& formatData, const Geometry& geometry) {
        Formatter(formatData, geometry.type);
        geometry.visit([&formatData](const auto& g) { Formatter(formatData, g); });
    }

    /// @brief Featureオブジェクト
    /// @detail https://tools.ietf.org/html/rfc7946#section-3.2
    class Feature : public detail::GeoJSONBase {
    public:
        /// @brief Geometryオブジェクト
        Geometry geometry;

        /// @brief プロパティのオブジェクト
        JSON properties;

        /// @brief 識別子
        Optional<std::variant<String, double>> id = none;

        /// @brief Featureオブジェクトを作る
        /// @param object JSON内のオブジェクト
        Feature(const JSON& object)
            : detail::GeoJSONBase({ object })
            , geometry(object[U"geometry"])
            /*, properties(object[U"properties"])*/ {
            if (type != GeoJSONType::Feature) {
                throw Error(U"\"type\" must be \"Feature\"");
            }
            if (object.hasElement(U"properties")) { // propertiesは必須なので本来このifはいらないが、JapanCityGeoJsonがpropertiesを入れていないため暫定的にifに入れてある
                properties = object[U"properties"];
            }
            if (object.hasElement(U"id")) {
                switch (object[U"id"].getType()) {
                case JSONValueType::String:
                    id = { object[U"id"].getString() };
                    break;
                case JSONValueType::Number:
                    id = { object[U"id"].get<double>() };
                    break;
                default:
                    throw Error(U"\"id\" must be either a String or Number");
                }
            }
        }
    };

    inline void Formatter(FormatData& formatData, const Feature& feature) {
        Formatter(formatData, feature.type);
        formatData.string.append(U": ");
        Formatter(formatData, feature.geometry);
    }

    /// @brief FeatureCollectionオブジェクト
    /// @detail https://tools.ietf.org/html/rfc7946#section-3.3
    class FeatureCollection : public detail::GeoJSONBase {
    public:
        /// @brief Featureの配列
        Array<Feature> features;

        /// @brief FeatureCollectionオブジェクトを作る
        /// @param object JSON内のオブジェクト
        FeatureCollection(const JSON& object)
            : detail::GeoJSONBase({ object })
            , features(readArray<Feature>(object[U"features"], [](const JSON& element) { return Feature{ element }; })) {
            if (type != GeoJSONType::FeatureCollection) {
                throw Error(U"\"type\" must be \"FeatureCollection\"");
            }
        }
    };

    inline void Formatter(FormatData& formatData, const FeatureCollection& featureCollection) {
        Formatter(formatData, featureCollection.type);
        formatData.string.append(U":");
        for (const auto& feature : featureCollection.features) {
            formatData.string.append(U"\n  ");
            Formatter(formatData, feature);
        }
    }
}