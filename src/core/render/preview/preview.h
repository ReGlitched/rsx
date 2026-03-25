#pragma once

// Vertex Shader: Shader Resources
constexpr int VSRSRC_BONE_MATRIX            = 60u; // g_boneMatrix
constexpr int VSRSRC_BONE_MATRIX_PREV_FRAME = 62u; // g_boneMatrixPrevFrame

// Pixel Shader: Shader Resources
constexpr int PSRSRC_CUBEMAP              = 40u; // IndirectSpecularTextureCubeArray
constexpr int PSRSRC_CSMDEPTHATLASSAMPLER = 43u; // CSMAtlasDepthSampler
constexpr int PSRSRC_SHADOWMAP            = 44u; // t_shadowMaps
constexpr int PSRSRC_CLOUDMASK            = 45u; // cloudMaskTexture
constexpr int PSRSRC_STATICSHADOWTEXTURE  = 46u; // staticShadowTexture
constexpr int PSRSRC_GLOBAL_LIGHTS        = 62u; // s_globalLights
constexpr int PSRSRC_CUBEMAP_SAMPLES      = 72u; // g_cubemapSamples


void Preview_MapTransformsBuffer(CDXDrawData* drawData);
void Preview_MapModelInstanceBuffer(CDXDrawData* drawData);

// Preview types
void Preview_Model(CDXDrawData* drawData);
void Preview_Texture(CDXDrawData* drawData);
