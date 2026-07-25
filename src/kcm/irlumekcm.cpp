// SPDX-License-Identifier: GPL-3.0-or-later

#include "irlumekcm.h"

#include <KPluginFactory>

IrlumeKcm::IrlumeKcm(QObject *parent, const KPluginMetaData &data)
    : KQuickConfigModule(parent, data), m_systemState(this)
{
    setButtons(NoAdditionalButton);
    refresh();
}

SystemState *IrlumeKcm::systemState()
{
    return &m_systemState;
}

void IrlumeKcm::refresh()
{
    m_systemState.apply(m_probe.probe());
}

K_PLUGIN_CLASS_WITH_JSON(IrlumeKcm, "kcm_irlume.json")

#include "irlumekcm.moc"
