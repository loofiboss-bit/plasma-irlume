// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "systemprobe.h"
#include "systemstate.h"

#include <KQuickConfigModule>

class IrlumeKcm final : public KQuickConfigModule
{
    Q_OBJECT

    Q_PROPERTY(SystemState *systemState READ systemState CONSTANT)

  public:
    IrlumeKcm(QObject *parent, const KPluginMetaData &data);

    [[nodiscard]] SystemState *systemState();
    Q_INVOKABLE void refresh();

  private:
    SystemProbe m_probe;
    SystemState m_systemState;
};
