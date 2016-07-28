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

//! \file

#ifndef BLACKGUI_LOADINDICATOR_H
#define BLACKGUI_LOADINDICATOR_H

#include "blackgui/blackguiexport.h"

#include <QColor>
#include <QObject>
#include <QSize>
#include <QWidget>
#include <Qt>

class QPaintEvent;
class QPainter;
class QTimerEvent;

namespace BlackGui
{
    /**
     * The QProgressIndicator class lets an application display a progress indicator to show that a lengthy task is under way.
     * Progress indicators are indeterminate and do nothing more than spin to show that the application is busy.
     * \note based on https://github.com/mojocorp/QProgressIndicator under MIT license
     */
    class BLACKGUI_EXPORT CLoadIndicator : public QWidget
    {
        Q_OBJECT

    public:
        //! Constructor
        CLoadIndicator(int width = 64, int height = 64, QWidget *parent = nullptr);

        //! Returns the delay between animation steps.
        //! \return The number of milliseconds between animation steps. By default, the animation delay is set to 40 milliseconds.
        //! \sa setAnimationDelay
        int animationDelay() const { return m_delayMs; }

        //! Returns a Boolean value indicating whether the component is currently animated.
        //! \return Animation state.
        //! \sa startAnimation stopAnimation
        bool isAnimated() const;

        //! Returns a Boolean value indicating whether the receiver shows itself even when it is not animating.
        //! \return Return true if the progress indicator shows itself even when it is not animating. By default, it returns false.
        //! \sa setDisplayedWhenStopped
        bool isDisplayedWhenStopped() const;

        //! Returns the color of the component.
        //! \sa setColor
        const QColor &color() const { return m_color; }

        //! \copydoc QWidget::sizeHint
        virtual QSize sizeHint() const override;

        //! \copydoc QWidget::heightForWidth
        virtual int heightForWidth(int w) const override;

        //! Paint to another painter
        void paint(QPainter &painter) const;

        //! Center this load indicator
        void centerLoadIndicator(const QPoint &middle);

    signals:
        //! Animation has been updated
        void updatedAnimation();

    public slots:
        //! Starts the spin animation.
        //! \sa stopAnimation isAnimated
        void startAnimation(bool processEvents = false);

        //! Stops the spin animation.
        //! \sa startAnimation isAnimated
        void stopAnimation();

        //! Sets the delay between animation steps.
        //! Setting the \a delay to a value larger than 40 slows the animation, while setting the \a delay to a smaller value speeds it up.
        //! \param delay The delay, in milliseconds.
        //! \sa animationDelay
        void setAnimationDelay(int delay);

        //! Sets whether the component hides itself when it is not animating.
        //! \param state The animation state. Set false to hide the progress indicator when it is not animating; otherwise true.
        //! \sa isDisplayedWhenStopped
        void setDisplayedWhenStopped(bool state);

        //! Sets the color of the components to the given color.
        //! \sa color
        void setColor(const QColor &color);

    protected:
        //! \copydoc QWidget::timerEvent
        virtual void timerEvent(QTimerEvent *event) override;

        //! \copydoc QWidget::paintEvent
        virtual void paintEvent(QPaintEvent *event) override;

        //! Is parent widget visible
        bool isParentVisible() const;

    private:
        int m_angle                 = 0;
        int m_timerId               = -1;
        int m_delayMs               = 1000;
        bool m_displayedWhenStopped = false;
        QColor m_color              = Qt::blue;
    };
} // ns

#endif
