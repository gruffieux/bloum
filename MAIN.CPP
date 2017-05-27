#include <dxerror.h>
#include <cinput.h>
#include <cmsgreporter.h>
#include <cconfigfile.h>
#include <ckeyboardlistener.h>
#include <cpanel.h>
#include <canimation.h>
#include <cdate.h>
#include "ctest.h"
#include "main.h"
#include "BloumRes/resource.h"

/*
BUGS:
*/

//////////////////////////////
// Prototypes des fonctions //
//////////////////////////////

void InitWindow(HINSTANCE hInst, int nCmdShow);									//Initialise une fenêtre
void InitDirectDraw();															//Initialise l'affichage, charge les images dans les surfaces
void DestroyDirectDraw();														//Détruit les objets DirectDraw
void RestoreDirectDraw();
void InitDirectInput();															//Initialise les périphériques d'entrées
void DestroyDirectInput();														//Détruit les objets DirectInput
void InitObjects();																//Initialise les objets
void DestroyAll();																//Detruit le tout
LRESULT CALLBACK WndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);	//Procédure window, procède à tous les messages
																				//envoyés sur la fenêtre par l'OS
bool LoadConfig();
void PlayScene(int n);															//Affiche les images de la scène et gère les actions des objets
bool ProcessKbInput();															//Procède aux entrées du clavier
void ProcessMouseInput();

////////////////////////
// Variables globales //
////////////////////////

//Variables et structures
int NrScene;
bool running, reinit;
CardinalPoint Edge;

//Objets de base nécessaire à l'application
HWND hWndMain;
Display *Screen;
Input *Keyboard, *Mouse;

//Objets
ConfigFile Config;
KeyboardListener keybListener;
Str BigText;
Panel *Info;
TileEngine *Map;
Visual *Background, *Wall[NB_WALL];
Test *Square, *Triangle;
Cursor *cursor;
Clock BloumClock;

///////////////////////////////
// Définitions des fonctions //
///////////////////////////////

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nCmdShow)
{
	int i;
	MSG msg;
	HMODULE ResModule;

	NrScene = 0;
	ResModule = LoadLibrary("BloumRes.dll");

	Screen = new Display();
	Keyboard = new Input();
	Mouse = new Input();
	Info = new Panel(4, 3, 60, 5, 5, false, Text(), "Panneau d'informations");
	Map = new TileEngine();
	Background = new Visual();
	for (i = 0; i < NB_WALL; i++)
		Wall[i] = new Visual();
	Square = new Test(NULL, NULL, 10, 10, 0, 0.1, 0.1, "Carre");
	Triangle = new Test(40, 40, 30, 30, 0, 0.3, 0.3, "Triangle");
	cursor = new Cursor();

	if (!LoadConfig())
	{
		MessageBox(NULL, "Impossible de charger la configuration", NULL, MB_ICONERROR);
		SendMessage(NULL, WM_QUIT, NULL, NULL);
		return 0;
	}

	BigText = BuildString("Bienvenue tut dans tut %s et plutôt tut %d fois que tut %d!!!", "Bloum", 2, 1);

	InitDXErrorMsg();
	InitWindow(hInst, nCmdShow);

	Screen->SetDxWnd(hWndMain);
	Screen->SetResInst(ResModule);
	Keyboard->SetDxWnd(hWndMain);
	Keyboard->SetInstance(hInst);
	Mouse->SetDxWnd(hWndMain);
	Mouse->SetInstance(hInst);

	InitDirectDraw();
	InitDirectInput();
	InitObjects();

	while (1)
		if (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
		{
			if(!GetMessage(&msg, NULL, 0, 0 ))
				return msg.wParam;
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			if (running)
			{
				if (reinit)
				{
					reinit = false;
					DestroyDirectDraw();
					DestroyDirectInput();
					RestoreDirectDraw();
					InitDirectInput();
				}
				else
				{
					PlayScene(NrScene);
					ProcessMouseInput();
					ProcessKbInput();
				}
			}
		}

	return 0;
}

void InitWindow(HINSTANCE hInst, int nCmdShow)
{
	WNDCLASS wc; //Structure contenant les informations de la fenêtre
	
	//----------Enregistrement d'une classe window----------

	//Assignation de valeure pour chaques champs de la structure wc
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInst;
	wc.hIcon = LoadIcon((HINSTANCE) NULL, IDI_APPLICATION);
	wc.hCursor = LoadCursor((HINSTANCE) NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = "MainWndClass";

	//Enregistrement
	if (!RegisterClass(&wc))
		ReportDxError(S_FALSE, WIN32_, ERR_REGISTERCLASS);
	
	//----------Création de la fenêtre----------

	hWndMain = CreateWindow(wc.lpszClassName, "Bloum", WS_POPUPWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, Screen->GetCurrentMode()->GetWidth(), Screen->GetCurrentMode()->GetHeight(), (HWND) NULL, (HMENU) NULL, hInst, (LPVOID) NULL);

	if (!hWndMain)
		ReportDxError(S_FALSE, WIN32_, ERR_CREATEWIN);

	ShowWindow(hWndMain, nCmdShow); //Afficher la fenêtre
	UpdateWindow(hWndMain);
}

void InitDirectDraw()
{
	int i;

	Screen->SetDisplayMode();

	Background->addAni(IDB_BACKGROUND, "", 1, 32, 32, -1, 90);

	for (i = 0; i < NB_WALL; i++)
		Wall[i]->addAni(IDB_WALL, "", 1, 32, 32, -1, 90);

	Map->GetTileData()->addAni(IDB_MAP, "", 0, NULL, NULL, RGB(255, 255, 255), 90);
	
	Square->addAni(IDB_SQUARE, "", 1, 10, 10, -1, 0);

	Triangle->addAni(IDB_TRIANGLE_STOP, "", 1, 30, 30, RGB(0, 0, 0), 90);
	Triangle->addAni(IDB_TRIANGLE_LOOP, "", 4, 30, 30, RGB(0, 0, 0), 90);

	cursor->addAni(IDB_CURSOR, "", 1, 9, 8, RGB(255, 255, 255), 90);
}

void DestroyDirectDraw()
{
	int i;

	Info->DestroyText();

	for (i = 0; i < NB_WALL; i++)
		Wall[i]->destroyAni();

	Background->destroyAni();
	Map->GetTileData()->destroyAni();
	Square->destroyAni();
	Triangle->destroyAni();
	cursor->destroyAni();
	
	Screen->Destroy();
}

void RestoreDirectDraw()
{
	int i;

	Screen->SetDisplayMode();

	Info->Refresh();
	Info->Initialize();

	Background->restoreAni();

	for (i = 0; i < NB_WALL; i++)
		Wall[i]->restoreAni();

	Map->GetTileData()->restoreAni();
	
	Square->restoreAni();

	Triangle->restoreAni();

	cursor->restoreAni();
}

void InitDirectInput()
{
	Keyboard->CreateDevice(GUID_SysKeyboard, &c_dfDIKeyboard, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
	Mouse->CreateDevice(GUID_SysMouse, &c_dfDIMouse, DISCL_EXCLUSIVE | DISCL_FOREGROUND);
}

void DestroyDirectInput()
{
	Keyboard->Destroy();
	Mouse->Destroy();
}

void InitObjects()
{
	int i;

	Edge.west = 0;
	Edge.east = Screen->GetCurrentMode()->GetWidth();
	Edge.north = 0;
	Edge.south = Screen->GetCurrentMode()->GetHeight();

	Info->GetGrid(0, 2)->ColSpan = 4;
	Info->AlignCols();
	Info->AlignRows();
	Info->Init();
	Info->Initialize();
	Info->show();

	Background->setMosaic(true);
	*Background->getDimension() = Axe(Screen->GetCurrentMode()->GetWidth(), Screen->GetCurrentMode()->GetHeight());

	Map->GetScreenData()->dimension.x = Screen->GetCurrentMode()->GetWidth();
	Map->GetScreenData()->dimension.y = Screen->GetCurrentMode()->GetHeight();
	Map->GetMapFile()->SetFileName("map.dat");
	Map->LoadMap(0);

	*cursor->getDimension() = Axe(9, 8);
	cursor->getMover()->centerPosition(0, 0, Screen->GetCurrentMode()->GetWidth(), Screen->GetCurrentMode()->GetHeight());

	Square->getPosition()->x = 40;
	Square->getPosition()->y = Screen->GetCurrentMode()->GetHeight() - 20;

	Triangle->SetMap(Map);

	*Wall[0]->getDimension() = Axe(Animation::getAnimationElement(0, Wall[0]->getAnimations())->GetSprite()->GetWidth() * 3, Animation::getAnimationElement(0, Wall[0]->getAnimations())->GetSprite()->GetHeight() * 3);
	*Wall[0]->getPosition() = Axe(Screen->GetCurrentMode()->GetWidth() / 2 - Wall[0]->getDimension()->x / 2, Screen->GetCurrentMode()->GetHeight() / 2 - Wall[0]->getDimension()->y / 2);

	*Wall[1]->getDimension() = Axe(Animation::getAnimationElement(0, Wall[1]->getAnimations())->GetSprite()->GetWidth() * 4, Animation::getAnimationElement(0, Wall[0]->getAnimations())->GetSprite()->GetHeight());
	*Wall[1]->getPosition() = Axe(0, Screen->GetCurrentMode()->GetHeight() / 2 - Wall[1]->getDimension()->y / 2);
	
	*Wall[2]->getDimension() = Axe(Animation::getAnimationElement(0, Wall[2]->getAnimations())->GetSprite()->GetWidth(), Animation::getAnimationElement(0, Wall[2]->getAnimations())->GetSprite()->GetHeight() * 3);
	*Wall[2]->getPosition() = Axe(Screen->GetCurrentMode()->GetWidth() / 2 - Wall[2]->getDimension()->x / 2, 0);

	*Wall[3]->getDimension() = Axe(Animation::getAnimationElement(0, Wall[3]->getAnimations())->GetSprite()->GetWidth() * 4, Animation::getAnimationElement(0, Wall[3]->getAnimations())->GetSprite()->GetHeight());
	*Wall[3]->getPosition() = Axe(Screen->GetCurrentMode()->GetWidth() - Wall[3]->getDimension()->x, Screen->GetCurrentMode()->GetHeight() / 2 - Wall[3]->getDimension()->y / 2);
	
	*Wall[4]->getDimension() = Axe(Animation::getAnimationElement(0, Wall[4]->getAnimations())->GetSprite()->GetWidth(), Animation::getAnimationElement(0, Wall[4]->getAnimations())->GetSprite()->GetHeight() * 3);
	*Wall[4]->getPosition() = Axe(Screen->GetCurrentMode()->GetWidth() / 2 - Wall[4]->getDimension()->x / 2, Screen->GetCurrentMode()->GetHeight() - Wall[4]->getDimension()->y);

	*Wall[5]->getDimension() = Axe(Animation::getAnimationElement(0, Wall[5]->getAnimations())->GetSprite()->GetWidth(), Animation::getAnimationElement(0, Wall[5]->getAnimations())->GetSprite()->GetHeight() * 5);
	*Wall[5]->getPosition() = Axe(Wall[1]->getPosition()->x + Wall[1]->getDimension()->x, Wall[1]->getPosition()->y + Wall[1]->getDimension()->y / 2 - Wall[5]->getDimension()->y / 2);

	*Wall[6]->getDimension() = Axe(Animation::getAnimationElement(0, Wall[6]->getAnimations())->GetSprite()->GetWidth() * 5, Animation::getAnimationElement(0, Wall[6]->getAnimations())->GetSprite()->GetHeight());
	*Wall[6]->getPosition() = Axe(Wall[2]->getPosition()->x + Wall[2]->getDimension()->x / 2 - Wall[6]->getDimension()->x / 2, Wall[2]->getPosition()->y + Wall[2]->getDimension()->y);

	*Wall[7]->getDimension() = Axe(Animation::getAnimationElement(0, Wall[7]->getAnimations())->GetSprite()->GetWidth(), Animation::getAnimationElement(0, Wall[7]->getAnimations())->GetSprite()->GetHeight() * 5);
	*Wall[7]->getPosition() = Axe(Wall[3]->getPosition()->x - Wall[7]->getDimension()->x, Wall[3]->getPosition()->y + Wall[3]->getDimension()->y / 2 - Wall[7]->getDimension()->y / 2);

	*Wall[8]->getDimension() = Axe(Animation::getAnimationElement(0, Wall[8]->getAnimations())->GetSprite()->GetWidth() * 5, Animation::getAnimationElement(0, Wall[8]->getAnimations())->GetSprite()->GetHeight());
	*Wall[8]->getPosition() = Axe(Wall[4]->getPosition()->x + Wall[4]->getDimension()->x / 2 - Wall[8]->getDimension()->x / 2, Wall[4]->getPosition()->y - Wall[8]->getDimension()->y);

	for (i = 0; i < NB_WALL; i++)
		Wall[i]->setMosaic(true);
}

void DestroyAll()
{
	int i;

	DestroyDirectDraw();
	DestroyDirectInput();

	delete Info;
	delete Map;
	delete Background;
	for (i = 0; i < NB_WALL; i++)
		delete Wall[i];
	delete Square;
	delete Triangle;
	delete cursor;
	delete Screen;
	delete Keyboard;
	delete Mouse;
}

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_QUIT:
		DestroyAll();
		FreeLibrary(Screen->GetResInst());
		PostQuitMessage(0); //Ferme la fenêtre
		return 0;
	case WM_SETCURSOR:
		SetCursor(NULL); //Définit la valeur du curseur
		return 0;
	case WM_ACTIVATE:
		//Switché sur une autre application
		if (LOWORD(wParam) == WA_INACTIVE || (BOOL)HIWORD(wParam))
		{
			running = false;
			reinit = true;
		}
		else
			running = true;
		return 0;
	}

	if (msg == RegisterWindowMessage("GabGamesConfig"))
	{
		if (wParam == 1 && LoadConfig())
			InitObjects();
		ShowWindow(hWndMain, SW_RESTORE);
		return 0;
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}

bool LoadConfig()
{
	if (!Config.Open(true, false, false))
		return false;

	Config.ReadGraphic(Screen->GetCurrentMode());
	Config.ReadKeys();
	Config.Close();

	return true;
}

void PlayScene(int n)
{
	int i, ObstacleCount, x, y;
	CardinalPoint Obstacle[2500];

	ObstacleCount = 0;

	switch(n)
	{
	case 0:
		Background->runAnimations();
		for (i = 0; i < NB_WALL; i++)
		{
			Obstacle[i].west = (int)Wall[i]->getPosition()->x;
			Obstacle[i].east = (int)Wall[i]->getPosition()->x + (int)Wall[i]->getDimension()->x;
			Obstacle[i].north = (int)Wall[i]->getPosition()->y;
			Obstacle[i].south = (int)Wall[i]->getPosition()->y + (int)Wall[i]->getDimension()->y;
		}
		for (i = 0; i < NB_WALL; i++)
			if (!Wall[i]->getRemoved())
				Wall[i]->runAnimations();
		if (!Square->getRemoved())
		{
			Square->Move(&Edge, true, false);
			for (i = 0; i < NB_WALL; i++)
				Square->GenerateCollision(&Obstacle[i], Obstacle, NB_WALL, true, false);
		}
		if (!Triangle->getRemoved())
		{
			Triangle->Move(&Edge, false, true);
			for (i = 0; i < NB_WALL; i++)
				Triangle->GenerateCollision(&Obstacle[i], Obstacle, NB_WALL, true, true);
		}
		break;
	case 1:
		Map->RenderMap(0);
		for (x = 0; x < Map->GetMapData()->TileCount.x; x++)
			for (y = 0; y < Map->GetMapData()->TileCount.y; y++)
			{
				if (Map->GetMapData()->TileID[y][x][0] >= 2)
				{
					Obstacle[ObstacleCount].west = Map->GetMapData()->TileZone[x].west - (int)Map->GetScreenData()->Camera.getPosition()->x;
					Obstacle[ObstacleCount].east = Map->GetMapData()->TileZone[x].east - (int)Map->GetScreenData()->Camera.getPosition()->x;
					Obstacle[ObstacleCount].north = Map->GetMapData()->TileZone[y].north - (int)Map->GetScreenData()->Camera.getPosition()->y;
					Obstacle[ObstacleCount].south = Map->GetMapData()->TileZone[y].south - (int)Map->GetScreenData()->Camera.getPosition()->y;
					ObstacleCount++;
				}
			}
		if (!Triangle->getRemoved())
			Triangle->SetMapPosition();
		if (!Square->getRemoved())
		{
			Map->Scroll(Square, &Edge, true, false);
			for (i = 0; i < ObstacleCount; i++)
				Square->GenerateCollision(&Obstacle[i], Obstacle, ObstacleCount, true, false);
		}
		if (!Triangle->getRemoved())
		{
			Triangle->GenerateMapPosition();
			Triangle->Move();
			for (i = 0; i < ObstacleCount; i++)
				Triangle->GenerateCollision(&Obstacle[i], Obstacle, ObstacleCount, true, true);
		}
		break;
	}

	if (!Square->getRemoved())
	{
		Square->setVisibleZone();
		Square->runAni(STOP, 0);
	}

	if (!Triangle->getRemoved())
	{
		Triangle->setVisibleZone();
		if (Triangle->GetMagnitude() <= 0)
			Triangle->runAni(STOP, (double)0);
		else
			Triangle->runAni(LOOP, (double)5);
	}

	BloumClock.goDate();

	if (Info->getVisible())
	{
		Date date = Date::currentDate();
		Screen->LoadText(Info->GetTextData(0, 1)->GetSprite(), NULL, BuildString("%.2d/%.2d/%.4d", date.getDay(), date.getMonth(), date.getYear()).Get()/*BloumClock.getDateString(Clock::DATE_FORMAT, LOCALE_SYSTEM_DEFAULT, NULL, "dd/MM/yyyy")*/, RGB(0, 0, 0), RGB(255, 255, 0));
		Info->GetTextData(0, 1)->GetSprite()->SetColorKey(RGB(255, 255, 255));
		Screen->LoadText(Info->GetTextData(1, 1)->GetSprite(), NULL, BuildString("%.2d:%.2d:%.2d", date.getHour(), date.getMinute(), date.getSecond()).Get()/*BloumClock.getDateString(Clock::HOUR_FORMAT, LOCALE_SYSTEM_DEFAULT, NULL, "HH:mm:ss")*/, RGB(0, 0, 0), RGB(255, 255, 0));
		Info->GetTextData(1, 1)->GetSprite()->SetColorKey(RGB(255, 255, 255));
		Screen->LoadText(Info->GetTextData(3, 1)->GetSprite(), NULL, Animation::getAnimationElement(0, cursor->getAnimations())->GetSprite()->GetCurrentAngle(), RGB(0, 0, 0), RGB(255, 255, 0));
		Info->GetTextData(3, 1)->GetSprite()->SetColorKey(RGB(255, 255, 255));
		Screen->LoadText(Info->GetTextData(0, 0)->GetSprite(), NULL, "Colonne 1", RGB(0, 0, 0), RGB(255, 255, 0));
		Info->GetTextData(0, 0)->GetSprite()->SetColorKey(RGB(255, 255, 255));
		Screen->LoadText(Info->GetTextData(1, 0)->GetSprite(), NULL, "Colonne 2", RGB(0, 0, 0), RGB(255, 255, 0));
		Info->GetTextData(1, 0)->GetSprite()->SetColorKey(RGB(255, 255, 255));
		Screen->LoadText(Info->GetTextData(2, 0)->GetSprite(), NULL, "Colonne 3", RGB(0, 0, 0), RGB(255, 255, 0));
		Info->GetTextData(2, 0)->GetSprite()->SetColorKey(RGB(255, 255, 255));
		Screen->LoadText(Info->GetTextData(3, 0)->GetSprite(), NULL, "Colonne 4", RGB(0, 0, 0), RGB(255, 255, 0));
		Info->GetTextData(3, 0)->GetSprite()->SetColorKey(RGB(255, 255, 255));
		Info->GetTextData(2, 1)->Update(128, RGB(0, 0, 0), RGB(255, 255, 0), &BigText);
		Screen->LoadText(Info->GetTextData(0, 2)->GetSprite(), NULL, ReplaceStringByAnother(BigText, "tut", "titum tam").Get(), RGB(0, 0, 0), RGB(255, 255, 0));
		Info->GetTextData(0, 2)->GetSprite()->SetColorKey(RGB(255, 255, 255));
		Info->Refresh();
		Info->Run();
	}

	if (!cursor->getRemoved())
	{
		cursor->setVisibleZone();
		cursor->runAni(STOP, 0);
	}

	Screen->FlipFlap();
}

bool ProcessKbInput()
{
	if (!keybListener.startCaptureKeys(Keyboard))
		return false;

	if (keybListener.keyPressed((Str)"Gauche", Config.GetKeyMap()))
	{
		Square->SetAngle(PI);
		cursor->rotateAni(180);
	}

	if (keybListener.keyPressed((Str)"Droite", Config.GetKeyMap()))
	{
		Square->SetAngle(0);
		cursor->rotateAni(0);
	}

	if (keybListener.keyPressed((Str)"Haut", Config.GetKeyMap()))
	{
		Square->SetAngle(3 * PI / 2);
		cursor->rotateAni(90);
	}

	if (keybListener.keyPressed((Str)"Bas", Config.GetKeyMap()))
	{
		Square->SetAngle(PI / 2);
		cursor->rotateAni(270);
	}

	if (keybListener.keyPressed((Str)"Gauche", Config.GetKeyMap()) && keybListener.keyPressed((Str)"Haut", Config.GetKeyMap()))
		Square->SetAngle(5 * PI / 4);

	if (keybListener.keyPressed((Str)"Gauche", Config.GetKeyMap()) && keybListener.keyPressed((Str)"Bas", Config.GetKeyMap()))
		Square->SetAngle(3 * PI / 4);

	if (keybListener.keyPressed((Str)"Droite", Config.GetKeyMap()) && keybListener.keyPressed((Str)"Haut", Config.GetKeyMap()))
		Square->SetAngle(7 * PI / 4);

	if (keybListener.keyPressed((Str)"Droite", Config.GetKeyMap()) && keybListener.keyPressed((Str)"Bas", Config.GetKeyMap()))
		Square->SetAngle(PI / 4);

	if (!keybListener.keyPressed((Str)"Gauche", Config.GetKeyMap()) && !keybListener.keyPressed((Str)"Droite", Config.GetKeyMap()) && !keybListener.keyPressed((Str)"Haut", Config.GetKeyMap()) && !keybListener.keyPressed((Str)"Bas", Config.GetKeyMap()))
		Square->DecreaseMagnitude(0);
	else
		Square->IncreaseMagnitude(5);

	if (keybListener.keyPressed((Str)"Diminuer angle", Config.GetKeyMap()))
		Triangle->DecreaseAngle(0, 2 * PI, 50);

	if (keybListener.keyPressed((Str)"Augmenter angle", Config.GetKeyMap()))
		Triangle->IncreaseAngle(0, 2 * PI, 50);

	if (keybListener.keyPressed((Str)"Augmenter vitesse", Config.GetKeyMap()))
		Triangle->IncreaseMagnitude(100);

	if (keybListener.keyPressed((Str)"Diminuer vitesse", Config.GetKeyMap()))
		Triangle->DecreaseMagnitude(0);

	if (keybListener.keyReleased((Str)"Afficher / cacher infos", Config.GetKeyMap()))
		if (Info->getVisible())
			Info->hide();
		else
			Info->show();

	if (keybListener.keyPressed((Str)"Scène 0", Config.GetKeyMap()))
	{
		if (NrScene != 0)
		{
			Triangle->initDynColors(1);
			Triangle->addDynColor(0, new DynColor(RGB(255, 0, 0), RGB(255, 153, 51)));
			Triangle->replaceAniColors();
		}
		NrScene = 0;
	}

	if (keybListener.keyPressed((Str)"Scène 1", Config.GetKeyMap()))
	{
		if (NrScene != 1)
		{
			Triangle->initDynColors(1);
			Triangle->addDynColor(0, new DynColor(RGB(255, 153, 51), RGB(255, 0, 0)));
			Triangle->replaceAniColors();
		}
		NrScene = 1;
	}

	if (keybListener.keyPressed((Str)"Configuration", Config.GetKeyMap()))
	{
		ShowWindow(hWndMain, SW_MINIMIZE);
		ShellExecute(hWndMain, "open", "GameConfig.exe", NULL, NULL, SW_SHOW);
	}

	if (keybListener.keyReleased((Str)"Plein écran / fenêtré", Config.GetKeyMap()))
	{
		DestroyDirectDraw();
		DestroyDirectInput();
		Screen->GetCurrentMode()->Switch();
		RestoreDirectDraw();
		InitDirectInput();
	}

	if (keybListener.keyPressed((Str)"Quitter", Config.GetKeyMap()))
		WndProc(hWndMain, WM_QUIT, NULL, NULL);

	keybListener.captureKeys();

	return true;
}

void ProcessMouseInput()
{
	DIMOUSESTATE DIms;
	HRESULT hr;
	CardinalPoint ScreenLimit;

	ScreenLimit.east = Screen->GetCurrentMode()->GetWidth();
	ScreenLimit.south = Screen->GetCurrentMode()->GetHeight();

	hr = Mouse->GetDevice()->GetDeviceState(sizeof(DIMOUSESTATE), (LPVOID)&DIms);

	if (hr != DI_OK)
		ReportDxError(S_FALSE, DIRECTINPUT, ERR_DI_GETDEVICESTATE);
	else
	{
		cursor->getMover()->GenerateMouseAngle(DIms.lX, DIms.lY);
		cursor->getMover()->Move(DIms.lX, DIms.lY, &ScreenLimit, true);
		cursor->stickToMover();
	}
}
