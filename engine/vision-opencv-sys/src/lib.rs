// SPDX-License-Identifier: GPL-3.0-or-later

//! Narrow safe wrapper around the reviewed KFaceAuth/OpenCV C ABI.
//!
//! This crate is the only Rust package permitted to contain `unsafe` code.
//! C++ objects and OpenCV types never cross the boundary.

#![deny(unsafe_op_in_unsafe_fn)]

use std::ffi::{CStr, c_char, c_int, c_void};
use std::fmt;
use std::marker::PhantomData;
use std::ptr::NonNull;
use std::rc::Rc;

const STATUS_OK: c_int = 0;
const STATUS_INVALID_ARGUMENT: c_int = 1;
const STATUS_RUNTIME_FAILURE: c_int = 2;
const STATUS_OUTPUT_TOO_LARGE: c_int = 3;
const STATUS_MALFORMED_OUTPUT: c_int = 4;
const STATUS_HARDENING_FAILURE: c_int = 5;

#[repr(C)]
#[derive(Clone, Copy, Debug, Default)]
pub struct RawDetection {
    pub values: [f32; 15],
}

unsafe extern "C" {
    fn kfaceauth_yunet_disable_core_dumps() -> c_int;
    fn kfaceauth_yunet_opencv_version() -> *const c_char;
    fn kfaceauth_yunet_create(
        model_bytes: *const u8,
        model_size: usize,
        width: i32,
        height: i32,
        score_threshold: f32,
        nms_threshold: f32,
        top_k: i32,
        detector_out: *mut *mut c_void,
    ) -> c_int;
    fn kfaceauth_yunet_detect(
        detector: *mut c_void,
        bgr_bytes: *const u8,
        bgr_size: usize,
        width: i32,
        height: i32,
        stride: usize,
        detections: *mut RawDetection,
        detection_capacity: usize,
        detection_count: *mut usize,
    ) -> c_int;
    fn kfaceauth_yunet_destroy(detector: *mut c_void);
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum BridgeError {
    InvalidArgument,
    RuntimeFailure,
    OutputTooLarge,
    MalformedOutput,
    HardeningFailure,
    UnknownStatus,
}

impl fmt::Display for BridgeError {
    fn fmt(&self, formatter: &mut fmt::Formatter<'_>) -> fmt::Result {
        formatter.write_str(match self {
            Self::InvalidArgument => "OpenCV bridge rejected an invalid argument",
            Self::RuntimeFailure => "OpenCV YuNet runtime failed",
            Self::OutputTooLarge => "OpenCV YuNet returned too many detections",
            Self::MalformedOutput => "OpenCV YuNet returned malformed output",
            Self::HardeningFailure => "vision worker core-dump hardening failed",
            Self::UnknownStatus => "OpenCV bridge returned an unknown status",
        })
    }
}

impl std::error::Error for BridgeError {}

fn status_result(status: c_int) -> Result<(), BridgeError> {
    match status {
        STATUS_OK => Ok(()),
        STATUS_INVALID_ARGUMENT => Err(BridgeError::InvalidArgument),
        STATUS_RUNTIME_FAILURE => Err(BridgeError::RuntimeFailure),
        STATUS_OUTPUT_TOO_LARGE => Err(BridgeError::OutputTooLarge),
        STATUS_MALFORMED_OUTPUT => Err(BridgeError::MalformedOutput),
        STATUS_HARDENING_FAILURE => Err(BridgeError::HardeningFailure),
        _ => Err(BridgeError::UnknownStatus),
    }
}

/// Disables core dumps for the current short-lived worker process.
///
/// # Errors
///
/// Returns a stable bridge error if the operating system refuses hardening.
pub fn disable_core_dumps() -> Result<(), BridgeError> {
    // SAFETY: this function takes no pointers and has no caller-owned state.
    status_result(unsafe { kfaceauth_yunet_disable_core_dumps() })
}

#[must_use]
pub fn opencv_version() -> String {
    // SAFETY: OpenCV returns a process-lifetime static NUL-terminated string.
    let pointer = unsafe { kfaceauth_yunet_opencv_version() };
    if pointer.is_null() {
        return "unknown".to_owned();
    }
    // SAFETY: the bridge contract guarantees a valid static C string.
    unsafe { CStr::from_ptr(pointer) }
        .to_string_lossy()
        .into_owned()
}

pub struct Detector {
    handle: NonNull<c_void>,
    _not_send_or_sync: PhantomData<Rc<()>>,
}

impl Detector {
    /// Constructs an OpenCV CPU YuNet detector from already verified bytes.
    ///
    /// # Errors
    ///
    /// Returns a stable bridge error for rejected parameters or runtime load
    /// failures.
    pub fn new(
        model_bytes: &[u8],
        width: u32,
        height: u32,
        score_threshold: f32,
        nms_threshold: f32,
        top_k: usize,
    ) -> Result<Self, BridgeError> {
        let width = i32::try_from(width).map_err(|_| BridgeError::InvalidArgument)?;
        let height = i32::try_from(height).map_err(|_| BridgeError::InvalidArgument)?;
        let top_k = i32::try_from(top_k).map_err(|_| BridgeError::InvalidArgument)?;
        let mut handle = std::ptr::null_mut();
        // SAFETY: slices provide valid pointers and lengths for the duration
        // of the call; the output pointer refers to local initialized storage.
        status_result(unsafe {
            kfaceauth_yunet_create(
                model_bytes.as_ptr(),
                model_bytes.len(),
                width,
                height,
                score_threshold,
                nms_threshold,
                top_k,
                &mut handle,
            )
        })?;
        let handle = NonNull::new(handle).ok_or(BridgeError::RuntimeFailure)?;
        Ok(Self {
            handle,
            _not_send_or_sync: PhantomData,
        })
    }

    /// Runs one bounded BGR frame and returns raw 15-float YuNet rows.
    ///
    /// # Errors
    ///
    /// Returns a stable bridge error for invalid geometry, runtime failure, or
    /// malformed output.
    pub fn detect(
        &self,
        bgr_bytes: &[u8],
        width: u32,
        height: u32,
        stride: usize,
        maximum_detections: usize,
    ) -> Result<Vec<RawDetection>, BridgeError> {
        let width = i32::try_from(width).map_err(|_| BridgeError::InvalidArgument)?;
        let height = i32::try_from(height).map_err(|_| BridgeError::InvalidArgument)?;
        if maximum_detections == 0 || maximum_detections > 5_000 {
            return Err(BridgeError::InvalidArgument);
        }
        let mut detections = vec![RawDetection::default(); maximum_detections];
        let mut count = 0_usize;
        // SAFETY: all buffers remain alive and uniquely writable as required
        // for the call; the opaque handle originated from the same bridge.
        status_result(unsafe {
            kfaceauth_yunet_detect(
                self.handle.as_ptr(),
                bgr_bytes.as_ptr(),
                bgr_bytes.len(),
                width,
                height,
                stride,
                detections.as_mut_ptr(),
                detections.len(),
                &mut count,
            )
        })?;
        if count > detections.len() {
            detections.fill(RawDetection::default());
            return Err(BridgeError::MalformedOutput);
        }
        detections.truncate(count);
        Ok(detections)
    }
}

impl Drop for Detector {
    fn drop(&mut self) {
        // SAFETY: the handle is uniquely owned and destroyed exactly once.
        unsafe { kfaceauth_yunet_destroy(self.handle.as_ptr()) };
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn rejects_invalid_constructor_inputs_before_runtime_use() {
        assert!(matches!(
            Detector::new(&[], 320, 320, 0.9, 0.3, 8),
            Err(BridgeError::InvalidArgument)
        ));
    }

    #[test]
    fn reports_an_opencv_version() {
        assert!(!opencv_version().is_empty());
    }
}
