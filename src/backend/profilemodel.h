// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "irlumeprocess.h"

#include <QAbstractListModel>
#include <QString>
#include <QStringList>
#include <QVector>

class EnrollmentSession;

class ProfileModel final : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(bool contractAvailable READ contractAvailable NOTIFY stateChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY stateChanged)
    Q_PROPERTY(int profileCount READ profileCount NOTIFY profilesChanged)
    Q_PROPERTY(Workflow workflow READ workflow NOTIFY stateChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY stateChanged)
    Q_PROPERTY(QString stageLabel READ stageLabel NOTIFY stateChanged)
    Q_PROPERTY(int capturedScans READ capturedScans NOTIFY stateChanged)
    Q_PROPERTY(int totalScans READ totalScans NOTIFY stateChanged)
    Q_PROPERTY(QString errorCode READ errorCode NOTIFY stateChanged)
    Q_PROPERTY(bool canRetry READ canRetry NOTIFY stateChanged)
    Q_PROPERTY(bool cancellable READ cancellable NOTIFY stateChanged)
    Q_PROPERTY(int maxProfiles READ maxProfiles NOTIFY stateChanged)
    Q_PROPERTY(bool mergeConfirmationRequired READ mergeConfirmationRequired NOTIFY stateChanged)
    Q_PROPERTY(QString pendingMergeProfileName READ pendingMergeProfileName NOTIFY stateChanged)
    Q_PROPERTY(int pendingMergeScanCount READ pendingMergeScanCount NOTIFY stateChanged)

  public:
    enum Role
    {
        ProfileIdRole = Qt::UserRole + 1,
        DisplayNameRole,
        ScanCountRole,
        ScansRole,
    };

    enum class Workflow
    {
        Idle,
        CheckingContract,
        LoadingProfiles,
        Enrolling,
        VerifyingEnrollment,
        Testing,
        AddingScan,
        Deleting,
        DeletingScan,
        Renaming,
        AwaitingMergeConfirmation,
        CleaningUp,
    };
    Q_ENUM(Workflow)

    explicit ProfileModel(QObject *parent = nullptr);
    ProfileModel(IrlumeProcess *process, QObject *parent);
    ProfileModel(IrlumeProcess *process, EnrollmentSession *enrollmentSession, QObject *parent);

    [[nodiscard]] int rowCount(const QModelIndex &parent = {}) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    [[nodiscard]] bool contractAvailable() const;
    [[nodiscard]] bool busy() const;
    [[nodiscard]] int profileCount() const;
    [[nodiscard]] Workflow workflow() const;
    [[nodiscard]] QString statusText() const;
    [[nodiscard]] QString stageLabel() const;
    [[nodiscard]] int capturedScans() const;
    [[nodiscard]] int totalScans() const;
    [[nodiscard]] QString errorCode() const;
    [[nodiscard]] bool canRetry() const;
    [[nodiscard]] bool cancellable() const;
    [[nodiscard]] int maxProfiles() const;
    [[nodiscard]] bool mergeConfirmationRequired() const;
    [[nodiscard]] QString pendingMergeProfileName() const;
    [[nodiscard]] int pendingMergeScanCount() const;

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void enroll();
    Q_INVOKABLE void testRecognition(const QString &profileId);
    Q_INVOKABLE void addAppearanceScan(const QString &profileId);
    Q_INVOKABLE void deleteProfile(const QString &profileId);
    Q_INVOKABLE void deleteScan(const QString &profileId, const QString &scanId);
    Q_INVOKABLE void renameProfile(const QString &profileId, const QString &newName);
    Q_INVOKABLE void renameScan(const QString &profileId, const QString &scanId, const QString &newName);
    Q_INVOKABLE void confirmIdentityMerge(bool keepAddedScans);
    Q_INVOKABLE void cancel();
    Q_INVOKABLE void retry();

  Q_SIGNALS:
    void stateChanged();
    void profilesChanged();

  private:
    struct Scan
    {
        QString id;
        QString displayName;
    };

    struct Profile
    {
        QString id;
        QString displayName;
        int scanCount = 0;
        QVector<Scan> scans;
    };

    enum class RequestedAction
    {
        None,
        Enroll,
        Test,
        AddScan,
    };

    void handleEvent(const IrlumeProcess::Event &event);
    void handleOperationError(IrlumeProcess::Operation operation, const QString &code, bool retryable);
    bool start(IrlumeProcess::Operation operation, Workflow workflow, const QString &profileId = {},
               const QString &scanId = {}, const QString &newName = {});
    void updateProgress(const QJsonObject &data);
    bool loadProfiles(const QJsonObject &data);
    bool hasProfile(const QString &profileId) const;
    bool hasScan(const QString &profileId, const QString &scanId) const;
    void beginEnrollmentVerification(const QJsonObject &data);
    void completeAuthenticationTest(const QJsonObject &data);
    void cleanUpUnverifiedEnrollment(const QString &reasonCode);
    void startNextCleanup();
    void completeCleanup();
    void finishSuccess(const QString &message);
    void finishError(const QString &code, bool retryable, const QString &message = {});
    bool validateProfileMutation(const QJsonObject &data, bool deletion) const;
    void setWorkflow(Workflow workflow, const QString &status);
    void resetTransientState();
    [[nodiscard]] QString labelForStage(const QString &stage) const;
    [[nodiscard]] QString messageForError(const QString &code) const;

    IrlumeProcess *m_process = nullptr;
    EnrollmentSession *m_enrollmentSession = nullptr;
    QVector<Profile> m_profiles;
    bool m_contractAvailable = false;
    bool m_busy = false;
    Workflow m_workflow = Workflow::Idle;
    QString m_statusText;
    QString m_stageLabel;
    int m_capturedScans = 0;
    int m_totalScans = 0;
    QString m_errorCode;
    bool m_canRetry = false;
    int m_maxProfiles = 3;
    int m_maxScansPerProfile = 20;
    RequestedAction m_lastAction = RequestedAction::None;
    QString m_lastProfileId;
    QString m_activeProfileId;
    QString m_activeScanId;
    QString m_activeNewName;
    QString m_pendingNewProfileId;
    QString m_pendingMergedProfileId;
    QString m_pendingMergeProfileName;
    QStringList m_pendingMergeScanIds;
    QStringList m_cleanupScanIds;
    bool m_mergeConfirmationRequired = false;
    QString m_cleanupReason;
    QString m_statusAfterRefresh;
};
