/* Copyright (C) 2014
 * swift project Community / Contributors
 *
 * This file is part of swift Project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "enableforviewbasedindicator.h"
#include "blackgui/views/viewbase.h"

using namespace BlackMisc;
using namespace BlackGui;
using namespace BlackGui::Views;

namespace BlackGui
{
    void CEnableForViewBasedIndicator::enableLoadIndicator(bool enable)
    {
        Q_ASSERT_X(m_viewWithIndicator, Q_FUNC_INFO, "Missing view to display indicator");
        if (m_viewWithIndicator) { m_viewWithIndicator->enableLoadIndicator(enable); }
    }

    bool CEnableForViewBasedIndicator::isShowingLoadIndicator() const
    {
        Q_ASSERT_X(m_viewWithIndicator, Q_FUNC_INFO, "Missing view to display indicator");
        if (m_viewWithIndicator) { return m_viewWithIndicator->isShowingLoadIndicator(); }
        return false;
    }

    void CEnableForViewBasedIndicator::showLoadIndicator()
    {
        Q_ASSERT_X(m_viewWithIndicator, Q_FUNC_INFO, "Missing view to display indicator");
        if (m_viewWithIndicator) { m_viewWithIndicator->showLoadIndicator(); }
    }

    void CEnableForViewBasedIndicator::hideLoadIndicator()
    {
        Q_ASSERT_X(m_viewWithIndicator, Q_FUNC_INFO, "Missing view to display indicator");
        if (m_viewWithIndicator) { m_viewWithIndicator->hideLoadIndicator(); }
    }

    CEnableForViewBasedIndicator::CEnableForViewBasedIndicator(BlackGui::Views::CViewBaseNonTemplate *viewWithIndicator)
        : m_viewWithIndicator(viewWithIndicator)
    {
        // void
    }

    void CEnableForViewBasedIndicator::setViewWithIndicator(BlackGui::Views::CViewBaseNonTemplate *viewWithIndicator)
    {
        this->m_viewWithIndicator = viewWithIndicator;
    }

} // namespace
