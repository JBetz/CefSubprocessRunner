// Copyright (c) 2013 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "client_app_renderer.h"

#include "include/base/cef_logging.h"

// Must match the value in client_handler.cc.
const char kOnFocusMessage[] = "ClientRenderer.OnFocus";
const char kOnFocusOutMessage[] = "ClientRenderer.OnFocusOut";
const char kOnMouseOverMessage[] = "ClientRenderer.OnMouseOver";
const char kOnNavigateMessage[] = "ClientRenderer.OnNavigate";
const char kOnMessageMessage[] = "ClientRenderer.OnMessage";
const char kOnEvalMessage[] = "ClientRenderer.OnEval";

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

class MouseOverHandler : public CefV8Handler {
 public:
  MouseOverHandler() {}

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
  IMPLEMENT_REFCOUNTING(MouseOverHandler);
};

class MessageHandler : public CefV8Handler {
 public:
  MessageHandler() {}

  virtual bool Execute(const CefString& name,
                       CefRefPtr<CefV8Value> object,
                       const CefV8ValueList& arguments,
                       CefRefPtr<CefV8Value>& retval,
                       CefString& exception) override {
    CefRefPtr<CefV8Context> context = CefV8Context::GetCurrentContext();
    CefRefPtr<CefV8Value> window = context->GetGlobal();
    CefRefPtr<CefFrame> frame = context->GetFrame();
    CefRefPtr<CefV8Value> event = arguments.front();
    
    CefRefPtr<CefProcessMessage> message =
        CefProcessMessage::Create(kOnMessageMessage);

    CefRefPtr<CefDictionaryValue> messageArguments =
        CefDictionaryValue::Create();
    CefString source = event->GetValue("source")->GetStringValue();
    messageArguments->SetString("source", source);
    CefRefPtr<CefV8Value> data = event->GetValue("data");

    CefRefPtr<CefV8Value> json = window->GetValue("JSON");
    CefRefPtr<CefV8Value> stringifyFunction = json->GetValue("stringify");
    CefV8ValueList stringifyArguments;
    stringifyArguments.push_back(data);
    const CefString& stringifiedData =
        stringifyFunction->ExecuteFunction(json, stringifyArguments)
            ->GetStringValue();
    messageArguments->SetString("data", stringifiedData);
    message->GetArgumentList()->SetDictionary(0, messageArguments);
    frame->SendProcessMessage(PID_BROWSER, message);
    return true;
  }

  // Provide the reference counting implementation for this class.
  IMPLEMENT_REFCOUNTING(MessageHandler);
};

class NavigateHandler : public CefV8Handler {
 public:
  NavigateHandler() {}

  virtual bool Execute(const CefString& name,
                       CefRefPtr<CefV8Value> object,
                       const CefV8ValueList& arguments,
                       CefRefPtr<CefV8Value>& retval,
                       CefString& exception) override {
    CefRefPtr<CefV8Context> context = CefV8Context::GetCurrentContext();
    CefRefPtr<CefFrame> frame = context->GetFrame();
    CefRefPtr<CefV8Value> event = arguments.front();
    
    // Initialize process message
    CefRefPtr<CefProcessMessage> message = CefProcessMessage::Create(kOnNavigateMessage);
    CefRefPtr<CefDictionaryValue> messageArguments = CefDictionaryValue::Create();

    // Extract navigation information
    CefRefPtr<CefV8Value> destination = event->GetValue("destination");
    CefRefPtr<CefDictionaryValue> destinationFields =
        CefDictionaryValue::Create();
    destinationFields->SetString("id", destination->GetValue("id")->GetStringValue());
    destinationFields->SetInt("index",
                                 destination->GetValue("index")->GetIntValue());
    destinationFields->SetString("key",
                                 destination->GetValue("key")->GetStringValue());
    destinationFields->SetBool(
        "sameDocument", destination->GetValue("sameDocument")->GetBoolValue());
    destinationFields->SetString(
        "url", destination->GetValue("url")->GetStringValue());
    messageArguments->SetDictionary("destination", destinationFields);

    CefRefPtr<CefV8Value> formData = event->GetValue("formData");
    if (!formData->IsNull()) {
      // TODO: Populate dictionary with form fields
      CefRefPtr<CefDictionaryValue> formFields =
          CefDictionaryValue::Create();
      messageArguments->SetDictionary("formData", formFields);
    }

    bool hashChange = event->GetValue("hashChange")->GetBoolValue();
    messageArguments->SetBool("hashChange", hashChange);
    
    CefString navigationType = event->GetValue("navigationType")->GetStringValue();
    messageArguments->SetString("navigationType", navigationType);
    
    bool userInitiated = event->GetValue("userInitiated")->GetBoolValue();
    messageArguments->SetBool("userInitiated", userInitiated);

    // Send to browser process
    message->GetArgumentList()->SetDictionary(0, messageArguments);
    frame->SendProcessMessage(PID_BROWSER, message);
    
    return true;
  }

  // Provide the reference counting implementation for this class.
  IMPLEMENT_REFCOUNTING(NavigateHandler);
};

class FocusOutHandler : public CefV8Handler {
 public:
  FocusOutHandler() {}

  virtual bool Execute(const CefString& name,
                       CefRefPtr<CefV8Value> object,
                       const CefV8ValueList& arguments,
                       CefRefPtr<CefV8Value>& retval,
                       CefString& exception) override {
    CefRefPtr<CefV8Context> context = CefV8Context::GetCurrentContext();
    CefRefPtr<CefFrame> frame = context->GetFrame();
    CefRefPtr<CefV8Value> event = arguments.front();
    CefRefPtr<CefV8Value> relatedTarget = event->GetValue("relatedTarget");
    CefRefPtr<CefProcessMessage> message =
        CefProcessMessage::Create(kOnFocusOutMessage);
    CefRefPtr<CefDictionaryValue> messageArguments =
        CefDictionaryValue::Create();

    if (!relatedTarget->IsNull()) {
      messageArguments->SetString(
          "tagName", relatedTarget->GetValue("tagName")->GetStringValue());
      CefRefPtr<CefV8Value> attributes = relatedTarget->GetValue("attributes");
      messageArguments->SetString(
          "type", attributes->GetValue("type")->GetStringValue());
      messageArguments->SetBool(
          "isEditable", attributes->GetValue("isEditable")->GetBoolValue());
    }
    message->GetArgumentList()->SetDictionary(0, messageArguments);
    frame->SendProcessMessage(PID_BROWSER, message);
    return true;
  }

  // Provide the reference counting implementation for this class.
  IMPLEMENT_REFCOUNTING(FocusOutHandler);
};

void ClientAppRenderer::OnContextCreated(CefRefPtr<CefBrowser> browser,
                                         CefRefPtr<CefFrame> frame,
                                         CefRefPtr<CefV8Context> context) {
  CefRefPtr<CefV8Value> window = context->GetGlobal();
  
  // mouse over
  CefRefPtr<CefV8Handler> mouseOverHandler = new MouseOverHandler();
  CefV8ValueList mouseOverArguments;
  mouseOverArguments.push_back(CefV8Value::CreateString("mouseover"));
  mouseOverArguments.push_back(CefV8Value::CreateFunction("onMouseOver", mouseOverHandler));
  window->GetValue("addEventListener")->ExecuteFunction(window, mouseOverArguments);

  // navigation
  CefRefPtr<CefV8Value> navigation = window->GetValue("navigation");
  CefRefPtr<CefV8Handler> navigateHandler = new NavigateHandler();
  CefV8ValueList navigateArguments;
  navigateArguments.push_back(CefV8Value::CreateString("navigate"));
  navigateArguments.push_back(
      CefV8Value::CreateFunction("onNavigate", navigateHandler));
  navigation->GetValue("addEventListener")->ExecuteFunction(navigation, navigateArguments);

  // focus out
  CefRefPtr<CefV8Value> document = window->GetValue("document");
  CefRefPtr<CefV8Handler> focusOutHandler = new FocusOutHandler();
  CefV8ValueList focusOutArguments;
  focusOutArguments.push_back(CefV8Value::CreateString("focusout"));
  focusOutArguments.push_back(
      CefV8Value::CreateFunction("onFocusOut", focusOutHandler));
  document->GetValue("addEventListener")
      ->ExecuteFunction(document, focusOutArguments);

  // message
  CefRefPtr<CefV8Handler> messageHandler = new MessageHandler();
  CefV8ValueList messageArguments;
  messageArguments.push_back(CefV8Value::CreateString("message"));
  messageArguments.push_back(
      CefV8Value::CreateFunction("onMessage", messageHandler));
  window->GetValue("addEventListener")
      ->ExecuteFunction(window, messageArguments);
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

// Custom handler for our promise "then" callback
class PromiseThenHandler : public CefV8Handler {
 public:
  explicit PromiseThenHandler(CefRefPtr<CefFrame> frame, CefProcessId sourceProcessId, const CefString messageId)
      : frame(frame), sourceProcessId(sourceProcessId), messageId(messageId) {}

  bool Execute(const CefString& name,
               CefRefPtr<CefV8Value> object,
               const CefV8ValueList& arguments,
               CefRefPtr<CefV8Value>& retval,
               CefString& exception) override {
    if (name != "onPromiseResolved" || arguments.size() == 0) {
      return false;
    }
    CefRefPtr<CefV8Context> context = frame->GetV8Context();
    CefRefPtr<CefV8Value> window = context->GetGlobal();
    CefRefPtr<CefV8Value> json = window->GetValue("JSON");
    CefRefPtr<CefV8Value> stringifyFunction = json->GetValue("stringify");
    CefV8ValueList stringifyArguments;
    stringifyArguments.push_back(arguments[0]);
    const CefString& result =
        stringifyFunction->ExecuteFunction(json, stringifyArguments)
            ->GetStringValue();
    CefRefPtr<CefProcessMessage> responseMessage =
        CefProcessMessage::Create(kOnEvalMessage);
    CefRefPtr<CefDictionaryValue> messageArguments =
        CefDictionaryValue::Create();
    messageArguments->SetString("id", messageId);
    messageArguments->SetString("success", result);
    responseMessage->GetArgumentList()->SetDictionary(0, messageArguments);
    frame->SendProcessMessage(sourceProcessId, responseMessage);
    retval = arguments[0];
    return true;
  }

 private:
  CefRefPtr<CefFrame> frame;
  CefProcessId sourceProcessId;
  const CefString messageId;
  IMPLEMENT_REFCOUNTING(PromiseThenHandler);
};

bool ClientAppRenderer::OnProcessMessageReceived(
    CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefFrame> frame,
    CefProcessId source_process,
    CefRefPtr<CefProcessMessage> message) {
  DCHECK_EQ(source_process, PID_BROWSER);

  CefRefPtr<CefV8Context> context = frame->GetV8Context();
  context->Enter();
  const CefString& name = message->GetName();
  bool handled = false;
  if (name == "Eval") {
    CefRefPtr<CefListValue> argumentList = message->GetArgumentList();
    CefRefPtr<CefDictionaryValue> arguments = argumentList->GetDictionary(0);
    const CefString& id = arguments->GetString("id");
    const CefString& code = arguments->GetString("code");
    const CefString& script_url = arguments->GetString("scriptUrl");
    int start_line = arguments->GetInt("startLine");
    CefRefPtr<CefV8Value> retval;
    CefRefPtr<CefV8Exception> exception;
    bool success =
        context->Eval(code, script_url, start_line, retval, exception);
    if (success && retval->IsPromise()) {
      CefRefPtr<CefV8Value> thenFunction = retval->GetValue("then");
      CefRefPtr<PromiseThenHandler> handler =
          new PromiseThenHandler(frame, source_process, id);
      CefRefPtr<CefV8Value> onResolvedFunc =
          CefV8Value::CreateFunction("onPromiseResolved", handler);
      thenFunction->ExecuteFunction(retval, {onResolvedFunc});
     } else {
      CefRefPtr<CefProcessMessage> responseMessage =
          CefProcessMessage::Create(kOnEvalMessage);
      CefRefPtr<CefDictionaryValue> messageArguments =
          CefDictionaryValue::Create();
      messageArguments->SetString("id", id);

      if (success) {
        CefRefPtr<CefV8Value> window = context->GetGlobal();
        CefRefPtr<CefV8Value> json = window->GetValue("JSON");
        CefRefPtr<CefV8Value> stringifyFunction = json->GetValue("stringify");
        CefV8ValueList stringifyArguments;
        stringifyArguments.push_back(retval);
        const CefString& result =
            stringifyFunction->ExecuteFunction(json, stringifyArguments)
                ->GetStringValue();
        messageArguments->SetString("success", result);
      } else {
        CefRefPtr<CefDictionaryValue> errorData = CefDictionaryValue::Create();
        errorData->SetInt("endColumn", exception->GetEndColumn());
        errorData->SetInt("endPosition", exception->GetEndPosition());
        errorData->SetInt("lineNumber", exception->GetLineNumber());
        errorData->SetString("message", exception->GetMessage());
        errorData->SetString("endColumn", exception->GetScriptResourceName());
        errorData->SetString("sourceLine", exception->GetSourceLine());
        errorData->SetInt("startColumn", exception->GetStartColumn());
        errorData->SetInt("startPosition", exception->GetStartPosition());
        messageArguments->SetDictionary("error", errorData);
      }
      responseMessage->GetArgumentList()->SetDictionary(0, messageArguments);
      frame->SendProcessMessage(source_process, responseMessage);
    }
  }
  handled = true;
  context->Exit();

  DelegateSet::iterator it = delegates_.begin();
  for (; it != delegates_.end() && !handled; ++it) {
    handled = (*it)->OnProcessMessageReceived(this, browser, frame,
                                              source_process, message);
  }

  return handled;
}