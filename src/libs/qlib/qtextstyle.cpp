/*
 * QTextStyle - text layering
 * 09-12-99: Created!
 * (C) 1999 MarketGraph/RvG
 */
 
#include <qlib/textstyle.h>
#include <qlib/debug.h>
DEBUG_ENABLE

QTextStyle::QTextStyle()
{
  layers=0;
}
QTextStyle::~QTextStyle()
{
}

QTextStyleLayer *QTextStyle::GetLayer(int n)
{
  // Check arg
  if(n<0||n>=layers)return 0;
  QTextStyleLayer *l;
  return &layer[n];
}

bool QTextStyle::AddLayer(const QColor *col,int x,int y)
{
  QTextStyleLayer *l;
  
  if(layers==MAX_LAYER)
    return FALSE;
  
  l=&layer[layers];
  l->color.Set(col);
  l->x=x;
  l->y=y;
  layers++;
  return TRUE;
}

