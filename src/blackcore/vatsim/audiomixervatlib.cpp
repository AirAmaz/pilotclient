/* Copyright (C) 2014
 * swift project community / contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "blackcore/vatsim/audiomixervatlib.h"

namespace BlackCore
{
    namespace Vatsim
    {
        CAudioMixerVatlib::CAudioMixerVatlib(QObject *parent)
            : IAudioMixer(parent)
        {
            m_audioMixer.reset(Vat_CreateAudioMixer());
        }

        void CAudioMixerVatlib::makeMixerConnection(InputPort inputPort, OutputPort outputPort)
        {
            Vat_MakeMixerConnection(m_audioMixer.data(), inputPort, outputPort, true);
        }

        void CAudioMixerVatlib::removeMixerConnection(InputPort inputPort, OutputPort outputPort)
        {
            Vat_MakeMixerConnection(m_audioMixer.data(), inputPort, outputPort, false);
        }

        bool CAudioMixerVatlib::hasMixerConnection(InputPort inputPort, OutputPort outputPort)
        {
            return Vat_HasMixerConnection(m_audioMixer.data(), inputPort, outputPort);
        }
    } // ns
} // ns
