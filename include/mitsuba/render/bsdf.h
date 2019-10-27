/*
    Mitsuba 2: A Retargetable Forward and Inverse Renderer
    Copyright 2019, Realistic Graphics Lab, EPFL.

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE.txt file.
*/

#pragma once

#include <mitsuba/render/interaction.h>

NAMESPACE_BEGIN(mitsuba)

/**
 * \brief Specifies the transport mode when sampling or
 * evaluating a scattering function
 */
enum class TransportMode : uint32_t {
    /// Radiance transport
    Radiance,

    /// Importance transport
    Importance,

    /// Specifies the number of supported transport modes
    TransportModes = 2
};

/**
 * \brief This list of flags is used to classify the different types of lobes
 * that are implemented in a BSDF instance.
 *
 * They are also useful for picking out individual components, e.g., by setting
 * combinations in \ref BSDFContext::type_mask.
 */
enum class BSDFFlags : uint32_t {
    // =============================================================
    //                      BSDF lobe types
    // =============================================================

    /// No flags set (default value)
    None                 = 0x00000,

    /// 'null' scattering event, i.e. particles do not undergo deflection
    Null                 = 0x00001,

    /// Ideally diffuse reflection
    DiffuseReflection    = 0x00002,

    /// Ideally diffuse transmission
    DiffuseTransmission  = 0x00004,

    /// Glossy reflection
    GlossyReflection     = 0x00008,

    /// Glossy transmission
    GlossyTransmission   = 0x00010,

    /// Reflection into a discrete set of directions
    DeltaReflection      = 0x00020,

    /// Transmission into a discrete set of directions
    DeltaTransmission    = 0x00040,

    /// Reflection into a 1D space of directions
    Delta1DReflection    = 0x00080,

    /// Transmission into a 1D space of directions
    Delta1DTransmission  = 0x00100,

    // =============================================================
    //!                   Other lobe attributes
    // =============================================================

    /// The lobe is not invariant to rotation around the normal
    Anisotropic          = 0x01000,

    /// The BSDF depends on the UV coordinates
    SpatiallyVarying     = 0x02000,

    /// Flags non-symmetry (e.g. transmission in dielectric materials)
    NonSymmetric         = 0x04000,

    /// Supports interactions on the front-facing side
    FrontSide            = 0x08000,

    /// Supports interactions on the back-facing side
    BackSide             = 0x10000,

    /// Does the implementation require access to texture-space differentials
    NeedsDifferentials   = 0x20000,

    // =============================================================
    //!                 Compound lobe attributes
    // =============================================================

    /// Any reflection component (scattering into discrete, 1D, or 2D set of directions)
    Reflection   = DiffuseReflection | DeltaReflection |
                   Delta1DReflection | GlossyReflection,

    /// Any transmission component (scattering into discrete, 1D, or 2D set of directions)
    Transmission = DiffuseTransmission | DeltaTransmission |
                   Delta1DTransmission | GlossyTransmission | Null,

    /// Diffuse scattering into a 2D set of directions
    Diffuse      = DiffuseReflection | DiffuseTransmission,

    /// Non-diffuse scattering into a 2D set of directions
    Glossy       = GlossyReflection | GlossyTransmission,

    /// Scattering into a 2D set of directions
    Smooth       = Diffuse | Glossy,

    /// Scattering into a discrete set of directions
    Delta        = Null | DeltaReflection | DeltaTransmission,

    /// Scattering into a 1D space of directions
    Delta1D      = Delta1DReflection | Delta1DTransmission,

    /// Any kind of scattering
    All          = Diffuse | Glossy | Delta | Delta1D
};

/// Allows or-ing of BSDFFlags
constexpr BSDFFlags operator |(BSDFFlags f1, BSDFFlags f2) {
    return static_cast<BSDFFlags>(static_cast<uint32_t>(f1) | static_cast<uint32_t>(f2));
}
/// Allows and-ing of BSDFFlags
constexpr BSDFFlags operator &(BSDFFlags f1, BSDFFlags f2) {
    return static_cast<BSDFFlags>(static_cast<uint32_t>(f1) & static_cast<uint32_t>(f2));
}
/// Allows and-ing of BSDFFlags
template <typename UInt32>
constexpr UInt32 operator &(const UInt32 &f1, BSDFFlags f2) {
    return (f1 & static_cast<uint32_t>(f2));
}
/// Allows not-ing of BSDFFlags
constexpr BSDFFlags operator ~(BSDFFlags f1) {
    return static_cast<BSDFFlags>(~static_cast<uint32_t>(f1));
}
/// Allows using unary `+` for conversion from BSDFFlags to the underlying type
constexpr auto operator+(BSDFFlags e) noexcept {
    return static_cast<std::underlying_type_t<BSDFFlags>>(e);
}

/// Check presence of a flag in a combined BSDFFlag
constexpr bool has_flag(BSDFFlags flags, BSDFFlags f) {
    return (static_cast<uint32_t>(flags) & static_cast<uint32_t>(f)) != 0;
}
template <typename UInt32>
constexpr bool has_flag(const UInt32 &flags, BSDFFlags f) {
    return neq(flags & UInt32(static_cast<uint32_t>(f)), 0u);
}

/**
 * \brief Context data structure for BSDF evaluation and sampling
 *
 * BSDF models in Mitsuba can be queried and sampled using a variety of
 * different modes -- for instance, a rendering algorithm can indicate whether
 * radiance or importance is being transported, and it can also restrict
 * evaluation and sampling to a subset of lobes in a a multi-lobe BSDF model.
 *
 * The \ref BSDFContext data structure encodes these preferences and is
 * supplied to most \ref BSDF methods.
 */
struct MTS_EXPORT_RENDER BSDFContext {
    // =============================================================
    //! @{ \name Fields
    // =============================================================

    /// Transported mode (radiance or importance)
    TransportMode mode;

    /*
     * Bit mask for requested BSDF component types to be sampled/evaluated
     * The default value (equal to \ref BSDFFlags::All) enables all components.
     */
    uint32_t type_mask = (uint32_t) 0x1FFu;

    /// Integer value of requested BSDF component index to be sampled/evaluated.
    uint32_t component = (uint32_t) -1;

    //! @}
    // =============================================================

    BSDFContext(TransportMode mode = TransportMode::Radiance)
        : mode(mode) { }

    BSDFContext(TransportMode mode, uint32_t type_mask, uint32_t component)
        : mode(mode), type_mask(type_mask), component(component) { }

    /**
     * \brief Reverse the direction of light transport in the record
     *
     * This updates the transport mode (radiance to importance and vice versa).
     */
    void reverse() {
        mode = (TransportMode) (1 - (int) mode);
    }

    /**
     * Checks whether a given BSDF component type and BSDF component index are
     * enabled in this context.
     */
    bool is_enabled(BSDFFlags type_, uint32_t component_ = 0) const {
        auto type_number = static_cast<uint32_t>(type_);
        return (type_mask == (uint32_t) -1 || (type_mask & type_number) == type_number)
            && (component == (uint32_t) -1 || component == component_);
    }
};

/// Data structure holding the result of BSDF sampling operations.
template <typename Float, typename Spectrum> struct BSDFSample3 {
    // =============================================================
    //! @{ \name Type declarations
    // =============================================================

    using Vector3f = Vector<Float, 3>;
    using UInt32   = uint32_array_t<Float>;

    //! @}
    // =============================================================

    // =============================================================
    //! @{ \name Fields
    // =============================================================

    /// Normalized outgoing direction in local coordinates
    Vector3f wo;

    /// Probability density at the sample
    Float pdf;

    /// Relative index of refraction in the sampled direction
    Float eta;

    /// Stores the component type that was sampled by \ref BSDF::sample()
    UInt32 sampled_type;

    /// Stores the component index that was sampled by \ref BSDF::sample()
    UInt32 sampled_component;

    //! @}
    // =============================================================

    // =============================================================
    //! @{ \name Methods
    // =============================================================

    /**
     * \brief Given a surface interaction an an incident/exitant direction
     * pair (wi, wo), create a query record to evaluate the BSDF or its
     * sampling density.
     *
     * By default, all components will be sampled regardless of what measure
     * they live on.
     *
     * \param wo
     *      An outgoing direction in local coordinates. This should
     *      be a normalized direction vector that points \a away from
     *      the scattering event.
     */
    BSDFSample3(const Vector3f &wo)
        : wo(wo), pdf(0.f), eta(1.f), sampled_type(0),
          sampled_component(uint32_t(-1)) { }


    //! @}
    // =============================================================

    ENOKI_STRUCT(BSDFSample3, wo, pdf, eta, sampled_type, sampled_component);
};


/**
 * \brief Bidirectional Scattering Distribution Function (BSDF) interface
 *
 * This class provides an abstract interface to all %BSDF plugins in Mitsuba.
 * It exposes functions for evaluating and sampling the model, and for querying
 * associated probability densities.
 *
 * By default, functions in class sample and evaluate the complete BSDF, but it
 * also allows to pick and choose individual components of multi-lobed BSDFs
 * based on their properties and component indices. This selection is specified
 * using a context data structure that is provided along with every operation.
 *
 * When polarization is enabled, BSDF sampling and evaluation returns 4x4
 * Mueller matrices that describe how scattering changes the polarization state
 * of incident light. Mueller matrices (e.g. for mirrors) are expressed with
 * respect to a reference coordinate system for the incident and outgoing
 * direction. The convention used here is that these coordinate systems are
 * given by <tt>coordinate_system(wi)</tt> and <tt>coordinate_system(wo)</tt>,
 * where 'wi' and 'wo' are the incident and outgoing direction in local
 * coordinates.
 *
 * \sa BSDFContext
 * \sa BSDFSample3
 */
template <typename Float, typename Spectrum>
// class MTS_EXPORT_RENDER BSDF : public DifferentiableObject {
class MTS_EXPORT_RENDER BSDF : public Object {
public:
    MTS_REGISTER_CLASS(BSDF, Object)
    MTS_IMPORT_TYPES();

    /**
     * \brief Importance sample the BSDF model
     *
     * The function returns a sample data structure along with the importance
     * weight, which is the value of the BSDF divided by the probability
     * density, and multiplied by the cosine foreshortening factor (if needed
     * --- it is omitted for degenerate BSDFs like smooth mirrors/dielectrics).
     *
     * If the supplied context data strutcures selects subset of components in
     * a multi-lobe BRDF model, the sampling is restricted to this subset.
     * Depending on the provided transport type, either the BSDF or its adjoint
     * version is sampled.
     *
     * When sampling a continuous/non-delta component, this method also
     * multiplies by the cosine foreshorening factor with respect to the
     * sampled direction.
     *
     * \param ctx
     *     A context data structure describing which lobes to sample,
     *     and whether radiance or importance are being transported.
     *
     * \param si
     *     A surface interaction data structure describing the underlying
     *     surface position. The incident direction is obtained from
     *     the field <tt>si.wi</tt>.
     *
     * \param sample1
     *     A uniformly distributed sample on \f$[0,1]\f$. It is used
     *     to select the BSDF lobe in multi-lobe models.
     *
     * \param sample2
     *     A uniformly distributed sample on \f$[0,1]^2\f$. It is
     *     used to generate the sampled direction.
     *
     * \return A pair (bs, value) consisting of
     *
     *     bs:    Sampling record, indicating the sampled direction, PDF values
     *            and other information. The contents are undefined if sampling
     *            failed.
     *
     *     value: The BSDF value (multiplied by the cosine foreshortening
     *            factor when a non-delta component is sampled). A zero
     *            spectrum indicates that sampling failed.
     */
    virtual std::pair<BSDFSample3f, Spectrum>
    sample(const BSDFContext &ctx,
           const SurfaceInteraction3f &si,
           Float sample1,
           const Point2f &sample2,
           Mask active = true) const = 0;

    /**
     * \brief Evaluate the BSDF f(wi, wo) or its adjoint version f^{*}(wi, wo)
     * and multiply by the cosine foreshortening term.
     *
     * Based on the information in the supplied query context \c ctx, this
     * method will either evaluate the entire BSDF or query individual
     * components (e.g. the diffuse lobe). Only smooth (i.e. non Dirac-delta)
     * components are supported: calling \c eval() on a perfectly specular
     * material will return zero.
     *
     * Note that the incident direction does not need to be explicitly
     * specified. It is obtained from the field <tt>si.wi</tt>.
     *
     * \param ctx
     *     A context data structure describing which lobes to evalute,
     *     and whether radiance or importance are being transported.
     *
     * \param si
     *     A surface interaction data structure describing the underlying
     *     surface position. The incident direction is obtained from
     *     the field <tt>si.wi</tt>.
     *
     * \param wo
     *     The outgoing direction
     */
    virtual Spectrum eval(const BSDFContext &ctx,
                          const SurfaceInteraction3f &si,
                          const Vector3f &wo,
                          Mask active = true) const = 0;

    /**
     * \brief Compute the probability per unit solid angle of sampling a
     * given direction
     *
     * This method provides access to the probability density that would result
     * when supplying the same BSDF context and surface interaction data
     * structures to the \ref sample() method. It correctly handles changes in
     * probability when only a subset of the components is chosen for sampling
     * (this can be done using the \ref BSDFContext::component and \ref
     * BSDFContext::type_mask fields).
     *
     * Note that the incident direction does not need to be explicitly
     * specified. It is obtained from the field <tt>si.wi</tt>.
     *
     * \param ctx
     *     A context data structure describing which lobes to evalute,
     *     and whether radiance or importance are being transported.
     *
     * \param si
     *     A surface interaction data structure describing the underlying
     *     surface position. The incident direction is obtained from
     *     the field <tt>si.wi</tt>.
     *
     * \param wo
     *     The outgoing direction
     */
    virtual Float pdf(const BSDFContext &ctx,
                      const SurfaceInteraction3f &si,
                      const Vector3f &wo,
                      Mask active = true) const = 0;

    /**
     * \brief Evaluate un-scattered transmission component of the BSDF
     *
     * This method will evaluate the un-scattered transmission (\ref
     * BSDFFlags::Null) of the BSDF for light arriving from direction \c w.
     * The default implementation returns zero.
     *
     * \param si
     *     A surface interaction data structure describing the underlying
     *     surface position. The incident direction is obtained from
     *     the field <tt>si.wi</tt>.
     */
    virtual Spectrum eval_tr(const SurfaceInteraction3f &si,
                             Mask active = true) const;

    // -----------------------------------------------------------------------
    //! @{ \name BSDF property accessors (components, flags, etc)
    // -----------------------------------------------------------------------

    /// Flags for all components combined.
    BSDFFlags flags(Mask /*active*/ = true) const { return m_flags; }

    /// Flags for a specific component of this BSDF.
    BSDFFlags flags(size_t i, Mask /*active*/ = true) const {
        Assert(i < m_components.size());
        return m_components[i];
    }

    /// Does the implementation require access to texture-space differentials?
    bool needs_differentials(Mask /* active */ = true) const {
        return m_flags & BSDFFlags::NeedsDifferentials;
    }

    /// Number of components this BSDF is comprised of.
    size_t component_count(Mask /* active */ = true) const {
        return m_components.size();
    }

    /// Return a string identifier
    std::string id() const override { return m_id; }

    /// Return a human-readable representation of the BSDF
    std::string to_string() const override = 0;

    //! @}
    // -----------------------------------------------------------------------

    ENOKI_CALL_SUPPORT_FRIEND()

protected:
    BSDF(const Properties &props);
    virtual ~BSDF();

protected:
    /// Combined flags for all components of this BSDF.
    BSDFFlags m_flags;

    /// Flags for each component of this BSDF.
    std::vector<BSDFFlags> m_components;

    /// Identifier (if available)
    std::string m_id;
};

// -----------------------------------------------------------------------
//! @{ \name Misc implementations
// -----------------------------------------------------------------------

extern MTS_EXPORT_RENDER std::ostream &operator<<(
    std::ostream &os, const TransportMode &mode);

extern MTS_EXPORT_RENDER std::ostream &operator<<(std::ostream &os,
                                                  const BSDFContext& ctx);

template <typename Float, typename Spectrum>
std::ostream &operator<<(std::ostream &os, const BSDFSample3<Float, Spectrum>& bs) {
    os << "BSDFSample[" << std::endl
        << "  wo = " << bs.wo << "," << std::endl
        << "  pdf = " << bs.pdf << "," << std::endl
        << "  eta = " << bs.eta << "," << std::endl
        << "  sampled_type = " << type_mask_to_string(bs.sampled_type) << "," << std::endl
        << "  sampled_component = " << bs.sampled_component << std::endl
        << "]";
    return os;
}

//template <typename Point3>
//typename SurfaceInteraction<Point3>::BSDFPtr
//SurfaceInteraction<Point3>::bsdf(const RayDifferential3 &ray) {
//    const BSDFPtr bsdf = shape->bsdf();
//
//    /// TODO: revisit the 'false' default for autodiff mode once there are actually BRDFs using differentials
//    if constexpr (!is_diff_array_v<Point3>) {
//        if (!has_uv_partials() && any(bsdf->needs_differentials()))
//            compute_partials(ray);
//    }
//
//    return bsdf;
//}

//! @}
// -----------------------------------------------------------------------
NAMESPACE_END(mitsuba)

#include "detail/bsdf.inl"
