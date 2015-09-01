/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_layers_EmbedLiteCompositorParent_h
#define mozilla_layers_EmbedLiteCompositorParent_h

#define COMPOSITOR_PERFORMANCE_WARNING

#include "Layers.h"
#include "mozilla/WidgetUtils.h"
#include "mozilla/layers/CompositorChild.h"
#include "mozilla/layers/CompositorParent.h"

namespace mozilla {

namespace layers {
class LayerManagerComposite;
}

namespace embedlite {

class EmbedLiteCompositorParent : public mozilla::layers::CompositorParent
{
public:
  EmbedLiteCompositorParent(nsIWidget* widget, uint32_t windowId,
		            bool aRenderToEGLSurface,
                            int aSurfaceWidth, int aSurfaceHeight);

  bool RenderToContext(gfx::DrawTarget* aTarget);
  void SetSurfaceSize(int width, int height);
  void SetScreenRotation(const mozilla::ScreenRotation& rotation);
  void* GetPlatformImage(int* width, int* height);
  void SuspendRendering();
  void ResumeRendering();

protected:
  virtual ~EmbedLiteCompositorParent();
  virtual PLayerTransactionParent*
  AllocPLayerTransactionParent(const nsTArray<LayersBackend>& aBackendHints,
                               const uint64_t& aId,
                               TextureFactoryIdentifier* aTextureFactoryIdentifier,
                               bool* aSuccess) override;
  virtual void ScheduleTask(CancelableTask*, int) override;
  void PrepareOffscreen();

private:
  bool Invalidate();
  void UpdateTransformState();
  bool RenderGL(TimeStamp aScheduleTime);

  uint32_t mWindowId;
  gfx::Matrix mWorldTransform;
  mozilla::ScreenRotation mRotation;
  bool mUseScreenRotation;
  CancelableTask* mCurrentCompositeTask;
  gfx::IntSize mLastViewSize;
};

} // embedlite
} // mozilla

#endif // mozilla_layers_EmbedLiteCompositorParent_h
