/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "nsISupportsImpl.h"

#include "mozilla/Atomics.h"
#include "nsIThread.h"
#include "nsThreadUtils.h"

#include "gtest/gtest.h"
#include "mozilla/gtest/MozAssertions.h"

using namespace mozilla;

class nsThreadSafeAutoRefCntRunner final : public Runnable {
 public:
  NS_IMETHOD Run() final {
    for (int i = 0; i < 10000; i++) {
      if (++sRefCnt == 1) {
        sIncToOne++;
      }
      if (--sRefCnt == 0) {
        sDecToZero++;
      }
    }
    return NS_OK;
  }

  static ThreadSafeAutoRefCnt sRefCnt;
  static Atomic<uint32_t, Relaxed> sIncToOne;
  static Atomic<uint32_t, Relaxed> sDecToZero;

  nsThreadSafeAutoRefCntRunner() : Runnable("nsThreadSafeAutoRefCntRunner") {}

 private:
  ~nsThreadSafeAutoRefCntRunner() = default;
};

ThreadSafeAutoRefCnt nsThreadSafeAutoRefCntRunner::sRefCnt;
Atomic<uint32_t, Relaxed> nsThreadSafeAutoRefCntRunner::sIncToOne(0);
Atomic<uint32_t, Relaxed> nsThreadSafeAutoRefCntRunner::sDecToZero(0);

class nsThreadSafeAutoRefCntDecrementWithLimitRunner final : public Runnable {
 public:
  static constexpr size_t kDecrementsPerThread = 1000;

  NS_IMETHOD Run() final {
    for (size_t i = 0; i < kDecrementsPerThread; i++) {
      auto [ok, count] = sRefCnt.DecrementWithLimit<1>();
      if (!ok) {
        sLimitHits++;
        break;
      }
    }
    return NS_OK;
  }

  static ThreadSafeAutoRefCnt sRefCnt;
  // Relaxed ordering so sLimitHits doesn't affect the ordering properties of
  // DecrementWithLimit being tested.
  static Atomic<uint32_t, Relaxed> sLimitHits;

  nsThreadSafeAutoRefCntDecrementWithLimitRunner()
      : Runnable("nsThreadSafeAutoRefCntDecrementWithLimitRunner") {}

 private:
  ~nsThreadSafeAutoRefCntDecrementWithLimitRunner() = default;
};

ThreadSafeAutoRefCnt nsThreadSafeAutoRefCntDecrementWithLimitRunner::sRefCnt;
Atomic<uint32_t, Relaxed>
    nsThreadSafeAutoRefCntDecrementWithLimitRunner::sLimitHits(0);

// When a refcounted object is actually owned by a cache, we may not
// want to release the object after last reference gets released. In
// this pattern, the cache may rely on the balance of increment to one
// and decrement to zero, so that it can maintain a counter for GC.
TEST(AutoRefCnt, ThreadSafeAutoRefCntBalance)
{
  static const size_t kThreadCount = 4;
  nsCOMPtr<nsIThread> threads[kThreadCount];
  for (auto& thread : threads) {
    nsresult rv = NS_NewNamedThread("AutoRefCnt Test", getter_AddRefs(thread),
                                    new nsThreadSafeAutoRefCntRunner);
    EXPECT_NS_SUCCEEDED(rv);
  }
  for (const auto& thread : threads) {
    thread->Shutdown();
  }
  EXPECT_EQ(nsThreadSafeAutoRefCntRunner::sRefCnt, nsrefcnt(0));
  EXPECT_EQ(nsThreadSafeAutoRefCntRunner::sIncToOne,
            nsThreadSafeAutoRefCntRunner::sDecToZero);
}

// Verify DecrementWithLimit with no lost updates when the limit is never hit.
// Four threads each perform 1000 decrements from 4001; all succeed, final
// value is 1 and the limit is never reached.
TEST(AutoRefCnt, ThreadSafeAutoRefCntDecrementWithLimitNoHit)
{
  static const size_t kThreadCount = 4;
  const nsrefcnt kInitial =
      kThreadCount *
          nsThreadSafeAutoRefCntDecrementWithLimitRunner::kDecrementsPerThread +
      1;
  nsThreadSafeAutoRefCntDecrementWithLimitRunner::sRefCnt = kInitial;
  nsThreadSafeAutoRefCntDecrementWithLimitRunner::sLimitHits = 0;

  nsCOMPtr<nsIThread> threads[kThreadCount];
  for (auto& thread : threads) {
    nsresult rv =
        NS_NewNamedThread("AutoRefCnt Test", getter_AddRefs(thread),
                          new nsThreadSafeAutoRefCntDecrementWithLimitRunner);
    EXPECT_NS_SUCCEEDED(rv);
  }
  for (const auto& thread : threads) {
    thread->Shutdown();
  }
  EXPECT_EQ(nsThreadSafeAutoRefCntDecrementWithLimitRunner::sRefCnt,
            nsrefcnt(1));
  EXPECT_EQ(nsThreadSafeAutoRefCntDecrementWithLimitRunner::sLimitHits,
            uint32_t(0));
}

// Verify DecrementWithLimit correctly enforces the limit. Of four threads each
// attempting 1000 decrements, exactly kExpectedLimitHits will hit the limit,
// and the final value is 1.
TEST(AutoRefCnt, ThreadSafeAutoRefCntDecrementWithLimitHit)
{
  static const size_t kThreadCount = 4;
  static const size_t kExpectedLimitHits = 2;
  // Set the initial value so that exactly (kThreadCount - kExpectedLimitHits)
  // threads can complete all their decrements; the rest hit the limit.
  const nsrefcnt kInitial =
      (kThreadCount - kExpectedLimitHits) *
          nsThreadSafeAutoRefCntDecrementWithLimitRunner::kDecrementsPerThread +
      1;
  nsThreadSafeAutoRefCntDecrementWithLimitRunner::sRefCnt = kInitial;
  nsThreadSafeAutoRefCntDecrementWithLimitRunner::sLimitHits = 0;

  nsCOMPtr<nsIThread> threads[kThreadCount];
  for (auto& thread : threads) {
    nsresult rv =
        NS_NewNamedThread("AutoRefCnt Test", getter_AddRefs(thread),
                          new nsThreadSafeAutoRefCntDecrementWithLimitRunner);
    EXPECT_NS_SUCCEEDED(rv);
  }
  for (const auto& thread : threads) {
    thread->Shutdown();
  }
  EXPECT_EQ(nsThreadSafeAutoRefCntDecrementWithLimitRunner::sRefCnt,
            nsrefcnt(1));
  EXPECT_EQ(nsThreadSafeAutoRefCntDecrementWithLimitRunner::sLimitHits,
            uint32_t(kExpectedLimitHits));
}
