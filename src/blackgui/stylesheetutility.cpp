/* Copyright (C) 2013
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "blackconfig/buildconfig.h"
#include "blackgui/stylesheetutility.h"
#include "blackmisc/fileutils.h"
#include "blackmisc/restricted.h"

#include <QAbstractScrollArea>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFileInfoList>
#include <QFlags>
#include <QFont>
#include <QIODevice>
#include <QRegularExpression>
#include <QStyleOption>
#include <QStylePainter>
#include <QTextStream>
#include <QWidget>
#include <QtGlobal>

using namespace BlackConfig;
using namespace BlackMisc;

namespace BlackGui
{
    CStyleSheetUtility::CStyleSheetUtility(BlackMisc::Restricted<CGuiApplication>, QObject *parent) : QObject(parent)
    {
        this->read();
        connect(&this->m_fileWatcher, &QFileSystemWatcher::directoryChanged, this, &CStyleSheetUtility::ps_qssDirectoryChanged);
        connect(&this->m_fileWatcher, &QFileSystemWatcher::fileChanged, this, &CStyleSheetUtility::ps_qssDirectoryChanged);
    }

    const QString &CStyleSheetUtility::fontStyleAsString(const QFont &font)
    {
        static const QString n("normal");
        static const QString i("italic");
        static const QString o("oblique");
        static const QString e;

        switch (font.style())
        {
        case QFont::StyleNormal: return n;
        case QFont::StyleItalic: return i;
        case QFont::StyleOblique: return o;
        default: return e;
        }
    }

    const QString &CStyleSheetUtility::fontWeightAsString(const QFont &font)
    {
        if (font.weight() < static_cast<int>(QFont::Normal))
        {
            static const QString l("light");
            return l;
        }
        else if (font.weight() < static_cast<int>(QFont::DemiBold))
        {
            static const QString n("normal");
            return n;
        }
        else if (font.weight() < static_cast<int>(QFont::Bold))
        {
            static const QString d("demibold");
            return d;
        }
        else if (font.weight() < static_cast<int>(QFont::Black))
        {
            static const QString b("bold");
            return b;
        }
        else
        {
            static const QString b("black");
            return b;
        }
    }

    QString CStyleSheetUtility::fontAsCombinedWeightStyle(const QFont &font)
    {
        QString w = fontWeightAsString(font);
        QString s = fontStyleAsString(font);
        if (w == s) return w; // avoid "normal" "normal"
        if (w.isEmpty() && s.isEmpty()) return "normal";
        if (w.isEmpty()) return s;
        if (s.isEmpty()) return w;
        if (s == "normal") return w;
        return w.append(" ").append(s);
    }

    QString CStyleSheetUtility::fontColor() const
    {
        const QString s = this->style(fileNameFonts()).toLower();
        if (!s.contains("color:")) return "red";
        thread_local const QRegularExpression rx("color:\\s*(#*\\w+);");
        const QString c = rx.match(s).captured(1);
        return c.isEmpty() ? "red" : c;
    }

    bool CStyleSheetUtility::read()
    {
        QDir directory(CBuildConfig::getStylesheetsDir());
        if (!directory.exists()) { return false; }

        // qss/css files
        const bool needsWatcher = this->m_fileWatcher.files().isEmpty();
        if (needsWatcher) { this->m_fileWatcher.addPath(CBuildConfig::getStylesheetsDir()); } // directory to deleted file watching
        directory.setNameFilters({"*.qss", "*.css"});
        directory.setFilter(QDir::Files | QDir::Hidden | QDir::NoSymLinks);

        QMap<QString, QString> newStyleSheets;
        const QFileInfoList fileInfoList = directory.entryInfoList();
        for (const QFileInfo &fileInfo : fileInfoList)
        {
            const QString absolutePath = fileInfo.absoluteFilePath();
            QFile file(absolutePath);
            if (file.open(QFile::QIODevice::ReadOnly | QIODevice::Text))
            {
                if (needsWatcher) { this->m_fileWatcher.addPath(absolutePath); }
                QTextStream in(&file);
                const QString c = in.readAll();
                const QString f = fileInfo.fileName().toLower();

                // keep even empty files as placeholders
                newStyleSheets.insert(f, c);
            }
            file.close();
        }

        // ignore redundant re-reads
        if (newStyleSheets != this->m_styleSheets)
        {
            this->m_styleSheets = newStyleSheets;
            emit this->styleSheetsChanged();
        }
        return true;
    }

    QString CStyleSheetUtility::style(const QString &fileName) const
    {
        if (!this->containsStyle(fileName)) return QString();
        return this->m_styleSheets[fileName.toLower()].trimmed();
    }

    QString CStyleSheetUtility::styles(const QStringList &fileNames) const
    {
        const bool hasModifiedFont = this->containsStyle(fileNameFontsModified());
        bool fontAdded = false;

        QString style;
        for (const QString &fileName : fileNames)
        {
            const QString key = fileName.toLower().trimmed();
            if (!this->containsStyle(key)) { continue; }

            QString s;
            if (fileName == fileNameFonts() || fileName == fileNameFontsModified())
            {
                if (fontAdded) { continue; }
                fontAdded = true;
                s = hasModifiedFont ?
                    this->m_styleSheets[fileNameFontsModified().toLower()] :
                    this->m_styleSheets[fileNameFonts()];
            }
            else
            {
                s = this->m_styleSheets[key];
            }
            if (s.isEmpty()) continue;
            if (!style.isEmpty()) style.append("\n\n");
            style.append("/** file: ").append(fileName).append(" **/\n");
            style.append(s);
        }
        return style;
    }

    bool CStyleSheetUtility::containsStyle(const QString &fileName) const
    {
        if (fileName.isEmpty()) return false;
        return this->m_styleSheets.contains(fileName.toLower().trimmed());
    }

    bool CStyleSheetUtility::updateFont(const QFont &font)
    {
        QString fs;
        if (font.pixelSize() >= 0)
        {
            fs.append(font.pixelSize()).append("px");
        }
        else
        {
            fs.append(QString::number(font.pointSizeF())).append("pt");
        }
        return updateFont(font.family(), fs, fontStyleAsString(font), fontWeightAsString(font), "white");
    }

    bool CStyleSheetUtility::updateFont(const QString &fontFamily, const QString &fontSize, const QString &fontStyle, const QString &fontWeight, const QString &fontColor)
    {
        static const QString indent("     ");
        QString fontStyleSheet;
        fontStyleSheet.append(indent).append("font-family: \"").append(fontFamily).append("\";\n");
        fontStyleSheet.append(indent).append("font-size: ").append(fontSize).append(";\n");
        fontStyleSheet.append(indent).append("font-style: ").append(fontStyle).append(";\n");
        fontStyleSheet.append(indent).append("font-weight: ").append(fontWeight).append(";\n");
        fontStyleSheet.append(indent).append("color: ").append(fontColor).append(";\n");

        QString qss("QWidget {\n");
        qss.append(fontStyleSheet);
        qss.append("}\n");

        QFile fontFile(CBuildConfig::getStylesheetsDir() + "/" + fileNameFontsModified());
        bool ok = fontFile.open(QFile::Text | QFile::WriteOnly);
        if (ok)
        {
            QTextStream out(&fontFile);
            out << qss;
            fontFile.close();
            ok = this->read();
        }
        return ok;
    }

    bool CStyleSheetUtility::resetFont()
    {
        QFile fontFile(CBuildConfig::getStylesheetsDir() + "/" + fileNameFontsModified());
        return fontFile.remove();
    }

    QString CStyleSheetUtility::fontStyle(const QString &combinedStyleAndWeight)
    {
        static const QString n("normal");
        QString c = combinedStyleAndWeight.toLower();
        for (const QString &s : fontStyles())
        {
            if (c.contains(s))
            {
                return s;
            }
        }
        return n;
    }

    QString CStyleSheetUtility::fontWeight(const QString &combinedStyleAndWeight)
    {
        static const QString n("normal");
        QString c = combinedStyleAndWeight.toLower();
        for (const QString &w : fontWeights())
        {
            if (c.contains(w))
            {
                return w;
            }
        }
        return n;
    }

    const QString &CStyleSheetUtility::fileNameFonts()
    {
        static const QString f(getQssFileName("fonts"));
        return f;
    }

    const QString &CStyleSheetUtility::fileNameFontsModified()
    {
        static const QString f("fonts.modified.qss");
        return f;
    }

    bool CStyleSheetUtility::deleteModifiedFontFile()
    {
        const QString fn = CFileUtils::appendFilePaths(CBuildConfig::getStylesheetsDir(), fileNameFontsModified());
        QFile file(fn);
        if (!file.exists()) { return false; }
        bool r = file.remove();
        if (!r) { return false; }
        this->read();
        return true;
    }

    const QString &CStyleSheetUtility::fileNameSwiftStandardGui()
    {
        static const QString f(getQssFileName("swiftstdgui"));
        return f;
    }

    const QString &CStyleSheetUtility::fileNameInfoBar()
    {
        static const QString f(getQssFileName("infobar"));
        return f;
    }

    const QString &CStyleSheetUtility::fileNameNavigator()
    {
        static const QString f(getQssFileName("navigator"));
        return f;
    }

    const QString &CStyleSheetUtility::fileNameDockWidgetTab()
    {
        static const QString f(getQssFileName("dockwidgettab"));
        return f;
    }

    const QString &CStyleSheetUtility::fileNameStandardWidget()
    {
        static const QString f(getQssFileName("stdwidget"));
        return f;
    }

    const QString &CStyleSheetUtility::fileNameTextMessage()
    {
        static const QString f("textmessage.css");
        return f;
    }

    const QString &CStyleSheetUtility::fileNameFilterDialog()
    {
        static const QString f(getQssFileName("filterdialog"));
        return f;
    }

    const QString &CStyleSheetUtility::fileNameSwiftCore()
    {
        static const QString f(getQssFileName("swiftcore"));
        return f;
    }

    const QString &CStyleSheetUtility::fileNameSwiftData()
    {
        static const QString f(getQssFileName("swiftdata"));
        return f;
    }

    const QString &CStyleSheetUtility::fileNameSwiftLauncher()
    {
        static const QString f(getQssFileName("swiftlauncher"));
        return f;
    }

    const QStringList &CStyleSheetUtility::fontWeights()
    {
        static const QStringList w({"bold", "semibold", "light", "black", "normal"});
        return w;
    }

    const QStringList &CStyleSheetUtility::fontStyles()
    {
        static const QStringList s({"italic", "oblique", "normal"});
        return s;
    }

    const QString &CStyleSheetUtility::transparentBackgroundColor()
    {
        static const QString t = "background-color: transparent;";
        return t;
    }

    bool CStyleSheetUtility::useStyleSheetInDerivedWidget(QWidget *usedWidget, QStyle::PrimitiveElement element)
    {
        Q_ASSERT(usedWidget);
        if (!usedWidget) { return false; }

        Q_ASSERT(usedWidget->style());
        QStyle *style = usedWidget->style();
        if (!style) { return false; }

        // 1) QStylePainter: modern version of
        //    usedWidget->style()->drawPrimitive(element, &opt, &p, usedWidget);
        // 2) With viewport based widgets viewport has to be used
        // see http://stackoverflow.com/questions/37952348/enable-own-widget-for-stylesheet
        QAbstractScrollArea *sa = qobject_cast<QAbstractScrollArea *>(usedWidget);
        QStylePainter p(
            sa ? sa->viewport() :
            usedWidget);
        if (!p.isActive()) { return false; }

        QStyleOption opt;
        opt.initFrom(usedWidget);
        p.drawPrimitive(element, opt);
        return true;
    }

    QString CStyleSheetUtility::styleForIconCheckBox(const QString &checkedIcon, const QString &uncheckedIcon, const QString &width, const QString &height)
    {
        Q_ASSERT(!checkedIcon.isEmpty());
        Q_ASSERT(!uncheckedIcon.isEmpty());

        static const QString st = "QCheckBox::indicator { width: %1; height: %2; } QCheckBox::indicator:checked { image: url(%3); } QCheckBox::indicator:unchecked { image: url(%4); }";
        return st.arg(width).arg(height).arg(checkedIcon).arg(uncheckedIcon);
    }

    QString CStyleSheetUtility::concatStyles(const QString &style1, const QString &style2)
    {
        QString s1(style1.trimmed());
        QString s2(style2.trimmed());
        if (s1.isEmpty()) { return s2; }
        if (s2.isEmpty()) { return s1; }
        if (!s1.endsWith(";")) { s1 = s1.append("; "); }
        s1.append(s2);
        if (!s1.endsWith(";")) { s1 = s1.append(";"); }
        return s1;
    }

    void CStyleSheetUtility::setQSysInfoProperties(QWidget *widget, bool withChildWidgets)
    {
        Q_ASSERT_X(widget, Q_FUNC_INFO, "Missing widget");
        if (!widget->property("qsysKernelType").isValid())
        {
            widget->setProperty("qsysKernelType", QSysInfo::kernelType());
            widget->setProperty("qsysCurrentCpuArchitecture", QSysInfo::currentCpuArchitecture());
            widget->setProperty("qsysBuildCpuArchitecture", QSysInfo::buildCpuArchitecture());
            widget->setProperty("qsysProductType", QSysInfo::productType());
        }

        if (withChildWidgets)
        {
            for (QWidget *w : widget->findChildren<QWidget *>(QString(), Qt::FindDirectChildrenOnly))
            {
                CStyleSheetUtility::setQSysInfoProperties(w, true);
            }
        }
    }

    void CStyleSheetUtility::ps_qssDirectoryChanged(const QString &file)
    {
        Q_UNUSED(file);
        this->read();
    }

    QString CStyleSheetUtility::getQssFileName(const QString &fileName)
    {
        static const QString qss(".qss");
        QString fn(fileName);
        if (fn.endsWith(qss)) { fn.chop(qss.length()); }

        QString specific;
        if (CBuildConfig::isRunningOnWindowsNtPlatform())
        {
            specific = fn + ".win" + qss;
        }
        else if (CBuildConfig::isRunningOnMacOSXPlatform())
        {
            specific = fn + ".mac" + qss;
        }
        else if (CBuildConfig::isRunningOnLinuxPlatform())
        {
            specific = fn + ".linux" + qss;
        }
        return qssFileExists(specific) ? specific : fn + qss;
    }

    bool CStyleSheetUtility::qssFileExists(const QString &filename)
    {
        if (filename.isEmpty()) { return false; }
        const QFileInfo f(CFileUtils::appendFilePaths(CBuildConfig::getStylesheetsDir(), filename));
        return f.exists() && f.isReadable();
    }
}
