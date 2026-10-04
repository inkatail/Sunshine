/**
 * @file tools/dxgi.cpp
 * @brief Displays information about connected displays and GPUs
 */
// windows-legacy: target Windows 7; resolve per-monitor-V2 DPI awareness at runtime.
#define WINVER 0x0601
#define _WIN32_WINNT 0x0601
#include "src/platform/windows/utf_utils.h"
#include "src/utility.h"

#include <Windows.h>
#include <d3dcommon.h>
#include <dxgi.h>
#include <format>
#include <iostream>

// windows-legacy: with WINVER=0x0601 the Win10 DPI-awareness-context constants are hidden.
// Define them locally for the GetProcAddress path (values from winuser.h, Win10 1703+).
#ifndef DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
  #define SUNSHINE_DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 ((DPI_AWARENESS_CONTEXT) -4)
#else
  #define SUNSHINE_DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
#endif
#ifndef DPI_AWARENESS_CONTEXT
DECLARE_HANDLE(DPI_AWARENESS_CONTEXT);
#endif

using namespace std::literals;

namespace dxgi {
  template<class T>
  void Release(T *dxgi) {
    dxgi->Release();
  }

  using factory1_t = util::safe_ptr<IDXGIFactory1, Release<IDXGIFactory1>>;
  using adapter_t = util::safe_ptr<IDXGIAdapter1, Release<IDXGIAdapter1>>;
  using output_t = util::safe_ptr<IDXGIOutput, Release<IDXGIOutput>>;
}  // namespace dxgi

int main(int argc, char *argv[]) {
  // Set ourselves as per-monitor DPI aware for accurate resolution values on High DPI systems.
  // windows-legacy: SetProcessDpiAwarenessContext is Win10 1703+. Resolve dynamically and
  // fall back to SetProcessDPIAware (Vista+) so this tool still runs on 7/8.x.
  {
    using set_dpi_awareness_ctx_fn = BOOL(WINAPI *)(DPI_AWARENESS_CONTEXT);
    auto user32 = LoadLibraryW(L"user32.dll");
    auto fn = user32 ? reinterpret_cast<set_dpi_awareness_ctx_fn>(GetProcAddress(user32, "SetProcessDpiAwarenessContext")) : nullptr;
    if (fn) {
      fn(SUNSHINE_DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    } else {
      SetProcessDPIAware();
    }
    if (user32) {
      FreeLibrary(user32);
    }
  }

  dxgi::factory1_t::pointer factory_p {};
  const HRESULT status = CreateDXGIFactory1(IID_IDXGIFactory1, static_cast<void **>(static_cast<void *>(&factory_p)));
  dxgi::factory1_t factory {factory_p};
  if (FAILED(status)) {
    std::cout << "Failed to create DXGIFactory1 [0x"sv << util::hex(status).to_string_view() << ']' << std::endl;
    return -1;
  }

  dxgi::adapter_t::pointer adapter_p {};
  for (int x = 0; factory->EnumAdapters1(x, &adapter_p) != DXGI_ERROR_NOT_FOUND; ++x) {
    dxgi::adapter_t adapter {adapter_p};

    DXGI_ADAPTER_DESC1 adapter_desc;
    adapter->GetDesc1(&adapter_desc);

    std::cout << "====== ADAPTER =====" << std::endl;
    std::cout << "Device Name       : " << utf_utils::to_utf8(std::wstring(adapter_desc.Description)) << std::endl;
    std::cout << "Device Vendor ID  : " << "0x" << util::hex(adapter_desc.VendorId).to_string() << std::endl;
    std::cout << "Device Device ID  : " << "0x" << util::hex(adapter_desc.DeviceId).to_string() << std::endl;
    std::cout << "Device Video Mem  : " << std::format("{} MiB", adapter_desc.DedicatedVideoMemory / 1048576) << std::endl;
    std::cout << "Device Sys Mem    : " << std::format("{} MiB", adapter_desc.DedicatedSystemMemory / 1048576) << std::endl;
    std::cout << "Share Sys Mem     : " << std::format("{} MiB", adapter_desc.SharedSystemMemory / 1048576) << std::endl;

    dxgi::output_t::pointer output_p {};
    bool has_outputs = false;
    for (int y = 0; adapter->EnumOutputs(y, &output_p) != DXGI_ERROR_NOT_FOUND; ++y) {
      // Print the header only when we find the first output
      if (!has_outputs) {
        std::cout << std::endl
                  << "    ====== OUTPUT ======" << std::endl;
        has_outputs = true;
      }

      dxgi::output_t output {output_p};

      DXGI_OUTPUT_DESC desc;
      output->GetDesc(&desc);

      auto width = desc.DesktopCoordinates.right - desc.DesktopCoordinates.left;
      auto height = desc.DesktopCoordinates.bottom - desc.DesktopCoordinates.top;

      std::cout << "    Output Name       : " << utf_utils::to_utf8(std::wstring(desc.DeviceName)) << std::endl;
      std::cout << "    AttachedToDesktop : " << (desc.AttachedToDesktop ? "yes" : "no") << std::endl;
      std::cout << "    Resolution        : " << std::format("{}x{}", width, height) << std::endl;
    }
    std::cout << std::endl;
  }

  return 0;
}
