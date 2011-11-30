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
#include <Graphs/glcommon.h>


#define MIN(a,b) (((a)<(b)) ? (a) : (b));
#define MAX(a,b) (((a)<(b)) ? (b) : (a));
enum FlagType { FT_Bar, FT_Dot, FT_Span };

//const int default_height=160;

void InitGraphs();
void DoneGraphs();

extern QFont * defaultfont;
extern QFont * mediumfont;
extern QFont * bigfont;

void GetTextExtent(QString text, int & width, int & height, QFont *font=defaultfont);

class gGraphView;
class gGraph;

const int textque_max=512;

class GLBuffer
{
public:
    GLBuffer(int max=2048,int type=GL_LINES);
    virtual ~GLBuffer();
    void scissor(GLshort x1, GLshort y1, GLshort x2, GLshort y2) { s1=x1; s2=y1; s3=x2; s4=y2; m_scissor=true; }
    virtual void draw(){}
    void reset() { m_cnt=0; }
    int max() { return m_max; }
    int cnt() { return m_cnt; }
    bool full() { return m_cnt>=m_max; }
    void setSize(float f) { m_size=f; }
    void setAntiAlias(bool b) { m_antialias=b; }
    void forceAntiAlias(bool b) { m_forceantialias=b; }
    void setColor(QColor col) { m_color=col; }
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
};

class GLShortBuffer:public GLBuffer
{
public:
    GLShortBuffer(int max=2048,int type=GL_LINES);
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

class GLFloatBuffer:public GLBuffer
{
public:
    GLFloatBuffer(int max=2048,int type=GL_LINES);
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

struct TextQue
{
    short x,y;
    float angle;
    QString text;
    QColor color;
    QFont *font;
};

class MyScrollBar:public QScrollBar
{
public:
    MyScrollBar(QWidget * parent=0);
    void SendWheelEvent(QWheelEvent * e);
};

enum LayerPosition { LayerLeft, LayerRight, LayerTop, LayerBottom, LayerCenter, LayerOverlay };

class Layer
{
    friend class gGraph;
public:
    Layer(ChannelID code);
    virtual ~Layer();

    virtual void SetDay(Day * d);
    virtual void SetCode(ChannelID c) { m_code=c; }
    const ChannelID & code() { return m_code; }
    virtual bool isEmpty();

    virtual void deselect() { }

    virtual qint64 Minx() { if (m_day) return m_day->first(); return m_minx; }
    virtual qint64 Maxx() { if (m_day) return m_day->last(); return m_maxx; }
    virtual EventDataType Miny() { return m_miny; }
    virtual EventDataType Maxy() { return m_maxy; }
    virtual void setMinY(EventDataType val) { m_miny=val; }
    virtual void setMaxY(EventDataType val) { m_maxy=val; }
    virtual void setMinX(qint64 val) { m_minx=val; }
    virtual void setMaxX(qint64 val) { m_maxx=val; }

    void setVisible(bool b) { m_visible=b; }
    bool visible() { return m_visible; }

    void setMovable(bool b) { m_movable=b; }
    bool movable() { return m_movable; }

    virtual void paint(gGraph & gv,int left,int top,int width, int height)=0;

    void setLayout(LayerPosition position, short width, short height, short order);
    void setPos(short x, short y) { m_X=x; m_Y=y; }

    int Width() { return m_width; }
    int Height() { return m_height; }

    LayerPosition position() { return m_position; }
    //void X() { return m_X; }
    //void Y() { return m_Y; }


    virtual void drawGLBuf();
    short m_refcount;
    void addref() { m_refcount++; }
    bool unref() { m_refcount--; if (m_refcount<=0) return true; return false; }
protected:
    void addGLBuf(GLBuffer *buf) { mgl_buffers.push_back(buf); }
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
    QVector<GLBuffer *> mgl_buffers;

    // Default layer mouse handling = Do nothing
    virtual bool wheelEvent(QWheelEvent * event) { Q_UNUSED(event); return false; }
    virtual bool mouseMoveEvent(QMouseEvent * event) { Q_UNUSED(event); return false; }
    virtual bool mousePressEvent(QMouseEvent * event) { Q_UNUSED(event); return false; }
    virtual bool mouseReleaseEvent(QMouseEvent * event) { Q_UNUSED(event); return false; }
    virtual bool mouseDoubleClickEvent(QMouseEvent * event) { Q_UNUSED(event); return false; }
    virtual bool keyPressEvent(QKeyEvent * event) { Q_UNUSED(event); return false; }
};

class LayerGroup:public Layer
{
public:
    LayerGroup();
    virtual ~LayerGroup();
    virtual void AddLayer(Layer *l);

    virtual qint64 Minx();
    virtual qint64 Maxx();
    virtual EventDataType Miny();
    virtual EventDataType Maxy();
    virtual bool isEmpty();
    virtual void SetDay(Day * d);
    virtual void drawGLBuf();

protected:
    QVector<Layer *> layers;

    //overide mouse handling to pass to sublayers..
};

class gGraph;

class gThread:public QThread
{
public:
    gThread(gGraphView *g);
    ~gThread();

    void run();
    void die() { m_running=false; }
    QMutex mutex;
protected:
    gGraphView *graphview;
    volatile bool m_running;
};

class gToolTip: public QObject
{
    Q_OBJECT
public:
    gToolTip(gGraphView * graphview);
    virtual ~gToolTip();
    virtual void display(QString text, int x, int y, int timeout=2000);
    virtual void paint(); //actually paints it.
    void cancel();
protected:
    gGraphView * m_graphview;
    QTimer * timer;
    QPoint m_pos;
    int tw,th;
    QString m_text;
    bool m_visible;
    int m_spacer;
protected slots:
    void timerDone();
};

class gGraph:public QObject
{
    Q_OBJECT
public:
    friend class gGraphView;

    //gGraph();
    gGraph(gGraphView * graphview=NULL, QString title="",int height=100,short group=0);
    virtual ~gGraph();
    void deselect();
    void Trigger(int ms);

    void setVisible(bool b) { m_visible=b; }
    bool visible() { return m_visible; }

    float height() { return m_height; }
    void setHeight(float height) { m_height=height; }

    int minHeight() { return m_min_height; }
    void setMinHeight(int height) { m_min_height=height; }

    int maxHeight() { return m_max_height; }
    void setMaxHeight(int height) { m_max_height=height; }

    bool isEmpty();

    void AddLayer(Layer * l,LayerPosition position=LayerCenter, short pixelsX=0, short pixelsY=0, short order=0, bool movable=false, short x=0, short y=0);

    void qglColor(QColor col);
    void renderText(QString text, int x,int y, float angle=0.0, QColor color=Qt::black, QFont *font=defaultfont);
    void roundY(EventDataType &miny, EventDataType &maxy);

    void drawGLBuf();
    QString title() { return m_title; }

    //virtual void repaint(); // Repaint individual graph..

    virtual void ResetBounds();
    virtual void SetXBounds(qint64 minx, qint64 maxx);

    virtual qint64 MinX();
    virtual qint64 MaxX();
    virtual EventDataType MinY();
    virtual EventDataType MaxY();

    virtual void SetMinX(qint64 v);
    virtual void SetMaxX(qint64 v);
    virtual void SetMinY(EventDataType v);
    virtual void SetMaxY(EventDataType v);
    virtual void setForceMinY(EventDataType v) { f_miny=v; m_enforceMinY=true; }
    virtual void setForceMaxY(EventDataType v) { f_maxy=v; m_enforceMaxY=true; }
    virtual EventDataType forceMinY() { return rec_miny; }
    virtual EventDataType forceMaxY() { return rec_maxy; }
    virtual void setRecMinY(EventDataType v) { rec_miny=v; }
    virtual void setRecMaxY(EventDataType v) { rec_maxy=v; }
    virtual EventDataType RecMinY() { return rec_miny; }
    virtual EventDataType RecMaxY() { return rec_maxy; }

    void resize(int width, int height);      // margin recalcs..

    qint64 max_x,min_x,rmax_x,rmin_x;
    EventDataType max_y,min_y,rmax_y,rmin_y, f_miny, f_maxy, rec_miny, rec_maxy;
    void setEnforceMinY(bool b) { m_enforceMinY=b; }
    void setEnforceMaxY(bool b) { m_enforceMaxY=b; }
    bool blockZoom() { return m_blockzoom; }
    void setBlockZoom(bool b) { m_blockzoom=b; }
    int flipY(int y); // flip GL coordinates

    short group() { return m_group; }
    void setGroup(short group) { m_group=group; }
    void DrawTextQue();
    void setDay(Day * day);
    virtual void paint(int originX, int originY, int width, int height);
    void ToolTip(QString text, int x, int y, int timeout=2000);
    void redraw();
    void timedRedraw(int ms);
    void setMargins(short left,short right,short top,short bottom) {
        m_marginleft=left; m_marginright=right;
        m_margintop=top; m_marginbottom=bottom;
    }

    GLShortBuffer * lines();
    GLShortBuffer * backlines();
    GLShortBuffer * quads();
    short m_marginleft, m_marginright, m_margintop, m_marginbottom;
    short left,right,top,bottom; // dirty magin hacks..

    QRect m_lastbounds;
    QTimer * timer;

protected:
    //void invalidate();

    virtual void wheelEvent(QWheelEvent * event);
    virtual void mouseMoveEvent(QMouseEvent * event);
    virtual void mousePressEvent(QMouseEvent * event);
    virtual void mouseReleaseEvent(QMouseEvent * event);
    virtual void mouseDoubleClickEvent(QMouseEvent * event);
    virtual void keyPressEvent(QKeyEvent * event);

    void ZoomX(double mult,int origin_px);

    gGraphView * m_graphview;
    QString m_title;
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
    GLBuffer * m_quad;
    bool m_enforceMinY,m_enforceMaxY;
signals:

protected slots:
    void Timeout();
};

class gGraphView : public QGLWidget
{
    Q_OBJECT
public:
    explicit gGraphView(QWidget *parent = 0,gGraphView * shared=0);
    virtual ~gGraphView();
    void addGraph(gGraph *g,short group=0);

    static const int titleWidth=30;
    static const int graphSpacer=4;

    float findTop(gGraph * graph);

    float scaleY() { return m_scaleY; }

    void ResetBounds(bool refresh=true); //short group=0);
    void SetXBounds(qint64 minx, qint64 maxx, short group=0,bool refresh=true);
    void SaveSettings(QString title);
    bool LoadSettings(QString title);

    gGraph *findGraph(QString name);

    //bool hasGraphs() { return m_graphs.size()>0; }

    void deselect();
    QPoint pointClicked() { return m_point_clicked; }
    QPoint globalPointClicked() { return m_global_point_clicked; }
    void setPointClicked(QPoint p) { m_point_clicked=p; }
    void setGlobalPointClicked(QPoint p) { m_global_point_clicked=p; }
    void timedRedraw(int ms);

    gGraph *m_selected_graph;
    gToolTip * m_tooltip;
    QTimer * timer;

    void AddTextQue(QString & text, short x, short y, float angle=0.0, QColor color=Qt::black, QFont * font=defaultfont);
    void DrawTextQue();

    int size() { return m_graphs.size(); }
    gGraph * operator[](int i) { return m_graphs[i]; }

    MyScrollBar * scrollBar() { return m_scrollbar; }
    void setScrollBar(MyScrollBar *sb);
    void updateScrollBar();
    void updateScale();         // update scale & Scrollbar
    void resetLayout();

    int horizTravel() { return m_horiz_travel; }
    void setEmptyText(QString s) { m_emptytext=s; }

#ifdef ENABLE_THREADED_DRAWING
    QMutex text_mutex;
    QMutex gl_mutex;
    QSemaphore * masterlock;
    bool useThreads() { return m_idealthreads>1; }
    QVector<gThread *> m_threads;
    int m_idealthreads;
    QMutex dl_mutex;
#endif
    void setDay(Day * day);
    GLShortBuffer * lines, * backlines, *quads;

    gGraph * popGraph(); // exposed for multithreaded drawing
    void hideSplitter() { m_showsplitter=false; }
    void showSplitter() { m_showsplitter=true; }
    void trashGraphs();
protected:
    Day * m_day;
    float totalHeight();
    float scaleHeight();

    virtual void initializeGL();
    virtual void paintGL();
    virtual void resizeGL(int width, int height);
    virtual void resizeEvent(QResizeEvent *);

    void setOffsetY(int offsetY);
    void setOffsetX(int offsetX);

    virtual void mouseMoveEvent(QMouseEvent * event);
    virtual void mousePressEvent(QMouseEvent * event);
    virtual void mouseReleaseEvent(QMouseEvent * event);
    virtual void mouseDoubleClickEvent(QMouseEvent * event);
    virtual void wheelEvent(QWheelEvent * event);
    virtual void keyPressEvent(QKeyEvent * event);

    void queGraph(gGraph *,int originX, int originY, int width, int height); // que graphs for drawing (used internally by paintGL)
    QList<gGraph *> m_drawlist;

    gGraphView *m_shared;       // convenient link to daily's graphs.
    QVector<gGraph *> m_graphs;
    QHash<QString,gGraph*> m_graphsbytitle;

    int m_offsetY,m_offsetX;          // Scroll Offsets
    float m_scaleY;

    bool m_sizer_dragging;
    int m_sizer_index;

    bool m_button_down;
    QPoint m_point_clicked;
    QPoint m_global_point_clicked;
    QPoint m_sizer_point;
    int m_horiz_travel;

    MyScrollBar * m_scrollbar;

    bool m_graph_dragging;
    int m_graph_index;

    TextQue m_textque[textque_max];
    int m_textque_items;
    int m_lastxpos,m_lastypos;

    QString m_emptytext;
    bool m_showsplitter;
signals:


public slots:
    void scrollbarValueChanged(int val);
    void refreshTimeout();
};

#endif // GGRAPHVIEW_H
