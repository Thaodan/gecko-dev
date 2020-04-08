/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "EmbedLog.h"

#include "EmbedLiteCompositorBridgeParent.h"
#include "BasicLayers.h"
#include "EmbedLiteApp.h"
#include "EmbedLiteWindow.h"
#include "EmbedLiteWindowBaseParent.h"
#include "mozilla/layers/LayerManagerComposite.h"
#include "mozilla/layers/AsyncCompositionManager.h"
#include "mozilla/layers/LayerTransactionParent.h"
#include "mozilla/layers/CompositorOGL.h"
#include "mozilla/layers/TextureClientSharedSurface.h" // for SharedSurfaceTextureClient
#include "mozilla/Preferences.h"
#include "gfxUtils.h"
#include "nsRefreshDriver.h"

#include "math.h"

#include "GLContext.h"                  // for GLContext
#include "GLScreenBuffer.h"             // for GLScreenBuffer
#include "SharedSurfaceEGL.h"           // for SurfaceFactory_EGLImage
#include "SharedSurfaceGL.h"            // for SurfaceFactory_GLTexture, etc
#include "SurfaceTypes.h"               // for SurfaceStreamType
#include "ClientLayerManager.h"         // for ClientLayerManager, etc

using namespace mozilla::layers;
using namespace mozilla::gfx;
using namespace mozilla::gl;

namespace mozilla {
namespace embedlite {

static const int sDefaultPaintInterval = nsRefreshDriver::DefaultInterval();

EmbedLiteCompositorBridgeParent::EmbedLiteCompositorBridgeParent(nsIWidget* widget,
                                                     uint32_t windowId,
                                                     bool aRenderToEGLSurface,
                                                     int aSurfaceWidth,
                                                     int aSurfaceHeight)
#if 0
  : CompositorParent(widget, aRenderToEGLSurface, aSurfaceWidth, aSurfaceHeight)
  , mWindowId(windowId)
  , mCurrentCompositeTask(nullptr)
  , mSurfaceSize(aSurfaceWidth, aSurfaceHeight)
  , mRenderMutex("EmbedLiteCompositorBridgeParent render mutex")
{
  EmbedLiteWindowBaseParent* parentWindow = EmbedLiteWindowBaseParent::From(mWindowId);
  LOGT("this:%p, window:%p, sz[%i,%i]", this, parentWindow, aSurfaceWidth, aSurfaceHeight);
  Preferences::AddBoolVarCache(&mUseExternalGLContext,
                               "embedlite.compositor.external_gl_context", false);
  parentWindow->SetCompositor(this);
}
#else
{}
#endif

EmbedLiteCompositorBridgeParent::~EmbedLiteCompositorBridgeParent()
{
  LOGT();
}

PLayerTransactionParent*
EmbedLiteCompositorBridgeParent::AllocPLayerTransactionParent(const nsTArray<LayersBackend>& aBackendHints,
                                                        const uint64_t& aId,
                                                        TextureFactoryIdentifier* aTextureFactoryIdentifier,
                                                        bool* aSuccess)
{
  PLayerTransactionParent* p =
    CompositorBridgeParent::AllocPLayerTransactionParent(aBackendHints,
                                                   aId,
                                                   aTextureFactoryIdentifier,
                                                   aSuccess);

  EmbedLiteWindow* win = EmbedLiteApp::GetInstance()->GetWindowByID(mWindowId);
  if (win) {
    win->GetListener()->CompositorCreated();
  }

  if (!mUseExternalGLContext) {
    // Prepare Offscreen rendering context
    PrepareOffscreen();
  }
  return p;
}

bool EmbedLiteCompositorBridgeParent::DeallocPLayerTransactionParent(PLayerTransactionParent *aLayers)
{
    bool deallocated = CompositorBridgeParent::DeallocPLayerTransactionParent(aLayers);
    LOGT();
    return deallocated;
}

void
EmbedLiteCompositorBridgeParent::PrepareOffscreen()
{

  const CompositorBridgeParent::LayerTreeState* state = CompositorBridgeParent::GetIndirectShadowTree(RootLayerTreeId());
  NS_ENSURE_TRUE(state && state->mLayerManager, );

  GLContext* context = static_cast<CompositorOGL*>(state->mLayerManager->GetCompositor())->gl();
  NS_ENSURE_TRUE(context, );

  if (context->IsOffscreen()) {
    GLScreenBuffer* screen = context->Screen();
    if (screen) {
      UniquePtr<SurfaceFactory> factory;

      layers::TextureFlags flags = layers::TextureFlags::ORIGIN_BOTTOM_LEFT;
      if (!screen->mCaps.premultAlpha) {
          flags |= layers::TextureFlags::NON_PREMULTIPLIED;
      }

      auto forwarder = state->mLayerManager->AsShadowForwarder();
      printf("=============== caps.premultAlpha: %d ptr: %p\n", screen->mCaps.premultAlpha, forwarder);
      if (context->GetContextType() == GLContextType::EGL) {
        // [Basic/OGL Layers, OMTC] WebGL layer init.
        factory = SurfaceFactory_EGLImage::Create(context, screen->mCaps, forwarder, flags);
      } else {
        // [Basic Layers, OMTC] WebGL layer init.
        // Well, this *should* work...
        GLContext* nullConsGL = nullptr; // Bug 1050044.
        factory = MakeUnique<SurfaceFactory_GLTexture>(context, screen->mCaps, forwarder, flags);
      }
      if (factory) {
        screen->Morph(Move(factory));
      }
    }
  }
}

void
EmbedLiteCompositorBridgeParent::UpdateTransformState()
{
  const CompositorBridgeParent::LayerTreeState* state = CompositorBridgeParent::GetIndirectShadowTree(RootLayerTreeId());
  NS_ENSURE_TRUE(state && state->mLayerManager, );

  CompositorOGL *compositor = static_cast<CompositorOGL*>(state->mLayerManager->GetCompositor());
  NS_ENSURE_TRUE(compositor, );

  GLContext* context = compositor->gl();
  NS_ENSURE_TRUE(context, );

  if (context->IsOffscreen() && context->OffscreenSize() != mSurfaceSize && context->ResizeOffscreen(mSurfaceSize)) {
    ScheduleRenderOnCompositorThread();
  }
}

void
EmbedLiteCompositorBridgeParent::ScheduleTask(CancelableTask* task, int time)
{
#if 0
  if (Invalidate()) {
    task->Cancel();
    CancelCurrentCompositeTask();
  } else {
    CompositorParent::ScheduleTask(task, time);
  }
#endif
}

bool
EmbedLiteCompositorBridgeParent::Invalidate()
{
  if (!mUseExternalGLContext) {
    UpdateTransformState();
    mCurrentCompositeTask = NewRunnableMethod(this, &EmbedLiteCompositorBridgeParent::RenderGL, TimeStamp::Now());
    MessageLoop::current()->PostDelayedTask(FROM_HERE, mCurrentCompositeTask, sDefaultPaintInterval);
    return true;
  }

  return false;
}

bool EmbedLiteCompositorBridgeParent::RenderGL(TimeStamp aScheduleTime)
{
//  mLastCompose = aScheduleTime;
  if (mCurrentCompositeTask) {
    mCurrentCompositeTask->Cancel();
    mCurrentCompositeTask = nullptr;
  }

  const CompositorParent::LayerTreeState* state = CompositorParent::GetIndirectShadowTree(RootLayerTreeId());
  NS_ENSURE_TRUE(state && state->mLayerManager, false);

  GLContext* context = static_cast<CompositorOGL*>(state->mLayerManager->GetCompositor())->gl();
  NS_ENSURE_TRUE(context, false);
  if (!context->IsCurrent()) {
    context->MakeCurrent(true);
  }
  NS_ENSURE_TRUE(context->IsCurrent(), false);

  {
    ScopedScissorRect autoScissor(context);
    GLenum oldTexUnit;
    context->GetUIntegerv(LOCAL_GL_ACTIVE_TEXTURE, &oldTexUnit);
    CompositeToTarget(nullptr);
    context->fActiveTexture(oldTexUnit);
  }

  if (context->IsOffscreen()) {
    // RenderGL is called always from Gecko compositor thread.
    // GLScreenBuffer::PublishFrame does swap buffers and that
    // cannot happen while reading previous frame on EmbedLiteCompositorBridgeParent::GetPlatformImage
    // (potentially from another thread).
    MutexAutoLock lock(mRenderMutex);
    GLScreenBuffer* screen = context->Screen();
    MOZ_ASSERT(screen);

    if (screen->Size().IsEmpty() || !screen->PublishFrame(screen->Size())) {
      NS_ERROR("Failed to publish context frame");
      return false;
    }
    // Temporary hack, we need two extra paints in order to get initial picture
    static int sInitialPaintCount = 0;
    if (sInitialPaintCount < 2) {
      ScheduleRenderOnCompositorThread();
      sInitialPaintCount++;
    }
  }

  return false;
}

void EmbedLiteCompositorBridgeParent::SetSurfaceSize(int width, int height)
{
  if (width > 0 && height > 0 && (mSurfaceSize.width != width || mSurfaceSize.height != height)) {
    SetEGLSurfaceSize(width, height);
    mSurfaceSize = gfx::IntSize(width, height);
  }
}

void*
EmbedLiteCompositorBridgeParent::GetPlatformImage(int* width, int* height)
{
  MutexAutoLock lock(mRenderMutex);
  const CompositorParent::LayerTreeState* state = CompositorParent::GetIndirectShadowTree(RootLayerTreeId());
  NS_ENSURE_TRUE(state && state->mLayerManager, nullptr);

  GLContext* context = static_cast<CompositorOGL*>(state->mLayerManager->GetCompositor())->gl();
  NS_ENSURE_TRUE(context, nullptr);
  NS_ENSURE_TRUE(context->IsOffscreen(), nullptr);

  GLScreenBuffer* screen = context->Screen();
  MOZ_ASSERT(screen);
  NS_ENSURE_TRUE(screen->Front(), nullptr);
  SharedSurface* sharedSurf = screen->Front()->Surf();
  NS_ENSURE_TRUE(sharedSurf, nullptr);
  sharedSurf->WaitSync();

  *width = sharedSurf->mSize.width;
  *height = sharedSurf->mSize.height;

  if (sharedSurf->mType == SharedSurfaceType::EGLImageShare) {
    SharedSurface_EGLImage* eglImageSurf = SharedSurface_EGLImage::Cast(sharedSurf);
    return eglImageSurf->mImage;
  }

  return nullptr;
}

void
EmbedLiteCompositorBridgeParent::SuspendRendering()
{
  CompositorParent::SchedulePauseOnCompositorThread();
}

void
EmbedLiteCompositorBridgeParent::ResumeRendering()
{
  if (mSurfaceSize.width > 0 && mSurfaceSize.height > 0) {
    CompositorParent::ScheduleResumeOnCompositorThread(mSurfaceSize.width,
                                                       mSurfaceSize.height);
    CompositorParent::ScheduleRenderOnCompositorThread();
  }
}

} // namespace embedlite
} // namespace mozilla

