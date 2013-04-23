#include <Windows.h>
#include <gl\GL.h>
#include <gl\GLU.h>
#include <gl\glaux.h>

#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glu32.lib")
HGLRC hRC	= NULL;	// постоянный контекст рендеринга
HDC	hDC		= NULL;
HWND hWnd	= NULL;
HINSTANCE hInstance;

bool keys[256];		// массив для клавиш
bool active		= true;	// флаг сворачивания в трэй
bool fullscreen = true;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

GLvoid ReSizeGLScene (GLsizei width, GLsizei height) {
	if(height == 0) height = 1;
	glViewport(0, 0, width, height);	// сброс текущей области вывода

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	gluPerspective( 45.0f, (GLfloat)width/(GLfloat)height, 0.1f, 100.0f );

	glMatrixMode (GL_MODELVIEW );
	glLoadIdentity();
}

int InitGL( GLvoid ) {
	glShadeModel(GL_SMOOTH);		// плавное цветное сглаживание
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
// буфер глубины	
	glClearDepth(1.0f);			
	glEnable(GL_DEPTH_TEST);	
	glDepthFunc(GL_LEQUAL);		
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	return true;
}

int DrawGLScene(GLvoid) {
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();
	glTranslatef(-1.5f, 0.0f, -6.0f);
	glBegin(GL_TRIANGLES);
		glVertex3f( 0.0f, 1.0f, 0.0f);  // Вверх
		glVertex3f(-1.0f,-1.0f, 0.0f);  // Слева снизу
		glVertex3f( 1.0f,-1.0f, 0.0f);  // Справа снизу
	glEnd();
	return true;
}

GLvoid KillGLWindow(GLvoid) {
	if(fullscreen) {
		ChangeDisplaySettings(NULL, 0);
		ShowCursor(true);
	}
	if(hRC) {
		if(!wglMakeCurrent(NULL, NULL)) {
			MessageBox(NULL, L"Release Of DC And RC Failed", L"ShutDown Error", MB_OK|MB_ICONINFORMATION);
		}
		if(!wglDeleteContext(hRC)) {
			MessageBox(NULL, L"Release Renderind Context Failed", L"ShutDown Error", MB_OK|MB_ICONINFORMATION);
			hRC = NULL;
		}
		if(hDC && !ReleaseDC(hWnd, hDC)) {
			MessageBox(NULL, L"Release Device Context Failed.", L"ShutDown Error", MB_OK|MB_ICONINFORMATION);
			hDC = NULL;
		}
		if(hWnd && !DestroyWindow(hWnd)) {
			MessageBox(NULL, L"Could Not Release hWnd.", L"ShutDown Error", MB_OK|MB_ICONINFORMATION);
			hWnd = NULL;
		}
		if(!UnregisterClass (L"OpenGL", hInstance)) {
			MessageBox(NULL, L"Cout Not Unregister Class.", L"ShutDown Error", MB_OK|MB_ICONINFORMATION);
			hInstance = NULL;
		}
	}
}

BOOL CreateGLWindow(LPCWSTR title, int width, int height, int bits, bool fullscreenflag) {
	GLuint PixelFormat;
	WNDCLASS wc;
	DWORD dwExStyle;	// Расширенный стиль окна
	DWORD dwStyle;		// обычный стиль окна
	RECT WindowRect;
	WindowRect.left		= (long)0;
	WindowRect.right	= (long)width;
	WindowRect.top		= (long)0;
	WindowRect.bottom	= (long)height;

	fullscreen = fullscreenflag;

	hInstance			= GetModuleHandle(NULL);
	wc.style			= CS_HREDRAW|CS_VREDRAW|CS_OWNDC;
	wc.lpfnWndProc		= (WNDPROC)WndProc;
	wc.cbClsExtra		= 0;
	wc.cbWndExtra		= 0;
	wc.hInstance		= hInstance;
	wc.hIcon			= LoadIcon(NULL, IDI_WINLOGO);
	wc.hCursor			= LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground	= NULL;
	wc.lpszMenuName		= NULL;
	wc.lpszClassName	= L"OpenGL";

	if(!RegisterClass(&wc)) {
		MessageBox(NULL, L"Failed To Register The Window Class", L"Error", MB_OK|MB_ICONEXCLAMATION);
		return false;
	}

	if(fullscreen) {
		DEVMODE dmScreenSettings;            // Режим устройства
		memset( &dmScreenSettings, 0, sizeof( dmScreenSettings ) );    // Очистка для хранения установок
		dmScreenSettings.dmSize=sizeof( dmScreenSettings );      // Размер структуры Devmode
		dmScreenSettings.dmPelsWidth  =   width;        // Ширина экрана
		dmScreenSettings.dmPelsHeight  =   height;        // Высота экрана
		dmScreenSettings.dmBitsPerPel  =   bits;        // Глубина цвета
		dmScreenSettings.dmFields= DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;// Режим Пикселя
		if(ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN) !=DISP_CHANGE_SUCCESSFUL) {
			if(MessageBox(NULL, L"The Requested Fullscreen Mode Is Not Suppirted By\nYour Video Card. Use Windowed Mode Instead?",
				L"NeHe GL", MB_YESNO|MB_ICONEXCLAMATION) == IDYES ) { fullscreen = false; }
			else { MessageBox(NULL, L"Program WIll Now Close", L"Error", MB_OK|MB_ICONSTOP); return false; }
		}
	}
	if(fullscreen) {
		dwExStyle = WS_EX_APPWINDOW;
		dwStyle = WS_POPUP;
		ShowCursor(false);
	}
	else {
		dwExStyle = WS_EX_APPWINDOW|WS_EX_WINDOWEDGE;
		dwStyle = WS_OVERLAPPEDWINDOW;
	}

	AdjustWindowRectEx(&WindowRect, dwStyle, false, dwExStyle);

	if( !( hWnd = CreateWindowEx(  dwExStyle, L"OpenGL", title, WS_CLIPSIBLINGS|WS_CLIPCHILDREN|dwStyle,
		0, 0, WindowRect.right-WindowRect.left, WindowRect.bottom-WindowRect.top, NULL, NULL, hInstance, NULL ) ) ) {
			KillGLWindow();                // Восстановить экран
			MessageBox( NULL, L"Window Creation Error.", L"ERROR", MB_OK | MB_ICONEXCLAMATION );
			return false;                // Вернуть false
	}
 
	static  PIXELFORMATDESCRIPTOR pfd = {
		sizeof(PIXELFORMATDESCRIPTOR), 1, PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER | PFD_TYPE_RGBA,
		bits, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 32, 0, 0, PFD_MAIN_PLANE, 0, 0, 0, 0 
	};

	if( !( hDC = GetDC( hWnd ) ) ) {
		KillGLWindow();                // Восстановить экран
		MessageBox( NULL, L"Can't Create A GL Device Context.", L"ERROR", MB_OK | MB_ICONEXCLAMATION );
		return false;                // Вернуть false
	}
	if( !( PixelFormat = ChoosePixelFormat( hDC, &pfd ) ) )        // Найден ли подходящий формат пикселя?
	{
		KillGLWindow();                // Восстановить экран
		MessageBox( NULL, L"Can't Find A Suitable PixelFormat.", L"ERROR", MB_OK | MB_ICONEXCLAMATION );
		return false;                // Вернуть false
	}
	if( !SetPixelFormat( hDC, PixelFormat, &pfd ) )          // Возможно ли установить Формат Пикселя?
	{
		KillGLWindow();                // Восстановить экран
		MessageBox( NULL, L"Can't Set The PixelFormat.", L"ERROR", MB_OK | MB_ICONEXCLAMATION );
		return false;                // Вернуть false
	}
	if( !( hRC = wglCreateContext( hDC ) ) )          // Возможно ли установить Контекст Рендеринга?
	{
		KillGLWindow();                // Восстановить экран
		MessageBox( NULL, L"Can't Create A GL Rendering Context.", L"ERROR", MB_OK | MB_ICONEXCLAMATION);
		return false;                // Вернуть false
	}
	if( !wglMakeCurrent( hDC, hRC ) )            // Попробовать активировать Контекст Рендеринга
	{
		KillGLWindow();                // Восстановить экран
		MessageBox( NULL, L"Can't Activate The GL Rendering Context.", L"ERROR", MB_OK | MB_ICONEXCLAMATION );
		return false;                // Вернуть false
	}
	ShowWindow( hWnd, SW_SHOW );             // Показать окно
	SetForegroundWindow( hWnd );             // Слегка повысим приоритет
	SetFocus( hWnd );						// Установить фокус клавиатуры на наше окно
	ReSizeGLScene( width, height );          // Настроим перспективу для нашего OpenGL экрана.

	if( !InitGL() )                  // Инициализация только что созданного окна
	{
		KillGLWindow();                // Восстановить экран
		MessageBox( NULL, L"Initialization Failed.", L"ERROR", MB_OK | MB_ICONEXCLAMATION );
		return false;                // Вернуть false
	}
	return true;                  // Всё в порядке!
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch(uMsg) {
	case WM_ACTIVATE:
		if( !HIWORD(wParam) ) {
			active = true;
		}
		else {
			active = false;
		}
		return 0;
	case WM_SYSCOMMAND:
		switch (wParam) {
		case SC_SCREENSAVE:
		case SC_MONITORPOWER:
			return 0;
		}
		break;
	case WM_CLOSE:
		PostQuitMessage(0);
		return 0;
	case WM_KEYDOWN:
		keys[wParam] = true;
		return 0;
	case WM_KEYUP:
		keys[wParam] = false;
		return 0;
	case WM_SIZE:
		ReSizeGLScene(LOWORD(lParam), HIWORD(lParam));
		return 0;
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	MSG msg;
	BOOL done = false;
	if(MessageBox(NULL, L"Хотите ли Вы запустить приложение в полноэкранном режиме?", L"FullScreen",
		MB_YESNO|MB_ICONQUESTION) == IDNO ) {
			fullscreen = false;
	}
	if(!CreateGLWindow(L"NeHe OpenGL окно", 1024, 768, 32, fullscreen)) {
		return 0;
	}
	while(!done) {
		if(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			if(msg.message == WM_QUIT) {
				done = true;
			}
			else {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		else {
			if(active) {
				if(keys[VK_ESCAPE]) {
					done = true;
				}
				else {
					DrawGLScene();
					SwapBuffers(hDC);
				}
				if(keys[VK_F1]) {
					keys[VK_F1] = false;
					KillGLWindow();
					fullscreen = !fullscreen;
					if(!CreateGLWindow(L"NeHe OpenGL структура", 1024, 768, 32, fullscreen)) {
						return 0;
					}
				}
			}
		}
	}
	KillGLWindow();
	return msg.wParam;
}