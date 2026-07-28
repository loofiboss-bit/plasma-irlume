// SPDX-License-Identifier: GPL-3.0-or-later

#include "profilemodel.h"

#include <QCoreApplication>

namespace
{
QString translate(const char *text)
{
    return QCoreApplication::translate("ProfileModel", text);
}
} // namespace

ProfileModel::ProfileModel(QObject *parent) : QAbstractListModel(parent) {}

int ProfileModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_profiles.size();
}

QVariant ProfileModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_profiles.size())
        return {};
    const Profile &profile = m_profiles.at(index.row());
    if (role == ProfileIdRole)
        return profile.id;
    if (role == DisplayNameRole)
        return profile.displayName;
    if (role == ScanCountRole)
        return profile.scanDisplayNames.size();
    if (role == ScansRole)
    {
        QVariantList scans;
        for (const QString &displayName : profile.scanDisplayNames)
        {
            scans.push_back(QVariantMap{
                {QStringLiteral("scanId"), QString()},
                {QStringLiteral("displayName"), displayName},
            });
        }
        return scans;
    }
    return {};
}

QHash<int, QByteArray> ProfileModel::roleNames() const
{
    return {
        {ProfileIdRole, QByteArrayLiteral("profileId")},
        {DisplayNameRole, QByteArrayLiteral("displayName")},
        {ScanCountRole, QByteArrayLiteral("scanCount")},
        {ScansRole, QByteArrayLiteral("scans")},
    };
}

void ProfileModel::applySnapshot(const EngineSnapshot &snapshot)
{
    m_contractAvailable = snapshot.contractAvailable();
    m_mutationSupported = snapshot.capabilities.supports(EngineFeature::ProfileMutation);
    m_maxProfiles = snapshot.capabilities.maxProfiles;
    m_resultState = snapshot.profiles.state;
    m_retryable = snapshot.profiles.error && snapshot.profiles.error->retryable;

    if (snapshot.profiles.state == ResultState::Loading || snapshot.profiles.state == ResultState::Pending)
    {
        m_statusText = translate("Updating the read-only profile list…");
        Q_EMIT stateChanged();
        return;
    }

    beginResetModel();
    m_profiles.clear();
    m_readOnlyAvailable = snapshot.profiles.state == ResultState::Available && snapshot.profiles.data.has_value();
    if (snapshot.profiles.data)
    {
        for (const EngineProfile &profile : snapshot.profiles.data->profiles)
        {
            m_profiles.push_back({profile.stableId.value_or(QString()), profile.displayName, profile.scanDisplayNames});
        }
    }
    endResetModel();
    m_errorCode.clear();
    if (!snapshot.contractAvailable())
    {
        m_errorCode =
            snapshot.handshake.error ? snapshot.handshake.error->code : QStringLiteral("machine-contract-unavailable");
        m_statusText = translate("The backend did not complete a valid read-only handshake.");
    }
    else if (snapshot.profiles.state == ResultState::NotAdvertised)
    {
        m_statusText = translate("The backend does not advertise read-only profile listing.");
    }
    else if (snapshot.profiles.state == ResultState::Failed)
    {
        m_errorCode = snapshot.profiles.error ? snapshot.profiles.error->code : QStringLiteral("profiles-unavailable");
        m_statusText = translate("The read-only profile list is unavailable.");
    }
    else
    {
        m_statusText =
            translate("Profiles are shown read-only. Contract 1 does not support enrollment or profile changes.");
    }
    Q_EMIT profilesChanged();
    Q_EMIT stateChanged();
}

bool ProfileModel::contractAvailable() const
{
    return m_contractAvailable;
}

bool ProfileModel::readOnlyAvailable() const
{
    return m_readOnlyAvailable;
}

bool ProfileModel::mutationSupported() const
{
    return m_mutationSupported;
}

bool ProfileModel::busy() const
{
    return m_resultState == ResultState::Loading || m_resultState == ResultState::Pending;
}

ResultState ProfileModel::availabilityState() const
{
    return m_resultState;
}

int ProfileModel::profileCount() const
{
    return m_profiles.size();
}

ProfileModel::Workflow ProfileModel::workflow() const
{
    return Workflow::Idle;
}

QString ProfileModel::statusText() const
{
    return m_statusText;
}

QString ProfileModel::stageLabel() const
{
    return {};
}

int ProfileModel::capturedScans() const
{
    return 0;
}

int ProfileModel::totalScans() const
{
    return 0;
}

QString ProfileModel::errorCode() const
{
    return m_errorCode;
}

bool ProfileModel::canRetry() const
{
    return m_retryable && !busy();
}

bool ProfileModel::cancellable() const
{
    return false;
}

int ProfileModel::maxProfiles() const
{
    return m_maxProfiles;
}

bool ProfileModel::mergeConfirmationRequired() const
{
    return false;
}

QString ProfileModel::pendingMergeProfileName() const
{
    return {};
}

int ProfileModel::pendingMergeScanCount() const
{
    return 0;
}

void ProfileModel::refresh()
{
    Q_EMIT refreshRequested();
}

void ProfileModel::failCapability()
{
    m_errorCode = QStringLiteral("capability-unavailable");
    m_statusText =
        translate("The installed backend exposes read-only Contract 1 but not the required mutation capability.");
    Q_EMIT stateChanged();
}

void ProfileModel::enroll()
{
    failCapability();
}

void ProfileModel::testRecognition(const QString &)
{
    failCapability();
}

void ProfileModel::addAppearanceScan(const QString &)
{
    failCapability();
}

void ProfileModel::deleteProfile(const QString &)
{
    failCapability();
}

void ProfileModel::deleteScan(const QString &, const QString &)
{
    failCapability();
}

void ProfileModel::renameProfile(const QString &, const QString &)
{
    failCapability();
}

void ProfileModel::renameScan(const QString &, const QString &, const QString &)
{
    failCapability();
}

void ProfileModel::confirmIdentityMerge(bool)
{
    failCapability();
}

void ProfileModel::cancel() {}

void ProfileModel::retry()
{
    failCapability();
}
