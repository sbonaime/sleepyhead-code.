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
//    GetTextExtent(m_text,x,y);
    int z=(width+gYAxis::Margin)/5;
    int p=left-gYAxis::Margin;


    top+=4;
    w.DrawStaticText(st_label,p,top);

    //w.renderText(m_text,p,top,0,m_textcolor);

    p+=z;
    w.DrawStaticText(st_min,p,top);

    p+=z;
    w.DrawStaticText(st_avg,p,top);

    p+=z;
    w.DrawStaticText(st_p90,p,top);

    p+=z;
    w.DrawStaticText(st_max,p,top);

}


void gStatsLine::SetDay(Day *d)
{
    Layer::SetDay(d);
    if (!m_day) return;
    m_min=d->min(m_code);
    m_max=d->max(m_code);
    m_avg=d->wavg(m_code);
    m_p90=d->p90(m_code);

    st_label.setText(m_label);
    st_min.setText("Min="+QString::number(m_min,'f',2));
    st_max.setText("Max="+QString::number(m_max,'f',2));
    st_avg.setText("Avg="+QString::number(m_avg,'f',2));
    st_p90.setText("90%="+QString::number(m_p90,'f',2));

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
