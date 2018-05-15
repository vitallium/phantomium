// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/task_runner_util.h"
#include "base/task_scheduler/post_task.h"
#include "phantomium/app/phantomium.h"

#if defined(OS_WIN)
#include "components/crash/content/app/crash_switches.h"
#include "components/crash/content/app/run_as_crashpad_handler_win.h"
#include "sandbox/win/src/sandbox_types.h"
#endif

namespace phantomium {

Phantomium::Phantomium()
    : browser_(nullptr),
      page_(nullptr),
      weak_factory_(this) {}

Phantomium::~Phantomium() = default;

#if !defined(CHROME_MULTIPLE_DLL_CHILD)
void Phantomium::OnStart(headless::HeadlessBrowser* browser) {
  browser_ = browser;

  headless::HeadlessBrowserContext::Builder context_builder =
      browser_->CreateBrowserContextBuilder();
  context_builder.SetIncognitoMode(true);

  browser_context_ = context_builder.Build();
  browser->SetDefaultBrowserContext(browser_context_);

  // Get the URL from the command line.
  base::CommandLine::StringVector args =
      base::CommandLine::ForCurrentProcess()->GetArgs();
  if (args.empty()) {
    LOG(ERROR) << "No URL to load";
    Shutdown();
    return;
  }
  GURL url(args[0]);
  const base::FilePath file_name(args[1]);

  page_ = CreatePage();
  page_->SetBrowserContext(browser_context_);
  page_->AddObserver(this);
  page_->Load(url, file_name);
}
#endif

void Phantomium::Shutdown() {
  browser_->Shutdown();
}

void Phantomium::OnPhantomiumPageDestruct() {
  base::AutoLock lock(lock_);
  browser_context_->Close();
  Shutdown();
}

std::unique_ptr<PhantomiumPage> Phantomium::CreatePage() {
  return base::WrapUnique(new PhantomiumPage());
}

}  // namespace phantomium
