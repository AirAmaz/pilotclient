/* Copyright (C) 2013
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "blackmisc/input/keyboardkeylist.h"
#include "blackmisc/input/keycodes.h"

namespace BlackMisc
{
    namespace Input
    {
        CKeyboardKeyList::CKeyboardKeyList() { }

        CKeyboardKeyList::CKeyboardKeyList(CKeyboardKey key)
        {
            this->push_back(key);
        }

        CKeyboardKeyList::CKeyboardKeyList(const CSequence<CKeyboardKey> &baseClass) :
            CSequence<CKeyboardKey>(baseClass)
        { }

        const CKeyboardKeyList &CKeyboardKeyList::allSupportedKeys()
        {
            static const CKeyboardKeyList allKeys =
            {
                CKeyboardKey(Key_ControlLeft),
                CKeyboardKey(Key_ControlRight),
                CKeyboardKey(Key_AltLeft),
                CKeyboardKey(Key_AltRight),
                CKeyboardKey(Key_ShiftLeft),
                CKeyboardKey(Key_ShiftRight),
                CKeyboardKey(Key_Period),
                CKeyboardKey(Key_Plus),
                CKeyboardKey(Key_Minus),
                CKeyboardKey(Key_Comma),
                CKeyboardKey(Key_Multiply),
                CKeyboardKey(Key_Divide),
                CKeyboardKey(Key_A),
                CKeyboardKey(Key_B),
                CKeyboardKey(Key_C),
                CKeyboardKey(Key_D),
                CKeyboardKey(Key_E),
                CKeyboardKey(Key_F),
                CKeyboardKey(Key_G),
                CKeyboardKey(Key_H),
                CKeyboardKey(Key_I),
                CKeyboardKey(Key_J),
                CKeyboardKey(Key_K),
                CKeyboardKey(Key_L),
                CKeyboardKey(Key_M),
                CKeyboardKey(Key_N),
                CKeyboardKey(Key_O),
                CKeyboardKey(Key_P),
                CKeyboardKey(Key_Q),
                CKeyboardKey(Key_R),
                CKeyboardKey(Key_S),
                CKeyboardKey(Key_T),
                CKeyboardKey(Key_U),
                CKeyboardKey(Key_V),
                CKeyboardKey(Key_W),
                CKeyboardKey(Key_X),
                CKeyboardKey(Key_Y),
                CKeyboardKey(Key_Z),
                CKeyboardKey(Key_0),
                CKeyboardKey(Key_1),
                CKeyboardKey(Key_2),
                CKeyboardKey(Key_3),
                CKeyboardKey(Key_4),
                CKeyboardKey(Key_5),
                CKeyboardKey(Key_6),
                CKeyboardKey(Key_7),
                CKeyboardKey(Key_8),
                CKeyboardKey(Key_9),
                CKeyboardKey(Key_Numpad0),
                CKeyboardKey(Key_Numpad1),
                CKeyboardKey(Key_Numpad2),
                CKeyboardKey(Key_Numpad3),
                CKeyboardKey(Key_Numpad4),
                CKeyboardKey(Key_Numpad5),
                CKeyboardKey(Key_Numpad6),
                CKeyboardKey(Key_Numpad7),
                CKeyboardKey(Key_Numpad8),
                CKeyboardKey(Key_Numpad9)
            };

            return allKeys;
        }
    } // ns
} // ns
