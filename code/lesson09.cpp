#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>							// Windows的头文件
#include <stdio.h>								// 标准输入/输出库的头文件
#include <GL/glut.h>
#include <GL/GLAUX.H>								// GLaux库的头文件

HGLRC           hRC = NULL;							// 窗口着色描述表句柄
HDC             hDC = NULL;							// OpenGL渲染描述表句柄
HWND            hWnd = NULL;						// 保存我们的窗口句柄
HINSTANCE       hInstance;							// 保存程序的实例

bool	keys[256];									// 保存键盘按键的数组
bool	active = TRUE;								// 窗口的活动标志，缺省为TRUE（最小化）
bool	fullscreen = TRUE;							// 全屏标志缺省，缺省设定成全屏模式

LRESULT	CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);		// WndProc的定义


BOOL	twinkle;						// 闪烁
BOOL	tp;								// 'T' 按下了么? 

const int num = 50;							// 绘制的星星数

typedef struct							// 为星星创建一个结构
{
	int r, g, b;						// 星星的颜色
	GLfloat dist;						// 星星距离中心的距离
	GLfloat angle;						// 当前星星所处的角度
}stars;									// 结构命名为stars

stars star[num];						// 使用 'stars' 结构生成一个包含 'num'个元素的 'star'数组

GLfloat	zoom = -15.0f;					// 星星离观察者的距离
GLfloat tilt = 90.0f;					// 星星的倾角
GLfloat	spin;							// 闪烁星星的自转

GLuint	loop;							// 全局 Loop 变量
GLuint	texture[1];						// 存放一个纹理

AUX_RGBImageRec* LoadBMP(char* Filename) {			// 载入位图文件
	FILE* File = NULL;								// 文件句柄
	if (!Filename) {								// 确认已给出文件名
		return NULL;								// 若无返回 NULL
	}
	File = fopen(Filename, "r");					// 检查文件是否存在
	if (File) {										// 文件存在么?
		fclose(File);								// 关闭文件句柄
		return auxDIBImageLoad(Filename);			// 载入位图并返回指针
	}
	return NULL;									// 如果载入失败返回 NULL
}

int LoadGLTextures()								// 载入位图并转换成纹理
{
	int Status = FALSE;								// 状态指示器

	AUX_RGBImageRec* TextureImage[1];				// 为纹理分配存储空间

	memset(TextureImage, 0, sizeof(void*) * 1);		// 将指针设为 NULL

	// 载入位图，查错，如果未找到位图文件则退出
	if (TextureImage[0] = LoadBMP((char*)"Data/Star.bmp")){
		Status = TRUE;							// 将 Status 设为TRUE
		glGenTextures(1, &texture[0]);			// 创建一个纹理
		// 创建一个线性滤波纹理
		glBindTexture(GL_TEXTURE_2D, texture[0]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, 3, TextureImage[0]->sizeX, TextureImage[0]->sizeY, 0, GL_RGB, GL_UNSIGNED_BYTE, TextureImage[0]->data);
	}

	if (TextureImage[0]) {						// 如果纹理存在
		if (TextureImage[0]->data) {			// 如果纹理图像存在
			free(TextureImage[0]->data);		// 释放纹理图像所占的内存
		}
		free(TextureImage[0]);					// 释放图像结构
	}
	return Status;								// 返回 Status的值
}

int InitGL(GLvoid) {									// 此处开始对OpenGL进行所有设置
	if (!LoadGLTextures()) {							// 调用纹理载入子例程
		return FALSE;									// 如果未能载入，返回FALSE
	}

	glEnable(GL_TEXTURE_2D);							// 启用纹理映射
	glShadeModel(GL_SMOOTH);							// 启用阴影平滑
	glClearColor(0.0f, 0.0f, 0.0f, 0.5f);				// 黑色背景
	glClearDepth(1.0f);									// 设置深度缓存
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);	// 真正精细的透视修正
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);					// 设置混色函数取得半透明效果
	glEnable(GL_BLEND);									// 启用混色

	for (loop = 0; loop < num; loop++) {				// 创建循环设置全部星星
		star[loop].angle = 0.0f;						// 所有星星都从零角度开始
		star[loop].dist = (float(loop) / num) * 5.0f;	// 计算星星离中心的距离
		star[loop].r = rand() % 256;					// 为star[loop]设置随机红色分量
		star[loop].g = rand() % 256;					// 为star[loop]设置随机绿色分量
		star[loop].b = rand() % 256;					// 为star[loop]设置随机蓝色分量
	}
	return TRUE;										// 初始化一切OK
}

GLvoid ReSizeGLScene(GLsizei width, GLsizei height) {			// 重置OpenGL窗口大小
	if (height == 0) {											// 防止被零除
		height = 1;												// 将Height设为1
	}
	glViewport(0, 0, width, height);							// 重置当前的视口
	glMatrixMode(GL_PROJECTION);								// 选择投影矩阵
	glLoadIdentity();											// 重置投影矩阵

	// 设置视口的大小
	gluPerspective(45.0f, (GLfloat)width / (GLfloat)height, 0.1f, 100.0f);

	glMatrixMode(GL_MODELVIEW);									// 选择模型观察矩阵
	glLoadIdentity();											// 重置模型观察矩阵
}

int DrawGLScene(GLvoid) {								// 此过程中包括所有的绘制代码
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	// 清除屏幕及深度缓存
	glBindTexture(GL_TEXTURE_2D, texture[0]);			// 选择纹理

	for (loop = 0; loop < num; loop++) {				// 循环设置所有的星星
		glLoadIdentity();								// 绘制每颗星星之前，重置模型观察矩阵
		glTranslatef(0.0f, 0.0f, zoom);					// 深入屏幕里面
		glRotatef(tilt, 1.0f, 0.0f, 0.0f);				// 倾斜视角
		glRotatef(star[loop].angle, 0.0f, 1.0f, 0.0f);	// 旋转至当前所画星星的角度
		glTranslatef(star[loop].dist, 0.0f, 0.0f);		// 沿X轴正向移动
		glRotatef(-star[loop].angle, 0.0f, 1.0f, 0.0f);	// 取消当前星星的角度
		glRotatef(-tilt, 1.0f, 0.0f, 0.0f);				// 取消屏幕倾斜

		if (twinkle) {									// 启用闪烁效果
			// 使用byte型数值指定一个颜色
			glColor4ub(star[(num - loop) - 1].r, star[(num - loop) - 1].g, star[(num - loop) - 1].b, 255);
			glBegin(GL_QUADS);							// 开始绘制纹理映射过的四边形
			glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.0f, -1.0f, 0.0f);
			glTexCoord2f(1.0f, 0.0f); glVertex3f(1.0f, -1.0f, 0.0f);
			glTexCoord2f(1.0f, 1.0f); glVertex3f(1.0f, 1.0f, 0.0f);
			glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.0f, 1.0f, 0.0f);
			glEnd();									// 四边形绘制结束
		}

		glRotatef(spin, 0.0f, 0.0f, 1.0f);				// 绕z轴旋转星星
		// 使用byte型数值指定一个颜色
		glColor4ub(star[loop].r, star[loop].g, star[loop].b, 255);
		glBegin(GL_QUADS);								// 开始绘制纹理映射过的四边形
		glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.0f, -1.0f, 0.0f);
		glTexCoord2f(1.0f, 0.0f); glVertex3f(1.0f, -1.0f, 0.0f);
		glTexCoord2f(1.0f, 1.0f); glVertex3f(1.0f, 1.0f, 0.0f);
		glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.0f, 1.0f, 0.0f);
		glEnd();										// 四边形绘制结束

		spin += 0.00001f;								// 星星的公转
		star[loop].angle += float(loop) / (num * 10);	// 改变星星的自转角度
		star[loop].dist -= 0.00001f;					// 改变星星离中心的距离

		if (star[loop].dist < 0.0f) {					// 星星到达中心了么
			star[loop].dist += 5.0f;					// 往外移5个单位
			star[loop].r = rand() % 256;				// 赋一个新红色分量
			star[loop].g = rand() % 256;				// 赋一个新绿色分量
			star[loop].b = rand() % 256;				// 赋一个新蓝色分量
		}
	}
	return TRUE;										// 一切正常
}

GLvoid KillGLWindow(GLvoid) {
	if (fullscreen) {											// 我们处于全屏模式吗?
		ChangeDisplaySettings(NULL, 0);							// 是的话，切换回桌面
		ShowCursor(TRUE);										// 显示鼠标指针
	}
	if (hRC) {													// 我们拥有OpenGL渲染描述表吗?
		if (!wglMakeCurrent(NULL, NULL)) {						// 我们能否释放DC和RC描述表?
			MessageBox(NULL, "释放DC或RC失败。", "关闭错误", MB_OK | MB_ICONINFORMATION);
		}
		if (!wglDeleteContext(hRC)) {							// 我们能否删除RC?
			MessageBox(NULL, "释放RC失败。", "关闭错误", MB_OK | MB_ICONINFORMATION);
		}
		hRC = NULL;												// 将RC设为 NULL
	}
	if (hDC && !ReleaseDC(hWnd, hDC)) {							// 我们能否释放 DC?
		MessageBox(NULL, "释放DC失败。", "关闭错误", MB_OK | MB_ICONINFORMATION);
		hDC = NULL;												// 将 DC 设为 NULL
	}
	if (hWnd && !DestroyWindow(hWnd)) {							// 能否销毁窗口?
		MessageBox(NULL, "释放窗口句柄失败。", "关闭错误", MB_OK | MB_ICONINFORMATION);
		hWnd = NULL;											// 将 hWnd 设为 NULL
	}
	if (!UnregisterClass("OpenG", hInstance)) {					// 能否注销类?
		MessageBox(NULL, "不能注销窗口类。", "关闭错误", MB_OK | MB_ICONINFORMATION);
		hInstance = NULL;										// 将 hInstance 设为 NULL
	}
}

BOOL CreateGLWindow(char* title, int width, int height, int bits, bool fullscreenflag) {
	GLuint	PixelFormat;											// 保存查找匹配的结果
	WNDCLASS	wc;													// 窗口类结构
	DWORD	dwExStyle;												// 扩展窗口风格
	DWORD	dwStyle;												// 窗口风格

	RECT WindowRect;												// 取得矩形的左上角和右下角的坐标值
	WindowRect.left = (long)0;										// 将Left   设为 0
	WindowRect.right = (long)width;									// 将Right  设为要求的宽度
	WindowRect.top = (long)0;										// 将Top    设为 0
	WindowRect.bottom = (long)height;								// 将Bottom 设为要求的高度

	fullscreen = fullscreenflag;									// 设置全局全屏标志

	hInstance = GetModuleHandle(NULL);								// 取得我们窗口的实例
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;					// 移动时重画，并为窗口取得DC
	wc.lpfnWndProc = (WNDPROC)WndProc;								// WndProc处理消息
	wc.cbClsExtra = 0;												// 无额外窗口数据
	wc.cbWndExtra = 0;												// 无额外窗口数据
	wc.hInstance = hInstance;										// 设置实例
	wc.hIcon = LoadIcon(NULL, IDI_WINLOGO);							// 装入缺省图标
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);						// 装入鼠标指针
	wc.hbrBackground = NULL;										// GL不需要背景
	wc.lpszMenuName = NULL;											// 不需要菜单
	wc.lpszClassName = "OpenG";										// 设定类名字

	if (!RegisterClass(&wc)) {										// 尝试注册窗口类
		MessageBox(NULL, "注册窗口失败", "错误", MB_OK | MB_ICONEXCLAMATION);
		return FALSE;												// 退出并返回FALSE
	}

	if (fullscreen) {												// 要尝试全屏模式吗?
		DEVMODE dmScreenSettings;									// 设备模式
		memset(&dmScreenSettings, 0, sizeof(dmScreenSettings));		// 确保内存清空为零
		dmScreenSettings.dmSize = sizeof(dmScreenSettings);			// Devmode 结构的大小
		dmScreenSettings.dmPelsWidth = width;						// 所选屏幕宽度
		dmScreenSettings.dmPelsHeight = height;						// 所选屏幕高度
		dmScreenSettings.dmBitsPerPel = bits;						// 每象素所选的色彩深度
		dmScreenSettings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

		// 尝试设置显示模式并返回结果。注: CDS_FULLSCREEN 移去了状态条。
		if (ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL) {
			// 若模式失败，提供两个选项：退出或在窗口内运行。
			if (MessageBox(NULL, "全屏模式在当前显卡上设置失败！\n使用窗口模式？", "NeHe G", MB_YESNO | MB_ICONEXCLAMATION) == IDYES) {
				fullscreen = FALSE;									// 选择窗口模式(Fullscreen=FALSE)
			}
			else {
				// 弹出一个对话框，告诉用户程序结束
				MessageBox(NULL, "程序将被关闭", "错误", MB_OK | MB_ICONSTOP);
				return FALSE;										//  退出并返回 FALSE
			}
		}
	}

	if (fullscreen) {												// 仍处于全屏模式吗?
		dwExStyle = WS_EX_APPWINDOW;								// 扩展窗体风格
		dwStyle = WS_POPUP;											// 窗体风格
		ShowCursor(FALSE);											// 隐藏鼠标指针
	}
	else {
		dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;				// 扩展窗体风格
		dwStyle = WS_OVERLAPPEDWINDOW;								//  窗体风格
	}

	AdjustWindowRectEx(&WindowRect, dwStyle, FALSE, dwExStyle);		// 调整窗口达到真正要求的大小

	//创建窗口并检查窗口是否成功创建
	if (!(hWnd = CreateWindowEx(dwExStyle,							// 扩展窗体风格
		"OpenG",													// 类名字
		title,														// 窗口标题
		WS_CLIPSIBLINGS |											// 必须的窗体风格属性
		WS_CLIPCHILDREN |											// 必须的窗体风格属性
		dwStyle,													// 选择的窗体属性
		0, 0,														// 窗口位置
		WindowRect.right - WindowRect.left,							// 计算调整好的窗口宽度
		WindowRect.bottom - WindowRect.top,							// 计算调整好的窗口高度
		NULL,														// 无父窗口
		NULL,														// 无菜单
		hInstance,													// 实例
		NULL))) {													// 不向WM_CREATE传递任何东西

		KillGLWindow();												// 重置显示区
		MessageBox(NULL, "不能创建一个窗口设备描述表", "错误", MB_OK | MB_ICONEXCLAMATION);
		return FALSE;												// 返回 FALSE
	}

	//下面的代码描述象素格式
	static	PIXELFORMATDESCRIPTOR pfd =								//pfd 告诉窗口我们所希望的东西，即窗口使用的像素格式
	{
		sizeof(PIXELFORMATDESCRIPTOR),								// 上述格式描述符的大小
		1,															// 版本号
		PFD_DRAW_TO_WINDOW |										// 格式支持窗口
		PFD_SUPPORT_OPENGL |										// 格式必须支持OpenGL
		PFD_DOUBLEBUFFER,											// 必须支持双缓冲
		PFD_TYPE_RGBA,												// 申请 RGBA 格式
		bits,														// 选定色彩深度
		0, 0, 0, 0, 0, 0,											// 忽略的色彩位
		0,															// 无Alpha缓存
		0,															// 忽略Shift Bit
		0,															// 无累加缓存
		0, 0, 0, 0,													// 忽略聚集位
		32,															// 32位 Z-缓存 (深度缓存)
		0,															// 无蒙板缓存
		0,															// 无辅助缓存
		PFD_MAIN_PLANE,												// 主绘图层
		0,															// Reserved
		0, 0, 0														// 忽略层遮罩
	};

	if (!(hDC = GetDC(hWnd))) {										// 取得设备描述表了么?
		KillGLWindow();												// 重置显示区
		MessageBox(NULL, "不能创建一种相匹配的像素格式", "错误", MB_OK | MB_ICONEXCLAMATION);
		return FALSE;												// 返回 FALSE
	}

	if (!(PixelFormat = ChoosePixelFormat(hDC, &pfd))) {			// Windows 找到相应的象素格式了吗?
		KillGLWindow();												// 重置显示区
		MessageBox(NULL, "不能设置像素格式", "错误", MB_OK | MB_ICONEXCLAMATION);
		return FALSE;												// 返回 FALSE
	}

	if (!SetPixelFormat(hDC, PixelFormat, &pfd)) {					// 能够设置象素格式么?
		KillGLWindow();												// 重置显示区
		MessageBox(NULL, "不能设置像素格式", "错误", MB_OK | MB_ICONEXCLAMATION);
		return FALSE;												// 返回 FALSE
	}

	if (!(hRC = wglCreateContext(hDC))) {							// 能否取得着色描述表?
		KillGLWindow();												// 重置显示区
		MessageBox(NULL, "不能创建OpenGL渲染描述表", "错误", MB_OK | MB_ICONEXCLAMATION);
		return FALSE;												// 返回 FALSE
	}

	if (!wglMakeCurrent(hDC, hRC)) {								// 尝试激活着色描述表
		KillGLWindow();												// 重置显示区
		MessageBox(NULL, "不能激活当前的OpenGL渲然描述表", "错误", MB_OK | MB_ICONEXCLAMATION);
		return FALSE;												// 返回 FALSE
	}

	ShowWindow(hWnd, SW_SHOW);										// 显示窗口
	SetForegroundWindow(hWnd);										// 略提高优先级
	SetFocus(hWnd);													// 设置键盘的焦点至此窗口
	ReSizeGLScene(width, height);									// 设置透视 GL 屏幕

	if (!InitGL()) {												// 初始化新建的GL窗口
		KillGLWindow();												// 重置显示区
		MessageBox(NULL, "Initialization Failed.", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return FALSE;												// 返回 FALSE
	}

	return TRUE;													// 成功
}

LRESULT CALLBACK WndProc(HWND	hWnd,								// 窗口的句柄
	UINT	uMsg,													// 窗口的消息
	WPARAM	wParam,													// 附加的消息内容
	LPARAM	lParam)													// 附加的消息内容
{
	switch (uMsg) {													// 检查Windows消息

	case WM_ACTIVATE:											// 监视窗口激活消息
	{
		if (!HIWORD(wParam)) {									// 检查最小化状态
			active = TRUE;										// 程序处于激活状态
		}
		else {
			active = FALSE;										// 程序不再激活
		}
		return 0;												// 返回消息循环
	}

	case WM_SYSCOMMAND:											// 系统中断命令
	{
		switch (wParam) {										// 检查系统调用
		case SC_SCREENSAVE:									// 屏保要运行?
		case SC_MONITORPOWER:								// 显示器要进入节电模式?
			return 0;											// 阻止发生
		}
		break;													// 退出
	}

	case WM_CLOSE:												// 收到Close消息?
	{
		PostQuitMessage(0);										// 发出退出消息
		return 0;												// 返回
	}

	case WM_KEYDOWN:											// 有键按下么?
	{
		keys[wParam] = TRUE;									// 如果是，设为TRUE
		return 0;												// 返回
	}

	case WM_KEYUP:												// 有键放开么?
	{
		keys[wParam] = FALSE;									// 如果是，设为FALSE
		return 0;												// 返回
	}

	case WM_SIZE:												// 调整OpenGL窗口大小
	{
		ReSizeGLScene(LOWORD(lParam), HIWORD(lParam));			// LoWord=Width,HiWord=Height
		return 0;												// 返回
	}
	}

	// 向 DefWindowProc传递所有未处理的消息，window自行处理
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(_In_ HINSTANCE	hInstance,							// 当前窗口实例
	_In_opt_ HINSTANCE	hPrevInstance,									// 前一个窗口实例
	_In_ LPSTR		lpCmdLine,											// 命令行参数
	_In_ int		nCmdShow)											// 窗口显示状态
{
	MSG	msg;														// Windowsx消息结构
	BOOL	done = FALSE;											// 用来退出循环的Bool 变量

	// 提示用户选择运行模式，可选代码
	if (MessageBox(NULL, "你想在全屏模式下运行么？", "设置全屏模式", MB_YESNO | MB_ICONQUESTION) == IDNO) {
		fullscreen = FALSE;											// FALSE为窗口模式
	}

	// 创建OpenGL窗口
	if (!CreateGLWindow((char*)"OpenGL程序框架", 640, 480, 32, fullscreen)) {
		return 0;													// 失败退出
	}

	while (!done) {													// 保持循环直到 done=TRUE
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {				// 有消息在等待吗?
			if (msg.message == WM_QUIT) {							// 收到退出消息?
				done = TRUE;										// 是，则done=TRUE
			}
			else {													// 不是，处理窗口消息
				TranslateMessage(&msg);								// 翻译消息
				DispatchMessage(&msg);								// 发送消息
			}
		}
		else {														// 如果没有消息
			// 绘制场景。监视ESC键和来自DrawGLScene()的退出消息
			if (active) {											// 程序激活的么?
				if (keys[VK_ESCAPE]) {								// ESC 按下了么?
					done = TRUE;									// ESC 发出退出信号
				}
				else {												// 不是退出的时候，刷新屏幕
					DrawGLScene();									// 绘制场景
					SwapBuffers(hDC);								// 交换缓存 (双缓存)

					if (keys['T'] && !tp) {				// 是否T 键已按下并且 tp值为 FALSE
						tp = TRUE;						// 若是，将tp设为TRUE
						twinkle = !twinkle;				// 翻转 twinkle的值
					}
					if (!keys['T']) {					// T 键已松开了么？
						tp = FALSE;						// 若是 ，tp为 FALSE
					}

					if (keys[VK_UP]) {				// 上方向键按下了么？
						tilt -= 0.5f;				// 屏幕向上倾斜
					}

					if (keys[VK_DOWN]) {			// 下方向键按下了么？
						tilt += 0.5f;				// 屏幕向下倾斜
					}

					if (keys['I']) {				// 向上翻页键按下了么
						zoom -= 0.2f;				// 缩小
					}

					if (keys['O']) {				// 向下翻页键按下了么？
						zoom += 0.2f;				// 放大
					}

					if (keys[VK_F1]) {										// F1键按下了么?
						keys[VK_F1] = FALSE;								// 若是，使对应的Key数组中的值为 FALSE
						KillGLWindow();										// 销毁当前的窗口
						fullscreen = !fullscreen;							// 切换 全屏 / 窗口 模式
						// 重建 OpenGL 窗口
						if (!CreateGLWindow((char*)"OpenGL 程序框架", 640, 480, 32, fullscreen)) {
							return 0;										// 如果窗口未能创建，程序退出
						}
					}
				}
			}
		}
	}

	// 关闭程序
	KillGLWindow();													// 销毁窗口
	return (msg.wParam);											// 退出程序
}
