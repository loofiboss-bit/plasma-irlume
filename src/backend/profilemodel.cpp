// SPDX-License-Identifier: GPL-3.0-or-later

#include "profilemodel.h"

#include "enrollmentsession.h"

#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonObject>

#include <algorithm>

namespace
{
QString translate(const char *text)
{
    return QCoreApplication::translate("ProfileModel", text);
}

bool isSafeDisplayName(const QString &value)
{
    if (value.isEmpty() || value.size() > 80 || value != value.trimmed())
    {
        return false;
    }
    return std::none_of(value.cbegin(), value.cend(), [](QChar character) { return !character.isPrint(); });
}
} // namespace

ProfileModel::ProfileModel(QObject *parent) : ProfileModel(new IrlumeProcess, new EnrollmentSession, parent)
{
    m_process->setParent(this);
    m_enrollmentSession->setParent(this);
}

ProfileModel::ProfileModel(IrlumeProcess *process, QObject *parent) : ProfileModel(process, nullptr, parent) {}

ProfileModel::ProfileModel(IrlumeProcess *process, EnrollmentSession *enrollmentSession, QObject *parent)
    : QAbstractListModel(parent), m_process(process), m_enrollmentSession(enrollmentSession)
{
    Q_ASSERT(m_process);
    connect(m_process, &IrlumeProcess::eventReceived, this, &ProfileModel::handleEvent);
    connect(m_process, &IrlumeProcess::operationError, this, &ProfileModel::handleOperationError);
    if (m_enrollmentSession)
    {
        connect(m_enrollmentSession, &EnrollmentSession::eventReceived, this, &ProfileModel::handleEvent);
        connect(m_enrollmentSession, &EnrollmentSession::operationError, this, &ProfileModel::handleOperationError);
    }
}

int ProfileModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_profiles.size();
}

QVariant ProfileModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_profiles.size())
    {
        return {};
    }

    const Profile &profile = m_profiles.at(index.row());
    switch (role)
    {
    case ProfileIdRole:
        return profile.id;
    case DisplayNameRole:
        return profile.displayName;
    case ScanCountRole:
        return profile.scanCount;
    default:
        return {};
    }
}

QHash<int, QByteArray> ProfileModel::roleNames() const
{
    return {
        {ProfileIdRole, QByteArrayLiteral("profileId")},
        {DisplayNameRole, QByteArrayLiteral("displayName")},
        {ScanCountRole, QByteArrayLiteral("scanCount")},
    };
}

bool ProfileModel::contractAvailable() const
{
    return m_contractAvailable;
}

bool ProfileModel::busy() const
{
    return m_busy;
}

int ProfileModel::profileCount() const
{
    return m_profiles.size();
}

ProfileModel::Workflow ProfileModel::workflow() const
{
    return m_workflow;
}

QString ProfileModel::statusText() const
{
    return m_statusText;
}

QString ProfileModel::stageLabel() const
{
    return m_stageLabel;
}

int ProfileModel::capturedScans() const
{
    return m_capturedScans;
}

int ProfileModel::totalScans() const
{
    return m_totalScans;
}

QString ProfileModel::errorCode() const
{
    return m_errorCode;
}

bool ProfileModel::canRetry() const
{
    return m_canRetry;
}

bool ProfileModel::cancellable() const
{
    return m_busy && m_workflow != Workflow::CheckingContract && m_workflow != Workflow::LoadingProfiles &&
           m_workflow != Workflow::Deleting && m_workflow != Workflow::CleaningUp;
}

int ProfileModel::maxProfiles() const
{
    return m_maxProfiles;
}

void ProfileModel::refresh()
{
    if (m_busy)
    {
        return;
    }
    resetTransientState();
    m_contractAvailable = false;
    start(IrlumeProcess::Operation::Capabilities, Workflow::CheckingContract, {});
}

void ProfileModel::enroll()
{
    if (!m_contractAvailable || m_busy || m_profiles.size() >= m_maxProfiles)
    {
        return;
    }
    resetTransientState();
    m_lastAction = RequestedAction::Enroll;
    start(IrlumeProcess::Operation::Enroll, Workflow::Enrolling);
}

void ProfileModel::testRecognition(const QString &profileId)
{
    if (!m_contractAvailable || m_busy || !hasProfile(profileId))
    {
        return;
    }
    resetTransientState();
    m_lastAction = RequestedAction::Test;
    m_lastProfileId = profileId;
    start(IrlumeProcess::Operation::AuthTest, Workflow::Testing);
}

void ProfileModel::addAppearanceScan(const QString &profileId)
{
    if (!m_contractAvailable || m_busy || !hasProfile(profileId))
    {
        return;
    }
    resetTransientState();
    m_lastAction = RequestedAction::AddScan;
    m_lastProfileId = profileId;
    start(IrlumeProcess::Operation::AddScan, Workflow::AddingScan, profileId);
}

void ProfileModel::deleteProfile(const QString &profileId)
{
    if (!m_contractAvailable || m_busy || !hasProfile(profileId))
    {
        return;
    }
    resetTransientState();
    m_lastAction = RequestedAction::None;
    start(IrlumeProcess::Operation::DeleteProfile, Workflow::Deleting, profileId);
}

void ProfileModel::cancel()
{
    if (!m_busy || m_workflow == Workflow::CheckingContract || m_workflow == Workflow::LoadingProfiles ||
        m_workflow == Workflow::Deleting || m_workflow == Workflow::CleaningUp)
    {
        return;
    }
    m_statusText = translate("Cancelling and releasing the camera…");
    Q_EMIT stateChanged();
    if (m_enrollmentSession && m_enrollmentSession->active())
    {
        m_enrollmentSession->cancel();
    }
    else
    {
        m_process->cancel();
    }
}

void ProfileModel::retry()
{
    if (m_busy || !m_canRetry)
    {
        return;
    }
    const RequestedAction action = m_lastAction;
    const QString profileId = m_lastProfileId;
    m_canRetry = false;
    m_errorCode.clear();

    switch (action)
    {
    case RequestedAction::Enroll:
        enroll();
        break;
    case RequestedAction::Test:
        testRecognition(profileId);
        break;
    case RequestedAction::AddScan:
        addAppearanceScan(profileId);
        break;
    case RequestedAction::None:
        refresh();
        break;
    }
}

void ProfileModel::handleEvent(const IrlumeProcess::Event &event)
{
    if (!m_busy)
    {
        return;
    }

    if (event.type == QLatin1String("stage"))
    {
        m_stageLabel = labelForStage(event.data.value(QStringLiteral("stage")).toString());
        Q_EMIT stateChanged();
        return;
    }
    if (event.type == QLatin1String("progress"))
    {
        updateProgress(event.data);
        return;
    }
    if (!event.terminal)
    {
        return;
    }
    if (event.type == QLatin1String("failed") || event.type == QLatin1String("cancelled"))
    {
        const QString code = event.errorCode.isEmpty()
                                 ? (event.type == QLatin1String("cancelled") ? QStringLiteral("user-cancelled")
                                                                             : QStringLiteral("operation-failed"))
                                 : event.errorCode;
        if (event.operation == IrlumeProcess::Operation::AuthTest && !m_pendingNewProfileId.isEmpty())
        {
            cleanUpUnverifiedEnrollment(code);
        }
        else
        {
            finishError(code, event.retryable);
        }
        return;
    }

    switch (event.operation)
    {
    case IrlumeProcess::Operation::Capabilities:
    {
        const QJsonArray capabilities = event.data.value(QStringLiteral("capabilities")).toArray();
        const auto hasCapability = [&capabilities](const QString &required)
        {
            return std::any_of(capabilities.cbegin(), capabilities.cend(),
                               [&required](const QJsonValue &entry) { return entry.toString() == required; });
        };
        if (!hasCapability(QStringLiteral("profiles-json")) || !hasCapability(QStringLiteral("events-jsonl")) ||
            !hasCapability(QStringLiteral("position-report")) || !hasCapability(QStringLiteral("preview-ir-jpeg")))
        {
            finishError(QStringLiteral("structured-contract-unavailable"), false);
            return;
        }
        const int maxProfiles =
            event.data.value(QStringLiteral("limits")).toObject().value(QStringLiteral("max_profiles")).toInt(3);
        const int maxScans = event.data.value(QStringLiteral("limits"))
                                 .toObject()
                                 .value(QStringLiteral("max_scans_per_profile"))
                                 .toInt(20);
        if (maxProfiles < 1 || maxProfiles > 8 || maxScans < 1 || maxScans > 100)
        {
            finishError(QStringLiteral("invalid-capability-limits"), false);
            return;
        }
        m_maxProfiles = maxProfiles;
        m_maxScansPerProfile = maxScans;
        m_contractAvailable = true;
        start(IrlumeProcess::Operation::ListProfiles, Workflow::LoadingProfiles);
        break;
    }
    case IrlumeProcess::Operation::ListProfiles:
        if (!loadProfiles(event.data))
        {
            m_contractAvailable = false;
            finishError(QStringLiteral("invalid-profile-list"), false);
            return;
        }
        m_busy = false;
        m_workflow = Workflow::Idle;
        if (!m_statusAfterRefresh.isEmpty())
        {
            m_statusText = m_statusAfterRefresh;
            m_statusAfterRefresh.clear();
        }
        else
        {
            m_statusText = m_profiles.isEmpty() ? translate("No face profile is enrolled.")
                                                : translate("Face profiles are ready.");
        }
        Q_EMIT stateChanged();
        break;
    case IrlumeProcess::Operation::Enroll:
        beginEnrollmentVerification(event.data);
        break;
    case IrlumeProcess::Operation::AuthTest:
        completeAuthenticationTest(event.data);
        break;
    case IrlumeProcess::Operation::AddScan:
        if (!validateProfileMutation(event.data, false))
        {
            m_contractAvailable = false;
            finishError(QStringLiteral("unsafe-profile-mutation-result"), false);
            return;
        }
        m_statusAfterRefresh = translate("Appearance scan added.");
        finishSuccess(m_statusAfterRefresh);
        refresh();
        break;
    case IrlumeProcess::Operation::DeleteProfile:
        if (!validateProfileMutation(event.data, true))
        {
            m_contractAvailable = false;
            finishError(m_workflow == Workflow::CleaningUp ? QStringLiteral("cleanup-unconfirmed")
                                                           : QStringLiteral("unsafe-profile-mutation-result"),
                        false);
            return;
        }
        if (m_workflow == Workflow::CleaningUp)
        {
            const QString reason = m_cleanupReason;
            m_pendingNewProfileId.clear();
            finishError(reason, reason == QLatin1String("camera-busy"),
                        translate("Enrollment could not be verified, so the new profile was removed."));
        }
        else
        {
            m_statusAfterRefresh = translate("Face profile deleted.");
            finishSuccess(m_statusAfterRefresh);
            refresh();
        }
        break;
    }
}

void ProfileModel::handleOperationError(IrlumeProcess::Operation operation, const QString &code, bool retryable)
{
    if (!m_busy)
    {
        return;
    }
    if (operation == IrlumeProcess::Operation::AuthTest && !m_pendingNewProfileId.isEmpty())
    {
        cleanUpUnverifiedEnrollment(code);
        return;
    }
    if (operation == IrlumeProcess::Operation::Enroll)
    {
        m_contractAvailable = false;
        finishError(QStringLiteral("enrollment-state-unknown"), false,
                    translate("Enrollment ended without a confirmed terminal event. Refresh before trying again."));
        return;
    }
    if (operation == IrlumeProcess::Operation::Capabilities || operation == IrlumeProcess::Operation::ListProfiles ||
        (operation == IrlumeProcess::Operation::DeleteProfile && m_workflow == Workflow::CleaningUp))
    {
        m_contractAvailable = false;
    }
    finishError(code, retryable);
}

bool ProfileModel::start(IrlumeProcess::Operation operation, Workflow workflow, const QString &profileId)
{
    m_busy = true;
    m_activeProfileId = profileId;
    setWorkflow(workflow, {});
    const bool previewOperation = operation == IrlumeProcess::Operation::Enroll ||
                                  operation == IrlumeProcess::Operation::AuthTest ||
                                  operation == IrlumeProcess::Operation::AddScan;
    const bool started = previewOperation && m_enrollmentSession
                             ? m_enrollmentSession->startOperation(operation, profileId)
                             : m_process->startOperation(operation, profileId);
    if (started)
    {
        return true;
    }
    m_busy = false;
    finishError(QStringLiteral("operation-start-failed"), false);
    return false;
}

void ProfileModel::updateProgress(const QJsonObject &data)
{
    const int captured = data.value(QStringLiteral("captured_scans")).toInt(-1);
    const int total = data.value(QStringLiteral("total_scans")).toInt(-1);
    if (captured < 0 || total <= 0 || captured > total || total > m_maxScansPerProfile)
    {
        finishError(QStringLiteral("invalid-progress-event"), false);
        if (m_enrollmentSession && m_enrollmentSession->active())
        {
            m_enrollmentSession->cancel();
        }
        else
        {
            m_process->cancel();
        }
        return;
    }
    m_capturedScans = captured;
    m_totalScans = total;
    Q_EMIT stateChanged();
}

bool ProfileModel::loadProfiles(const QJsonObject &data)
{
    const QJsonValue profilesValue = data.value(QStringLiteral("profiles"));
    if (!profilesValue.isArray())
    {
        return false;
    }

    QVector<Profile> profiles;
    const QJsonArray array = profilesValue.toArray();
    if (array.size() > m_maxProfiles)
    {
        return false;
    }
    profiles.reserve(array.size());

    for (const QJsonValue &value : array)
    {
        const QJsonObject object = value.toObject();
        const QString id = object.value(QStringLiteral("profile_id")).toString();
        const QString displayName = object.value(QStringLiteral("display_name")).toString();
        const QJsonValue scansValue = object.value(QStringLiteral("scans"));
        if (!value.isObject() || !IrlumeProcess::isSafeOpaqueId(id) || !isSafeDisplayName(displayName) ||
            !scansValue.isArray() || scansValue.toArray().size() > 20)
        {
            return false;
        }
        profiles.push_back({id, displayName, static_cast<int>(scansValue.toArray().size())});
    }

    beginResetModel();
    m_profiles = std::move(profiles);
    endResetModel();
    Q_EMIT profilesChanged();
    return true;
}

bool ProfileModel::hasProfile(const QString &profileId) const
{
    return std::any_of(m_profiles.cbegin(), m_profiles.cend(),
                       [&profileId](const Profile &profile) { return profile.id == profileId; });
}

void ProfileModel::beginEnrollmentVerification(const QJsonObject &data)
{
    const QString profileId = data.value(QStringLiteral("profile_id")).toString();
    const bool created = data.value(QStringLiteral("created")).toBool(false);
    if (!created || !IrlumeProcess::isSafeOpaqueId(profileId))
    {
        finishError(QStringLiteral("invalid-enrollment-result"), false);
        return;
    }

    m_pendingNewProfileId = profileId;
    start(IrlumeProcess::Operation::AuthTest, Workflow::VerifyingEnrollment);
}

void ProfileModel::completeAuthenticationTest(const QJsonObject &data)
{
    if (data.value(QStringLiteral("profile_modified")).toBool(true) ||
        data.value(QStringLiteral("credential_released")).toBool(true) ||
        !data.value(QStringLiteral("matched")).isBool())
    {
        if (!m_pendingNewProfileId.isEmpty())
        {
            cleanUpUnverifiedEnrollment(QStringLiteral("unsafe-auth-test-result"));
        }
        else
        {
            finishError(QStringLiteral("unsafe-auth-test-result"), false);
        }
        return;
    }

    if (!data.value(QStringLiteral("matched")).toBool())
    {
        if (!m_pendingNewProfileId.isEmpty())
        {
            cleanUpUnverifiedEnrollment(QStringLiteral("recognition-not-matched"));
        }
        else
        {
            finishError(QStringLiteral("recognition-not-matched"), true);
        }
        return;
    }

    const bool verifiedEnrollment = !m_pendingNewProfileId.isEmpty();
    m_pendingNewProfileId.clear();
    const QString message = verifiedEnrollment ? translate("Enrollment completed and recognition was verified.")
                                               : translate("Recognition test passed.");
    if (verifiedEnrollment)
    {
        m_statusAfterRefresh = message;
    }
    finishSuccess(message);
    if (verifiedEnrollment)
    {
        refresh();
    }
}

void ProfileModel::cleanUpUnverifiedEnrollment(const QString &reasonCode)
{
    if (m_pendingNewProfileId.isEmpty())
    {
        finishError(reasonCode, false);
        return;
    }
    m_cleanupReason = reasonCode;
    start(IrlumeProcess::Operation::DeleteProfile, Workflow::CleaningUp, m_pendingNewProfileId);
}

void ProfileModel::finishSuccess(const QString &message)
{
    m_busy = false;
    m_workflow = Workflow::Idle;
    m_statusText = message;
    m_errorCode.clear();
    m_canRetry = false;
    m_stageLabel.clear();
    Q_EMIT stateChanged();
}

void ProfileModel::finishError(const QString &code, bool retryable, const QString &message)
{
    m_busy = false;
    m_workflow = Workflow::Idle;
    m_errorCode = code;
    m_canRetry = retryable || code == QLatin1String("camera-busy");
    m_statusText = message.isEmpty() ? messageForError(code) : message;
    m_stageLabel.clear();
    m_statusAfterRefresh.clear();
    Q_EMIT stateChanged();
}

bool ProfileModel::validateProfileMutation(const QJsonObject &data, bool deletion) const
{
    if (data.value(QStringLiteral("profile_id")).toString() != m_activeProfileId ||
        data.value(QStringLiteral("mutated_other_profiles")).toBool(true))
    {
        return false;
    }
    if (deletion)
    {
        return data.value(QStringLiteral("deleted")).toBool(false);
    }
    const int addedScans = data.value(QStringLiteral("added_scans")).toInt(0);
    return addedScans > 0 && addedScans <= 20;
}

void ProfileModel::setWorkflow(Workflow workflow, const QString &status)
{
    m_workflow = workflow;
    switch (workflow)
    {
    case Workflow::Idle:
        m_statusText = status;
        break;
    case Workflow::CheckingContract:
        m_statusText = translate("Checking the irlume integration contract…");
        break;
    case Workflow::LoadingProfiles:
        m_statusText = translate("Loading face profiles…");
        break;
    case Workflow::Enrolling:
        m_statusText = translate("Creating a face profile…");
        break;
    case Workflow::VerifyingEnrollment:
        m_statusText = translate("Testing the new profile before keeping it…");
        break;
    case Workflow::Testing:
        m_statusText = translate("Testing recognition without changing the profile…");
        break;
    case Workflow::AddingScan:
        m_statusText = translate("Adding an appearance scan…");
        break;
    case Workflow::Deleting:
        m_statusText = translate("Deleting the selected face profile…");
        break;
    case Workflow::CleaningUp:
        m_statusText = translate("Removing an unverified partial profile…");
        break;
    }
    Q_EMIT stateChanged();
}

void ProfileModel::resetTransientState()
{
    m_errorCode.clear();
    m_canRetry = false;
    m_stageLabel.clear();
    m_capturedScans = 0;
    m_totalScans = 0;
    m_cleanupReason.clear();
}

QString ProfileModel::labelForStage(const QString &stage) const
{
    if (stage == QLatin1String("camera-ready"))
    {
        return translate("Camera ready");
    }
    if (stage == QLatin1String("capture"))
    {
        return translate("Capturing appearance");
    }
    if (stage == QLatin1String("liveness"))
    {
        return translate("Checking liveness");
    }
    if (stage == QLatin1String("matching"))
    {
        return translate("Comparing with the selected profile");
    }
    if (stage == QLatin1String("cleanup"))
    {
        return translate("Cleaning up");
    }
    return translate("Working");
}

QString ProfileModel::messageForError(const QString &code) const
{
    if (code == QLatin1String("structured-contract-unavailable") || code == QLatin1String("invalid-json-document"))
    {
        return translate("This irlume release does not provide the structured profile-management contract.");
    }
    if (code == QLatin1String("engine-not-installed"))
    {
        return translate("irlume is not installed.");
    }
    if (code == QLatin1String("camera-busy"))
    {
        return translate("The camera is busy. Close other camera applications, then try again.");
    }
    if (code == QLatin1String("user-cancelled"))
    {
        return translate("The operation was cancelled and the camera was released.");
    }
    if (code == QLatin1String("recognition-not-matched"))
    {
        return translate("Recognition did not match. The test did not change the profile.");
    }
    if (code == QLatin1String("cancellation-unconfirmed"))
    {
        return translate("Camera release could not be confirmed. Refresh before trying again.");
    }
    if (code == QLatin1String("engine-timeout"))
    {
        return translate("irlume did not finish in time.");
    }
    if (code == QLatin1String("cleanup-unconfirmed"))
    {
        return translate("Removal of the unverified profile could not be confirmed. Refresh before continuing.");
    }
    return translate("The face profile operation could not be completed.");
}
