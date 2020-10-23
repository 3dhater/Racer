/*
 * Racer Menus
 * 13-07-2001: Created! (17:22:59)
 * NOTES:
 * FUTURE:
 * - Move out all the mesh allocations, since RButton now handles
 * the buttons.
 * (C) MarketGraph/RvG
 */

#include "main.h"
#include <qlib/debug.h>
#pragma hdrstop
#include <raceru/button.h>
#include <raceru/window.h>
#include <d3/mesh.h>
DEBUG_ENABLE

static DBitMapTexture *texBgr,*texLogo;
static DMesh *mBgr,*mLogo;
static QDraw *draw;
DTexFont *font[2];
//static float scaleX,scaleY;

#ifdef Q_USE_FMOD
//FSOUND_STREAM *streamMusic;
#endif

struct RMenu
{
 public:
  enum Max
  {
    MAX_TEXT=10
  };
  int count;
  string text[MAX_TEXT];
  string title;
  int    curSel;                 // Which item is selected?
  DMesh *mSel;
  RButton *button[MAX_TEXT];

  RMenu();
  ~RMenu();
  void Destroy();
  
  void Reset();
  void Add(cstring s);
  void SetTitle(cstring s);
  
  // Selection
  void SelectNext();
  void SelectPrev();
  
  // Paint
  void PaintTitle();
  void Paint();

  // Animation
  void AnimIn();
};

RMenu *menuMain;

/**************
* RMenu class *
**************/
RMenu::RMenu()
{
  int i;

  count=0;
  title=0;
  curSel=0;
  // Create a selection cursor
  mSel=new DMesh();
  mSel->DefineFlat2D(RScaleX(300),RScaleY(30));
  mSel->EnableCSG();
  for(i=0;i<MAX_TEXT;i++)
    button[i]=0;
}
RMenu::~RMenu()
{
  Destroy();
}
void RMenu::SetTitle(cstring s)
{
  QFREE(title);
  title=qstrdup(s);
}
void RMenu::Destroy()
{
qdbg("RMenu Destroy(); count=%d\n",count);
  int i;
  for(i=0;i<count;i++)
  {
    QFREE(text[i]);
    QDELETE(button[i]);
  }
  QFREE(title);
  QDELETE(mSel);
  count=0;
}
void RMenu::Reset()
// Reset lines
{
  int i;
  for(i=0;i<count;i++)
  { QFREE(text[i]);
    QDELETE(button[i]);
  }
  count=0;
}
void RMenu::Add(cstring s)
{
  QRect r;
  if(count>=MAX_TEXT)return;
  text[count]=qstrdup(s);
  r.y=int(RScaleY(200)+count*RScaleY(40));
  r.wid=int(RScaleX(300)); r.hgt=int(RScaleY(30));
  r.x=int(RScaleX(800/2)-r.wid/2);
  button[count]=new RButton(QSHELL,&r,s,font[0]);
  count++;
}
void rrPaintText(DTexFont *font,cstring s,int x,int y)
// Scaled version
{
  RScaleXY(x,y,&x,&y);
  font->Paint(s,x,y);
}

/**************
* Menu select *
**************/
void RMenu::SelectNext()
{
  if(!count)return;
  curSel=(curSel+1)%count;
}
void RMenu::SelectPrev()
{
  if(!count)return;
  curSel--;
  if(curSel<0)
    curSel=count-1;
}

/*************
* Menu paint *
*************/
void RMenu::PaintTitle()
{
  glDisable(GL_DEPTH_TEST);
  
  if(title)
    rrPaintText(font[1],title,-380,240);
}

void RMenu::Paint()
{
  glPushMatrix();
  PaintTitle();
  glPopMatrix();
}

void RMenu::AnimIn()
// Trigger every button to come in
{
  int i,t=400;

  for(i=0;i<count;i++)
    button[i]->AnimIn(t,i*t/3);
}

/***********
* Painting *
***********/
DTexFont *RMenuGetFont(int n)
// Returns loaded fonts for convenience of menu components
{
  return font[n];
}
static void RMenuBgrPaint()
{
  QCV->Clear();
  // No separate specular
  QCV->GetGLContext()->SetSeparateSpecularColor(FALSE);
  glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_REPLACE);
  // Some fog leftovers from the game?
  glDisable(GL_FOG);
  mBgr->Paint();
}
void RMenuPaintLogo()
{
  QCV->Set3D();
  glDisable(GL_DEPTH_TEST);
  glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_REPLACE);
  mLogo->Paint();
}
void RMenuSetTitle(cstring s)
{
  menuMain->SetTitle(s);
}
void Swap()
{
  draw->Swap();
}
void RMenuPaintGlobal(int flags)
// Paint menu things that are visually through all menus
// flags&1; clear below titlebar
{
  RMenuBgrPaint();
  menuMain->PaintTitle();
  if(flags&1)
  {
    // Clear lower line
    QRect r,r2;
    r.x=0;
    r.y=76;
    r.wid=800;
    r.hgt=600-r.y+1;            // Add some leeway for rounding errors
    RScaleRect(&r,&r2);
    //rrScaleRect(&r,&r2);
    QCV->SetColor(0,0,0);
    QCV->Rectfill(&r2);
  }
}
void RMenuSwap()
{
  Swap();
}

/***********
* Creation *
***********/
void RMenuCreate()
{
  // Possibly some music
  RMGR->musicMgr->PlayRandom(RMusicManager::MENU);

  // Where to draw?
  //draw=Q_BC?Q_BC:QSHELL;
  if(Q_BC)draw=Q_BC; else draw=QSHELL;
  
  menuMain=new RMenu();
  
  // Get background picture
  QImage img("data/images/menu_bgr.tga");
  if(img.IsRead())
  {
    texBgr=new DBitMapTexture(&img);
  }
  
  // Create quad for bgr display
  mBgr=new DMesh();
  mBgr->DefineFlat2D(draw->GetWidth(),draw->GetHeight());
  QRect r(0,0,img.GetWidth(),img.GetHeight());
  mBgr->GetPoly(0)->DefineTexture(texBgr,&r);
  
  // Create logo polygon
  {
    QImage img("data/images/logo.tga");
    if(img.IsRead())
      texLogo=new DBitMapTexture(&img);
    mLogo=new DMesh();
    mLogo->DefineFlat2D(RScaleX(170),RScaleY(110));
    mLogo->Move(RScaleX(300),RScaleY(230),0);
    QRect r(0,0,img.GetWidth(),img.GetHeight());
    mLogo->GetPoly(0)->DefineTexture(texLogo,&r);
    mLogo->EnableCSG();
    mLogo->GetPoly(0)->SetBlendMode(DPoly::BLEND_SRC_ALPHA);
  }

  // Fonts
  if(!font[0])
  {
    font[0]=new DTexFont("data/fonts/euro16");
    font[0]->SetScale(RScaleX(1),RScaleY(1));
    font[1]=new DTexFont("data/fonts/euro40");
    font[1]->SetScale(RScaleX(1),RScaleY(1));
  }
}
void RMenuDestroy()
{
qdbg("RMenuDestroy()\n");

#ifdef Q_USE_FMOD_OBS
  // Turn off music
  if(streamMusic)
  {
    FSOUND_Stream_Close(streamMusic);
    streamMusic=0;
  }
#endif

  QDELETE(texBgr);
  QDELETE(mBgr);
  QDELETE(texLogo);
  QDELETE(mLogo);
  // Keep fonts for in the game?
  QDELETE(font[0]);
  QDELETE(font[1]);
  QDELETE(menuMain);
}

/******
* Run *
******/
static void menuIdle()
{
//qdbg("menuIdle\n");
  // Keep music going (also done in RManager, but that's not active)
  if(RMGR->musicMgr->NextSongRequested())
    RMGR->musicMgr->PlayNext();
}

static void DoRace()
{
  //RMGR->infoMain->SetInt("multiplayer.allow_remote",0);
  RMGR->infoMain->Write();
}
static void DoHost()
{
  RMGR->infoMain->SetInt("multiplayer.allow_remote",1);
  RMGR->infoMain->Write();
}
static bool DoJoin()
// Returns FALSE if cancelled
{
  int r;
  char buf[256];

  RMGR->infoMain->GetString("multiplayer.server",buf,"localhost");
  // Put up a dialog; some PC drivers may have a hard time putting up
  // 2 windows at the same time. Better drivers will ofcourse just do
  // what they're supposed to do.
  r=QDlgString("Join network game","Enter server name",buf,sizeof(buf));
  if(r==FALSE)return FALSE;
  RMGR->infoMain->SetInt("multiplayer.allow_remote",0);
  RMGR->infoMain->SetString("multiplayer.server",buf);
  RMGR->infoMain->Write();
  // Get focus back on single window
  QCV->Select();
  return TRUE;
}
static void DisableButtons()
{
  int i;
  for(i=0;i<menuMain->count;i++)
    menuMain->button[i]->Hide();
}
static void EnableButtons()
{
  int i;
  for(i=0;i<menuMain->count;i++)
    menuMain->button[i]->Show();
}

static void Do()
{
  int i=0;
  bool    r;
  QEvent *e=0;

 paint:
  
  menuMain->SetTitle("Racer v0.5.0 alpha 1");
  while(1)
  {
    // Continously redraw menu, since animations may be playing.
    // Also saves us from handling Expose events.
    // Repaint entire interface
    RMenuBgrPaint();
    menuMain->Paint();
    //QWM->Paint();
    rrPaintGUI();
    RMenuPaintLogo();
    Swap();

    menuIdle();

    // Handle input; keep event queue from constipating
    while(QEventPending())
    {
      app->Run1();
      e=app->GetCurrentEvent();
      if(e->type==QEvent::CLICK)
      {
        for(i=0;i<menuMain->count;i++)
        {
          if(e->win==menuMain->button[i])
          {
            switch(i)
            {
              case 0: DoRace(); return;
              case 1: DoHost(); return;
              case 2: if(DoJoin())return; goto paint;
              case 3: DisableButtons(); rrSelectCar();
                      EnableButtons(); goto paint;
              case 4: DisableButtons(); rrSelectTrk();
                      EnableButtons(); goto paint;
              case 5: DisableButtons(); r=rrSelectReplay();
                      EnableButtons();
                      // Return and view replay if one was selected
                      if(r)return;
                      goto paint;
              case 6: RMenuDestroy(); app->Exit(0); break;
            }
          }
        }
      }
    }
  }
}

void RMenuRun()
// Display and handle menu
{
  // Reset OpenGL state
  //glDisable(GL_LIGHTING);
  //glDisable(GL_DEPTH_TEST);
  glShadeModel(GL_SMOOTH);

  //app->SetIdleProc(menuIdle);

  RMenuBgrPaint();
  Swap();
  
  // Main menu
  menuMain->Reset();
  menuMain->Add("Race!");
  menuMain->Add("Host multiplayer game");
  menuMain->Add("Join multiplayer game");
  menuMain->Add("Select car");
  menuMain->Add("Select track");
  menuMain->Add("View replay");
  menuMain->Add("Exit");
  menuMain->AnimIn();

  Do();
  // Free resources
  RMenuDestroy();
}
