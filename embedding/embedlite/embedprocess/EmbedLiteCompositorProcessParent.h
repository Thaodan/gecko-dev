/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_layers_EmbedLiteCompositorProcessParent_h
#define mozilla_layers_EmbedLiteCompositorProcessParent_h

#define COMPOSITOR_PERFORMANCE_WARNING

#include "mozilla/layers/CompositorBridgeParent.h"
#include "mozilla/layers/CompositorBridgeChild.h"
#include "Layers.h"
#include "EmbedLiteViewThreadParent.h"

namespace mozilla {
namespace embedlite {

class EmbedLiteCompositorProcessParent final : public PCompositorBridgeParent,
                                               public ShadowLayersManager
{
  friend class CompositorBridgeParent;

  NS_INLINE_DECL_THREADSAFE_REFCOUNTING_WITH_MAIN_THREAD_DESTRUCTION(EmbedLiteCompositorProcessParent)
public:
  EmbedLiteCompositorProcessParent(Transport* aTransport, ProcessId aOtherProcess, int aSurfaceWidth, int aSurfaceHeight, uint32_t id);

  // IToplevelProtocol::CloneToplevel()
  virtual IToplevelProtocol*
  CloneToplevel(const InfallibleTArray<mozilla::ipc::ProtocolFdMapping>& aFds,
                base::ProcessHandle aPeerProcess,
                mozilla::ipc::ProtocolCloneContext* aCtx) override;

  virtual void ActorDestroy(ActorDestroyReason aWhy) override;

  virtual bool RecvGetFrameUniformity(FrameUniformityData* aOutData) override { return true; }
  // FIXME/bug 774388: work out what shutdown protocol we need.
  virtual bool RecvRequestOverfill() override { return true; }
  virtual bool RecvWillStop() override { return true; }
  virtual bool RecvStop() override { return true; }
  virtual bool RecvPause() override { return true; }
  virtual bool RecvResume() override { return true; }
  virtual bool RecvNotifyHidden(const uint64_t& id) override { return true; }
  virtual bool RecvNotifyVisible(const uint64_t& id) override { return true; }
  virtual bool RecvNotifyChildCreated(const uint64_t& child) override;
  virtual bool RecvAdoptChild(const uint64_t& child) override { return false; }
  virtual bool RecvMakeSnapshot(const SurfaceDescriptor& aInSnapshot,
                                const nsIntRect& aRect) { return true; }
  virtual bool RecvMakeWidgetSnapshot(const SurfaceDescriptor& aInSnapshot) override { return true; }
  virtual bool RecvFlushRendering() override { return true; }
  virtual bool RecvGetTileSize(int32_t* aWidth, int32_t* aHeight) override;

  virtual bool RecvNotifyRegionInvalidated(const nsIntRegion& aRegion) { return true; }
  virtual bool RecvStartFrameTimeRecording(const int32_t& aBufferSize, uint32_t* aOutStartIndex) override { return true; }
  virtual bool RecvStopFrameTimeRecording(const uint32_t& aStartIndex, InfallibleTArray<float>* intervals) override  { return true; }

  /**
   * Tells this CompositorBridgeParent to send a message when the compositor has received the transaction.
   */
  virtual bool RecvRequestNotifyAfterRemotePaint() override;

  virtual PLayerTransactionParent*
    AllocPLayerTransactionParent(const nsTArray<LayersBackend>& aBackendHints,
                                 const uint64_t& aId,
                                 TextureFactoryIdentifier* aTextureFactoryIdentifier,
                                 bool *aSuccess) override;

  virtual bool DeallocPLayerTransactionParent(PLayerTransactionParent* aLayers) override;

  virtual void ShadowLayersUpdated(LayerTransactionParent* aLayerTree,
                                   const uint64_t& aTransactionId,
                                   const TargetConfig& aTargetConfig,
                                   const InfallibleTArray<PluginWindowData>& aPlugins,
                                   bool aIsFirstPaint,
                                   bool aScheduleComposite,
                                   uint32_t aPaintSequenceNumber,
                                   bool aIsRepeatTransaction,
                                   int32_t aPaintSyncId) override;
  virtual void ForceComposite(LayerTransactionParent* aLayerTree) override;
  virtual bool SetTestSampleTime(LayerTransactionParent* aLayerTree,
                                 const TimeStamp& aTime) override;
  virtual void LeaveTestMode(LayerTransactionParent* aLayerTree) override;
  virtual void ApplyAsyncProperties(LayerTransactionParent* aLayerTree) override;
  virtual void FlushApzRepaints(const LayerTransactionParent* aLayerTree) override;
  virtual void GetAPZTestData(const LayerTransactionParent* aLayerTree,
                              APZTestData* aOutData) override;
  virtual void SetConfirmedTargetAPZC(const LayerTransactionParent* aLayerTree,
                                      const uint64_t& aInputBlockId,
                                      const nsTArray<ScrollableLayerGuid>& aTargets) override;

  virtual AsyncCompositionManager* GetCompositionManager(LayerTransactionParent* aParent) override;

  virtual bool RecvRemotePluginsReady() override { return false; }

  void DidComposite(uint64_t aId);

  /**
   * A new child process has been configured to push transactions
   * directly to us.  Transport is to its thread context.
   */
  static PCompositorBridgeParent*
  Create(Transport* aTransport, ProcessId aOtherProcess, int aSurfaceWidth, int aSurfaceHeight, uint32_t id);

private:
  // Private destructor, to discourage deletion outside of Release():
  virtual ~EmbedLiteCompositorProcessParent();

  void DeferredDestroy();
  void InitializeLayerManager(const nsTArray<LayersBackend>& aBackendHints);

  // There can be many CPCPs, and IPDL-generated code doesn't hold a
  // reference to top-level actors.  So we hold a reference to
  // ourself.  This is released (deferred) in ActorDestroy().
  RefPtr<EmbedLiteCompositorProcessParent> mSelfRef;
  Transport* mTransport;
  // Child side's process Id.
  base::ProcessId mChildProcessId;

  // If true, we should send a RemotePaintIsReady message when the layer transaction
  // is received
  bool mNotifyAfterRemotePaint;

  RefPtr<LayerManagerComposite> mLayerManager;
  RefPtr<Compositor> mCompositor;
  RefPtr<AsyncCompositionManager> mCompositionManager;
  uint64_t mCompositorID;
  nsIntSize mEGLSurfaceSize;
};

} // embedlite
} // mozilla

#endif // mozilla_layers_EmbedLiteCompositorProcessParent_h
