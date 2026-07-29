// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "faceauthbackend.h"
#include "systemstate.h"

#include <QByteArray>
#include <QObject>
#include <QString>

struct SystemProbeInputs
{
    QByteArray osRelease;
    QString plasmaVersion;
    QString displayManagerTarget;
    QByteArray secureBootVariable;
    bool secureBootVariablePresent = false;
    EngineSnapshot engine;
};

class SystemProbe final : public QObject
{
    Q_OBJECT

  public:
    explicit SystemProbe(QObject *parent = nullptr);
    void requestProbe(quint64 generation, const EngineSnapshot &engine);
    [[nodiscard]] SystemStateSnapshot probe(const EngineSnapshot &engine) const;
    [[nodiscard]] static SystemStateSnapshot evaluate(const SystemProbeInputs &inputs);
    [[nodiscard]] static QString parseOsReleaseValue(const QByteArray &contents, const QString &key);

  Q_SIGNALS:
    void probeCompleted(quint64 generation, const SystemStateSnapshot &snapshot);

  private:
    quint64 m_latestGeneration = 0;
};
