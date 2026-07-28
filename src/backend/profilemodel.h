// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "faceauthbackend.h"

#include <QAbstractListModel>
#include <QString>
#include <QVector>

class ProfileModel final : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(bool contractAvailable READ contractAvailable NOTIFY stateChanged)
    Q_PROPERTY(bool readOnlyAvailable READ readOnlyAvailable NOTIFY stateChanged)
    Q_PROPERTY(bool mutationSupported READ mutationSupported NOTIFY stateChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY stateChanged)
    Q_PROPERTY(ResultState availabilityState READ availabilityState NOTIFY stateChanged)
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

    [[nodiscard]] int rowCount(const QModelIndex &parent = {}) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    void applySnapshot(const EngineSnapshot &snapshot);
    [[nodiscard]] bool contractAvailable() const;
    [[nodiscard]] bool readOnlyAvailable() const;
    [[nodiscard]] bool mutationSupported() const;
    [[nodiscard]] bool busy() const;
    [[nodiscard]] ResultState availabilityState() const;
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
    void refreshRequested();

  private:
    struct Profile
    {
        QString id;
        QString displayName;
        QVector<QString> scanDisplayNames;
    };

    void failCapability();

    QVector<Profile> m_profiles;
    bool m_contractAvailable = false;
    bool m_readOnlyAvailable = false;
    bool m_mutationSupported = false;
    bool m_retryable = false;
    ResultState m_resultState = ResultState::NotAdvertised;
    int m_maxProfiles = 0;
    QString m_statusText;
    QString m_errorCode;
};
