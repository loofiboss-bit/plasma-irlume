// SPDX-License-Identifier: GPL-3.0-or-later

#![forbid(unsafe_code)]

use std::io::{Read, Write};

use kfaceauth_protocol::{
    Capabilities, CodecError, MAX_REQUEST_BYTES, MAX_RESPONSE_BYTES, Request, Response, Status,
    decode_request, read_frame, write_frame,
};

#[must_use]
pub const fn handle_request(request: Request) -> Response {
    match request {
        Request::Capabilities => Response::Capabilities(Capabilities::local_identity()),
        Request::Status => Response::Status(Status::local_identity()),
    }
}

/// Processes exactly one bounded request and writes exactly one bounded response.
///
/// # Errors
///
/// Returns [`CodecError`] for malformed or oversized frames and for I/O errors.
pub fn serve_once<R: Read, W: Write>(reader: &mut R, writer: &mut W) -> Result<(), CodecError> {
    let request_payload = read_frame(reader, MAX_REQUEST_BYTES)?;
    let request = decode_request(&request_payload)?;
    let response_payload = kfaceauth_protocol::encode_response(handle_request(request));
    write_frame(writer, &response_payload, MAX_RESPONSE_BYTES)
}

#[cfg(test)]
mod tests {
    use super::*;
    use kfaceauth_protocol::{OperationSupport, decode_response, encode_request};
    use std::io::Cursor;

    #[test]
    fn only_status_and_capabilities_are_dispatched() {
        let response = handle_request(Request::Capabilities);
        let Response::Capabilities(capabilities) = response else {
            panic!("expected capabilities");
        };
        assert_eq!(capabilities.detector_analysis, OperationSupport::Supported);
        assert_eq!(
            capabilities.embedding_extraction,
            OperationSupport::Supported
        );
        assert_eq!(
            capabilities.user_session_enrollment,
            OperationSupport::Supported
        );
        assert_eq!(
            capabilities.explicit_local_verification,
            OperationSupport::Supported
        );
        assert_eq!(
            capabilities.pam_authentication,
            OperationSupport::Unsupported
        );
        assert_eq!(
            capabilities.pam_configuration,
            OperationSupport::Unsupported
        );
        assert_eq!(
            capabilities.encrypted_user_session_persistence,
            OperationSupport::Supported
        );
    }

    #[test]
    fn framed_dispatch_is_bounded() {
        let payload = encode_request(Request::Status);
        let mut input = Vec::new();
        write_frame(&mut input, &payload, MAX_REQUEST_BYTES).unwrap();
        let mut output = Vec::new();
        serve_once(&mut Cursor::new(input), &mut output).unwrap();
        let response_payload = read_frame(&mut Cursor::new(output), MAX_RESPONSE_BYTES).unwrap();
        assert!(matches!(
            decode_response(&response_payload).unwrap(),
            Response::Status(_)
        ));
    }
}
