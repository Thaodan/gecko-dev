/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef nskdeshellservice_h____
#define nskdeshellservice_h____

#include "nsIGNOMEShellService.h"
#include "nsToolkitShellService.h"
#include "nsString.h"
#include "mozilla/Attributes.h"

class nsKDEShellService final : public nsIGNOMEShellService,
                                public nsToolkitShellService
{
public:
  nsKDEShellService() : mCheckedThisSession(false) { }

  NS_DECL_ISUPPORTS
  NS_DECL_NSISHELLSERVICE
  NS_DECL_NSIGNOMESHELLSERVICE

  nsresult Init();

private:
  ~nsKDEShellService() {}

  bool mCheckedThisSession;
};

#endif // nskdeshellservice_h____
