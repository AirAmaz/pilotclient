#include "avcallsign.h"

namespace BlackMisc
{
    namespace Aviation
    {
        /*
         * Convert to string
         */
        QString CCallsign::convertToQString(bool /** i18n **/) const
        {
            return this->m_callsign;
        }

        /*
         * Marshall to DBus
         */
        void CCallsign::marshallToDbus(QDBusArgument &argument) const
        {
            argument << this->m_callsignAsSet;
            argument << this->m_callsignPronounced;
        }

        /*
         * Unmarshall from DBus
         */
        void CCallsign::unmarshallFromDbus(const QDBusArgument &argument)
        {
            argument >> this->m_callsignAsSet;
            argument >> this->m_callsignPronounced;
            this->m_callsign = CCallsign::unifyCallsign(this->m_callsignAsSet);
        }

        /*
         * Unify the callsign
         */
        QString CCallsign::unifyCallsign(const QString &callsign)
        {
            QString unified = callsign.toUpper();
            unified = unified.remove(QRegExp("[^a-zA-Z\\d\\s]"));
            return unified;
        }

        /*
         * Callsign as Observer
         */
        QString CCallsign::getAsObserverCallsignString() const
        {
            if (this->isEmpty()) return "";
            QString obs = this->getStringAsSet();
            if (obs.endsWith("_OBS", Qt::CaseInsensitive)) return obs;
            if (obs.contains('_')) obs = obs.left(obs.lastIndexOf('_'));
            return obs.append("_OBS").toUpper();
        }

        /*
         * Equals callsign?
         */
        bool CCallsign::equalsString(const QString &callsignString) const
        {
            if (callsignString.isEmpty()) return false;
            if (this->isEmpty()) return false;
            if (callsignString == this->m_callsign || callsignString == this->m_callsignAsSet) return true;
            return false;
        }

        /*
         * Equal?
         */
        bool CCallsign::operator ==(const CCallsign &other) const
        {
            if (this == &other) return true;
            return other.asString() == this->asString();
        }

        /*
         * Unequal?
         */
        bool CCallsign::operator !=(const CCallsign &other) const
        {
            return !((*this) == other);
        }

        /*
         * Less than?
         */
        bool CCallsign::operator <(const CCallsign &other) const
        {
            return this->m_callsign < other.m_callsign;
        }

        /*
         * Hash
         */
        uint CCallsign::getValueHash() const
        {
            return qHash(this->m_callsign);
        }

        /*
         * Compare
         */
        int CCallsign::compare(const QVariant &qv) const
        {
            Q_ASSERT(qv.canConvert<CCallsign>());
            Q_ASSERT(!qv.isNull() && qv.isValid());
            return this->compare(qv.value<CCallsign>());
        }

        /*
         * Compare
         */
        int CCallsign::compare(const CCallsign &callsign) const
        {
            return this->m_callsign.compare(callsign.asString(), Qt::CaseInsensitive);
        }

        /*
         * Register metadata
         */
        void CCallsign::registerMetadata()
        {
            qRegisterMetaType<CCallsign>();
            qDBusRegisterMetaType<CCallsign>();
        }

    } // namespace
} // namespace
