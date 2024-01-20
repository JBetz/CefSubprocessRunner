// Copyright (c) 2013 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "client_app_renderer.h"

#include "include/base/cef_logging.h"

// Must match the value in client_handler.cc.
const char kOnFocusMessage[] = "ClientRenderer.OnFocus";
const char kOnMouseOverMessage[] = "ClientRenderer.OnMouseOver";

ClientAppRenderer::ClientAppRenderer() {
  CreateDelegates(delegates_);
}

void ClientAppRenderer::OnWebKitInitialized() {
  DelegateSet::iterator it = delegates_.begin();
  for (; it != delegates_.end(); ++it) {
    (*it)->OnWebKitInitialized(this);
  }
}

void ClientAppRenderer::OnBrowserCreated(
    CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefDictionaryValue> extra_info) {
  DelegateSet::iterator it = delegates_.begin();
  for (; it != delegates_.end(); ++it) {
    (*it)->OnBrowserCreated(this, browser, extra_info);
  }
}

void ClientAppRenderer::OnBrowserDestroyed(CefRefPtr<CefBrowser> browser) {
  DelegateSet::iterator it = delegates_.begin();
  for (; it != delegates_.end(); ++it) {
    (*it)->OnBrowserDestroyed(this, browser);
  }
}

CefRefPtr<CefLoadHandler> ClientAppRenderer::GetLoadHandler() {
  CefRefPtr<CefLoadHandler> load_handler;
  DelegateSet::iterator it = delegates_.begin();
  for (; it != delegates_.end() && !load_handler.get(); ++it) {
    load_handler = (*it)->GetLoadHandler(this);
  }

  return load_handler;
}

class MyV8Handler : public CefV8Handler {
 public:
  MyV8Handler() {}

  virtual bool Execute(const CefString& name,
                       CefRefPtr<CefV8Value> object,
                       const CefV8ValueList& arguments,
                       CefRefPtr<CefV8Value>& retval,
                       CefString& exception) override {
    CefRefPtr<CefV8Context> context = CefV8Context::GetCurrentContext();
    CefRefPtr<CefFrame> frame = context->GetFrame();
    CefRefPtr<CefV8Value> event = arguments.front();
    CefV8ValueList stopPropagationArguments;
    event->GetValue("stopPropagation")
        ->ExecuteFunction(event, stopPropagationArguments);
    CefRefPtr<CefV8Value> target = event->GetValue("target");

    CefRefPtr<CefProcessMessage> message =
        CefProcessMessage::Create(kOnMouseOverMessage);

    CefRefPtr<CefDictionaryValue> messageArguments =
        CefDictionaryValue::Create();
    CefString tagName = target->GetValue("tagName")->GetStringValue();
    messageArguments->SetString("tagName", tagName);
    if (tagName == "INPUT") {
      messageArguments->SetString("type",
                                  target->GetValue("type")->GetStringValue());
    }
    CefV8ValueList closestArguments;
    closestArguments.push_back(CefV8Value::CreateString("a"));
    target->GetValue("closest")->ExecuteFunction(target, closestArguments);

    CefRefPtr<CefV8Value> closestResult =
        target->GetValue("closest")->ExecuteFunction(target, closestArguments);
    CefV8ValueList getBoundingClientRectArguments;
    CefRefPtr<CefV8Value> rectangle;
    if (closestResult->IsObject()) {
      messageArguments->SetString(
          "href", closestResult->GetValue("href")->GetStringValue());
      rectangle =
          closestResult->GetValue("getBoundingClientRect")
              ->ExecuteFunction(closestResult, getBoundingClientRectArguments);
    } else {
      rectangle = target->GetValue("getBoundingClientRect")
                      ->ExecuteFunction(target, getBoundingClientRectArguments);
    }
    messageArguments->SetDouble("top",
                                rectangle->GetValue("top")->GetDoubleValue());
    messageArguments->SetDouble("left",
                                rectangle->GetValue("left")->GetDoubleValue());
    messageArguments->SetDouble(
        "bottom", rectangle->GetValue("bottom")->GetDoubleValue());
    messageArguments->SetDouble("right",
                                rectangle->GetValue("right")->GetDoubleValue());
    message->GetArgumentList()->SetDictionary(0, messageArguments);
    frame->SendProcessMessage(PID_BROWSER, message);
    return true;
  }

  // Provide the reference counting implementation for this class.
  IMPLEMENT_REFCOUNTING(MyV8Handler);
};

void ClientAppRenderer::OnContextCreated(CefRefPtr<CefBrowser> browser,
                                         CefRefPtr<CefFrame> frame,
                                         CefRefPtr<CefV8Context> context) {
  CefRefPtr<CefV8Value> window = context->GetGlobal();
  CefRefPtr<CefV8Handler> handler = new MyV8Handler();
  CefV8ValueList arguments;
  arguments.push_back(CefV8Value::CreateString("mouseover"));
  arguments.push_back(CefV8Value::CreateFunction("onMouseOver", handler));
  CefRefPtr<CefV8Value> result =
      window->GetValue("addEventListener")->ExecuteFunction(window, arguments);
}

void ClientAppRenderer::OnContextReleased(CefRefPtr<CefBrowser> browser,
                                          CefRefPtr<CefFrame> frame,
                                          CefRefPtr<CefV8Context> context) {
  DelegateSet::iterator it = delegates_.begin();
  for (; it != delegates_.end(); ++it) {
    (*it)->OnContextReleased(this, browser, frame, context);
  }
}

void ClientAppRenderer::OnUncaughtException(
    CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefFrame> frame,
    CefRefPtr<CefV8Context> context,
    CefRefPtr<CefV8Exception> exception,
    CefRefPtr<CefV8StackTrace> stackTrace) {
  DelegateSet::iterator it = delegates_.begin();
  for (; it != delegates_.end(); ++it) {
    (*it)->OnUncaughtException(this, browser, frame, context, exception,
                               stackTrace);
  }
}

void ClientAppRenderer::OnFocusedNodeChanged(CefRefPtr<CefBrowser> browser,
                                             CefRefPtr<CefFrame> frame,
                                             CefRefPtr<CefDOMNode> node) {
  if (node.get()) {
    CefRefPtr<CefProcessMessage> message =
        CefProcessMessage::Create(kOnFocusMessage);
    CefRefPtr<CefDictionaryValue> messageArguments =
        CefDictionaryValue::Create();
    messageArguments->SetString("tagName", node->GetElementTagName());
    messageArguments->SetString("type", node->GetElementAttribute("type"));
    messageArguments->SetBool("isEditable", node->IsEditable());
    message->GetArgumentList()->SetDictionary(0, messageArguments);
    frame->SendProcessMessage(PID_BROWSER, message);
  }
}

bool ClientAppRenderer::OnProcessMessageReceived(
    CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefFrame> frame,
    CefProcessId source_process,
    CefRefPtr<CefProcessMessage> message) {
  DCHECK_EQ(source_process, PID_BROWSER);

  bool handled = false;

  DelegateSet::iterator it = delegates_.begin();
  for (; it != delegates_.end() && !handled; ++it) {
    handled = (*it)->OnProcessMessageReceived(this, browser, frame,
                                              source_process, message);
  }

  return handled;
}