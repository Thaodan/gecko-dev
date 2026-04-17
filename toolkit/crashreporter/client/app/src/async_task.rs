/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

//! Manage work across multiple threads.
//!
//! Each thread has thread-bound data which can be accessed in queued task functions.

use crate::thread_bound::ThreadBound;
use std::cell::RefCell;
use std::future::Future;
use std::pin::Pin;
use std::sync::{Arc, Weak};

pub type TaskFn<T> = Box<dyn FnOnce(&T) + Send + 'static>;

pub type BoxFuture<'a, T> = Pin<Box<dyn Future<Output = T> + 'a>>;

type AsyncTaskSend<T> = dyn Fn(TaskFn<T>) + Send + Sync;

pub enum AsyncTask<T> {
    Strong(Arc<AsyncTaskSend<T>>),
    Weak(Weak<AsyncTaskSend<T>>),
}

impl<T> Clone for AsyncTask<T> {
    fn clone(&self) -> Self {
        match self {
            Self::Strong(s) => Self::Strong(s.clone()),
            Self::Weak(w) => Self::Weak(w.clone()),
        }
    }
}

impl<T> AsyncTask<T> {
    pub fn new<F: Fn(TaskFn<T>) + Send + Sync + 'static>(send: F) -> Self {
        AsyncTask::Strong(Arc::new(send))
    }

    pub fn weak(&self) -> Self {
        match self {
            Self::Strong(s) => Self::Weak(Arc::downgrade(s)),
            Self::Weak(w) => Self::Weak(w.clone()),
        }
    }

    fn strong(&self) -> Option<Self> {
        match self {
            Self::Strong(s) => Some(Self::Strong(s.clone())),
            Self::Weak(w) => w.upgrade().map(Self::Strong),
        }
    }

    pub fn push<F: FnOnce(&T) + Send + 'static>(&self, f: F) {
        match self {
            Self::Strong(a) => a(Box::new(f)),
            Self::Weak(w) => {
                if let Some(a) = w.upgrade() {
                    a(Box::new(f));
                }
            }
        }
    }

    /// NOTE: any Wakers stored by futures will hold a reference to this AsyncTask, so if the
    /// AsyncTask lifetime is relevant, you must ensure the futures will call wake or be dropped by
    /// other means.
    pub fn push_async<F: for<'a> FnOnce(&'a T) -> BoxFuture<'a, ()> + Send + 'static>(&self, f: F)
    where
        T: 'static,
    {
        let Some(inner) = self.strong() else {
            return;
        };
        self.push(move |v| {
            let waker = FutWaker {
                task: inner,
                fut: ThreadBound::new(RefCell::new(f(v))),
            };
            // SAFETY: The future will only ever be polled on the same AsyncTask target thread, so
            // any references it has will remain valid. Any references it has must either be
            // `'static` or those from the thread-bound data. We change `T` to `()` because `T` also
            // needs to be static, but from this point on we don't use `T` anyway. This is safe to
            // do because it changes the `&T` reference (where `T: Sized`) passed to the function to
            // be `&()`, so the function still receives a pointer argument (we don't break any
            // calling convention stuff) and will do nothing with it.
            let waker = Arc::new(unsafe { std::mem::transmute::<_, FutWaker<'static, ()>>(waker) });
            waker.poll();
        });
    }

    pub fn wait<R: Send + 'static, F: FnOnce(&T) -> R + Send + 'static>(&self, f: F) -> R {
        let (tx, rx) = std::sync::mpsc::sync_channel(0);
        self.push(move |v| tx.send(f(v)).unwrap());
        rx.recv().unwrap()
    }
}

struct FutWaker<'a, T> {
    task: AsyncTask<T>,
    // The RefCell is technically unnecessary (poll() will only be called from a dedicated thread
    // that owns the data), but it avoids some unsafe code and this isn't performance-critical.
    fut: ThreadBound<RefCell<BoxFuture<'a, ()>>>,
}

impl FutWaker<'static, ()> {
    fn poll(self: Arc<Self>) {
        let waker = std::task::Waker::from(self.clone());
        let mut cx = std::task::Context::from_waker(&waker);
        // We don't need to know whether the poll returns pending or ready; if pending, the waker
        // will handle queueing things again.
        let _ = self.fut.borrow().borrow_mut().as_mut().poll(&mut cx);
    }
}

impl std::task::Wake for FutWaker<'static, ()> {
    fn wake(self: Arc<Self>) {
        let inner = self.clone();
        self.task.push(move |()| inner.poll());
    }
}
