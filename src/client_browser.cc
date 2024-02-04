// Copyright (c) 2016 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "client_browser.h"
#include "main_context.h"

#include "include/base/cef_logging.h"
#include "include/cef_command_line.h"
#include "include/cef_crash_util.h"
#include "include/cef_file_util.h"
#include "default_client_handler.h"
#include "main_context.h"
#include "root_window_manager.h"
#include "client_switches.h"

namespace client {
namespace browser {

namespace {

class ClientBrowserDelegate : public ClientAppBrowser::Delegate {
 public:
  ClientBrowserDelegate() {}

  void OnRegisterCustomPreferences(
      CefRefPtr<ClientAppBrowser> app,
      cef_preferences_type_t type,
      CefRawPtr<CefPreferenceRegistrar> registrar) override {
  }

  void OnContextInitialized(CefRefPtr<ClientAppBrowser> app) override {
    if (CefCrashReportingEnabled()) {
      // Set some crash keys for testing purposes. Keys must be defined in the
      // "crash_reporter.cfg" file. See cef_crash_util.h for details.
      CefSetCrashKeyValue("testkey_small1", "value1_small_browser");
      CefSetCrashKeyValue("testkey_small2", "value2_small_browser");
      CefSetCrashKeyValue("testkey_medium1", "value1_medium_browser");
      CefSetCrashKeyValue("testkey_medium2", "value2_medium_browser");
      CefSetCrashKeyValue("testkey_large1", "value1_large_browser");
      CefSetCrashKeyValue("testkey_large2", "value2_large_browser");
    }

    const std::string& crl_sets_path =
        CefCommandLine::GetGlobalCommandLine()->GetSwitchValue(
            switches::kCRLSetsPath);
    if (!crl_sets_path.empty()) {
      // Load the CRLSets file from the specified path.
      CefLoadCRLSetsFile(crl_sets_path);
    }
  }

  void OnBeforeCommandLineProcessing(
      CefRefPtr<ClientAppBrowser> app,
      CefRefPtr<CefCommandLine> command_line) override {
    // Append Chromium command line parameters if touch events are enabled
    if (MainContext::Get()->TouchEventsEnabled()) {
      command_line->AppendSwitchWithValue("touch-events", "enabled");
    }
  }

  bool OnAlreadyRunningAppRelaunch(
      CefRefPtr<ClientAppBrowser> app,
      CefRefPtr<CefCommandLine> command_line,
      const CefString& current_directory) override {
    // Add logging for some common switches that the user may attempt to use.
    static const char* kIgnoredSwitches[] = {
        switches::kEnableChromeRuntime,
        switches::kMultiThreadedMessageLoop,
        switches::kOffScreenRenderingEnabled,
        switches::kUseViews,
    };
    for (size_t i = 0;
         i < sizeof(kIgnoredSwitches) / sizeof(kIgnoredSwitches[0]); ++i) {
      if (command_line->HasSwitch(kIgnoredSwitches[i])) {
        LOG(WARNING) << "The --" << kIgnoredSwitches[i]
                     << " command-line switch is ignored on app relaunch.";
      }
    }

    // Create a new root window based on |command_line|.
    auto config = std::make_unique<RootWindowConfig>(command_line->Copy());

    MainContext::Get()->GetRootWindowManager()->CreateRootWindow(
        std::move(config));

    // Relaunch was handled.
    return true;
  }

  CefRefPtr<CefClient> GetDefaultClient(
      CefRefPtr<ClientAppBrowser> app) override {
    // Default client handler for unmanaged browser windows. Used with the
    // Chrome runtime only.
    LOG(INFO) << "Creating a chrome browser with the default client";
    return new DefaultClientHandler();
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(ClientBrowserDelegate);
  IMPLEMENT_REFCOUNTING(ClientBrowserDelegate);
};

}  // namespace

void CreateDelegates(ClientAppBrowser::DelegateSet& delegates) {
  delegates.insert(new ClientBrowserDelegate);
}

}  // namespace browser
}  // namespace client