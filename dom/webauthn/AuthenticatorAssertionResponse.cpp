/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "mozilla/dom/WebAuthenticationBinding.h"
#include "mozilla/dom/AuthenticatorAssertionResponse.h"
#include "mozilla/HoldDropJSObjects.h"

namespace mozilla::dom {

NS_IMPL_CYCLE_COLLECTION_CLASS(AuthenticatorAssertionResponse)
NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(AuthenticatorAssertionResponse,
                                                AuthenticatorResponse)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN_INHERITED(AuthenticatorAssertionResponse,
                                               AuthenticatorResponse)
  NS_IMPL_CYCLE_COLLECTION_TRACE_PRESERVED_WRAPPER
NS_IMPL_CYCLE_COLLECTION_TRACE_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(
    AuthenticatorAssertionResponse, AuthenticatorResponse)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_ADDREF_INHERITED(AuthenticatorAssertionResponse, AuthenticatorResponse)
NS_IMPL_RELEASE_INHERITED(AuthenticatorAssertionResponse, AuthenticatorResponse)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(AuthenticatorAssertionResponse)
NS_INTERFACE_MAP_END_INHERITING(AuthenticatorResponse)

AuthenticatorAssertionResponse::AuthenticatorAssertionResponse(
    nsPIDOMWindowInner* aParent)
    : AuthenticatorResponse(aParent) {}

JSObject* AuthenticatorAssertionResponse::WrapObject(
    JSContext* aCx, JS::Handle<JSObject*> aGivenProto) {
  return AuthenticatorAssertionResponse_Binding::Wrap(aCx, this, aGivenProto);
}

void AuthenticatorAssertionResponse::GetAuthenticatorData(
    JSContext* aCx, JS::MutableHandle<JSObject*> aValue, ErrorResult& aRv) {
  JSObject* value = mAuthenticatorData.ToArrayBuffer(aCx);
  if (!value) {
    aRv.NoteJSContextException(aCx);
    return;
  }
  aValue.set(value);
}

nsresult AuthenticatorAssertionResponse::SetAuthenticatorData(
    CryptoBuffer& aBuffer) {
  if (NS_WARN_IF(!mAuthenticatorData.Assign(aBuffer))) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  return NS_OK;
}

void AuthenticatorAssertionResponse::GetSignature(
    JSContext* aCx, JS::MutableHandle<JSObject*> aValue, ErrorResult& aRv) {
  JSObject* value = mSignature.ToArrayBuffer(aCx);
  if (!value) {
    aRv.NoteJSContextException(aCx);
    return;
  }
  aValue.set(value);
}

nsresult AuthenticatorAssertionResponse::SetSignature(CryptoBuffer& aBuffer) {
  if (NS_WARN_IF(!mSignature.Assign(aBuffer))) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  return NS_OK;
}

void AuthenticatorAssertionResponse::GetUserHandle(
    JSContext* aCx, JS::MutableHandle<JSObject*> aValue, ErrorResult& aRv) {
  // Per
  // https://w3c.github.io/webauthn/#ref-for-dom-authenticatorassertionresponse-userhandle%E2%91%A0
  // this should return null if the handle is unset.
  if (mUserHandle.IsEmpty()) {
    aValue.set(nullptr);
  } else {
    JSObject* value = mUserHandle.ToArrayBuffer(aCx);
    if (!value) {
      aRv.NoteJSContextException(aCx);
      return;
    }
    aValue.set(value);
  }
}

nsresult AuthenticatorAssertionResponse::SetUserHandle(CryptoBuffer& aBuffer) {
  if (NS_WARN_IF(!mUserHandle.Assign(aBuffer))) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  return NS_OK;
}

}  // namespace mozilla::dom
