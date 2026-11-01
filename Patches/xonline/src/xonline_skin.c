typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned int DWORD;
typedef int BOOL;
typedef void* HANDLE;
#define STDCALL __attribute__((stdcall))
#define THISCALL __attribute__((thiscall))
#define INVALID_HANDLE_VALUE ((HANDLE)-1)
#define GENERIC_READ 0x80000000
#define FILE_SHARE_READ 1
#define OPEN_EXISTING 3
#define FILE_ATTRIBUTE_NORMAL 0x80

#define ReadFile ((BOOL (STDCALL *)(HANDLE,void*,DWORD,DWORD*,void*))0x0007bb2d)
#define CloseHandle ((BOOL (STDCALL *)(HANDLE))0x0007b432)
#define CreateFileA ((HANDLE (STDCALL *)(const char*,DWORD,DWORD,void*,DWORD,DWORD,HANDLE))0x0007b476)
#define CreateDirectoryA ((BOOL (STDCALL *)(const char*,void*))0x0007d413)
#define IoCreateSymbolicLink (*(int (STDCALL **)(void*,void*))0x00012030)
#define IoDeleteSymbolicLink (*(int (STDCALL **)(void*))0x00012034)
#define RtlInitAnsiString (*(void (STDCALL **)(void*,const char*))0x00012038)

typedef struct
{
    WORD Length;
    WORD MaximumLength;
    char* Buffer;
} ANSI_STRING;

static char s_skin[64];
static char s_skinText[16384];
static char s_skinDrive;
static int s_loaded;
static int s_tMapped;
static int lower(int c) { return c >= 'A' && c <= 'Z' ? c + 32 : c; }
static int ieq_n(const char* a,const char* b,int n)
{
    for(int i=0;i<n;i++) if(!a[i] || lower(a[i])!=lower(b[i])) return 0;
    return 1;
}
static int ieq(const char* a,const char* b)
{
    int i=0;
    if(!a||!b)return 0;
    while(a[i]&&b[i]){if(lower(a[i])!=lower(b[i]))return 0;i++;}
    return a[i]==b[i];
}
static void ini_value(const char* text,const char* section,const char* key,char* out,int cap)
{
    int sl=0,in=0; while(section[sl]) sl++; out[0]=0;
    for(const char* p=text;*p;)
    {
        const char* line=p; while(*p&&*p!='\n')p++; const char* end=p; if(*p)p++;
        while(end>line&&(end[-1]=='\r'||end[-1]==' '||end[-1]=='\t'))end--;
        while(line<end&&(*line==' '||*line=='\t'))line++;
        if(line==end||*line==';'||*line=='#')continue;
        if(*line=='['){ in=(end-line>=sl+2&&line[sl+1]==']'&&ieq_n(line+1,section,sl)); continue; }
        int kl=0; while(key[kl])kl++;
        if(in&&end-line>kl&&ieq_n(line,key,kl))
        {
            const char* q=line+kl; while(q<end&&(*q==' '||*q=='\t'))q++;
            if(q<end&&*q=='='){ q++; while(q<end&&(*q==' '||*q=='\t'))q++; int i=0; while(q<end&&i<cap-1)out[i++]=*q++; out[i]=0; return; }
        }
    }
}
static int read_file(const char* path,char* dst,int cap)
{
    if(!s_tMapped)
    {
        ANSI_STRING link,target;
        RtlInitAnsiString(&link,"\\??\\T:");
        RtlInitAnsiString(&target,"\\Device\\Harddisk0\\Partition1\\TDATA\\fffe0000");
        IoDeleteSymbolicLink(&link);
        IoCreateSymbolicLink(&link,&target);
        CreateDirectoryA("T:\\UIXLite",0);
        CreateDirectoryA("T:\\UIXLite\\Skins",0);
        s_tMapped=1;
    }
    HANDLE h=CreateFileA(path,GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
    if(h==INVALID_HANDLE_VALUE)return 0;
    DWORD got=0; BOOL ok=ReadFile(h,dst,(DWORD)(cap-1),&got,0); CloseHandle(h);
    if(!ok)return 0; dst[got]=0; return (int)got;
}
static void ensure_skin(void)
{
    if(s_loaded)return;
    static char cfg[4096];
    if(!read_file("T:\\UIXLite\\config.ini",cfg,sizeof(cfg)))return;
    s_skinDrive='T';
    ini_value(cfg,"default","CurrentSkin",s_skin,sizeof(s_skin));
    if(!s_skin[0]){s_loaded=1;return;}
    if(s_skin[0])
    {
        char p[192]; int n=0;
        const char* pre="T:\\UIXLite\\Skins\\";
        for(int i=0;pre[i];i++)p[n++]=pre[i];
        p[0]=s_skinDrive;
        for(int i=0;s_skin[i]&&n<120;i++)p[n++]=s_skin[i]; p[n++]='\\';
        for(int i=0;s_skin[i]&&n<180;i++)p[n++]=s_skin[i];
        p[n++]='.';p[n++]='x';p[n++]='b';p[n++]='x';p[n]=0;
        if(!read_file(p,s_skinText,sizeof(s_skinText))){s_skinText[0]=0;return;}
    }
    s_loaded=1;
}

typedef int (STDCALL *LaunchLower)(const char*,void*,DWORD,DWORD,void*);
int STDCALL patch_xonline_return(const char* file,void* directory,DWORD mode,DWORD title,void* data)
{
    HANDLE h=CreateFileA("Y:\\xb0xdash.xbe",GENERIC_READ,FILE_SHARE_READ,0,
                         OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
    if(h!=INVALID_HANDLE_VALUE)
    {
        CloseHandle(h);
        file="xb0xdash.xbe";
    }
    return ((LaunchLower)0x0007b3c9)(file,directory,mode,title,data);
}

static int parse_rgba(const char* p,const char* end,DWORD* out)
{
    int v[4]={0,0,0,255},count=0; while(p<end&&*p!='=')p++; if(p<end)p++;
    while(p<end&&count<4){ if(*p>='0'&&*p<='9'){int n=0;while(p<end&&*p>='0'&&*p<='9')n=n*10+(*p++-'0');v[count++]=n&255;}else p++; }
    if(count<3)return 0; *out=((DWORD)v[3]<<24)|((DWORD)v[0]<<16)|((DWORD)v[1]<<8)|(DWORD)v[2]; return 1;
}
static int find_colors(const char* name,DWORD* a,DWORD* b)
{
    if(!s_skinText[0]||!name)return 0; int in=0,found=0,nl=0;while(name[nl])nl++;
    for(const char* p=s_skinText;*p;)
    {
        const char* l=p;while(*p&&*p!='\n')p++;const char* e=p;if(*p)p++;
        while(e>l&&(e[-1]=='\r'||e[-1]==' '||e[-1]=='\t'))e--;while(l<e&&(*l==' '||*l=='\t'))l++;
        if(l<e&&*l=='['){in=(e-l>=nl+2&&l[nl+1]==']'&&ieq_n(l+1,name,nl));continue;}
        if(!in)continue;
        if(e-l>=6&&ieq_n(l,"ColorA",6)){DWORD x;if(parse_rgba(l,e,&x)){*a=x;found=1;}}
        else if(e-l>=6&&ieq_n(l,"ColorB",6)){DWORD x;if(parse_rgba(l,e,&x)){*b=x;found=1;}}
        else if(e-l>=5&&ieq_n(l,"Color",5)){DWORD x;if(parse_rgba(l,e,&x)){*a=*b=x;found=1;}}
    }
    return found;
}
static int find_color(const char* name,DWORD* color)
{
    if(!s_skinText[0]||!name)return 0; int in=0,nl=0;while(name[nl])nl++;
    for(const char* p=s_skinText;*p;)
    {
        const char* l=p;while(*p&&*p!='\n')p++;const char* e=p;if(*p)p++;
        while(e>l&&(e[-1]=='\r'||e[-1]==' '||e[-1]=='\t'))e--;while(l<e&&(*l==' '||*l=='\t'))l++;
        if(l<e&&*l=='['){in=(e-l>=nl+2&&l[nl+1]==']'&&ieq_n(l+1,name,nl));continue;}
        if(in&&e-l>=5&&ieq_n(l,"Color",5)&&!(e-l>=6&&(l[5]=='A'||l[5]=='a'||l[5]=='B'||l[5]=='b')))
            return parse_rgba(l,e,color);
    }
    return 0;
}
static const char* skin_texture_path(const char* path,char* out,int cap)
{
    ensure_skin();
    if(!path)return path;
    const char* base=path;
    for(const char* q=path;*q;q++)if(*q=='\\'||*q=='/')base=q+1;
    if(!s_skin[0])return path;
    int n=0;const char* pre="T:\\UIXLite\\Skins\\";
    for(int i=0;pre[i]&&n<cap-1;i++)out[n++]=pre[i];
    for(int i=0;s_skin[i]&&n<cap-1;i++)out[n++]=s_skin[i];
    if(n<cap-1)out[n++]='\\';
    for(int i=0;base[i]&&n<cap-1;i++)out[n++]=base[i];
    out[n]=0;
    HANDLE h=CreateFileA(out,GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
    if(h==INVALID_HANDLE_VALUE)return path;
    CloseHandle(h);
    return out;
}
typedef int (THISCALL *XbxMethod)(void*,const char*);
int THISCALL patch_xonline_xbx_size(void* object,const char* path)
{
    char skinPath[192];
    return ((XbxMethod)0x000a0e2c)(object,skin_texture_path(path,skinPath,sizeof(skinPath)));
}
int THISCALL patch_xonline_xbx_open(void* object,const char* path)
{
    char skinPath[192];
    return ((XbxMethod)0x000a0efc)(object,skin_texture_path(path,skinPath,sizeof(skinPath)));
}

HANDLE STDCALL patch_xonline_createfile(const char* path,DWORD access,DWORD share,void* sa,DWORD create,DWORD flags,HANDLE tmpl)
{
    ensure_skin();
    if(path&&s_skin[0]&&ieq_n(path,"B:\\Media\\Xbx\\",13))
    {
        const char* base=path;for(const char* q=path;*q;q++)if(*q=='\\'||*q=='/')base=q+1;
        char p[192];int n=0;const char* pre="T:\\UIXLite\\Skins\\";
        for(int i=0;pre[i];i++)p[n++]=pre[i];for(int i=0;s_skin[i]&&n<120;i++)p[n++]=s_skin[i];p[n++]='\\';
        p[0]=s_skinDrive;
        for(int i=0;base[i]&&n<191;i++)p[n++]=base[i];p[n]=0;
        HANDLE h=CreateFileA(p,access,share,sa,create,flags,tmpl);
        if(h!=INVALID_HANDLE_VALUE)return h;
    }
    return CreateFileA(path,access,share,sa,create,flags,tmpl);
}

// XOnline's two-part HDTV setup. The first hook changes the device present
// parameters; the second changes the UI's cached floating-point dimensions.
// Both retain retail behavior unless HDTV + widescreen + 720p + NTSC are set.
__asm__(
    ".globl patch_xonline_video_device\n"
    "patch_xonline_video_device:\n"
    " pushl %esi\n"
    " movl $0x0007a9fa,%eax\n"
    " call *%eax\n"
    " cmpl $1,%eax\n"
    " jne 1f\n"
    " movl $0x0007a9b1,%eax\n"
    " call *%eax\n"
    " andl $3,%eax\n"
    " cmpl $3,%eax\n"
    " jne 1f\n"
    " movl $0x0007a988,%eax\n"
    " call *%eax\n"
    " cmpl $2,%eax\n"
    " je 0f\n"
    " cmpl $1,%eax\n"
    " jne 1f\n"
    "0:\n"
    " movl 0x14(%esp),%esi\n"
    " movl 0x28(%esi),%eax\n"
    " andl $0xffffffdf,%eax\n"
    " orl $0x40,%eax\n"
    " movl %eax,0x28(%esi)\n"
    " movl $1280,(%esi)\n"
    " movl $720,4(%esi)\n"
    " movl $0,0x10(%esi)\n"
    " movl $0,0x2c(%esi)\n"
    " movl $0,0x30(%esi)\n"
    "1:\n"
    " popl %esi\n"
    " movl $0x000e66e0,%eax\n"
    " call *%eax\n"
    " jmp 0x0004d72c\n");

__asm__(
    ".globl patch_xonline_video_ui\n"
    "patch_xonline_video_ui:\n"
    " movl $0x0007a9fa,%eax\n"
    " call *%eax\n"
    " cmpl $1,%eax\n"
    " jne 1f\n"
    " movl $0x0007a9b1,%eax\n"
    " call *%eax\n"
    " andl $3,%eax\n"
    " cmpl $3,%eax\n"
    " jne 1f\n"
    " movl $0x0007a988,%eax\n"
    " call *%eax\n"
    " cmpl $2,%eax\n"
    " je 0f\n"
    " cmpl $1,%eax\n"
    " jne 1f\n"
    "0:\n"
    " movl $0x44a00000,-4(%ebp)\n"
    " movl $0x44340000,-8(%ebp)\n"
    "1:\n"
    " pushl 0x001935c4\n"
    " jmp 0x00098ea8\n");

void THISCALL patch_xonline_colors(void* object,DWORD a,DWORD b)
{
    ensure_skin();
    const char* name=object?*(const char**)((BYTE*)object+4):0;
    find_colors(name,&a,&b);
    *(DWORD*)((BYTE*)object+0x54)=a;
    *(DWORD*)((BYTE*)object+0x58)=b;
}

void THISCALL patch_xonline_rgba(void* object,float r,float g,float b,float a)
{
    ensure_skin(); DWORD c;
    const char* name=object?*(const char**)((BYTE*)object+4):0;
    int found=find_color(name,&c);
    if(found)
    {
        r=(float)((c>>16)&255)/255.0f; g=(float)((c>>8)&255)/255.0f;
        b=(float)(c&255)/255.0f; a=(float)((c>>24)&255)/255.0f;
    }
    float* o=(float*)object;
    for(int base=2;base<=14;base+=4){o[base]=r;o[base+1]=g;o[base+2]=b;o[base+3]=a;}
}
