#include "SleepLib/day.h"
#include "gYAxis.h"
#include "gStatsLine.h"

gStatsLine::gStatsLine(ChannelID code,QString label,QColor textcolor)
    :Layer(code),m_label(label),m_textcolor(textcolor)
{
}
void gStatsLine::paint(gGraph & w, int left, int top, int width, int height)
{
    if (!m_visible) return;
    //if (m_empty) return;

    float x,y;
    m_text=m_label;
    GetTextExtent(m_text,x,y);
    int z=(width+gYAxis::Margin)/5;
    int p=left-gYAxis::Margin;


    top+=8+y;
    w.renderText(m_text,p,top,0,m_textcolor);

    p+=z;
    m_text="Min="+QString::number(m_min,'f',2);
    GetTextExtent(m_text,x,y);
    w.renderText(m_text,p,top,0,m_textcolor);

    p+=z;
    m_text="Avg="+QString::number(m_avg,'f',2);
    GetTextExtent(m_text,x,y);
    w.renderText(m_text,p,top,0,m_textcolor);

    p+=z;
    m_text="90%="+QString::number(m_p90,'f',2);
    GetTextExtent(m_text,x,y);
    w.renderText(m_text,p,top,0,m_textcolor);

    p+=z;
    m_text="Max="+QString::number(m_max,'f',2);
    GetTextExtent(m_text,x,y);
    w.renderText(m_text,p,top,0,m_textcolor);

//    GetTextExtent(m_text,m_tx,m_ty);

}


void gStatsLine::SetDay(Day *d)
{
    Layer::SetDay(d);
    if (!m_day) return;
    m_min=d->min(m_code);
    m_max=d->max(m_code);
    m_avg=d->wavg(m_code);
    m_p90=d->p90(m_code);

    m_text.clear();

 //   m_stext.setText(m_text);
    //  m_empty=true;
/*    for (int i=0;i<m_codes.size();i++) {
        if (m_day->count(m_codes[i])>0) {
            m_empty=false;
            break;
        }
    } */

}
//bool gStatsLine::isEmpty()
//{
//    return m_empty;
//}
