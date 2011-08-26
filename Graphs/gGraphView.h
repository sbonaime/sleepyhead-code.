#ifndef GGRAPHVIEW_H
#define GGRAPHVIEW_H

#include <QGLWidget>
#include <QScrollBar>
#include <QResizeEvent>
#include <SleepLib/day.h>
#include <Graphs/glcommon.h>


#define MIN(a,b) (((a)<(b)) ? (a) : (b));
#define MAX(a,b) (((a)<(b)) ? (b) : (a));

class gGraphView;
class gGraph;

const int textque_max=2048;

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


protected:
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

protected:
    QVector<Layer *> layers;
};

class gGraph
{
    friend class gGraphView;
public:
    gGraph(gGraphView * graphview=NULL, QString title="",int height=100);
    virtual ~gGraph();

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

    QString title() { return m_title; }

    virtual void repaint(); // Repaint individual graph..

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

    void resize(int width, int height);      // margin recalcs..

    qint64 max_x,min_x,max_y,min_y;
    qint64 rmax_x,rmin_x,rmax_y,rmin_y;

    bool blockZoom() { return m_blockzoom; }
    void setBlockZoom(bool b) { m_blockzoom=b; }
    int flipY(int y); // flip GL coordinates

protected:
    virtual void paint(int originX, int originY, int width, int height);
    void invalidate();

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

    int m_marginleft, m_marginright, m_margintop, m_marginbottom;

    int left,right,top,bottom; // dirty magin hacks..

    int m_min_height;
    int m_max_height;
    bool m_visible;
    bool m_blockzoom;
    QRect m_lastbounds;
    QRect m_selection;
    bool m_selecting_area;
    QPoint m_current;
};

class gGraphView : public QGLWidget
{
    Q_OBJECT
public:
    explicit gGraphView(QWidget *parent = 0);
    virtual ~gGraphView();
    void AddGraph(gGraph *g);

    void setScrollBar(MyScrollBar *sb);
    MyScrollBar * scrollBar() { return m_scrollbar; }
    static const int titleWidth=30;
    static const int graphSpacer=5;

    float findTop(gGraph * graph);

    float scaleY() { return m_scaleY; }

    void ResetBounds();
    void SetXBounds(qint64 minx, qint64 maxx);

    bool hasGraphs() { return m_graphs.size()>0; }

    QPoint pointClicked() { return m_point_clicked; }
    QPoint globalPointClicked() { return m_global_point_clicked; }
    void setPointClicked(QPoint p) { m_point_clicked=p; }
    void setGlobalPointClicked(QPoint p) { m_global_point_clicked=p; }

    gGraph *m_selected_graph;

    void AddTextQue(QString & text, short x, short y, float angle, QColor & color, QFont * font);
protected:

    void DrawTextQue();

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

    void updateScrollBar();
    void updateScale();         // update scale & Scrollbar

    QVector<gGraph *> m_graphs;
    int m_offsetY,m_offsetX;          // Scroll Offsets
    float m_scaleY;

    bool m_sizer_dragging;
    int m_sizer_index;

    bool m_button_down;
    QPoint m_point_clicked;
    QPoint m_global_point_clicked;
    QPoint m_sizer_point;

    MyScrollBar * m_scrollbar;

    bool m_graph_dragging;
    int m_graph_index;

    TextQue m_textque[textque_max];
    int m_textque_items;
signals:


public slots:
    void scrollbarValueChanged(int val);

};

#endif // GGRAPHVIEW_H
