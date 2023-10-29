#include <windows.h>
#include <math.h>
#include "Mesh.h"

#define ScreenWidth 640
#define ScreenHeight 480
#define FieldOfView 60.0f
#define NearClip 0.01f
#define FarClip 1000.0f

typedef struct {float x; float y; float z;} float3;
typedef struct {float2 min; float2 max;} AABB;

float RotationX = 0.0f, RotationY = 0.0f;
float CameraRotYX[4][4], CameraRotYXZ[4][4]; 
float CameraTR[4][4], CameraMatrix[4][4], ViewMatrix[4][4];
float ProjectionViewMatrix[4][4], MVP[4][4];

float Clamp(float x, float a, float b)
{
	return fmaxf(a, fminf(b, x));
}

float Deg2rad(float x) 
{
	return (x * 3.14159265358979323846f / 180.0f);
}

AABB BoundingBox (int ax, int ay, int bx, int by, int cx, int cy)
{
	float2 tmin = {fminf(fminf(ax, bx), cx), fminf(fminf(ay, by), cy)};
	float2 tmax = {fmaxf(fmaxf(ax, bx), cx), fmaxf(fmaxf(ay, by), cy)};
	AABB box = {tmin, tmax};
	return box;
}

float3 Barycentric(float2 p0, float2 p1, float2 p2, float2 p3)
{
	float2 a = {p2.x - p1.x, p2.y - p1.y};
	float2 b = {p3.x - p1.x, p3.y - p1.y};
	float2 c = {p0.x - p1.x, p0.y - p1.y};
	float d = a.x * b.y - b.x * a.y;
	float v = (c.x * b.y - b.x * c.y) / d;
	float w = (a.x * c.y - c.x * a.y) / d;
	float u = 1.0f - v - w;
	float3 p = {u,v,w};
	return p;
}

void Mul(float mat1[][4], float mat2[][4], float res[][4])
{
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			res[i][j] = 0;
			for (int k = 0; k < 4; k++) 
			{
				res[i][j] += mat1[i][k]*mat2[k][j];
			}
		}
	}
}

float ModelMatrix[4][4] = 
{
	1.0f,0.0f,0.0f,0.0f,
	0.0f,1.0f,0.0f,0.0f,
	0.0f,0.0f,1.0f,0.0f,
	0.0f,0.0f,0.0f,1.0f
};

float CameraTranslationMatrix[4][4] = 
{
	1.0f,0.0f,0.0f,0.0f,
	0.0f,1.0f,0.0f,1.5f,
	0.0f,0.0f,1.0f,-20.0f,
	0.0f,0.0f,0.0f,1.0f
};

float CameraRotationYMatrix[4][4] = 
{
	1.0f,0.0f,0.0f,0.0f,
	0.0f,1.0f,0.0f,0.0f,
	0.0f,0.0f,1.0f,0.0f,
	0.0f,0.0f,0.0f,1.0f
};

float CameraRotationXMatrix[4][4] = 
{
	1.0f,0.0f,0.0f,0.0f,
	0.0f,1.0f,0.0f,0.0f,
	0.0f,0.0f,1.0f,0.0f,
	0.0f,0.0f,0.0f,1.0f
};

float CameraRotationZMatrix[4][4] = 
{
	1.0f,0.0f,0.0f,0.0f,
	0.0f,1.0f,0.0f,0.0f,
	0.0f,0.0f,1.0f,0.0f,
	0.0f,0.0f,0.0f,1.0f
};

float CameraScaleMatrix[4][4] = 
{
	1.0f,0.0f,0.0f,0.0f,
	0.0f,1.0f,0.0f,0.0f,
	0.0f,0.0f,-1.0f,0.0f,
	0.0f,0.0f,0.0f,1.0f
};

float ProjectionMatrix[4][4] = 
{
	0.0f,0.0f,0.0f,0.0f,
	0.0f,0.0f,0.0f,0.0f,
	0.0f,0.0f,0.0f,0.0f,
	0.0f,0.0f,-1.0f,0.0f
};

void Inverse( float param[4][4], float k[4][4])
{
	float invOut[16];
	float m[16] = 
	{
		param[0][0],param[0][1],param[0][2],param[0][3],
		param[1][0],param[1][1],param[1][2],param[1][3],
		param[2][0],param[2][1],param[2][2],param[2][3],
		param[3][0],param[3][1],param[3][2],param[3][3]
	};
	float inv[16], det;
	inv[0]  =  m[5]*m[10]*m[15]-m[5]*m[11]*m[14]-m[9]*m[6]*m[15]+m[9]*m[7]*m[14]+m[13]*m[6]*m[11]-m[13]*m[7]*m[10];
	inv[4]  = -m[4]*m[10]*m[15]+m[4]*m[11]*m[14]+m[8]*m[6]*m[15]-m[8]*m[7]*m[14]-m[12]*m[6]*m[11]+m[12]*m[7]*m[10];
	inv[8]  =  m[4] *m[9]*m[15]-m[4]*m[11]*m[13]-m[8]*m[5]*m[15]+m[8]*m[7]*m[13]+m[12]*m[5]*m[11]-m[12]*m[7] *m[9];
	inv[12] = -m[4] *m[9]*m[14]+m[4]*m[10]*m[13]+m[8]*m[5]*m[14]-m[8]*m[6]*m[13]-m[12]*m[5]*m[10]+m[12]*m[6] *m[9];
	inv[1]  = -m[1]*m[10]*m[15]+m[1]*m[11]*m[14]+m[9]*m[2]*m[15]-m[9]*m[3]*m[14]-m[13]*m[2]*m[11]+m[13]*m[3]*m[10];
	inv[5]  =  m[0]*m[10]*m[15]-m[0]*m[11]*m[14]-m[8]*m[2]*m[15]+m[8]*m[3]*m[14]+m[12]*m[2]*m[11]-m[12]*m[3]*m[10];
	inv[9]  = -m[0] *m[9]*m[15]+m[0]*m[11]*m[13]+m[8]*m[1]*m[15]-m[8]*m[3]*m[13]-m[12]*m[1]*m[11]+m[12]*m[3] *m[9];
	inv[13] =  m[0] *m[9]*m[14]-m[0]*m[10]*m[13]-m[8]*m[1]*m[14]+m[8]*m[2]*m[13]+m[12]*m[1]*m[10]-m[12]*m[2] *m[9];
	inv[2]  =  m[1] *m[6]*m[15]-m[1] *m[7]*m[14]-m[5]*m[2]*m[15]+m[5]*m[3]*m[14]+m[13]*m[2] *m[7]-m[13]*m[3] *m[6];
	inv[6]  = -m[0] *m[6]*m[15]+m[0] *m[7]*m[14]+m[4]*m[2]*m[15]-m[4]*m[3]*m[14]-m[12]*m[2] *m[7]+m[12]*m[3] *m[6];
	inv[10] =  m[0] *m[5]*m[15]-m[0] *m[7]*m[13]-m[4]*m[1]*m[15]+m[4]*m[3]*m[13]+m[12]*m[1] *m[7]-m[12]*m[3] *m[5];
	inv[14] = -m[0] *m[5]*m[14]+m[0] *m[6]*m[13]+m[4]*m[1]*m[14]-m[4]*m[2]*m[13]-m[12]*m[1] *m[6]+m[12]*m[2] *m[5];
	inv[3]  = -m[1] *m[6]*m[11]+m[1] *m[7]*m[10]+m[5]*m[2]*m[11]-m[5]*m[3]*m[10] -m[9]*m[2] *m[7] +m[9]*m[3] *m[6];
	inv[7]  =  m[0] *m[6]*m[11]-m[0] *m[7]*m[10]-m[4]*m[2]*m[11]+m[4]*m[3]*m[10] +m[8]*m[2] *m[7] -m[8]*m[3] *m[6];
	inv[11] = -m[0] *m[5]*m[11]+m[0] *m[7]*m[9] +m[4]*m[1]*m[11]-m[4]*m[3] *m[9] -m[8]*m[1] *m[7] +m[8]*m[3] *m[5];
	inv[15] =  m[0] *m[5]*m[10]-m[0] *m[6]*m[9] -m[4]*m[1]*m[10]+m[4]*m[2] *m[9] +m[8]*m[1] *m[6] -m[8]*m[2] *m[5];
	det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];
	det = 1.0 / det;
	for (int i = 0; i < 16; i++) invOut[i] = inv[i] * det;	
	k[0][0] = invOut[0];  k[0][1] = invOut[1];  k[0][2] = invOut[2];  k[0][3] = invOut[3];
	k[1][0] = invOut[4];  k[1][1] = invOut[5];  k[1][2] = invOut[6];  k[1][3] = invOut[7];
	k[2][0] = invOut[8];  k[2][1] = invOut[9];  k[2][2] = invOut[10]; k[2][3] = invOut[11];
	k[3][0] = invOut[12]; k[3][1] = invOut[13]; k[3][2] = invOut[14]; k[3][3] = invOut[15];  
}

float4 ObjectToClipPos( float m[4][4], float4 v)
{
	float x = m[0][0] * v.x + m[0][1] * v.y + m[0][2] * v.z + m[0][3] * v.w;
	float y = m[1][0] * v.x + m[1][1] * v.y + m[1][2] * v.z + m[1][3] * v.w;
	float z = m[2][0] * v.x + m[2][1] * v.y + m[2][2] * v.z + m[2][3] * v.w;
	float w = m[3][0] * v.x + m[3][1] * v.y + m[3][2] * v.z + m[3][3] * v.w;
	float4 vertex = {x,y,z,w};
	return vertex;
}

void KeyboardMovement()
{
	float forward[3] = {ViewMatrix[2][0],ViewMatrix[2][1],ViewMatrix[2][2]};
	float strafe[3] = {ViewMatrix[0][0],ViewMatrix[1][0],ViewMatrix[2][0]};
	float dz = 0.0f;
	float dx = 0.0f;
	if (GetAsyncKeyState(0x57)) dz =  1.0f;
	if (GetAsyncKeyState(0x53)) dz = -1.0f ;
	if (GetAsyncKeyState(0x44)) dx =  1.0f;
	if (GetAsyncKeyState(0x41)) dx = -1.0f ;
	if (GetAsyncKeyState(0x45)) CameraTranslationMatrix[1][3] += 0.01f ;
	if (GetAsyncKeyState(0x51)) CameraTranslationMatrix[1][3] -= 0.01f ; 
	float eyeVector[3] = {CameraTranslationMatrix[0][3],CameraTranslationMatrix[1][3] ,CameraTranslationMatrix[2][3]};
	eyeVector[0] += (-dz * forward[0] + dx * strafe[0]) * 0.1f;
	eyeVector[1] += (-dz * forward[1] + dx * strafe[1]) * 0.1f;
	eyeVector[2] += (-dz * forward[2] + dx * strafe[2]) * 0.1f;
	CameraTranslationMatrix[0][3] = eyeVector[0];
	//CameraTranslationMatrix[1][3] = eyeVector[1];
	CameraTranslationMatrix[2][3] = eyeVector[2];
}

void MouseLook(float w, float h)
{	
	POINT point;
	int mx = (int)w >> 1;
	int my = (int)h >> 1;
	GetCursorPos(&point);
	if( (point.x == mx) && (point.y == my) ) return;
	SetCursorPos(mx, my);
	float deltaZ = (float)((mx - point.x)) ;
	float deltaX = (float)((my - point.y)) ;
	if (deltaX>0.0f) RotationX-=2.0f; 
	if (deltaX<0.0f) RotationX+=2.0f; 
	if (deltaZ>0.0f) RotationY-=2.0f; 
	if (deltaZ<0.0f) RotationY+=2.0f; 
	CameraRotationXMatrix[1][1] = cos(Deg2rad(RotationX));
	CameraRotationXMatrix[1][2] = (-1.0f)*sin(Deg2rad(RotationX));
	CameraRotationXMatrix[2][1] = sin(Deg2rad(RotationX));
	CameraRotationXMatrix[2][2] = cos(Deg2rad(RotationX));
	CameraRotationYMatrix[0][0] = cos(Deg2rad(RotationY));
	CameraRotationYMatrix[0][2] = sin(Deg2rad(RotationY));
	CameraRotationYMatrix[2][0] = (-1.0f)*sin(Deg2rad(RotationY));
	CameraRotationYMatrix[2][2] = cos(Deg2rad(RotationY));
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg==WM_CLOSE || uMsg==WM_DESTROY || (uMsg==WM_KEYDOWN && wParam==VK_ESCAPE))
	{
		PostQuitMessage(0); return 0;
	}
	else
	{
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	ShowCursor(0);
	int exit = 0;
	MSG msg;
	WNDCLASS win = {CS_OWNDC|CS_HREDRAW|CS_VREDRAW, WindowProc, 0, 0, 0, 0, 0, (HBRUSH)(COLOR_WINDOW+1), 0, "Demo"};
	RegisterClass(&win);
	HWND hwnd = CreateWindowEx(0, win.lpszClassName, "Demo", WS_VISIBLE|WS_POPUP, 0, 0, ScreenWidth, ScreenHeight, 0, 0, 0, 0);
	HDC hdc = GetDC(hwnd);
	BITMAPINFO bmi = {{sizeof(BITMAPINFOHEADER),ScreenWidth,ScreenHeight,1,32,BI_RGB,0,0,0,0,0},{0,0,0,0}};
	static int colormap[ScreenWidth*ScreenHeight];
	static float depthmap[ScreenWidth*ScreenHeight];
	int triangles = sizeof(vertices) / sizeof(vertices[0]) / 3;
	int texRes = (int) sqrt(sizeof(texture) / sizeof(texture[0]));
	ProjectionMatrix[0][0] = ((1.0f/tan(Deg2rad(FieldOfView/2.0f)))/((float)ScreenWidth/(float)ScreenHeight));
	ProjectionMatrix[1][1] = (1.0f/tan(Deg2rad(FieldOfView/2.0f)));
	ProjectionMatrix[2][2] = (-1.0f)* (FarClip+NearClip)/(FarClip-NearClip);
	ProjectionMatrix[2][3] = (-1.0f)*(2.0f*FarClip*NearClip)/(FarClip-NearClip);
	while (!exit)
	{
		while(PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			if( msg.message==WM_QUIT ) exit = 1;
			TranslateMessage( &msg );
			DispatchMessage( &msg );
		}
		for (int x = 0; x < ScreenWidth; x++)
		{
			for (int y = 0; y < ScreenHeight; y++)
			{
				colormap[y*ScreenWidth+x] = 0;
				depthmap[y*ScreenWidth+x] = FarClip;
			}
		}
		MouseLook(ScreenWidth,ScreenHeight);
		KeyboardMovement();
		Mul(CameraRotationYMatrix, CameraRotationXMatrix, CameraRotYX);
		Mul(CameraRotYX, CameraRotationZMatrix, CameraRotYXZ);
		Mul(CameraTranslationMatrix, CameraRotYXZ, CameraTR);
		Mul(CameraTR, CameraScaleMatrix, CameraMatrix);
		Inverse(CameraMatrix, ViewMatrix);
		Mul(ProjectionMatrix, ViewMatrix, ProjectionViewMatrix);
		Mul(ProjectionViewMatrix, ModelMatrix, MVP);
		for (int i = 0; i < triangles; i++)
		{
			float4 p1 = ObjectToClipPos(MVP, vertices[i*3+0]);
			float4 p2 = ObjectToClipPos(MVP, vertices[i*3+1]);
			float4 p3 = ObjectToClipPos(MVP, vertices[i*3+2]);
			float2 u1 = {uvs[i*3+0].x / p1.w, uvs[i*3+0].y / p1.w};
			float2 u2 = {uvs[i*3+1].x / p2.w, uvs[i*3+1].y / p2.w};
			float2 u3 = {uvs[i*3+2].x / p3.w, uvs[i*3+2].y / p3.w};	
			float2 screenPosA = {(p1.x/p1.w+1.0f)*0.5f*(float)ScreenWidth, (p1.y/p1.w+1.0f)*0.5f*(float)ScreenHeight};
			float2 screenPosB = {(p2.x/p2.w+1.0f)*0.5f*(float)ScreenWidth, (p2.y/p2.w+1.0f)*0.5f*(float)ScreenHeight};
			float2 screenPosC = {(p3.x/p3.w+1.0f)*0.5f*(float)ScreenWidth, (p3.y/p3.w+1.0f)*0.5f*(float)ScreenHeight};
			AABB box = BoundingBox ((int)screenPosA.x, (int)screenPosA.y, (int)screenPosB.x, (int)screenPosB.y, (int)screenPosC.x, (int)screenPosC.y);
			int minX = (int)Clamp(box.min.x, 1, ScreenWidth - 1); 
			int maxX = (int)Clamp(box.max.x, 1, ScreenWidth - 1);
			int minY = (int)Clamp(box.min.y, 1, ScreenHeight - 1);
			int maxY = (int)Clamp(box.max.y, 1, ScreenHeight - 1);
			for (int x = minX; x <= maxX; x++)
			{
				for (int y = minY; y <= maxY; y++)
				{
					float2 fragCoord = {(float)x, (float)y};
					float3 q = Barycentric(fragCoord, screenPosA, screenPosB, screenPosC);
					if (!((q.x >= 0.0) && (q.x <= 1.0) && (q.y >= 0.0) && (q.y <= 1.0) && (q.z >= 0.0) && (q.z <= 1.0))) continue;
					float z = 1.0f / (q.x * 1.0f / p1.w + q.y * 1.0f / p2.w + q.z * 1.0f / p3.w);
					float depth = (q.x * p1.w + q.y * p2.w + q.z * p3.w);
					if (depth < depthmap[y*ScreenWidth+x])
					{
						int index = y * ScreenWidth + x;
						depthmap[index] = depth;
						int texcoordX = (int)(((u1.x * q.x + u2.x * q.y + u3.x * q.z) * z) * texRes);
						int texcoordY = (int)(((u1.y * q.x + u2.y * q.y + u3.y * q.z) * z) * texRes);
						unsigned int id = texcoordY * texRes + texcoordX;
						if (texcoordX >= 0 && texcoordX <= texRes && texcoordY >= 0 && texcoordY <= texRes)
						{
							colormap[index] = texture[id];
						}
					}
				}
			}
		}
		StretchDIBits(hdc,0,0,ScreenWidth,ScreenHeight,0,0,ScreenWidth,ScreenHeight,colormap,&bmi,DIB_RGB_COLORS,SRCCOPY);
	}
	return 0;
}