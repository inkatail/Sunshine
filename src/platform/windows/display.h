/**
 * @file src/platform/windows/display.h
 * @brief Declarations for the Windows display backend.
 */
#pragma once

// platform includes
#include <d3d11.h>
#include <d3d11_4.h>
#include <d3dcommon.h>
#include <dwmapi.h>
#include <dxgi.h>
#include <dxgi1_6.h>
#include <unknwn.h>
// windows-legacy: WinRT WGC headers require Win10 SDK + cppwinrt and Windowsapp.lib.
// Excluded when built with -DSUNSHINE_ENABLE_WGC=OFF (Win7/8.x builds).
#ifndef SUNSHINE_NO_WGC
  #include <winrt/windows.graphics.capture.h>
#endif

// local includes
#include "src/platform/common.h"
#include "src/utility.h"
#include "src/video.h"

namespace platf::dxgi {
  extern const char *format_str[];

  // Add D3D11_CREATE_DEVICE_DEBUG here to enable the D3D11 debug runtime.
  // You should have a debugger like WinDbg attached to receive debug messages.
  auto constexpr D3D11_CREATE_DEVICE_FLAGS = 0;  ///< Protocol or platform constant for d3 d11 create device flags.

  /**
   * @brief Release the COM or platform reference owned by the pointer.
   *
   * @param dxgi DXGI object whose COM reference should be released.
   */
  template<class T>
  void Release(T *dxgi) {
    dxgi->Release();
  }

  /**
   * @brief Create a D3D11 device with downlevel fallbacks.
   *
   * @details windows-legacy: vanilla Windows 7 (without KB2670838) rejects
   * `D3D_FEATURE_LEVEL_11_1`, and pre-8 runtimes reject
   * `D3D11_CREATE_DEVICE_VIDEO_SUPPORT`, both with `E_INVALIDARG`. This helper
   * retries without them so the same binary runs on 7/8.x and 10+.
   * @param adapter DXGI adapter to create the device on, or null for default.
   * @param flags D3D11 creation flags.
   * @param device Created device on success.
   * @param out_level Negotiated feature level on success.
   * @param device_ctx Immediate device context on success (may be null).
   * @return `S_OK` on success, else the last D3D11 failure.
   */
  HRESULT d3d11_create_device_retry(IDXGIAdapter *adapter, UINT flags, ID3D11Device **device, D3D_FEATURE_LEVEL *out_level, ID3D11DeviceContext **device_ctx);

  /**
   * @brief Owning COM pointer for the DXGI factory used to enumerate adapters.
   */
  using factory1_t = util::safe_ptr<IDXGIFactory1, Release<IDXGIFactory1>>;
  /**
   * @brief Owning COM pointer for a DXGI device interface.
   */
  using dxgi_t = util::safe_ptr<IDXGIDevice, Release<IDXGIDevice>>;
  /**
   * @brief Owning COM pointer for a DXGI 1.1 device interface.
   */
  using dxgi1_t = util::safe_ptr<IDXGIDevice1, Release<IDXGIDevice1>>;
  /**
   * @brief Owning COM pointer for the D3D11 device used by capture and conversion.
   */
  using device_t = util::safe_ptr<ID3D11Device, Release<ID3D11Device>>;
  /**
   * @brief Owning COM pointer for the D3D11.1 device interface.
   */
  using device1_t = util::safe_ptr<ID3D11Device1, Release<ID3D11Device1>>;
  /**
   * @brief Owning COM pointer for the immediate D3D11 device context.
   */
  using device_ctx_t = util::safe_ptr<ID3D11DeviceContext, Release<ID3D11DeviceContext>>;
  /**
   * @brief Owning COM pointer for a DXGI display adapter.
   */
  using adapter_t = util::safe_ptr<IDXGIAdapter1, Release<IDXGIAdapter1>>;
  /**
   * @brief Owning COM pointer for a DXGI output.
   */
  using output_t = util::safe_ptr<IDXGIOutput, Release<IDXGIOutput>>;
  /**
   * @brief Owning COM pointer for a DXGI output that supports duplication.
   */
  using output1_t = util::safe_ptr<IDXGIOutput1, Release<IDXGIOutput1>>;
  /**
   * @brief Owning COM pointer for a DXGI output with HDR metadata support.
   */
  using output5_t = util::safe_ptr<IDXGIOutput5, Release<IDXGIOutput5>>;
  /**
   * @brief Owning COM pointer for a DXGI output with modern display descriptors.
   */
  using output6_t = util::safe_ptr<IDXGIOutput6, Release<IDXGIOutput6>>;
  /**
   * @brief Owning COM pointer for the DXGI desktop duplication session.
   */
  using dup_t = util::safe_ptr<IDXGIOutputDuplication, Release<IDXGIOutputDuplication>>;
  /**
   * @brief Owning COM pointer for a D3D11 2D texture.
   */
  using texture2d_t = util::safe_ptr<ID3D11Texture2D, Release<ID3D11Texture2D>>;
  /**
   * @brief Owning COM pointer for a D3D11 1D texture.
   */
  using texture1d_t = util::safe_ptr<ID3D11Texture1D, Release<ID3D11Texture1D>>;
  /**
   * @brief Owning COM pointer for the DXGI resource view of a shared texture.
   */
  using resource_t = util::safe_ptr<IDXGIResource, Release<IDXGIResource>>;
  /**
   * @brief Owning COM pointer for the DXGI 1.1 resource sharing interface.
   */
  using resource1_t = util::safe_ptr<IDXGIResource1, Release<IDXGIResource1>>;
  /**
   * @brief Owning COM pointer for the D3D11 multithread-protection interface.
   */
  using multithread_t = util::safe_ptr<ID3D11Multithread, Release<ID3D11Multithread>>;
  /**
   * @brief Owning COM pointer for a D3D11 vertex shader.
   */
  using vs_t = util::safe_ptr<ID3D11VertexShader, Release<ID3D11VertexShader>>;
  /**
   * @brief Owning COM pointer for a D3D11 pixel shader.
   */
  using ps_t = util::safe_ptr<ID3D11PixelShader, Release<ID3D11PixelShader>>;
  /**
   * @brief Owning COM pointer for a D3D11 blend state.
   */
  using blend_t = util::safe_ptr<ID3D11BlendState, Release<ID3D11BlendState>>;
  /**
   * @brief Owning COM pointer for a D3D11 input layout.
   */
  using input_layout_t = util::safe_ptr<ID3D11InputLayout, Release<ID3D11InputLayout>>;
  /**
   * @brief Owning COM pointer for a D3D11 render-target view.
   */
  using render_target_t = util::safe_ptr<ID3D11RenderTargetView, Release<ID3D11RenderTargetView>>;
  /**
   * @brief Owning COM pointer for a D3D11 shader-resource view.
   */
  using shader_res_t = util::safe_ptr<ID3D11ShaderResourceView, Release<ID3D11ShaderResourceView>>;
  /**
   * @brief Owning COM pointer for a D3D11 buffer.
   */
  using buf_t = util::safe_ptr<ID3D11Buffer, Release<ID3D11Buffer>>;
  /**
   * @brief D3D rasterizer state handle type.
   */
  using raster_state_t = util::safe_ptr<ID3D11RasterizerState, Release<ID3D11RasterizerState>>;
  /**
   * @brief D3D sampler state handle type.
   */
  using sampler_state_t = util::safe_ptr<ID3D11SamplerState, Release<ID3D11SamplerState>>;
  /**
   * @brief Owning COM pointer for a compiled shader bytecode blob.
   */
  using blob_t = util::safe_ptr<ID3DBlob, Release<ID3DBlob>>;
  /**
   * @brief D3D depth-stencil state handle type.
   */
  using depth_stencil_state_t = util::safe_ptr<ID3D11DepthStencilState, Release<ID3D11DepthStencilState>>;
  /**
   * @brief Owning COM pointer for a D3D11 depth-stencil view.
   */
  using depth_stencil_view_t = util::safe_ptr<ID3D11DepthStencilView, Release<ID3D11DepthStencilView>>;
  /**
   * @brief Owning COM pointer for a keyed mutex on a shared DXGI resource.
   */
  using keyed_mutex_t = util::safe_ptr<IDXGIKeyedMutex, Release<IDXGIKeyedMutex>>;

  namespace video {
    /**
     * @brief Owning COM pointer for the D3D11 video device interface.
     */
    using device_t = util::safe_ptr<ID3D11VideoDevice, Release<ID3D11VideoDevice>>;
    /**
     * @brief Owning COM pointer for the D3D11 video context interface.
     */
    using ctx_t = util::safe_ptr<ID3D11VideoContext, Release<ID3D11VideoContext>>;
    /**
     * @brief Owning COM pointer for a D3D11 video processor.
     */
    using processor_t = util::safe_ptr<ID3D11VideoProcessor, Release<ID3D11VideoProcessor>>;
    /**
     * @brief Owning COM pointer for a video processor output view.
     */
    using processor_out_t = util::safe_ptr<ID3D11VideoProcessorOutputView, Release<ID3D11VideoProcessorOutputView>>;
    /**
     * @brief Owning COM pointer for a video processor input view.
     */
    using processor_in_t = util::safe_ptr<ID3D11VideoProcessorInputView, Release<ID3D11VideoProcessorInputView>>;
    /**
     * @brief Owning COM pointer for a video processor capability enumerator.
     */
    using processor_enum_t = util::safe_ptr<ID3D11VideoProcessorEnumerator, Release<ID3D11VideoProcessorEnumerator>>;
  }  // namespace video

  class hwdevice_t;

  /**
   * @brief Cursor position and visibility for the current capture frame.
   */
  struct cursor_t {
    std::vector<std::uint8_t> img_data;  ///< Raw pointer-shape bytes from DXGI output duplication.

    DXGI_OUTDUPL_POINTER_SHAPE_INFO shape_info;  ///< Shape info.
    int x;  ///< X.
    int y;  ///< Y.
    bool visible;  ///< Whether the cursor is visible.
  };

  /**
   * @brief GPU resources used to render a captured cursor shape.
   */
  class gpu_cursor_t {
  public:
    gpu_cursor_t():
        cursor_view {0, 0, 0, 0, 0.0f, 1.0f} {};

    /**
     * @brief Update the cursor position and display geometry used for rendering.
     *
     * @param topleft_x Cursor left edge in desktop coordinates.
     * @param topleft_y Cursor top edge in desktop coordinates.
     * @param display_width Captured display width in pixels.
     * @param display_height Captured display height in pixels.
     * @param display_rotation DXGI rotation applied to the captured display.
     * @param visible Whether the cursor should be visible in the frame.
     */
    void set_pos(LONG topleft_x, LONG topleft_y, LONG display_width, LONG display_height, DXGI_MODE_ROTATION display_rotation, bool visible) {
      this->topleft_x = topleft_x;
      this->topleft_y = topleft_y;
      this->display_width = display_width;
      this->display_height = display_height;
      this->display_rotation = display_rotation;
      this->visible = visible;
      update_viewport();
    }

    /**
     * @brief Replace the cursor texture and update its render viewport.
     *
     * @param texture_width Cursor texture width in pixels.
     * @param texture_height Cursor texture height in pixels.
     * @param texture D3D11 cursor texture to render.
     */
    void set_texture(LONG texture_width, LONG texture_height, texture2d_t &&texture) {
      this->texture = std::move(texture);
      this->texture_width = texture_width;
      this->texture_height = texture_height;
      update_viewport();
    }

    /**
     * @brief Recalculate the D3D viewport for the cursor texture.
     */
    void update_viewport() {
      switch (display_rotation) {
        case DXGI_MODE_ROTATION_UNSPECIFIED:
        case DXGI_MODE_ROTATION_IDENTITY:
          cursor_view.TopLeftX = topleft_x;
          cursor_view.TopLeftY = topleft_y;
          cursor_view.Width = texture_width;
          cursor_view.Height = texture_height;
          break;

        case DXGI_MODE_ROTATION_ROTATE90:
          cursor_view.TopLeftX = topleft_y;
          cursor_view.TopLeftY = display_width - texture_width - topleft_x;
          cursor_view.Width = texture_height;
          cursor_view.Height = texture_width;
          break;

        case DXGI_MODE_ROTATION_ROTATE180:
          cursor_view.TopLeftX = display_width - texture_width - topleft_x;
          cursor_view.TopLeftY = display_height - texture_height - topleft_y;
          cursor_view.Width = texture_width;
          cursor_view.Height = texture_height;
          break;

        case DXGI_MODE_ROTATION_ROTATE270:
          cursor_view.TopLeftX = display_height - texture_height - topleft_y;
          cursor_view.TopLeftY = topleft_x;
          cursor_view.Width = texture_height;
          cursor_view.Height = texture_width;
          break;
      }
    }

    texture2d_t texture;  ///< D3D11 texture backing the captured frame.
    LONG texture_width;  ///< Texture width.
    LONG texture_height;  ///< Texture height.

    LONG topleft_x;  ///< Topleft x.
    LONG topleft_y;  ///< Topleft y.

    LONG display_width;  ///< Display width.
    LONG display_height;  ///< Display height.
    DXGI_MODE_ROTATION display_rotation;  ///< Display rotation.

    shader_res_t input_res;  ///< Input res.

    D3D11_VIEWPORT cursor_view;  ///< Cursor view.

    bool visible;  ///< Whether the cursor is visible.
  };

  /**
   * @brief Shared D3D11/DXGI state used by Windows display capture backends.
   */
  class display_base_t: public display_t {
  public:
    /**
     * @brief Initialize D3D11 desktop duplication for the selected output.
     *
     * @param config Configuration values to apply.
     * @param display_name Display name.
     * @return 0 on success; nonzero or negative platform status on failure.
     */
    int init(const ::video::config_t &config, const std::string &display_name);

    capture_e capture(const push_captured_image_cb_t &push_captured_image_cb, const pull_free_image_cb_t &pull_free_image_cb, bool *cursor) override;

    factory1_t factory;  ///< DXGI factory used to enumerate adapters and outputs.
    adapter_t adapter;  ///< DXGI adapter containing the selected output.
    output_t output;  ///< DXGI output duplicated for capture.
    device_t device;  ///< D3D11 device used for desktop duplication capture.
    device_ctx_t device_ctx;  ///< D3D11 device context used for copy and render operations.
    DXGI_RATIONAL display_refresh_rate;  ///< Display refresh rate.
    int display_refresh_rate_rounded;  ///< Display refresh rate rounded.

    DXGI_MODE_ROTATION display_rotation = DXGI_MODE_ROTATION_UNSPECIFIED;  ///< Display rotation.
    int width_before_rotation;  ///< Width before rotation.
    int height_before_rotation;  ///< Height before rotation.

    int client_frame_rate;  ///< Client frame rate.
    DXGI_RATIONAL client_frame_rate_strict;  ///< Client frame rate strict.

    DXGI_FORMAT capture_format;  ///< Capture format.
    D3D_FEATURE_LEVEL feature_level;  ///< Feature level.

    std::unique_ptr<high_precision_timer> timer = create_high_precision_timer();  ///< Timer.

    /**
     * @brief Enumerates supported d3 DKMT SCHEDULINGPRIORITYCLASS options.
     */
    typedef enum _D3DKMT_SCHEDULINGPRIORITYCLASS {
      D3DKMT_SCHEDULINGPRIORITYCLASS_IDLE,  ///< Idle priority class
      D3DKMT_SCHEDULINGPRIORITYCLASS_BELOW_NORMAL,  ///< Below normal priority class
      D3DKMT_SCHEDULINGPRIORITYCLASS_NORMAL,  ///< Normal priority class
      D3DKMT_SCHEDULINGPRIORITYCLASS_ABOVE_NORMAL,  ///< Above normal priority class
      D3DKMT_SCHEDULINGPRIORITYCLASS_HIGH,  ///< High priority class
      D3DKMT_SCHEDULINGPRIORITYCLASS_REALTIME  ///< Realtime priority class
    } D3DKMT_SCHEDULINGPRIORITYCLASS;  ///< Alias for D3 DKMT SCHEDULINGPRIORITYCLASS.

    /**
     * @brief Kernel-mode D3DKMT adapter handle.
     */
    typedef UINT D3DKMT_HANDLE;

    /**
     * @brief Win32 D3DKMT adapter-open request structure.
     */
    typedef struct _D3DKMT_OPENADAPTERFROMLUID {
      LUID AdapterLuid;  ///< Adapter luid.
      D3DKMT_HANDLE hAdapter;  ///< D3DKMT adapter handle used for low-level display queries.
    } D3DKMT_OPENADAPTERFROMLUID;  ///< Alias for D3 DKMT OPENADAPTERFROMLUID.

    /**
     * @brief Win32 D3DKMT WDDM 2.7 capability flags.
     */
    typedef struct _D3DKMT_WDDM_2_7_CAPS {
      union {
        struct
        {
          UINT HwSchSupported : 1;  ///< Whether hardware-accelerated GPU scheduling is supported.
          UINT HwSchEnabled : 1;  ///< Whether hardware-accelerated GPU scheduling is enabled.
          UINT HwSchEnabledByDefault : 1;  ///< Whether hardware scheduling is enabled by default.
          UINT IndependentVidPnVSyncControl : 1;  ///< Whether independent VidPn VSync control is supported.
          UINT Reserved : 28;  ///< Reserved capability bits.
        };

        UINT Value;  ///< Packed representation of the WDDM 2.7 capability flags.
      };
    } D3DKMT_WDDM_2_7_CAPS;  ///< Alias for D3 DKMT WDDM 2 7 CAPS.

    /**
     * @brief Win32 D3DKMT adapter information query.
     */
    typedef struct _D3DKMT_QUERYADAPTERINFO {
      D3DKMT_HANDLE hAdapter;  ///< D3DKMT adapter handle used for low-level display queries.
      UINT Type;  ///< Type.
      VOID *pPrivateDriverData;  ///< P private driver data.
      UINT PrivateDriverDataSize;  ///< Private driver data size.
    } D3DKMT_QUERYADAPTERINFO;  ///< Alias for D3 DKMT QUERYADAPTERINFO.

    const UINT KMTQAITYPE_WDDM_2_7_CAPS = 70;  ///< Protocol or platform constant for kmtqaitype wddm 2 7 caps.

    /**
     * @brief Win32 D3DKMT adapter-close request structure.
     */
    typedef struct _D3DKMT_CLOSEADAPTER {
      D3DKMT_HANDLE hAdapter;  ///< D3DKMT adapter handle used for low-level display queries.
    } D3DKMT_CLOSEADAPTER;  ///< Alias for D3 DKMT CLOSEADAPTER.

    /**
     * @brief Function pointer for setting D3DKMT process scheduling priority.
     */
    typedef NTSTATUS(WINAPI *PD3DKMTSetProcessSchedulingPriorityClass)(HANDLE, D3DKMT_SCHEDULINGPRIORITYCLASS);
    /**
     * @brief Function pointer for opening a D3DKMT adapter by LUID.
     */
    typedef NTSTATUS(WINAPI *PD3DKMTOpenAdapterFromLuid)(D3DKMT_OPENADAPTERFROMLUID *);
    /**
     * @brief Function pointer for querying D3DKMT adapter information.
     */
    typedef NTSTATUS(WINAPI *PD3DKMTQueryAdapterInfo)(D3DKMT_QUERYADAPTERINFO *);
    /**
     * @brief Function pointer for closing a D3DKMT adapter handle.
     */
    typedef NTSTATUS(WINAPI *PD3DKMTCloseAdapter)(D3DKMT_CLOSEADAPTER *);

    /**
     * @brief Report whether the active display mode is HDR.
     *
     * @return True when the active display mode is HDR.
     */
    virtual bool is_hdr() override;
    /**
     * @brief Read HDR metadata for the active display mode.
     *
     * @param metadata Output structure populated with HDR metadata.
     * @return True when HDR metadata was written to `metadata`.
     */
    virtual bool get_hdr_metadata(SS_HDR_METADATA &metadata) override;

    /**
     * @brief Convert a DXGI format enum to a diagnostic string.
     *
     * @param format Pixel, audio, or protocol format being converted.
     * @return Static string describing the DXGI format.
     */
    const char *dxgi_format_to_string(DXGI_FORMAT format);
    /**
     * @brief Convert a DXGI colorspace enum to a diagnostic string.
     *
     * @param type DXGI colorspace value reported by the output.
     * @return Static string describing the colorspace.
     */
    const char *colorspace_to_string(DXGI_COLOR_SPACE_TYPE type);
    /**
     * @brief Get supported capture formats.
     *
     * @return Capture formats supported by this display backend.
     */
    virtual std::vector<DXGI_FORMAT> get_supported_capture_formats() = 0;

  protected:
    /**
     * @brief Get pixel pitch.
     *
     * @return Bytes per pixel for the active capture format.
     */
    int get_pixel_pitch() {
      return (capture_format == DXGI_FORMAT_R16G16B16A16_FLOAT) ? 8 : 4;
    }

    /**
     * @brief Capture a display frame into the provided image object.
     *
     * @param pull_free_image_cb Callback that provides an available image buffer.
     * @param img_out Captured image buffer returned to the streaming pipeline.
     * @param timeout Maximum time to wait for the operation.
     * @param cursor_visible Cursor visible.
     * @return Capture status reported to the streaming pipeline.
     */
    virtual capture_e snapshot(const pull_free_image_cb_t &pull_free_image_cb, std::shared_ptr<platf::img_t> &img_out, std::chrono::milliseconds timeout, bool cursor_visible) = 0;
    /**
     * @brief Release resources associated with the last captured snapshot.
     *
     * @return Capture status after releasing the current snapshot.
     */
    virtual capture_e release_snapshot() = 0;
    /**
     * @brief Finish cursor composition into a RAM-backed image.
     *
     * @param img Image or frame object to read from or populate.
     * @param dummy Unused placeholder required by the interface signature.
     * @return Capture status after finalizing the captured image.
     */
    virtual int complete_img(img_t *img, bool dummy) = 0;
  };

  /**
   * Display component for devices that use software encoders.
   */
  class display_ram_t: public display_base_t {
  public:
    /**
     * @brief Allocate an image buffer compatible with this display backend.
     *
     * @return Allocated img object, or null when unavailable.
     */
    std::shared_ptr<img_t> alloc_img() override;
    /**
     * @brief Populate a fallback image when real capture data is unavailable.
     *
     * @param img Image or frame object to read from or populate.
     * @return Capture status reported to the streaming pipeline.
     */
    int dummy_img(img_t *img) override;
    /**
     * @brief Finish cursor composition into a GPU-backed image.
     *
     * @param img Image or frame object to read from or populate.
     * @param dummy Unused placeholder required by the interface signature.
     * @return Capture status after finalizing the captured image.
     */
    int complete_img(img_t *img, bool dummy) override;
    /**
     * @brief Get supported capture formats.
     *
     * @return Capture formats supported by this display backend.
     */
    std::vector<DXGI_FORMAT> get_supported_capture_formats() override;

    /**
     * @brief Create AVCodec encode device.
     *
     * @param pix_fmt Sunshine pixel format to convert or allocate for.
     * @return Constructed AVCodec encode device object.
     */
    std::unique_ptr<avcodec_encode_device_t> make_avcodec_encode_device(pix_fmt_e pix_fmt) override;

    D3D11_MAPPED_SUBRESOURCE img_info;  ///< CPU mapping information for the captured texture.
    texture2d_t texture;  ///< D3D11 texture backing the captured frame.
  };

  /**
   * Display component for devices that use hardware encoders.
   */
  class display_vram_t: public display_base_t, public std::enable_shared_from_this<display_vram_t> {
  public:
    /**
     * @brief Allocate an image buffer compatible with this display backend.
     *
     * @return Allocated img object, or null when unavailable.
     */
    std::shared_ptr<img_t> alloc_img() override;
    /**
     * @brief Populate a fallback image when real capture data is unavailable.
     *
     * @param img_base Image buffer populated with a fallback frame.
     * @return Capture status reported to the streaming pipeline.
     */
    int dummy_img(img_t *img_base) override;
    /**
     * @brief Finalize a VRAM image after capture or dummy-frame generation.
     *
     * @param img_base Image buffer whose D3D resources are finalized.
     * @param dummy Unused placeholder required by the interface signature.
     * @return Capture status after finalizing the captured image.
     */
    int complete_img(img_t *img_base, bool dummy) override;
    /**
     * @brief Get supported capture formats.
     *
     * @return Capture formats supported by this display backend.
     */
    std::vector<DXGI_FORMAT> get_supported_capture_formats() override;

    bool is_codec_supported(std::string_view name, const ::video::config_t &config) override;

    /**
     * @brief Create AVCodec encode device.
     *
     * @param pix_fmt Sunshine pixel format to convert or allocate for.
     * @return Constructed AVCodec encode device object.
     */
    std::unique_ptr<avcodec_encode_device_t> make_avcodec_encode_device(pix_fmt_e pix_fmt) override;

    /**
     * @brief Create NVENC encode device.
     *
     * @param pix_fmt Sunshine pixel format to convert or allocate for.
     * @return Constructed NVENC encode device object.
     */
    std::unique_ptr<nvenc_encode_device_t> make_nvenc_encode_device(pix_fmt_e pix_fmt) override;

    std::atomic<uint32_t> next_image_id;  ///< Next image ID.
  };

  /**
   * Display duplicator that uses the DirectX Desktop Duplication API.
   */
  class duplication_t {
  public:
    dup_t dup;  ///< Desktop Duplication capture session.
    bool has_frame {};  ///< Whether a frame is currently held by the duplication object.
    std::chrono::steady_clock::time_point last_protected_content_warning_time {};  ///< Last protected content warning time.

    /**
     * @brief Initialize WGC capture for the selected display.
     *
     * @param display Display object or identifier associated with the operation.
     * @param config Configuration values to apply.
     * @return 0 on success; nonzero or negative platform status on failure.
     */
    int init(display_base_t *display, const ::video::config_t &config);
    /**
     * @brief Acquire the next frame from the Windows capture backend.
     *
     * @param frame_info Frame info.
     * @param timeout Maximum time to wait for the operation.
     * @param res_p Res p.
     * @return Capture status for the frame acquisition attempt.
     */
    capture_e next_frame(DXGI_OUTDUPL_FRAME_INFO &frame_info, std::chrono::milliseconds timeout, resource_t::pointer *res_p);
    /**
     * @brief Reset the object to its initial empty state.
     *
     * @param dup_p Dup p.
     * @return Reset status.
     */
    capture_e reset(dup_t::pointer dup_p = dup_t::pointer());
    /**
     * @brief Release resources associated with frame.
     *
     * @return Capture status after releasing the current frame.
     */
    capture_e release_frame();

    ~duplication_t();
  };

  /**
   * Display backend that uses DDAPI with a software encoder.
   */
  class display_ddup_ram_t: public display_ram_t {
  public:
    /**
     * @brief Initialize shared D3D capture resources for a RAM frame.
     *
     * @param config Configuration values to apply.
     * @param display_name Display name.
     * @return 0 on success; nonzero or negative platform status on failure.
     */
    int init(const ::video::config_t &config, const std::string &display_name);
    /**
     * @brief Capture a display frame into the provided image object.
     *
     * @param pull_free_image_cb Callback that provides an available image buffer.
     * @param img_out Captured image buffer returned to the streaming pipeline.
     * @param timeout Maximum time to wait for the operation.
     * @param cursor_visible Cursor visible.
     * @return Capture status reported to the streaming pipeline.
     */
    capture_e snapshot(const pull_free_image_cb_t &pull_free_image_cb, std::shared_ptr<platf::img_t> &img_out, std::chrono::milliseconds timeout, bool cursor_visible) override;
    /**
     * @brief Release resources associated with the last captured snapshot.
     *
     * @return Capture status after releasing the current snapshot.
     */
    capture_e release_snapshot() override;

    duplication_t dup;  ///< Desktop Duplication session used to acquire frames.
    cursor_t cursor;  ///< Cursor.
  };

  /**
   * Display backend that uses DDAPI with a hardware encoder.
   */
  class display_ddup_vram_t: public display_vram_t {
  public:
    /**
     * @brief Initialize D3D cursor rendering resources for RAM capture.
     *
     * @param config Configuration values to apply.
     * @param display_name Display name.
     * @return 0 on success; nonzero or negative platform status on failure.
     */
    int init(const ::video::config_t &config, const std::string &display_name);
    /**
     * @brief Capture a display frame into the provided image object.
     *
     * @param pull_free_image_cb Callback that provides an available image buffer.
     * @param img_out Captured image buffer returned to the streaming pipeline.
     * @param timeout Maximum time to wait for the operation.
     * @param cursor_visible Cursor visible.
     * @return Capture status reported to the streaming pipeline.
     */
    capture_e snapshot(const pull_free_image_cb_t &pull_free_image_cb, std::shared_ptr<platf::img_t> &img_out, std::chrono::milliseconds timeout, bool cursor_visible) override;
    /**
     * @brief Release resources associated with the last captured snapshot.
     *
     * @return Capture status after releasing the current snapshot.
     */
    capture_e release_snapshot() override;

    duplication_t dup;  ///< Desktop Duplication session used to acquire frames.
    sampler_state_t sampler_linear;  ///< Sampler linear.

    blend_t blend_alpha;  ///< Blend alpha.
    blend_t blend_invert;  ///< Blend invert.
    blend_t blend_disable;  ///< Blend disable.

    ps_t cursor_ps;  ///< Cursor ps.
    vs_t cursor_vs;  ///< Cursor vs.

    gpu_cursor_t cursor_alpha;  ///< Cursor alpha.
    gpu_cursor_t cursor_xor;  ///< Cursor xor.

    texture2d_t old_surface_delayed_destruction;  ///< Old surface delayed destruction.
    std::chrono::steady_clock::time_point old_surface_timestamp;  ///< Old surface timestamp.
    std::variant<std::monostate, texture2d_t, std::shared_ptr<platf::img_t>> last_frame_variant;  ///< Last frame variant.
  };

  /**
   * Display duplicator that uses the Windows.Graphics.Capture API.
   * windows-legacy: excluded when built with SUNSHINE_NO_WGC (Win7/8.x builds).
   */
#ifndef SUNSHINE_NO_WGC
  class wgc_capture_t {
    winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice uwp_device {nullptr};
    winrt::Windows::Graphics::Capture::GraphicsCaptureItem item {nullptr};
    winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool frame_pool {nullptr};
    winrt::Windows::Graphics::Capture::GraphicsCaptureSession capture_session {nullptr};
    winrt::Windows::Graphics::Capture::Direct3D11CaptureFrame produced_frame {nullptr};
    winrt::Windows::Graphics::Capture::Direct3D11CaptureFrame consumed_frame {nullptr};
    SRWLOCK frame_lock = SRWLOCK_INIT;
    CONDITION_VARIABLE frame_present_cv;

    void on_frame_arrived(winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool const &sender, winrt::Windows::Foundation::IInspectable const &);

  public:
    wgc_capture_t();
    ~wgc_capture_t();

    /**
     * @brief Initialize D3D cursor rendering resources for GPU capture.
     *
     * @param display Display object or identifier associated with the operation.
     * @param config Configuration values to apply.
     * @return 0 on success; nonzero or negative platform status on failure.
     */
    int init(display_base_t *display, const ::video::config_t &config);
    /**
     * @brief Acquire the next frame from the Windows capture backend.
     *
     * @param timeout Maximum time to wait for the operation.
     * @param out Output object populated by the operation.
     * @param out_time QPC timestamp associated with the acquired frame.
     * @return Capture status for the frame acquisition attempt.
     */
    capture_e next_frame(std::chrono::milliseconds timeout, ID3D11Texture2D **out, uint64_t &out_time);
    /**
     * @brief Release resources associated with frame.
     *
     * @return Capture status after releasing the current frame.
     */
    capture_e release_frame();
    /**
     * @brief Enable or disable cursor composition in Windows.Graphics.Capture frames.
     *
     * @param cursor_visible Whether the cursor should be included in captured frames.
     * @return Zero when the cursor visibility state was accepted by the capture session.
     */
    int set_cursor_visible(bool cursor_visible);
  };

  /**
   * Display backend that uses Windows.Graphics.Capture with a software encoder.
   */
  class display_wgc_ram_t: public display_ram_t {
    wgc_capture_t dup;

  public:
    /**
     * @brief Initialize shared D3D capture resources for a GPU frame.
     *
     * @param config Configuration values to apply.
     * @param display_name Display name.
     * @return 0 on success; nonzero or negative platform status on failure.
     */
    int init(const ::video::config_t &config, const std::string &display_name);
    /**
     * @brief Capture a display frame into the provided image object.
     *
     * @param pull_free_image_cb Callback that provides an available image buffer.
     * @param img_out Captured image buffer returned to the streaming pipeline.
     * @param timeout Maximum time to wait for the operation.
     * @param cursor_visible Cursor visible.
     * @return Capture status reported to the streaming pipeline.
     */
    capture_e snapshot(const pull_free_image_cb_t &pull_free_image_cb, std::shared_ptr<platf::img_t> &img_out, std::chrono::milliseconds timeout, bool cursor_visible) override;
    /**
     * @brief Release resources associated with the last captured snapshot.
     *
     * @return Capture status after releasing the current snapshot.
     */
    capture_e release_snapshot() override;
  };

  /**
   * Display backend that uses Windows.Graphics.Capture with a hardware encoder.
   */
  class display_wgc_vram_t: public display_vram_t {
    wgc_capture_t dup;

  public:
    /**
     * @brief Initialize Windows Graphics Capture frame-pool resources.
     *
     * @param config Configuration values to apply.
     * @param display_name Display name.
     * @return 0 on success; nonzero or negative platform status on failure.
     */
    int init(const ::video::config_t &config, const std::string &display_name);
    /**
     * @brief Capture a display frame into the provided image object.
     *
     * @param pull_free_image_cb Callback that provides an available image buffer.
     * @param img_out Captured image buffer returned to the streaming pipeline.
     * @param timeout Maximum time to wait for the operation.
     * @param cursor_visible Cursor visible.
     * @return Capture status reported to the streaming pipeline.
     */
    capture_e snapshot(const pull_free_image_cb_t &pull_free_image_cb, std::shared_ptr<platf::img_t> &img_out, std::chrono::milliseconds timeout, bool cursor_visible) override;
    /**
     * @brief Release resources associated with the last captured snapshot.
     *
     * @return Capture status after releasing the current snapshot.
     */
    capture_e release_snapshot() override;
  };
#endif  // SUNSHINE_NO_WGC

  /**
   * Display capture that uses NVIDIA Framebuffer Capture (NvFBC) on Windows.
   *
   * @details windows-legacy: NvFBC is the GameStream-era capture path that works
   * on Windows 7, where Desktop Duplication (8+) and Windows.Graphics.Capture
   * (10+) are unavailable. The driver DLL (`NvFBC64.dll` / `NvFBC.dll`) is
   * loaded at runtime; nothing here links against NVIDIA at build time.
   * The ToSys target delivers ARGB frames to system memory with the HW cursor
   * composited, matching `B8G8R8A8_UNORM`.
   */
  class nvfbc_capture_t {
  public:
    nvfbc_capture_t();
    ~nvfbc_capture_t();

    nvfbc_capture_t(const nvfbc_capture_t &) = delete;
    nvfbc_capture_t &operator=(const nvfbc_capture_t &) = delete;

    /**
     * @brief Check whether NvFBC capture is usable on this system.
     *
     * @details Loads the driver DLL once per process, enables NvFBC, and
     * queries `bIsCapturePossible`. Safe to call on non-NVIDIA systems.
     * @return True when an NvFBC session can be created.
     */
    static bool available();

    /**
     * @brief Map a GDI display name to the NvFBC adapter ordinal.
     *
     * @param display_name GDI display name (e.g. `\\\\.\\DISPLAY1`), or empty for default.
     * @return NVIDIA adapter ordinal, or 0 when mapping fails.
     */
    static unsigned int adapter_index_for_display(const std::string &display_name);

    /**
     * @brief Create an NvFBC ToSys session for the given adapter.
     *
     * @param adapter_idx NvFBC adapter ordinal.
     * @param width Desktop width used for the session (informational).
     * @param height Desktop height used for the session (informational).
     * @return 0 on success; -1 when the session cannot be created.
     */
    int init(unsigned int adapter_idx, int width, int height);
    /**
     * @brief Grab the next desktop frame into the session buffer.
     *
     * @param timeout Maximum time to wait for a new frame.
     * @return Capture status for the grab attempt.
     */
    capture_e grab(std::chrono::milliseconds timeout);
    /**
     * @brief Release the NvFBC session.
     *
     * @return Capture status after releasing the session.
     */
    capture_e release_session();

    /**
     * @brief Access the latest grabbed frame buffer (ARGB, driver-owned).
     *
     * @return Pointer to the frame bytes, valid until the next grab/release.
     */
    const std::uint8_t *frame_buffer() const {
      return frame_bytes;
    }

    /**
     * @brief Padded frame-buffer width reported by the last grab.
     *
     * @return Stride in pixels between the starts of consecutive rows.
     */
    unsigned int buffer_stride_pixels() const {
      return buffer_stride;
    }

    /**
     * @brief Desktop width reported by the last grab.
     *
     * @return Captured desktop width in pixels.
     */
    unsigned int frame_width() const {
      return last_width;
    }

    /**
     * @brief Desktop height reported by the last grab.
     *
     * @return Captured desktop height in pixels.
     */
    unsigned int frame_height() const {
      return last_height;
    }

  private:
    void *session = nullptr;  ///< NvFBC ToSys interface object from CreateEx.
    void *buffer_storage = nullptr;  ///< Driver-owned frame buffer from ToSys setup.
    const std::uint8_t *frame_bytes = nullptr;  ///< Latest grabbed frame bytes.
    unsigned int buffer_stride = 0;  ///< Padded buffer stride in pixels.
    unsigned int last_width = 0;  ///< Desktop width from the last grab.
    unsigned int last_height = 0;  ///< Desktop height from the last grab.
    bool initialized = false;  ///< Session is active.
  };

  /**
   * Display backend that uses NvFBC ToSys with a software encoder.
   */
  class display_nvfbc_ram_t: public display_ram_t {
    nvfbc_capture_t session;

  public:
    /**
     * @brief Initialize NvFBC capture for the selected display.
     *
     * @param config Configuration values to apply.
     * @param display_name Display name.
     * @return 0 on success; nonzero or negative platform status on failure.
     */
    int init(const ::video::config_t &config, const std::string &display_name);
    /**
     * @brief Capture a display frame into the provided image object.
     *
     * @param pull_free_image_cb Callback that provides an available image buffer.
     * @param img_out Captured image buffer returned to the streaming pipeline.
     * @param timeout Maximum time to wait for the operation.
     * @param cursor_visible Cursor visibility (cursor is composited by NvFBC).
     * @return Capture status reported to the streaming pipeline.
     */
    capture_e snapshot(const pull_free_image_cb_t &pull_free_image_cb, std::shared_ptr<platf::img_t> &img_out, std::chrono::milliseconds timeout, bool cursor_visible) override;
    /**
     * @brief Release resources associated with the last captured snapshot.
     *
     * @return Capture status after releasing the current snapshot.
     */
    capture_e release_snapshot() override;
  };

  /**
   * Display backend that uses NvFBC ToSys with a hardware encoder.
   *
   * @details Frames are captured to system memory by NvFBC, uploaded through a
   * D3D11 staging texture, and shared with the encoder device, so NVIDIA
   * NVENC streaming works on Windows 7 without Desktop Duplication.
   */
  class display_nvfbc_vram_t: public display_vram_t {
    nvfbc_capture_t session;
    texture2d_t staging;  ///< CPU-writable staging texture for NvFBC uploads.

  public:
    /**
     * @brief Initialize NvFBC capture and D3D upload resources.
     *
     * @param config Configuration values to apply.
     * @param display_name Display name.
     * @return 0 on success; nonzero or negative platform status on failure.
     */
    int init(const ::video::config_t &config, const std::string &display_name);
    /**
     * @brief Capture a display frame into the provided image object.
     *
     * @param pull_free_image_cb Callback that provides an available image buffer.
     * @param img_out Captured image buffer returned to the streaming pipeline.
     * @param timeout Maximum time to wait for the operation.
     * @param cursor_visible Cursor visibility (cursor is composited by NvFBC).
     * @return Capture status reported to the streaming pipeline.
     */
    capture_e snapshot(const pull_free_image_cb_t &pull_free_image_cb, std::shared_ptr<platf::img_t> &img_out, std::chrono::milliseconds timeout, bool cursor_visible) override;
    /**
     * @brief Release resources associated with the last captured snapshot.
     *
     * @return Capture status after releasing the current snapshot.
     */
    capture_e release_snapshot() override;
  };
}  // namespace platf::dxgi
