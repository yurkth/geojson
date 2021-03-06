#pragma once
#include <Siv3D.hpp>
#include <variant>

// [ToDo] 一部の地形表示がバグる (おそらく OpenSiv3D の問題なので要調査）
// [ToDo] 形状を扱いやすい座標に変換する関数

namespace s3d {
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

    void Formatter(FormatData& formatData, const GeoJSONType& type);

    /// @brief GeoJSON オブジェクトの基本クラス
    /// @detail https://tools.ietf.org/html/rfc7946#section-3
    class GeoJSONBase {
    public:

        /// @brief デフォルトコンストラクタ
        SIV3D_NODISCARD_CXX20
            GeoJSONBase() = default;

        /// @brief GeoJSON オブジェクトを作成します。
        /// @param object JSON データ
        SIV3D_NODISCARD_CXX20
            GeoJSONBase(const JSON& object);

        /// @brief バウンディングボックスデータを返します。
        /// @return バウンディングボックスデータ
        [[nodiscard]]
        const Array<double>& getBBox() const noexcept;

        /// @brief GeoJSON type を返します。
        /// @return このオブジェクトの GeoJSON type
        [[nodiscard]]
        GeoJSONType getType() const noexcept;

    protected:

        /// @brief バウンディングボックス
        /// @detail https://tools.ietf.org/html/rfc7946#section-5
        Array<double> m_bbox;

        /// @brief GeoJSON type
        GeoJSONType m_type = GeoJSONType::Feature;
    };

    /// @brief GeoJSON Geometry オブジェクト
    /// @detail https://tools.ietf.org/html/rfc7946#section-3.1
    class GeoJSONGeometry : public GeoJSONBase {
    public:

        /// @brief デフォルトコンストラクタ
        SIV3D_NODISCARD_CXX20
            GeoJSONGeometry() = default;

        /// @brief Geometry オブジェクトを作成します。
        /// @param object JSON データ
        SIV3D_NODISCARD_CXX20
            GeoJSONGeometry(const JSON& object);

        /// @brief 形状データを取得します。
        /// @tparam Type 形状データの型
        /// @return 形状データ
        template <class Type>
        [[nodiscard]]
        Type get() const;

        /// @brief 形状データをもとに MultiPolygon を作成して返します。
        /// @return 形状データをもとに作成した　MultiPolygon
        [[nodiscard]]
        MultiPolygon getPolygons() const;

        /// @brief 図形を引数にして関数を呼び出します。
        /// @tparam Visitor Visitor オブジェクトの型
        /// @param visitor Visitor オブジェクト
        template <class Visitor>
        void visit(Visitor&& visitor) const {
            return std::visit(std::forward<Visitor>(visitor), getCache());
        }

        friend void Formatter(FormatData& formatData, const GeoJSONGeometry& geometry) {
            _Formatter(formatData, geometry);
        }

    private:

        using MonoState = int32; // std::visit で Format できるように。

        using GeometryVariant = std::variant<
            MonoState,
            Vec2,
            Array<Vec2>,
            LineString,
            Array<LineString>,
            Polygon,
            Array<Polygon>,
            Array<GeoJSONGeometry>>;

        JSON m_coordinates;

        mutable GeometryVariant m_cache;

        const GeometryVariant& getCache() const;

        static void _Formatter(FormatData& formatData, const GeoJSONGeometry& geometry);
    };

    /// @brief GeoJSON Feature オブジェクト
    /// @detail https://tools.ietf.org/html/rfc7946#section-3.2
    class GeoJSONFeature : public GeoJSONBase {
    public:

        /// @brief GeoJSON Feature オブジェクトを作成します。
        /// @param object JSON データ
        SIV3D_NODISCARD_CXX20
            GeoJSONFeature(const JSON& object);

        /// @brief GeoJSONGeometry データを返します。
        /// @return GeoJSONGeometry データ
        [[nodiscard]]
        const GeoJSONGeometry& getGeometry() const noexcept;

        /// @brief GeoJSON Feature の properties を返します。
        /// @return GeoJSON Feature の properties
        [[nodiscard]]
        const JSON& getProperties() const noexcept;

        /// @brief GeoJSON Feature の id を返します。
        /// @return GeoJSON Feature の id
        [[nodiscard]]
        const std::variant<std::monostate, String, double>& getID() const noexcept;

        friend void Formatter(FormatData& formatData, const GeoJSONFeature& feature) {
            _Formatter(formatData, feature);
        }

    private:

        /// @brief Geometry オブジェクト
        GeoJSONGeometry m_geometry;

        /// @brief プロパティオブジェクト
        JSON m_properties;

        /// @brief 識別子
        std::variant<std::monostate, String, double> m_id;

        static void _Formatter(FormatData& formatData, const GeoJSONFeature& feature);
    };

    /// @brief FeatureCollectionオブジェクト
    /// @detail https://tools.ietf.org/html/rfc7946#section-3.3
    class GeoJSONFeatureCollection : public GeoJSONBase {
    public:

        /// @brief FeatureCollection オブジェクトを作成します。
        /// @param object JSON データ
        SIV3D_NODISCARD_CXX20
            GeoJSONFeatureCollection(const JSON& object);

        /// @brief GeoJSON Feature の一覧を返します。
        /// @return GeoJSON Feature の一覧
        [[nodiscard]]
        const Array<GeoJSONFeature>& getFeatures() const noexcept;

        friend void Formatter(FormatData& formatData, const GeoJSONFeatureCollection& featureCollection) {
            _Formatter(formatData, featureCollection);
        }

    private:

        /// @brief Featureの配列
        Array<GeoJSONFeature> m_features;

        static void _Formatter(FormatData& formatData, const GeoJSONFeatureCollection& featureCollection);
    };
}