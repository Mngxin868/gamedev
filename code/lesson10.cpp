#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>							// Windows的头文件
#include <stdio.h>								// 标准输入/输出库的头文件
#include <GL/glut.h>
#include <GL/GLAUX.H>								// GLaux库的头文件
#include <math.h>

HGLRC           hRC = NULL;							// 窗口着色描述表句柄
HDC             hDC = NULL;							// OpenGL渲染描述表句柄
HWND            hWnd = NULL;						// 保存我们的窗口句柄
HINSTANCE       hInstance;							// 保存程序的实例

bool	keys[256];									// 保存键盘按键的数组
bool	active = TRUE;								// 窗口的活动标志，缺省为TRUE（最小化）
bool	fullscreen = TRUE;							// 全屏标志缺省，缺省设定成全屏模式
bool	blend;										// Blending ON/OFF
bool	bp;											// 'B'按下了吗?
bool	fp;											// 'F'按下了吗?

const float piover180 = 0.0174532925f;
float heading;
float xpos;
float zpos;

GLfloat	yrot;										// Y 旋转量
GLfloat walkbias = 0;
GLfloat walkbiasangle = 0;
GLfloat lookupdown = 0.0f;
GLfloat	z = 0.0f;									// 深入屏幕的距离

GLuint	filter;										// 滤波类型
GLuint	texture[3];									// 3种纹理的储存空间

//三角形
typedef struct tagVERTEX {							// 创建Vertex顶点结构
	float x, y, z;									// 3D 坐标
	float u, v;										// 纹理坐标
} VERTEX;											// 命名为VERTEX

//多边形
typedef struct tagTRIANGLE {						// 创建Triangle三角形结构
	VERTEX vertex[3];								// VERTEX矢量数组，大小为3
} TRIANGLE;											// 命名为 TRIANGLE

//3D区段
typedef struct tagSECTOR {							// 创建Sector区段结构
	int numtriangles;								// Sector中的三角形个数
	TRIANGLE* triangle;								// 指向三角数组的指针
} SECTOR;											// 命名为SECTOR

SECTOR sector1;										// 模型1

LRESULT	CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);		// WndProc的定义

//读取文件中的有效行
void readstr(FILE* f, char* string) {						//  读入一个字符串
	do {													// 循环开始
		fgets(string, 255, f);								// 读入一行
	} while ((string[0] == '/') || (string[0] == '\n'));	// 考察是否有必要进行处理
	return;													// 返回
}

// 设置我们的世界
void SetupWorld(){
	float x, y, z, u, v;									// 3D 和 纹理坐标
	int numtriangles;										// 区段的三角形数量
	FILE* filein;
	char oneline[255];										// 存储数据的字符串
	filein = fopen("data/world.txt", "rt");				

	readstr(filein, oneline);
	sscanf(oneline, "NUMPOLLIES %d\n", &numtriangles);

	sector1.triangle = new TRIANGLE[numtriangles];			// 为numtriangles个三角形分配内存并设定指针
	sector1.numtriangles = numtriangles;					// 定义区段1中的三角形数量
	for (int loop = 0; loop < numtriangles; loop++){		// 遍历所有的三角形
		for (int vert = 0; vert < 3; vert++){				// 遍历所有的顶点
			readstr(filein, oneline);						// 读入一行数据
			// 读入各自的顶点数据
			sscanf(oneline, "%f %f %f %f %f", &x, &y, &z, &u, &v);
			// 将顶点数据存入各自的顶点
			sector1.triangle[loop].vertex[vert].x = x;	// 区段 1,  第 loop 个三角形, 第  vert 个顶点, 值 x =x
			sector1.triangle[loop].vertex[vert].y = y;	// 区段 1,  第 loop 个三角形, 第  vert 个顶点, 值 y =y
			sector1.triangle[loop].vertex[vert].z = z;	// 区段 1,  第 loop 个三角形, 第  vert 个顶点, 值  z =z
			sector1.triangle[loop].vertex[vert].u = u;	// 区段 1,  第 loop 个三角形, 第  vert 个顶点, 值  u =u
			sector1.triangle[loop].vertex[vert].v = v;	// 区段 1,  第 loop 个三角形, 第  vert 个顶点, 值  e=v
		}
	}
	fclose(filein);
	return;
}

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
	if (TextureImage[0] = LoadBMP((char*)"Data/Mud.bmp")){
		Status = TRUE;										// 将 Status 设为TRUE
		glGenTextures(3, &texture[0]);						// 创建纹理

		// 使用来自位图数据生成的典型纹理

		// 创建 Nearest 滤波贴图
		glBindTexture(GL_TEXTURE_2D, texture[0]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, 3, TextureImage[0]->sizeX, TextureImage[0]->sizeY, 0, GL_RGB, GL_UNSIGNED_BYTE, TextureImage[0]->data);

		// 创建线性滤波纹理
		glBindTexture(GL_TEXTURE_2D, texture[1]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, 3, TextureImage[0]->sizeX, TextureImage[0]->sizeY, 0, GL_RGB, GL_UNSIGNED_BYTE, TextureImage[0]->data);

		// 创建 MipMapped 纹理
		glBindTexture(GL_TEXTURE_2D, texture[2]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
		gluBuild2DMipmaps(GL_TEXTURE_2D, 3, TextureImage[0]->sizeX, TextureImage[0]->sizeY, GL_RGB, GL_UNSIGNED_BYTE, TextureImage[0]->data);

	}
	if (TextureImage[0]) {									// 纹理是否存在
		if (TextureImage[0]->data) {						// 纹理图像是否存在
			free(TextureImage[0]->data);					// 释放纹理图像占用的内存
		}
		free(TextureImage[0]);								// 释放图像结构
	}
	return Status;											// 返回 Status
}

int InitGL(GLvoid) {									// 此处开始对OpenGL进行所有设置
	if (!LoadGLTextures()) {							// 调用纹理载入子例程
		return FALSE;									// 如果未能载入，返回FALSE
	}

	glEnable(GL_TEXTURE_2D);							// 启用纹理映射
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);					// 设置混色函数取得半透明效果
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);				// 黑色背景
	glClearDepth(1.0f);									// 设置深度缓存
	glDepthFunc(GL_LESS);								// 要进行的深度测试类型
	glEnable(GL_DEPTH_TEST);							// 启用深度测试
	glShadeModel(GL_SMOOTH);							// 启用阴影平滑
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);	// 真正精细的透视修正

	SetupWorld();
	
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

int DrawGLScene(GLvoid) {									// 绘制 OpenGL 场景
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);		// 清除 场景 和 深度缓冲
	glLoadIdentity();										// 重置当前矩阵

	GLfloat x_m, y_m, z_m, u_m, v_m;						// 顶点的临时 X, Y, Z, U 和 V 的数值
	GLfloat xtrans = -xpos;									// 用于游戏者沿X轴平移时的大小
	GLfloat ztrans = -zpos;									// 用于游戏者沿Z轴平移时的大小
	GLfloat ytrans = -walkbias - 0.25f;						// 用于头部的上下摆动
	GLfloat sceneroty = 360.0f - yrot;						// 位于游戏者方向的360度角

	int numtriangles;										// 保有三角形数量的整数

	glRotatef(lookupdown, 1.0f, 0, 0);						// 上下旋转
	glRotatef(sceneroty, 0, 1.0f, 0);						// 根据游戏者正面所对方向所作的旋转

	glTranslatef(xtrans, ytrans, ztrans);					// 以游戏者为中心的平移场景
	glBindTexture(GL_TEXTURE_2D, texture[filter]);			// 根据 filter 选择的纹理

	numtriangles = sector1.numtriangles;					// 取得Sector1的三角形数量

	// 逐个处理三角形
	for (int loop_m = 0; loop_m < numtriangles; loop_m++) {	// 遍历所有的三角形
		glBegin(GL_TRIANGLES);								// 开始绘制三角形
		glNormal3f(0.0f, 0.0f, 1.0f);						// 指向前面的法线
		x_m = sector1.triangle[loop_m].vertex[0].x;			// 第一点的 X 分量
		y_m = sector1.triangle[loop_m].vertex[0].y;			// 第一点的 Y 分量
		z_m = sector1.triangle[loop_m].vertex[0].z;			// 第一点的 Z 分量
		u_m = sector1.triangle[loop_m].vertex[0].u;			// 第一点的 U  纹理坐标
		v_m = sector1.triangle[loop_m].vertex[0].v;			// 第一点的 V  纹理坐标
		glTexCoord2f(u_m, v_m); glVertex3f(x_m, y_m, z_m);	// 设置纹理坐标和顶点

		x_m = sector1.triangle[loop_m].vertex[1].x;			// 第二点的 X 分量
		y_m = sector1.triangle[loop_m].vertex[1].y;			// 第二点的 Y 分量
		z_m = sector1.triangle[loop_m].vertex[1].z;			// 第二点的 Z 分量
		u_m = sector1.triangle[loop_m].vertex[1].u;			// 第二点的 U  纹理坐标
		v_m = sector1.triangle[loop_m].vertex[1].v;			// 第二点的 V  纹理坐标
		glTexCoord2f(u_m, v_m); glVertex3f(x_m, y_m, z_m);	// 设置纹理坐标和顶点

		x_m = sector1.triangle[loop_m].vertex[2].x;			// 第三点的 X 分量
		y_m = sector1.triangle[loop_m].vertex[2].y;			// 第三点的 Y 分量
		z_m = sector1.triangle[loop_m].vertex[2].z;			// 第三点的 Z 分量
		u_m = sector1.triangle[loop_m].vertex[2].u;			// 第二点的 U  纹理坐标
		v_m = sector1.triangle[loop_m].vertex[2].v;			// 第二点的 V  纹理坐标
		glTexCoord2f(u_m, v_m); glVertex3f(x_m, y_m, z_m);	// 设置纹理坐标和顶点
		glEnd();											// 三角形绘制结束
	}
	return TRUE;											// 返回
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

					if (keys['B'] && !bp){							// B 键已按下并且松开了?
						bp = TRUE;									// bp 设为 TRUE
						blend = !blend;								// 切换混合模式
						if (!blend){								// 如果没有混合
							glDisable(GL_BLEND);					// 终止混色
							glEnable(GL_DEPTH_TEST);				// 启动深度缓存
						}
						else{
							glEnable(GL_BLEND);						// 启动混色
							glDisable(GL_DEPTH_TEST);				// 关闭深度缓存
						}
					}
					if (!keys['B']){								//B键松开了么?
						bp = FALSE;									//若是，则将bp设为FALSE
					}

					if (keys['F'] && !fp) {							// F键按下了么?
						fp = TRUE;									// fp 设为 TRUE
						filter += 1;								// filter的值加一
						if (filter > 2) {							// 大于2了么?
							filter = 0;								// 若是重置为0
						}
					}
					if (!keys['F']) {								// F键放开了么?
						fp = FALSE;									// 若是fp设为FALSE
					}

					if (keys['I']) {								// I按下了么
						z -= 0.02f;									// 若按下，将木箱移向屏幕内部
						lookupdown -= 0.1f;
					}
					if (keys['O']) {								// O按下了么
						z += 0.02f;									// 若按下的话，将木箱移向观察者
						lookupdown += 0.1f;
					}

					if (keys[VK_UP]) {								// 向上方向键按下了么?
						xpos -= (float)sin((double)heading * piover180) * 0.05f;		// 沿游戏者所在的X平面移动
						zpos -= (float)cos((double)heading * piover180) * 0.05f;		// 沿游戏者所在的Z平面移动
						if (walkbiasangle >= 359.0f) {									// 如果walkbiasangle大于359度
							walkbiasangle = 0.0f;										// 将 walkbiasangle 设为0
						}
						else {															// 否则
							walkbiasangle += 10;										// 如果 walkbiasangle < 359 ，则增加 10
						}
						walkbias = (float)sin((double)walkbiasangle * piover180) / 20.0f;		// 使游戏者产生跳跃感
					}

					if (keys[VK_DOWN]) {							// 向下方向键按下了么？
						xpos += (float)sin((double)heading * piover180) * 0.05f;		// 沿游戏者所在的X平面移动
						zpos += (float)cos((double)heading * piover180) * 0.05f;		// 沿游戏者所在的Z平面移动
						if (walkbiasangle <= 1.0f) {									// 如果walkbiasangle小于1度
							walkbiasangle = 359.0f;										// 使 walkbiasangle 等于 359
						}
						else {															// 否则
							walkbiasangle -= 10;										// 如果 walkbiasangle > 1 减去 10
						}
						walkbias = (float)sin((double)walkbiasangle * piover180) / 20.0f;		// 使游戏者产生跳跃感
					}

					if (keys[VK_RIGHT]){				// 右方向键按下了么?
						heading -= 1.0f;	
						yrot = heading;					// 向左旋转场景
					}

					if (keys[VK_LEFT]){					// 左方向键按下了么?
						heading += 1.0f;
						yrot = heading;					// 向右侧旋转场景
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
