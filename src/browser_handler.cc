#include "browser_handler.h"
#include "browser_process_handler.h"
#include "rpc.hpp"
#include <rpc.h>
#include <SDL3/sdl.h>
#include <include/cef_scheme.h>

#include <windows.h>

using json = nlohmann::json;

BrowserHandler::BrowserHandler(BrowserProcessHandler* browserProcessHandler, CefRect pageRectangle)
    : browserProcessHandler(browserProcessHandler),
      pageRectangle(pageRectangle),
      popupRectangle(NULL),
      popupVisible(false) {}

CefRefPtr<CefBrowser> BrowserHandler::GetBrowser() {
  return this->browser;
}

void BrowserHandler::SetBrowser(CefRefPtr<CefBrowser> browser_) {
  this->browser = browser_;
}

CefRefPtr<CefRenderHandler> BrowserHandler::GetRenderHandler() {
  return this;
}

CefRefPtr<CefDisplayHandler> BrowserHandler::GetDisplayHandler() {
  return this;
}

bool BrowserHandler::OnProcessMessageReceived(CefRefPtr<CefBrowser> browser_,
                                 CefRefPtr<CefFrame> frame,
                                 CefProcessId source_process,
                                 CefRefPtr<CefProcessMessage> message) {
    const CefString& name = message->GetName();
  if (name == "RenderProcessHandler.OnNavigate" ||
        name == "RenderProcessHandler.OnMouseOver" || name == "RenderProcessHandler.Eval") {
      CefRefPtr<CefListValue> args = message->GetArgumentList();
      if (!args || args->GetSize() == 0 || args->GetType(0) != VTYPE_STRING) {
        SDL_Log("OnProcessMessageReceived without a string argument");
        return false;
      }
      std::string payload = args->GetString(0).ToString();
      browserProcessHandler->BrowserProcessHandler::SendMessage(payload);
      return true;
    }
    return false;
}

void BrowserHandler::GetViewRect(CefRefPtr<CefBrowser> browser_,
                                 CefRect& rect) {
  rect = pageRectangle;
}

void BrowserHandler::OnPaint(CefRefPtr<CefBrowser> browser_,
                             PaintElementType type,
                             const RectList& dirtyRects,
                             const void* buffer,
                             int width,
                             int height) {
  CefPaintEvent message;
  message.width = width;
  message.width = width;
  message.height = height;
  json j = message;
  browserProcessHandler->SendMessage(j.dump());
}

void BrowserHandler::OnAcceleratedPaint(
    CefRefPtr<CefBrowser> browser_,
    PaintElementType type,
    const RectList& dirtyRects,
    const CefAcceleratedPaintInfo& info) {
  UUID id;
  UuidCreate(&id);
  CefAcceleratedPaintEvent message;
  message.id = id;
  message.browserId = browser_->GetIdentifier();
  message.format = info.format;

  // Duplicate the shared texture handle into the application process
  // (hardcoded PID = 1 for now) before sending it.
  HANDLE sourceHandle = info.shared_texture_handle;
  DWORD appPid = 38524;
  HANDLE appProcess = OpenProcess(PROCESS_DUP_HANDLE, FALSE, appPid);
  if (appProcess == NULL) {
    SDL_Log("OpenProcess(%u) failed: %lu", (unsigned)appPid, GetLastError());
  } else {
    HANDLE dupHandle = NULL;
    if (!DuplicateHandle(GetCurrentProcess(),
                         sourceHandle,
                         appProcess,
                         &dupHandle,
                         0,
                         FALSE,
                         DUPLICATE_SAME_ACCESS)) {
      SDL_Log("DuplicateHandle failed: %lu", GetLastError());
    } else {
      message.sharedTextureHandle = reinterpret_cast<uintptr_t>(dupHandle);
    }
    CloseHandle(appProcess);
  }
  json j = message;
  browserProcessHandler->SendMessage(j.dump());
  browserProcessHandler->WaitForResponse<CefAcknowledgement>(id);
}


bool BrowserHandler::GetScreenInfo(CefRefPtr<CefBrowser> browser_,
                                   CefScreenInfo& screen_info) {
  return false;
}

bool BrowserHandler::GetScreenPoint(CefRefPtr<CefBrowser> browser_,
                                    int viewX,
                                    int viewY,
                                    int& screenX,
                                    int& screenY) {
  return false;
}

void BrowserHandler::OnPopupShow(CefRefPtr<CefBrowser> browser_, bool show) {
  popupVisible = show;
}

void BrowserHandler::OnPopupSize(CefRefPtr<CefBrowser> browser_,
                                 const CefRect& rect) {
  *popupRectangle = rect;
}

void BrowserHandler::OnAddressChange(CefRefPtr<CefBrowser> browser_,
                                    CefRefPtr<CefFrame> frame,
                                    const CefString& url) {
  UUID id;
  UuidCreate(&id);

  CefAddressChangeEvent message;
  message.id = id;
  message.browserId = browser_->GetIdentifier();
  message.url = url.ToString();
  json j = message;
  browserProcessHandler->SendMessage(j.dump());
}

void BrowserHandler::OnTitleChange(CefRefPtr<CefBrowser> browser_,
                                   const CefString& title) {
  UUID id;
  UuidCreate(&id);
  CefTitleChangeEvent message;
  message.id = id;
  message.browserId = browser_->GetIdentifier();
  message.title = title.ToString();
  json j = message;
  browserProcessHandler->SendMessage(j.dump());
}

bool BrowserHandler::OnConsoleMessage(CefRefPtr<CefBrowser> browser_,
                                      cef_log_severity_t level,
                                      const CefString& message,
                                      const CefString& source,
                                      int line) {
  UUID id;
  UuidCreate(&id);
  CefConsoleMessageEvent msg;
  msg.id = id;
  msg.browserId = browser_->GetIdentifier();
  msg.level = static_cast<int>(level);
  msg.message = message.ToString();
  msg.source = source.ToString();
  msg.line = line;
  json j = msg;
  browserProcessHandler->SendMessage(j.dump());
  return true;
}

void BrowserHandler::OnLoadingProgressChange(CefRefPtr<CefBrowser> browser_,
                                              double progress) {
  UUID id;
  UuidCreate(&id);
  CefLoadingProgressChangeEvent message;
  message.id = id;
  message.browserId = browser_->GetIdentifier();
  message.progress = progress;
  json j = message;
  browserProcessHandler->SendMessage(j.dump());
}

bool BrowserHandler::OnCursorChange(CefRefPtr<CefBrowser> browser_,
                                    CefCursorHandle cursor,
                                    cef_cursor_type_t type,
                                    const CefCursorInfo& custom_cursor_info) {
  UUID id;
  UuidCreate(&id);
  CefCursorChangeEvent message;
  message.id = id;
  message.browserId = browser_->GetIdentifier();
  message.cursorHandle = reinterpret_cast<uintptr_t>(cursor);
  message.cursorType = static_cast<int>(type);
  json j = message;
  browserProcessHandler->SendMessage(j.dump());
  return true;
}