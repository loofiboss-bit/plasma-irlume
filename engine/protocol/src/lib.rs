// SPDX-License-Identifier: GPL-3.0-or-later

#![forbid(unsafe_code)]

use std::fmt;
use std::io::{self, Read, Write};

pub const PROTOCOL_VERSION: u16 = 1;
pub const MAX_REQUEST_BYTES: usize = 4 * 1024;
pub const MAX_RESPONSE_BYTES: usize = 16 * 1024;
pub const MAX_REQUEST_BYTES_WIRE: u32 = 4 * 1024;
pub const MAX_RESPONSE_BYTES_WIRE: u32 = 16 * 1024;
pub const OPERATION_CAPABILITIES: u16 = 1 << 0;
pub const OPERATION_STATUS: u16 = 1 << 1;

const REQUEST_CAPABILITIES: u8 = 1;
const REQUEST_STATUS: u8 = 2;
const RESPONSE_CAPABILITIES: u8 = 0x81;
const RESPONSE_STATUS: u8 = 0x82;
const RESPONSE_ERROR: u8 = 0xff;

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum Request {
    Capabilities,
    Status,
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum OperationSupport {
    Unsupported,
    Supported,
}

impl OperationSupport {
    const fn wire_value(self) -> u8 {
        match self {
            Self::Unsupported => 0,
            Self::Supported => 1,
        }
    }

    fn from_wire(value: u8) -> Result<Self, CodecError> {
        match value {
            0 => Ok(Self::Unsupported),
            1 => Ok(Self::Supported),
            _ => Err(CodecError::InvalidPayload),
        }
    }
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum DisabledCapability {
    Disabled,
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum EngineState {
    Skeleton,
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub struct Capabilities {
    pub protocol_min: u16,
    pub protocol_max: u16,
    pub max_request_bytes: u32,
    pub max_response_bytes: u32,
    pub operations: u16,
    pub vision: OperationSupport,
    pub enrollment: OperationSupport,
    pub authentication: OperationSupport,
    pub pam_configuration: OperationSupport,
    pub template_persistence: OperationSupport,
    pub network_access: DisabledCapability,
    pub runtime_model_downloads: DisabledCapability,
}

impl Capabilities {
    #[must_use]
    pub const fn milestone_one() -> Self {
        Self {
            protocol_min: PROTOCOL_VERSION,
            protocol_max: PROTOCOL_VERSION,
            max_request_bytes: MAX_REQUEST_BYTES_WIRE,
            max_response_bytes: MAX_RESPONSE_BYTES_WIRE,
            operations: OPERATION_CAPABILITIES | OPERATION_STATUS,
            vision: OperationSupport::Unsupported,
            enrollment: OperationSupport::Unsupported,
            authentication: OperationSupport::Unsupported,
            pam_configuration: OperationSupport::Unsupported,
            template_persistence: OperationSupport::Unsupported,
            network_access: DisabledCapability::Disabled,
            runtime_model_downloads: DisabledCapability::Disabled,
        }
    }
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub struct Status {
    pub state: EngineState,
    pub vision: OperationSupport,
    pub enrollment: OperationSupport,
    pub authentication: OperationSupport,
    pub pam_configuration: OperationSupport,
    pub template_persistence: OperationSupport,
}

impl Status {
    #[must_use]
    pub const fn milestone_one() -> Self {
        Self {
            state: EngineState::Skeleton,
            vision: OperationSupport::Unsupported,
            enrollment: OperationSupport::Unsupported,
            authentication: OperationSupport::Unsupported,
            pam_configuration: OperationSupport::Unsupported,
            template_persistence: OperationSupport::Unsupported,
        }
    }
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum ErrorCode {
    InvalidRequest,
    UnsupportedVersion,
    UnsupportedOperation,
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum Response {
    Capabilities(Capabilities),
    Status(Status),
    Error(ErrorCode),
}

#[derive(Debug)]
pub enum CodecError {
    Io(io::Error),
    EmptyFrame,
    FrameTooLarge { length: usize, maximum: usize },
    InvalidPayload,
    UnsupportedVersion(u16),
}

impl fmt::Display for CodecError {
    fn fmt(&self, formatter: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Self::Io(error) => write!(formatter, "local protocol I/O failed: {error}"),
            Self::EmptyFrame => formatter.write_str("local protocol frame is empty"),
            Self::FrameTooLarge { length, maximum } => {
                write!(
                    formatter,
                    "local protocol frame length {length} exceeds {maximum}"
                )
            }
            Self::InvalidPayload => formatter.write_str("local protocol payload is invalid"),
            Self::UnsupportedVersion(version) => {
                write!(formatter, "local protocol version {version} is unsupported")
            }
        }
    }
}

impl std::error::Error for CodecError {}

impl From<io::Error> for CodecError {
    fn from(error: io::Error) -> Self {
        Self::Io(error)
    }
}

#[must_use]
pub fn encode_request(request: Request) -> Vec<u8> {
    let kind = match request {
        Request::Capabilities => REQUEST_CAPABILITIES,
        Request::Status => REQUEST_STATUS,
    };
    let version = PROTOCOL_VERSION.to_be_bytes();
    vec![version[0], version[1], kind, 0]
}

/// Decodes one bounded request payload.
///
/// # Errors
///
/// Returns [`CodecError`] when the payload is empty, oversized, malformed, or
/// uses an unsupported protocol version.
pub fn decode_request(payload: &[u8]) -> Result<Request, CodecError> {
    validate_payload(payload, MAX_REQUEST_BYTES)?;
    if payload.len() != 4 || payload[3] != 0 {
        return Err(CodecError::InvalidPayload);
    }
    validate_version(payload)?;
    match payload[2] {
        REQUEST_CAPABILITIES => Ok(Request::Capabilities),
        REQUEST_STATUS => Ok(Request::Status),
        _ => Err(CodecError::InvalidPayload),
    }
}

#[must_use]
pub fn encode_response(response: Response) -> Vec<u8> {
    let version = PROTOCOL_VERSION.to_be_bytes();
    let mut payload = vec![version[0], version[1], 0, 0];
    match response {
        Response::Capabilities(capabilities) => {
            payload[2] = RESPONSE_CAPABILITIES;
            payload.extend_from_slice(&capabilities.protocol_min.to_be_bytes());
            payload.extend_from_slice(&capabilities.protocol_max.to_be_bytes());
            payload.extend_from_slice(&capabilities.max_request_bytes.to_be_bytes());
            payload.extend_from_slice(&capabilities.max_response_bytes.to_be_bytes());
            payload.extend_from_slice(&capabilities.operations.to_be_bytes());
            payload.extend_from_slice(&support_bytes(
                capabilities.vision,
                capabilities.enrollment,
                capabilities.authentication,
                capabilities.pam_configuration,
                capabilities.template_persistence,
            ));
            payload.push(0);
            payload.push(0);
        }
        Response::Status(status) => {
            payload[2] = RESPONSE_STATUS;
            payload.push(1);
            payload.extend_from_slice(&support_bytes(
                status.vision,
                status.enrollment,
                status.authentication,
                status.pam_configuration,
                status.template_persistence,
            ));
        }
        Response::Error(code) => {
            payload[2] = RESPONSE_ERROR;
            payload.push(match code {
                ErrorCode::InvalidRequest => 1,
                ErrorCode::UnsupportedVersion => 2,
                ErrorCode::UnsupportedOperation => 3,
            });
        }
    }
    payload
}

/// Decodes one bounded response payload.
///
/// # Errors
///
/// Returns [`CodecError`] when the payload is empty, oversized, malformed, or
/// uses an unsupported protocol version.
pub fn decode_response(payload: &[u8]) -> Result<Response, CodecError> {
    validate_payload(payload, MAX_RESPONSE_BYTES)?;
    if payload.len() < 5 || payload[3] != 0 {
        return Err(CodecError::InvalidPayload);
    }
    validate_version(payload)?;
    match payload[2] {
        RESPONSE_CAPABILITIES => decode_capabilities(payload),
        RESPONSE_STATUS => decode_status(payload),
        RESPONSE_ERROR if payload.len() == 5 => {
            let code = match payload[4] {
                1 => ErrorCode::InvalidRequest,
                2 => ErrorCode::UnsupportedVersion,
                3 => ErrorCode::UnsupportedOperation,
                _ => return Err(CodecError::InvalidPayload),
            };
            Ok(Response::Error(code))
        }
        _ => Err(CodecError::InvalidPayload),
    }
}

/// Reads one length-prefixed frame without allocating beyond `maximum`.
///
/// # Errors
///
/// Returns [`CodecError`] for I/O errors, zero-length frames, or declared
/// lengths greater than `maximum`.
pub fn read_frame<R: Read>(reader: &mut R, maximum: usize) -> Result<Vec<u8>, CodecError> {
    let mut header = [0_u8; 4];
    reader.read_exact(&mut header)?;
    let length = u32::from_be_bytes(header) as usize;
    if length == 0 {
        return Err(CodecError::EmptyFrame);
    }
    if length > maximum {
        return Err(CodecError::FrameTooLarge { length, maximum });
    }
    let mut payload = vec![0_u8; length];
    reader.read_exact(&mut payload)?;
    Ok(payload)
}

/// Writes one bounded length-prefixed frame.
///
/// # Errors
///
/// Returns [`CodecError`] for empty or oversized payloads and for I/O errors.
pub fn write_frame<W: Write>(
    writer: &mut W,
    payload: &[u8],
    maximum: usize,
) -> Result<(), CodecError> {
    validate_payload(payload, maximum)?;
    let length = u32::try_from(payload.len()).map_err(|_| CodecError::FrameTooLarge {
        length: payload.len(),
        maximum,
    })?;
    writer.write_all(&length.to_be_bytes())?;
    writer.write_all(payload)?;
    writer.flush()?;
    Ok(())
}

fn validate_payload(payload: &[u8], maximum: usize) -> Result<(), CodecError> {
    if payload.is_empty() {
        return Err(CodecError::EmptyFrame);
    }
    if payload.len() > maximum {
        return Err(CodecError::FrameTooLarge {
            length: payload.len(),
            maximum,
        });
    }
    Ok(())
}

fn validate_version(payload: &[u8]) -> Result<(), CodecError> {
    if payload.len() < 2 {
        return Err(CodecError::InvalidPayload);
    }
    let version = u16::from_be_bytes([payload[0], payload[1]]);
    if version != PROTOCOL_VERSION {
        return Err(CodecError::UnsupportedVersion(version));
    }
    Ok(())
}

const fn support_bytes(
    vision: OperationSupport,
    enrollment: OperationSupport,
    authentication: OperationSupport,
    pam_configuration: OperationSupport,
    template_persistence: OperationSupport,
) -> [u8; 5] {
    [
        vision.wire_value(),
        enrollment.wire_value(),
        authentication.wire_value(),
        pam_configuration.wire_value(),
        template_persistence.wire_value(),
    ]
}

fn decode_support(payload: &[u8]) -> Result<[OperationSupport; 5], CodecError> {
    Ok([
        OperationSupport::from_wire(payload[0])?,
        OperationSupport::from_wire(payload[1])?,
        OperationSupport::from_wire(payload[2])?,
        OperationSupport::from_wire(payload[3])?,
        OperationSupport::from_wire(payload[4])?,
    ])
}

fn decode_capabilities(payload: &[u8]) -> Result<Response, CodecError> {
    if payload.len() != 25 || payload[23] != 0 || payload[24] != 0 {
        return Err(CodecError::InvalidPayload);
    }
    let support = decode_support(&payload[18..23])?;
    Ok(Response::Capabilities(Capabilities {
        protocol_min: u16::from_be_bytes([payload[4], payload[5]]),
        protocol_max: u16::from_be_bytes([payload[6], payload[7]]),
        max_request_bytes: u32::from_be_bytes([payload[8], payload[9], payload[10], payload[11]]),
        max_response_bytes: u32::from_be_bytes([
            payload[12],
            payload[13],
            payload[14],
            payload[15],
        ]),
        operations: u16::from_be_bytes([payload[16], payload[17]]),
        vision: support[0],
        enrollment: support[1],
        authentication: support[2],
        pam_configuration: support[3],
        template_persistence: support[4],
        network_access: DisabledCapability::Disabled,
        runtime_model_downloads: DisabledCapability::Disabled,
    }))
}

fn decode_status(payload: &[u8]) -> Result<Response, CodecError> {
    if payload.len() != 10 || payload[4] != 1 {
        return Err(CodecError::InvalidPayload);
    }
    let support = decode_support(&payload[5..10])?;
    Ok(Response::Status(Status {
        state: EngineState::Skeleton,
        vision: support[0],
        enrollment: support[1],
        authentication: support[2],
        pam_configuration: support[3],
        template_persistence: support[4],
    }))
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::io::Cursor;

    #[test]
    fn request_round_trips_are_closed() {
        for request in [Request::Capabilities, Request::Status] {
            assert_eq!(decode_request(&encode_request(request)).unwrap(), request);
        }
        assert!(matches!(
            decode_request(&[0, 1, 9, 0]),
            Err(CodecError::InvalidPayload)
        ));
    }

    #[test]
    fn responses_round_trip() {
        let responses = [
            Response::Capabilities(Capabilities::milestone_one()),
            Response::Status(Status::milestone_one()),
            Response::Error(ErrorCode::UnsupportedOperation),
        ];
        for response in responses {
            let encoded = encode_response(response);
            assert!(encoded.len() <= MAX_RESPONSE_BYTES);
            assert_eq!(decode_response(&encoded).unwrap(), response);
        }
    }

    #[test]
    fn oversized_frame_is_rejected_before_payload_read() {
        let header = u32::try_from(MAX_REQUEST_BYTES + 1).unwrap().to_be_bytes();
        let mut reader = Cursor::new(header);
        assert!(matches!(
            read_frame(&mut reader, MAX_REQUEST_BYTES),
            Err(CodecError::FrameTooLarge { .. })
        ));
        assert_eq!(reader.position(), 4);
    }

    #[test]
    fn fragmented_reads_and_writes_are_bounded() {
        let payload = encode_request(Request::Status);
        let mut framed = Vec::new();
        write_frame(&mut framed, &payload, MAX_REQUEST_BYTES).unwrap();
        let decoded = read_frame(&mut Cursor::new(framed), MAX_REQUEST_BYTES).unwrap();
        assert_eq!(decoded, payload);
    }

    #[test]
    fn zero_length_and_wrong_version_fail_closed() {
        assert!(matches!(
            read_frame(&mut Cursor::new([0, 0, 0, 0]), MAX_REQUEST_BYTES),
            Err(CodecError::EmptyFrame)
        ));
        assert!(matches!(
            decode_request(&[0, 2, REQUEST_STATUS, 0]),
            Err(CodecError::UnsupportedVersion(2))
        ));
    }
}
