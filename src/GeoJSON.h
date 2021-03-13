#pragma once
#include <Siv3D.hpp>
#include <variant>

// [ToDo] �ꕔ�̒n�`�\�����o�O�� (�����炭 OpenSiv3D �̖��Ȃ̂ŗv�����j
// [ToDo] �`��������₷�����W�ɕϊ�����֐�

namespace s3d {
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

    void Formatter(FormatData& formatData, const GeoJSONType& type);

    /// @brief GeoJSON �I�u�W�F�N�g�̊�{�N���X
    /// @detail https://tools.ietf.org/html/rfc7946#section-3
    class GeoJSONBase {
    public:

        /// @brief �f�t�H���g�R���X�g���N�^
        SIV3D_NODISCARD_CXX20
            GeoJSONBase() = default;

        /// @brief GeoJSON �I�u�W�F�N�g���쐬���܂��B
        /// @param object JSON �f�[�^
        SIV3D_NODISCARD_CXX20
            GeoJSONBase(const JSON& object);

        /// @brief �o�E���f�B���O�{�b�N�X�f�[�^��Ԃ��܂��B
        /// @return �o�E���f�B���O�{�b�N�X�f�[�^
        [[nodiscard]]
        const Array<double>& getBBox() const noexcept;

        /// @brief GeoJSON type ��Ԃ��܂��B
        /// @return ���̃I�u�W�F�N�g�� GeoJSON type
        [[nodiscard]]
        GeoJSONType getType() const noexcept;

    protected:

        /// @brief �o�E���f�B���O�{�b�N�X
        /// @detail https://tools.ietf.org/html/rfc7946#section-5
        Array<double> m_bbox;

        /// @brief GeoJSON type
        GeoJSONType m_type = GeoJSONType::Feature;
    };

    /// @brief GeoJSON Geometry �I�u�W�F�N�g
    /// @detail https://tools.ietf.org/html/rfc7946#section-3.1
    class GeoJSONGeometry : public GeoJSONBase {
    public:

        /// @brief �f�t�H���g�R���X�g���N�^
        SIV3D_NODISCARD_CXX20
            GeoJSONGeometry() = default;

        /// @brief Geometry �I�u�W�F�N�g���쐬���܂��B
        /// @param object JSON �f�[�^
        SIV3D_NODISCARD_CXX20
            GeoJSONGeometry(const JSON& object);

        /// @brief �`��f�[�^���擾���܂��B
        /// @tparam Type �`��f�[�^�̌^
        /// @return �`��f�[�^
        template <class Type>
        [[nodiscard]]
        Type get() const;

        /// @brief �`��f�[�^�����Ƃ� MultiPolygon ���쐬���ĕԂ��܂��B
        /// @return �`��f�[�^�����Ƃɍ쐬�����@MultiPolygon
        [[nodiscard]]
        MultiPolygon getPolygons() const;

        /// @brief �}�`�������ɂ��Ċ֐����Ăяo���܂��B
        /// @tparam Visitor Visitor �I�u�W�F�N�g�̌^
        /// @param visitor Visitor �I�u�W�F�N�g
        template <class Visitor>
        void visit(Visitor&& visitor) const {
            return std::visit(std::forward<Visitor>(visitor), getCache());
        }

        friend void Formatter(FormatData& formatData, const GeoJSONGeometry& geometry) {
            _Formatter(formatData, geometry);
        }

    private:

        using MonoState = int32; // std::visit �� Format �ł���悤�ɁB

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

    /// @brief GeoJSON Feature �I�u�W�F�N�g
    /// @detail https://tools.ietf.org/html/rfc7946#section-3.2
    class GeoJSONFeature : public GeoJSONBase {
    public:

        /// @brief GeoJSON Feature �I�u�W�F�N�g���쐬���܂��B
        /// @param object JSON �f�[�^
        SIV3D_NODISCARD_CXX20
            GeoJSONFeature(const JSON& object);

        /// @brief GeoJSONGeometry �f�[�^��Ԃ��܂��B
        /// @return GeoJSONGeometry �f�[�^
        [[nodiscard]]
        const GeoJSONGeometry& getGeometry() const noexcept;

        /// @brief GeoJSON Feature �� properties ��Ԃ��܂��B
        /// @return GeoJSON Feature �� properties
        [[nodiscard]]
        const JSON& getProperties() const noexcept;

        /// @brief GeoJSON Feature �� id ��Ԃ��܂��B
        /// @return GeoJSON Feature �� id
        [[nodiscard]]
        const std::variant<std::monostate, String, double>& getID() const noexcept;

        friend void Formatter(FormatData& formatData, const GeoJSONFeature& feature) {
            _Formatter(formatData, feature);
        }

    private:

        /// @brief Geometry �I�u�W�F�N�g
        GeoJSONGeometry m_geometry;

        /// @brief �v���p�e�B�I�u�W�F�N�g
        JSON m_properties;

        /// @brief ���ʎq
        std::variant<std::monostate, String, double> m_id;

        static void _Formatter(FormatData& formatData, const GeoJSONFeature& feature);
    };

    /// @brief FeatureCollection�I�u�W�F�N�g
    /// @detail https://tools.ietf.org/html/rfc7946#section-3.3
    class GeoJSONFeatureCollection : public GeoJSONBase {
    public:

        /// @brief FeatureCollection �I�u�W�F�N�g���쐬���܂��B
        /// @param object JSON �f�[�^
        SIV3D_NODISCARD_CXX20
            GeoJSONFeatureCollection(const JSON& object);

        /// @brief GeoJSON Feature �̈ꗗ��Ԃ��܂��B
        /// @return GeoJSON Feature �̈ꗗ
        [[nodiscard]]
        const Array<GeoJSONFeature>& getFeatures() const noexcept;

        friend void Formatter(FormatData& formatData, const GeoJSONFeatureCollection& featureCollection) {
            _Formatter(formatData, featureCollection);
        }

    private:

        /// @brief Feature�̔z��
        Array<GeoJSONFeature> m_features;

        static void _Formatter(FormatData& formatData, const GeoJSONFeatureCollection& featureCollection);
    };
}