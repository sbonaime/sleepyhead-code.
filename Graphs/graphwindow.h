/********************************************************************
 gGraphWindow Headers
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*********************************************************************/

#ifndef GRAPHWINDOW_H
#define GRAPHWINDOW_H

#include <QGLContext>
#include <QGLWidget>
#include <QDateTime>
#include <list>
using namespace std;
#include "graphlayer.h"
#include "glcommon.h"

#define MIN(a,b) (((a)<(b)) ? (a) : (b));
#define MAX(a,b) (((a)<(b)) ? (b) : (a));


class gLayer;

class gGraphWindow : public QGLWidget
{
    Q_OBJECT
public:
    explicit gGraphWindow(QWidget *parent, const QString & title, QGLContext * context,Qt::WindowFlags f=0);
    explicit gGraphWindow(QWidget *parent, const QString & title, QGLWidget * shared,Qt::WindowFlags f=0);
    virtual ~gGraphWindow();

signals:
public slots:

public:
    QBitmap * RenderBitmap(int width,int height);

    virtual void paintGL();
    virtual void resizeGL(int width, int height);

    //virtual void OnMouseWheel(wxMouseEvent &event);
    virtual void mouseMoveEvent(QMouseEvent * event);
    virtual void mousePressEvent(QMouseEvent * event);
    virtual void mouseReleaseEvent(QMouseEvent * event);
    virtual void mouseDoubleClickEvent(QMouseEvent * event);
    //virtual void OnMouseRightDClick(wxMouseEvent * event);
    virtual void OnMouseLeftDown(QMouseEvent * event);
    virtual void OnMouseLeftRelease (QMouseEvent * event);
    virtual void OnMouseRightDown(QMouseEvent * event);
    virtual void OnMouseRightRelease(QMouseEvent * event);

    int GetScrX(void) const { return m_scrX; };
    int GetScrY(void) const { return m_scrY; };

    // For mouse to screen use only.. work in OpenGL points where possible
    const QString & Title(void ) { return m_title; };

      void SetMargins(float top, float right, float bottom, float left);  // OpenGL width of each corners margin

      float GetTopMargin(void) const { return m_marginTop; };
      float GetBottomMargin(void) const { return m_marginBottom; };
      float GetLeftMargin(void) const { return m_marginLeft; };
      float GetRightMargin(void) const { return m_marginRight; };

      void SetTopMargin(float i) { m_marginTop=i; };
      void SetBottomMargin(float i) { m_marginBottom=i; };
      void SetLeftMargin(float i) { m_marginLeft=i; };
      void SetRightMargin(float i) { m_marginRight=i; };

      inline float Width() { return m_scrX-m_marginLeft-m_marginRight; };  // Width of OpenGL main drawing area
      inline int Height() { return m_scrY-m_marginTop-m_marginBottom; };   // Height of ""...

      void LinkZoom(gGraphWindow *g) { link_zoom.push_back(g); }; // Linking graphs changes zoom behaviour..
      //void LinkMove(gGraphWindow *g) { link_move.push_back(g); }; // Linking graphs changes zoom behaviour..

      virtual double MinX();
      virtual double MaxX();
      virtual double MinY();
      virtual double MaxY();

      virtual double RealMinX();
      virtual double RealMaxX();
      virtual double RealMinY();
      virtual double RealMaxY();

      virtual void SetMinX(double v);
      virtual void SetMaxX(double v);
      virtual void SetMinY(double v);
      virtual void SetMaxY(double v);

      virtual void ResetXBounds();
      virtual void SetXBounds(double minx, double maxx);
      virtual void ZoomX(double mult,int origin_px);

      virtual void ZoomXPixels(int x1, int x2);           // Zoom between two selected points on screen
      virtual void ZoomXPixels(int x1,int x2,double &rx1,double &rx2);

      virtual void MoveX(int i);                          // Move x bounds by i Pixels
      virtual void MoveX(int i,double &min, double & max);

      inline float x2p(double x) {
          double xx=max_x-min_x;
          double wid=Width();
          double w=((wid/xx)*(x-min_x));
          return w+GetLeftMargin();
      };
      inline double p2x(float px) {
          double xx=max_x-min_x;
          double wx=px-GetLeftMargin();
          double ww=wx/Width();
          return min_x+(xx*ww);
      };
      inline int y2p(double y) {
          double yy=max_y-min_y;
          double h=(Height()/yy)*(y-min_y);
          return h+GetBottomMargin();
      };
      inline double p2y(float py) {
          double yy=max_y-min_y;
          double hy=py-GetBottomMargin();
          double hh=hy/Height();
          return min_y+(yy*hh);
      };

      void Render(float scrx,float scry);

      //virtual void Update();
      //virtual void Update();
      void AddLayer(gLayer *l);

      virtual void DataChanged(gLayer *layer);

      double max_x,min_x,max_y,min_y;
      double rmax_x,rmin_x,rmax_y,rmin_y;

      void SetBlockZoom(bool b) { m_block_zoom=b; };
      //void SetBlockMove(bool b) { m_block_move=b; };
      bool BlockZoom() { return m_block_zoom; };
      QGLContext *gl_context;
      //FTFont *texfont;
      void SetGradientBackground(bool b) { m_gradient_background=b; };
      bool GradientBackground() { return m_gradient_background; };

  protected:
      void initializeGL();
      list<gGraphWindow *>link_zoom;
      //list<gGraphWindow *>link_move;

      //bool m_block_move;
      bool m_block_zoom;
      bool m_drag_foobar;
      double m_foobar_pos,m_foobar_moved;
      bool m_gradient_background;
      std::list<gLayer *> layers;
      QString m_title;
      int    m_scrX;
      int    m_scrY;
      QPoint m_mouseLClick,m_mouseRClick,m_mouseRClick_start;

      float m_marginTop, m_marginRight, m_marginBottom, m_marginLeft;

      QRect m_mouseRBrect,m_mouseRBlast;
      bool m_mouseLDown,m_mouseRDown,m_datarefresh;

      gLayer *foobar;
      gLayer *xaxis;
      gLayer *yaxis;
      gLayer *gtitle;

      QDateTime ti;
      gLayer *lastlayer;

};

#endif // GRAPHWINDOW_H
