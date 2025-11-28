#pragma once

#include "include/internal/cef_types_wrappers.h"
#include "json.hpp"
#include <optional>
#include <rpc.h>
#include <string>

using json = nlohmann::json;

// HANDLE
inline void to_json(json& j, const HANDLE& m) {
  j = static_cast<std::uint64_t>(reinterpret_cast<uintptr_t>(m));
}

inline void from_json(const json& j, HANDLE& m) {
  std::uint64_t v = j.get<std::uint64_t>();
  m = reinterpret_cast<HANDLE>(static_cast<uintptr_t>(v));
}

// UUID
inline void to_json(json& j, const UUID& m) {
  RPC_CSTR str;
  UuidToStringA(const_cast<UUID*>(&m), &str);
  j = std::string(reinterpret_cast<char*>(str));
  RpcStringFreeA(&str);
}

inline void from_json(const json& j, UUID& m) {
  std::string str = j.get<std::string>();
  RPC_CSTR rpcStr = reinterpret_cast<RPC_CSTR>(const_cast<char*>(str.c_str()));
  UuidFromStringA(rpcStr, &m);
}

// CEF types
inline void to_json(json& j, const CefRect& m) {
  j = json::object();
  j["x"] = m.x;
  j["y"] = m.y;
  j["width"] = m.width;
  j["height"] = m.height;
}

inline void from_json(const json& j, CefRect& r) {
  j.at("x").get_to(r.x);
  j.at("y").get_to(r.y);
  j.at("width").get_to(r.width);
  j.at("height").get_to(r.height);
}

inline void to_json(json& j, const CefPoint& m) {
  j = json::object();
  j["x"] = m.x;
  j["y"] = m.y;
}

inline void to_json(json& j, const CefSize& m) {
  j = json::object();
  j["width"] = m.width;
  j["height"] = m.height;
}

inline void to_json(json& j, const CefCursorInfo& m) {
  j = json::object();
  j["hotspot"] = m.hotspot;
  j["image_scale_factor"] = m.image_scale_factor;
  j["buffer"] = "";
  j["size"] = m.size;
}

// Request messages
struct CefCreateBrowserRequest {
  UUID id;
  std::string url;
  CefRect rectangle;
  std::optional<std::string> html;
};

inline void from_json(const json& j, CefCreateBrowserRequest& m) {
  j.at("id").get_to(m.id);
  j.at("url").get_to(m.url);
  j.at("rectangle").get_to(m.rectangle);
  j.at("html").get_to(m.html);
}

struct CefEvalRequest {
  UUID id;
  int browserId;
  std::string code;
  std::string scriptUrl;
  int startLine;
};

inline void from_json(const json& j, CefEvalRequest& m) {
  j.at("id").get_to(m.id);
  j.at("browserId").get_to(m.browserId);
  j.at("code").get_to(m.code);
  j.at("scriptUrl").get_to(m.scriptUrl);
  j.at("startLine").get_to(m.startLine);
}

inline void from_json(const json& j, CefMouseEvent& m) {
  j.at("x").get_to(m.x);
  j.at("y").get_to(m.y);
  j.at("modifiers").get_to(m.modifiers);
}

struct CefMouseClickEvent {
  UUID id;
  int browserId;
  CefMouseEvent mouseEvent;
  int button;
  bool mouseUp;
  int clickCount;
};

inline void from_json(const json& j, CefMouseClickEvent& m) {
  j.at("id").get_to(m.id);
  j.at("browserId").get_to(m.browserId);
  j.at("mouseEvent").get_to(m.mouseEvent);
  j.at("button").get_to(m.button);
  j.at("mouseUp").get_to(m.mouseUp);
  j.at("clickCount").get_to(m.clickCount);
}

struct CefMouseMoveEvent {
  UUID id;
  int browserId;
  CefMouseEvent mouseEvent;
  bool mouseLeave;
};

inline void from_json(const json& j, CefMouseMoveEvent& m) {
  j.at("id").get_to(m.id);
  j.at("browserId").get_to(m.browserId);
  j.at("mouseEvent").get_to(m.mouseEvent);
  j.at("mouseLeave").get_to(m.mouseLeave);
}

struct CefMouseWheelEvent {
  UUID id;
  int browserId;
  CefMouseEvent mouseEvent;
  int deltaX;
  int deltaY;
};

inline void from_json(const json& j, CefMouseWheelEvent& m) {
  j.at("id").get_to(m.id);
  j.at("browserId").get_to(m.browserId);
  j.at("mouseEvent").get_to(m.mouseEvent);
  j.at("deltaX").get_to(m.deltaX);
  j.at("deltaY").get_to(m.deltaY);
}

struct CefKeyboardEvent {
  UUID id;
  int browserId;
  CefKeyEvent keyEvent;
};

inline void from_json(const json& j, CefKeyEvent& m) {
  j.at("type").get_to(m.type);
  j.at("modifiers").get_to(m.modifiers);
  j.at("windows_key_code").get_to(m.windows_key_code);
  j.at("native_key_code").get_to(m.native_key_code);
  j.at("is_system_key").get_to(m.is_system_key);
  std::string character = j.at("character");
  m.character = character.empty() ? 0 : character[0];
  std::string unmodified_character = j.at("unmodified_character");
  m.unmodified_character = unmodified_character.empty() ? 0 : unmodified_character[0];
  j.at("focus_on_editable_field").get_to(m.focus_on_editable_field);
}

inline void from_json(const json& j, CefKeyboardEvent& m) {
  j.at("id").get_to(m.id);
  j.at("browserId").get_to(m.browserId);
  j.at("keyEvent").get_to(m.keyEvent);
}

struct CefNavigateDestination {
  std::string id;
  int index;
  std::string key;
  bool sameDocument;
  std::string url;
};

inline void to_json(json& j, const CefNavigateDestination& m) {
  j = json::object();
  j["id"] = m.id;
  j["index"] = m.index;
  j["key"] = m.key;
  j["sameDocument"] = m.sameDocument;
  j["url"] = m.url;
}

struct CefNavigateEvent {
  UUID id;
  int browserId;
  CefNavigateDestination destination;
  std::optional<std::map<std::string, std::string>> formData;
  bool hashChange;
  std::string navigationType;
  bool userInitiated;
};

inline void to_json(json& j, const CefNavigateEvent& m) {
  j = json::object();
  j["type"] = "CefNavigateEvent";
  j["id"] = m.id;
  j["browserId"] = m.browserId;
  j["destination"] = m.destination;
  if (m.formData.has_value()) {
    j["formData"] = m.formData.value();
  } else {
    j["formData"] = nullptr;
  }
  j["hashChange"] = m.hashChange;
  j["navigationType"] = m.navigationType;
  j["userInitiated"] = m.userInitiated;
}

struct CefMouseOverEvent {
  UUID id;
  int browserId;
  std::string tagName;
  std::optional<std::string> inputType;
  std::string href;
  CefRect rectangle;
};

inline void to_json(json& j, const CefMouseOverEvent& m) {
  j = json::object();
  j["type"] = "CefMouseOverEvent";
  j["id"] = m.id;
  j["browserId"] = m.browserId;
  j["tagName"] = m.tagName;
  if (m.inputType.has_value()) {
    j["inputType"] = m.inputType.value();
  }
  j["href"] = m.href;
  j["rectangle"] = m.rectangle;
}

// Response messages
struct CefCreateBrowserResponse {
  UUID id;
  int browserId;
};

inline void to_json(json& j, const CefCreateBrowserResponse& m) {
  j = json::object();
  j["type"] = "CefCreateBrowserResponse";
  j["id"] = m.id;
  j["browserId"] = m.browserId;
}

struct CefEvalError {
  int endColumn;
  int endPosition;
  int lineNumber;
  std::string message;
  std::string scriptResourceName;
  std::string sourceLine;
  int startColumn;
  int startPosition;
};

inline void to_json(json& j, const CefEvalError& m) {
  j = json::object();
  j["endColumn"] = m.endColumn;
  j["endPosition"] = m.endPosition;
  j["lineNumber"] = m.lineNumber;
  j["message"] = m.message;
  j["scriptResourceName"] = m.scriptResourceName;
  j["sourceLine"] = m.sourceLine;
  j["startColumn"] = m.startColumn;
  j["startPosition"] = m.startPosition;
}

struct CefEvalResponse {
  UUID id;
  int browserId;
  bool success;
  std::optional<CefEvalError> error;
  std::optional<std::string> result;
};

inline void to_json(json& j, const CefEvalResponse& m) {
  j = json::object();
  j["type"] = "CefEvalResponse";
  j["id"] = m.id;
  j["browserId"] = m.browserId;
  j["success"] = m.success;
  if (m.error.has_value()) {
    j["error"] = m.error.value();
  }
  if (m.result.has_value()) {
    j["result"] = m.result.value();
  }
}

struct CefPaintEvent {
  UUID id;
  int browserId;
  int width;
  int height;
};

inline void to_json(json& j, const CefPaintEvent& m) {
  j = json::object();
  j["type"] = "CefPaintEvent";
  j["id"] = m.id;
  j["browserId"] = m.browserId;
  j["width"] = m.width;
  j["height"] = m.height;
}

struct CefAcceleratedPaintEvent {
  UUID id;
  int browserId;
  uintptr_t sharedTextureHandle;
  int format;
};

inline void to_json(json& j, const CefAcceleratedPaintEvent& m) {
  j = json::object();
  j["type"] = "CefAcceleratedPaintEvent";
  j["id"] = m.id;
  j["browserId"] = m.browserId;
  j["sharedTextureHandle"] = m.sharedTextureHandle;
  j["format"] = m.format;
}

struct CefCursorChangeEvent {
  UUID id;
  int browserId;
  uintptr_t cursorHandle;
  int cursorType;
  std::optional<CefCursorInfo> customCursorInfo;
};

inline void to_json(json& j, const CefCursorChangeEvent& m) {
  j = json::object();
  j["type"] = "CefCursorChangeEvent";
  j["id"] = m.id;
  j["browserId"] = m.browserId;
  j["cursorHandle"] = m.cursorHandle;
  j["cursorType"] = m.cursorType;
}

struct CefAddressChangeEvent {
  UUID id;
  int browserId;
  std::string url;
};

inline void to_json(json& j, const CefAddressChangeEvent& m) {
  j = json::object();
  j["type"] = "CefAddressChangeEvent";
  j["id"] = m.id;
  j["browserId"] = m.browserId;
  j["url"] = m.url;
}

struct CefTitleChangeEvent {
  UUID id;
  int browserId;
  std::string title;
};

inline void to_json(json& j, const CefTitleChangeEvent& m) {
  j = json::object();
  j["type"] = "CefTitleChangeEvent";
  j["id"] = m.id;
  j["browserId"] = m.browserId;
  j["title"] = m.title;
}

struct CefConsoleMessageEvent {
  UUID id;
  int browserId;
  int level;
  std::string message;
  std::string source;
  int line;
};

inline void to_json(json& j, const CefConsoleMessageEvent& m) {
  j = json::object();
  j["type"] = "CefConsoleMessageEvent";
  j["id"] = m.id;
  j["browserId"] = m.browserId;
  j["level"] = m.level;
  j["message"] = m.message;
  j["source"] = m.source;
  j["line"] = m.line;
}

struct CefLoadingProgressChangeEvent {
  UUID id;
  int browserId;
  double progress;
};

inline void to_json(json& j, const CefLoadingProgressChangeEvent& m) {
  j = json::object();
  j["type"] = "CefLoadingProgressChangeEvent";
  j["id"] = m.id;
  j["browserId"] = m.browserId;
  j["progress"] = m.progress;
}

struct CefAcknowledgement {
  UUID id;
};

inline void from_json(const json& j, CefAcknowledgement& m) {
  j.at("id").get_to(m.id);
}
