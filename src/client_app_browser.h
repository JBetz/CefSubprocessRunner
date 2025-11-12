#ifndef CEF_TESTS_SHARED_RENDERER_CLIENT_APP_BROWSER_H_
#define CEF_TESTS_SHARED_RENDERER_CLIENT_APP_BROWSER_H_
#pragma once

#include <set>

#include "client_app.h"

class ClientAppBrowser : public ClientApp, public CefBrowserProcessHandler {
 public:
  ClientAppBrowser();

 private:
  IMPLEMENT_REFCOUNTING(ClientAppBrowser);
  DISALLOW_COPY_AND_ASSIGN(ClientAppBrowser);
};

#endif  // CEF_TESTS_SHARED_COMMON_CLIENT_APP_BROWSER_H_