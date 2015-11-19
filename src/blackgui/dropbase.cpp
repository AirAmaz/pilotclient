/* Copyright (C) 2015
 * swift project Community / Contributors
 *
 * This file is part of swift Project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "dropbase.h"
#include "guiutility.h"

using namespace BlackMisc;

namespace BlackGui
{
    CDropBase::CDropBase()
    { }

    void CDropBase::setAcceptedMetaTypeIds(const QList<int> &ids)
    {
        m_acceptedMetaTypes = ids;
    }

    void CDropBase::addAcceptedMetaTypeId(int id)
    {
        m_acceptedMetaTypes.append(id);
    }

    bool CDropBase::isDropAllowed() const
    {
        return m_allowDrop;
    }

    void CDropBase::allowDrop(bool allowed)
    {
        this->m_allowDrop = allowed;
    }

    bool CDropBase::acceptDrop(const QMimeData *mime) const
    {
        Q_ASSERT_X(!this->m_acceptedMetaTypes.isEmpty(), Q_FUNC_INFO, "no accepted meta type ids");
        if (m_acceptedMetaTypes.isEmpty()) { return false; }
        if (!m_allowDrop || !CGuiUtility::hasSwiftVariantMimeType(mime)) { return false; }
        int metaTypeId = CGuiUtility::metaTypeIdFromSwiftDragAndDropData(mime);
        if (metaTypeId == QMetaType::UnknownType) { return false; }
        bool accept =  m_acceptedMetaTypes.contains(metaTypeId);
        return accept;
    }

    CVariant CDropBase::toCVariant(const QMimeData *mime) const
    {
        return CGuiUtility::fromSwiftDragAndDropData(mime);
    }

} // ns
