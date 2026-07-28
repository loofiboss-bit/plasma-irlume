// SPDX-License-Identifier: GPL-3.0-or-later

#![forbid(unsafe_code)]

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum VisionAvailability {
    UnavailableInMilestoneOne,
}

#[must_use]
pub const fn availability() -> VisionAvailability {
    VisionAvailability::UnavailableInMilestoneOne
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn vision_is_explicitly_unavailable() {
        assert_eq!(
            availability(),
            VisionAvailability::UnavailableInMilestoneOne
        );
    }
}
