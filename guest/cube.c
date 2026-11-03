#include <windows.h>
#include <stdio.h>
#include <math.h>
#ifdef USE_D3D8
#include <d3d8.h>
#define API "Direct3D 8"
typedef IDirect3D8 D3D; typedef IDirect3DDevice8 DEV;
#else
#include <d3d9.h>
#define API "Direct3D 9"
typedef IDirect3D9 D3D; typedef IDirect3DDevice9 DEV;
#endif
#define FVF (D3DFVF_XYZ|D3DFVF_DIFFUSE)
typedef struct {float x,y,z;DWORD c;} V;
static const V cube[8]={{-1,-1,-1,0xffff0000},{1,-1,-1,0xff00ff00},{1,1,-1,0xff0000ff},{-1,1,-1,0xffffff00},{-1,-1,1,0xffff00ff},{1,-1,1,0xff00ffff},{1,1,1,0xffffffff},{-1,1,1,0xff808080}};
static const WORD idx[36]={0,3,2,0,2,1,4,5,6,4,6,7,0,1,5,0,5,4,3,7,6,3,6,2,0,4,7,0,7,3,1,2,6,1,6,5};
static LRESULT CALLBACK wp(HWND h,UINT m,WPARAM w,LPARAM l){if(m==WM_DESTROY){PostQuitMessage(0);return 0;}return DefWindowProcA(h,m,w,l);}
static void id(D3DMATRIX*m){memset(m,0,sizeof *m);m->_11=m->_22=m->_33=m->_44=1;}
/* Usage: cube [-fs WIDTH HEIGHT] [-t SECONDS] */
int main(int ac,char**av){
 int fs=0,fw=0,fh=0,limit=0;
 for(int i=1;i<ac;i++){
  if(!strcmp(av[i],"-fs")&&i+2<ac){fs=1;fw=atoi(av[++i]);fh=atoi(av[++i]);}
  else if(!strcmp(av[i],"-t")&&i+1<ac)limit=atoi(av[++i]);
 }
 setvbuf(stdout,0,_IONBF,0);
 HMODULE dm=GetModuleHandleA(
#ifdef USE_D3D8
 "d3d8.dll"
#else
 "d3d9.dll"
#endif
 );char path[260]="?";GetModuleFileNameA(dm,path,260);printf("API      : %s\nDLL      : %s\n",API,path);
 WNDCLASSA wc={0};wc.lpfnWndProc=wp;wc.hInstance=GetModuleHandle(0);wc.lpszClassName="cube";wc.hCursor=LoadCursor(0,IDC_ARROW);RegisterClassA(&wc);
 HWND h=fs?CreateWindowA("cube",API " cube",WS_POPUP|WS_VISIBLE,0,0,fw,fh,0,0,wc.hInstance,0):CreateWindowA("cube",API " cube",WS_OVERLAPPEDWINDOW|WS_VISIBLE,40,40,800,600,0,0,wc.hInstance,0);
#ifdef USE_D3D8
 D3D*d=Direct3DCreate8(D3D_SDK_VERSION);
#else
 D3D*d=Direct3DCreate9(D3D_SDK_VERSION);
#endif
 if(!d){puts("FAILED: Direct3DCreate returned NULL");return 2;}
#ifdef USE_D3D8
 D3DADAPTER_IDENTIFIER8 ai;
 if(SUCCEEDED(d->lpVtbl->GetAdapterIdentifier(d,0,D3DENUM_NO_WHQL_LEVEL,&ai)))
#else
 D3DADAPTER_IDENTIFIER9 ai;
 if(SUCCEEDED(d->lpVtbl->GetAdapterIdentifier(d,0,0,&ai)))
#endif
  printf("Adapter  : %s\nDriver   : %s\n",ai.Description,ai.Driver);
 D3DDISPLAYMODE dm2;d->lpVtbl->GetAdapterDisplayMode(d,0,&dm2);
 D3DPRESENT_PARAMETERS pp;memset(&pp,0,sizeof pp);pp.Windowed=TRUE;pp.SwapEffect=D3DSWAPEFFECT_DISCARD;pp.BackBufferFormat=dm2.Format;pp.EnableAutoDepthStencil=TRUE;pp.AutoDepthStencilFormat=D3DFMT_D24S8;pp.hDeviceWindow=h;
 if(fs){pp.Windowed=FALSE;pp.BackBufferWidth=fw;pp.BackBufferHeight=fh;pp.BackBufferFormat=D3DFMT_X8R8G8B8;pp.BackBufferCount=1;ShowCursor(FALSE);}
 printf("Mode     : %s %dx%d\n",fs?"fullscreen":"windowed",fs?fw:800,fs?fh:600);
 DEV*v=0;HRESULT hr=d->lpVtbl->CreateDevice(d,0,D3DDEVTYPE_HAL,h,D3DCREATE_SOFTWARE_VERTEXPROCESSING,&pp,&v);
 printf("HAL device: hr=%08lx %s\n",(unsigned long)hr,SUCCEEDED(hr)?"OK":"FAILED");
 if(FAILED(hr))return 3;
 v->lpVtbl->SetRenderState(v,D3DRS_LIGHTING,FALSE);v->lpVtbl->SetRenderState(v,D3DRS_ZENABLE,TRUE);v->lpVtbl->SetRenderState(v,D3DRS_CULLMODE,D3DCULL_NONE);
#ifdef USE_D3D8
 v->lpVtbl->SetVertexShader(v,FVF);
#else
 v->lpVtbl->SetFVF(v,FVF);
#endif
 D3DMATRIX M;id(&M);M._43=6;v->lpVtbl->SetTransform(v,D3DTS_VIEW,&M);
 memset(&M,0,sizeof M);M._11=1.8f;M._22=2.4f;M._33=1.01f;M._34=1;M._43=-0.101f;v->lpVtbl->SetTransform(v,D3DTS_PROJECTION,&M);
 DWORD t0=GetTickCount(),st=t0;int frames=0,fails=0;MSG msg;
 for(;;){
  while(PeekMessageA(&msg,0,0,0,PM_REMOVE)){if(msg.message==WM_QUIT){v->lpVtbl->Release(v);d->lpVtbl->Release(d);printf("Exit OK, present failures: %d\n",fails);return 0;}TranslateMessage(&msg);DispatchMessageA(&msg);}
  float a=(GetTickCount()-st)*0.001f,c=cos(a),s=sin(a),c2=cos(a*.7f),s2=sin(a*.7f);
  id(&M);M._11=c;M._13=-s;M._21=s*s2;M._22=c2;M._23=c*s2;M._31=s*c2;M._32=-s2;M._33=c*c2;v->lpVtbl->SetTransform(v,D3DTS_WORLD,&M);
  v->lpVtbl->Clear(v,0,0,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,0xff102040,1.0f,0);
  v->lpVtbl->BeginScene(v);
#ifdef USE_D3D8
  v->lpVtbl->DrawIndexedPrimitiveUP(v,D3DPT_TRIANGLELIST,0,8,12,idx,D3DFMT_INDEX16,cube,sizeof(V));
#else
  v->lpVtbl->DrawIndexedPrimitiveUP(v,D3DPT_TRIANGLELIST,0,8,12,idx,D3DFMT_INDEX16,cube,sizeof(V));
#endif
  v->lpVtbl->EndScene(v);
  if(FAILED(v->lpVtbl->Present(v,0,0,0,0)))fails++;
  if(limit&&GetTickCount()-st>=(DWORD)limit*1000u)DestroyWindow(h);
  frames++;DWORD t=GetTickCount();if(t-t0>=5000){printf("%d frames in %.1f s = %.1f FPS\n",frames,(t-t0)/1000.f,frames*1000.f/(t-t0));t0=t;frames=0;}
 }
}
