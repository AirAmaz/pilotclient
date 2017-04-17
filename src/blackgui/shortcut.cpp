/* Copyright (C) 2015
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 *
 * Class based on qLed: Copyright (C) 2010 by P. Sereno, http://www.sereno-online.com
 */

#include "blackgui/shortcut.h"

#include <QKeySequence>
#include <Qt>

namespace BlackGui
{
    const QKeySequence &CShortcut::keyStash()
    {
        static const QKeySequence k(Qt::ALT + Qt::Key_S);
        return k;
    }

    const QKeySequence &CShortcut::keyClearSelection()
    {
        static const QKeySequence k(Qt::CTRL + Qt::Key_Space);
        return k;
    }

    const QKeySequence &CShortcut::keySelectAll()
    {
        static const QKeySequence k(Qt::CTRL + Qt::Key_A);
        return k;
    }

    const QKeySequence &BlackGui::CShortcut::keyDisplayFilter()
    {
        static const QKeySequence k(Qt::CTRL + Qt::Key_F);
        return k;
    }

    const QKeySequence &CShortcut::keySave()
    {
        static const QKeySequence k(Qt::CTRL + Qt::Key_S);
        return k;
    }

    const QKeySequence &CShortcut::keySaveViews()
    {
        // remark CTRL+S not working in views
        static const QKeySequence k(Qt::SHIFT + Qt::Key_S);
        return k;
    }

    const QKeySequence &CShortcut::keyDelete()
    {
        static const QKeySequence k(Qt::Key_Delete);
        return k;
    }

    const QKeySequence &CShortcut::keyEscape()
    {
        static const QKeySequence k(Qt::Key_Escape);
        return k;
    }

    const QKeySequence &CShortcut::keyCopy()
    {
        static const QKeySequence k(Qt::CTRL + Qt::Key_C);
        return k;
    }
} // ns
