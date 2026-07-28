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
    beginResetModel();
    m_profiles.clear();
    m_readOnlyAvailable = snapshot.profiles.has_value();
    m_mutationSupported = snapshot.capabilities.mutationSupported;
    m_maxProfiles = snapshot.capabilities.maxProfiles;
    if (snapshot.profiles)
    {
        for (const EngineProfile &profile : snapshot.profiles->profiles)
        {
            m_profiles.push_back({profile.stableId.value_or(QString()), profile.displayName, profile.scanDisplayNames});
        }
    }
    endResetModel();
    m_errorCode.clear();
    if (!snapshot.contractAvailable)
    {
        m_errorCode = snapshot.errors.isEmpty() ? QStringLiteral("machine-contract-unavailable")
                                                : snapshot.errors.constFirst().code;
        m_statusText = translate("The backend did not complete a valid read-only handshake.");
    }
    else if (!snapshot.capabilities.profilesRead)
    {
        m_statusText = translate("The backend does not advertise read-only profile listing.");
    }
    else if (!snapshot.profiles)
    {
        m_errorCode =
            snapshot.errors.isEmpty() ? QStringLiteral("profiles-unavailable") : snapshot.errors.constLast().code;
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
    return false;
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
    return false;
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
