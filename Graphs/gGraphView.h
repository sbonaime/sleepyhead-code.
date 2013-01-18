/*
 gGraphView Header
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*/

#ifndef GGRAPHVIEW_H
#define GGRAPHVIEW_H

#include <QGLWidget>
#include <QScrollBar>
#include <QResizeEvent>
#include <SleepLib/day.h>
#include <QThread>
#include <QMutex>
#include <QSemaphore>
#include <QWaitCondition>
#include <QPixmap>
#include <Graphs/glcommon.h>


#define MIN(a,b) (((a)<(b)) ? (a) : (b));
#define MAX(a,b) (((a)<(b)) ? (b) : (a));

enum FlagType { FT_Bar, FT_Dot, FT_Span };

//const int default_height=160;

//! \brief Initialize the Graph Fonts
void InitGraphs();
//! \brief Destroy the Graph Fonts
void DoneGraphs();

extern QFont * defaultfont;
extern QFont * mediumfont;
extern QFont * bigfont;

extern QHash<QString, QImage *> images;

/*! \brief Gets the width and height parameters for supplied text
    \param QString text - The text string in question
    \param int & width
    \param int & height
    \param QFont * font - The selected font used in the size calculations
    */
void GetTextExtent(QString text, int & width, int & height, QFont *font=defaultfont);

//! \brief Return the height of the letter x for the selected font.
int GetXHeight(QFont *font=defaultfont);

class gGraphView;
class gGraph;

const int textque_max=512;

typedef quint32 RGBA;
/*union RGBA {
   struct {
        GLubyte red;
        GLubyte green;
        GLubyte blue;
        GLubyte alpha;
    } bytes;
    quint32 value;
}; */

#ifdef BUILD_WITH_MSVC
__declspec(align(1))
#endif
struct gVertex
{
    gVertex(GLshort _x, GLshort _y, GLuint _c) { x=_x; y=_y; color=_c; }
    GLshort x;
    GLshort y;
    RGBA color;
}
#ifndef BUILD_WITH_MSVC
__attribute__((packed))
#endif
;

class gVertexBuffer
{
public:
    gVertexBuffer(int max=2048,int type=GL_LINES);
    ~gVertexBuffer();

    void add(GLshort x1, GLshort y1, RGBA color);
    void add(GLshort x1, GLshort y1, GLshort x2, GLshort y2, RGBA color);
    void add(GLshort x1, GLshort y1, GLshort x2, GLshort y2, GLshort x3, GLshort y3, GLshort x4, GLshort y4, RGBA color);
    void add(GLshort x1, GLshort y1, GLshort x2, GLshort y2, GLshort x3, GLshort y3, GLshort x4, GLshort y4, RGBA color, RGBA color2);

    void add(GLshort x1, GLshort y1);
    void add(GLshort x1, GLshort y1, GLshort x2, GLshort y2);
    void add(GLshort x1, GLshort y1, GLshort x2, GLshort y2, GLshort x3, GLshort y3, GLshort x4, GLshort y4);

    void unsafe_add(GLshort x1, GLshort y1);
    void unsafe_add(GLshort x1, GLshort y1, GLshort x2, GLshort y2);
    void unsafe_add(GLshort x1, GLshort y1, GLshort x2, GLshort y2, GLshort x3, GLshort y3, GLshort x4, GLshort y4);

    void draw();

    void scissor(GLshort x, GLshort y, GLshort width, GLshort height) { s_x=x; s_y=y; s_width=width; s_height=height; m_scissor=true; }

    void reset() { m_cnt=0; }
    int Max() { return m_max; }
    int cnt() { return m_cnt; }
    GLuint type() { return m_type; }
    float size() { return m_size; }
    bool full() { return m_cnt>=m_max; }

    void forceAntiAlias(bool b) { m_forceantialias=b; }
    void setSize(float f) { m_size=f; }
    void setAntiAlias(bool b) { m_antialias=b; }
    void setStipple(GLshort stipple) { m_stipple=stipple; }
    void setStippleOn(bool b) { m_stippled=b; }
    void setColor(QColor col);
    void setBlendFunc(GLuint b1, GLuint b2) { m_blendfunc1=b1; m_blendfunc2=b2; }

protected:
    //! \brief Maximum number of gVertex points contained in buffer
    int m_max;
    //! \brief Indicates type of GL vertex information (GL_LINES, GL_QUADS, etc)
    GLuint m_type;
    //! \brief Count of Vertex points used this draw cycle.
    int m_cnt;
    //! \brief Line/Point thickness
    float m_size;

    bool m_scissor;
    bool m_antialias;
    bool m_forceantialias;
    bool m_stippled;

    //! \brief Contains list of Vertex & Color points
    gVertex * buffer;
    //! \brief GL Scissor parameters
    GLshort s_x,s_y,s_width,s_height;
    //! \brief Current drawing color
    GLuint m_color;
    //! \brief Stipple bitfield
    GLshort m_stipple;
    //! \brief Source GL Blend Function
    GLuint m_blendfunc1;
    //! \brief Destination GL Blend Function
    GLuint m_blendfunc2;
};

/*! \class GLBuffer
    \brief Base Object to hold an OpenGL draw list
    */
class GLBuffer
{
public:
    GLBuffer(int max=2048,int type=GL_LINES,bool stippled=false);
    virtual ~GLBuffer();
    void scissor(GLshort x1, GLshort y1, GLshort x2, GLshort y2) { s1=x1; s2=y1; s3=x2; s4=y2; m_scissor=true; }
    virtual void draw(){}
    void reset() { m_cnt=0; }
    int Max() { return m_max; }
    int cnt() { return m_cnt; }
    bool full() { return m_cnt>=m_max; }
    float size() { return m_size; }
    int type() { return m_type; }
    void setSize(float f) { m_size=f; }
    void setAntiAlias(bool b) { m_antialias=b; }
    void forceAntiAlias(bool b) { m_forceantialias=b; }
    void setColor(QColor col) { m_color=col; }
    void setBlendFunc(GLuint b1, GLuint b2) { m_blendfunc1=b1; m_blendfunc2=b2; }
protected:
    int m_max;
    int m_type;     // type (GL_LINES, GL_QUADS, etc)
    int m_cnt;      // cnt
    int m_colcnt;
    QColor m_color;
    float m_size;
    int s1,s2,s3,s4;
    bool m_scissor;
    bool m_antialias;
    bool m_forceantialias;
    QMutex mutex;
    bool m_stippled;
    GLuint m_blendfunc1, m_blendfunc2;
};

/* ! \class GLShortBuffer
    \brief Holds an OpenGL draw list composed of 16bit integers and vertex colors
class GLShortBuffer:public GLBuffer
{
public:
    GLShortBuffer(int max=2048,int type=GL_LINES, bool stippled=false);
    virtual ~GLShortBuffer();

    // use one or the other.. can't use both
    // color free version is faster
    void add(GLshort x, GLshort y);
    void add(GLshort x1, GLshort y1, GLshort x2, GLshort y2);
    void add(GLshort x1, GLshort y1, GLshort x2, GLshort y2, GLshort x3, GLshort y3, GLshort x4, GLshort y4);

    // color per vertex version
    void add(GLshort x, GLshort y,QColor & col);    // add with vertex color
    void add(GLshort x1, GLshort y1, GLshort x2, GLshort y2,QColor & col); // add with vertex colors
    void add(GLshort x1, GLshort y1, GLshort x2, GLshort y2,GLshort x3, GLshort y3, GLshort x4, GLshort y4,QColor & col); // add with vertex colors

    virtual void draw();

    //inline GLshort & operator [](int i) { return buffer[i]; }
protected:
    GLshort * buffer;
    GLubyte * colors;
};
    */

/*! \class GLFloatBuffer
    \brief Holds an OpenGL draw list composed of 32bit GLfloat objects and vertex colors
    */
class GLFloatBuffer:public GLBuffer
{
public:
    GLFloatBuffer(int max=2048,int type=GL_LINES, bool stippled=false);
    virtual ~GLFloatBuffer();

    void add(GLfloat x, GLfloat y,QColor & col);    // add with vertex color
    void add(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2,QColor & col); // add with vertex colors
    void add(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2,GLfloat x3, GLfloat y3, GLfloat x4, GLfloat y4,QColor & col); // add with vertex colors
    void quadGrTB(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2,GLfloat x3, GLfloat y3, GLfloat x4, GLfloat y4,QColor & col,QColor & col2); // add with vertex colors
    void quadGrLR(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2,GLfloat x3, GLfloat y3, GLfloat x4, GLfloat y4,QColor & col,QColor & col2); // add with vertex colors

    virtual void draw();
protected:
    GLfloat * buffer;
    GLubyte * colors;
};

/*! \struct TextQue
    \brief Holds a single item of text for the drawing queue
    */
struct TextQue
{
    //! \variable contains the x axis screen position to draw the text
    short x;
    //! \variable contains the y axis screen position to draw the text
    short y;
    //! \variable the angle in degrees for drawing rotated text
    float angle;
    //! \variable the actual text to draw
    QString text;
    //! \variable the color the text will be drawn in
    QColor color;
    //! \variable a pointer to the QFont to use to draw this text
    QFont *font;
    //! \variable whether to use antialiasing to draw this text
    bool antialias;
};

/*! \class MyScrollBar
    \brief An custom scrollbar to interface with gGraphWindow
    */
class MyScrollBar:public QScrollBar
{
public:
    MyScrollBar(QWidget * parent=0);
    void SendWheelEvent(QWheelEvent * e);
};

enum LayerPosition { LayerLeft, LayerRight, LayerTop, LayerBottom, LayerCenter, LayerOverlay };

/*! \class Layer
    \brief The base component for all individual Graph layers
    */
class Layer
{
    friend class gGraph;
public:
    Layer(ChannelID code);
    virtual ~Layer();

    //! \brief This gets called on day selection, allowing this layer to precalculate any drawing data
    virtual void SetDay(Day * d);

    //! \brief Set the ChannelID used in this layer
    virtual void SetCode(ChannelID c) { m_code=c; }
    //! \brief Return the ChannelID used in this layer
    const ChannelID & code() { return m_code; }

    //! \brief returns true if this layer contains no data.
    virtual bool isEmpty();

    //! \brief Override and returns true if there are any highlighted components
    virtual bool isSelected() { return false; }

    //! \brief Deselect any highlighted components
    virtual void deselect() { }

    //! \brief Return this layers physical minimum date boundary
    virtual qint64 Minx() { if (m_day) return m_day->first(); return m_minx; }

    //! \brief Return this layers physical maximum date boundary
    virtual qint64 Maxx() { if (m_day) return m_day->last(); return m_maxx; }

    //! \brief Return this layers physical minimum Yaxis value
    virtual EventDataType Miny() { return m_miny; }

    //! \brief Return this layers physical maximum Yaxis value
    virtual EventDataType Maxy() { return m_maxy; }




    //! \brief Set this layers physical minimum date boundary
    virtual void setMinX(qint64 val) { m_minx=val; }

    //! \brief Set this layers physical maximum date boundary
    virtual void setMaxX(qint64 val) { m_maxx=val; }

    //! \brief Set this layers physical minimum Yaxis value
    virtual void setMinY(EventDataType val) { m_miny=val; }

    //! \brief Set this layers physical maximum Yaxis value
    virtual void setMaxY(EventDataType val) { m_maxy=val; }

    //! \brief Set this layers Visibility status
    void setVisible(bool b) { m_visible=b; }

    //! \brief Return this layers Visibility status
    bool visible() { return m_visible; }

    //! \brief Set this layers Moveability status (not really used yet)
    void setMovable(bool b) { m_movable=b; }

    //! \brief Return this layers Moveability status (not really used yet)
    bool movable() { return m_movable; }

    /*! \brief Override this for the drawing code, using GLBuffer components for drawing
        \param gGraph & gv    Graph Object that holds this layer
        \param int left
        \param int top
        \param int width
        \param int height
      */
    virtual void paint(gGraph & gv,int left,int top,int width, int height)=0;


    //! \brief Set the layout position and order for this layer.
    void setLayout(LayerPosition position, short width, short height, short order);

    void setPos(short x, short y) { m_X=x; m_Y=y; }

    int Width() { return m_width; }
    int Height() { return m_height; }

    //! \brief Return this Layers Layout Position.
    LayerPosition position() { return m_position; }
    //void X() { return m_X; }
    //void Y() { return m_Y; }


    //! \brief Draw all this layers custom GLBuffers (ie. the actual OpenGL Vertices)
    virtual void drawGLBuf(float linesize);

    //! \brief not sure why I needed the reference counting stuff.
    short m_refcount;
    void addref() { m_refcount++; }
    bool unref() { m_refcount--; if (m_refcount<=0) return true; return false; }

protected:
    //! \brief Add a GLBuffer (vertex) object customized to this layer
    void addGLBuf(GLBuffer *buf) { mgl_buffers.push_back(buf); }
    void addVertexBuffer(gVertexBuffer *buf) { mv_buffers.push_back(buf); }
    //QRect bounds; // bounds, relative to top of individual graph.
    Day *m_day;
    bool m_visible;
    bool m_movable;
    qint64 m_minx,m_maxx;
    EventDataType m_miny,m_maxy;
    ChannelID m_code;
    short m_width;                   // reserved x pixels needed for this layer.  0==Depends on position..
    short m_height;                  // reserved y pixels needed for this layer.  both 0 == expand to all free area.
    short m_X;                       // offset for repositionable layers..
    short m_Y;
    short m_order;                     // order for positioning..
    LayerPosition m_position;

    //! \brief A vector containing all this layers custom drawing buffers
    QVector<GLBuffer *> mgl_buffers;
    QVector<gVertexBuffer *> mv_buffers;

    //! \brief Mouse wheel moved somewhere over this layer
    virtual bool wheelEvent(QWheelEvent * event) { Q_UNUSED(event); return false; }
    //! \brief Mouse moved somewhere over this layer
    virtual bool mouseMoveEvent(QMouseEvent * event) { Q_UNUSED(event); return false; }
    //! \brief Mouse left or right button pressed somewhere on this layer
    virtual bool mousePressEvent(QMouseEvent * event) { Q_UNUSED(event); return false; }
    //! \brief Mouse button released that was originally pressed somewhere on this layer
    virtual bool mouseReleaseEvent(QMouseEvent * event) { Q_UNUSED(event); return false; }
    //! \brief Mouse button double clicked somewhere on this layer
    virtual bool mouseDoubleClickEvent(QMouseEvent * event) { Q_UNUSED(event); return false; }
    //! \brief A key was pressed on the keyboard while the graph area was focused.
    virtual bool keyPressEvent(QKeyEvent * event) { Q_UNUSED(event); return false; }
};

/*! \class LayerGroup
    \brief Contains a list of graph Layer obejcts
    */
class LayerGroup:public Layer
{
public:
    LayerGroup();
    virtual ~LayerGroup();

    //! \brief Add Layer to this Layer Group
    virtual void AddLayer(Layer *l);

    //! \brief Returns the minimum time value for all Layers contained in this group (milliseconds since epoch)
    virtual qint64 Minx();

    //! \brief Returns the maximum time value for all Layers contained in this group (milliseconds since epoch)
    virtual qint64 Maxx();

    //! \brief Returns the minimum Y-axis value for all Layers contained in this group
    virtual EventDataType Miny();

    //! \brief Returns the maximum Y-axis value for all Layers contained in this group
    virtual EventDataType Maxy();

    //! \brief Check all layers contained and return true if none contain data
    virtual bool isEmpty();

    //! \brief Calls SetDay for all Layers contained in this object
    virtual void SetDay(Day * d);

    //! \brief Calls drawGLBuf for all Layers contained in this object
    virtual void drawGLBuf(float linesize);

    //! \brief Return the list of Layers this object holds
    QVector<Layer *> & getLayers() { return layers; }

protected:
    //! \brief Contains all Layer objects in this group
    QVector<Layer *> layers;
};

class gGraph;

/*! \class gThread
    \brief Part of the Threaded drawing code

    This is currently broken, as Qt didn't behave anyway, and it offered no performance improvement drawing-wise
    */
class gThread:public QThread
{
public:
    gThread(gGraphView *g);
    ~gThread();

    //! \brief Start thread process
    void run();

    //! \brief Kill thread process
    void die() { m_running=false; }
    QMutex mutex;
protected:
    gGraphView *graphview;
    volatile bool m_running;
};

/*! \class gToolTip
    \brief Popup Tooltip to display information over the OpenGL graphs
    */
class gToolTip: public QObject
{
    Q_OBJECT
public:
    //! \brief Initializes the ToolTip object, and connects the timeout to the gGraphView object
    gToolTip(gGraphView * graphview);
    virtual ~gToolTip();

    /*! \fn virtual void display(QString text, int x, int y, int timeout=2000);
        \brief Set the tooltips display message, position, and timeout value
        */
    virtual void display(QString text, int x, int y, int timeout=2000);

    //! \brief Queue the actual OpenGL drawing instructions
    virtual void paint(); //actually paints it.

    //! \brief Close the tooltip early.
    void cancel();

    //! \brief Returns true if the tooltip is currently visible
    bool visible() { return m_visible; }
protected:
    gGraphView * m_graphview;
    QTimer * timer;
    QPoint m_pos;
    int tw,th;
    QString m_text;
    bool m_visible;
    int m_spacer;
    QPixmap m_pixmap;
    GLuint m_textureID;
    bool m_invalidate;
protected slots:

    //! \brief Timeout to hide tooltip, and redraw without it.
    void timerDone();
};

/*! \class gGraph
    \brief Single Graph object, containing multiple layers and Layer layout code
    */
class gGraph:public QObject
{
    Q_OBJECT
public:
    friend class gGraphView;

    /*! \brief Creates a new graph object
        \param gGraphView * graphview if not null, links the graph to that object
        \param QString title containing the graph Title which is rendered vertically
        \param int height containing the opening height for this graph
        \param short group containing which graph-link group this graph belongs to
        */
    gGraph(gGraphView * graphview=NULL, QString title="", QString units="", int height=100,short group=0);
    virtual ~gGraph();

    //! \brief Tells all Layers to deselect any highlighting
    void deselect();

    //! \brief Returns true if any Layers have anything highlighted
    bool isSelected();

    //! \brief Starts the singleshot Timer running, for ms milliseconds
    void Trigger(int ms);

    /*! \fn QPixmap renderPixmap(int width, int height, float fontscale=1.0);
        \brief Returns a QPixmap containing a snapshot of the graph rendered at size widthxheight
        \param int width    Width of graph 'screenshot'
        \param int height   Height of graph 'screenshot'
        \param float fontscale Scaling value to adjust DPI (when used for HighRes printing)

        Note if width or height is more than the OpenGL system allows, it could result in a crash
        Keeping them under 2048 is a reasonably safe value.
        */
    QPixmap renderPixmap(int width, int height, bool printing=false);

    //! \brief Set Graph visibility status
    void setVisible(bool b) { m_visible=b; }

    //! \brief Return Graph visibility status
    bool visible() { return m_visible; }

    //! \brief Return height element. This is used by the scaler in gGraphView.
    float height() { return m_height; }

    //! \brief Set the height element. (relative to the total of all heights)
    void setHeight(float height) { m_height=height; invalidate_yAxisImage=true;}

    int minHeight() { return m_min_height; }
    void setMinHeight(int height) { m_min_height=height; }

    int maxHeight() { return m_max_height; }
    void setMaxHeight(int height) { m_max_height=height; }

    //! \brief Returns true if the vertical graph title is shown
    bool showTitle() { return m_showTitle; }

    //! \brief Set whether or not to render the vertical graph title
    void setShowTitle(bool b) { m_showTitle=b; }

    //! \brief Returns printScaleX, used for DPI scaling in report printing
    float printScaleX();

    //! \brief Returns printScaleY, used for DPI scaling in report printing
    float printScaleY();

    //! \brief Returns true if none of the included layers have data attached
    bool isEmpty();

    //! \brief Add Layer l to graph object, allowing you to specify position,  margin sizes, order, movability status and offsets
    void AddLayer(Layer * l,LayerPosition position=LayerCenter, short pixelsX=0, short pixelsY=0, short order=0, bool movable=false, short x=0, short y=0);


    void qglColor(QColor col);

    //! \brief Queues text for gGraphView object to draw it.
    void renderText(QString text, int x,int y, float angle=0.0, QColor color=Qt::black, QFont *font=defaultfont,bool antialias=true);

    //! \brief Rounds Y scale values to make them look nice.. Applies the Graph Preference min/max settings.
    void roundY(EventDataType &miny, EventDataType &maxy);

    //! \brief Process all Layers GLBuffer (Vertex) objects, drawing the actual OpenGL stuff.
    void drawGLBuf();

    //! \brief Returns the Graph's (vertical) title
    QString title() { return m_title; }

    //! \brief Sets the Graph's (vertical) title
    void setTitle(const QString title) { m_title=title; }

    //! \brief Returns the measurement Units the Y scale is referring to
    QString units() { return m_units; }

    //! \brief Sets the measurement Units the Y scale is referring to
    void setUnits(const QString units) { m_units=units; }

    //virtual void repaint(); // Repaint individual graph..

    //! \brief Resets the graphs X & Y dimensions based on the Layer data
    virtual void ResetBounds();

    //! \brief Sets the time range selected for this graph (in milliseconds since 1970 epoch)
    virtual void SetXBounds(qint64 minx, qint64 maxx);

    //! \brief Returns the physical Minimum time for all layers contained (in milliseconds since epoch)
    virtual qint64 MinX();

    //! \brief Returns the physical Maximum time for all layers contained (in milliseconds since epoch)
    virtual qint64 MaxX();

    //! \brief Returns the physical Minimum Y scale value for all layers contained
    virtual EventDataType MinY();

    //! \brief Returns the physical Maximum Y scale value for all layers contained
    virtual EventDataType MaxY();

    //! \brief Sets the physical start of this graphs time range (in milliseconds since epoch)
    virtual void SetMinX(qint64 v);

    //! \brief Sets the physical end of this graphs time range (in milliseconds since epoch)
    virtual void SetMaxX(qint64 v);

    //! \brief Sets the physical Minimum Y scale value used while drawing this graph
    virtual void SetMinY(EventDataType v);

    //! \brief Sets the physical Maximum Y scale value used while drawing this graph
    virtual void SetMaxY(EventDataType v);

    //! \brief Forces Y Minimum to always select this value
    virtual void setForceMinY(EventDataType v) { f_miny=v; m_enforceMinY=true; }
    //! \brief Forces Y Maximum to always select this value
    virtual void setForceMaxY(EventDataType v) { f_maxy=v; m_enforceMaxY=true; }


    virtual EventDataType forceMinY() { return rec_miny; }
    virtual EventDataType forceMaxY() { return rec_maxy; }

    //! \brief Set recommended Y minimum.. It won't go under this unless the data does. It won't go above this.
    virtual void setRecMinY(EventDataType v) { rec_miny=v; }
    //! \brief Set recommended Y minimum.. It won't go above this unless the data does. It won't go under this.
    virtual void setRecMaxY(EventDataType v) { rec_maxy=v; }

    //! \brief Returns the recommended Y minimum.. It won't go under this unless the data does. It won't go above this.
    virtual EventDataType RecMinY() { return rec_miny; }
    //! \brief Returns the recommended Y maximum.. It won't go under this unless the data does. It won't go above this.
    virtual EventDataType RecMaxY() { return rec_maxy; }

    //! \brief Called when main graph area is resized
    void resize(int width, int height);      // margin recalcs..

    qint64 max_x,min_x,rmax_x,rmin_x;
    EventDataType max_y,min_y,rmax_y,rmin_y, f_miny, f_maxy, rec_miny, rec_maxy;

    // not sure why there's two.. I can't remember
    void setEnforceMinY(bool b) { m_enforceMinY=b; }
    void setEnforceMaxY(bool b) { m_enforceMaxY=b; }

    //! \brief Returns whether this graph shows overall timescale, or a zoomed area
    bool blockZoom() { return m_blockzoom; }
    //! \brief Sets whether this graph shows an overall timescale, or a zoomed area.
    void setBlockZoom(bool b) { m_blockzoom=b; }

    //! \brief Flips the GL coordinates from the graphs perspective.. Used in Scissor calculations
    int flipY(int y); // flip GL coordinates

    //! \brief Returns the graph-linking group this Graph belongs in
    short group() { return m_group; }

    //! \brief Sets the graph-linking group this Graph belongs in
    void setGroup(short group) { m_group=group; }

    //! \brief Forces the main gGraphView object to draw all Text Components
    void DrawTextQue();

    //! \brief Sends supplied day object to all Graph layers so they can precalculate stuff
    void setDay(Day * day);

    //! \brief Returns the current day object
    Day * day() { return m_day; }

    //! \brief The Layer, layout and title drawing code
    virtual void paint(int originX, int originY, int width, int height);

    //! \brief Gives the supplied data to the main ToolTip object for display
    void ToolTip(QString text, int x, int y, int timeout=2000);

    //! \brief Public version of updateGL(), to redraw all graphs.. Not for normal use
    void redraw();

    //! \brief Asks the main gGraphView to redraw after ms milliseconds
    void timedRedraw(int ms);

    //! \brief Sets the margins for the four sides of this graph.
    void setMargins(short left,short right,short top,short bottom) {
        m_marginleft=left; m_marginright=right;
        m_margintop=top; m_marginbottom=bottom;
    }

    //! \brief Returns this graphs left margin
    short marginLeft();
    //! \brief Returns this graphs right margin
    short marginRight();
    //! \brief Returns this graphs top margin
    short marginTop();
    //! \brief Returns this graphs bottom margin
    short marginBottom();

    //! \brief Returns the main gGraphView objects gVertexBuffer line list.
    gVertexBuffer * lines();
    //! \brief Returns the main gGraphView objects gVertexBuffer background line list.
    gVertexBuffer * backlines();
    //! \brief Returns the main gGraphView objects gVertexBuffer front line list.
    gVertexBuffer * frontlines();
    //! \brief Returns the main gGraphView objects gVertexBuffer quads list.
    gVertexBuffer * quads();

    // //! \brief Returns the main gGraphView objects gVertexBuffer stippled line list.
    //GLShortBuffer * stippled();

    //gVertexBuffer * vlines(); // testing new vertexbuffer

    short left,right,top,bottom; // dirty magin hacks..

    Layer * getLineChart();
    QRect m_lastbounds;
    QTimer * timer;

    // This gets set to true to force a redraw of the yAxis tickers when graphs are resized.
    bool invalidate_yAxisImage;
    bool invalidate_xAxisImage;

    //! \brief Returns a Vector reference containing all this graphs layers
    QVector<Layer *>  & layers() { return m_layers; }

    gGraphView * graphView() { return m_graphview; }
    short m_marginleft, m_marginright, m_margintop, m_marginbottom;
protected:
    //void invalidate();

    //! \brief Mouse Wheel events
    virtual void wheelEvent(QWheelEvent * event);

    //! \brief Mouse Movement events
    virtual void mouseMoveEvent(QMouseEvent * event);

    //! \brief Mouse Button Pressed events
    virtual void mousePressEvent(QMouseEvent * event);

    //! \brief Mouse Button Released events
    virtual void mouseReleaseEvent(QMouseEvent * event);

    //! \brief Mouse Button Double Clicked events
    virtual void mouseDoubleClickEvent(QMouseEvent * event);

    //! \brief Key Pressed event
    virtual void keyPressEvent(QKeyEvent * event);

    //! \brief Change the current selected time boundaries by mult, from origin position origin_px
    void ZoomX(double mult,int origin_px);

    //! \brief The Main gGraphView object holding this graph (this can be pinched temporarily by print code)
    gGraphView * m_graphview;
    QString m_title;
    QString m_units;

    //! \brief Vector containing all this graphs Layers
    QVector<Layer *> m_layers;
    float m_height,m_width;

    int m_min_height;
    int m_max_height;
    bool m_visible;
    bool m_blockzoom;
    QRect m_selection;
    bool m_selecting_area;
    QPoint m_current;
    short m_group;
    short m_lastx23;
    Day * m_day;
    gVertexBuffer * m_quad;
    bool m_enforceMinY,m_enforceMaxY;
    bool m_showTitle;
    bool m_printing;
signals:

protected slots:
    //! \brief Deselects any highlights, and schedules a main gGraphView redraw
    void Timeout();
};

/*! \struct myPixmapCache
    \brief My version of Pixmap cache with texture binding support

 */
struct myPixmapCache
{
    quint64 last_used;
    QPixmap *pixmap;
    GLuint textureID;
};


/*! \class gGraphView
    \brief Main OpenGL Graph Area, derived from QGLWidget

    This widget contains a list of graphs, and provides the means to display them, scroll via an attached
    scrollbar, change the order around, and resize individual graphs.

    It replaced QScrollArea and multiple QGLWidgets graphs, and a very buggy QSplitter.

    It led to quite a performance increase over the old Qt method.

    */
class gGraphView : public QGLWidget
{
    Q_OBJECT
public:
    /*! \fn explicit gGraphView(QWidget *parent = 0,gGraphView * shared=0);
        \brief Constructs a new gGraphView object (main graph area)
        \param QWidget * parent
        \param gGraphView * shared

        The shared parameter allows for OpenGL context sharing.
        But this must not be shared with Printers snapshot gGraphView object,
        or it will lead to display/font corruption
        */
    explicit gGraphView(QWidget *parent = 0,gGraphView * shared=0);
    virtual ~gGraphView();

    //! \brief Add gGraph g to this gGraphView, in the requested graph-linkage group
    void addGraph(gGraph *g,short group=0);

    //! \brief The width of the vertical text Title area for all graphs
    static const int titleWidth=30;

    //! \brief The splitter is drawn inside this gap
    static const int graphSpacer=4;

    //! \brief Finds the top pixel position of the supplied graph
    float findTop(gGraph * graph);

    //! \brief Returns the scaleY value, which is used when laying out less graphs than fit on the screen.
    float scaleY() { return m_scaleY; }

    //! \brief Sets the scaleY value, which is used when laying out less graphs than fit on the screen.
    void setScaleY(float sy) { m_scaleY=sy; }

    //! \brief Returns the current selected time range
    void GetXBounds(qint64 & st,qint64 & et);

    //! \brief Returns the maximum time range bounds
    void GetRXBounds(qint64 & st, qint64 & et);

    //! \brief Resets the time range to default for this day. Refreshing the display if refresh==true.
    void ResetBounds(bool refresh=true); //short group=0);

    //! \brief Supplies time range to all graph objects in linked group, refreshing if requested
    void SetXBounds(qint64 minx, qint64 maxx, short group=0,bool refresh=true);

    //! \brief Saves the current graph order, heights, min & Max Y values to disk
    void SaveSettings(QString title);

    //! \brief Loads the current graph order, heights, min & max Y values from disk
    bool LoadSettings(QString title);

    //! \brief Returns the graph object matching the supplied name, NULL if it does not exist.
    gGraph *findGraph(QString name);

    inline float printScaleX() { return print_scaleX; }
    inline float printScaleY() { return print_scaleY; }
    inline void setPrintScaleX(float x) { print_scaleX=x; }
    inline void setPrintScaleY(float y) { print_scaleY=y; }

    //! \brief Returns true if all Graph objects contain NO day data. ie, graph area is completely empty.
    bool isEmpty();

    //! \brief Tell all graphs to deslect any highlighted areas
    void deselect();

    QPoint pointClicked() { return m_point_clicked; }
    QPoint globalPointClicked() { return m_global_point_clicked; }
    void setPointClicked(QPoint p) { m_point_clicked=p; }
    void setGlobalPointClicked(QPoint p) { m_global_point_clicked=p; }

    //! \brief Set a redraw timer for ms milliseconds, clearing any previous redraw timer.
    void timedRedraw(int ms);

    //! \brief Start the animation sequence changing/reloading day data. (fade out)
    void fadeOut();

    //! \brief Start the animation sequence showing new Day's data. (fade in)
    void fadeIn(bool dir=false);

    //! \brief Call UpdateGL unless animation is in progress
    void redraw();

    gGraph *m_selected_graph;
    gToolTip * m_tooltip;
    QTimer * timer;

    //! \brief Show the current selection time in the statusbar area
    void selectionTime();

    //! \brief Add the Text information to the Text Drawing Queue (called by gGraphs renderText method)
    void AddTextQue(QString & text, short x, short y, float angle=0.0, QColor color=Qt::black, QFont * font=defaultfont, bool antialias=true);

    //! \brief Draw all Text in the text drawing queue, via QPainter
    void DrawTextQue();

    //! \brief Returns number of graphs contained (whether they are visible or not)
    int size() { return m_graphs.size(); }

    //! \brief Return individual graph by index value
    gGraph * operator[](int i) { return m_graphs[i]; }

    //! \brief Returns the custom scrollbar object linked to this gGraphArea
    MyScrollBar * scrollBar() { return m_scrollbar; }
    //! \brief Sets the custom scrollbar object linked to this gGraphArea
    void setScrollBar(MyScrollBar *sb);

    //! \brief Calculates the correct scrollbar parameters for all visible, non empty graphs.
    void updateScrollBar();

    //! \brief Called on resize, fits graphs when too few to show, by scaling to fit screen size. Calls updateScrollBar()
    void updateScale();         // update scale & Scrollbar

    //! \brief Resets all contained graphs to have a uniform height.
    void resetLayout();

    //! \brief Returns a count of all visible, non-empty Graphs.
    int visibleGraphs();

    //! \brief Returns the horizontal travel of the mouse, for use in Mouse Handling code.
    int horizTravel() { return m_horiz_travel; }

    //! \brief Sets the message displayed when there are no graphs to draw
    void setEmptyText(QString s) { m_emptytext=s; }

    void setCubeImage(QImage *);

    // Cube fun
    QVector<QImage *> cubeimg;
    GLuint cubetex;

#ifdef ENABLE_THREADED_DRAWING
    QMutex text_mutex;
    QMutex gl_mutex;
    QSemaphore * masterlock;
    bool useThreads() { return m_idealthreads>1; }
    QVector<gThread *> m_threads;
    int m_idealthreads;
    QMutex dl_mutex;
#endif



    //! \brief Sends day object to be distributed to all Graphs Layers objects
    void setDay(Day * day);

    gVertexBuffer *lines, *backlines, *quads, *frontlines;

    //! \brief pops a graph off the list for multithreaded drawing code
    gGraph * popGraph(); // exposed for multithreaded drawing

    //! \brief Hides the splitter, used in report printing code
    void hideSplitter() { m_showsplitter=false; }

    //! \brief Re-enabled the in-between graph splitters.
    void showSplitter() { m_showsplitter=true; }

    //! \brief Trash all graph objects listed (without destroying Graph contents)
    void trashGraphs();

    //! \brief Use a QGLFrameBufferObject to render to a pixmap
    QImage fboRenderPixmap(int w,int h);

    //! \brief Use a QGLPixelBuffer to render to a pixmap
    QImage pbRenderPixmap(int w,int h);

    //! \brief Enable or disable the Text Pixmap Caching system preference overide
    void setUsePixmapCache(bool b) { use_pixmap_cache=b; }

    //! \brief Return whether or not the Pixmap Cache for text rendering is being used.
    bool usePixmapCache();

protected:
    //! \brief Set up the OpenGL basics for the QGLWidget underneath
    virtual void initializeGL();

    //! \brief Resize the OpenGL ViewPort prior to redrawing
    virtual void resizeGL(int width, int height);

    //! \brief The heart of the OpenGL drawing code
    virtual void paintGL();


    Day * m_day;

    //! \brief Calculates the sum of all graph heights
    float totalHeight();

    //! \brief Calculates the sum of all graph heights, taking scaling into consideration
    float scaleHeight();



    //! \brief Graph drawing routines, returns true if there weren't any graphs to draw
    bool renderGraphs();


    //! \brief Update the OpenGL area when the screen is resized
    virtual void resizeEvent(QResizeEvent *);

    //! \brief Set the Vertical offset (used in scrolling)
    void setOffsetY(int offsetY);

    //! \brief Set the Horizontal offset (not used yet)
    void setOffsetX(int offsetX);

    //! \brief Mouse Moved somewhere in main gGraphArea, propagates to the individual graphs
    virtual void mouseMoveEvent(QMouseEvent * event);
    //! \brief Mouse Button Press Event somewhere in main gGraphArea, propagates to the individual graphs
    virtual void mousePressEvent(QMouseEvent * event);
    //! \brief Mouse Button Release Event somewhere in main gGraphArea, propagates to the individual graphs
    virtual void mouseReleaseEvent(QMouseEvent * event);
    //! \brief Mouse Button Double Click Event somewhere in main gGraphArea, propagates to the individual graphs
    virtual void mouseDoubleClickEvent(QMouseEvent * event);
    //! \brief Mouse Wheel Event somewhere in main gGraphArea, propagates to the individual graphs
    virtual void wheelEvent(QWheelEvent * event);
    //! \brief Keyboard event while main gGraphArea has focus.
    virtual void keyPressEvent(QKeyEvent * event);

    //! \brief Add Graph to drawing queue, mainly for the benefit of multithreaded drawing code
    void queGraph(gGraph *,int originX, int originY, int width, int height);

    //! \brief the list of graphs to draw this frame
    QList<gGraph *> m_drawlist;

    //! \brief Linked graph object containing shared GL Context (in this app, daily view's gGraphView)
    gGraphView *m_shared;

    //! \brief List of all graphs contained in this area
    QVector<gGraph *> m_graphs;

    //! \brief List of all graphs contained, indexed by title
    QHash<QString,gGraph*> m_graphsbytitle;

    //! \variable Vertical scroll offset (adjusted when scrollbar gets moved)
    int m_offsetY;
    //! \variable Horizontal scroll offset (unused, but can be made to work if necessary)
    int m_offsetX;
    //! \variable Scale used to enlarge graphs when less graphs than can fit on screen.
    float m_scaleY;

    void renderSomethingFun(float alpha=1);

    bool m_sizer_dragging;
    int m_sizer_index;

    bool m_button_down;
    QPoint m_point_clicked;
    QPoint m_global_point_clicked;
    QPoint m_sizer_point;
    int m_horiz_travel;

    MyScrollBar * m_scrollbar;
    QTimer *redrawtimer;

    bool m_graph_dragging;
    int m_graph_index;

    //! \brief List of all queue text to draw.. not sure why I didn't use a vector here.. Might of been a leak issue
    TextQue m_textque[textque_max];

    int m_textque_items;
    int m_lastxpos,m_lastypos;

    QString m_emptytext;
    bool m_showsplitter;

    qint64 m_minx,m_maxx;
    float print_scaleX,print_scaleY;

    QPixmap previous_day_snapshot;
    QPixmap current_day_snapshot;
    bool m_fadingOut;
    bool m_fadingIn;
    bool m_limbo;
    bool m_fadedir;
    bool m_inAnimation;
    bool m_blockUpdates;

    QPoint m_mouse;

    QTime m_animationStarted;

    // turn this into a struct later..
    QHash<QString,myPixmapCache *> pixmap_cache;
    qint32 pixmap_cache_size;
    bool use_pixmap_cache;


    //QVector<GLuint> texid;
signals:


public slots:
    //! \brief Callback from the ScrollBar, to change scroll position
    void scrollbarValueChanged(int val);

    //! \brief Simply refreshes the GL view, called when timeout expires.
    void refreshTimeout();

};

#endif // GGRAPHVIEW_H
