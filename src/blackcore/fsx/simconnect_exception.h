/* Copyright (C) 2013 VATSIM Community / contributors
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BLACKCORE_FSX_SIMCONNECT_EXCEPTION_H
#define BLACKCORE_FSX_SIMCONNECT_EXCEPTION_H

#include "simconnect/SimConnect.h"

namespace BlackCore
{
    namespace FSX
    {
        //! \brief Handles SimConnect exceptions
        class CSimConnectException
        {
        public:
            CSimConnectException();

            /*!
             * \brief Handle exception
             * \param exception
             */
            static void handleException(SIMCONNECT_EXCEPTION exception);
        };
    }
}

#endif // BLACKCORE_FSX_SIMCONNECT_EXCEPTION_H
