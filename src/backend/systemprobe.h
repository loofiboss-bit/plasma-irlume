// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

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
    bool irlumePresent = false;
    QString irlumeVersionOutput;
    QString irlumeStatusOutput;
    QString irlumeDoctorOutput;
    QString irlumeLoginStatusOutput;
};

class SystemProbe final
{
  public:
    [[nodiscard]] SystemStateSnapshot probe() const;
    [[nodiscard]] static SystemStateSnapshot evaluate(const SystemProbeInputs &inputs);
    [[nodiscard]] static QString parseOsReleaseValue(const QByteArray &contents, const QString &key);
};
