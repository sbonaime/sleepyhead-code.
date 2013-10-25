/*
 gYAxis Header
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*/

#ifndef GYAXIS_H
#define GYAXIS_H

#include "gGraphView.h"


/*! \class gXGrid
    \brief Draws the horizintal major/minor grids over graphs
   */
class gXGrid:public Layer
{
public:
    //! \brief Constructs an gXGrid object with default settings, and col for line colour.
    gXGrid(QColor col=QColor("black"));
    virtual ~gXGrid();

    //! \brief Draw the horizontal lines by adding the to the Vertex GLbuffers
    virtual void paint(gGraph & w,int left,int top, int width, int height);

    //! \brief set the visibility status of Major lines
    void setShowMinorLines(bool b) { m_show_minor_lines=b; }

    //! \brief set the visibility status of Minor lines
    void setShowMajorLines(bool b) { m_show_major_lines=b; }

    //! \brief Returns the visibility status of minor lines
    bool showMinorLines() { return m_show_minor_lines; }

    //! \brief Returns the visibility status of Major lines
    bool showMajorLines() { return m_show_major_lines; }
protected:
    bool m_show_major_lines;
    bool m_show_minor_lines;
    QColor m_major_color;
    QColor m_minor_color;
};

/*! \class gYAxis
   \brief Draws the YAxis tick markers, and numeric labels
   */
class gYAxis:public Layer
{
    public:
        //! \brief Construct a gYAxis object, with QColor col for tickers & text
        gYAxis(QColor col=Qt::black);
        virtual ~gYAxis();

        //! \brief Draw the horizontal tickers display
        virtual void paint(gGraph & w,int left,int top, int width, int height);

//        void SetShowMinorLines(bool b) { m_show_minor_lines=b; }
//        void SetShowMajorLines(bool b) { m_show_major_lines=b; }
//        bool ShowMinorLines() { return m_show_minor_lines; }
//        bool ShowMajorLines() { return m_show_major_lines; }

        //! \brief Sets the visibility status of minor ticks
        void SetShowMinorTicks(bool b) { m_show_minor_ticks=b; }

        //! \brief Sets the visibility status of Major ticks
        void SetShowMajorTicks(bool b) { m_show_major_ticks=b; }

        //! \brief Returns the visibility status of Minor ticks
        bool ShowMinorTicks() { return m_show_minor_ticks; }

        //! \brief Returns the visibility status of Major ticks
        bool ShowMajorTicks() { return m_show_major_ticks; }

        //! \brief Formats the ticker value.. Override to implement other types
        virtual const QString Format(EventDataType v, int dp);

        //! \brief Left Margin space in pixels
        static const int Margin=60;

        //! \brief Set the scale of the Y axis values.. Values can be multiplied by this to convert formats
        void SetScale(float f) { m_yaxis_scale=f; }

        //! \brief Returns the scale of the Y axis values.. Values can be multiplied by this to convert formats
        float Scale() { return m_yaxis_scale; }

    protected:
        //bool m_show_major_lines;
        //bool m_show_minor_lines;
        bool m_show_minor_ticks;
        bool m_show_major_ticks;
        float m_yaxis_scale;

        QColor m_line_color;
        QColor m_text_color;
        gVertexBuffer * lines;
        virtual bool mouseMoveEvent(QMouseEvent * event,gGraph * graph);
        virtual bool mouseDoubleClickEvent(QMouseEvent * event, gGraph * graph);

        QImage m_image;
        GLuint m_textureID;

};

/*! \class gYAxisTime
   \brief Draws the YAxis tick markers, and labels in time format
   */
class gYAxisTime:public gYAxis
{
public:
    //! \brief Construct a gYAxisTime object, with QColor col for tickers & times
    gYAxisTime(bool hr12=true, QColor col=Qt::black) : gYAxis(col), show_12hr(hr12) {}
    virtual ~gYAxisTime() {}
protected:
    //! \brief Overrides gYAxis Format to display Time format
    virtual const QString Format(EventDataType v, int dp);

    //! \brief Whether to format as 12 or 24 hour times
    bool show_12hr;
};


/*! \class gYAxisWeight
   \brief Draws the YAxis tick markers, and labels in weight format
   */
class gYAxisWeight:public gYAxis
{
public:
    //! \brief Construct a gYAxisWeight object, with QColor col for tickers & weight values
    gYAxisWeight(UnitSystem us=US_Metric, QColor col=Qt::black) :gYAxis(col), m_unitsystem(us) {}
    virtual ~gYAxisWeight() {}

    //! \brief Returns the current UnitSystem displayed (eg, US_Metric (the rest of the world), US_Archiac (American) )
    UnitSystem unitSystem() { return m_unitsystem; }

    //! \brief Set the unit system displayed by this YTicker
    void setUnitSystem(UnitSystem us) { m_unitsystem=us; }
protected:
    //! \brief Overrides gYAxis Format to display Time format
    virtual const QString Format(EventDataType v, int dp);
    UnitSystem m_unitsystem;
};


#endif // GYAXIS_H
