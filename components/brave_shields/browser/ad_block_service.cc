/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/ad_block_service.h"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include "base/base_paths.h"
#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/callback.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_restrictions.h"
#include "brave/components/brave_shields/browser/dat_file_util.h"
#include "brave/vendor/ad-block/ad_block_client.h"

#define DAT_FILE "ABPFilterParserData.dat"
// TODO: Repalce this with the real version at runtime
#define DAT_FILE_URL "https://s3.amazonaws.com/adblock-data/3/ABPFilterParserData.dat"

namespace brave_shields {

GURL AdBlockService::g_ad_block_url(DAT_FILE_URL);

AdBlockService::AdBlockService() :
    BaseBraveShieldsService(DAT_FILE, AdBlockService::g_ad_block_url),
    ad_block_client_(new AdBlockClient()) {
}

AdBlockService::~AdBlockService() {
  Cleanup();
}

void AdBlockService::Cleanup() {
  ad_block_client_.reset();
}

bool AdBlockService::ShouldStartRequest(const GURL& url,
    content::ResourceType resource_type,
    const std::string& tab_host) {

  FilterOption current_option = FONoFilterOption;
  content::ResourceType internalResource = (content::ResourceType)resource_type;
  if (content::RESOURCE_TYPE_STYLESHEET == internalResource) {
    current_option = FOStylesheet;
  } else if (content::RESOURCE_TYPE_IMAGE == internalResource) {
    current_option = FOImage;
  } else if (content::RESOURCE_TYPE_SCRIPT == internalResource) {
    current_option = FOScript;
  }

  if (ad_block_client_->matches(url.spec().c_str(),
        current_option,
        tab_host.c_str())) {
    // LOG(ERROR) << "AdBlockService::Check(), host: " << tab_host
    //  << ", resource type: " << resource_type
    //  << ", url.spec(): " << url.spec();
    return false;
  }

  return true;
}

bool AdBlockService::Init() {
   if (!GetDATFileData(DAT_FILE, buffer_)) {
    LOG(ERROR) << "Could not obtain ad block data file";
    return false;
  }
  if (!ad_block_client_->deserialize((char*)&buffer_.front())) {
    ad_block_client_.reset();
    LOG(ERROR) << "AdBlockService::InitAdBlock deserialize failed";
    return false;
  }
  return true;
}

// static
void AdBlockService::SetAdBlockURLForTest(const GURL& url) {
  g_ad_block_url = url;
}

///////////////////////////////////////////////////////////////////////////////

// The brave shields factory. Using the Brave Shields as a singleton
// is the job of the browser process.
std::unique_ptr<BaseBraveShieldsService> AdBlockServiceFactory() {
  return base::MakeUnique<AdBlockService>();
}

}  // namespace brave_shields
