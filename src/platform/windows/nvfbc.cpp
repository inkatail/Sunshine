/**
 * @file src/platform/windows/nvfbc.cpp
 * @brief NVIDIA Framebuffer Capture (NvFBC) backend for Windows.
 *
 * @details windows-legacy: GameStream-era capture path for Windows 7, where
 * Desktop Duplication (8+) and Windows.Graphics.Capture (10+) do not exist.
 * The driver DLL is loaded at runtime, so this translation unit links against
 * nothing NVIDIA-specific and the binary still starts on non-NVIDIA systems.
 */
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

// local includes
#include "display.h"
#include "misc.h"
#include "nvfbc_win.h"
#include "os_version.h"
#include "src/logging.h"
#include "utf_utils.h"

namespace platf {
  using namespace std::literals;
}

namespace platf::dxgi {
  namespace {
    /**
     * @brief Process-lifetime loader for the NVIDIA driver NvFBC DLL.
     */
    struct nvfbc_library_t {
      HMODULE handle = nullptr;  ///< Loaded NvFBC64.dll / NvFBC.dll.
      nvfbc_win::nvfbc_create_ex_fn create_ex = nullptr;  ///< NvFBC_CreateEx.
      nvfbc_win::nvfbc_get_status_ex_fn get_status = nullptr;  ///< NvFBC_GetStatusEx.
      nvfbc_win::nvfbc_enable_fn enable = nullptr;  ///< NvFBC_Enable.
      bool enable_attempted = false;  ///< NvFBC_Enable was attempted once.

      /**
       * @brief Load the driver DLL and resolve entry points.
       *
       * @return True when all required entry points are available.
       */
      bool load() {
        if (handle) {
          return true;
        }

        // Prefer the explicit System32 path (avoids search-order hijack),
        // fall back to the plain name which also resolves via System32.
#ifdef _WIN64
        constexpr wchar_t dll_name[] = L"NvFBC64.dll";
#else
        constexpr wchar_t dll_name[] = L"NvFBC.dll";
#endif
        wchar_t system_dir[MAX_PATH] = {};
        if (GetSystemDirectoryW(system_dir, ARRAYSIZE(system_dir))) {
          std::wstring full = std::wstring {system_dir} + L"\\" + dll_name;
          handle = LoadLibraryW(full.c_str());
        }
        if (!handle) {
          handle = LoadLibraryW(dll_name);
        }
        if (!handle) {
          return false;
        }

        create_ex = reinterpret_cast<nvfbc_win::nvfbc_create_ex_fn>(GetProcAddress(handle, "NvFBC_CreateEx"));
        get_status = reinterpret_cast<nvfbc_win::nvfbc_get_status_ex_fn>(GetProcAddress(handle, "NvFBC_GetStatusEx"));
        enable = reinterpret_cast<nvfbc_win::nvfbc_enable_fn>(GetProcAddress(handle, "NvFBC_Enable"));
        if (!create_ex || !get_status || !enable) {
          FreeLibrary(handle);
          handle = nullptr;
          create_ex = nullptr;
          get_status = nullptr;
          enable = nullptr;
          return false;
        }
        return true;
      }
    };

    /**
     * @brief Access the process-wide NvFBC library handle.
     *
     * @return Singleton loader instance.
     */
    nvfbc_library_t &nvfbc_library() {
      static nvfbc_library_t instance;
      return instance;
    }

    /**
     * @brief Attempt NvFBC_Enable once per process.
     *
     * @details Enabling requires elevation and persists driver-side; failure
     * only warns because the feature may already be enabled.
     * @param lib Loaded library handle.
     */
    void ensure_nvfbc_enabled(nvfbc_library_t &lib) {
      if (lib.enable_attempted) {
        return;
      }
      lib.enable_attempted = true;
      if (lib.enable(nvfbc_win::NVFBC_WIN_STATE_ENABLE) != nvfbc_win::NVFBC_WIN_SUCCESS) {
        BOOST_LOG(warning) << "NvFBC_Enable() failed (admin rights may be required once per driver install); continuing in case NvFBC is already enabled"sv;
      }
    }

    /**
     * @brief Default GeForce unlock key (16 bytes, public nvfbcwrp magic).
     *
     * @details Quadro/Tesla/GRID do not need a key. On GeForce, CreateEx
     * requires 16 bytes of private data; when the bundled key stops working
     * with newer drivers, provide one via the `NVFBC_PRIV_DATA` hex string
     * (LookingGlass convention) or install the nvfbcwrp wrapper DLL.
     * @return Default key bytes.
     */
    std::vector<std::uint8_t> default_priv_data() {
      // 0xAEF57AC5, 0x401D1A39, 0x1B856BBE, 0x9ED0CEBA in little-endian order.
      return {0xC5, 0x7A, 0xF5, 0xAE, 0x39, 0x1A, 0x1D, 0x40, 0xBE, 0x6B, 0x85, 0x1B, 0xBA, 0xCE, 0xD0, 0x9E};
    }

    /**
     * @brief Parse a hex string into bytes, ignoring separators.
     *
     * @param text Hex text such as `AEF57AC5...` (spaces/colons tolerated).
     * @return Parsed bytes, possibly empty.
     */
    std::vector<std::uint8_t> parse_hex_bytes(const std::string &text) {
      std::vector<std::uint8_t> out;
      unsigned int nibble_count = 0;
      std::uint8_t current = 0;
      for (char c : text) {
        unsigned int v;
        if (c >= '0' && c <= '9') {
          v = c - '0';
        } else if (c >= 'a' && c <= 'f') {
          v = c - 'a' + 10;
        } else if (c >= 'A' && c <= 'F') {
          v = c - 'A' + 10;
        } else {
          continue;
        }
        current = (std::uint8_t) ((current << 4) | v);
        if (++nibble_count == 2) {
          out.push_back(current);
          current = 0;
          nibble_count = 0;
        }
      }
      return out;
    }

    /**
     * @brief Resolve the GeForce private-data key for CreateEx.
     *
     * @return Key bytes from `NVFBC_PRIV_DATA` when set, else the default key.
     */
    std::vector<std::uint8_t> priv_data_key() {
      if (const char *env = std::getenv("NVFBC_PRIV_DATA")) {
        if (auto parsed = parse_hex_bytes(env); !parsed.empty()) {
          return parsed;
        }
        BOOST_LOG(warning) << "NVFBC_PRIV_DATA is set but could not be parsed as hex; using the built-in key"sv;
      }
      return default_priv_data();
    }

    /**
     * @brief Throttled warning for DRM-protected content.
     */
    void warn_protected_content() {
      static std::chrono::steady_clock::time_point last_warning {};
      const auto now = std::chrono::steady_clock::now();
      if (now > last_warning + std::chrono::seconds(10)) {
        BOOST_LOG(warning) << "NvFBC reports DRM-protected content on screen; affected regions may capture black"sv;
        last_warning = now;
      }
    }
  }  // namespace

  nvfbc_capture_t::nvfbc_capture_t() = default;

  nvfbc_capture_t::~nvfbc_capture_t() {
    release_session();
  }

  bool nvfbc_capture_t::available() {
    auto &lib = nvfbc_library();
    if (!lib.load()) {
      return false;
    }
    ensure_nvfbc_enabled(lib);

    nvfbc_win::nvfbc_status_t status {};
    status.dwVersion = nvfbc_win::struct_version(sizeof(status), 2);
    status.dwAdapterIdx = 0;
    if (lib.get_status(&status) != nvfbc_win::NVFBC_WIN_SUCCESS) {
      return false;
    }
    return status.bIsCapturePossible != 0;
  }

  unsigned int nvfbc_capture_t::adapter_index_for_display(const std::string &display_name) {
    // Map the GDI display name to the NvFBC adapter ordinal by counting only
    // NVIDIA adapters in DXGI enumeration order. Empty selects the default.
    if (display_name.empty()) {
      return 0;
    }

    factory1_t factory;
    if (FAILED(CreateDXGIFactory1(IID_IDXGIFactory1, (void **) &factory))) {
      return 0;
    }

    const auto wanted = utf_utils::from_utf8(display_name);
    unsigned int nvidia_ordinal = 0;
    adapter_t::pointer adapter_p {};
    for (int x = 0; factory->EnumAdapters1(x, &adapter_p) != DXGI_ERROR_NOT_FOUND; ++x) {
      adapter_t adapter {adapter_p};
      DXGI_ADAPTER_DESC desc {};
      if (FAILED(adapter->GetDesc(&desc))) {
        continue;
      }
      const bool is_nvidia = desc.VendorId == 0x10DE;
      output_t::pointer output_p {};
      for (int y = 0; adapter->EnumOutputs(y, &output_p) != DXGI_ERROR_NOT_FOUND; ++y) {
        output_t output {output_p};
        DXGI_OUTPUT_DESC out_desc {};
        if (FAILED(output->GetDesc(&out_desc))) {
          continue;
        }
        if (out_desc.DeviceName == wanted) {
          return is_nvidia ? nvidia_ordinal : 0;
        }
      }
      if (is_nvidia) {
        ++nvidia_ordinal;
      }
    }
    return 0;
  }

  int nvfbc_capture_t::init(unsigned int adapter_idx, int width, int height) {
    auto &lib = nvfbc_library();
    if (!lib.load()) {
      BOOST_LOG(error) << "NvFBC is unavailable: driver DLL could not be loaded"sv;
      return -1;
    }
    ensure_nvfbc_enabled(lib);

    nvfbc_win::nvfbc_status_t status {};
    status.dwVersion = nvfbc_win::struct_version(sizeof(status), 2);
    status.dwAdapterIdx = adapter_idx;
    if (lib.get_status(&status) != nvfbc_win::NVFBC_WIN_SUCCESS || !status.bIsCapturePossible) {
      BOOST_LOG(error) << "NvFBC capture is not possible on adapter "sv << adapter_idx;
      return -1;
    }

    auto key = priv_data_key();
    nvfbc_win::nvfbc_create_params_t params {};
    params.dwVersion = nvfbc_win::struct_version(sizeof(params), 2);
    params.dwInterfaceType = nvfbc_win::NVFBC_TO_SYS;
    params.dwAdapterIdx = adapter_idx;
    params.pPrivateData = key.data();
    params.dwPrivateDataSize = (nvfbc_win::nv_u32_t) key.size();
    if (lib.create_ex(&params) != nvfbc_win::NVFBC_WIN_SUCCESS || !params.pNvFBC) {
      BOOST_LOG(error) << "NvFBC_CreateEx(ToSys) failed on adapter "sv << adapter_idx << "; on GeForce this usually means the unlock key was rejected (see NVFBC_PRIV_DATA)"sv;
      return -1;
    }

    auto *iface = static_cast<nvfbc_win::tosys_interface_t *>(params.pNvFBC);
    nvfbc_win::tosys_setup_params_t setup {};
    setup.dwVersion = nvfbc_win::struct_version(sizeof(setup), 3);
    setup.bWithHWCursor = 1;  // Cursor composited by the driver (GameStream behavior).
    setup.eMode = nvfbc_win::TOSYS_ARGB;
    setup.ppBuffer = &buffer_storage;
    if (iface->setup(&setup) != nvfbc_win::NVFBC_WIN_SUCCESS || !buffer_storage) {
      BOOST_LOG(error) << "NvFBC ToSys setup failed"sv;
      iface->release();
      return -1;
    }

    session = iface;
    initialized = true;
    last_width = (unsigned int) width;
    last_height = (unsigned int) height;
    BOOST_LOG(info) << "NvFBC ToSys session active (adapter "sv << adapter_idx << ", "sv << width << 'x' << height << ')';
    return 0;
  }

  capture_e nvfbc_capture_t::grab(std::chrono::milliseconds timeout) {
    if (!initialized || !session) {
      return capture_e::error;
    }
    auto *iface = static_cast<nvfbc_win::tosys_interface_t *>(session);

    nvfbc_win::nvfbc_frame_grab_info_t info {};
    nvfbc_win::tosys_grab_params_t params {};
    params.dwVersion = nvfbc_win::struct_version(sizeof(params), 1);
    if (timeout.count() <= 0) {
      params.dwFlags = nvfbc_win::TOSYS_NOWAIT;
    } else {
      params.dwFlags = nvfbc_win::TOSYS_WAIT_WITH_TIMEOUT;
      params.dwWaitTime = (nvfbc_win::nv_u32_t) timeout.count();
    }
    params.eGMode = nvfbc_win::TOSYS_SOURCEMODE_FULL;
    params.pNvFBCFrameGrabInfo = &info;

    const auto result = iface->grab(&params);
    if (result == nvfbc_win::NVFBC_WIN_ERROR_INVALIDATED_SESSION || info.bMustRecreate) {
      BOOST_LOG(warning) << "NvFBC session invalidated; recreating capture"sv;
      release_session();
      return capture_e::reinit;
    }
    if (result == nvfbc_win::NVFBC_WIN_ERROR_DYNAMIC_DISABLE) {
      BOOST_LOG(error) << "NvFBC dynamically disabled; recreating capture"sv;
      release_session();
      return capture_e::reinit;
    }
    if (result == nvfbc_win::NVFBC_WIN_ERROR_PROTECTED_CONTENT || info.bProtectedContent) {
      warn_protected_content();
      return capture_e::timeout;
    }
    if (result != nvfbc_win::NVFBC_WIN_SUCCESS) {
      BOOST_LOG(error) << "NvFBC grab failed ["sv << (int) result << ']';
      return capture_e::error;
    }
    if (!buffer_storage || !info.dwWidth || !info.dwHeight || !info.dwBufferWidth) {
      return capture_e::error;
    }

    frame_bytes = static_cast<const std::uint8_t *>(buffer_storage);
    buffer_stride = info.dwBufferWidth;
    last_width = info.dwWidth;
    last_height = info.dwHeight;
    return capture_e::ok;
  }

  capture_e nvfbc_capture_t::release_session() {
    if (session) {
      static_cast<nvfbc_win::tosys_interface_t *>(session)->release();
      session = nullptr;
    }
    buffer_storage = nullptr;
    frame_bytes = nullptr;
    initialized = false;
    return capture_e::ok;
  }

  int display_nvfbc_ram_t::init(const ::video::config_t &config, const std::string &display_name) {
    if (!nvfbc_capture_t::available()) {
      BOOST_LOG(debug) << "NvFBC unavailable on this system"sv;
      return -1;
    }

    // Geometry comes from GDI so no DXGI duplication-capable output is needed.
    DEVMODEW mode {};
    mode.dmSize = sizeof(mode);
    std::wstring wide_name = display_name.empty() ? std::wstring {} : utf_utils::from_utf8(display_name);
    if (!EnumDisplaySettingsW(display_name.empty() ? nullptr : wide_name.c_str(), ENUM_CURRENT_SETTINGS, &mode)) {
      BOOST_LOG(error) << "NvFBC: failed to query display settings for ["sv << display_name << ']';
      return -1;
    }

    width = (int) mode.dmPelsWidth;
    height = (int) mode.dmPelsHeight;
    width_before_rotation = width;
    height_before_rotation = height;
    display_rotation = DXGI_MODE_ROTATION_IDENTITY;
    offset_x = mode.dmPosition.x - GetSystemMetrics(SM_XVIRTUALSCREEN);
    offset_y = mode.dmPosition.y - GetSystemMetrics(SM_YVIRTUALSCREEN);
    env_width = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    env_height = GetSystemMetrics(SM_CYVIRTUALSCREEN);
    capture_format = DXGI_FORMAT_B8G8R8A8_UNORM;

    // Factory is only used by the shared capture() loop for IsCurrent() checks.
    if (FAILED(CreateDXGIFactory1(IID_IDXGIFactory1, (void **) &factory))) {
      BOOST_LOG(error) << "NvFBC: failed to create DXGI factory"sv;
      return -1;
    }

    client_frame_rate = config.framerate;
    client_frame_rate_strict = {0, 0};
    if (config.framerateX100 > 0) {
      const AVRational fps = ::video::framerate_to_rational(config);
      client_frame_rate_strict = DXGI_RATIONAL {static_cast<UINT>(fps.num), static_cast<UINT>(fps.den)};
    }

    if (!timer || !*timer) {
      BOOST_LOG(error) << "Uninitialized high precision timer"sv;
      return -1;
    }

    const unsigned int adapter_idx = nvfbc_capture_t::adapter_index_for_display(display_name);
    if (session.init(adapter_idx, width, height)) {
      return -1;
    }

    BOOST_LOG(info) << "NvFBC RAM capture active on ["sv << display_name << "] "sv << width << 'x' << height;
    return 0;
  }

  capture_e display_nvfbc_ram_t::snapshot(const pull_free_image_cb_t &pull_free_image_cb, std::shared_ptr<platf::img_t> &img_out, std::chrono::milliseconds timeout, bool /*cursor_visible*/) {
    // The HW cursor is composited by the driver (bWithHWCursor), so there is
    // nothing to blend here.
    if (auto status = session.grab(timeout); status != capture_e::ok) {
      return status;
    }

    // NvFBC reports the full desktop; bail to reinit on mode changes.
    if ((int) session.frame_width() < width || (int) session.frame_height() < height) {
      BOOST_LOG(info) << "NvFBC frame size changed; reinitializing capture"sv;
      return capture_e::reinit;
    }

    if (!pull_free_image_cb(img_out)) {
      return capture_e::interrupted;
    }
    auto img = (img_t *) img_out.get();

    // ARGB rows are tightly packed from our point of view; reuse the RAM
    // image finalization for buffer management.
    img_info.RowPitch = (UINT) (width * 4);
    if (complete_img(img, false)) {
      return capture_e::error;
    }

    // Crop the requested display out of the full-desktop buffer. The buffer
    // origin is the virtual-desktop origin, matching offset_x/offset_y which
    // are relative to SM_XVIRTUALSCREEN/SM_YVIRTUALSCREEN.
    const std::size_t src_stride = (std::size_t) session.buffer_stride_pixels() * 4;
    const std::uint8_t *src = session.frame_buffer()
      + (std::size_t) offset_y * src_stride
      + (std::size_t) offset_x * 4;
    std::uint8_t *dst = (std::uint8_t *) img->data;
    for (int y = 0; y < height; ++y) {
      std::copy_n(src + (std::size_t) y * src_stride, (std::size_t) width * 4, dst + (std::size_t) y * img->row_pitch);
    }

    img->frame_timestamp = std::chrono::steady_clock::now();
    return capture_e::ok;
  }

  capture_e display_nvfbc_ram_t::release_snapshot() {
    // ToSys grabs are synchronous copies; no frame is held.
    return capture_e::ok;
  }
}  // namespace platf::dxgi
