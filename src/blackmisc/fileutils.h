/* Copyright (C) 2015
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKMISC_CFILEUTILS_H
#define BLACKMISC_CFILEUTILS_H

#include "blackmisc/blackmiscexport.h"

#include <QDir>
#include <QFileInfo>
#include <QFileInfoList>
#include <QLockFile>
#include <QString>
#include <QStringList>
#include <Qt>
#include <functional>

class QDateTime;

namespace BlackMisc
{
    /*!
     * Utility class for file operations
     */
    class BLACKMISC_EXPORT CFileUtils
    {
    public:
        //! Our JSON file name appendix
        static const QString &jsonAppendix();

        //! JSON wildcard + appendix
        static const QString &jsonWildcardAppendix();

        //! Write string to text file
        static bool writeStringToFile(const QString &content, const QString &fileNameAndPath);

        //! Write string to file, with a lock so two applications can't access at the same time
        static bool writeStringToLockedFile(const QString &content, const QString &fileNameAndPath);

        //! Read file into string
        static QString readFileToString(const QString &fileNameAndPath);

        //! Read file into string, with a lock so two applications can't access at the same time
        static QString readLockedFileToString(const QString &fileNameAndPath);

        //! Read file into string
        static QString readFileToString(const QString &filePath, const QString &fileName);

        //! Read file into string, with a lock so two applications can't access at the same time
        static QString readLockedFileToString(const QString &filePath, const QString &fileName);

        //! Write string to text file in background
        static bool writeStringToFileInBackground(const QString &content, const QString &fileNameAndPath);

        //! Write byte array to file
        static bool writeByteArrayToFile(const QByteArray &data, const QString &fileNameAndPath);

        //! Append file paths
        //! \sa CNetworkUtils::buildUrl for URLs
        static QString appendFilePaths(const QString &path1, const QString &path2);

        //! Strip file from path a/b/c.json a/b
        static QString stripFileFromPath(const QString &path);

        //! Append file paths
        //! \sa CNetworkUtils::buildUrl for URLs
        static QString appendFilePaths(const QString &path1, const QString &path2, const QString &path3);

        //! If `sourceDir` is a directory, copies it recursively, so that `sourceDir` becomes `destinationDir`.
        //! If it is a file, just copies the file.
        static bool copyRecursively(const QString &sourceDir, const QString &destinationDir);

        //! Normalize file path to Qt standard, e.g by turning \ to /
        static QString normalizeFilePathToQtStandard(const QString &filePath);

        //! Make directory paths relative to root directory
        //! \remark unlike QDir::relativePath here reltive paths are only created when a directory is a subdir of rootDirectory
        static QStringList makeDirectoriesRelative(const QStringList &directories, const QString &rootDirectory, Qt::CaseSensitivity cs = osFileNameCaseSensitivity());

        //! Case sensitivity for current OS
        static Qt::CaseSensitivity osFileNameCaseSensitivity();

        //! Is directory path matching the exclude path?
        static bool matchesExcludeDirectory(const QString &directoryPath, const QString &excludePattern, Qt::CaseSensitivity cs = osFileNameCaseSensitivity());

        //! Directory to be excluded?
        static bool isExcludedDirectory(const QDir &directory, const QStringList &excludeDirectories, Qt::CaseSensitivity cs = osFileNameCaseSensitivity());

        //! Directory to be excluded?
        static bool isExcludedDirectory(const QFileInfo &fileInfo, const QStringList &excludeDirectories, Qt::CaseSensitivity cs = osFileNameCaseSensitivity());

        //! Directory to be excluded?
        static bool isExcludedDirectory(const QString &directoryPath, const QStringList &excludeDirectories, Qt::CaseSensitivity cs = osFileNameCaseSensitivity());

        //! Removes sub directories in list: A/B A/B/C B B/D -> A/B B returned
        static QStringList removeSubDirectories(const QStringList &directories, Qt::CaseSensitivity cs = osFileNameCaseSensitivity());

        //! Find first existing file or directory (means exists on file system)
        static QString findFirstExisting(const QStringList &filesOrDirectory);

        //! Returns path to first file in dir which matches the optional wildcard and predicate, or empty string.
        static QString findFirstFile(const QDir &dir, bool recursive, const QStringList &nameFilters = {}, const QStringList &excludeDirectories = {}, std::function<bool(const QFileInfo &)> predicate = {});

        //! True if there exists a file in dir which matches the optional wildcard and predicate.
        static bool containsFile(const QDir &dir, bool recursive, const QStringList &nameFilters = {}, const QStringList &excludeDirectories = {}, std::function<bool(const QFileInfo &)> predicate = {});

        //! Returns path to first file in dir newer than the given time, optionally matching a wildcard, or empty string.
        static QString findFirstNewerThan(const QDateTime &time, const QDir &dir, bool recursive, const QStringList &nameFilters = {}, const QStringList &excludeDirectories = {});

        //! True if there exists a file in dir newer than the given time, optionally matching a wildcard.
        static bool containsFileNewerThan(const QDateTime &time, const QDir &dir, bool recursive, const QStringList &nameFilters = {}, const QStringList &excludeDirectories = {});

        //! Returns list of all files in dir, optionally matching a wildcard and predicate.
        static QFileInfoList enumerateFiles(const QDir &dir, bool recursive, const QStringList &nameFilters = {}, const QStringList &excludeDirectories = {}, std::function<bool(const QFileInfo &)> predicate = {});

        //! Returns path to the newest file in dir, optionally matching a wildcard, or empty string.
        static QString findNewestFile(const QDir &dir, bool recursive, const QStringList &nameFilters = {}, const QStringList &excludeDirectories = {});

        //! Get all swift executables
        static const QStringList &getSwiftExecutables();

        //! Error message explaining why a QLockFile failed to lock.
        static QString lockFileError(const QLockFile &lockFile);

        //! UNC file paths on Qt start with "/", but UNC file paths only work when they start with "//"
        //! \remark On Windows starting with "/" means an UNC path, on UNIX it varies, see http://unix.stackexchange.com/a/12291/19428
        static QString fixWindowsUncPath(const QString &filePath);

        //! Fix UNC file paths
        static QStringList fixWindowsUncPaths(const QStringList &filePaths);

        //! Human readable (GB, MB, ..) file size
        static QString humanReadableFileSize(qint64 size);
    };
} // ns

#endif // guard
