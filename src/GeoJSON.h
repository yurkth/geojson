#pragma once
#include <Siv3D.hpp>
#include <variant>

namespace geojson {
    /// @brief GeoJSON�Œ�`����Ă���I�u�W�F�N�g�̌^
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
        /// @brief GeoJSON�I�u�W�F�N�g�̊��N���X
        /// @detail https://tools.ietf.org/html/rfc7946#section-3
        class GeoJSONBase {
        private:
            /// @brief GeoJSON�I�u�W�F�N�g�̌^��ǂݎ��
            /// @param object JSON���̃I�u�W�F�N�g
            /// @return �I�u�W�F�N�g�̌^
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
            /// @brief �I�u�W�F�N�g����z���ǂݎ��
            /// @tparam T �z���value_type
            /// @param arr JSON���̔z��
            /// @param function �z��̗v�f�ɓK�p����֐�
            /// @param exclusion ��������ǂݔ�΂��v�f��
            /// @return �ǂݎ�����z��
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
            /// @brief GeoJSON�I�u�W�F�N�g�̌^
            GeoJSONType type;

            /// @brief �o�E���f�B���O�{�b�N�X�̔z��
            /// @detail https://tools.ietf.org/html/rfc7946#section-5
            Optional<Array<double>> bbox = none; // bbox���g���������ɂ��Ă͖�����

            /// @brief GeoJSON�I�u�W�F�N�g�����
            /// @param object JSON���̃I�u�W�F�N�g
            GeoJSONBase(const JSON& object)
                : type(readType(object[U"type"])) {
                if (object.hasElement(U"bbox")) {
                    bbox = readArray<double>(object[U"bbox"], [](const JSON& element) { return element.get<double>(); });
                }
            }
        };
    }

    /// @brief Geometry�I�u�W�F�N�g
    /// @detail https://tools.ietf.org/html/rfc7946#section-3.1
    class Geometry : public detail::GeoJSONBase {
    private:
        /// @brief Geometry�I�u�W�F�N�g�̎��}�`
        std::variant<
            Vec2,
            Array<Vec2>,
            LineString,
            Array<LineString>,
            Polygon,
            Array<Polygon>, // MultiPolygon�ɂ���H
            Array<Geometry>
        > m_data;

        /// @brief ���W��ǂݎ��
        /// @param arr JSON���̔z��
        /// @return �ǂݎ�������W
        static Vec2 readVec2(const JSON& arr) {
            return { arr[0].get<double>(), arr[1].get<double>() };
        }

        /// @brief �|���S����ǂݎ��
        /// @param arr JSON���̔z��
        /// @return �ǂݎ�����|���S��
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
        /// @brief Geometry�I�u�W�F�N�g�����
        /// @param object JSON���̃I�u�W�F�N�g
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

        /// @brief �}�`�̃f�[�^���擾����
        /// @tparam T �}�`�̌^
        /// @return �}�`
        template <class T>
        T getData() const {
            return std::get<T>(m_data);
        }

        /// @brief �}�`�������ɂ��Ċ֐����Ăяo��
        /// @tparam Visitor Visitor�̖߂�l�̌^
        /// @param visitor Visitor�I�u�W�F�N�g
        template <class Visitor>
        void visit(Visitor&& visitor) const {
            return std::visit(visitor, m_data);
        }
    };

    inline void Formatter(FormatData& formatData, const Geometry& geometry) {
        Formatter(formatData, geometry.type);
        geometry.visit([&formatData](const auto& g) { Formatter(formatData, g); });
    }

    /// @brief Feature�I�u�W�F�N�g
    /// @detail https://tools.ietf.org/html/rfc7946#section-3.2
    class Feature : public detail::GeoJSONBase {
    public:
        /// @brief Geometry�I�u�W�F�N�g
        Geometry geometry;

        /// @brief �v���p�e�B�̃I�u�W�F�N�g
        JSON properties;

        /// @brief ���ʎq
        Optional<std::variant<String, double>> id = none;

        /// @brief Feature�I�u�W�F�N�g�����
        /// @param object JSON���̃I�u�W�F�N�g
        Feature(const JSON& object)
            : detail::GeoJSONBase({ object })
            , geometry(object[U"geometry"])
            /*, properties(object[U"properties"])*/ {
            if (type != GeoJSONType::Feature) {
                throw Error(U"\"type\" must be \"Feature\"");
            }
            if (object.hasElement(U"properties")) { // properties�͕K�{�Ȃ̂Ŗ{������if�͂���Ȃ����AJapanCityGeoJson��properties�����Ă��Ȃ����ߎb��I��if�ɓ���Ă���
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

    /// @brief FeatureCollection�I�u�W�F�N�g
    /// @detail https://tools.ietf.org/html/rfc7946#section-3.3
    class FeatureCollection : public detail::GeoJSONBase {
    public:
        /// @brief Feature�̔z��
        Array<Feature> features;

        /// @brief FeatureCollection�I�u�W�F�N�g�����
        /// @param object JSON���̃I�u�W�F�N�g
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