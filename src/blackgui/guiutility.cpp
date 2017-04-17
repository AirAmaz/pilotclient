/* Copyright (C) 2014
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "blackgui/enableforframelesswindow.h"
#include "blackgui/guiutility.h"
#include "blackgui/overlaymessagesframe.h"
#include "blackmisc/verify.h"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QLayout>
#include <QLayoutItem>
#include <QList>
#include <QMainWindow>
#include <QMetaType>
#include <QMimeData>
#include <QObject>
#include <QRegularExpression>
#include <QTabWidget>
#include <QThreadStorage>
#include <QWidget>
#include <QTimer>
#include <Qt>
#include <QtGlobal>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>

using namespace BlackMisc;

namespace BlackGui
{
    CEnableForFramelessWindow *CGuiUtility::mainFramelessEnabledApplicationWindow()
    {
        const QWidgetList tlw = topLevelApplicationWidgetsWithName();
        for (QWidget *w : tlw)
        {
            // best choice is to check on frameless window
            CEnableForFramelessWindow *mw = dynamic_cast<CEnableForFramelessWindow *>(w);
            if (mw && mw->isMainApplicationWindow()) { return mw; }
        }
        return nullptr;
    }

    QWidget *CGuiUtility::mainApplicationWindow()
    {
        CEnableForFramelessWindow *mw = mainFramelessEnabledApplicationWindow();
        if (mw && mw->getWidget())
        {
            return mw->getWidget();
        }

        // second choice, try via QMainWindow
        const QWidgetList tlw = topLevelApplicationWidgetsWithName();
        for (QWidget *w : tlw)
        {
            QMainWindow *qmw = qobject_cast<QMainWindow *>(w);
            if (!qmw) { continue; }
            if (!qmw->parentWidget()) { return qmw; }
        }
        return nullptr;
    }

    bool CGuiUtility::isMainWindowFrameless()
    {
        CEnableForFramelessWindow *mw = mainFramelessEnabledApplicationWindow();
        return (mw && mw->isFrameless());
    }

    bool CGuiUtility::lenientTitleComparison(const QString &title, const QString &comparison)
    {
        if (title == comparison) { return true; }

        QString t(title.trimmed().toLower().simplified());
        QString c(comparison.trimmed().toLower().simplified());
        Q_ASSERT_X(!t.isEmpty(), Q_FUNC_INFO, "missing value");
        Q_ASSERT_X(!c.isEmpty(), Q_FUNC_INFO, "missing value");
        if (t == c) { return true; }

        // further unify
        static QThreadStorage<QRegularExpression> tsRegex;
        if (! tsRegex.hasLocalData()) { tsRegex.setLocalData(QRegularExpression("[^a-z0-9\\s]")); }
        const QRegularExpression &regexp = tsRegex.localData();
        t = t.remove(regexp);
        c = c.remove(regexp);
        return t == c;
    }

    bool CGuiUtility::setComboBoxValueByStartingString(QComboBox *box, const QString &candidate, const QString &unspecified)
    {
        if (!box) { return false; }
        if (!candidate.isEmpty())
        {
            for (int i = 0; i < box->count(); i++)
            {
                QString t(box->itemText(i));
                if (t.startsWith(candidate, Qt::CaseInsensitive))
                {
                    box->setCurrentIndex(i);
                    return true;
                }
            }
        }

        // not found
        if (unspecified.isEmpty()) { return false; }
        for (int i = 0; i < box->count(); i++)
        {
            QString t(box->itemText(i));
            if (t.startsWith(unspecified, Qt::CaseInsensitive))
            {
                box->setCurrentIndex(i);
                return true;
            }
        }
        return false;
    }

    bool CGuiUtility::hasSwiftVariantMimeType(const QMimeData *mime)
    {
        return mime && mime->hasFormat(swiftJsonDragAndDropMimeType());
    }

    CVariant CGuiUtility::fromSwiftDragAndDropData(const QMimeData *mime)
    {
        if (hasSwiftVariantMimeType(mime))
        {
            return fromSwiftDragAndDropData(mime->data(swiftJsonDragAndDropMimeType()));
        }
        return CVariant();
    }

    CVariant CGuiUtility::fromSwiftDragAndDropData(const QByteArray &utf8Data)
    {
        if (utf8Data.isEmpty()) { return CVariant(); }
        QJsonDocument jsonDoc(QJsonDocument::fromJson(utf8Data));
        QJsonObject jsonObj(jsonDoc.object());
        QString typeName(jsonObj.value("type").toString());
        int typeId = QMetaType::type(qPrintable(typeName));

        // check if a potential valid value object
        if (typeName.isEmpty() || typeId == QMetaType::UnknownType) { return CVariant(); }

        CVariant valueVariant;
        const CStatusMessage status = valueVariant.convertFromJsonNoThrow(jsonObj, {}, {});
        if (status.isFailure()) { return CVariant(); }
        return valueVariant;
    }

    int CGuiUtility::metaTypeIdFromSwiftDragAndDropData(const QMimeData *mime)
    {
        constexpr int Unknown = static_cast<int>(QMetaType::UnknownType);

        if (!hasSwiftVariantMimeType(mime)) { return Unknown; }
        QJsonDocument jsonDoc(QJsonDocument::fromJson(mime->data(swiftJsonDragAndDropMimeType())));
        QJsonObject jsonObj(jsonDoc.object());
        if (jsonObj.isEmpty()) { return Unknown; }
        QString typeName(jsonObj.value("type").toString());
        if (typeName.isEmpty()) { return Unknown; }
        int typeId = QMetaType::type(qPrintable(typeName));
        return typeId;
    }

    COverlayMessagesFrame *CGuiUtility::nextOverlayMessageFrame(QWidget *widget, int maxLevels)
    {
        if (!widget || maxLevels < 1) { return nullptr; }
        COverlayMessagesFrame *o = qobject_cast<COverlayMessagesFrame *> (widget);
        if (o) { return o; }
        int cl = 0;
        QWidget *cw = widget->parentWidget();
        while (cl < maxLevels && cw)
        {
            o = qobject_cast<COverlayMessagesFrame *> (cw);
            if (o) { return o; }
            cl++;
            cw = cw->parentWidget();
        }
        return nullptr;
    }

    const QString &CGuiUtility::swiftJsonDragAndDropMimeType()
    {
        static const QString m("text/json/swift");
        return m;
    }

    void CGuiUtility::checkBoxReadOnly(QCheckBox *checkBox, bool readOnly)
    {
        static const QCheckBox defaultBox;
        BLACK_VERIFY_X(checkBox, Q_FUNC_INFO, "no checkbox");
        if (!checkBox) { return; }

        if (readOnly)
        {
            checkBox->setAttribute(Qt::WA_TransparentForMouseEvents);
            checkBox->setFocusPolicy(Qt::NoFocus);
        }
        else
        {
            checkBox->setAttribute(Qt::WA_TransparentForMouseEvents, defaultBox.testAttribute(Qt::WA_TransparentForMouseEvents));
            checkBox->setFocusPolicy(defaultBox.focusPolicy());
        }
    }

    QWidgetList CGuiUtility::topLevelApplicationWidgetsWithName()
    {
        QWidgetList tlw = QApplication::topLevelWidgets();
        QWidgetList rl;
        foreach (QWidget *w, tlw)
        {
            if (w->objectName().isEmpty()) { continue; }
            rl.append(w);
        }
        return rl;
    }

    QPoint CGuiUtility::mainWindowPosition()
    {
        CEnableForFramelessWindow *mw = mainFramelessEnabledApplicationWindow();
        return (mw) ? mw->getWidget()->pos() : QPoint();
    }

    QString CGuiUtility::replaceTabCountValue(const QString &oldName, int count)
    {
        const QString v = QString(" (").append(QString::number(count)).append(")");
        if (oldName.isEmpty()) { return v; }
        int index = oldName.lastIndexOf('(');
        if (index == 0) { return v; }
        if (index < 0) { return QString(oldName).trimmed().append(v); }
        return QString(oldName.left(index)).trimmed().append(v);
    }

    void CGuiUtility::deleteLayout(QLayout *layout, bool deleteWidgets)
    {
        // http://stackoverflow.com/a/7569928/356726
        if (!layout) { return; }
        QLayoutItem *item {nullptr};
        while ((item = layout->takeAt(0)))
        {
            QLayout *sublayout {nullptr};
            QWidget *widget {nullptr};
            if ((sublayout = item->layout()))
            {
                deleteLayout(sublayout, deleteWidgets);
            }
            else if ((widget = item->widget()))
            {
                widget->hide();
                if (deleteWidgets)
                {
                    delete widget;
                }
            }
            else {delete item;}
        }

        // then finally
        delete layout;
    }

    bool CGuiUtility::staysOnTop(QWidget *widget)
    {
        if (!widget) { return false; }
        const Qt::WindowFlags flags = widget->windowFlags();
        return Qt::WindowStaysOnTopHint & flags;
    }

    QTabWidget *CGuiUtility::parentTabWidget(QWidget *widget, int maxLevels)
    {
        int level = 0;
        do
        {
            widget = widget->parentWidget();
            if (!widget) { return nullptr; }
            QTabWidget *tw = qobject_cast<QTabWidget *>(widget);
            if (tw) { return tw; }
            level++;
        }
        while (level < maxLevels);
        return nullptr;
    }

    bool CGuiUtility::toggleStayOnTop(QWidget *widget)
    {
        if (!widget) { return false; }
        Qt::WindowFlags flags = widget->windowFlags();
        if (Qt::WindowStaysOnTopHint & flags)
        {
            flags &= ~Qt::WindowStaysOnTopHint;
            flags |= Qt::WindowStaysOnBottomHint;
        }
        else
        {
            flags &= ~Qt::WindowStaysOnBottomHint;
            flags |= Qt::WindowStaysOnTopHint;
        }
        widget->setWindowFlags(flags);
        widget->show();
        return Qt::WindowStaysOnTopHint & flags;
    }

    QString CGuiUtility::marginsToString(const QMargins &margins)
    {
        const QString s("%1:%2:%3:%4");
        return s.arg(margins.left()).arg(margins.top()).arg(margins.right()).arg(margins.bottom());
    }

    QMargins CGuiUtility::stringToMargins(const QString &str)
    {
        const QStringList parts = str.split(":");
        Q_ASSERT_X(parts.size() == 4, Q_FUNC_INFO, "malformed");
        bool ok = false;
        const int l = parts.at(0).toInt(&ok);
        Q_ASSERT_X(ok, Q_FUNC_INFO, "malformed number");
        const int t = parts.at(1).toInt(&ok);
        Q_ASSERT_X(ok, Q_FUNC_INFO, "malformed number");
        const int r = parts.at(2).toInt(&ok);
        Q_ASSERT_X(ok, Q_FUNC_INFO, "malformed number");
        const int b = parts.at(3).toInt(&ok);
        Q_ASSERT_X(ok, Q_FUNC_INFO, "malformed number");
        Q_UNUSED(ok);
        return QMargins(l, t, r, b);
    }

    QList<int> CGuiUtility::indexToUniqueRows(const QModelIndexList &indexes)
    {
        QList<int> rows;
        for (const QModelIndex &i : indexes)
        {
            const int r = i.row();
            if (rows.contains(r)) { continue; }
            rows.append(r);
        }
        return rows;
    }

    bool CGuiUtility::isTopLevelWidget(QWidget *widget)
    {
        return QApplication::topLevelWidgets().contains(widget);
    }

    QGraphicsOpacityEffect *CGuiUtility::fadeInWidget(int durationMs, QWidget *widget, double startValue, double endValue)
    {
        // http://stackoverflow.com/questions/19087822/how-to-make-qt-widgets-fade-in-or-fade-out#
        Q_ASSERT(widget);
        QGraphicsOpacityEffect *effect = new QGraphicsOpacityEffect(widget);
        widget->setGraphicsEffect(effect);
        QPropertyAnimation *a = new QPropertyAnimation(effect, "opacity");
        a->setDuration(durationMs);
        a->setStartValue(startValue);
        a->setEndValue(endValue);
        a->setEasingCurve(QEasingCurve::InBack);
        a->start(QPropertyAnimation::DeleteWhenStopped);
        return effect;
    }

    QGraphicsOpacityEffect *CGuiUtility::fadeOutWidget(int durationMs, QWidget *widget, double startValue, double endValue)
    {
        Q_ASSERT(widget);
        QGraphicsOpacityEffect *effect = new QGraphicsOpacityEffect(widget);
        widget->setGraphicsEffect(effect);
        QPropertyAnimation *a = new QPropertyAnimation(effect, "opacity");
        a->setDuration(durationMs);
        a->setStartValue(startValue);
        a->setEndValue(endValue);
        a->setEasingCurve(QEasingCurve::OutBack);
        a->start(QPropertyAnimation::DeleteWhenStopped);
        return effect;
    }
} // ns
