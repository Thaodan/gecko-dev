/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "mozilla/ArrayUtils.h"

#include "nsCOMPtr.h"
#include "nsKDEShellService.h"
#include "nsShellService.h"
#include "nsKDEUtils.h"
#include "nsIPrefService.h"
#include "nsIProcess.h"
#include "nsIFile.h"
#include "nsServiceManagerUtils.h"
#include "nsComponentManagerUtils.h"
#include "nsIMutableArray.h"
#include "nsISupportsPrimitives.h"
#include "nsArrayUtils.h"

using namespace mozilla;

nsresult
nsKDEShellService::Init()
{
    if( !nsKDEUtils::kdeSupport())
        return NS_ERROR_NOT_AVAILABLE;
    return NS_OK;
}

NS_IMPL_ISUPPORTS(nsKDEShellService, nsIGNOMEShellService, nsIShellService)

NS_IMETHODIMP
nsKDEShellService::IsDefaultBrowser(bool aForAllTypes,
                                    bool* aIsDefaultBrowser)
{
    *aIsDefaultBrowser = false;

    nsCOMPtr<nsIMutableArray> command = do_CreateInstance( NS_ARRAY_CONTRACTID );
    if (!command)
        return NS_ERROR_FAILURE;

    nsCOMPtr<nsISupportsCString> str = do_CreateInstance( NS_SUPPORTS_CSTRING_CONTRACTID );
    if (!str)
        return NS_ERROR_FAILURE;

    str->SetData("ISDEFAULTBROWSER"_ns);
    command->AppendElement( str );

    if( nsKDEUtils::command( command ))
        *aIsDefaultBrowser = true;
    return NS_OK;
}

NS_IMETHODIMP
nsKDEShellService::SetDefaultBrowser(bool aForAllUsers)
{
    nsCOMPtr<nsIMutableArray> command = do_CreateInstance( NS_ARRAY_CONTRACTID );
    if (!command)
        return NS_ERROR_FAILURE;

    nsCOMPtr<nsISupportsCString> cmdstr = do_CreateInstance( NS_SUPPORTS_CSTRING_CONTRACTID );
    nsCOMPtr<nsISupportsCString> paramstr = do_CreateInstance( NS_SUPPORTS_CSTRING_CONTRACTID );
    if (!cmdstr || !paramstr)
        return NS_ERROR_FAILURE;

    cmdstr->SetData("SETDEFAULTBROWSER"_ns);
    command->AppendElement( cmdstr );

    paramstr->SetData("ALLTYPES"_ns);
    command->AppendElement( paramstr );

    return nsKDEUtils::command( command ) ? NS_OK : NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsKDEShellService::GetCanSetDesktopBackground(bool* aResult)
{
  *aResult = true;
  return NS_OK;
}

NS_IMETHODIMP
nsKDEShellService::SetDesktopBackground(dom::Element* aElement,
                                        int32_t aPosition,
                                        const nsACString& aImageName)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsKDEShellService::GetDesktopBackgroundColor(PRUint32 *aColor)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsKDEShellService::SetDesktopBackgroundColor(PRUint32 aColor)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsKDEShellService::IsDefaultForScheme(nsTSubstring<char> const& aScheme, bool* aIsDefaultBrowser)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

