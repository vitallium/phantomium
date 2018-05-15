// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/base64.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/task_runner_util.h"
#include "base/task_scheduler/post_task.h"
#include "content/public/browser/browser_thread.h"
#include "net/base/filename_util.h"
#include "net/base/io_buffer.h"
#include "phantomium/lib/phantomium_page.h"

namespace phantomium {

PhantomiumPage::PhantomiumPage()
    : processed_page_ready_(false),
#if !defined(CHROME_MULTIPLE_DLL_CHILD)
      browser_context_(nullptr),
      web_contents_(nullptr),
#endif
      devtools_client_(headless::HeadlessDevToolsClient::Create()),
      weak_factory_(this) {}

PhantomiumPage::~PhantomiumPage() = default;

void PhantomiumPage::Load(const GURL& url, const base::FilePath& file_name) {
  file_name_ = file_name;


  headless::HeadlessWebContents::Builder builder(
      browser_context_->CreateWebContentsBuilder());
  builder.SetInitialURL(url);

  web_contents_ = builder.Build();
  web_contents_->AddObserver(this);

  file_task_runner_ = base::CreateSequencedTaskRunnerWithTraits(
      {base::MayBlock(), base::TaskPriority::BACKGROUND});
}

void PhantomiumPage::OnTargetCrashed(
    const headless::inspector::TargetCrashedParams& params) {
  LOG(ERROR) << "Abnormal renderer termination.";
}

void PhantomiumPage::SetBrowserContext(
    headless::HeadlessBrowserContext* browser_context) {
  DCHECK(!browser_context_);
  browser_context_ = browser_context;
}

// This method is called when the tab is ready for DevTools inspection.
void PhantomiumPage::DevToolsTargetReady() {
  // Attach our DevTools client to the tab so that we can send commands to it
  // and observe events.
  web_contents_->GetDevToolsTarget()->AttachClient(devtools_client_.get());

  // Start observing events from DevTools's page domain. This lets us get
  // notified when the page has finished loading. Note that it is possible
  // the page has already finished loading by now.
  devtools_client_->GetPage()->AddObserver(this);
  devtools_client_->GetPage()->Enable();
}

void PhantomiumPage::OnLoadEventFired(
    const headless::page::LoadEventFiredParams& params) {
  if (processed_page_ready_)
    return;
  processed_page_ready_ = true;
  PrintToPDF();
}

void PhantomiumPage::Shutdown() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  devtools_client_->GetInspector()->GetExperimental()->RemoveObserver(this);
  devtools_client_->GetPage()->RemoveObserver(this);
  if (web_contents_->GetDevToolsTarget()) {
    web_contents_->GetDevToolsTarget()->DetachClient(devtools_client_.get());
  }

  web_contents_->RemoveObserver(this);
  web_contents_ = nullptr;

  browser_context_ = nullptr;

  // Inform observers that we're going away.
  {
    base::AutoLock lock(observers_lock_);
    for (auto& observer : observers_)
      observer.OnPhantomiumPageDestruct();
  }
}

void PhantomiumPage::PrintToPDF() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  devtools_client_->GetPage()->GetExperimental()->PrintToPDF(
      headless::page::PrintToPDFParams::Builder()
          .SetDisplayHeaderFooter(true)
          .SetPrintBackground(true)
          .SetPreferCSSPageSize(true)
          .Build(),
      base::BindOnce(&PhantomiumPage::OnPDFCreated,
                     weak_factory_.GetWeakPtr()));
}

void PhantomiumPage::OnPDFCreated(
    std::unique_ptr<headless::page::PrintToPDFResult> result) {
  if (!result) {
    LOG(ERROR) << "Print to PDF failed";
    Shutdown();
    return;
  }

  WriteFile(result->GetData());
}

void PhantomiumPage::WriteFile(const std::string& base64_data) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  // TODO(vitallium): If empty print to STDOUT
  if (file_name_.empty()) {
    LOG(ERROR) << "Empty filename";
    Shutdown();
    return;
  }

  std::string decoded_data;
  if (!base::Base64Decode(base64_data, &decoded_data)) {
    LOG(ERROR) << "Failed to decode base64 data";
    OnFileOpened(std::string(), file_name_, base::File::FILE_ERROR_FAILED);
    return;
  }

  file_proxy_ = std::make_unique<base::FileProxy>(file_task_runner_.get());
  if (!file_proxy_->CreateOrOpen(
          file_name_, base::File::FLAG_CREATE_ALWAYS | base::File::FLAG_WRITE,
          base::BindOnce(&PhantomiumPage::OnFileOpened,
                         weak_factory_.GetWeakPtr(), decoded_data,
                         file_name_))) {
    // Operation could not be started.
    OnFileOpened(std::string(), file_name_, base::File::FILE_ERROR_FAILED);
  }
}

void PhantomiumPage::OnFileOpened(const std::string& decoded_data,
                                  const base::FilePath file_name,
                                  base::File::Error error_code) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (!file_proxy_->IsValid()) {
    LOG(ERROR) << "Writing to file " << file_name.value()
               << " was unsuccessful, could not open file: "
               << base::File::ErrorToString(error_code);
    return;
  }

  auto buf = base::MakeRefCounted<net::IOBufferWithSize>(decoded_data.size());
  memcpy(buf->data(), decoded_data.data(), decoded_data.size());

  if (!file_proxy_->Write(
          0, buf->data(), buf->size(),
          base::BindOnce(&PhantomiumPage::OnFileWritten,
                         weak_factory_.GetWeakPtr(), file_name, buf->size()))) {
    // Operation may have completed successfully or failed.
    OnFileWritten(file_name, buf->size(), base::File::FILE_ERROR_FAILED, 0);
  }
}

void PhantomiumPage::OnFileWritten(const base::FilePath file_name,
                                   const size_t length,
                                   base::File::Error error_code,
                                   int write_result) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (write_result < static_cast<int>(length)) {
    // TODO(eseckler): Support recovering from partial writes.
    LOG(ERROR) << "Writing to file " << file_name.value()
               << " was unsuccessful: "
               << base::File::ErrorToString(error_code);
  } else {
    LOG(INFO) << "Written to file " << file_name.value() << ".";
  }
  if (!file_proxy_->Close(base::BindOnce(&PhantomiumPage::OnFileClosed,
                                         weak_factory_.GetWeakPtr()))) {
    // Operation could not be started.
    OnFileClosed(base::File::FILE_ERROR_FAILED);
  }
}

void PhantomiumPage::OnFileClosed(base::File::Error error_code) {
  Shutdown();
}

void PhantomiumPage::AddObserver(Observer* obs) {
  base::AutoLock lock(observers_lock_);
  observers_.AddObserver(obs);
}

void PhantomiumPage::RemoveObserver(Observer* obs) {
  base::AutoLock lock(observers_lock_);
  observers_.RemoveObserver(obs);
}

}  // namespace phantomium
