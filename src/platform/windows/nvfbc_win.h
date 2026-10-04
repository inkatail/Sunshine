/**
 * @file src/platform/windows/nvfbc_win.h
 * @brief Minimal NVIDIA Framebuffer Capture (NvFBC) declarations for Windows.
 *
 * @details Sunshine loads `NvFBC64.dll` / `NvFBC.dll` from the NVIDIA driver at
 * runtime via `LoadLibrary` / `GetProcAddress`, so no NVIDIA SDK is needed at
 * build time and the binary still starts on non-NVIDIA systems.
 *
 * The struct layouts below must match the NVIDIA Capture SDK (Windows)
 * binary interface. Field order, sizes, and the `NVFBC_STRUCT_VERSION` packing
 * (`sizeof | (ver << 16) | (0x70 << 24)`) follow the openly vendored SDK
 * headers mirrored by the nvfbc-relay sample project and the LookingGlass
 * host capture implementation, which are used here as ABI references.
 * `static_assert`s on the key sizes guard against drift.
 *
 * References:
 * - NVIDIA Capture SDK Programming Guide (flow: Enable -> GetStatusEx ->
 *   CreateEx -> Setup -> GrabFrame -> Release; one session per head; create
 *   before the streamed app goes fullscreen).
 * - nvfbc-relay samples (NvFBCLibrary loader: System32/NvFBC64.dll,
 *   SysWOW64/NvFBC.dll, NVFBC_TARGET_ADAPTER env selection).
 * - LookingGlass NVFBC host capture (ToSys reference usage, NVFBC_PRIV_DATA
 *   env convention for the GeForce unlock key).
 * - nvidia-patch nvfbcwrp (GeForce `pPrivateData` magic + wrapper technique).
 *
 * Only the ToSys (system-memory) target is declared here. Dx9Vid/HWEnc can be
 * added later following the same pattern.
 */
#pragma once

// platform includes
#include <windows.h>

namespace platf::nvfbc_win {
  using nv_u8_t = unsigned char;  ///< 8-bit unsigned value.
  using nv_u32_t = unsigned long;  ///< 32-bit unsigned value.
  using nv_u64_t = unsigned long long;  ///< 64-bit unsigned value.

  auto constexpr NVFBC_DLL_VERSION = 0x70;  ///< NvFBC API version for struct packing.

  /**
   * @brief Pack an NvFBC struct version from its size and revision.
   *
   * @param size Byte size of the struct.
   * @param ver Revision of the struct.
   * @return Packed version for `dwVersion` fields.
   */
  constexpr nv_u32_t struct_version(nv_u32_t size, nv_u32_t ver) {
    return size | (ver << 16) | (NVFBC_DLL_VERSION << 24);
  }

  /**
   * @brief Calling convention for NvFBC entry points.
   */
#define NVFBC_WIN_API __stdcall

  /**
   * @brief Status codes returned by NvFBC APIs.
   */
  enum nvfbc_result_t : int {
    NVFBC_WIN_SUCCESS = 0,  ///< Success.
    NVFBC_WIN_ERROR_GENERIC = -1,  ///< Unexpected failure.
    NVFBC_WIN_ERROR_INVALID_PARAM = -2,  ///< Bad parameter (includes NULL).
    NVFBC_WIN_ERROR_INVALIDATED_SESSION = -3,  ///< Session invalid, recreate it.
    NVFBC_WIN_ERROR_PROTECTED_CONTENT = -4,  ///< Protected content, skip frame.
    NVFBC_WIN_ERROR_DRIVER_FAILURE = -5,  ///< GPU driver failure.
    NVFBC_WIN_ERROR_UNSUPPORTED = -7,  ///< API unsupported by this NvFBC.
    NVFBC_WIN_ERROR_INCOMPATIBLE_DRIVER = -9,  ///< Driver incompatible with NvFBC.
    NVFBC_WIN_ERROR_OUT_OF_MEMORY = -11,  ///< Allocation failure.
    NVFBC_WIN_ERROR_INCOMPATIBLE_VERSION = -13,  ///< Bad `dwVersion` in param struct.
    NVFBC_WIN_ERROR_INVALID_TARGET = -18,  ///< Adapter cannot be used for capture.
    NVFBC_WIN_ERROR_DYNAMIC_DISABLE = -20,  ///< NvFBC dynamically disabled, session dead.
  };

  /**
   * @brief NvFBC enable states for `NvFBC_Enable`.
   */
  enum nvfbc_state_t : nv_u32_t {
    NVFBC_WIN_STATE_DISABLE = 0,  ///< Disable NvFBC.
    NVFBC_WIN_STATE_ENABLE = 1,  ///< Enable NvFBC.
  };

  /**
   * @brief Per-frame capture status reported by grab calls.
   */
  struct nvfbc_frame_grab_info_t {
    DWORD dwWidth;  ///< Current captured buffer width.
    DWORD dwHeight;  ///< Current captured buffer height.
    DWORD dwBufferWidth;  ///< Padded pixel-buffer width.
    DWORD dwReserved;  ///< Reserved.
    BOOL bOverlayActive;  ///< Overlay was active.
    BOOL bMustRecreate;  ///< Session must be recreated via CreateEx.
    BOOL bFirstBuffer;  ///< First capture call or first after mode change.
    BOOL bHWMouseVisible;  ///< HW cursor enabled by OS at grab time.
    BOOL bProtectedContent;  ///< Protected content active.
    DWORD dwDriverInternalError;  ///< Lower-layer diagnostic status.
    BOOL bStereoOn;  ///< Stereo was on.
    BOOL bIGPUCapture;  ///< Frame captured from iGPU.
    DWORD dwSourcePID;  ///< PID that caused the last captured screen update.
    DWORD dwReserved3;  ///< Reserved.
    DWORD bIsHDR : 1;  ///< Grabbed content is HDR.
    DWORD bReservedBit1 : 1;  ///< Reserved.
    DWORD bReservedBits : 30;  ///< Reserved.
    DWORD dwWaitModeUsed;  ///< Blocking mode used for this grab.
    nv_u32_t dwReserved2[11];  ///< Reserved, zero.
  };

  static_assert(sizeof(nvfbc_frame_grab_info_t) == 108, "NvFBCFrameGrabInfo layout mismatch");

  /**
   * @brief Adapter status queried via `NvFBC_GetStatusEx`.
   */
  struct nvfbc_status_t {
    nv_u32_t dwVersion;  ///< Struct version, set to `NVFBC_STATUS_VER`.
    nv_u32_t bIsCapturePossible : 1;  ///< NvFBC feature enabled for this adapter.
    nv_u32_t bCurrentlyCapturing : 1;  ///< NvFBC capturing on this adapter ordinal.
    nv_u32_t bCanCreateNow : 1;  ///< Deprecated, do not rely on it.
    nv_u32_t bSupportMultiHead : 1;  ///< Multi-head grab supported.
    nv_u32_t bSupportConfigurableDiffMap : 1;  ///< Configurable diff-map block size.
    nv_u32_t bSupportImageClassification : 1;  ///< Classification map supported.
    nv_u32_t bReservedBits : 26;  ///< Reserved.
    nv_u32_t dwNvFBCVersion;  ///< Highest NvFBC interface version supported.
    nv_u32_t dwAdapterIdx;  ///< Adapter ordinal to query.
    void *pPrivateData;  ///< Optional private data.
    nv_u32_t dwPrivateDataSize;  ///< Optional private data size.
    nv_u32_t dwReserved[59];  ///< Reserved, zero.
    void *pReserved[31];  ///< Reserved, null.
  };

  static_assert(sizeof(nvfbc_status_t) == 512, "NvFBCStatusEx layout mismatch");

  /**
   * @brief Parameters for `NvFBC_CreateEx`.
   */
  struct nvfbc_create_params_t {
    nv_u32_t dwVersion;  ///< Struct version, set to `NVFBC_CREATE_PARAMS_VER`.
    nv_u32_t dwInterfaceType;  ///< Requested interface id (e.g. `NVFBC_TO_SYS`).
    nv_u32_t dwMaxDisplayWidth;  ///< Maximum display width allowed (out).
    nv_u32_t dwMaxDisplayHeight;  ///< Maximum display height allowed (out).
    void *pDevice;  ///< Device pointer (D3D9 device for Dx9/CUDA paths, null for ToSys).
    void *pPrivateData;  ///< GeForce unlock key (optional on Quadro).
    nv_u32_t dwPrivateDataSize;  ///< Size of private data (16 for the GeForce key).
    nv_u32_t dwInterfaceVersion;  ///< Capture interface version (0 selects default).
    void *pNvFBC;  ///< Created interface object (out).
    nv_u32_t dwAdapterIdx;  ///< Adapter ordinal to capture (ignored if pDevice set).
    nv_u32_t dwNvFBCVersion;  ///< Highest supported interface version (out).
    void *cudaCtx;  ///< CUDA context for the CUDA interface only.
    void *pPrivateData2;  ///< Extra private data (optional).
    nv_u32_t dwPrivateData2Size;  ///< Extra private data size.
    nv_u32_t dwReserved[55];  ///< Reserved, zero.
    void *pReserved[27];  ///< Reserved, null.
  };

  static_assert(sizeof(nvfbc_create_params_t) == 512, "NvFBCCreateParams layout mismatch");

  /**
   * @brief Entry point signatures exported by NvFBC64.dll / NvFBC.dll.
   */
  using nvfbc_create_ex_fn = nvfbc_result_t(NVFBC_WIN_API *)(void *params);  ///< `NvFBC_CreateEx`.
  using nvfbc_get_status_ex_fn = nvfbc_result_t(NVFBC_WIN_API *)(nvfbc_status_t *status);  ///< `NvFBC_GetStatusEx`.
  using nvfbc_enable_fn = nvfbc_result_t(NVFBC_WIN_API *)(nvfbc_state_t state);  ///< `NvFBC_Enable`.
  using nvfbc_get_sdk_version_fn = nvfbc_result_t(NVFBC_WIN_API *)(nv_u32_t *version);  ///< `NvFBC_GetSDKVersion`.
  using nvfbc_set_global_flags_fn = void(NVFBC_WIN_API *)(DWORD flags);  ///< `NvFBC_SetGlobalFlags`.

  auto constexpr NVFBC_TO_SYS = 0x1205;  ///< Interface id for the ToSys target.

  /**
   * @brief Output pixel formats for the ToSys target.
   */
  enum tosys_buffer_format_t : nv_u32_t {
    TOSYS_ARGB = 0,  ///< 32bpp ARGB (memory order B,G,R,A; matches B8G8R8A8_UNORM).
    TOSYS_RGB = 1,  ///< 24bpp RGB.
    TOSYS_YYYYUV420p = 2,  ///< 12bpp YUV420 planar.
    TOSYS_RGB_PLANAR = 3,  ///< 24bpp planar RGB.
    TOSYS_XOR = 4,  ///< 24bpp XOR against prior frame.
    TOSYS_YUV444p = 5,  ///< 8bpc YUV444 planar.
    TOSYS_ARGB10 = 6,  ///< 32bpp A2B10G10R10.
  };

  /**
   * @brief Grab modes for the ToSys target.
   */
  enum tosys_grab_mode_t : nv_u32_t {
    TOSYS_SOURCEMODE_FULL = 0,  ///< Grab full resolution.
    TOSYS_SOURCEMODE_SCALE = 1,  ///< Scale to target width/height.
    TOSYS_SOURCEMODE_CROP = 2,  ///< Crop subwindow at start X/Y of target size.
  };

  /**
   * @brief Grab flags for the ToSys target.
   */
  enum tosys_grab_flags_t : nv_u32_t {
    TOSYS_NOFLAGS = 0x0,  ///< Wait for a new frame or HW cursor move.
    TOSYS_NOWAIT = 0x1,  ///< Never wait.
    TOSYS_WAIT_WITH_TIMEOUT = 0x10,  ///< Wait up to `dwWaitTime` milliseconds.
  };

  /**
   * @brief Setup parameters for a ToSys capture session.
   */
  struct tosys_setup_params_t {
    nv_u32_t dwVersion;  ///< Struct version, set to `NVFBC_TOSYS_SETUP_PARAMS_VER`.
    nv_u32_t bWithHWCursor : 1;  ///< Composite the HW cursor into captured frames.
    nv_u32_t bDiffMap : 1;  ///< Enable the diff-map feature.
    nv_u32_t bEnableSeparateCursorCapture : 1;  ///< Enable separate cursor stream.
    nv_u32_t bHDRRequest : 1;  ///< Request HDR capture.
    nv_u32_t bClassificationMap : 1;  ///< Enable the classification map.
    nv_u32_t bReservedBits : 27;  ///< Reserved.
    tosys_buffer_format_t eMode;  ///< Output image format.
    nv_u32_t eDiffMapBlockSize;  ///< Diff-map block size (0 selects 128x128 default).
    nv_u32_t dwClassificationMapStampWidth;  ///< Classification stamp width.
    nv_u32_t dwClassificationMapStampHeight;  ///< Classification stamp height.
    void **ppBuffer;  ///< NvFBC output buffers (out).
    void **ppDiffMap;  ///< Diff-map buffers (out).
    void *hCursorCaptureEvent;  ///< Signaled on cursor updates (out).
    void **ppClassificationMap;  ///< Classification map buffers (out).
    nv_u32_t dwReserved[56];  ///< Reserved, zero.
    void *pReserved[28];  ///< Reserved, null.
  };

  static_assert(sizeof(tosys_setup_params_t) == 504, "ToSys setup params layout mismatch");

  /**
   * @brief Per-call parameters for a ToSys grab.
   */
  struct tosys_grab_params_t {
    nv_u32_t dwVersion;  ///< Struct version, set to `NVFBC_TOSYS_GRAB_FRAME_PARAMS_VER`.
    nv_u32_t dwFlags;  ///< Bit-mask of `tosys_grab_flags_t` values.
    nv_u32_t dwTargetWidth;  ///< Target width (SCALE/CROP modes).
    nv_u32_t dwTargetHeight;  ///< Target height (SCALE/CROP modes).
    nv_u32_t dwStartX;  ///< Crop origin X (CROP mode).
    nv_u32_t dwStartY;  ///< Crop origin Y (CROP mode).
    tosys_grab_mode_t eGMode;  ///< Frame grab mode.
    nv_u32_t dwWaitTime;  ///< Wait limit in ms (WAIT_WITH_TIMEOUT).
    nvfbc_frame_grab_info_t *pNvFBCFrameGrabInfo;  ///< Grab feedback (in/out).
    nv_u32_t dwReserved[56];  ///< Reserved, zero.
    void *pReserved[31];  ///< Reserved, null.
  };

  static_assert(sizeof(tosys_grab_params_t) == 512, "ToSys grab params layout mismatch");

  /**
   * @brief ToSys capture interface (5-method COM-style vtable).
   *
   * @details Returned in `NvFBCCreateParams::pNvFBC` when `dwInterfaceType` is
   * `NVFBC_TO_SYS`. Methods are invoked through the vtable in declaration
   * order, matching both MSVC and MinGW object layout for single inheritance.
   */
  class tosys_interface_t {
  public:
    /**
     * @brief Configure the ToSys capture session.
     *
     * @param params Setup parameters.
     * @return NvFBC status code.
     */
    virtual nvfbc_result_t NVFBC_WIN_API setup(tosys_setup_params_t *params) = 0;
    /**
     * @brief Capture the desktop into the session system-memory buffer.
     *
     * @param params Grab parameters and feedback.
     * @return NvFBC status code.
     */
    virtual nvfbc_result_t NVFBC_WIN_API grab(tosys_grab_params_t *params) = 0;
    /**
     * @brief Capture HW cursor data on shape change.
     *
     * @param params Cursor buffer, reserved for future use (pass null).
     * @return NvFBC status code.
     */
    virtual nvfbc_result_t NVFBC_WIN_API cursor_capture(void *params) = 0;
    /**
     * @brief High-precision GPU-based CPU sleep.
     *
     * @param microseconds Sleep duration in microseconds.
     * @return NvFBC status code.
     */
    virtual nvfbc_result_t NVFBC_WIN_API gpu_sleep(long long microseconds) = 0;
    /**
     * @brief Destroy the ToSys capture session.
     *
     * @return NvFBC status code.
     */
    virtual nvfbc_result_t NVFBC_WIN_API release() = 0;
  };

  auto constexpr NVFBC_TO_DX9_VID = 0x2003;  ///< Interface id for the Dx9Vid target.
  auto constexpr DX9VID_MAX_DIFF_MAP_SIZE = 0x00040000;  ///< Suggested client diff-map allocation.

  /**
   * @brief Output pixel formats for the Dx9Vid target.
   */
  enum dx9vid_buffer_format_t : nv_u32_t {
    DX9VID_ARGB = 0,  ///< 32-bit packed ARGB.
    DX9VID_NV12 = 1,  ///< YUV 4:2:0.
    DX9VID_ARGB10 = 2,  ///< 32-bit packed A2B10G10R10.
  };

  /**
   * @brief Grab modes for the Dx9Vid target.
   */
  enum dx9vid_grab_mode_t : nv_u32_t {
    DX9VID_SOURCEMODE_FULL = 0,  ///< Grab full resolution.
    DX9VID_SOURCEMODE_SCALE = 1,  ///< Scale to target width/height.
    DX9VID_SOURCEMODE_CROP = 2,  ///< Crop subwindow at start X/Y of target size.
  };

  /**
   * @brief Grab flags for the Dx9Vid target.
   */
  enum dx9vid_grab_flags_t : nv_u32_t {
    DX9VID_NOFLAGS = 0x0,  ///< Wait for a new frame or HW cursor move.
    DX9VID_NOWAIT = 0x1,  ///< Never wait.
    DX9VID_WAIT_WITH_TIMEOUT = 0x10,  ///< Wait up to `dwWaitTime` milliseconds.
  };

  /**
   * @brief Client output buffer addresses for the Dx9Vid target.
   *
   * @details The surfaces must be client-created D3D9 surfaces; NvFBC copies
   * the grabbed desktop into the selected one. Shared D3D9Ex textures qualify,
   * which is what makes D3D11 zero-copy pickup possible.
   */
  struct dx9vid_out_buf_t {
    void *primary;  ///< Grabbed desktop image surface (IDirect3DSurface9 *).
    void *secondary;  ///< Reserved, set to null.
  };

  static_assert(sizeof(dx9vid_out_buf_t) == 16, "Dx9Vid output buffer layout mismatch");

  /**
   * @brief Setup parameters for a Dx9Vid capture session.
   */
  struct dx9vid_setup_params_t {
    nv_u32_t dwVersion;  ///< Struct version, set to `NVFBC_TODX9VID_SETUP_PARAMS_VER`.
    nv_u32_t bWithHWCursor : 1;  ///< Composite the HW cursor into captured frames.
    nv_u32_t bStereoGrab : 1;  ///< Reserved, zero.
    nv_u32_t bDiffMap : 1;  ///< Enable the diff-map feature.
    nv_u32_t bEnableSeparateCursorCapture : 1;  ///< Enable separate cursor stream.
    nv_u32_t bHDRRequest : 1;  ///< Request HDR capture.
    nv_u32_t bClassificationMap : 1;  ///< Enable the classification map.
    nv_u32_t bReservedBits : 26;  ///< Reserved.
    dx9vid_buffer_format_t eMode;  ///< Output image format.
    nv_u32_t dwNumBuffers;  ///< Number of client output buffers.
    nv_u32_t eDiffMapBlockSize;  ///< Diff-map block size (0 selects 128x128).
    nv_u32_t eStereoFmt;  ///< Reserved, zero.
    nv_u32_t dwDiffMapBuffSize;  ///< Client diff-map allocation size.
    nv_u32_t dwClassificationMapBuffSize;  ///< Client classification allocation.
    nv_u32_t dwClassificationMapStampWidth;  ///< Classification stamp width.
    nv_u32_t dwClassificationMapStampHeight;  ///< Classification stamp height.
    void **ppDiffMap;  ///< Client diff-map buffers (VirtualAlloc, one per buffer).
    void **ppClassificationMap;  ///< Client classification buffers.
    dx9vid_out_buf_t *ppBuffer;  ///< Client output buffers.
    void *hCursorCaptureEvent;  ///< Signaled on cursor updates (out).
    nv_u32_t dwReserved[22];  ///< Reserved, zero.
    void *pReserved[12];  ///< Reserved, null.
  };

  static_assert(sizeof(dx9vid_setup_params_t) == 256, "Dx9Vid setup params layout mismatch");

  /**
   * @brief Per-call parameters for a Dx9Vid grab.
   */
  struct dx9vid_grab_params_t {
    nv_u32_t dwVersion;  ///< Struct version, set to `NVFBC_TODX9VID_GRAB_FRAME_PARAMS_VER`.
    nv_u32_t dwFlags;  ///< Bit-mask of `dx9vid_grab_flags_t` values.
    nv_u32_t dwTargetWidth;  ///< Target width (SCALE/CROP modes).
    nv_u32_t dwTargetHeight;  ///< Target height (SCALE/CROP modes).
    nv_u32_t dwStartX;  ///< Crop origin X (CROP mode).
    nv_u32_t dwStartY;  ///< Crop origin Y (CROP mode).
    dx9vid_grab_mode_t eGMode;  ///< Frame grab mode.
    nv_u32_t dwBufferIdx;  ///< Output buffer index to grab into.
    nvfbc_frame_grab_info_t *pNvFBCFrameGrabInfo;  ///< Grab feedback (in/out).
    nv_u32_t dwWaitTime;  ///< Wait limit in ms (WAIT_WITH_TIMEOUT).
    nv_u32_t dwReserved[23];  ///< Reserved, zero.
    void *pReserved[15];  ///< Reserved, null.
  };

  static_assert(sizeof(dx9vid_grab_params_t) == 256, "Dx9Vid grab params layout mismatch");

  /**
   * @brief Dx9Vid capture interface (5-method COM-style vtable).
   */
  class dx9vid_interface_t {
  public:
    /**
     * @brief Register client output buffers for the session.
     *
     * @param params Setup parameters.
     * @return NvFBC status code.
     */
    virtual nvfbc_result_t NVFBC_WIN_API setup(dx9vid_setup_params_t *params) = 0;
    /**
     * @brief Capture the desktop into the selected client buffer.
     *
     * @param params Grab parameters and feedback.
     * @return NvFBC status code.
     */
    virtual nvfbc_result_t NVFBC_WIN_API grab(dx9vid_grab_params_t *params) = 0;
    /**
     * @brief High-precision GPU-based CPU sleep.
     *
     * @param microseconds Sleep duration in microseconds.
     * @return NvFBC status code.
     */
    virtual nvfbc_result_t NVFBC_WIN_API gpu_sleep(long long microseconds) = 0;
    /**
     * @brief Destroy the Dx9Vid capture session.
     *
     * @return NvFBC status code.
     */
    virtual nvfbc_result_t NVFBC_WIN_API release() = 0;
    /**
     * @brief Capture HW cursor data on shape change.
     *
     * @param params Reserved for future use (pass null).
     * @return NvFBC status code.
     */
    virtual nvfbc_result_t NVFBC_WIN_API cursor_capture(void *params) = 0;
  };

#undef NVFBC_WIN_API
}  // namespace platf::nvfbc_win
