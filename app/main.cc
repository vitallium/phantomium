// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "build/build_config.h"

#include "base/bind.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "content/public/app/content_main.h"
#include "headless/public/headless_browser.h"
#include "net/base/filename_util.h"
#include "phantomium/app/phantomium.h"
#include "phantomium/app/phantomium_switches.h"
#include "ui/gfx/geometry/size.h"

#if defined(OS_WIN)
#include "content/public/app/sandbox_helper_win.h"
#include "sandbox/win/src/sandbox_types.h"
#endif

bool ParseWindowSize(const std::string& window_size,
                     gfx::Size* parsed_window_size) {
  int width, height = 0;
  if (sscanf(window_size.c_str(), "%d%*[x,]%d", &width, &height) >= 2 &&
      width >= 0 && height >= 0) {
    parsed_window_size->set_width(width);
    parsed_window_size->set_height(height);
    return true;
  }
  return false;
}

#if defined(OS_WIN)
int PhantomiumMain(HINSTANCE instance,
                      sandbox::SandboxInterfaceInfo* sandbox_info) {
  base::CommandLine::Init(0, nullptr);

  headless::RunChildProcessIfNeeded(instance, sandbox_info);
  headless::HeadlessBrowser::Options::Builder builder(0, nullptr);
  builder.SetInstance(instance);
  builder.SetSandboxInfo(std::move(sandbox_info));
#else
int PhantomiumMain(int argc, const char** argv) {
  base::CommandLine::Init(argc, argv);
  headless::RunChildProcessIfNeeded(argc, argv);
  headless::HeadlessBrowser::Options::Builder builder(argc, argv);
#endif  // defined(OS_WIN)
  phantomium::Phantomium phantomium;
  base::CommandLine& command_line(*base::CommandLine::ForCurrentProcess());

  // command-lind options
  if (command_line.HasSwitch(
          phantomium::switches::kWindowSize)) {
    std::string window_size =
        command_line.GetSwitchValueASCII(phantomium::switches::kWindowSize);
    gfx::Size parsed_window_size;
    if (!ParseWindowSize(window_size, &parsed_window_size)) {
      LOG(ERROR) << "Malformed window size";
      return EXIT_FAILURE;
    }
    builder.SetWindowSize(parsed_window_size);
  } else {
    builder.SetWindowSize(gfx::Size(800, 600));
  }

  return HeadlessBrowserMain(
    builder.Build(),
    base::BindOnce(
      &phantomium::Phantomium::OnStart, base::Unretained(&phantomium)));
}

int PhantomiumMain(const content::ContentMainParams& params) {
#if defined(OS_WIN)
  return PhantomiumMain(params.instance, params.sandbox_info);
#else
  return PhantomiumMain(params.argc, params.argv);
#endif
}

int main(int argc, const char** argv) {
#if defined(OS_WIN)
  sandbox::SandboxInterfaceInfo sandbox_info = {0};
  content::InitializeSandboxInfo(&sandbox_info);
  return PhantomiumMain(0, &sandbox_info);
#else
  return PhantomiumMain(argc, argv);
#endif  // defined(OS_WIN)
}
