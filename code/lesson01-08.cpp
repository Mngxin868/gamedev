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

BOOL	light;										// 光源的开/关
BOOL	lp;											// L键按下了么?
BOOL	fp;											// F键按下了么?


GLfloat		rtri;									// 用于三角形的角度
GLfloat		rquad;									// 用于四边形的角度

GLfloat		xrot;									// X 旋转量
GLfloat		yrot;									// Y 旋转量
GLfloat		zrot;									// Z 旋转量
GLfloat xspeed;										// X 旋转速度
GLfloat yspeed;										// Y 旋转速度
GLfloat zspeed;										// Z 旋转速度

GLfloat	z = -5.0f;									// 深入屏幕的距离

//设置创建光源的数组
GLfloat LightAmbient[] = { 0.5f, 0.5f, 0.5f, 1.0f }; 				// 环境光参数
GLfloat LightDiffuse[] = { 1.0f, 1.0f, 1.0f, 1.0f };				// 漫射光参数
GLfloat LightPosition[] = { 0.0f, 0.0f, 2.0f, 1.0f };				// 光源位置

GLuint	filter;										// 滤波类型
GLuint	texture[3];									// 3种纹理的储存空间

bool    blend;										// 是否混合?
bool	bp;											// B 键按下了么?


LRESULT	CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);			// WndProc的定义

//载入图片文件
AUX_RGBImageRec* LoadBMP(char* Filename) {					// 载入位图图象
	FILE* File = NULL;										// 文件句柄
	if (!Filename) {										// 确保文件名已提供
		return NULL;										// 如果没提供，返回 NULL
	}
	File = fopen(Filename, "r");							// 尝试打开文件
	if (File) {												// 文件存在么?
		fclose(File);										// 关闭句柄
		return auxDIBImageLoad(Filename);					// 载入位图并返回指针
	}
	return NULL;											// 如果载入失败，返回 NULL
}

//载入位图
int LoadGLTextures() {										// 载入位图(调用上面的代码)并转换成纹理
	int Status = FALSE;										// 状态指示器
	AUX_RGBImageRec* TextureImage[1];						// 创建纹理的存储空间
	memset(TextureImage, 0, sizeof(void*) * 1);				// 将指针设为 NULL
	// 载入位图，检查有无错误，如果位图没找到则退出
	if (TextureImage[0] = LoadBMP((char*)"Data/data1.bmp")) {
		Status = TRUE;										// 将 Status 设为 TRUE
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

//重新设置OpenGL场景的大小
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

//对OpenGL进行所有的设置
int InitGL(GLvoid)	{											// 此处开始对OpenGL进行所有设置
	if (!LoadGLTextures()) {									// 调用纹理载入子例程
		return FALSE;											// 如果未能载入，返回FALSE
	}
	glEnable(GL_TEXTURE_2D);									// 启用纹理映射

	glShadeModel(GL_SMOOTH);									// 启用阴影平滑
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);						// 获取颜色，黑色背景
	
	glClearDepth(1.0f);											// 设置深度缓存
	glEnable(GL_DEPTH_TEST);									// 启用深度测试
	glDepthFunc(GL_LEQUAL);										// 所作深度测试的类型

	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);			// 告诉系统对透视进行修正
	
	glLightfv(GL_LIGHT1, GL_AMBIENT, LightAmbient);				// 设置环境光
	glLightfv(GL_LIGHT1, GL_DIFFUSE, LightDiffuse);				// 设置漫射光
	glLightfv(GL_LIGHT1, GL_POSITION, LightPosition);			// 设置光源位置
	glEnable(GL_LIGHT1);										// 启用一号光源

	glColor4f(1.0f, 1.0f, 1.0f, 0.5f);							// 全亮度， 50% Alpha 混合
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);							// 基于源象素alpha通道值的半透明混合函数

	return TRUE;												// 初始化 OK
	//如果有错误发生的话，您可以加上您自己的代码返回FALSE。
}

//绘图代码(旧)
int OldDrawGLScene(GLvoid) {							
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);			// 清除屏幕和深度缓存
	glLoadIdentity();											// 重置当前的模型观察矩阵
	
	glTranslatef(-1.5f, 0.0f, -6.0f);							// 左移 1.5 单位，并移入屏幕 6.0

	glRotatef(rtri, 0.0f, 1.0f, 0.0f);							// 绕Y轴旋转三角形

	glBegin(GL_TRIANGLES);										// 绘制三角形、金字塔
	//前侧面
		glColor3f(1.0f, 0.0f, 0.0f);							// 设置当前色为红色
	glVertex3f(0.0f, 1.0f, 0.0f);								// 上顶点
		glColor3f(0.0f, 1.0f, 0.0f);							// 设置当前色为绿色
	glVertex3f(-1.0f, -1.0f, 1.0f);								// 左下
		glColor3f(0.0f, 0.0f, 1.0f);							// 设置当前色为蓝色
	glVertex3f(1.0f, -1.0f, 1.0f);								// 右下
	//右侧面
		glColor3f(1.0f, 0.0f, 0.0f);							//红色
	glVertex3f(0.0f, 1.0f, 0.0f);								// 三角形的上顶点 (右侧面)
		glColor3f(0.0f, 0.0f, 1.0f);							// 蓝色
	glVertex3f(1.0f, -1.0f, 1.0f);								// 三角形的左下顶点 (右侧面)
		glColor3f(0.0f, 1.0f, 0.0f);							// 绿色
	glVertex3f(1.0f, -1.0f, -1.0f);								// 三角形的右下顶点 (右侧面)
	//后侧面
		glColor3f(1.0f, 0.0f, 0.0f);							// 红色
	glVertex3f(0.0f, 1.0f, 0.0f);								// 三角形的上顶点 (后侧面)
		glColor3f(0.0f, 1.0f, 0.0f);							// 绿色
	glVertex3f(1.0f, -1.0f, -1.0f);								// 三角形的左下顶点 (后侧面)
		glColor3f(0.0f, 0.0f, 1.0f);							// 蓝色
	glVertex3f(-1.0f, -1.0f, -1.0f);							// 三角形的右下顶点 (后侧面)
	//左侧面
		glColor3f(1.0f, 0.0f, 0.0f);							// 红色
	glVertex3f(0.0f, 1.0f, 0.0f);								// 三角形的上顶点 (左侧面)
		glColor3f(0.0f, 0.0f, 1.0f);							// 蓝色
	glVertex3f(-1.0f, -1.0f, -1.0f);							// 三角形的左下顶点 (左侧面)
		glColor3f(0.0f, 1.0f, 0.0f);							// 绿色
	glVertex3f(-1.0f, -1.0f, 1.0f);								// 三角形的右下顶点 (左侧面)
	

	glEnd();													// 三角形绘制结束

	//glTranslatef(3.0f, 0.0f, 0.0f);							// 相对上一次，右移3单位
	glLoadIdentity();											// 重置当前的模型观察矩阵
	glTranslatef(1.5f, 0.0f, -7.0f);							// 先右移再移入屏幕
	glRotatef(rquad, 1.0f, 1.0f, 1.0f);							// 在XYZ轴上旋转立方体
	glBegin(GL_QUADS);											// 开始绘制立方体
	
		glColor3f(0.0f, 1.0f, 0.0f);							// 颜色改为蓝色
	glVertex3f(1.0f, 1.0f, -1.0f);								// 四边形的右上顶点 (顶面)
	glVertex3f(-1.0f, 1.0f, -1.0f);								// 四边形的左上顶点 (顶面)
	glVertex3f(-1.0f, 1.0f, 1.0f);								// 四边形的左下顶点 (顶面)
	glVertex3f(1.0f, 1.0f, 1.0f);								// 四边形的右下顶点 (顶面)
	
		glColor3f(1.0f, 0.5f, 0.0f);							// 颜色改成橙色
	glVertex3f(1.0f, -1.0f, 1.0f);								// 四边形的右上顶点(底面)
	glVertex3f(-1.0f, -1.0f, 1.0f);								// 四边形的左上顶点(底面)
	glVertex3f(-1.0f, -1.0f, -1.0f);							// 四边形的左下顶点(底面)
	glVertex3f(1.0f, -1.0f, -1.0f);								// 四边形的右下顶点(底面)
	
		glColor3f(1.0f, 0.0f, 0.0f);							// 颜色改成红色
	glVertex3f(1.0f, 1.0f, 1.0f);								// 四边形的右上顶点(前面)
	glVertex3f(-1.0f, 1.0f, 1.0f);								// 四边形的左上顶点(前面)
	glVertex3f(-1.0f, -1.0f, 1.0f);								// 四边形的左下顶点(前面)
	glVertex3f(1.0f, -1.0f, 1.0f);								// 四边形的右下顶点(前面)

		glColor3f(0.0f, 0.0f, 1.0f);							// 颜色改成蓝色
	glVertex3f(-1.0f, 1.0f, 1.0f);								// 四边形的右上顶点(左面)
	glVertex3f(-1.0f, 1.0f, -1.0f);								// 四边形的左上顶点(左面)
	glVertex3f(-1.0f, -1.0f, -1.0f);							// 四边形的左下顶点(左面)
	glVertex3f(-1.0f, -1.0f, 1.0f);								// 四边形的右下顶点(左面)

		glColor3f(1.0f, 0.0f, 1.0f);							// 颜色改成紫罗兰色
	glVertex3f(1.0f, 1.0f, -1.0f);								// 四边形的右上顶点(右面)
	glVertex3f(1.0f, 1.0f, 1.0f);								// 四边形的左上顶点(右面)
	glVertex3f(1.0f, -1.0f, 1.0f);								// 四边形的左下顶点(右面)
	glVertex3f(1.0f, -1.0f, -1.0f);								// 四边形的右下顶点(右面)

	glEnd();													// 正方形绘制结束

	rtri += 0.2f;												// 增加三角形的旋转变量
	rquad -= 0.15f;												// 减少四边形的旋转变量

	return TRUE;												//  一切 OK
}

//绘图代码
int DrawGLScene(GLvoid) {										// 从这里开始进行所有的绘制
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);			// 清除屏幕和深度缓存
	glLoadIdentity();											// 重置当前的模型观察矩阵
	glTranslatef(0.0f, 0.0f, z);								// 移入屏幕 5 个单位

	glRotatef(xrot, 1.0f, 0.0f, 0.0f);							// 绕X轴旋转
	glRotatef(yrot, 0.0f, 1.0f, 0.0f);							// 绕Y轴旋转
	glRotatef(zrot, 0.0f, 0.0f, 1.0f);							// 绕Z轴旋转

	//glBindTexture(GL_TEXTURE_2D, texture[0]);					// 选择纹理
	glBindTexture(GL_TEXTURE_2D, texture[filter]);				// 选择由filter决定的纹理

	glBegin(GL_QUADS);
	// 前面
	glNormal3f(0.0f, 0.0f, 1.0f);					// 法线指向观察者
	glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.0f, -1.0f, 1.0f);	// 纹理和四边形的左下
	glTexCoord2f(1.0f, 0.0f); glVertex3f(1.0f, -1.0f, 1.0f);	// 纹理和四边形的右下
	glTexCoord2f(1.0f, 1.0f); glVertex3f(1.0f, 1.0f, 1.0f);		// 纹理和四边形的右上
	glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.0f, 1.0f, 1.0f);	// 纹理和四边形的左上
	// 后面
	glNormal3f(0.0f, 0.0f, -1.0f);					// 法线背向观察者
	glTexCoord2f(1.0f, 0.0f); glVertex3f(-1.0f, -1.0f, -1.0f);	// 纹理和四边形的右下
	glTexCoord2f(1.0f, 1.0f); glVertex3f(-1.0f, 1.0f, -1.0f);	// 纹理和四边形的右上
	glTexCoord2f(0.0f, 1.0f); glVertex3f(1.0f, 1.0f, -1.0f);	// 纹理和四边形的左上
	glTexCoord2f(0.0f, 0.0f); glVertex3f(1.0f, -1.0f, -1.0f);	// 纹理和四边形的左下
	// 顶面
	glNormal3f(0.0f, 1.0f, 0.0f);					// 法线向上
	glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.0f, 1.0f, -1.0f);	// 纹理和四边形的左上
	glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.0f, 1.0f, 1.0f);	// 纹理和四边形的左下
	glTexCoord2f(1.0f, 0.0f); glVertex3f(1.0f, 1.0f, 1.0f);		// 纹理和四边形的右下
	glTexCoord2f(1.0f, 1.0f); glVertex3f(1.0f, 1.0f, -1.0f);	// 纹理和四边形的右上
	// 底面
	glNormal3f(0.0f, -1.0f, 0.0f);					// 法线朝下
	glTexCoord2f(1.0f, 1.0f); glVertex3f(-1.0f, -1.0f, -1.0f);	// 纹理和四边形的右上
	glTexCoord2f(0.0f, 1.0f); glVertex3f(1.0f, -1.0f, -1.0f);	// 纹理和四边形的左上
	glTexCoord2f(0.0f, 0.0f); glVertex3f(1.0f, -1.0f, 1.0f);	// 纹理和四边形的左下
	glTexCoord2f(1.0f, 0.0f); glVertex3f(-1.0f, -1.0f, 1.0f);	// 纹理和四边形的右下
	// 右面
	glNormal3f(1.0f, 0.0f, 0.0f);					// 法线朝右
	glTexCoord2f(1.0f, 0.0f); glVertex3f(1.0f, -1.0f, -1.0f);	// 纹理和四边形的右下
	glTexCoord2f(1.0f, 1.0f); glVertex3f(1.0f, 1.0f, -1.0f);	// 纹理和四边形的右上
	glTexCoord2f(0.0f, 1.0f); glVertex3f(1.0f, 1.0f, 1.0f);		// 纹理和四边形的左上
	glTexCoord2f(0.0f, 0.0f); glVertex3f(1.0f, -1.0f, 1.0f);	// 纹理和四边形的左下
	// 左面
	glNormal3f(-1.0f, 0.0f, 0.0f);					// 法线朝左
	glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.0f, -1.0f, -1.0f);	// 纹理和四边形的左下
	glTexCoord2f(1.0f, 0.0f); glVertex3f(-1.0f, -1.0f, 1.0f);	// 纹理和四边形的右下
	glTexCoord2f(1.0f, 1.0f); glVertex3f(-1.0f, 1.0f, 1.0f);	// 纹理和四边形的右上
	glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.0f, 1.0f, -1.0f);	// 纹理和四边形的左上
	glEnd();

	xrot = xspeed;								// X 轴旋转
	yrot = yspeed;								// Y 轴旋转
	zrot = zspeed;								// Z 轴旋转
	return true;								// 继续运行

}

// 正常销毁窗口，并释放资源
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

//窗口的创建
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

	if (fullscreen)	{												// 要尝试全屏模式吗?
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

//处理所有的窗口消息
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

//Windows程序的入口，将会调用窗口创建例程，处理窗口消息，类似于main函数
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
					
					if (keys['L'] && !lp) {							// L 键已按下并且松开了?
						lp = TRUE;									// lp 设为 TRUE
						light = !light;								// 切换光源的 TRUE/FALSE
						if (!light) {								// 如果没有光源
							glDisable(GL_LIGHTING);					// 禁用光源
						}
						else {										// 否则
							glEnable(GL_LIGHTING);					// 启用光源
						}
					}
					if (!keys['L']) {								// L键松开了么?
						lp = FALSE;									// 若是，则将lp设为FALSE
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
					}
					if (keys['O']) {								// O按下了么
						z += 0.02f;									// 若按下的话，将木箱移向观察者
					}

					if (keys[VK_UP]) {								// Up方向键按下了么?
						xspeed -= 0.01f;							// 若是,减少xspeed
					}
					if (keys[VK_DOWN]) {							// Down方向键按下了么?
						xspeed += 0.01f;							// 若是,增加xspeed
					}
					if (keys[VK_RIGHT]) {							// Right方向键按下了么?
						yspeed += 0.01f;							// 若是,增加yspeed
					}
					if (keys[VK_LEFT]) {							// Left方向键按下了么?
						yspeed -= 0.01f;							// 若是, 减少yspeed
					}

					if (keys['B'] && !bp) {							// B 健按下且bp为 FALSE么?
						bp = TRUE;									// 若是， bp 设为 TRUE
						blend = !blend;								// 切换混合选项的 TRUE / FALSE
						if (blend) {								// 混合打开了么?
							glEnable(GL_BLEND);						// 打开混合
							glDisable(GL_DEPTH_TEST);				// 关闭深度测试
						}
						else {										// 否则
							glDisable(GL_BLEND);					// 关闭混合
							glEnable(GL_DEPTH_TEST);				// 打开深度测试
						}
					}
					if (!keys['B']) {								//  B 键松开了么?
						bp = FALSE;									// 若是， bp设为 FALSE
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

