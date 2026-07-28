// SPDX-License-Identifier: GPL-3.0-or-later

#![forbid(unsafe_code)]

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum TemplateAvailability {
    PersistenceUnavailableInMilestoneOne,
}

#[must_use]
pub const fn availability() -> TemplateAvailability {
    TemplateAvailability::PersistenceUnavailableInMilestoneOne
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn persistence_is_explicitly_unavailable() {
        assert_eq!(
            availability(),
            TemplateAvailability::PersistenceUnavailableInMilestoneOne
        );
    }
}
