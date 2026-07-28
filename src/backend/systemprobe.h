// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "faceauthbackend.h"
#include "systemstate.h"

#include <QByteArray>
#include <QString>

struct SystemProbeInputs
{
    QByteArray osRelease;
    QString plasmaVersion;
    QString displayManagerTarget;
    QByteArray secureBootVariable;
    bool secureBootVariablePresent = false;
    bool tpmPresent = false;
    EngineSnapshot engine;
};

class SystemProbe final
{
  public:
    [[nodiscard]] SystemStateSnapshot probe(const EngineSnapshot &engine) const;
    [[nodiscard]] static SystemStateSnapshot evaluate(const SystemProbeInputs &inputs);
    [[nodiscard]] static QString parseOsReleaseValue(const QByteArray &contents, const QString &key);
};
