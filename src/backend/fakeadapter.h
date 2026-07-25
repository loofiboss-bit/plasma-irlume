// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "systemstate.h"

#include <QStringList>

class SystemStateAdapter
{
  public:
    virtual ~SystemStateAdapter() = default;

    [[nodiscard]] virtual QStringList scenarioNames() const = 0;
    [[nodiscard]] virtual SystemStateSnapshot stateForScenario(int index) const = 0;
};

class FakeSystemStateAdapter final : public SystemStateAdapter
{
  public:
    enum Scenario
    {
        SecureIr,
        RgbOnly,
        NoCamera,
        MissingIrlume,
        UnsupportedIrlume,
        BrokenDaemon,
        PamDrift,
        ScenarioCount,
    };

    [[nodiscard]] QStringList scenarioNames() const override;
    [[nodiscard]] SystemStateSnapshot stateForScenario(int index) const override;
};
