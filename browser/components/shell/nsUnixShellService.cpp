/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */


#include "nsUnixShellService.h"
#include "nsGNOMEShellService.h"
#include "nsKDEShellService.h"
#include "nsKDEUtils.h"
#include "mozilla/ModuleUtils.h"

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsGNOMEShellService, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsKDEShellService, Init)

NS_IMETHODIMP
nsUnixShellServiceConstructor(REFNSIID aIID, void **aResult)
{
    if( nsKDEUtils::kdeSupport())
        return nsKDEShellServiceConstructor( aIID, aResult );
    return nsGNOMEShellServiceConstructor( aIID, aResult );
}
