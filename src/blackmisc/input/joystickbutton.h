/* Copyright (C) 2015
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKMISC_INPUT_JOYSTICKBUTTON_H
#define BLACKMISC_INPUT_JOYSTICKBUTTON_H

#include "blackmisc/blackmiscexport.h"
#include "blackmisc/valueobject.h"
#include "blackmisc/propertyindex.h"

namespace BlackMisc
{
    namespace Input
    {
        //! Value object representing a joystick button
        class BLACKMISC_EXPORT CJoystickButton : public CValueObject<CJoystickButton>
        {
        public:
            //! Properties by index
            enum ColumnIndex
            {
                IndexButton = 0,
                IndexButtonAsString,
                IndeButtonObject, // just for updates
            };

            //! Default constructor
            CJoystickButton() = default;

            //! Constructor
            CJoystickButton(int buttonIndex);

            //! Get button index
            int getButtonIndex() const { return m_buttonIndex; }

            //! Get button as String
            QString getButtonAsString() const { return buttonIndexToString(m_buttonIndex); }

            //! Set button index
            void setButtonIndex(int buttonIndex);

            //! Is valid?
            bool isValid() const { return m_buttonIndex >= 0 ? true : false; }

            //! Set button object
            void setButtonObject(const CJoystickButton &button);

            //! \copydoc BlackMisc::Mixin::Index::setPropertyByIndex
            void setPropertyByIndex(const CVariant &variant, const BlackMisc::CPropertyIndex &index);

            //! \copydoc BlackMisc::Mixin::Index::propertyByIndex
            CVariant propertyByIndex(const CPropertyIndex &index) const;

            //! Button index to string
            static QString buttonIndexToString(qint32 buttonIndex);

            //! Button index from string
            static int buttonIndexFromString(const QString &button);

            //! Invalid button index
            static int getInvalidIndex() { return m_invalidIndex; }

            //! \copydoc BlackMisc::Mixin::String::toQString
            QString convertToQString(bool i18n = false) const;

        private:
            int m_buttonIndex = m_invalidIndex;

            static constexpr int m_invalidIndex = -1;

            BLACK_METACLASS(
                CJoystickButton,
                BLACK_METAMEMBER(buttonIndex)
            );
        };
    }
}

Q_DECLARE_METATYPE(BlackMisc::Input::CJoystickButton)

#endif // guard
