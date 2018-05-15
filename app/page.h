// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PHANTOMIUM_PAGE_H_
#define PHANTOMIUM_PAGE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/files/file_proxy.h"
#include "base/memory/weak_ptr.h"
#include "base/sequenced_task_runner.h"
#include "headless/app/shell_navigation_request.h"
#include "headless/public/devtools/domains/emulation.h"
#include "headless/public/devtools/domains/inspector.h"
#include "headless/public/devtools/domains/page.h"
#include "headless/public/devtools/domains/runtime.h"
#include "headless/public/headless_browser.h"
#include "headless/public/headless_devtools_client.h"
#include "headless/public/headless_web_contents.h"
#include "headless/public/util/deterministic_dispatcher.h"
#include "net/base/file_stream.h"

class GURL;

namespace phantomium {

// An application which implements a simple headless browser.
class PhantomiumPage : public headless::HeadlessWebContents::Observer,
                       public headless::inspector::ExperimentalObserver,
                       public headless::page::ExperimentalObserver {
 public:
  PhantomiumPage();
  ~PhantomiumPage() override;

  virtual void OnStart(HeadlessBrowser* browser);

  HeadlessDevToolsClient* devtools_client() const {
    return devtools_client_.get();
  }

 private:
  // HeadlessWebContents::Observer implementation:
  void DevToolsTargetReady() override;
  void OnTargetCrashed(const inspector::TargetCrashedParams& params) override;

  // page::Observer implementation:
  void OnLoadEventFired(const page::LoadEventFiredParams& params) override;

  // network::Observer implementation:
  void OnRequestIntercepted(
      const network::RequestInterceptedParams& params) override;

  virtual void Shutdown();

  void FetchTimeout();

  void PollReadyState();

  void OnReadyState(std::unique_ptr<runtime::EvaluateResult> result);

  void OnPageReady();

  void PrintToPDF();

  void OnPDFCreated(std::unique_ptr<page::PrintToPDFResult> result);

  void WriteFile(const std::string& switch_string,
                 const std::string& default_file_name,
                 const std::string& base64_data);
  void OnFileOpened(const std::string& decoded_data,
                    const base::FilePath file_name,
                    base::File::Error error_code);
  void OnFileWritten(const base::FilePath file_name,
                     const size_t length,
                     base::File::Error error_code,
                     int write_result);
  void OnFileClosed(base::File::Error error_code);

  bool RemoteDebuggingEnabled() const;

  GURL url_;
  HeadlessBrowser* browser_;  // Not owned.
  std::unique_ptr<HeadlessDevToolsClient> devtools_client_;
#if !defined(CHROME_MULTIPLE_DLL_CHILD)
  HeadlessWebContents* web_contents_;
  HeadlessBrowserContext* browser_context_;
#endif
  bool processed_page_ready_;
  scoped_refptr<base::SequencedTaskRunner> file_task_runner_;
  std::unique_ptr<base::FileProxy> file_proxy_;
  std::unique_ptr<DeterministicDispatcher> deterministic_dispatcher_;
  base::WeakPtrFactory<HeadlessShell> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(HeadlessShell);
};

}  // namespace phantomium

#endif  // PHANTOMIUM_PAGE_H_
