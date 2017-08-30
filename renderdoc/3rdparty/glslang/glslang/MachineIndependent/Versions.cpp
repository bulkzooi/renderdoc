//
// Copyright (C) 2002-2005  3Dlabs Inc. Ltd.
// Copyright (C) 2012-2013 LunarG, Inc.
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//
//    Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//
//    Redistributions in binary form must reproduce the above
//    copyright notice, this list of conditions and the following
//    disclaimer in the documentation and/or other materials provided
//    with the distribution.
//
//    Neither the name of 3Dlabs Inc. Ltd. nor the names of its
//    contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
// ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//

//
// Help manage multiple profiles, versions, extensions etc.
//
// These don't return error codes, as the presumption is parsing will
// always continue as if the tested feature were enabled, and thus there
// is no error recovery needed.
//

//
// HOW TO add a feature enabled by an extension.
//
// To add a new hypothetical "Feature F" to the front end, where an extension
// "XXX_extension_X" can be used to enable the feature, do the following.
//
// OVERVIEW: Specific features are what are error-checked for, not
//    extensions:  A specific Feature F might be enabled by an extension, or a
//    particular version in a particular profile, or a stage, or combinations, etc.
//
//    The basic mechanism is to use the following to "declare" all the things that
//    enable/disable Feature F, in a code path that implements Feature F:
//
//        requireProfile()
//        profileRequires()
//        requireStage()
//        checkDeprecated()
//        requireNotRemoved()
//        requireExtensions()
//
//    Typically, only the first two calls are needed.  They go into a code path that
//    implements Feature F, and will log the proper error/warning messages.  Parsing
//    will then always continue as if the tested feature was enabled.
//
//    There is typically no if-testing or conditional parsing, just insertion of the calls above.
//    However, if symbols specific to the extension are added (step 5), they will
//    only be added under tests that the minimum version and profile are present.
//
// 1) Add a symbol name for the extension string at the bottom of Versions.h:
//
//     const char* const XXX_extension_X = "XXX_extension_X";
//
// 2) Add extension initialization to TParseVersions::initializeExtensionBehavior(),
//    the first function below:
//
//     extensionBehavior[XXX_extension_X] = EBhDisable;
//
// 3) Add any preprocessor directives etc. in the next function, TParseVersions::getPreamble():
//
//           "#define XXX_extension_X 1\n"
//
//    The new-line is important, as that ends preprocess tokens.
//
// 4) Insert a profile check in the feature's path (unless all profiles support the feature,
//    for some version level).  That is, call requireProfile() to constrain the profiles, e.g.:
//
//         // ... in a path specific to Feature F...
//         requireProfile(loc,
//                        ECoreProfile | ECompatibilityProfile,
//                        "Feature F");
//
// 5) For each profile that supports the feature, insert version/extension checks:
//
//    The mostly likely scenario is that Feature F can only be used with a
//    particular profile if XXX_extension_X is present or the version is
//    high enough that the core specification already incorporated it.
//
//        // following the requireProfile() call...
//        profileRequires(loc,
//                        ECoreProfile | ECompatibilityProfile,
//                        420,             // 0 if no version incorporated the feature into the core spec.
//                        XXX_extension_X, // can be a list of extensions that all add the feature
//                        "Feature F Description");
//
//    This allows the feature if either A) one of the extensions is enabled or
//    B) the version is high enough.  If no version yet incorporates the feature
//    into core, pass in 0.
//
//    This can be called multiple times, if different profiles support the
//    feature starting at different version numbers or with different
//    extensions.
//
//    This must be called for each profile allowed by the initial call to requireProfile().
//
//    Profiles are all masks, which can be "or"-ed together.
//
//        ENoProfile
//        ECoreProfile
//        ECompatibilityProfile
//        EEsProfile
//
//    The ENoProfile profile is only for desktop, before profiles showed up in version 150;
//    All other #version with no profile default to either es or core, and so have profiles.
//
//    You can select all but a particular profile using ~.  The following basically means "desktop":
//
//        ~EEsProfile
//
// 6) If built-in symbols are added by the extension, add them in Initialize.cpp:  Their use
//    will be automatically error checked against the extensions enabled at that moment.
//    see the comment at the top of Initialize.cpp for where to put them.  Establish them at
//    the earliest release that supports the extension.  Then, tag them with the
//    set of extensions that both enable them and are necessary, given the version of the symbol
//    table. (There is a different symbol table for each version.)
//

#include "parseVersions.h"
#include "localintermediate.h"

namespace glslang {

//
// Initialize all extensions, almost always to 'disable', as once their features
// are incorporated into a core version, their features are supported through allowing that
// core version, not through a pseudo-enablement of the extension.
//
void TParseVersions::initializeExtensionBehavior()
{
    extensionBehavior[E_GL_OES_texture_3D]                   = EBhDisable;
    extensionBehavior[E_GL_OES_standard_derivatives]         = EBhDisable;
    extensionBehavior[E_GL_EXT_frag_depth]                   = EBhDisable;
    extensionBehavior[E_GL_OES_EGL_image_external]           = EBhDisable;
    extensionBehavior[E_GL_EXT_shader_texture_lod]           = EBhDisable;
    extensionBehavior[E_GL_EXT_shadow_samplers]              = EBhDisable;
    extensionBehavior[E_GL_ARB_texture_rectangle]            = EBhDisable;
    extensionBehavior[E_GL_3DL_array_objects]                = EBhDisable;
    extensionBehavior[E_GL_ARB_shading_language_420pack]     = EBhDisable;
    extensionBehavior[E_GL_ARB_texture_gather]               = EBhDisable;
    extensionBehavior[E_GL_ARB_gpu_shader5]                  = EBhDisablePartial;
    extensionBehavior[E_GL_ARB_separate_shader_objects]      = EBhDisable;
    extensionBehavior[E_GL_ARB_compute_shader]               = EBhDisable;
    extensionBehavior[E_GL_ARB_tessellation_shader]          = EBhDisable;
    extensionBehavior[E_GL_ARB_enhanced_layouts]             = EBhDisable;
    extensionBehavior[E_GL_ARB_texture_cube_map_array]       = EBhDisable;
    extensionBehavior[E_GL_ARB_shader_texture_lod]           = EBhDisable;
    extensionBehavior[E_GL_ARB_explicit_attrib_location]     = EBhDisable;
    extensionBehavior[E_GL_ARB_shader_image_load_store]      = EBhDisable;
    extensionBehavior[E_GL_ARB_shader_atomic_counters]       = EBhDisable;
    extensionBehavior[E_GL_ARB_shader_draw_parameters]       = EBhDisable;
    extensionBehavior[E_GL_ARB_shader_group_vote]            = EBhDisable;
    extensionBehavior[E_GL_ARB_derivative_control]           = EBhDisable;
    extensionBehavior[E_GL_ARB_shader_texture_image_samples] = EBhDisable;
    extensionBehavior[E_GL_ARB_viewport_array]               = EBhDisable;
    extensionBehavior[E_GL_ARB_gpu_shader_int64]             = EBhDisable;
    extensionBehavior[E_GL_ARB_shader_ballot]                = EBhDisable;
    extensionBehavior[E_GL_ARB_sparse_texture2]              = EBhDisable;
    extensionBehavior[E_GL_ARB_sparse_texture_clamp]         = EBhDisable;
    extensionBehavior[E_GL_ARB_shader_stencil_export]        = EBhDisable;
//    extensionBehavior[E_GL_ARB_cull_distance]                = EBhDisable;    // present for 4.5, but need extension control over block members
    extensionBehavior[E_GL_ARB_post_depth_coverage]          = EBhDisable;
    extensionBehavior[E_GL_ARB_shader_viewport_layer_array]  = EBhDisable;

    extensionBehavior[E_GL_EXT_shader_non_constant_global_initializers] = EBhDisable;
    extensionBehavior[E_GL_EXT_shader_image_load_formatted]             = EBhDisable;
    extensionBehavior[E_GL_EXT_post_depth_coverage]                     = EBhDisable;

    // #line and #include
    extensionBehavior[E_GL_GOOGLE_cpp_style_line_directive]          = EBhDisable;
    extensionBehavior[E_GL_GOOGLE_include_directive]                 = EBhDisable;

#ifdef AMD_EXTENSIONS
    extensionBehavior[E_GL_AMD_shader_ballot]                        = EBhDisable;
    extensionBehavior[E_GL_AMD_shader_trinary_minmax]                = EBhDisable;
    extensionBehavior[E_GL_AMD_shader_explicit_vertex_parameter]     = EBhDisable;
    extensionBehavior[E_GL_AMD_gcn_shader]                           = EBhDisable;
    extensionBehavior[E_GL_AMD_gpu_shader_half_float]                = EBhDisable;
    extensionBehavior[E_GL_AMD_texture_gather_bias_lod]              = EBhDisable;
    extensionBehavior[E_GL_AMD_gpu_shader_int16]                     = EBhDisable;
    extensionBehavior[E_GL_AMD_shader_image_load_store_lod]          = EBhDisable;
#endif

#ifdef NV_EXTENSIONS
    extensionBehavior[E_GL_NV_sample_mask_override_coverage]         = EBhDisable;
    extensionBehavior[E_SPV_NV_geometry_shader_passthrough]          = EBhDisable;
    extensionBehavior[E_GL_NV_viewport_array2]                       = EBhDisable;
    extensionBehavior[E_GL_NV_stereo_view_rendering]                 = EBhDisable;
    extensionBehavior[E_GL_NVX_multiview_per_view_attributes]        = EBhDisable;
#endif

    // AEP
    extensionBehavior[E_GL_ANDROID_extension_pack_es31a]             = EBhDisable;
    extensionBehavior[E_GL_KHR_blend_equation_advanced]              = EBhDisable;
    extensionBehavior[E_GL_OES_sample_variables]                     = EBhDisable;
    extensionBehavior[E_GL_OES_shader_image_atomic]                  = EBhDisable;
    extensionBehavior[E_GL_OES_shader_multisample_interpolation]     = EBhDisable;
    extensionBehavior[E_GL_OES_texture_storage_multisample_2d_array] = EBhDisable;
    extensionBehavior[E_GL_EXT_geometry_shader]                      = EBhDisable;
    extensionBehavior[E_GL_EXT_geometry_point_size]                  = EBhDisable;
    extensionBehavior[E_GL_EXT_gpu_shader5]                          = EBhDisable;
    extensionBehavior[E_GL_EXT_primitive_bounding_box]               = EBhDisable;
    extensionBehavior[E_GL_EXT_shader_io_blocks]                     = EBhDisable;
    extensionBehavior[E_GL_EXT_tessellation_shader]                  = EBhDisable;
    extensionBehavior[E_GL_EXT_tessellation_point_size]              = EBhDisable;
    extensionBehavior[E_GL_EXT_texture_buffer]                       = EBhDisable;
    extensionBehavior[E_GL_EXT_texture_cube_map_array]               = EBhDisable;

    // OES matching AEP
    extensionBehavior[E_GL_OES_geometry_shader]          = EBhDisable;
    extensionBehavior[E_GL_OES_geometry_point_size]      = EBhDisable;
    extensionBehavior[E_GL_OES_gpu_shader5]              = EBhDisable;
    extensionBehavior[E_GL_OES_primitive_bounding_box]   = EBhDisable;
    extensionBehavior[E_GL_OES_shader_io_blocks]         = EBhDisable;
    extensionBehavior[E_GL_OES_tessellation_shader]      = EBhDisable;
    extensionBehavior[E_GL_OES_tessellation_point_size]  = EBhDisable;
    extensionBehavior[E_GL_OES_texture_buffer]           = EBhDisable;
    extensionBehavior[E_GL_OES_texture_cube_map_array]   = EBhDisable;

    // EXT extensions
    extensionBehavior[E_GL_EXT_device_group]             = EBhDisable;
    extensionBehavior[E_GL_EXT_multiview]                = EBhDisable;

    // OVR extensions
    extensionBehavior[E_GL_OVR_multiview]                = EBhDisable;
    extensionBehavior[E_GL_OVR_multiview2]               = EBhDisable;
}

// Get code that is not part of a shared symbol table, is specific to this shader,
// or needed by the preprocessor (which does not use a shared symbol table).
void TParseVersions::getPreamble(std::string& preamble)
{
    if (profile == EEsProfile) {
        preamble =
            "#define GL_ES 1\n"
            "#define GL_FRAGMENT_PRECISION_HIGH 1\n"
            "#define GL_OES_texture_3D 1\n"
            "#define GL_OES_standard_derivatives 1\n"
            "#define GL_EXT_frag_depth 1\n"
            "#define GL_OES_EGL_image_external 1\n"
            "#define GL_EXT_shader_texture_lod 1\n"
            "#define GL_EXT_shadow_samplers 1\n"

            // AEP
            "#define GL_ANDROID_extension_pack_es31a 1\n"
            "#define GL_KHR_blend_equation_advanced 1\n"
            "#define GL_OES_sample_variables 1\n"
            "#define GL_OES_shader_image_atomic 1\n"
            "#define GL_OES_shader_multisample_interpolation 1\n"
            "#define GL_OES_texture_storage_multisample_2d_array 1\n"
            "#define GL_EXT_geometry_shader 1\n"
            "#define GL_EXT_geometry_point_size 1\n"
            "#define GL_EXT_gpu_shader5 1\n"
            "#define GL_EXT_primitive_bounding_box 1\n"
            "#define GL_EXT_shader_io_blocks 1\n"
            "#define GL_EXT_tessellation_shader 1\n"
            "#define GL_EXT_tessellation_point_size 1\n"
            "#define GL_EXT_texture_buffer 1\n"
            "#define GL_EXT_texture_cube_map_array 1\n"

            // OES matching AEP
            "#define GL_OES_geometry_shader 1\n"
            "#define GL_OES_geometry_point_size 1\n"
            "#define GL_OES_gpu_shader5 1\n"
            "#define GL_OES_primitive_bounding_box 1\n"
            "#define GL_OES_shader_io_blocks 1\n"
            "#define GL_OES_tessellation_shader 1\n"
            "#define GL_OES_tessellation_point_size 1\n"
            "#define GL_OES_texture_buffer 1\n"
            "#define GL_OES_texture_cube_map_array 1\n"
            "#define GL_EXT_shader_non_constant_global_initializers 1\n"
            ;
    } else {
        preamble =
            "#define GL_FRAGMENT_PRECISION_HIGH 1\n"
            "#define GL_ARB_texture_rectangle 1\n"
            "#define GL_ARB_shading_language_420pack 1\n"
            "#define GL_ARB_texture_gather 1\n"
            "#define GL_ARB_gpu_shader5 1\n"
            "#define GL_ARB_separate_shader_objects 1\n"
            "#define GL_ARB_compute_shader 1\n"
            "#define GL_ARB_tessellation_shader 1\n"
            "#define GL_ARB_enhanced_layouts 1\n"
            "#define GL_ARB_texture_cube_map_array 1\n"
            "#define GL_ARB_shader_texture_lod 1\n"
            "#define GL_ARB_explicit_attrib_location 1\n"
            "#define GL_ARB_shader_image_load_store 1\n"
            "#define GL_ARB_shader_atomic_counters 1\n"
            "#define GL_ARB_shader_draw_parameters 1\n"
            "#define GL_ARB_shader_group_vote 1\n"
            "#define GL_ARB_derivative_control 1\n"
            "#define GL_ARB_shader_texture_image_samples 1\n"
            "#define GL_ARB_viewport_array 1\n"
            "#define GL_ARB_gpu_shader_int64 1\n"
            "#define GL_ARB_shader_ballot 1\n"
            "#define GL_ARB_sparse_texture2 1\n"
            "#define GL_ARB_sparse_texture_clamp 1\n"
            "#define GL_ARB_shader_stencil_export 1\n"
//            "#define GL_ARB_cull_distance 1\n"    // present for 4.5, but need extension control over block members
            "#define GL_ARB_post_depth_coverage 1\n"
            "#define GL_EXT_shader_non_constant_global_initializers 1\n"
            "#define GL_EXT_shader_image_load_formatted 1\n"
            "#define GL_EXT_post_depth_coverage 1\n"

#ifdef AMD_EXTENSIONS
            "#define GL_AMD_shader_ballot 1\n"
            "#define GL_AMD_shader_trinary_minmax 1\n"
            "#define GL_AMD_shader_explicit_vertex_parameter 1\n"
            "#define GL_AMD_gcn_shader 1\n"
            "#define GL_AMD_gpu_shader_half_float 1\n"
            "#define GL_AMD_texture_gather_bias_lod 1\n"
            "#define GL_AMD_gpu_shader_int16 1\n"
            "#define GL_AMD_shader_image_load_store_lod 1\n"
#endif

#ifdef NV_EXTENSIONS
            "#define GL_NV_sample_mask_override_coverage 1\n"
            "#define GL_NV_geometry_shader_passthrough 1\n"
            "#define GL_NV_viewport_array2 1\n"
#endif
            ;

        if (version >= 150) {
            // define GL_core_profile and GL_compatibility_profile
            preamble += "#define GL_core_profile 1\n";

            if (profile == ECompatibilityProfile)
                preamble += "#define GL_compatibility_profile 1\n";
        }
    }

    if ((profile != EEsProfile && version >= 140) ||
        (profile == EEsProfile && version >= 310)) {
        preamble +=
            "#define GL_EXT_device_group 1\n"
            "#define GL_EXT_multiview 1\n"
            ;
    }

    if (version >= 300 /* both ES and non-ES */) {
        preamble +=
            "#define GL_OVR_multiview 1\n"
            "#define GL_OVR_multiview2 1\n"
            ;
    }

    // #line and #include
    preamble +=
            "#define GL_GOOGLE_cpp_style_line_directive 1\n"
            "#define GL_GOOGLE_include_directive 1\n"
            ;

    // #define VULKAN XXXX
    const int numberBufSize = 12;
    char numberBuf[numberBufSize];
    if (spvVersion.vulkanGlsl > 0) {
        preamble += "#define VULKAN ";
        snprintf(numberBuf, numberBufSize, "%d", spvVersion.vulkanGlsl);
        preamble += numberBuf;
        preamble += "\n";
    }
    // #define GL_SPIRV XXXX
    if (spvVersion.openGl > 0) {
        preamble += "#define GL_SPIRV ";
        snprintf(numberBuf, numberBufSize, "%d", spvVersion.openGl);
        preamble += numberBuf;
        preamble += "\n";
    }

}

//
// When to use requireProfile():
//
//     Use if only some profiles support a feature.  However, if within a profile the feature
//     is version or extension specific, follow this call with calls to profileRequires().
//
// Operation:  If the current profile is not one of the profileMask,
// give an error message.
//
void TParseVersions::requireProfile(const TSourceLoc& loc, int profileMask, const char* featureDesc)
{
    if (! (profile & profileMask))
        error(loc, "not supported with this profile:", featureDesc, ProfileName(profile));
}

//
// Map from stage enum to externally readable text name.
//
const char* StageName(EShLanguage stage)
{
    switch(stage) {
    case EShLangVertex:         return "vertex";
    case EShLangTessControl:    return "tessellation control";
    case EShLangTessEvaluation: return "tessellation evaluation";
    case EShLangGeometry:       return "geometry";
    case EShLangFragment:       return "fragment";
    case EShLangCompute:        return "compute";
    default:                    return "unknown stage";
    }
}

//
// When to use profileRequires():
//
//     If a set of profiles have the same requirements for what version or extensions
//     are needed to support a feature.
//
//     It must be called for each profile that needs protection.  Use requireProfile() first
//     to reduce that set of profiles.
//
// Operation: Will issue warnings/errors based on the current profile, version, and extension
// behaviors.  It only checks extensions when the current profile is one of the profileMask.
//
// A minVersion of 0 means no version of the profileMask support this in core,
// the extension must be present.
//

// entry point that takes multiple extensions
void TParseVersions::profileRequires(const TSourceLoc& loc, int profileMask, int minVersion, int numExtensions, const char* const extensions[], const char* featureDesc)
{
    if (profile & profileMask) {
        bool okay = false;
        if (minVersion > 0 && version >= minVersion)
            okay = true;
        for (int i = 0; i < numExtensions; ++i) {
            switch (getExtensionBehavior(extensions[i])) {
            case EBhWarn:
                infoSink.info.message(EPrefixWarning, ("extension " + TString(extensions[i]) + " is being used for " + featureDesc).c_str(), loc);
                // fall through
            case EBhRequire:
            case EBhEnable:
                okay = true;
                break;
            default: break; // some compilers want this
            }
        }

        if (! okay)
            error(loc, "not supported for this version or the enabled extensions", featureDesc, "");
    }
}

// entry point for the above that takes a single extension
void TParseVersions::profileRequires(const TSourceLoc& loc, int profileMask, int minVersion, const char* extension, const char* featureDesc)
{
    profileRequires(loc, profileMask, minVersion, extension ? 1 : 0, &extension, featureDesc);
}

//
// When to use requireStage()
//
//     If only some stages support a feature.
//
// Operation: If the current stage is not present, give an error message.
//
void TParseVersions::requireStage(const TSourceLoc& loc, EShLanguageMask languageMask, const char* featureDesc)
{
    if (((1 << language) & languageMask) == 0)
        error(loc, "not supported in this stage:", featureDesc, StageName(language));
}

// If only one stage supports a feature, this can be called.  But, all supporting stages
// must be specified with one call.
void TParseVersions::requireStage(const TSourceLoc& loc, EShLanguage stage, const char* featureDesc)
{
    requireStage(loc, static_cast<EShLanguageMask>(1 << stage), featureDesc);
}

//
// Within a set of profiles, see if a feature is deprecated and give an error or warning based on whether
// a future compatibility context is being use.
//
void TParseVersions::checkDeprecated(const TSourceLoc& loc, int profileMask, int depVersion, const char* featureDesc)
{
    if (profile & profileMask) {
        if (version >= depVersion) {
            if (forwardCompatible)
                error(loc, "deprecated, may be removed in future release", featureDesc, "");
            else if (! suppressWarnings())
                infoSink.info.message(EPrefixWarning, (TString(featureDesc) + " deprecated in version " +
                                                       String(depVersion) + "; may be removed in future release").c_str(), loc);
        }
    }
}

//
// Within a set of profiles, see if a feature has now been removed and if so, give an error.
// The version argument is the first version no longer having the feature.
//
void TParseVersions::requireNotRemoved(const TSourceLoc& loc, int profileMask, int removedVersion, const char* featureDesc)
{
    if (profile & profileMask) {
        if (version >= removedVersion) {
            const int maxSize = 60;
            char buf[maxSize];
            snprintf(buf, maxSize, "%s profile; removed in version %d", ProfileName(profile), removedVersion);
            error(loc, "no longer supported in", featureDesc, buf);
        }
    }
}

void TParseVersions::unimplemented(const TSourceLoc& loc, const char* featureDesc)
{
    error(loc, "feature not yet implemented", featureDesc, "");
}

// Returns true if at least one of the extensions in the extensions parameter is requested. Otherwise, returns false.
// Warns appropriately if the requested behavior of an extension is "warn".
bool TParseVersions::checkExtensionsRequested(const TSourceLoc& loc, int numExtensions, const char* const extensions[], const char* featureDesc)
{
    // First, see if any of the extensions are enabled
    for (int i = 0; i < numExtensions; ++i) {
        TExtensionBehavior behavior = getExtensionBehavior(extensions[i]);
        if (behavior == EBhEnable || behavior == EBhRequire)
            return true;
    }

    // See if any extensions want to give a warning on use; give warnings for all such extensions
    bool warned = false;
    for (int i = 0; i < numExtensions; ++i) {
        TExtensionBehavior behavior = getExtensionBehavior(extensions[i]);
        if (behavior == EBhDisable && relaxedErrors()) {
            infoSink.info.message(EPrefixWarning, "The following extension must be enabled to use this feature:", loc);
            behavior = EBhWarn;
        }
        if (behavior == EBhWarn) {
            infoSink.info.message(EPrefixWarning, ("extension " + TString(extensions[i]) + " is being used for " + featureDesc).c_str(), loc);
            warned = true;
        }
    }
    if (warned)
        return true;
    return false;
}

//
// Use when there are no profile/version to check, it's just an error if one of the
// extensions is not present.
//
void TParseVersions::requireExtensions(const TSourceLoc& loc, int numExtensions, const char* const extensions[], const char* featureDesc)
{
    if (checkExtensionsRequested(loc, numExtensions, extensions, featureDesc)) return;

    // If we get this far, give errors explaining what extensions are needed
    if (numExtensions == 1)
        error(loc, "required extension not requested:", featureDesc, extensions[0]);
    else {
        error(loc, "required extension not requested:", featureDesc, "Possible extensions include:");
        for (int i = 0; i < numExtensions; ++i)
            infoSink.info.message(EPrefixNone, extensions[i]);
    }
}

//
// Use by preprocessor when there are no profile/version to check, it's just an error if one of the
// extensions is not present.
//
void TParseVersions::ppRequireExtensions(const TSourceLoc& loc, int numExtensions, const char* const extensions[], const char* featureDesc)
{
    if (checkExtensionsRequested(loc, numExtensions, extensions, featureDesc)) return;

    // If we get this far, give errors explaining what extensions are needed
    if (numExtensions == 1)
        ppError(loc, "required extension not requested:", featureDesc, extensions[0]);
    else {
        ppError(loc, "required extension not requested:", featureDesc, "Possible extensions include:");
        for (int i = 0; i < numExtensions; ++i)
            infoSink.info.message(EPrefixNone, extensions[i]);
    }
}

TExtensionBehavior TParseVersions::getExtensionBehavior(const char* extension)
{
    auto iter = extensionBehavior.find(TString(extension));
    if (iter == extensionBehavior.end())
        return EBhMissing;
    else
        return iter->second;
}

// Returns true if the given extension is set to enable, require, or warn.
bool TParseVersions::extensionTurnedOn(const char* const extension)
{
      switch (getExtensionBehavior(extension)) {
      case EBhEnable:
      case EBhRequire:
      case EBhWarn:
          return true;
      default:
          break;
      }
      return false;
}
// See if any of the extensions are set to enable, require, or warn.
bool TParseVersions::extensionsTurnedOn(int numExtensions, const char* const extensions[])
{
    for (int i = 0; i < numExtensions; ++i) {
        if (extensionTurnedOn(extensions[i])) return true;
    }
    return false;
}

//
// Change the current state of an extension's behavior.
//
void TParseVersions::updateExtensionBehavior(int line, const char* extension, const char* behaviorString)
{
    // Translate from text string of extension's behavior to an enum.
    TExtensionBehavior behavior = EBhDisable;
    if (! strcmp("require", behaviorString))
        behavior = EBhRequire;
    else if (! strcmp("enable", behaviorString))
        behavior = EBhEnable;
    else if (! strcmp("disable", behaviorString))
        behavior = EBhDisable;
    else if (! strcmp("warn", behaviorString))
        behavior = EBhWarn;
    else {
        error(getCurrentLoc(), "behavior not supported:", "#extension", behaviorString);
        return;
    }

    // update the requested extension
    updateExtensionBehavior(extension, behavior);

    // see if need to propagate to implicitly modified things
    if (strcmp(extension, "GL_ANDROID_extension_pack_es31a") == 0) {
        // to everything in AEP
        updateExtensionBehavior(line, "GL_KHR_blend_equation_advanced", behaviorString);
        updateExtensionBehavior(line, "GL_OES_sample_variables", behaviorString);
        updateExtensionBehavior(line, "GL_OES_shader_image_atomic", behaviorString);
        updateExtensionBehavior(line, "GL_OES_shader_multisample_interpolation", behaviorString);
        updateExtensionBehavior(line, "GL_OES_texture_storage_multisample_2d_array", behaviorString);
        updateExtensionBehavior(line, "GL_EXT_geometry_shader", behaviorString);
        updateExtensionBehavior(line, "GL_EXT_gpu_shader5", behaviorString);
        updateExtensionBehavior(line, "GL_EXT_primitive_bounding_box", behaviorString);
        updateExtensionBehavior(line, "GL_EXT_shader_io_blocks", behaviorString);
        updateExtensionBehavior(line, "GL_EXT_tessellation_shader", behaviorString);
        updateExtensionBehavior(line, "GL_EXT_texture_buffer", behaviorString);
        updateExtensionBehavior(line, "GL_EXT_texture_cube_map_array", behaviorString);
    }
    // geometry to io_blocks
    else if (strcmp(extension, "GL_EXT_geometry_shader") == 0)
        updateExtensionBehavior(line, "GL_EXT_shader_io_blocks", behaviorString);
    else if (strcmp(extension, "GL_OES_geometry_shader") == 0)
        updateExtensionBehavior(line, "GL_OES_shader_io_blocks", behaviorString);
    // tessellation to io_blocks
    else if (strcmp(extension, "GL_EXT_tessellation_shader") == 0)
        updateExtensionBehavior(line, "GL_EXT_shader_io_blocks", behaviorString);
    else if (strcmp(extension, "GL_OES_tessellation_shader") == 0)
        updateExtensionBehavior(line, "GL_OES_shader_io_blocks", behaviorString);
    else if (strcmp(extension, "GL_GOOGLE_include_directive") == 0)
        updateExtensionBehavior(line, "GL_GOOGLE_cpp_style_line_directive", behaviorString);
}

void TParseVersions::updateExtensionBehavior(const char* extension, TExtensionBehavior behavior)
{
    // Update the current behavior
    if (strcmp(extension, "all") == 0) {
        // special case for the 'all' extension; apply it to every extension present
        if (behavior == EBhRequire || behavior == EBhEnable) {
            error(getCurrentLoc(), "extension 'all' cannot have 'require' or 'enable' behavior", "#extension", "");
            return;
        } else {
            for (auto iter = extensionBehavior.begin(); iter != extensionBehavior.end(); ++iter)
                iter->second = behavior;
        }
    } else {
        // Do the update for this single extension
        auto iter = extensionBehavior.find(TString(extension));
        if (iter == extensionBehavior.end()) {
            switch (behavior) {
            case EBhRequire:
                error(getCurrentLoc(), "extension not supported:", "#extension", extension);
                break;
            case EBhEnable:
            case EBhWarn:
            case EBhDisable:
                warn(getCurrentLoc(), "extension not supported:", "#extension", extension);
                break;
            default:
                assert(0 && "unexpected behavior");
            }

            return;
        } else {
            if (iter->second == EBhDisablePartial)
                warn(getCurrentLoc(), "extension is only partially supported:", "#extension", extension);
            if (behavior == EBhEnable || behavior == EBhRequire)
                intermediate.addRequestedExtension(extension);
            iter->second = behavior;
        }
    }
}

// Call for any operation needing full GLSL integer data-type support.
void TParseVersions::fullIntegerCheck(const TSourceLoc& loc, const char* op)
{
    profileRequires(loc, ENoProfile, 130, nullptr, op);
    profileRequires(loc, EEsProfile, 300, nullptr, op);
}

// Call for any operation needing GLSL double data-type support.
void TParseVersions::doubleCheck(const TSourceLoc& loc, const char* op)
{
    requireProfile(loc, ECoreProfile | ECompatibilityProfile, op);
    profileRequires(loc, ECoreProfile, 400, nullptr, op);
    profileRequires(loc, ECompatibilityProfile, 400, nullptr, op);
}

#ifdef AMD_EXTENSIONS
// Call for any operation needing GLSL 16-bit integer data-type support.
void TParseVersions::int16Check(const TSourceLoc& loc, const char* op, bool builtIn)
{
    if (! builtIn) {
        requireExtensions(loc, 1, &E_GL_AMD_gpu_shader_int16, "shader int16");
        requireProfile(loc, ECoreProfile | ECompatibilityProfile, op);
        profileRequires(loc, ECoreProfile, 450, nullptr, op);
        profileRequires(loc, ECompatibilityProfile, 450, nullptr, op);
    }
}

// Call for any operation needing GLSL float16 data-type support.
void TParseVersions::float16Check(const TSourceLoc& loc, const char* op, bool builtIn)
{
    if (! builtIn) {
        requireExtensions(loc, 1, &E_GL_AMD_gpu_shader_half_float, "shader half float");
        requireProfile(loc, ECoreProfile | ECompatibilityProfile, op);
        profileRequires(loc, ECoreProfile, 450, nullptr, op);
        profileRequires(loc, ECompatibilityProfile, 450, nullptr, op);
    }
}
#endif

// Call for any operation needing GLSL 64-bit integer data-type support.
void TParseVersions::int64Check(const TSourceLoc& loc, const char* op, bool builtIn)
{
    if (! builtIn) {
        requireExtensions(loc, 1, &E_GL_ARB_gpu_shader_int64, "shader int64");
        requireProfile(loc, ECoreProfile | ECompatibilityProfile, op);
        profileRequires(loc, ECoreProfile, 450, nullptr, op);
        profileRequires(loc, ECompatibilityProfile, 450, nullptr, op);
    }
}

// Call for any operation removed because SPIR-V is in use.
void TParseVersions::spvRemoved(const TSourceLoc& loc, const char* op)
{
    if (spvVersion.spv != 0)
        error(loc, "not allowed when generating SPIR-V", op, "");
}

// Call for any operation removed because Vulkan SPIR-V is being generated.
void TParseVersions::vulkanRemoved(const TSourceLoc& loc, const char* op)
{
    if (spvVersion.vulkan >= 100)
        error(loc, "not allowed when using GLSL for Vulkan", op, "");
}

// Call for any operation that requires Vulkan.
void TParseVersions::requireVulkan(const TSourceLoc& loc, const char* op)
{
    if (spvVersion.vulkan == 0)
        error(loc, "only allowed when using GLSL for Vulkan", op, "");
}

// Call for any operation that requires SPIR-V.
void TParseVersions::requireSpv(const TSourceLoc& loc, const char* op)
{
    if (spvVersion.spv == 0)
        error(loc, "only allowed when generating SPIR-V", op, "");
}

} // end namespace glslang
