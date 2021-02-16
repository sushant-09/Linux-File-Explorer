#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <iostream>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <bits/stdc++.h>
#define CURSORUP(x) printf("\033[%dA",x)
#define CURSORDOWN(x) printf("\033[%dB",x)
#define CURSORBACK(x) printf("\033[%dD",x)
#define CLS printf("\033[H\033[J")
#define PUTCURSER(x,y) printf("%c[%d;%dH", 27, x, y)
#define CLEARLINE printf("\033[2K")
using namespace std;
struct termios originalterm, newterm;
vector<char*> currentEntries;
vector<bool> isDir;
char root[PATH_MAX];
char* currentDir;
stack<char *> back_stack;
stack<char *> forward_stack;
string command;
struct winsize w;
int nor,noc,cursorline=0,epp=0,localCursor=0;
static int peek_char=-1;
void printType(struct stat filestatus)
{
    switch (filestatus.st_mode & S_IFMT)
    {
    case S_IFBLK:
        printf("b");
        break; //block file
    case S_IFCHR:
        printf("c");
        break; //device file
    case S_IFDIR:
        printf("d");
        break; //directory
    case S_IFIFO:
        printf("p");
        break; //FIFO file
    case S_IFLNK:
        printf("l");
        break; //link
    case S_IFREG:
        printf("-");
        break; //normal file
    case S_IFSOCK:
        printf("s");
        break; //socket file
    default:
        printf("-");
        break;
    }
}
void printPermissions(struct stat filestatus)
{
    printf((filestatus.st_mode & S_IRUSR) ? "r" : "-");
    printf((filestatus.st_mode & S_IWUSR) ? "w" : "-");
    printf((filestatus.st_mode & S_IXUSR) ? "x" : "-");
    printf((filestatus.st_mode & S_IRGRP) ? "r" : "-");
    printf((filestatus.st_mode & S_IWGRP) ? "w" : "-");
    printf((filestatus.st_mode & S_IXGRP) ? "x" : "-");
    printf((filestatus.st_mode & S_IROTH) ? "r" : "-");
    printf((filestatus.st_mode & S_IWOTH) ? "w" : "-");
    printf((filestatus.st_mode & S_IXOTH) ? "x" : "-"); 
}
void printSize(struct stat filestatus)
{
    double size = filestatus.st_size;
    int unitIndex=0;
    char units[]={'B','K','M','G','T','P','E','Z','Y'};
    while(size>1024.0)
    {
        size/=1024.0;
        unitIndex++;
    }
    printf("%8.2f %c",size,units[unitIndex]);
}
void exitNonCanonicalmode()
{
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &originalterm);
}
void enterNonCanonicalmode()
{
    atexit(exitNonCanonicalmode);
    tcgetattr(STDIN_FILENO, &originalterm);
    newterm = originalterm;
    newterm.c_lflag &= ~(ECHO | ICANON); // | ISIG | IEXTEN
    // newterm.c_iflag &= ~(BRKINT);
    // newterm.c_cc[VMIN] = 1;
    // newterm.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &newterm);
}
void printEntry(char* entryName)
    {
        char *c; int i; struct passwd *pw; struct group *gp;
        struct stat filestatus;
        stat(entryName,&filestatus);
        printf(" ");
        printType(filestatus);
        printf(" ");
        printPermissions(filestatus);
        printf(" ");
        pw=getpwuid(filestatus.st_uid);
        printf("%s\t",pw->pw_name);
        gp=getgrgid(filestatus.st_gid);
        printf("%s",gp->gr_name);
        printSize(filestatus);
        printf(" ");
        c=ctime(&filestatus.st_mtime);
        for(i=4;i<=15;i++)
            printf("%c",c[i]);
        printf(" ");
        if(!S_ISDIR(filestatus.st_mode))
            printf("%s\n",entryName);
        else
            printf("\033[0;33m%s\033[0m\n",entryName);
    }
string getPath(string path)
{
    string s;
    if(path==".")
    {
        s=currentDir;
    }
    else if(path[0]=='~')
    {
        s=root;
        string s1(path.begin()+1,path.end());
        s.append(s1);
    }
    else if(path[0]=='.')
    {
        s=currentDir;
        string s1(path.begin()+1,path.end());
        s.append(s1);
    }
    else
    {
        s=currentDir;
        s.append("/"+path);
    }
    return s;
}
void exploreDirectory(const char *dirPath) //populates the currentEntries and isDir vectors for the current directory
{
    DIR* dirPointer=opendir(dirPath);
    chdir(dirPath);
    struct dirent *dirEntry;
    struct stat filestatus;
    isDir.clear();
    currentEntries.clear();
    cursorline=0;localCursor=0;
    while ((dirEntry=readdir(dirPointer)) != 0)
    {
        currentEntries.push_back(dirEntry->d_name);
        stat(dirEntry->d_name,&filestatus);
        isDir.push_back(S_ISDIR(filestatus.st_mode));
    }
    sort(currentEntries.begin(),currentEntries.end());
}
void display(int start)
{
    CLS;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    nor=w.ws_row; noc=w.ws_col;
    printf("\033[3J");                                              //clear scrollback buffer
    printf("\033[%d;%dH\u001b[0m\u001b[7m NORMAL MODE \u001b[0m",nor,noc/2-6);
    PUTCURSER(0,0);
    epp=min((int)currentEntries.size(),nor-3);
    for(int i=start;i<start+epp;i++)
    {
        printEntry(currentEntries[i]);
    }
    PUTCURSER(0,0);
}
void enterKey()
{
    if(isDir[cursorline] && strcmp(realpath(currentEntries[cursorline],NULL),currentDir)!=0)
    {
        while(!forward_stack.empty())
            forward_stack.pop();
        currentDir=realpath(currentEntries[cursorline],NULL);
        back_stack.push(currentDir);
        exploreDirectory(currentDir);
        display(0);
    }
    else
    {
		pid_t pid=fork();
		if(pid==0)
        {
			execl("/usr/bin/xdg-open","xdg-open",realpath(currentEntries[cursorline],NULL),NULL);
			exit(1);
        }
    }
}
void backspaceKey()
{
    if(strcmp(currentDir,root)!=0)
    {
        while(!forward_stack.empty())
            forward_stack.pop();
        currentDir=realpath("..",NULL);
        back_stack.push(currentDir);
        exploreDirectory(currentDir);
        display(0);
    }
}
void homeKey()
{
    if(strcmp(currentDir,root)!=0)
    {
        while(!forward_stack.empty())
            forward_stack.pop();
        currentDir=root;
        back_stack.push(currentDir);
        exploreDirectory(currentDir);
        display(0);
    }
}
void backwardKey()
{
    if(back_stack.size()>1)
    {
        forward_stack.push(currentDir);
        back_stack.pop();
        currentDir=back_stack.top();
        exploreDirectory(currentDir);
        display(0);
    }
}
void forwardKey()
{
    if(!forward_stack.empty())
    {
        currentDir=forward_stack.top();
        forward_stack.pop();
        back_stack.push(currentDir);
        exploreDirectory(currentDir);
        display(0);
    }
}
void cursorUp()
{
    if(cursorline>0)
    {
        cursorline--;
        if(localCursor>0)
        {
            CURSORUP(1);
            localCursor--;
        }
        else
        {
            display(cursorline);
            PUTCURSER(0,0);
        }
    }
}
void cursorDown()
{
    if(cursorline<(int)currentEntries.size()-1)
    {
        cursorline++;
        if(localCursor<epp-1)
        {
            CURSORDOWN(1);
            localCursor++;
        }
        else
        {
            display(cursorline-epp+1);
            PUTCURSER(nor-3,0);
        }
    }
}
void pageDown()
{
    cursorline=cursorline+(epp-localCursor)>((int)currentEntries.size()-epp)?((int)currentEntries.size()-epp):cursorline+(epp-localCursor);
    localCursor=0;
    display(cursorline);
    PUTCURSER(0,0);
}
void pageUp()
{
    cursorline=cursorline-(epp+localCursor)<0?0:cursorline-(epp+localCursor);
    localCursor=0;
    display(cursorline);
    PUTCURSER(0,0);
}
vector<string> parseCommand()
{
    string s;
    vector<string> v;
    for(int i=0; i<command.length();i++)
    {
        if(command[i]==' ')
        {
            v.push_back(s);
            s.clear();
            continue;
        }
        s+=command[i];
    }
    v.push_back(s);
    command.clear();
    return v;
}

void gotoCommand(string path)
{
    while(!forward_stack.empty())
        forward_stack.pop();
    string s=getPath(path);
    char p[s.length()];
    strcpy(p,s.c_str());
    currentDir=p;
    back_stack.push(currentDir);
    exploreDirectory(currentDir);
    display(0);
    PUTCURSER(nor,0);
    CLEARLINE;
    cout<<"command mode: ";
}
void renameCommand(string old, string _new)
{
    string s1=getPath(old);
    string s2=getPath(_new);
    rename(s1.c_str(),s2.c_str());
    exploreDirectory(currentDir);
    display(0);
    PUTCURSER(nor,0);
    CLEARLINE;
    cout<<"command mode: ";
}
void create_dirCommand(string name, string path)
{
    string dfolder=getPath(path);
    string dpath=dfolder+"/"+name;
    mkdir(dpath.c_str(),0777);
}
void create_fileCommand(string name, string path)
{
    string dfolder=getPath(path);
    string dpath=dfolder+"/"+name;
    FILE* file=fopen(dpath.c_str(),"w+");
    fclose(file);
}
void copyFile(string sourcePath, string dpath)
{
    FILE *ff,*tf;
    char ch;
    if((ff=fopen(sourcePath.c_str(),"r"))==NULL)
        return;
    if((tf=fopen(dpath.c_str(),"w"))==NULL)
        return;
    while(!feof(ff))
    {
        ch=getc(ff);
        putc(ch,tf);
    }
    struct stat fs;
    stat(sourcePath.c_str(),&fs);
    chown(dpath.c_str(),fs.st_uid, fs.st_gid);
    chmod(dpath.c_str(), fs.st_mode);
    fclose(ff);
    fclose(tf);
}
void copyDirectory(string sourcePath, string dpath)
{
    DIR* dp;
    dp=opendir(sourcePath.c_str());
    struct dirent* d;
    while(d=readdir(dp))
    {
        if(strcmp(d->d_name,".")==0 || strcmp(d->d_name,"..")==0)
            continue;
        else
        {
            string recursiveSourcePath=sourcePath+"/"+string(d->d_name);
            string recursiveDPath=dpath+"/"+string(d->d_name);
            struct stat s;
            stat(sourcePath.c_str(),&s);
            if(S_ISDIR(s.st_mode))
            {
                mkdir(recursiveDPath.c_str(),0777);
                copyDirectory(recursiveSourcePath,recursiveDPath);
            }
            else
            {
                copyFile(recursiveSourcePath,recursiveDPath);
            }
        }
    }
}
void copyCommand(string source, string destination)
{
    string sourcePath=getPath(source);
    string dfolder=getPath(destination);
    size_t index=sourcePath.find_last_of("/\\");
    string dpath=dfolder+"/"+sourcePath.substr(index+1,sourcePath.length()-index);
    struct stat s;
    if(stat(sourcePath.c_str(),&s)==0)
    {
        if(S_ISDIR(s.st_mode))
        {
            copyDirectory(sourcePath,dpath);
        }
        else
        {
            copyFile(sourcePath,dpath);
        }
    }
}
void deleteFile(string path)
{
    string delpath=getPath(path);
    remove(delpath.c_str());
}
void deleteRecursively(string delpath)
{
    DIR* dp;
    dp=opendir(delpath.c_str());
    struct dirent* d;
    while(d=readdir(dp))
    {
        if (strcmp(d->d_name, ".") == 0 || strcmp(d->d_name, "..") == 0)
            continue;
        else
        {
            string recpath=delpath+"/"+string(d->d_name);
            struct stat s;
            stat(recpath.c_str(),&s);
            if(S_ISDIR(s.st_mode))
            {
                deleteRecursively(recpath);
            }
            else
            {
                remove(recpath.c_str());
            }
        }
    }
    rmdir(delpath.c_str());
    closedir(dp);
    return;
}
void deleteDir(string path)
{
    string delpath=getPath(path);
    deleteRecursively(delpath);
}
bool searchCommand(string file)
{
    DIR* dir;
    dirent* cur;
    dir=opendir(root);
    while((cur=readdir(dir))!=NULL)
    {
        string name=cur->d_name;
        if(name=="." || name=="..")
            continue;
        if(cur->d_type==DT_DIR)
        {
            if(name==file)
            {
                closedir(dir);
                return true;
            }
        }
    }
    return false;
}
void readKeyboardCommand()
{
    char key;
    while(1)
    {
        fflush(0);
        key=cin.get();
        if(key==27)
        {
            PUTCURSER(nor,0);
            CLEARLINE;
            printf("\033[%d;%dH\u001b[0m\u001b[7m NORMAL MODE \u001b[0m",nor,noc/2-6);
            display(cursorline-localCursor);
            PUTCURSER(localCursor+1,0);
            return;
        }
        else if(key==8 || key==127)
        {
            CURSORBACK(1);
            printf("\033[K");
            command=command.substr(0,command.length()-1);
        }
        else if(key==10)
        {
            vector<string> cmd=parseCommand();
            if(cmd[0]=="goto")
            {
                gotoCommand(cmd[1]);
            }
            else if(cmd[0]=="rename")
            {
                renameCommand(cmd[1],cmd[2]);
            }
            else if(cmd[0]=="create_dir")
            {
                int count=cmd.size();
                for(int i=1;i<count-1;i++)
                {
                    create_dirCommand(cmd[i],cmd[count-1]);
                }
                PUTCURSER(nor,0);
                CLEARLINE;
                cout<<"command mode: ";
            }
            else if(cmd[0]=="create_file")
            {
                int count=cmd.size();
                for(int i=1;i<count-1;i++)
                {
                    create_fileCommand(cmd[i],cmd[count-1]);
                }
                PUTCURSER(nor,0);
                CLEARLINE;
                cout<<"command mode: ";
            }
            else if(cmd[0]=="copy")
            {
                int count=cmd.size();
                for(int i=1;i<count-1;i++)
                {
                    copyCommand(cmd[i],cmd[count-1]);
                }
                PUTCURSER(nor,0);
                CLEARLINE;
                cout<<"command mode: ";
            }
            else if(cmd[0]=="delete_file")
            {
                deleteFile(cmd[1]);
                PUTCURSER(nor,0);
                CLEARLINE;
                cout<<"command mode: ";
            }
            else if(cmd[0]=="delete_dir")
            {
                deleteDir(cmd[1]);
                PUTCURSER(nor,0);
                CLEARLINE;
                cout<<"command mode: ";
            }
            else if(cmd[0]=="move")
            {
                int count=cmd.size();
                for(int i=1;i<count-1;i++)
                {
                    copyCommand(cmd[i],cmd[count-1]);
                    string p=getPath(cmd[i]);
                    struct stat s;
                    stat(p.c_str(),&s);
                    if(S_ISDIR(s.st_mode))
                    {
                        deleteDir(cmd[i]);
                    }
                    else
                    {
                        deleteFile(cmd[i]);
                    }
                }
                PUTCURSER(nor,0);
                CLEARLINE;
                cout<<"command mode: ";
            }
            else if(cmd[0]=="search")
            {
                PUTCURSER(nor,0);
                CLEARLINE;
                cout<<"command mode: ";
                if(searchCommand(cmd[1]))
                    printf("true");
            }
        }
        else
        {
            printf("%c",key);
            command+=key;
        }
    }
}
void commandMode()
{
    PUTCURSER(nor,0);
    CLEARLINE;
    cout<<"command mode: ";
    readKeyboardCommand();
}
void readKeyboardNormal()
{
    char key;
    while(1)
    {
        fflush(0);
        key=cin.get();
        if(key==10)
        {
            enterKey();
        }
        else if(key==127)
        {
            backspaceKey();
        }
        else if((key=='h' || key=='H'))
        {
            homeKey();
        }
        else if(key==27)
        { 
            cin.get();
            switch(cin.get())
            {
                case 'A':
                cursorUp();
                break;
                case 'B':
                cursorDown();
                break;
                case 'C':
                forwardKey();
                break;
                case 'D':
                backwardKey();
                break;
            }
        }
        else if(key==':')
        {
            commandMode();
        }
        else if(key=='i' || key=='I')
        {
            pageUp();
        }
        else if(key=='k' || key=='K')
        {
            pageDown();
        }
        if(key=='q' || key=='Q')
        {
            CLS;
            exit(0);
        }
    }
}
int main(int argc, char* argv[])
{
    getcwd(root,sizeof(root));
    currentDir=root;
    back_stack.push(currentDir);
    exploreDirectory(currentDir);
    display(0);
    enterNonCanonicalmode();
    readKeyboardNormal();
}

