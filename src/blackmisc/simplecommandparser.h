/* Copyright (C) 2013
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKMISC_SIMPLECOMMANDPARSER_H
#define BLACKMISC_SIMPLECOMMANDPARSER_H

#include "blackmisc/blackmiscexport.h"
#include <QString>
#include <QStringList>

namespace BlackMisc
{
    //! \defgroup swiftdotcommands dot commands

    //! Utility methods for simple line parsing used with the command line
    //! \remarks case insensitive parsing, commands start with . as ".msg"
    class BLACKMISC_EXPORT CSimpleCommandParser
    {
    public:
        //! Constructor
        CSimpleCommandParser(const QStringList &knownCommands);

        //! Known command
        bool isKnownCommand() const { return m_knownCommand; }

        //! Parse
        void parse(const QString &commandLine);

        //! Is given command current command of command line
        bool matchesCommand(const QString &checkCommand, const QString &alias1 = "", const QString &alias2 = "");

        //! Command starting with pattern?
        bool commandStartsWith(const QString &startPattern) const;

        //! Command ending with pattern?
        bool commandEndsWith(const QString &endPattern) const;

        //! Get part, 0 is command itself
        const QString &part(int index) const;

        //! Remaining part after
        QString remainingStringAfter(int index) const;

        //! Count parts
        int countParts() const;

        //! Existing part
        bool hasPart(int index) const;

        //! Count parts, command excluded
        int countPartsWithoutCommand() const;

        //! Is part an integer?
        bool isInt(int index) const;

        //! Is part a double?
        bool isDouble(int index) const;

        //! Part as integer
        int toInt(int index, int def = -1) const;

        //! Part as bool
        bool toBool(int index, bool def = false) const;

        //! Part as double
        double toDouble(int index, double def = -1.0) const;

        //! Matches given part
        bool matchesPart(int index, const QString &toMatch, Qt::CaseSensitivity cs = Qt::CaseInsensitive) const;

    private:
        QString m_originalLine;      //!< line as entered by user
        QString m_cleanedLine;       //!< trimmed, no double spaces etc.
        QString m_commandPart;       //!< command part (e.g. ".msg", if any)
        QStringList m_splitParts;    //!< split parts (split by " ")
        QStringList m_knownCommands; //!< known / handled commands
        bool m_knownCommand = false; //!< known command

        //! Avoid wrong usage
        void setCheckedCommandList(const QStringList &commands);

        //! Remove leading dot: ".msg" -> "msg"
        static QString removeLeadingDot(const QString &candidate);

        //! Clean up a command string
        static QString formatCommand(const QString &command);

        //! Command, starts with dot
        static bool isCommand(const QString &candidate);
    };
}

#endif // guard
