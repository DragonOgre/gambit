//
// FILE: gambit.cc -- Main program for Gambit GUI
//
// $Id$
//

#include <string.h>
#include <ctype.h>

#include "wx/wxprec.h"
#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif  // WX_PRECOMP
#include "wx/wizard.h"
#include "wx/image.h"

#include "gambit.h"
#include "wxmisc.h"
#include "splash.h"
#include "dlabout.h"
#include "efgshow.h"
#include "nfgshow.h"
#include "system.h"
#include <signal.h>
#include <math.h>

typedef void (*fptr)(int);


void SigFPEHandler(int type)
{
  signal(SIGFPE, (fptr)SigFPEHandler);  // Reinstall signal handler.
  wxMessageBox("A floating point error has occured!\n"
	       "The results returned may be invalid");
}

//
// FIXME: Figure out how to handle math errors!
//
#ifdef UNUSED
class MathErrHandl : public wxDialog {
private:
  wxRadioBox *opt_box;
  int opt;
  static void ok_func(wxButton &ob, wxEvent &) 
    { ((MathErrHandl *)ob.GetClientData())->OnOk(); }
  void OnOk(void) { opt = opt_box->GetSelection(); Show(FALSE); }
    
public:
  MathErrHandl(const char *err);
  int Option(void) { return opt; }
};

MathErrHandl::MathErrHandl(const char *err)
  : wxDialog(0, 0, "Numerical Error")
{
  char *options[3] = {"Continue", "Ignore", "Quit"};
  /*
  wxMessage(this, (char *)err);
  this->NewLine();
  opt_box = new wxRadioBox(this, 0, "What now?", 
			   -1, -1, -1, -1, 3, 
			   options, 1, wxVERTICAL);
  this->NewLine();
  wxButton *ok = new wxButton(this, (wxFunction)ok_func, "OK");
  ok->SetClientData((char *)this);
  Fit();
  ok->Centre(wxHORIZONTAL);
  */
  ShowModal();
}
    
#ifdef __WXMSW__ // this handler is defined differently under win/unix
const int MATH_CONTINUE = 0;
const int MATH_IGNORE = 1;
const int MATH_QUIT = 2;
int _RTLENTRY _matherr (struct exception *e)
#else
#ifdef _LINUX
struct exception { char *name; double arg1, arg2; int type; };
#endif
int matherr(struct exception *e)
#endif
{
  static char *whyS[] = { "argument domain error",
                          "argument singularity ",
			  "overflow range error ",
			  "underflow range error",
			  "total loss of significance",
			  "partial loss of significance" };

  static int option = MATH_CONTINUE;
  char errMsg[80];
  if (option != MATH_IGNORE)   {
  sprintf (errMsg, "%s (%8g,%8g): %s\n",
	   e->name, e->arg1, e->arg2, whyS [e->type - 1]);
  MathErrHandl E(errMsg);
  option = E.Option();
  if (option==MATH_QUIT)   {
    wxExit();
  }
  // we did not really fix anything, but want no more warnings
  return 1;	
}
#endif  // UNUSED

bool GambitApp::OnInit(void)
{
  wxConfig config("Gambit");

  Splash *splash = new Splash(2);
  splash->Show(true);
  while (splash->IsShown()) {
    wxYield();
  }

  const long c_defaultFrameWidth = 400;
  const long c_defaultFrameHeight = 300;
  long frameWidth, frameHeight;
  config.Read("/Sizes/MainFrameWidth", &frameWidth,
	      c_defaultFrameWidth);
  config.Read("/Sizes/MainFrameHeight", &frameHeight,
	      c_defaultFrameHeight);
  
  // Create the main frame window.
  GambitFrame *gambitFrame = new GambitFrame(0, "Gambit", wxPoint(0, 0),
					     wxSize(frameWidth, frameHeight));
  gambitFrame->SetSizeHints(c_defaultFrameWidth, c_defaultFrameHeight);

  // Set up the help system.
  //  m_help.SetTempDir(".");
  wxInitAllImageHandlers();
  m_help.AddBook("help/guiman.hhp");
  m_help.AddBook("help/gclman.hhp");

  gambitFrame->Show(true);

  // Set up the error handling functions.
  // For some reason this does not work w/ BC++ (crash on exit)
#ifndef __BORLANDC__ 
  signal(SIGFPE, (fptr) SigFPEHandler);
#endif
    
  // Process command line arguments, if any.
  if (argc > 1) { 
    gambitFrame->LoadFile(argv[1]);
  }

  // Set current directory.
  wxGetApp().SetCurrentDir(gText(wxGetWorkingDirectory()));

  return true;
}

IMPLEMENT_APP(GambitApp)

//=====================================================================
//                       class GambitFrame
//=====================================================================

const int idGAMELISTCTRL = 1300;
const int menuOPTIONS = 2500;

class Game {
public:
  Efg::Game *m_efg;
  EfgShow *m_efgShow;
  Nfg *m_nfg;
  NfgShow *m_nfgShow;
  gText m_fileName;

  Game(Efg::Game *p_efg) : m_efg(p_efg), m_efgShow(0), m_nfg(0), m_nfgShow(0) { }
  Game(Nfg *p_nfg) : m_efg(0), m_efgShow(0), m_nfg(p_nfg), m_nfgShow(0) { }
};

GambitFrame::GambitFrame(wxFrame *p_parent, const wxString &p_title,
			 const wxPoint &p_position, const wxSize &p_size)
  : wxFrame(p_parent, -1, p_title, p_position, p_size),
    m_fileHistory(5)
{
#ifdef __WXMSW__
  SetIcon(wxIcon("gambit_icn"));
#else
#include "bitmaps/gambi.xpm"
  SetIcon(wxIcon(gambi_xpm));
#endif
    
  wxMenu *fileMenu = new wxMenu;
  fileMenu->Append(wxID_NEW, "&New\tCtrl-N", "Create a new game");
  fileMenu->Append(wxID_OPEN, "&Open\tCtrl-O", "Open a saved game");
  fileMenu->AppendSeparator();
  fileMenu->Append(wxID_EXIT, "E&xit\tCtrl-X", "Exit Gambit");

  wxConfig config("Gambit");
  m_fileHistory.Load(config);
  m_fileHistory.UseMenu(fileMenu);
  m_fileHistory.AddFilesToMenu();

  wxMenu *optionsMenu = new wxMenu;
  optionsMenu->Append(menuOPTIONS, "&Options",
		      "Configure Gambit to your tastes");

  wxMenu *helpMenu = new wxMenu;
  helpMenu->Append(wxID_HELP_CONTENTS, "&Contents", "Table of contents");
  helpMenu->Append(wxID_HELP_INDEX, "&Index", "Index of help file");
  helpMenu->AppendSeparator();
  helpMenu->Append(wxID_ABOUT, "&About", "About Gambit");
  
  wxMenuBar *menuBar = new wxMenuBar(wxMB_DOCKABLE);
  menuBar->Append(fileMenu, "&File");
  menuBar->Append(optionsMenu, "&Options");
  menuBar->Append(helpMenu, "&Help");

  SetMenuBar(menuBar);

  wxAcceleratorEntry entries[4];
  entries[0].Set(wxACCEL_CTRL, (int) 'N', wxID_NEW);
  entries[1].Set(wxACCEL_CTRL, (int) 'O', wxID_OPEN);
  entries[2].Set(wxACCEL_CTRL, (int) 'X', wxID_EXIT);
  entries[3].Set(wxACCEL_NORMAL, WXK_F1, wxID_HELP_CONTENTS);
  wxAcceleratorTable accel(4, entries);
  SetAcceleratorTable(accel);

  CreateStatusBar();
  MakeToolbar();

  m_gameListCtrl = new wxListCtrl(this, idGAMELISTCTRL,
				  wxPoint(0, 0), GetClientSize(),
				  wxLC_REPORT | wxLC_SINGLE_SEL);
  m_gameListCtrl->InsertColumn(0, "Title");
  m_gameListCtrl->InsertColumn(1, "Filename");
  m_gameListCtrl->InsertColumn(2, "Efg");
  m_gameListCtrl->InsertColumn(3, "Nfg");
}

GambitFrame::~GambitFrame()
{
  wxConfig config("Gambit");
  m_fileHistory.Save(config);

  for (int i = 1; i <= m_gameList.Length(); i++) {
    if (m_gameList[i]->m_nfg != 0) {
      delete m_gameList[i]->m_nfg;
    }
    if (m_gameList[i]->m_efg != 0) {
      delete m_gameList[i]->m_efg;
    }
    delete m_gameList[i];
  }
}

#include "bitmaps/new.xpm"
#include "bitmaps/open.xpm"
#include "bitmaps/help.xpm"

void GambitFrame::MakeToolbar(void)
{
  wxToolBar *toolBar = CreateToolBar(wxNO_BORDER | wxTB_FLAT | wxTB_DOCKABLE |
				     wxTB_HORIZONTAL);
  toolBar->SetMargins(4, 4);

  toolBar->AddTool(wxID_NEW, wxBITMAP(new), wxNullBitmap, false,
		   -1, -1, 0, "New game", "Create a new game");
  toolBar->AddTool(wxID_OPEN, wxBITMAP(open), wxNullBitmap, false,
		   -1, -1, 0, "Open file", "Open a saved game");
  toolBar->AddSeparator();
  toolBar->AddTool(wxID_HELP_CONTENTS, wxBITMAP(help), wxNullBitmap, false,
		   -1, -1, 0, "Help", "Table of contents");

  toolBar->Realize();
  toolBar->SetRows(1);
}

//--------------------------------------------------------------------
//              GambitFrame: Event-handling members
//--------------------------------------------------------------------

BEGIN_EVENT_TABLE(GambitFrame, wxFrame)
  EVT_MENU(wxID_NEW, GambitFrame::OnNew)
  EVT_MENU(wxID_OPEN, GambitFrame::OnLoad)
  EVT_MENU(wxID_EXIT, wxWindow::Close)
  EVT_MENU_RANGE(wxID_FILE1, wxID_FILE9, GambitFrame::OnMRUFile)
  EVT_MENU(menuOPTIONS, GambitFrame::OnOptions)
  EVT_MENU(wxID_HELP_CONTENTS, GambitFrame::OnHelpContents)
  EVT_MENU(wxID_HELP_INDEX, GambitFrame::OnHelpIndex)
  EVT_MENU(wxID_ABOUT, GambitFrame::OnHelpAbout)
  EVT_CLOSE(GambitFrame::OnCloseWindow)
  EVT_LIST_ITEM_SELECTED(idGAMELISTCTRL, GambitFrame::OnGameSelected)
  EVT_SIZE(GambitFrame::OnSize)
END_EVENT_TABLE()


class NewGameTypePage : public wxWizardPage {
private:
  wxRadioBox *m_gameType;
  wxWizardPage *m_efgPage, *m_nfgPage;

public:
  NewGameTypePage(wxWizard *, wxWizardPage *, wxWizardPage *);

  bool CreateEfg(void) const { return (m_gameType->GetSelection() == 0); }

  wxWizardPage *GetPrev(void) const { return 0; }
  wxWizardPage *GetNext(void) const
    { return (CreateEfg()) ? m_efgPage : m_nfgPage; }
};

NewGameTypePage::NewGameTypePage(wxWizard *p_parent,
				 wxWizardPage *p_efgPage,
				 wxWizardPage *p_nfgPage)
  : wxWizardPage(p_parent), m_efgPage(p_efgPage), m_nfgPage(p_nfgPage)
{
  SetAutoLayout(true);

  wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
  sizer->Add(new wxStaticText(this, -1,
			      "Step 1: Select the representation to work with"),
	     0, wxALL | wxCENTER, 10);

  wxString typeChoices[] = { "Extensive form", "Normal form" };
  m_gameType = new wxRadioBox(this, -1, "Representation",
			      wxDefaultPosition, wxDefaultSize,
			      2, typeChoices);
  sizer->Add(m_gameType, 0, wxALL | wxCENTER, 10);

  SetSizer(sizer);
  sizer->Fit(this);
  sizer->SetSizeHints(this);

  Layout();
}

const int idADDPLAYER_BUTTON = 2001;
const int idDELETEPLAYER_BUTTON = 2002;

class NfgPlayersPage : public wxWizardPageSimple {
private:
  wxButton *m_addButton, *m_deleteButton;
  wxGrid *m_nameGrid;

  // Event handlers
  void OnAddPlayer(wxCommandEvent &);
  void OnDeletePlayer(wxCommandEvent &);

public:
  NfgPlayersPage(wxWizard *);

  int NumPlayers(void) const { return m_nameGrid->GetRows(); }
  gText GetName(int) const;
  gArray<int> NumStrats(void) const;

  DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(NfgPlayersPage, wxWizardPageSimple)
  EVT_BUTTON(idADDPLAYER_BUTTON, NfgPlayersPage::OnAddPlayer)
  EVT_BUTTON(idDELETEPLAYER_BUTTON, NfgPlayersPage::OnDeletePlayer)
END_EVENT_TABLE()

NfgPlayersPage::NfgPlayersPage(wxWizard *p_parent)
  : wxWizardPageSimple(p_parent)
{
  SetAutoLayout(true);
  wxBoxSizer *buttonSizer = new wxBoxSizer(wxHORIZONTAL);
  m_addButton = new wxButton(this, idADDPLAYER_BUTTON, "Add player");
  buttonSizer->Add(m_addButton, 0, wxALL, 5);
  m_deleteButton = new wxButton(this, idDELETEPLAYER_BUTTON, "Delete player");
  m_deleteButton->Enable(false);
  buttonSizer->Add(m_deleteButton, 0, wxALL, 5);

  m_nameGrid = new wxGrid(this, -1, wxDefaultPosition, wxSize(250, 200));
  m_nameGrid->CreateGrid(2, 2);
  m_nameGrid->SetLabelValue(wxHORIZONTAL, "Label", 0);
  m_nameGrid->SetLabelValue(wxHORIZONTAL, "Strategies", 1);
  m_nameGrid->SetLabelAlignment(wxVERTICAL, wxCENTRE);
  m_nameGrid->SetCellValue("Player1", 0, 0);
  m_nameGrid->SetCellValue("Player2", 1, 0);
  m_nameGrid->SetCellValue("2", 0, 1);
  m_nameGrid->SetCellValue("2", 1, 1);
  m_nameGrid->DisableDragRowSize();
  m_nameGrid->DisableDragColSize();

  wxBoxSizer *topSizer = new wxBoxSizer(wxVERTICAL);
  topSizer->Add(new wxStaticText(this, -1,
				 "Step 2: Define the players for the game"),
		0, wxALL | wxCENTER, 5);
  topSizer->Add(buttonSizer, 0, wxALL | wxCENTER, 5);
  topSizer->Add(m_nameGrid, 0, wxALL | wxCENTER, 5);

  SetSizer(topSizer);
  topSizer->Fit(this);
  topSizer->SetSizeHints(this);

  Layout();
}

void NfgPlayersPage::OnAddPlayer(wxCommandEvent &)
{
  m_nameGrid->AppendRows();
  m_nameGrid->AdjustScrollbars();
  m_nameGrid->SetCellValue((char *) ("Player" + ToText(m_nameGrid->GetRows())),
			   m_nameGrid->GetRows() - 1, 0);
  m_nameGrid->SetCellValue("2", m_nameGrid->GetRows() - 1, 1);
  m_deleteButton->Enable(true);
}

void NfgPlayersPage::OnDeletePlayer(wxCommandEvent &)
{
  m_nameGrid->DeleteRows(m_nameGrid->GetCursorRow());
  m_nameGrid->AdjustScrollbars();
  m_deleteButton->Enable(m_nameGrid->GetRows() > 2);
}

gText NfgPlayersPage::GetName(int p_player) const
{
  return m_nameGrid->GetCellValue(p_player - 1, 0).c_str();
}

gArray<int> NfgPlayersPage::NumStrats(void) const
{
  gArray<int> numStrats(m_nameGrid->GetRows());
  for (int pl = 1; pl <= numStrats.Length(); pl++) {
    numStrats[pl] = atoi(m_nameGrid->GetCellValue(pl - 1, 1));
  }

  return numStrats;
}

class EfgPlayersPage : public wxWizardPageSimple {
private:
  wxButton *m_addButton, *m_deleteButton;
  wxGrid *m_nameGrid;

  // Event handlers
  void OnAddPlayer(wxCommandEvent &);
  void OnDeletePlayer(wxCommandEvent &);

public:
  EfgPlayersPage(wxWizard *);

  int NumPlayers(void) const { return m_nameGrid->GetRows(); }
  gText GetName(int) const;
  gArray<int> NumStrats(void) const;

  DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(EfgPlayersPage, wxWizardPageSimple)
  EVT_BUTTON(idADDPLAYER_BUTTON, EfgPlayersPage::OnAddPlayer)
  EVT_BUTTON(idDELETEPLAYER_BUTTON, EfgPlayersPage::OnDeletePlayer)
END_EVENT_TABLE()

EfgPlayersPage::EfgPlayersPage(wxWizard *p_parent)
  : wxWizardPageSimple(p_parent)
{
  SetAutoLayout(true);
  wxBoxSizer *buttonSizer = new wxBoxSizer(wxHORIZONTAL);
  m_addButton = new wxButton(this, idADDPLAYER_BUTTON, "Add player");
  buttonSizer->Add(m_addButton, 0, wxALL, 5);
  m_deleteButton = new wxButton(this, idDELETEPLAYER_BUTTON, "Delete player");
  m_deleteButton->Enable(false);
  buttonSizer->Add(m_deleteButton, 0, wxALL, 5);

  m_nameGrid = new wxGrid(this, -1, wxDefaultPosition, wxSize(250, 200));
  m_nameGrid->CreateGrid(2, 1);
  m_nameGrid->SetLabelValue(wxHORIZONTAL, "Label", 0);
  m_nameGrid->SetLabelAlignment(wxVERTICAL, wxCENTRE);
  m_nameGrid->SetCellValue("Player1", 0, 0);
  m_nameGrid->SetCellValue("Player2", 1, 0);
  m_nameGrid->DisableDragRowSize();
  m_nameGrid->DisableDragColSize();

  wxBoxSizer *topSizer = new wxBoxSizer(wxVERTICAL);
  topSizer->Add(new wxStaticText(this, -1,
				 "Step 2: Define the players for the game"),
		0, wxALL | wxCENTER, 5);
  topSizer->Add(buttonSizer, 0, wxALL | wxCENTER, 5);
  topSizer->Add(m_nameGrid, 0, wxALL | wxCENTER, 5);

  SetSizer(topSizer);
  topSizer->Fit(this);
  topSizer->SetSizeHints(this);

  Layout();
}

void EfgPlayersPage::OnAddPlayer(wxCommandEvent &)
{
  m_nameGrid->AppendRows();
  m_nameGrid->AdjustScrollbars();
  m_nameGrid->SetCellValue((char *) ("Player" + ToText(m_nameGrid->GetRows())),
			   m_nameGrid->GetRows() - 1, 0);
  m_deleteButton->Enable(true);
}

void EfgPlayersPage::OnDeletePlayer(wxCommandEvent &)
{
  m_nameGrid->DeleteRows(m_nameGrid->GetCursorRow());
  m_nameGrid->AdjustScrollbars();
  m_deleteButton->Enable(m_nameGrid->GetRows() > 2);
}

gText EfgPlayersPage::GetName(int p_player) const
{
  return m_nameGrid->GetCellValue(p_player - 1, 0).c_str();
}

void GambitFrame::OnNew(wxCommandEvent &)
{
  wxWizard *wizard = wxWizard::Create(this, -1, "Creating a new game");
  EfgPlayersPage *efgPage = new EfgPlayersPage(wizard);
  NfgPlayersPage *nfgPage = new NfgPlayersPage(wizard);
  NewGameTypePage *page1 = new NewGameTypePage(wizard, efgPage, nfgPage);
  efgPage->SetPrev(page1);
  nfgPage->SetPrev(page1);

  if (wizard->RunWizard(page1)) {
    if (page1->CreateEfg()) {
      FullEfg *efg = new FullEfg;
      for (int pl = 1; pl <= efgPage->NumPlayers(); pl++) {
	efg->NewPlayer()->SetName(efgPage->GetName(pl));
      }
      EfgShow *efgShow = new EfgShow(*efg, this);
      efgShow->SetFilename("");
      AddGame(efg, efgShow);
      SetActiveWindow(efgShow);
      m_fileHistory.UseMenu(efgShow->GetMenuBar()->GetMenu(0));
      m_fileHistory.AddFilesToMenu(efgShow->GetMenuBar()->GetMenu(0));
    }
    else {
      Nfg *nfg = new Nfg(nfgPage->NumStrats());
      for (int pl = 1; pl <= nfgPage->NumPlayers(); pl++) {
	nfg->Players()[pl]->SetName(nfgPage->GetName(pl));
      }
      NfgShow *nfgShow = new NfgShow(*nfg, this);
      nfgShow->SetFilename("");
      AddGame(nfg, nfgShow);
      SetActiveWindow(nfgShow);
      m_fileHistory.UseMenu(nfgShow->GetMenuBar()->GetMenu(0));
      m_fileHistory.AddFilesToMenu(nfgShow->GetMenuBar()->GetMenu(0));
    }
  }

  wizard->Destroy();
}

void GambitFrame::OnLoad(wxCommandEvent &)
{
  gText filename = wxFileSelector("Load data file", wxGetApp().CurrentDir(),
				  NULL, NULL, "*.?fg").c_str();
  if (filename == "") {
    return;
  }

  wxGetApp().SetCurrentDir(gPathOnly(filename));

  LoadFile(filename);
}

void GambitFrame::OnMRUFile(wxCommandEvent &p_event)
{
  LoadFile(m_fileHistory.GetHistoryFile(p_event.GetId() - wxID_FILE1).c_str());
}

void GambitFrame::OnOptions(wxCommandEvent &)
{
  wxGetApp().GetPreferences().EditOptions(this);
}

void GambitFrame::OnHelpAbout(wxCommandEvent &)
{
  dialogAbout dialog(this, "About Gambit...",
		     "Gambit Graphical User Interface",
		     "Version 0.97 (alpha)");
  dialog.ShowModal();
}

void GambitFrame::OnHelpContents(wxCommandEvent &)
{
  wxGetApp().HelpController().DisplaySection("Main page");
}

void GambitFrame::OnHelpIndex(wxCommandEvent &)
{
  wxGetApp().HelpController().DisplayContents();
}

void GambitFrame::LoadFile(const gText &p_filename)
{    
  gText filename(gFileNameFromPath(p_filename));
  filename = filename.Dncase();

  if (strstr((const char *) filename, ".nfg")) {
    // This must be a normal form.
    try {
      gFileInput infile(p_filename);
      Nfg *nfg = 0;

      ReadNfgFile(infile, nfg);

      if (!nfg) {
	wxMessageBox((char *) (p_filename + " is not a valid .nfg file"));
      }
      else {
	m_fileHistory.AddFileToHistory((char *) p_filename);
      }
      NfgShow *nfgShow = new NfgShow(*nfg, this);
      nfgShow->SetFilename(p_filename);
      AddGame(nfg, nfgShow);
      SetFilename(nfgShow, p_filename);
      SetActiveWindow(nfgShow);
      m_fileHistory.UseMenu(nfgShow->GetMenuBar()->GetMenu(0));
      m_fileHistory.AddFilesToMenu(nfgShow->GetMenuBar()->GetMenu(0));
      return;
    }
    catch (gFileInput::OpenFailed &) {
      wxMessageBox((char *) ("Could not open " + p_filename + " for reading"));
      return;
    }
  }
  else if (strstr((const char *) filename, ".efg")) {
    // This must be an extensive form.
    try {
      gFileInput infile(p_filename);
      FullEfg *efg = ReadEfgFile(infile);
                
      if (!efg) {
	wxMessageBox((char *) (filename + " is not a valid .efg file"));
      }
      else {
	m_fileHistory.AddFileToHistory((char *) p_filename);
      }

      EfgShow *efgShow = new EfgShow(*efg, this);
      efgShow->SetFilename(filename);
      AddGame(efg, efgShow);
      SetFilename(efgShow, p_filename);
      SetActiveWindow(efgShow);
      m_fileHistory.UseMenu(efgShow->GetMenuBar()->GetMenu(0));
      m_fileHistory.AddFilesToMenu(efgShow->GetMenuBar()->GetMenu(0));
      return;
    }
    catch (gFileInput::OpenFailed &) { 
      wxMessageBox((char *) ("Could not open " + filename + " for reading"));
      return;
    }
  }

  wxMessageBox("Unknown file type");
}

void GambitFrame::OnCloseWindow(wxCloseEvent &)
{
  wxConfig config("Gambit");
  config.Write("/Sizes/MainFrameWidth", (long) GetSize().GetWidth());
  config.Write("/Sizes/MainFrameHeight", (long) GetSize().GetHeight());

  while (m_gameList.Length() > 0) {
    if (m_gameList[1]->m_efgShow && m_gameList[1]->m_nfgShow) {
      delete m_gameList[1]->m_nfgShow;
      delete m_gameList[1]->m_efgShow;
    }
    else if (m_gameList[1]->m_nfgShow) {
      delete m_gameList[1]->m_nfgShow;
    }
    else {
      delete m_gameList[1]->m_efgShow;
    }
  }

  Destroy();
}

void GambitFrame::OnSize(wxSizeEvent &)
{
  m_gameListCtrl->SetSize(GetClientSize());
  int listWidth = GetClientSize().GetWidth();
  m_gameListCtrl->SetColumnWidth(0, listWidth / 2);
  m_gameListCtrl->SetColumnWidth(1, listWidth / 4);
  m_gameListCtrl->SetColumnWidth(2, listWidth / 8);
  m_gameListCtrl->SetColumnWidth(3, listWidth / 8);
}

void GambitFrame::UpdateGameList(void)
{
  m_gameListCtrl->DeleteAllItems();
  for (int i = 1; i <= m_gameList.Length(); i++) {
    Game *game = m_gameList[i];
    if (game->m_efg != 0) {
      m_gameListCtrl->InsertItem(i - 1, (char *) game->m_efg->GetTitle());
    }
    else {
      m_gameListCtrl->InsertItem(i - 1, (char *) game->m_nfg->GetTitle());
    }
    m_gameListCtrl->SetItem(i - 1, 1, (char *) game->m_fileName);
    m_gameListCtrl->SetItem(i - 1, 2, (game->m_efg != 0) ? "Yes" : "No");
    m_gameListCtrl->SetItem(i - 1, 3, (game->m_nfg != 0) ? "Yes" : "No");
  }
}

void GambitFrame::AddGame(Efg::Game *p_efg, EfgShow *p_efgShow)
{
  Game *game = new Game(p_efg);
  game->m_efgShow = p_efgShow;
  m_gameList.Append(game);
  UpdateGameList();
}

void GambitFrame::AddGame(Nfg *p_nfg, NfgShow *p_nfgShow)
{
  Game *game = new Game(p_nfg);
  game->m_nfgShow = p_nfgShow;
  m_gameList.Append(game);
  UpdateGameList();
}

void GambitFrame::AddGame(Efg::Game *p_efg, Nfg *p_nfg, NfgShow *p_nfgShow)
{
  for (int i = 1; i <= m_gameList.Length(); i++) {
    if (m_gameList[i]->m_efg == p_efg) {
      m_gameList[i]->m_nfg = p_nfg;
      m_gameList[i]->m_nfgShow = p_nfgShow;
      break;
    }
  }
  UpdateGameList();
}

void GambitFrame::RemoveGame(Efg::Game *p_efg)
{
  for (int i = 1; i <= m_gameList.Length(); i++) {
    if (m_gameList[i]->m_efg == p_efg) {
      m_fileHistory.RemoveMenu(m_gameList[i]->m_efgShow->GetMenuBar()->GetMenu(0));
      if (m_gameList[i]->m_nfg) {
	m_fileHistory.RemoveMenu(m_gameList[i]->m_nfgShow->GetMenuBar()->GetMenu(0));
	m_gameList[i]->m_nfgShow->Close();
	delete m_gameList[i]->m_nfg;
      }
      delete m_gameList.Remove(i);
      delete p_efg;
      break;
    }
  }
  UpdateGameList();
}

void GambitFrame::RemoveGame(Nfg *p_nfg)
{
  for (int i = 1; i <= m_gameList.Length(); i++) {
    if (m_gameList[i]->m_nfg == p_nfg) {
      m_fileHistory.RemoveMenu(m_gameList[i]->m_nfgShow->GetMenuBar()->GetMenu(0));
      if (m_gameList[i]->m_efg == 0) {
	delete m_gameList.Remove(i);
      }
      else {
	m_gameList[i]->m_nfg = 0;
	m_gameList[i]->m_nfgShow = 0;
      }
      delete p_nfg;
    }
  }
  UpdateGameList();
}

void GambitFrame::OnGameSelected(wxListEvent &p_event)
{
  if (m_gameList[p_event.GetSelection()+1]->m_efgShow) {
    //    wxActivateEvent event;
    // m_gameList[p_event.GetSelection()+1]->m_efgShow->AddPendingEvent(event);
    m_gameList[p_event.GetSelection()+1]->m_efgShow->Raise();
  }
}

EfgShow *GambitFrame::GetWindow(const Efg::Game *p_efg)
{
  for (int i = 1; i <= m_gameList.Length(); i++) {
    if (m_gameList[i]->m_efg == p_efg) {
      return m_gameList[i]->m_efgShow;
    }
  }
  return 0;
}

NfgShow *GambitFrame::GetWindow(const Nfg *p_nfg)
{
  for (int i = 1; i <= m_gameList.Length(); i++) {
    if (m_gameList[i]->m_nfg == p_nfg) {
      return m_gameList[i]->m_nfgShow;
    }
  }
  return 0;
}

void GambitFrame::SetFilename(EfgShow *p_efgShow, const gText &p_file)
{
  for (int i = 1; i <= m_gameList.Length(); i++) {
    if (m_gameList[i]->m_efgShow == p_efgShow) {
      m_gameList[i]->m_fileName = p_file;
      UpdateGameList();
      return;
    }
  }
}

void GambitFrame::SetFilename(NfgShow *p_nfgShow, const gText &p_file)
{
  for (int i = 1; i <= m_gameList.Length(); i++) {
    if (m_gameList[i]->m_nfgShow == p_nfgShow) {
      m_gameList[i]->m_fileName = p_file;
      UpdateGameList();
      return;
    }
  }
}

void GambitFrame::SetActiveWindow(EfgShow *p_efgShow)
{
  for (int i = 1; i <= m_gameList.Length(); i++) {
    if (m_gameList[i]->m_efgShow == p_efgShow) {
      wxListItem item;
      item.m_mask = wxLIST_MASK_STATE;
      item.m_itemId = i - 1;
      item.m_state = wxLIST_STATE_SELECTED;
      item.m_stateMask = wxLIST_STATE_SELECTED;
      m_gameListCtrl->SetItem(item);
      return;
    }
  }
}

void GambitFrame::SetActiveWindow(NfgShow *p_nfgShow)
{
  for (int i = 1; i <= m_gameList.Length(); i++) {
    if (m_gameList[i]->m_nfgShow == p_nfgShow) {
      wxListItem item;
      item.m_mask = wxLIST_MASK_STATE;
      item.m_itemId = i - 1;
      item.m_state = wxLIST_STATE_SELECTED;
      item.m_stateMask = wxLIST_STATE_SELECTED;
      m_gameListCtrl->SetItem(item);
      return;
    }
  }
}

//
// A general-purpose dialog box to display the description of the exception
//
void guiExceptionDialog(const gText &p_message, wxWindow *p_parent,
            long p_style /*= wxOK | wxCENTRE*/)
{
  gText message = "An internal error occurred in Gambit:\n" + p_message;
  wxMessageBox((char *) message, "Gambit Error", p_style, p_parent);
}

#include "base/garray.imp"
#include "base/gblock.imp"

template class gArray<Game *>;
template class gBlock<Game *>;
