#include "GeoJSON.hpp"

namespace s3d {
    namespace detail {
        static constexpr StringView GeoJSONTypeNameTable[] = {
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

        [[nodiscard]]
        static GeoJSONType GetGeoJSONType(const JSON& object) {
            if (object.isEmpty()) {
                throw Error{ U"detail::GetGeoJSONType(): JSON object is empty" };
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
                throw Error{ U"detail::GetGeoJSONType(): \"type\" value is wrong" };
            }
        }

        template <class Type, class Fty>
        [[nodiscard]]
        static Array<Type> GetArray(const JSON& arr, Fty convert, const CloseRing closeRing = CloseRing::Yes) {
            assert(arr.isArray());
            assert(1 <= arr.size());

            const size_t num_elements = (arr.size() - (closeRing ? 0 : 1));

            Array<Type> result(Arg::reserve = num_elements);

            for (size_t i = 0; i < num_elements; ++i) {
                result << convert(arr[i]);
            }

            return result;
        }

        [[nodiscard]]
        static Vec2 GetVec2(const JSON& arr) {
            assert(arr.isArray());
            assert(2 <= arr.size());

            return{ arr[0].get<double>(), arr[1].get<double>() };
        }

        [[nodiscard]]
        static Polygon GetPolygon(const JSON& arr) {
            Array<Vec2> outer = GetArray<Vec2>(arr[0], GetVec2, CloseRing::No);
            {
                if (not Geometry2D::IsClockwise(outer)) {
                    outer.reverse();
                }
            }

            Polygon polygon{ outer };

            for (size_t i = 1; i < arr.size(); i++) {
                Array<Vec2> inner = GetArray<Vec2>(arr[i], GetVec2, CloseRing::No);
                {
                    if (Geometry2D::IsClockwise(inner)) {
                        inner.reverse();
                    }
                }

                polygon.addHole(inner);
            }

            if (not polygon) {
                throw Error{ U"detail::GetPolygon(): Incorrect Polygon" };
            }

            return polygon;
        }
    }

    void Formatter(FormatData& formatData, const GeoJSONType& type) {
        formatData.string.append(detail::GeoJSONTypeNameTable[FromEnum(type)]);
    }

    //////////////////////////////////////////////////
    //
    //	GeoJSONBase
    //
    //////////////////////////////////////////////////

    GeoJSONBase::GeoJSONBase(const JSON& object) {
        if (object.hasElement(U"type")) {
            m_type = detail::GetGeoJSONType(object[U"type"]);
        }

        if (object.hasElement(U"bbox")) {
            m_bbox = detail::GetArray<double>(object[U"bbox"], [](const JSON& element) { return element.get<double>(); }, CloseRing::No);
        }
    }

    const Array<double>& GeoJSONBase::getBBox() const noexcept {
        return m_bbox;
    }

    GeoJSONType GeoJSONBase::getType() const noexcept {
        return m_type;
    }

    //////////////////////////////////////////////////
    //
    //	GeoJSONGeometry
    //
    //////////////////////////////////////////////////

    GeoJSONGeometry::GeoJSONGeometry(const JSON& object)
        : GeoJSONBase({ object }) {
        if (not InRange(m_type, GeoJSONType::Point, GeoJSONType::GeometryCollection)) {
            throw Error{ U"GeoJSONGeometry:GeoJSONGeometry(): Invalid GeoJSONType" };
        }

        if (not object.hasElement(U"coordinates")) {
            throw Error{ U"GeoJSONGeometry:GeoJSONGeometry(): geometry does not have coordinates" };
        }

        m_coordinates = object[U"coordinates"];
    }

    template <class Type>
    Type GeoJSONGeometry::get() const {
        return std::get<Type>(getCache());
    }

    MultiPolygon GeoJSONGeometry::getPolygons() const {
        const auto& cache = getCache();

        if (std::holds_alternative<Array<Polygon>>(cache)) {
            return MultiPolygon(std::get<Array<Polygon>>(cache));
        }
        else if (std::holds_alternative<Polygon>(cache)) {
            return MultiPolygon({ std::get<Polygon>(cache) });
        }
        else {
            return{};
        }
    }

    const GeoJSONGeometry::GeometryVariant& GeoJSONGeometry::getCache() const {
        if (std::holds_alternative<MonoState>(m_cache)) {
            switch (m_type) {
            case GeoJSONType::Point:
                m_cache = detail::GetVec2(m_coordinates);
                break;
            case GeoJSONType::MultiPoint:
                m_cache = detail::GetArray<Vec2>(m_coordinates, detail::GetVec2);
                break;
            case GeoJSONType::LineString:
                m_cache = LineString{ detail::GetArray<Vec2>(m_coordinates, detail::GetVec2) };
                break;
            case GeoJSONType::MultiLineString:
                m_cache = detail::GetArray<LineString>(
                    m_coordinates,
                    [](const JSON& element) { return LineString{ detail::GetArray<Vec2>(element, detail::GetVec2) }; });
                break;
            case GeoJSONType::Polygon:
                m_cache = detail::GetPolygon(m_coordinates);
                break;
            case GeoJSONType::MultiPolygon:
                m_cache = detail::GetArray<Polygon>(m_coordinates, detail::GetPolygon);
                break;
            case GeoJSONType::GeometryCollection:
                m_cache = detail::GetArray<GeoJSONGeometry>(
                    m_coordinates,
                    [](const JSON& element) { return GeoJSONGeometry{ element }; });
                break;
            default:
                throw Error{ U"GeoJSONGeometry:get(): \"{}\" is not \"geometry type\""_fmt(m_coordinates.getString()) };
            }
        }

        return m_cache;
    }

    void GeoJSONGeometry::_Formatter(FormatData& formatData, const GeoJSONGeometry& geometry) {
        Formatter(formatData, geometry.getType());

        geometry.visit([&formatData](const auto& g) { Formatter(formatData, g); });
    }

    //////////////////////////////////////////////////
    //
    //	GeoJSONFeature
    //
    //////////////////////////////////////////////////

    GeoJSONFeature::GeoJSONFeature(const JSON& object)
        : GeoJSONBase({ object }) {
        if (m_type != GeoJSONType::Feature) {
            throw Error{ U"GeoJSONFeature::GeoJSONFeature(): \"type\" must be \"Feature\"" };
        }

        if (not object.hasElement(U"geometry")) {
            throw Error{ U"GeoJSONFeature::GeoJSONFeature(): \"Feature\" does not have \"geometry\"" };
        }

        m_geometry = GeoJSONGeometry{ object[U"geometry"] };

        if (object.hasElement(U"properties")) // 本来必須だが、properties を持たない GeoJSON ファイルもある
        {
            m_properties = object[U"properties"];
        }

        if (object.hasElement(U"id")) {
            switch (object[U"id"].getType()) {
            case JSONValueType::String:
                m_id = object[U"id"].getString();
                break;
            case JSONValueType::Number:
                m_id = object[U"id"].get<double>();
                break;
            default:
                throw Error{ U"GeoJSONFeature::GeoJSONFeature(): \"id\" must be either String type or Number type" };
            }
        }
    }

    const GeoJSONGeometry& GeoJSONFeature::getGeometry() const noexcept {
        return m_geometry;
    }

    const JSON& GeoJSONFeature::getProperties() const noexcept {
        return m_properties;
    }

    const std::variant<std::monostate, String, double>& GeoJSONFeature::getID() const noexcept {
        return m_id;
    }

    void GeoJSONFeature::_Formatter(FormatData& formatData, const GeoJSONFeature& feature) {
        Formatter(formatData, feature.getType());

        formatData.string.append(U": ");

        Formatter(formatData, feature.getGeometry());
    }

    //////////////////////////////////////////////////
    //
    //	GeoJSONFeatureCollection
    //
    //////////////////////////////////////////////////

    GeoJSONFeatureCollection::GeoJSONFeatureCollection(const JSON& object)
        : GeoJSONBase({ object }) {
        if (m_type != GeoJSONType::FeatureCollection) {
            throw Error{ U"\"type\" must be \"FeatureCollection\"" };
        }

        if (not object.hasElement(U"features")) {
            throw Error{ U"GeoJSONFeatureCollection::GeoJSONFeatureCollection(): \"FeatureCollection\" does not have \"features\"" };
        }

        m_features = detail::GetArray<GeoJSONFeature>(object[U"features"], [](const JSON& element) { return GeoJSONFeature{ element }; });
    }

    const Array<GeoJSONFeature>& GeoJSONFeatureCollection::getFeatures() const noexcept {
        return m_features;
    }

    void GeoJSONFeatureCollection::_Formatter(FormatData& formatData, const GeoJSONFeatureCollection& featureCollection) {
        Formatter(formatData, featureCollection.getType());

        formatData.string.append(U":");

        for (const auto& feature : featureCollection.getFeatures()) {
            formatData.string.append(U"\n  ");

            Formatter(formatData, feature);
        }
    }
}