/*
 gFlagsLine Header
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*/

#ifndef GFLAGSLINE_H
#define GFLAGSLINE_H

#include "gGraphView.h"
#include "gspacer.h"

class gFlagsGroup;

/*! \class gYSpacer
    \brief A dummy vertical spacer object
   */
class gFlagsLabelArea:public gSpacer
{
    public:
        gFlagsLabelArea(gFlagsGroup * group);
        virtual void paint(gGraph & w,int left,int top, int width, int height) {
            Q_UNUSED(w)
            Q_UNUSED(left)
            Q_UNUSED(top)
            Q_UNUSED(width)
            Q_UNUSED(height)
        }
protected:
        gFlagsGroup * m_group;
        virtual bool mouseMoveEvent(QMouseEvent * event,gGraph * graph);


};


/*! \class gFlagsLine
    \brief One single line of event flags in the Event Flags chart
    */
class gFlagsLine:public Layer
{
    friend class gFlagsGroup;
    public:
        /*! \brief Constructs an individual gFlagsLine object
            \param code  The Channel the data is sourced from
            \param col   The colour to draw this flag
            \param label The label to show to the left of the Flags line.
            \param always_visible  Whether to always show this line, even if empty
            \param Type of Flag, either FT_Bar, or FT_Span
            */
        gFlagsLine(ChannelID code,QColor col=Qt::black,QString label="",bool always_visible=false,FlagType flt=FT_Bar);
        virtual ~gFlagsLine();

        //! \brief Drawing code to add the flags and span markers to the Vertex buffers.
        virtual void paint(gGraph & w,int left, int top, int width, int height);

        //! \brief Returns true if should always show this flag, even if it's empty
        bool isAlwaysVisible() { return m_always_visible; }
        //! \brief Set this to true to make a flag line always visible
        void setAlwaysVisible(bool b) { m_always_visible=b; }

        //! \brief Returns the label for this individual Event Flags line
        QString label() { return m_label; }

        //! \brief Sets the label for this individual Event Flags line
        void setLabel(QString s) { m_label=s; }

        void setTotalLines(int i) { total_lines=i; }
        void setLineNum(int i) { line_num=i; }
    protected:

        virtual bool mouseMoveEvent(QMouseEvent * event,gGraph * graph);

        QString m_label;
        bool m_always_visible;
        int total_lines,line_num;
        FlagType m_flt;
        QColor m_flag_color;
        gVertexBuffer *quads;
        gVertexBuffer *lines;
        int m_lx, m_ly;
};

/*! \class gFlagsGroup
    \brief Contains multiple gFlagsLine entries for the Events Flag graph
    */
class gFlagsGroup:public LayerGroup
{
    friend class gFlagsLabelArea;

public:
    gFlagsGroup();
    virtual ~gFlagsGroup();

    //! Draw filled rectangles behind Event Flag's, and an outlines around them all, Calls the individual paint for each gFlagLine
    virtual void paint(gGraph & w,int left, int top, int width, int height);

    //! Returns the first time represented by all gFlagLine layers, in milliseconds since epoch
    virtual qint64 Minx();
    //! Returns the last time represented by all gFlagLine layers, in milliseconds since epoch.
    virtual qint64 Maxx();

    //! Checks if each flag has data, and adds valid gFlagLines to the visible layers list
    virtual void SetDay(Day *);

    //! Returns true if none of the gFlagLine objects contain any data for this day
    virtual bool isEmpty() { return m_empty; }

    //! Returns the count of visible flag line entries
    int count() { return lvisible.size(); }

    //! Returns the height in pixels of each bar
    int barHeight() { return m_barh; }

    //! Returns a list of Visible gFlagsLine layers to draw
    QVector<gFlagsLine *> & visibleLayers() { return lvisible; }

protected:
    virtual bool mouseMoveEvent(QMouseEvent * event,gGraph * graph);

    gVertexBuffer *quads, *lines;
    QVector<gFlagsLine *> lvisible;
    float m_barh;
    bool m_empty;
};

#endif // GFLAGSLINE_H
