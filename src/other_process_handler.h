// Copyright (c) 2015 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#pragma once

#include "process_handler.h"

// Client app implementation for other process types.
class OtherProcessHandler : public ProcessHandler {
 public:
  OtherProcessHandler();

 private:
  IMPLEMENT_REFCOUNTING(OtherProcessHandler);
  DISALLOW_COPY_AND_ASSIGN(OtherProcessHandler);
};